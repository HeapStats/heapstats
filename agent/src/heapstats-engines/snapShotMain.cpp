/*!
 * \file snapShotMain.cpp
 * \brief This file is used to take snapshot.
 * Copyright (C) 2011-2017 Nippon Telegraph and Telephone Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "globals.hpp"
#include "vmFunctions.hpp"
#include "elapsedTimer.hpp"
#include "util.hpp"
#include "callbackRegister.hpp"
#include "snapShotMain.hpp"

/* Struct defines. */

/*!
 * \brief This structure is stored snapshot and class heap usage.
 */
typedef struct {
  TSnapShotContainer *snapshot;  /*!< Container of taking snapshot. */
  TClassCounter *counter;        /*!< Counter of class heap usage.  */
} TCollectContainers;

/* Variable defines. */

/*!
 * \brief ClassData Container.
 */
TClassContainer *clsContainer = NULL;
/*!
 * \brief SnapShot Processor.
 */
TSnapShotProcessor *snapShotProcessor = NULL;
/*!
 * \brief GC Watcher.
 */
TGCWatcher *gcWatcher = NULL;
/*!
 * \brief Timer Thread.
 */
TTimer *timer = NULL;
/*!
 * \brief Pthread mutex for user data dump request.<br>
 *        E.g. continue pushing dump key.<br>
 * <br>
 * This mutex used in below process.<br>
 *   - OnDataDumpRequest @ snapShotMain.cpp<br>
 *     To avoid a lot of data dump request parallel processing.<br>
 */
pthread_mutex_t dumpMutex = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;

/*!
 * \brief Pthread mutex for JVMTI IterateOverHeap calling.<br>
 * <br>
 * This mutex used in below process.<br>
 *   - TakeSnapShot @ snapShotMain.cpp<br>
 *     To avoid taking double snapshot use JVMTI.<br>
 *     Because agent cann't distinguish called from where in hook function.<br>
 */
pthread_mutex_t jvmtiMutex = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;

/*!
 * \brief Snapshot container instance queue to waiting output.
 */
TSnapShotQueue snapStockQueue;
/*!
 * \brief Container instance stored got snapshot by GC.
 */
TSnapShotContainer *snapshotByGC = NULL;
/*!
 * \brief Container instance stored got snapshot by CMSGC.
 */
TSnapShotContainer *snapshotByCMS = NULL;
/*!
 * \brief Container instance stored got snapshot by interval or dump request.
 */
TSnapShotContainer *snapshotByJvmti = NULL;

/*!
 * \brief Index of JVM class unloading event.
 */
int classUnloadEventIdx = -1;

/* Function defines. */

/*!
 * \brief Count object size in Heap.
 * \param clsTag   [in]     Tag of object's class.
 * \param size     [in]     Object size.
 * \param objTag   [in,out] Object's tag.
 * \param userData [in,out] User's data.
 */
jvmtiIterationControl JNICALL HeapObjectCallBack(jlong clsTag, jlong size,
                                                 jlong *objTag,
                                                 void *userData) {
  /* This callback is dummy. */
  return JVMTI_ITERATION_ABORT;
}

/*!
 * \brief New class loaded event.
 * \param jvmti  [in] JVMTI environment object.
 * \param env    [in] JNI environment object.
 * \param thread [in] Java thread object.
 * \param klass  [in] Newly loaded class object.
 */
void JNICALL
    OnClassPrepare(jvmtiEnv *jvmti, JNIEnv *env, jthread thread, jclass klass) {
  /* Get klassOop. */
  void *mirror = *(void **)klass;
  void *klassOop = TVMFunctions::getInstance()->AsKlassOop(mirror);

  if (likely(klassOop != NULL)) {
    /* Push new loaded class. */
    clsContainer->pushNewClass(klassOop);
  }
}

/*!
 * \brief Setting JVM information to snapshot.
 * \param snapshot [in] Snapshot instance.
 * \param cause    [in] Cause of taking a snapshot.<br>
 *                      e.g. GC, DumpRequest or Interval.
 */
inline void setSnapShotInfo(TInvokeCause cause, TSnapShotContainer *snapshot) {
  /* Get now date and time. */
  struct timeval tv;
  gettimeofday(&tv, NULL);

  /* Set snapshot information. */
  snapshot->setSnapShotTime((jlong)tv.tv_sec * 1000 + (jlong)tv.tv_usec / 1000);
  snapshot->setSnapShotCause(cause);
  snapshot->setJvmInfo(jvmInfo);
}

/*!
 * \brief Add snapshot to outputing wait queue.
 * \param snapshot [in] Snapshot instance.
 * \return Process result.<br>
 *         Maybe failed to add snapshot to queue, if value is false.<br>
 *         So that, you should be additionally handling snapshot instance.
 */
inline bool addSnapShotQueue(TSnapShotContainer *snapshot) {
  snapStockQueue.push(snapshot);
  return true;
}

/*!
 * \brief Pop snapshot from wait queue for output.
 * \return Snapshot instance.<br>
 *         Maybe queue is empty, if value is NULL.
 */
inline TSnapShotContainer *popSnapShotQueue(void) {
  TSnapShotContainer *snapshot = NULL;
  bool succeeded = snapStockQueue.try_pop(snapshot);
  return succeeded ? snapshot : NULL;
}

/*!
 * \brief Notify to processor for output snapshot to file.
 * \param snapshot [in] Snapshot instance.
 * \warning After this function has called, Don't use a param "snapshot" again.
 */
inline void notifySnapShot(TSnapShotContainer *snapshot) {
  try {
    /* Sending notification means able to output file. */
    snapShotProcessor->notify(snapshot);
  } catch (...) {
    logger->printWarnMsg("Snapshot processeor notify failed!.");
    TSnapShotContainer::releaseInstance(snapshot);
  }
}

/*!
 * \brief Set information and push waiting queue.
 * \param snapshot [in] Snapshot instance.
 */
inline void outputSnapShotByGC(TSnapShotContainer *snapshot) {
  setSnapShotInfo(GC, snapshot);

  /* Standby for next GC. */
  jvmInfo->resumeGCinfo();

  /* If failed to push waiting queue. */
  if (unlikely(!addSnapShotQueue(snapshot))) {
    TSnapShotContainer::releaseInstance(snapshot);
  }

  /* Send notification. */
  gcWatcher->notify();
}

/*!
 * \brief Interrupt inner garbage collection event.
 *        Call this function when JVM invoke many times of GC process
 *        during single JVMTI event of GC start and GC finish.
 */
void onInnerGarbageCollectionInterrupt(void) {
  /* Standby for next GC. */
  jvmInfo->resumeGCinfo();

  /* Clear unfinished snapshot data. */
  snapshotByGC->clear(false);
}

/*!
 * \brief Before garbage collection event.
 * \param jvmti [in] JVMTI environment object.
 */
void JNICALL OnGarbageCollectionStart(jvmtiEnv *jvmti) {
  snapshotByGC = TSnapShotContainer::getInstance();

  /* Enable inner GC event. */
  setupHookForInnerGCEvent(true, &onInnerGarbageCollectionInterrupt);
}

/*!
 * \brief After garbage collection event.
 * \param jvmti [in] JVMTI environment object.
 */
void JNICALL OnGarbageCollectionFinish(jvmtiEnv *jvmti) {
  /* Disable inner GC event. */
  setupHookForInnerGCEvent(false, NULL);

  /* If need getting snapshot. */
  if (gcWatcher->needToStartGCTrigger()) {
    /* Set information and push waiting queue. */
    outputSnapShotByGC(snapshotByGC);
  } else {
    TSnapShotContainer::releaseInstance(snapshotByGC);
  }

  snapshotByGC = NULL;
}

/*!
 * \brief After G1 garbage collection event.
 */
void OnG1GarbageCollectionFinish(void) {
  // jvmInfo->loadGCCause();
  jvmInfo->SetUnknownGCCause();

  /* Set information and push waiting queue. */
  outputSnapShotByGC(snapshotByGC);
  snapshotByGC = TSnapShotContainer::getInstance();
}

/*!
 * \brief Get class information.
 * \param klassOop      [in] Pointer of child java class object(KlassOopDesc).
 * \return Pointer of information of expceted class by klassOop.
 */
inline TObjectData *getObjectDataFromKlassOop(void *klassOop) {
  TObjectData *clsData = clsContainer->findClass(klassOop);
  if (unlikely(clsData == NULL)) {
    /* Push new loaded class to root class container. */
    clsData = clsContainer->pushNewClass(klassOop);
  }

  return clsData;
}

/*!
 * \brief Iterate oop field object callback for GC and JVMTI snapshot.
 * \param oop  [in] Java heap object(Inner class format).
 * \param data [in] User expected data.
 */
void iterateFieldObjectCallBack(void *oop, void *data) {
  TCollectContainers *containerInfo = (TCollectContainers *)data;
  void *klassOop = getKlassOopFromOop(oop);
  /* Sanity check. */
  if (unlikely(klassOop == NULL || containerInfo == NULL)) {
    return;
  }

  TSnapShotContainer *snapshot = containerInfo->snapshot;
  TClassCounter *parentCounter = containerInfo->counter;

  TChildClassCounter *clsCounter = NULL;

  /* Search child class. */
  clsCounter = snapshot->findChildClass(parentCounter, klassOop);

  if (unlikely(clsCounter == NULL)) {
    /* Get child class information. */
    TObjectData *clsData = getObjectDataFromKlassOop(klassOop);
    /* Push new child loaded class. */
    clsCounter = snapshot->pushNewChildClass(parentCounter, clsData);
  }

  if (unlikely(clsCounter == NULL)) {
    logger->printCritMsg("Couldn't get class counter!");
    return;
  }

  TVMFunctions *vmFunc = TVMFunctions::getInstance();
  jlong size = 0;
  if (clsCounter->objData->oopType == otInstance) {
    if (likely(clsCounter->objData->instanceSize != 0)) {
      size = clsCounter->objData->instanceSize;
    } else {
      vmFunc->GetObjectSize(NULL, (jobject)&oop, &size);
      clsCounter->objData->instanceSize = size;
    }

  } else {
    vmFunc->GetObjectSize(NULL, (jobject)&oop, &size);
  }

  /* Count perent class size and instance count. */
  snapshot->FastInc(clsCounter->counter, size);
}

/*!
 * \brief Calculate size of object and iterate child-class in heap.
 * \param snapshot [in] Snapshot instance.
 * \param oop      [in] Java heap object(Inner class format).
 */
inline void calculateObjectUsage(TSnapShotContainer *snapshot, void *oop) {
  void *klassOop = getKlassOopFromOop(oop);
  /* Sanity check. */
  if (unlikely(snapshot == NULL || klassOop == NULL)) {
    return;
  }

  snapshot->setIsCleared(false);

  TClassCounter *clsCounter = NULL;
  TObjectData *clsData = NULL;

  /* Get class information. */
  clsData = getObjectDataFromKlassOop(klassOop);
  if (unlikely(clsData == NULL)) {
    logger->printCritMsg("Couldn't get ObjectData!");
    return;
  }

  /* Search class. */
  clsCounter = snapshot->findClass(clsData);
  if (unlikely(clsCounter == NULL)) {
    /* Push new loaded class. */
    clsCounter = snapshot->pushNewClass(clsData);
  }

  if (unlikely(clsCounter == NULL)) {
    logger->printCritMsg("Couldn't get class counter!");
    return;
  }

  TVMFunctions *vmFunc = TVMFunctions::getInstance();
  TOopType oopType = clsData->oopType;
  jlong size = 0;

  if (oopType == otInstance) {
    if (likely(clsData->instanceSize != 0)) {
      size = clsData->instanceSize;
    } else {
      vmFunc->GetObjectSize(NULL, (jobject)&oop, &size);
      clsData->instanceSize = size;
    }

  } else {
    vmFunc->GetObjectSize(NULL, (jobject)&oop, &size);
  }

  /* Count perent class size and instance count. */
  snapshot->FastInc(clsCounter->counter, size);

  /* If we should not collect reftree or oop has no field. */
  if (!conf->CollectRefTree()->get() || !hasOopField(oopType)) {
    return;
  }

  TCollectContainers containerInfo;
  containerInfo.snapshot = snapshot;
  containerInfo.counter = clsCounter;

  TOopMapBlock *offsets = NULL;
  int offsetCount = 0;

  offsets = clsCounter->offsets;
  offsetCount = clsCounter->offsetCount;

  /* If offset list is unused yet. */
  if (unlikely(offsets == NULL && offsetCount < 0)) {
    /* Generate offset list. */
    generateIterateFieldOffsets(klassOop, oopType, &offsets, &offsetCount);
    clsCounter->offsets = offsets;
    clsCounter->offsetCount = offsetCount;
  }

  /* Iterate non-static field objects. */
  iterateFieldObject(&iterateFieldObjectCallBack, oop, oopType, &offsets,
                     &offsetCount, &containerInfo);
}

/*!
 * \brief Count object size in heap by GC.
 * \param oop  [in] Java heap object(Inner class format).
 * \param data [in] User expected data. Always this value is NULL.
 */
void HeapObjectCallbackOnGC(void *oop, void *data) {
  /* Calculate and merge to GC snapshot. */
  calculateObjectUsage(snapshotByGC, oop);
}

/*!
 * \brief Count object size in heap by CMSGC.
 * \param oop  [in] Java heap object(Inner class format).
 * \param data [in] User expected data. Always this value is NULL.
 */
void HeapObjectCallbackOnCMS(void *oop, void *data) {
  /* Calculate and merge to CMSGC snapshot. */
  calculateObjectUsage(snapshotByCMS, oop);
}

/*!
 * \brief Count object size in heap by JVMTI iterateOverHeap.
 * \param oop  [in] Java heap object(Inner class format).
 * \param data [in] User expected data. Always this value is NULL.
 */
void HeapObjectCallbackOnJvmti(void *oop, void *data) {
  /* Calculate and merge to JVMTI snapshot. */
  calculateObjectUsage(snapshotByJvmti, oop);
}

/*!
 * \brief This function is for class oop adjust callback by GC.
 * \param oldOop [in] Old pointer of java class object(KlassOopDesc).
 * \param newOop [in] New pointer of java class object(KlassOopDesc).
 */
void HeapKlassAdjustCallback(void *oldOop, void *newOop) {
  /* Class information update. */
  clsContainer->updateClass(oldOop, newOop);
}

/*!
 * \brief Event of before garbage collection by CMS collector.
 * \param jvmti [in] JVMTI environment object.
 */
void JNICALL OnCMSGCStart(jvmtiEnv *jvmti) {
  /* Get CMS state. */
  bool needShapShot = false;
  int cmsState = checkCMSState(gcStart, &needShapShot);

  /* If occurred snapshot target GC. */
  if (needShapShot) {
    /* If need getting snapshot. */
    if (gcWatcher->needToStartGCTrigger()) {
      /* Set information and push waiting queue. */
      outputSnapShotByGC(snapshotByCMS);
      snapshotByCMS = NULL;
    }
  }

  if (likely(snapshotByGC == NULL)) {
    snapshotByGC = TSnapShotContainer::getInstance();
  } else {
    snapshotByGC->clear(false);
  }

  if (likely(snapshotByCMS == NULL)) {
    snapshotByCMS = TSnapShotContainer::getInstance();
  } else if (cmsState == CMS_FINALMARKING) {
    snapshotByCMS->clear(false);
  }

  /* Enable inner GC event. */
  setupHookForInnerGCEvent(true, &onInnerGarbageCollectionInterrupt);
}

/*!
 * \brief Event of after garbage collection by CMS collector.
 * \param jvmti [in] JVMTI environment object.
 */
void JNICALL OnCMSGCFinish(jvmtiEnv *jvmti) {
  /* Disable inner GC event. */
  setupHookForInnerGCEvent(false, NULL);

  /* Get CMS state. */
  bool needShapShot = false;
  checkCMSState(gcFinish, &needShapShot);

  /* If occurred snapshot target GC. */
  if (needShapShot) {
    /* If need getting snapshot. */
    if (gcWatcher->needToStartGCTrigger()) {
      /* Set information and push waiting queue. */
      outputSnapShotByGC(snapshotByGC);
      snapshotByGC = NULL;
      snapshotByCMS->clear(false);
    }
  }
}

/*!
 * \brief Data dump request event for snapshot.
 * \param jvmti [in] JVMTI environment object.
 */
void JNICALL OnDataDumpRequestForSnapShot(jvmtiEnv *jvmti) {
  /* Avoid the plural simultaneous take snapshot by dump-request.        */
  /* E.g. keeping pushed dump key.                                       */
  /* Because classContainer register a redundancy class in TakeSnapShot. */
  ENTER_PTHREAD_SECTION(&dumpMutex) {
    /* Make snapshot. */
    TakeSnapShot(jvmti, NULL, DataDumpRequest);
  }
  EXIT_PTHREAD_SECTION(&dumpMutex)
}

/*!
 * \brief Take a heap information snapshot.
 * \param jvmti [in] JVMTI environment object.
 * \param env   [in] JNI environment object.
 * \param cause [in] Cause of taking a snapshot.<br>
 *                   e.g. GC, DumpRequest or Interval.
 */
void TakeSnapShot(jvmtiEnv *jvmti, JNIEnv *env, TInvokeCause cause) {
  TVMVariables *vmVal = TVMVariables::getInstance();

  /*
   * Initial Mark is 1st phase in CMS.
   * So we can process the SnapShot in SnapShot queue.
   */
  if (vmVal->getUseCMS() &&
      (vmVal->getCMS_collectorState() > CMS_INITIALMARKING)) {
    logger->printWarnMsg("CMS GC is working. Skip to take a SnapShot.");
    TSnapShotContainer *snapshot = popSnapShotQueue();

    if (likely(snapshot != NULL)) {
      TSnapShotContainer::releaseInstance(snapshot);
    }

    return;
  }

  /* Count working time. */
  static const char *label = "Take SnapShot";
  TElapsedTimer elapsedTime(label);

  /* Phase1: Heap Walking. */
  jvmtiError error = JVMTI_ERROR_INTERNAL;
  if (cause != GC) {
    TSnapShotContainer *snapshot = TSnapShotContainer::getInstance();

    if (likely(snapshot != NULL)) {
      /* Lock to avoid doubling call JVMTI. */
      ENTER_PTHREAD_SECTION(&jvmtiMutex) {
        snapshotByJvmti = snapshot;

        /* Enable JVMTI hooking. */
        if (likely(setJvmtiHookState(true))) {
          /* Count object size on heap. */
          error = jvmti->IterateOverHeap(JVMTI_HEAP_OBJECT_EITHER,
                                         &HeapObjectCallBack, NULL);

          /* Disable JVMTI hooking. */
          setJvmtiHookState(false);
        }

        snapshotByJvmti = NULL;
      }
      EXIT_PTHREAD_SECTION(&jvmtiMutex)
    }

    if (likely(error == JVMTI_ERROR_NONE)) {
      setSnapShotInfo(cause, snapshot);

      /* If failed to push waiting queue. */
      if (unlikely(!addSnapShotQueue(snapshot))) {
        error = JVMTI_ERROR_OUT_OF_MEMORY;
      }
    }

    if (unlikely(error != JVMTI_ERROR_NONE)) {
      /* Process failure. So data is junk. */
      TSnapShotContainer::releaseInstance(snapshot);
    }
  } else {
    /*
     * If cause is "GC", then we already collect heap object to snapshot
     * by override functions exist in "overrideBody.S".
     */
    error = JVMTI_ERROR_NONE;
  }

  /* Phase2: Output snapshot. */

  /* If failure count object size. */
  if (unlikely(isError(jvmti, error))) {
    logger->printWarnMsg("Heap snapshot failed!");
  } else {
    /* Pop snapshot. */
    TSnapShotContainer *snapshot = popSnapShotQueue();

    if (likely(snapshot != NULL)) {
      /* Set total memory. */
      snapshot->setTotalSize(jvmInfo->getTotalMemory());

      /* Notify to processor. */
      notifySnapShot(snapshot);
    }
  }

  /* Phase3: Reset Timer. */
  if (conf->TimerInterval()->get() > 0) {
    /* Sending notification means reset timer. */
    timer->notify();
  }
}

/*!
 * \brief Setting enable of JVMTI and extension events for snapshot function.
 * \param jvmti  [in] JVMTI environment object.
 * \param enable [in] Event notification is enable.
 * \return Setting process result.
 */
jint setEventEnableForSnapShot(jvmtiEnv *jvmti, bool enable) {
  jvmtiEventMode mode = enable ? JVMTI_ENABLE : JVMTI_DISABLE;

  /* Switch date dump event. */
  if (conf->TriggerOnDump()->get()) {
    TDataDumpRequestCallback::switchEventNotification(jvmti, mode);
  }

  /* Switch gc events. */
  if (conf->TriggerOnFullGC()->get() &&
      !TVMVariables::getInstance()->getUseG1()) {
    /* Switch JVMTI GC events. */
    TGarbageCollectionStartCallback::switchEventNotification(jvmti, mode);
    TGarbageCollectionFinishCallback::switchEventNotification(jvmti, mode);
  }

  return SUCCESS;
}

/*!
 * \brief Setting enable of agent each threads for snapshot function.
 * \param jvmti  [in] JVMTI environment object.
 * \param env    [in] JNI environment object.
 * \param enable [in] Event notification is enable.
 */
void setThreadEnableForSnapShot(jvmtiEnv *jvmti, JNIEnv *env, bool enable) {
  /* Start or suspend HeapStats agent threads. */
  try {
    /* Switch GC watcher state. */
    if (conf->TriggerOnFullGC()->get()) {
      if (enable) {
        gcWatcher->start(jvmti, env);
      } else {
        gcWatcher->stop();
      }

      if (TVMVariables::getInstance()->getUseG1()) {
        if (snapshotByGC == NULL) {
          snapshotByGC = TSnapShotContainer::getInstance();
        } else {
          snapshotByGC->clear(false);
        }
      }

      /* Switch GC hooking state. */
      setGCHookState(enable);
    }

    /* Switch interval snapshot timer state. */
    if (conf->TimerInterval()->get() > 0) {
      if (enable) {
        /* MS to Sec */
        timer->start(jvmti, env, conf->TimerInterval()->get() * 1000);
      } else {
        timer->stop();
      }
    }

    /* Switch snapshot processor state. */
    if (enable) {
      snapShotProcessor->start(jvmti, env);
    } else {
      snapShotProcessor->stop();
    }
  } catch (const char *errMsg) {
    logger->printWarnMsg(errMsg);
  }
}

/*!
 * \brief Clear current SnapShot.
 */
void clearCurrentSnapShot() {
  snapshotByGC->clear(false);
}

/*!
 * \brief JVM initialization event for snapshot function.
 * \param jvmti [in] JVMTI environment object.
 * \param env   [in] JNI environment object.
 */
void onVMInitForSnapShot(jvmtiEnv *jvmti, JNIEnv *env) {
  size_t maxMemSize = jvmInfo->getMaxMemory();
  /* Setup for hooking. */
  setupHook(&HeapObjectCallbackOnGC, &HeapObjectCallbackOnCMS,
            &HeapObjectCallbackOnJvmti, &HeapKlassAdjustCallback,
            &OnG1GarbageCollectionFinish, maxMemSize);

  /* JVMTI Extension Event Setup. */
  int eventIdx = GetClassUnloadingExtEventIndex(jvmti);

  /*
   * JVMTI extension event is influenced by JVM's implementation.
   * Extension event may no exist , if JVM was a little updated.
   * This is JVMTI's Specifications.
   * So result is disregard, even if failure processed extension event.
   */
  if (eventIdx < 0) {
    /* Miss get extension event list. */
    logger->printWarnMsg("Couldn't get ClassUnload event.");
  } else {
    /* if failed to set onClassUnload event. */
    if (isError(jvmti, jvmti->SetExtensionEventCallback(
                           eventIdx, (jvmtiExtensionEvent)&OnClassUnload))) {
      /* Failure setting extension event. */
      logger->printWarnMsg("Couldn't register ClassUnload event.");
    } else {
      classUnloadEventIdx = eventIdx;
    }
  }
}

/*!
 * \brief JVM finalization event for snapshot function.
 * \param jvmti [in] JVMTI environment object.
 * \param env   [in] JNI environment object.
 */
void onVMDeathForSnapShot(jvmtiEnv *jvmti, JNIEnv *env) {
  if (TVMVariables::getInstance()->getUseCMS()) {
    /* Disable inner GC event. */
    setupHookForInnerGCEvent(false, NULL);

    /* Get CMS state. */
    bool needShapShot = false;
    checkCMSState(gcLast, &needShapShot);

    /* If occurred snapshot target GC. */
    if (needShapShot && snapshotByCMS != NULL) {
      /* Output snapshot. */
      outputSnapShotByGC(snapshotByCMS);
      snapshotByCMS = NULL;
    }
  }

  TSnapShotContainer *snapshot = popSnapShotQueue();
  /* Output all waiting snapshot. */
  while (snapshot != NULL) {
    snapshot->setTotalSize(jvmInfo->getTotalMemory());
    notifySnapShot(snapshot);
    snapshot = popSnapShotQueue();
  }

  /* Disable class prepare/unload events. */
  TClassPrepareCallback::switchEventNotification(jvmti, JVMTI_DISABLE);

  if (likely(classUnloadEventIdx >= 0)) {
    jvmti->SetExtensionEventCallback(classUnloadEventIdx, NULL);
  }
}

/*!
 * \brief Agent initialization for snapshot function.
 * \param jvmti [in] JVMTI environment object.
 * \return Initialize process result.
 */
jint onAgentInitForSnapShot(jvmtiEnv *jvmti) {
  /* Initialize oop util. */
  if (unlikely(!oopUtilInitialize(jvmti))) {
    logger->printCritMsg(
        "Please check installation and version of java and debuginfo "
        "packages.");
    return GET_LOW_LEVEL_INFO_FAILED;
  }

  /* Initialize snapshot containers. */
  if (unlikely(!TSnapShotContainer::globalInitialize())) {
    logger->printCritMsg("TSnapshotContainer initialize failed!");
    return CLASSCONTAINER_INITIALIZE_FAILED;
  }

  /* Initialize TClassContainer. */
  try {
    clsContainer = new TClassContainer();
  } catch (...) {
    logger->printCritMsg("TClassContainer initialize failed!");
    return CLASSCONTAINER_INITIALIZE_FAILED;
  }

  /* Create thread instances that controlled snapshot trigger. */
  try {
    gcWatcher = new TGCWatcher(&TakeSnapShot, jvmInfo);

    snapShotProcessor = new TSnapShotProcessor(clsContainer, jvmInfo);

    timer = new TTimer(&TakeSnapShot, "HeapStats Snapshot Timer");

  } catch (const char *errMsg) {
    logger->printCritMsg(errMsg);
    return AGENT_THREAD_INITIALIZE_FAILED;
  } catch (...) {
    logger->printCritMsg("AgentThread initialize failed!");
    return AGENT_THREAD_INITIALIZE_FAILED;
  }

  return SUCCESS;
}

/*!
 * \brief Agent finalization for snapshot function.
 * \param env [in] JNI environment object.
 */
void onAgentFinalForSnapShot(JNIEnv *env) {
  /* Destroy SnapShot Processor instance. */
  delete snapShotProcessor;
  snapShotProcessor = NULL;

  /*
   * Delete snapshot instances
   */
  if (snapshotByCMS != NULL) {
    TSnapShotContainer::releaseInstance(snapshotByCMS);
  }
  if (snapshotByGC != NULL) {
    TSnapShotContainer::releaseInstance(snapshotByGC);
  }

  /* Finalize and deallocate old snapshot containers. */
  TSnapShotContainer::globalFinalize();

  /* Destroy object that is for snapshot. */
  delete clsContainer;
  clsContainer = NULL;

  /* Destroy object that is each snapshot trigger. */
  delete gcWatcher;
  gcWatcher = NULL;

  delete timer;
  timer = NULL;

  /* Finalize oop util. */
  oopUtilFinalize();
}

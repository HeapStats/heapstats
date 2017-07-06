/*!
 * \file deadlockFinder.cpp
 * \brief This file is used by find deadlock.
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

#include <sys/syscall.h>

#include "globals.hpp"
#include "vmFunctions.hpp"
#include "vmVariables.hpp"
#include "libmain.hpp"
#include "deadlockFinder.hpp"

/* Defines. */

/*!
 * \brief Thread state macro.<br>
 *        This value means the thread is existing on native code.<br>
 *        Definition of "enum JavaThreadState".
 *        (_thread_in_native state is used in JVMTI:JvmtiThreadEventTransition)
 * \sa hotspot/src/share/vm/utilities/globalDefinitions.hpp
 */
#define THREAD_IN_NATIVE 4

/*!
 * \brief Thread state macro.<br>
 *        This value means the thread is existing on VM.<br>
 *        Definition of "enum JavaThreadState".
 * \sa hotspot/src/share/vm/utilities/globalDefinitions.hpp
 */
#define THREAD_IN_VM 6
/*!
 * \brief Thread state macro.<br>
 *        This value means the thread is existing on Java.<br>
 *        Definition of "enum JavaThreadState".
 * \sa hotspot/src/share/vm/utilities/globalDefinitions.hpp
 */
#define THREAD_IN_JAVA 8


/* Class static variables. */

/*!
 * \brief Singleton instance of TDeadlockFinder.
 */
TDeadlockFinder *TDeadlockFinder::inst = NULL;


/*!
 * \brief Event handler of JVMTI MonitorContendedEnter for finding deadlock.
 * \param jvmti  [in] JVMTI environment.
 * \param env    [in] JNI environment of the event (current) thread.
 * \param thread [in] JNI local reference to the thread attempting to enter
 *                    the monitor.
 * \param object [in] JNI local reference to the monitor.
 * \warning Sometimes this callback is called synchronizing or synchronized
 *          at safepoint.<br>Please be carefully calling inner function of JVM.
 */
void JNICALL OnMonitorContendedEnterForDeadlock(jvmtiEnv *jvmti, JNIEnv *env,
                                                jthread thread,
                                                jobject object) {
  /* Check deadlock. */
  TDeadlockList *list = NULL;
  int threadCnt = 0;
  char threadName[256] = {0};
  threadCnt = TDeadlockFinder::checkDeadlock(object, &list);

  /* Deadlock is not occurred. */
  if (likely(threadCnt == 0)) {
    return;
  }

  /* Get thread name. */
  {
    jvmtiThreadInfo threadInfo = {0};
    jvmti->GetThreadInfo(thread, &threadInfo);
    strncpy(threadName, threadInfo.name, 255);

    /* Cleanup. */
    jvmti->Deallocate((unsigned char *)threadInfo.name);
    env->DeleteLocalRef(threadInfo.thread_group);
    env->DeleteLocalRef(threadInfo.context_class_loader);
  }

  /* Raise alert. */
  logger->printCritMsg(
      "ALERT(DEADLOCK): occurred deadlock. threadCount: %d, threadName: \"%s\"",
      threadCnt, threadName);

  /* Print deadlock threads. */
  for (TDeadlockList *item = list; item != NULL; item = item->next) {
    /* Get thread information. */
    TJavaThreadInfo threadInfo = {0};
    getThreadDetailInfo(jvmti, env, item->thread, &threadInfo);

    logger->printWarnMsg("thread name: %s, prio: %d", threadInfo.name,
                         threadInfo.priority);

    /* Cleanup. */
    free(threadInfo.name);
    free(threadInfo.state);
  }

  /* Cleanup deadlock thread list. */
  TDeadlockFinder::freeDeadlockList(list);

  /* Get now date and time. */
  TMSecTime nowTime = (TMSecTime)getNowTimeSec();

  if (conf->SnmpSend()->get()) {
    TDeadlockFinder::inst->sendSNMPTrap(nowTime, threadCnt, threadName);
  }

  if (conf->TriggerOnLogLock()->get()) {
    try {
      /* Invoke log collection. */
      TDeadlockFinder::inst->notify(nowTime);
    } catch (...) {
      logger->printWarnMsg("Log collection (deadlock) failed.");
    }
  } else if (unlikely(conf->KillOnError()->get())) {
    /*
     * Abort JVM process if kill_on_error is enabled.
     * If trigger_on_loglock is enabled, this code is skipped because we give
     * a chance to collect logs. abort() should be called by logging process.
     */
    forcedAbortJVM(jvmti, env, "deadlock occurred");
  }
}

/* Class methods. */

/*!
 * \brief Global initialization.
 * \return Process result.
 * \param event [in] Callback is used on deadlock occurred.
 * \warning Please call only once from main thread.
 */
bool TDeadlockFinder::globalInitialize(TDeadlockEventFunc event) {
  /* Variable for return code. */
  bool result = true;

  try {
    inst = new TDeadlockFinder(event);
  } catch (...) {
    logger->printCritMsg("Cannot initialize TDeadlockFinder.");
    result = false;
  }

  return result;
}

/*!
 * \brief Global finalization of deadlock finder.
 */
void TDeadlockFinder::globalFinalize(void) {
  delete inst;
  inst = NULL;
}

/*!
 * \brief Set JVMTI functional capabilities.
 * \param capabilities [out] JVMTI capabilities to add.
 * \param isOnLoad     [in]  OnLoad phase or not (Live phase).
 */
void TDeadlockFinder::setCapabilities(jvmtiCapabilities *capabilities,
                                      bool isOnLoad) {
  /* Set capabilities for monitor events. */
  if (conf->CheckDeadlock()->get()) {
    capabilities->can_generate_monitor_events = 1;
  }

  /* Set capabilities for thread dump which is made by JVMTI function. */
  capabilities->can_get_source_file_name = 1;
  capabilities->can_get_line_numbers = 1;
  capabilities->can_get_monitor_info = 1;

  /*
   * can_get_owned_monitor_stack_depth_info and
   * can_get_current_contended_monitor must be set at OnLoad phase.
   *
   * See also:
   *   hotspot/src/share/vm/prims/jvmtiManageCapabilities.cpp
   */
  if (isOnLoad) {
    capabilities->can_get_owned_monitor_stack_depth_info = 1;
    capabilities->can_get_current_contended_monitor = 1;
  }
}

/*!
* \brief TDeadlockFinder constructor.
* \param event [in] Callback is used on deadlock occurred.
*/
TDeadlockFinder::TDeadlockFinder(TDeadlockEventFunc event)
    : TAgentThread("HeapStats Deadlock Finder"), occurTime(0) {
  /* Sanity check. */
  if (unlikely(event == NULL)) {
    throw "Event callback is NULL.";
  }
  callFunc = event;
}

/*!
 * \brief TDeadlockFinder destructor.
 */
TDeadlockFinder::~TDeadlockFinder(void) { /* none. */ }

/*!
 * \brief Send SNMP trap which contains deadlock information.
 * \param nowTime The time of deadlock occurred.
 * \param threadCnt Number of threads which are related to deadlock.
 * \param name Thread name of deadlock occurred.
 */
void TDeadlockFinder::sendSNMPTrap(TMSecTime nowTime,
                                        int threadCnt, const char *name) {
  /* Trap OID. */
  char trapOID[50] = OID_DEADLOCKALERT;
  oid OID_ALERT_DATE[] = {SNMP_OID_HEAPALERT, 1};
  oid OID_DEAD_COUNT[] = {SNMP_OID_DEADLOCKALERT, 1};
  oid OID_DEAD_NAME[] = {SNMP_OID_DEADLOCKALERT, 2};
  char buff[256] = {0};

  try {
    /* Send resource trap. */
    TTrapSender sender;

    /* Setting sysUpTime */
    sender.setSysUpTime();

    /* Setting trapOID. */
    sender.setTrapOID(trapOID);

    /* Set alert date. */
    snprintf(buff, 255, "%llu", nowTime);
    sender.addValue(OID_ALERT_DATE, OID_LENGTH(OID_ALERT_DATE), buff,
                    SNMP_VAR_TYPE_COUNTER64);

    /* Set thread count. */
    snprintf(buff, 255, "%d", threadCnt);
    sender.addValue(OID_DEAD_COUNT, OID_LENGTH(OID_DEAD_COUNT), buff,
                    SNMP_VAR_TYPE_COUNTER32);

    /* Set thread name. */
    sender.addValue(OID_DEAD_NAME, OID_LENGTH(OID_DEAD_NAME), name,
                    SNMP_VAR_TYPE_STRING);

    /* Send trap. */
    if (unlikely(sender.sendTrap() != SNMP_PROC_SUCCESS)) {
      /* Ouput message and clean up. */
      sender.clearValues();
      throw 1;
    }

  } catch (...) {
    logger->printWarnMsg("Send SNMP deadlock trap failed!");
  }
}

/*!
 * \brief Check deadlock.
 * \param monitor [in]  Monitor object of thread contended.
 * \param list    [out] List of deadlock occurred threads.
 * \return Number of deadlock occurred threads.<br>
 *         Value is 0, if deadlock isn't occurred.
 */
int TDeadlockFinder::checkDeadlock(jobject monitor, TDeadlockList **list) {
  /* Sanity check. */
  if (unlikely(monitor == NULL || list == NULL)) {
    return 0;
  }

  TVMFunctions *vmFunc = TVMFunctions::getInstance();
  TVMVariables *vmVal = TVMVariables::getInstance();
  int deadlockCount = 0;
  void *thisThreadPtr = vmFunc->GetThread();
  void *thread_lock = *(void **)vmVal->getThreadsLock();
  if (unlikely(thisThreadPtr == NULL || thread_lock == NULL)) {
    /*
     * Thread class in JVM and thread lock should be set.
     * TDeadlockFinder::checkDeadlock() is called by MonitorContendedEnter
     * JVMTI event. If this event is fired, current (this) thread must be
     * live.
     */
    logger->printWarnMsg(
          "Deadlock detection failed: Cannot get current thread info.");
    return 0;
  }

  /* Get self thread id. */
  pid_t threadId = syscall(SYS_gettid);

  int *status = (int *)incAddress(thisThreadPtr,
                                  vmVal->getOfsJavaThreadThreadState());
  int original_status = *status;

  /*
   * Normally thread status is "_thread_in_native"
   * when thread is running in JVMTI event.
   * But thread status maybe changed "_thread_in_vm",
   * if call "get_lock_owner" and set true to paramter "doLock".
   * If VMThread found this thread has such status
   * when call "examine_state_of_thread" at synchronizing safepoint,
   * By JVM decided that the thread is waiting VM and callback,
   * thread state at safepoint set "_call_back" flag.
   * Besides, at end of JVMTI envent callback,
   * thread was changed thread status to "_thread_in_native_trans"
   * from "_thread_in_vm"
   * and JVM check thread about running status at safepoint.
   * JVM misunderstand deadlock occurred, so JVM abort by self.
   * Why JVM misunderstood.
   *  1, Safepoint code wait "_thread_in_native_trans" thread.
   *  2, "_call_back" flag means wait VM and callback.
   *  -> VMThread wait the thread and the thread wait VMThread.
   *
   * So thread status reset here by force.
   */
  *status = THREAD_IN_VM;

  /* Check deadlock. */
  bool foundDeadlock = findDeadLock(threadId, monitor);

  if (unlikely(foundDeadlock)) {
    /* Get threads. */
    getLockedThreads(threadId, monitor, list);

    /* Count list item. */
    for (TDeadlockList *item = (*list); item != NULL; item = item->next) {
      deadlockCount++;
    }
  }

  if (*status == THREAD_IN_VM) {
    /*
     * Reset "_thread_state".
     *
     * CAUTION!!
     *   According to the comment of Monitor::lock_without_safepoint_check() in
     *   hotspot/src/share/vm/runtime/mutex.cpp:
     *     If this is called with thread state set to be in VM, the safepoint
     *     synchronization code will deadlock!
     */
    *status = original_status;

    bool needLock = !vmFunc->MonitorOwnedBySelf(thread_lock);

    /*
     * Lock monitor to avoiding a collision
     * with safepoint operation to "ThreadSafepointState".
     * E.g. "examine_state_of_thread".
     */
    if (likely(needLock)) {
      if (isAtSafepoint()) {
        vmFunc->MonitorLockWithoutSafepointCheck(thread_lock);
      } else {
        vmFunc->MonitorLock(thread_lock);
      }
    }

    /* Reset "_safepoint_state". */
    vmFunc->ThreadSafepointStateDestroy(thisThreadPtr);
    vmFunc->ThreadSafepointStateCreate(thisThreadPtr);

    /* Release monitor. */
    if (likely(needLock)) {
      vmFunc->MonitorUnlock(thread_lock);
    }
  }

  return deadlockCount;
}

/*!
 * \brief JThread entry point.
 * \param jvmti [in] JVMTI environment object.
 * \param jni   [in] JNI environment object.
 * \param data  [in] Pointer of TDeadlockFinder.
 */
void JNICALL
    TDeadlockFinder::entryPoint(jvmtiEnv *jvmti, JNIEnv *jni, void *data) {
  /* Get self. */
  TDeadlockFinder *controller = (TDeadlockFinder *)data;
  /* Change running state. */
  controller->_isRunning = true;

  /* Loop for agent run. */
  while (!controller->_terminateRequest) {
    /* Variable for notification flag. */
    bool needProcess = false;

    ENTER_PTHREAD_SECTION(&controller->mutex) {
      /* If no exists request. */
      if (likely(controller->_numRequests == 0)) {
        /* Wait for notification or termination. */
        pthread_cond_wait(&controller->mutexCond, &controller->mutex);
      }

      /* If exists request. */
      if (likely(controller->_numRequests > 0)) {
        controller->_numRequests--;
        needProcess = true;

        /* Load and pop last deadlock datetime. */
        controller->occurTime = controller->timeList.front();
        controller->timeList.pop();
      }
    }
    EXIT_PTHREAD_SECTION(&controller->mutex)

    /* If get notification. */
    if (likely(needProcess)) {
      /* Call event callback. */
      (*controller->callFunc)(jvmti, jni, OccurredDeadlock);
    }
  }

  /* Change running state */
  controller->_isRunning = false;
}

/*!
 * \brief Make and begin Jthread.
 * \param jvmti [in] JVMTI environment object.
 * \param env   [in] JNI environment object.
 */
void TDeadlockFinder::start(jvmtiEnv *jvmti, JNIEnv *env) {
  /* Call super class's method. */
  TAgentThread::start(jvmti, env, TDeadlockFinder::entryPoint, this,
                      JVMTI_THREAD_MIN_PRIORITY);
}

/*!
 * \brief Notify occurred deadlock to this thread from other thread.
 * \param aTime [in] Time of occurred deadlock.
 */
void TDeadlockFinder::notify(jlong aTime) {
  bool raiseException = true;
  /* Send notification and count notify. */
  ENTER_PTHREAD_SECTION(&this->mutex) {
    try {
      /* Store and count data. */
      timeList.push(aTime);
      this->_numRequests++;

      /* Notify occurred deadlock. */
      pthread_cond_signal(&this->mutexCond);

      /* Reset exception flag. */
      raiseException = false;
    } catch (...) {
      /*
       * Maybe faield to allocate memory at "std:queue<T>::push()".
       * So we throw exception again at after release lock.
       */
    }
  }
  EXIT_PTHREAD_SECTION(&this->mutex)

  if (unlikely(raiseException)) {
    throw "Failed to TDeadlockFinder notify";
  }
}

/*!
 * \brief DeadLock search (recursive call).
 * \param startId [in] Thread id of TDeadlockFinder caller.
 * \param monitor [in] Monitor object of thread contended.
 */
FASTCALL bool TDeadlockFinder::findDeadLock(pid_t startId, jobject monitor) {
  TVMFunctions *vmFunc = TVMFunctions::getInstance();
  TVMVariables *vmVal = TVMVariables::getInstance();
  void *threadPtr = NULL;
  int *status = NULL;
  void *nativeThread = NULL;
  pid_t *ownerId = NULL;
  void *contendedMonitor = NULL;
  jobject monitorOop = NULL;

  /* Get owner thread of this monitor. */
  threadPtr = vmFunc->GetLockOwner(monitor, !isAtSafepoint());

  /* No deadlock (no owner thread of this monitor). */
  if (unlikely(threadPtr == NULL)) {
    return false;
  }

  /* Go deadlock check!!! */
  /* Is owner thread is self ? */

  /* Get OSThread ptr related to JavaThread. */
  nativeThread =
      (void *)*(ptrdiff_t *)incAddress(threadPtr,
                                       vmVal->getOfsJavaThreadOsthread());

  /* If failure get native thread. */
  if (unlikely(nativeThread == NULL)) {
    return false;
  }

  /* Get nid (LWP ID). */
  ownerId = (pid_t *)incAddress(nativeThread, vmVal->getOfsOSThreadThreadId());

  /* If occurred deadlock. */
  if (unlikely(*ownerId == startId)) {
    /* DeadLock !!! */
    return true;
  }

  /* Thread status check. */
  status = (int *)incAddress(threadPtr, vmVal->getOfsJavaThreadThreadState());
  if ((*status == THREAD_IN_JAVA) || (*status == THREAD_IN_VM)) {
    /* Owner thread is running. */
    return false;
  }

  /* Get ObjectMonitor pointer. */
  contendedMonitor = (void *)*(ptrdiff_t *)incAddress(
                         threadPtr, vmVal->getOfsThreadCurrentPendingMonitor());
  /* If pointer is illegal. */
  if (unlikely(contendedMonitor == NULL)) {
    return false;
  }

  monitorOop = (jobject)incAddress(contendedMonitor,
                                   vmVal->getOfsObjectMonitorObject());
  /* If java thread already has monitor. */
  if (unlikely((monitorOop == NULL) || (monitorOop == monitor))) {
    return false;
  }

  /* No deadlock in "this" thread. */

  /* Execute recursive call. */
  return findDeadLock(startId, monitorOop);
}

/*!
 * \brief Get threads which has occurred deadlock.
 * \param startId [in]  Thread id of TDeadlockFinder caller.
 * \param monitor [in]  Monitor object of thread contended.
 * \param list    [out] List of deadlock occurred threads.
 */
void TDeadlockFinder::getLockedThreads(pid_t startId, jobject monitor,
                                       TDeadlockList **list) {
  TVMFunctions *vmFunc = TVMFunctions::getInstance();
  TVMVariables *vmVal = TVMVariables::getInstance();
  pid_t *ownerId = NULL;
  TDeadlockList *listHead = NULL;
  TDeadlockList *oldRec = NULL;
  bool flagFailure = false;

  /* Infinite loop. */
  while (true) {
    void *threadPtr = NULL;
    void *nativeThread = NULL;
    void *contendedMonitor = NULL;
    jthread jThreadObj = NULL;
    TDeadlockList *threadRec = NULL;

    /* Get owner thread of this monitor. */
    threadPtr = vmFunc->GetLockOwner(monitor, !isAtSafepoint());

    if (unlikely(threadPtr == NULL)) {
      /* Shouldn't reach to here. */
      logger->printDebugMsg(
          "Deadlock detection failed: Cannot get lock owner thread.");
      flagFailure = true;
      break;
    }

    /* Convert to jni object. */
    jThreadObj = (jthread)incAddress(threadPtr,
                                     vmVal->getOfsJavaThreadThreadObj());

    /* Create list item. */
    threadRec = (TDeadlockList *)malloc(sizeof(TDeadlockList));
    if (unlikely(threadRec == NULL)) {
      /* Shouldn't reach to here. */
      logger->printWarnMsg(
          "Deadlock detection failed: Cannot allocate memory for "
          "TDeadLockList.");
      flagFailure = true;
      break;
    }

    /* Store thread. */
    threadRec->thread = jThreadObj;
    threadRec->next = NULL;
    if (likely(oldRec != NULL)) {
      oldRec->next = threadRec;
    } else {
      listHead = threadRec;
    }
    oldRec = threadRec;

    /* Get OSThread ptr related to JavaThread. */
    nativeThread =
        (void *)*(ptrdiff_t *)incAddress(threadPtr,
                                         vmVal->getOfsJavaThreadOsthread());

    if (unlikely(nativeThread == NULL)) {
      /* Shouldn't reach to here. */
      logger->printDebugMsg(
          "Deadlock detection failed: Cannot get native thread.");
      flagFailure = true;
      break;
    }

    /* Get nid (LWP ID). */
    ownerId = (pid_t *)incAddress(nativeThread,
                                  vmVal->getOfsOSThreadThreadId());

    /* If all thread already has collected. */
    if (*ownerId == startId) {
      break;
    }

    /* Get ObjectMonitor pointer. */
    contendedMonitor = (void *)*(ptrdiff_t *)incAddress(
                         threadPtr, vmVal->getOfsThreadCurrentPendingMonitor());

    if (unlikely(contendedMonitor == NULL)) {
      /* Shouldn't reach to here. */
      logger->printDebugMsg(
          "Deadlock detection failed: Cannot get contended monitor.");
      flagFailure = true;
      break;
    }

    monitor = (jobject)incAddress(contendedMonitor,
                                  vmVal->getOfsObjectMonitorObject());

    /* If illegal state. */
    if (unlikely(monitor == NULL)) {
      /* Shouldn't reach to here. */
      logger->printDebugMsg(
          "Deadlock detection failed: Cannot get monitor object.");
      flagFailure = true;
      break;
    }
  }

  /* If failure get deadlock threads. */
  if (unlikely(flagFailure)) {
    freeDeadlockList(listHead);
  } else {
    (*list) = listHead;
  }

  return;
}

/*!
 * \brief Deallocate deadlock thread list.
 * \param list [in] List of deadlock occurred threads.
 */
void TDeadlockFinder::freeDeadlockList(TDeadlockList *list) {
  /* Sanity check. */
  if (unlikely(list == NULL)) {
    return;
  }

  /* Cleanup. */
  for (TDeadlockList *next = list->next; next != NULL;) {
    TDeadlockList *listItem = next;
    next = next->next;

    free(listItem);
  }
  free(list);
}

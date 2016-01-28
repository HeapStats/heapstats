/*!
 * \file deadlockFinder.cpp
 * \brief This file is used by find deadlock.
 * Copyright (C) 2011-2015 Nippon Telegraph and Telephone Corporation
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

/*!
 * \brief String of symbol which is function getting monitor's owner.
 */
#define GETLOCKOWNER_SYMBOL "_ZN18ObjectSynchronizer14get_lock_ownerE6Handleb"

/*!
 * \brief String of symbol which is function getting this thread.
 */
#define GETTHISTHREAD_SYMBOL "_ZN18ThreadLocalStorage15get_thread_slowEv"

/*!
 * \brief String of symbol which is function create ThreadSafepointState.
 */
#define THREADSAFEPOINTSTATE_CREATE_SYMBOL \
  "_ZN20ThreadSafepointState6createEP10JavaThread"

/*!
 * \brief String of symbol which is function destroy ThreadSafepointState.
 */
#define THREADSAFEPOINTSTATE_DESTROY_SYMBOL \
  "_ZN20ThreadSafepointState7destroyEP10JavaThread"

/*!
 * \brief String of symbol which is function monitor locking.
 */
#define MONITOR_LOCK_SYMBOL "_ZN7Monitor4lockEv"

/*!
 * \brief String of symbol which is function monitor locking.
 */
#define MONITOR_LOCK_WTIHOUT_CHECK_SYMBOL \
  "_ZN7Monitor28lock_without_safepoint_checkEv"

/*!
 * \brief String of symbol which is function monitor unlocking.
 */
#define MONITOR_UNLOCK_SYMBOL "_ZN7Monitor6unlockEv"

/*!
 * \brief String of symbol which is function monitor owned by self.
 */
#define MONITOR_OWNED_BY_SELF_SYMBOL "_ZNK7Monitor13owned_by_selfEv"

/*!
 * \brief String of symbol which is function "Threads_lock" monitor.
 */
#define THREADS_LOCK_SYMBOL "Threads_lock"

/* Class static variables. */

/*!
 * \brief Singleton instance of TDeadlockFinder.
 */
TDeadlockFinder *TDeadlockFinder::inst = NULL;

/*!
 * \brief Flag of deadlock is checkable now.
 */
bool TDeadlockFinder::flagCheckableDeadlock = false;

/*!
 * \brief Offset of field "osthread" in class "JavaThread".
 */
off_t TDeadlockFinder::ofsJavaThreadOsthread = -1;

/*!
 * \brief Offset of field "_threadObj" in class "JavaThread".
 */
off_t TDeadlockFinder::ofsJavaThreadThreadObj = -1;

/*!
 * \brief Offset of field "_thread_state" in class "JavaThread".
 */
off_t TDeadlockFinder::ofsJavaThreadThreadState = -1;

/*!
 * \brief Offset of field "_current_pending_monitor" in class "Thread".
 */
off_t TDeadlockFinder::ofsThreadCurrentPendingMonitor = -1;

/*!
 * \brief Offset of field "_thread_id" in class "OSThread".
 */
off_t TDeadlockFinder::ofsOSThreadThreadId = -1;

/*!
 * \brief Offset of field "_object" in class "ObjectMonitor".
 */
off_t TDeadlockFinder::ofsObjectMonitorObject = -1;

/*!
 * \brief Function pointer of "ObjectSynchronizer::get_lock_owner".
 */
TVMGetLockOwnerFunction TDeadlockFinder::get_lock_owner = NULL;

/*!
 * \brief Function pointer of "ThreadLocalStorage::get_thread_slow".
 */
TVMGetThisThreadFunction TDeadlockFinder::get_this_thread = NULL;

/*!
 * \brief Function pointer of "void ThreadSafepointState::create".
 */
TVMThreadFunction TDeadlockFinder::threadSafepointStateCreate = NULL;

/*!
 * \brief Function pointer of "void ThreadSafepointState::destroy".
 */
TVMThreadFunction TDeadlockFinder::threadSafepointStateDestroy = NULL;

/*!
 * \brief Function pointer of "void Monitor::lock()".
 */
TVMMonitorFunction TDeadlockFinder::monitor_lock = NULL;

/*!
 * \brief Function pointer of
 *        "void Monitor::lock_without_safepoint_check()".
 */
TVMMonitorFunction TDeadlockFinder::monitor_lock_without_check = NULL;

/*!
 * \brief Function pointer of "void Monitor::unlock()".
 */
TVMMonitorFunction TDeadlockFinder::monitor_unlock = NULL;

/*!
 * \brief Function pointer of "bool Monitor::owned_by_self()".
 */
TVMOwnedBySelfFunction TDeadlockFinder::monitor_owned_by_self = NULL;

/*!
 * \brief Variable pointer of "Threads_lock" monitor.
 */
void *TDeadlockFinder::threads_lock = NULL;

/* Common methods. */

/*!
 * \brief Get safepoint state.
 * \return Is synchronizing or working at safepoint now.
 */
inline bool isAtSafepoint(void) {
  return (TVMVariables::getInstance()->getSafePointState() == 2);
}

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

  /* Get function of ObjectSynchronizer::get_lock_owner(). */
  get_lock_owner =
      (TVMGetLockOwnerFunction)symFinder->findSymbol(GETLOCKOWNER_SYMBOL);

  /* Get function of this thread with JVM inner class instance. */
  get_this_thread =
      (TVMGetThisThreadFunction)symFinder->findSymbol(GETTHISTHREAD_SYMBOL);

  /* Get function of thread operation. */
  threadSafepointStateCreate = (TVMThreadFunction)symFinder->findSymbol(
      THREADSAFEPOINTSTATE_CREATE_SYMBOL);
  threadSafepointStateDestroy = (TVMThreadFunction)symFinder->findSymbol(
      THREADSAFEPOINTSTATE_DESTROY_SYMBOL);

  /* Get function of monitor operation. */
  monitor_lock = (TVMMonitorFunction)symFinder->findSymbol(MONITOR_LOCK_SYMBOL);
  monitor_lock_without_check = (TVMMonitorFunction)symFinder->findSymbol(
      MONITOR_LOCK_WTIHOUT_CHECK_SYMBOL);
  monitor_unlock =
      (TVMMonitorFunction)symFinder->findSymbol(MONITOR_UNLOCK_SYMBOL);
  monitor_owned_by_self = (TVMOwnedBySelfFunction)symFinder->findSymbol(
      MONITOR_OWNED_BY_SELF_SYMBOL);

  /* Get function of "Threads_lock" monitor. */
  threads_lock = symFinder->findSymbol(THREADS_LOCK_SYMBOL);

  /* if failure get function pointer "get_lock_owner()". */
  if (unlikely(get_lock_owner == NULL || get_this_thread == NULL ||
               threadSafepointStateCreate == NULL ||
               threadSafepointStateDestroy == NULL || monitor_lock == NULL ||
               monitor_lock_without_check == NULL || monitor_unlock == NULL ||
               monitor_owned_by_self == NULL || threads_lock == NULL)) {
    logger->printWarnMsg("Failure search deadlock function's symbol.");
    result = false;
  }

  /* Symbol offsets. */
  TOffsetNameMap symOffsetList[] = {
      {"JavaThread", "_osthread", &ofsJavaThreadOsthread, NULL},
      {"JavaThread", "_threadObj", &ofsJavaThreadThreadObj, NULL},
      {"JavaThread", "_thread_state", &ofsJavaThreadThreadState, NULL},
      {"Thread", "_current_pending_monitor", &ofsThreadCurrentPendingMonitor,
       NULL},
      {"OSThread", "_thread_id", &ofsOSThreadThreadId, NULL},
      {"ObjectMonitor", "_object", &ofsObjectMonitorObject, NULL},
      /* End marker. */
      {NULL, NULL, NULL}};

  /* Search offset. */
  vmScanner->GetDataFromVMStructs(symOffsetList);

  /* If failure get offset information. */
  if (unlikely(ofsJavaThreadOsthread == -1 || ofsJavaThreadThreadObj == -1 ||
               ofsJavaThreadThreadState == -1 ||
               ofsThreadCurrentPendingMonitor == -1 ||
               ofsOSThreadThreadId == -1 || ofsObjectMonitorObject == -1)) {
    logger->printWarnMsg("Failure search offset in VMStructs.");
    result = false;
  }

  if (result) {
    try {
      inst = new TDeadlockFinder(event);
    } catch (...) {
      logger->printCritMsg("Cannot initialize TDeadlockFinder.");
      result = false;
    }
  }

  flagCheckableDeadlock = result;
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
  if (!isOnLoad) {
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
    TTrapSender sender(SNMP_VERSION_2c, conf->SnmpTarget()->get(),
                       conf->SnmpComName()->get(), 162);

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

  int deadlockCount = 0;
  if (likely(flagCheckableDeadlock)) {
    void *thisThreadPtr = get_this_thread();
    void *thread_lock = *(void **)threads_lock;
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

    int *status = (int *)incAddress(thisThreadPtr, ofsJavaThreadThreadState);
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
      bool needLock = !monitor_owned_by_self(thread_lock);

      /*
       * Lock monitor to avoiding a collision
       * with safepoint operation to "ThreadSafepointState".
       * E.g. "examine_state_of_thread".
       */
      if (likely(needLock)) {
        if (isAtSafepoint()) {
          monitor_lock_without_check(thread_lock);
        } else {
          monitor_lock(thread_lock);
        }
      }

      /* Reset "_thread_state". */
      *status = original_status;
      /* Reset "_safepoint_state". */
      threadSafepointStateDestroy(thisThreadPtr);
      threadSafepointStateCreate(thisThreadPtr);

      /* Release monitor. */
      if (likely(needLock)) {
        monitor_unlock(thread_lock);
      }
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
  void *threadPtr = NULL;
  int *status = NULL;
  void *nativeThread = NULL;
  pid_t *ownerId = NULL;
  void *contendedMonitor = NULL;
  jobject monitorOop = NULL;

  /* Get owner thread of this monitor. */
  threadPtr = get_lock_owner(monitor, !isAtSafepoint());

  /* No deadlock (no owner thread of this monitor). */
  if (unlikely(threadPtr == NULL)) {
    return false;
  }

  /* Go deadlock check!!! */
  /* Is owner thread is self ? */

  /* Get OSThread ptr related to JavaThread. */
  nativeThread =
      (void *)*(ptrdiff_t *)incAddress(threadPtr, ofsJavaThreadOsthread);

  /* If failure get native thread. */
  if (unlikely(nativeThread == NULL)) {
    return false;
  }

  /* Get nid (LWP ID). */
  ownerId = (pid_t *)incAddress(nativeThread, ofsOSThreadThreadId);

  /* If occurred deadlock. */
  if (unlikely(*ownerId == startId)) {
    /* DeadLock !!! */
    return true;
  }

  /* Thread status check. */
  status = (int *)incAddress(threadPtr, ofsJavaThreadThreadState);
  if ((*status == THREAD_IN_JAVA) || (*status == THREAD_IN_VM)) {
    /* Owner thread is running. */
    return false;
  }

  /* Get ObjectMonitor pointer. */
  contendedMonitor = (void *)*(ptrdiff_t *)incAddress(
      threadPtr, ofsThreadCurrentPendingMonitor);
  /* If pointer is illegal. */
  if (unlikely(contendedMonitor == NULL)) {
    return false;
  }

  monitorOop = (jobject)incAddress(contendedMonitor, ofsObjectMonitorObject);
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
    threadPtr = get_lock_owner(monitor, !isAtSafepoint());

    if (unlikely(threadPtr == NULL)) {
      /* Shouldn't reach to here. */
      logger->printDebugMsg(
          "Deadlock detection failed: Cannot get lock owner thread.");
      flagFailure = true;
      break;
    }

    /* Convert to jni object. */
    jThreadObj = (jthread)incAddress(threadPtr, ofsJavaThreadThreadObj);

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
        (void *)*(ptrdiff_t *)incAddress(threadPtr, ofsJavaThreadOsthread);

    if (unlikely(nativeThread == NULL)) {
      /* Shouldn't reach to here. */
      logger->printDebugMsg(
          "Deadlock detection failed: Cannot get native thread.");
      flagFailure = true;
      break;
    }

    /* Get nid (LWP ID). */
    ownerId = (pid_t *)incAddress(nativeThread, ofsOSThreadThreadId);

    /* If all thread already has collected. */
    if (*ownerId == startId) {
      break;
    }

    /* Get ObjectMonitor pointer. */
    contendedMonitor = (void *)*(ptrdiff_t *)incAddress(
        threadPtr, ofsThreadCurrentPendingMonitor);

    if (unlikely(contendedMonitor == NULL)) {
      /* Shouldn't reach to here. */
      logger->printDebugMsg(
          "Deadlock detection failed: Cannot get contended monitor.");
      flagFailure = true;
      break;
    }

    monitor = (jobject)incAddress(contendedMonitor, ofsObjectMonitorObject);

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

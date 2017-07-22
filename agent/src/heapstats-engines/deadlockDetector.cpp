/*!
 * \file deadlockDetector.cpp
 * \brief This file is used by find deadlock.
 * Copyright (C) 2017 Yasumasa Suenaga
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 */

#include <jvmti.h>
#include <jni.h>

#include <sched.h>

#ifdef HAVE_ATOMIC
#include <atomic>
#else
#include <cstdatomic>
#endif

#include "globals.hpp"
#include "elapsedTimer.hpp"
#include "util.hpp"
#include "libmain.hpp"
#include "callbackRegister.hpp"
#include "deadlockDetector.hpp"


#define BUFFER_SZ 256


/*!
 * \brief Function pointer to jmm_FindMonitorDeadlockedThreads()
 */
static Tjmm_FindMonitorDeadlockedThreads jmm_FindMonitorDeadlockedThreads;

/*!
 * \brief Processing flag.
 */
static std::atomic_int processing(0);


namespace dldetector {

  /*!
   * \brief Send SNMP Trap which contains deadlock information.
   * \param nowTime    [in]  The time of deadlock occurred.
   * \param numThreads [in]  Number of threads which are related to deadlock.
   * \param name       [in]  Thread name of deadlock occurred.
   */
  static void sendSNMPTrap(TMSecTime nowTime,
                           int numThreads, const char *name) {
    /* OIDs */
    char trapOID[50] = OID_DEADLOCKALERT;
    oid OID_ALERT_DATE[] = {SNMP_OID_HEAPALERT, 1};
    oid OID_DEAD_COUNT[] = {SNMP_OID_DEADLOCKALERT, 1};
    oid OID_DEAD_NAME[] = {SNMP_OID_DEADLOCKALERT, 2};

    char buf[BUFFER_SZ];

    TTrapSender sender;
    sender.setSysUpTime();
    sender.setTrapOID(trapOID);

    /* Set alert datetime. */
    snprintf(buf, BUFFER_SZ - 1, "%llu", nowTime);
    sender.addValue(OID_ALERT_DATE, OID_LENGTH(OID_ALERT_DATE), buf,
                    SNMP_VAR_TYPE_COUNTER64);

    /* Set thread count. */
    snprintf(buf, BUFFER_SZ - 1, "%d", numThreads);
    sender.addValue(OID_DEAD_COUNT, OID_LENGTH(OID_DEAD_COUNT), buf,
                    SNMP_VAR_TYPE_COUNTER32);

    /* Set thread name. */
    sender.addValue(OID_DEAD_NAME, OID_LENGTH(OID_DEAD_NAME), name,
                    SNMP_VAR_TYPE_STRING);

    /* Send trap. */
    if (unlikely(sender.sendTrap() != SNMP_PROC_SUCCESS)) {
      /* Ouput message and clean up. */
      sender.clearValues();
      logger->printWarnMsg("Could not send SNMP trap for deadlock!");
    }
  }

  /*!
   * \brief Notify deadlock occurrence via stdout/err and SNMP Trap.
   * \param jvmti           [in]  JVMTI environment.
   * \oaram env             [in]  JNI environment.
   * \param thread          [in]  jthread object which deadlock triggered.
   * \param monitor         [in]  Monitor object.
   * \param deadlockThreads [in]  Array of deadlock threads (not required).
   */
  static void notifyDeadlockOccurrence(jvmtiEnv *jvmti, JNIEnv *env,
                jthread thread, jobject monitor, jobjectArray deadlockThreads) {
    TProcessMark mark(processing);

    TMSecTime nowTime = (TMSecTime)getNowTimeSec();

    jvmtiThreadInfo threadInfo = {0};
    jvmti->GetThreadInfo(thread, &threadInfo);

    if (deadlockThreads == NULL) {
      deadlockThreads = jmm_FindMonitorDeadlockedThreads(env);
    }

    int numThreads = env->GetArrayLength(deadlockThreads);
    logger->printCritMsg(
                "ALERT(DEADLOCK): Deadlock occurred! count: %d, thread: \"%s\"",
                numThreads, threadInfo.name);

    if (conf->SnmpSend()->get()) {
      sendSNMPTrap(nowTime, numThreads, threadInfo.name);
    }

    jvmti->Deallocate((unsigned char *)threadInfo.name);

    if (conf->TriggerOnLogLock()->get()) {
      TElapsedTimer elapsedTime("Take LogInfo");
      int result = logManager->collectLog(jvmti, env,
                                          OccurredDeadlock, nowTime, "");
      if (unlikely(result != 0)) {
        logger->printWarnMsg("Could not collect log archive.");
      }
    }

    if (unlikely(conf->KillOnError()->get())) {
      forcedAbortJVM(jvmti, env, "deadlock occurred");
    }
  }

  /*!
   * \brief Event handler of JVMTI MonitorContendedEnter for finding deadlock.
   * \param jvmti  [in] JVMTI environment.
   * \param env    [in] JNI environment of the event (current) thread.
   * \param thread [in] JNI local reference to the thread attempting to enter
   *                    the monitor.
   * \param object [in] JNI local reference to the monitor.
   */
  void JNICALL OnMonitorContendedEnter(
                 jvmtiEnv *jvmti, JNIEnv *env, jthread thread, jobject object) {
    /* Find owner thread of the lock */
    jvmtiMonitorUsage monitor_usage;
    if (isError(jvmti, jvmti->GetObjectMonitorUsage(object, &monitor_usage))) {
      logger->printWarnMsg("Could not get monitor usage (%p)\n",
                           *(void **)object);
      return;
    }
    jvmti->Deallocate((unsigned char *)monitor_usage.waiters);
    jvmti->Deallocate((unsigned char *)monitor_usage.notify_waiters);
    if (monitor_usage.owner == NULL) { // Monitor has been released
      return;
    }

    /* Check thread state of monitor owner */
    jint thread_state;
    if (isError(jvmti,
                jvmti->GetThreadState(monitor_usage.owner, &thread_state))) {
      logger->printWarnMsg("Could not get thread state of monitor owner (%p)\n",
                           *(void **)monitor_usage.owner);
      return;
    } else {
      jint MonEnterState =
                     thread_state & JVMTI_THREAD_STATE_BLOCKED_ON_MONITOR_ENTER;
      if (MonEnterState == 0) {
        /* No dead lock because owner thread does not wait at the monitor. */
        return;
      }
    }

    /* Find dead lock via VM operation */
    jobjectArray dl_threads = jmm_FindMonitorDeadlockedThreads(env);
    if ((dl_threads != NULL) && (env->GetArrayLength(dl_threads) > 0)) {
      notifyDeadlockOccurrence(jvmti, env, thread, object, dl_threads);
    }
  }


  /*!
   * \brief Deadlock detector initializer.
   * \param jvmti [in]  JVMTI environment
   * \return Process result.
   * \warning This function MUST be called only once.
   */
  bool initialize(jvmtiEnv *jvmti) {
    jmm_FindMonitorDeadlockedThreads =
             (Tjmm_FindMonitorDeadlockedThreads)symFinder->findSymbol(
                                                        SYMBOL_DEADLOCK_FINDER);
    if (jmm_FindMonitorDeadlockedThreads == NULL) {
      logger->printCritMsg(
                    "Could not find symbol: " SYMBOL_DEADLOCK_FINDER "\n");
      return false;
    }

    jvmtiCapabilities capabilities = {0};
    capabilities.can_get_monitor_info = 1;
    TMonitorContendedEnterCallback::mergeCapabilities(&capabilities);
    if (isError(jvmti, jvmti->AddCapabilities(&capabilities))) {
      logger->printCritMsg(
                      "Couldn't set event capabilities for deadlock detector.");
      return false;
    }

    TMonitorContendedEnterCallback::registerCallback(&OnMonitorContendedEnter);
    TMonitorContendedEnterCallback::switchEventNotification(jvmti,
                                                            JVMTI_ENABLE);

    return true;
  }

  /*!
   * \brief Deadlock detector finalizer.
   *        This function unregisters JVMTI callback for deadlock detection.
   * \param jvmti [in]  JVMTI environment
   */
  void finalize(jvmtiEnv *jvmti) {
    TMonitorContendedEnterCallback::unregisterCallback(
                                                &OnMonitorContendedEnter);

    /* Refresh JVMTI event callbacks */
    registerJVMTICallbacks(jvmti);

    while (processing > 0) {
      sched_yield();
    }
  }

}


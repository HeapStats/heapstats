/*!
 * \file deadlockDetector.cpp
 * \brief This file is used by find deadlock.
 * Copyright (C) 2017-2019 Yasumasa Suenaga
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

#include <pthread.h>
#include <sched.h>

#include <atomic>
#include <map>

#include "globals.hpp"
#include "elapsedTimer.hpp"
#include "libmain.hpp"
#include "callbackRegister.hpp"
#include "deadlockDetector.hpp"


#define BUFFER_SZ 256


/*!
 * \brief Processing flag.
 */
static std::atomic_int processing(0);

/*!
 * \brief Monitor owner map.
 */
static std::map<jint, jint> monitor_owners;

/*!
 * \brief Monitor waiters list.
 */
static std::map<jint, jint> waiter_list;

/*!
 * \brief Mutex for maps
 */
static pthread_mutex_t mutex = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;


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
   * \param jvmti      [in]  JVMTI environment.
   * \oaram env        [in]  JNI environment.
   * \param thread     [in]  jthread object which deadlock triggered.
   * \param monitor    [in]  Monitor object.
   * \param numThreads [in]  Number of threads which are related to deadlock.
   */
  static void notifyDeadlockOccurrence(jvmtiEnv *jvmti, JNIEnv *env,
                                       jthread thread, jobject monitor,
                                       int numThreads) {
    TProcessMark mark(processing);

    TMSecTime nowTime = (TMSecTime)getNowTimeSec();

    jvmtiThreadInfo threadInfo = {0};
    jvmti->GetThreadInfo(thread, &threadInfo);

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
    /* Check owned monitors */
    jint monitor_cnt;
    jobject *owned_monitors;
    if (isError(jvmti, jvmti->GetOwnedMonitorInfo(thread,
                                                  &monitor_cnt,
                                                  &owned_monitors))) {
      logger->printWarnMsg("Could not get owned monitor info (%p)\n",
                           *(void **)thread);
      return;
    }
    if (monitor_cnt == 0) { // This thread does not have a monitor.
      jvmti->Deallocate((unsigned char *)owned_monitors);
      return;
    }

    jint monitor_hash;
    jint thread_hash;
    jvmti->GetObjectHashCode(thread, &thread_hash);
    jvmti->GetObjectHashCode(object, &monitor_hash);

    {
      TMutexLocker locker(&mutex);

      /* Avoid JDK-8185164 */
      bool canSkip = true;

      /* Store all owned monitors to owner list */
      for (int idx = 0; idx < monitor_cnt; idx++) {
        jint current_monitor_hash;
        jvmti->GetObjectHashCode(owned_monitors[idx], &current_monitor_hash);
        if (current_monitor_hash == monitor_hash) {
          continue;
        }
        monitor_owners[current_monitor_hash] = thread_hash;
        canSkip = false;
      }
      jvmti->Deallocate((unsigned char *)owned_monitors);

      if (!canSkip) {
        /* Add to waiters list */
        jvmti->GetObjectHashCode(object, &monitor_hash);
        waiter_list[thread_hash] = monitor_hash;

        /* Check deadlock */
        int numThreads = 1;
        while (true) {
          numThreads++;

          auto owner_itr = monitor_owners.find(monitor_hash);
          if (owner_itr == monitor_owners.end()) {
            break;  // No deadlock
          }

          auto waiter_itr = waiter_list.find(owner_itr->second);
          if (waiter_itr == waiter_list.end()) {
            break; // No deadlock
          }

          owner_itr = monitor_owners.find(waiter_itr->second);
          if (owner_itr == monitor_owners.end()) {
            break; // No deadlock
          }

          if (owner_itr->second == thread_hash) {
            // Deadlock!!
            notifyDeadlockOccurrence(jvmti, env, thread, object, numThreads);
            break;
          }

          monitor_hash = waiter_itr->second;
        }
      }
    }
  }


  /*!
   * \brief Event handler of JVMTI MonitorContendedEntered for finding deadlock.
   * \param jvmti  [in] JVMTI environment.
   * \param env    [in] JNI environment of the event (current) thread.
   * \param thread [in] JNI local reference to the thread attempting to enter
   *                    the monitor.
   * \param object [in] JNI local reference to the monitor.
   */
  void JNICALL OnMonitorContendedEntered(
                 jvmtiEnv *jvmti, JNIEnv *env, jthread thread, jobject object) {
    /* Check owned monitors */
    jint monitor_cnt;
    jobject *owned_monitors;
    if (isError(jvmti, jvmti->GetOwnedMonitorInfo(thread,
                                                  &monitor_cnt,
                                                  &owned_monitors))) {
      logger->printWarnMsg("Could not get owned monitor info (%p)\n",
                           *(void **)thread);
      return;
    }
    if (monitor_cnt <= 1) { // This thread does not have other monitor(s).
      jvmti->Deallocate((unsigned char *)owned_monitors);
      return;
    }

    {
      TMutexLocker locker(&mutex);

      /* Remove all owned monitors from owner list */
      for (int idx = 0; idx < monitor_cnt; idx++) {
        jint monitor_hash;
        jvmti->GetObjectHashCode(owned_monitors[idx], &monitor_hash);
        monitor_owners.erase(monitor_hash);
      }
      jvmti->Deallocate((unsigned char *)owned_monitors);

      /* Remove thread from waiters list */
      jint thread_hash;
      jvmti->GetObjectHashCode(thread, &thread_hash);
      waiter_list.erase(thread_hash);
    }
  }


  /*!
   * \brief Deadlock detector initializer.
   * \param jvmti    [in]  JVMTI environment
   * \parma isOnLoad [in]  OnLoad phase or not (Live phase).
   * \return Process result.
   * \warning This function MUST be called only once.
   */
  bool initialize(jvmtiEnv *jvmti, bool isOnLoad) {
    jvmtiCapabilities capabilities = {0};
    capabilities.can_get_monitor_info = 1;
    if (isOnLoad) {
      /*
       * can_get_owned_monitor_info must be set at OnLoad phase.
       *
       * See also:
       *   hotspot/src/share/vm/prims/jvmtiManageCapabilities.cpp
       */
      capabilities.can_get_owned_monitor_info = 1;
    }
    TMonitorContendedEnterCallback::mergeCapabilities(&capabilities);
    if (isError(jvmti, jvmti->AddCapabilities(&capabilities))) {
      logger->printCritMsg(
                      "Couldn't set event capabilities for deadlock detector.");
      return false;
    }

    /*
     * All JVMTI events are not fired at this point.
     * So we need not to lock this operation.
     */
    monitor_owners.clear();
    waiter_list.clear();

    TMonitorContendedEnterCallback::registerCallback(&OnMonitorContendedEnter);
    TMonitorContendedEnterCallback::switchEventNotification(jvmti,
                                                            JVMTI_ENABLE);
    TMonitorContendedEnteredCallback::registerCallback(
                                                 &OnMonitorContendedEntered);
    TMonitorContendedEnteredCallback::switchEventNotification(jvmti,
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
    TMonitorContendedEnteredCallback::unregisterCallback(
                                                &OnMonitorContendedEntered);

    /* Refresh JVMTI event callbacks */
    registerJVMTICallbacks(jvmti);

    while (processing > 0) {
      sched_yield();
    }

    {
      TMutexLocker locker(&mutex);
      monitor_owners.clear();
      waiter_list.clear();
    }
  }

}


/*!
 * \file deadlockFinder.hpp
 * \brief This file is used by find deadlock.
 * Copyright (C) 2011-2016 Nippon Telegraph and Telephone Corporation
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

#ifndef _DEADLOCK_FINDER_H
#define _DEADLOCK_FINDER_H

#include <jvmti.h>
#include <jni.h>

#include <queue>

#include "util.hpp"
#include "agentThread.hpp"

/*!
 * \brief This macro deginate calling function by using registry.
 */
#ifndef __amd64__
#define FASTCALL __attribute__((regparm(2)))
#else
#define FASTCALL
#endif

/*!
 * \brief This type is callback to periodic calling by timer.
 * \param jvmti [in] JVMTI environment object.
 * \param env   [in] JNI environment object.
 * \param cause [in] Cause of invoke function.<br>Maybe "OccurredDeadlock".
 */
typedef void (*TDeadlockEventFunc)(jvmtiEnv *jvmti, JNIEnv *env,
                                   TInvokeCause cause);


/*!
 * \brief This type is stored deadlock occurred thread list.
 */
struct TDeadlockList {
  jthread thread;
  /*!< A thread of deadlock occurred. */
  TDeadlockList *next;
  /*!< Next record item. */
};

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
                                                jthread thread, jobject object);

/*!
 * \brief This class is searching deadlock.
 */
class TDeadlockFinder : public TAgentThread {
 public:
  /*!
   * \brief Global initialization.
   * \return Process result.
   * \param event [in] Callback is used on deadlock occurred.
   * \warning Please call only once from main thread.
   */
  static bool globalInitialize(TDeadlockEventFunc event);

  /*!
   * \brief Global finalization of deadlock finder.
   */
  static void globalFinalize(void);

  /*!
   * \brief Set JVMTI functional capabilities.
   * \param capabilities [out] JVMTI capabilities to add.
   * \param isOnLoad     [in]  OnLoad phase or not (Live phase).
   */
  static void setCapabilities(jvmtiCapabilities *capabilities, bool isOnLoad);

  using TAgentThread::start;

  /*!
   * \brief Make and begin Jthread.
   * \param jvmti [in] JVMTI environment object.
   * \param env   [in] JNI environment object.
   */
  void start(jvmtiEnv *jvmti, JNIEnv *env);

  using TAgentThread::notify;

  /*!
   * \brief Notify occurred deadlock to this thread from other thread.
   * \param aTime [in] Time of occurred deadlock.
   */
  void notify(jlong aTime);

  /*!
   * \brief Send SNMP trap which contains deadlock information.
   * \param nowTime The time of deadlock occurred.
   * \param threadCnt Number of threads which are related to deadlock.
   * \param name Thread name of deadlock occurred.
   */
  void sendSNMPTrap(TMSecTime nowTime, int threadCnt, const char *name);

  /*!
   * \brief Check deadlock.
   * \param monitor [in]  Monitor object of thread contended.
   * \param list    [out] List of deadlock occurred threads.
   * \return Number of deadlock occurred threads.<br>
   *         Value is 0, if deadlock isn't occurred.
   */
  static int checkDeadlock(jobject monitor, TDeadlockList **list);

  /*!
   * \brief Deallocate deadlock thread list.
   * \param list [in] List of deadlock occurred threads.
   */
  static void freeDeadlockList(TDeadlockList *list);

  /*!
   * \brief Get deadlock time.
   * \return Time of finally occurred deadlock until now.
   */
  const inline jlong getDeadlockTime(void) { return occurTime; };

  /*!
   * \brief Get singleton instance of TDeadlockFinder.
   * \return Singleton instance of TDeadlockFinder.
   */
  static TDeadlockFinder *getInstance() { return inst; };

 protected:
  /*!
   * \brief TDeadlockFinder constructor.
   * \param event [in] Callback is used on deadlock occurred.
   */
  TDeadlockFinder(TDeadlockEventFunc event);

  /*!
   * \brief TDeadlockFinder destructor.
   */
  virtual ~TDeadlockFinder(void);

  /*!
   * \brief Event handler of JVMTI MonitorContendedEnter for finding
   *        deadlock.
   * \param jvmti  [in] JVMTI environment.
   * \param env    [in] JNI environment of the event (current) thread.
   * \param thread [in] JNI local reference to the thread attempting
   *                    to enter the monitor.
   * \param object [in] JNI local reference to the monitor.
   * \warning Sometimes this callback is called synchronizing or
   *          synchronized at safepoint.<br>Please be carefully calling
   *          inner function of JVM.
   */
  friend void JNICALL
      OnMonitorContendedEnterForDeadlock(jvmtiEnv *jvmti, JNIEnv *env,
                                         jthread thread, jobject object);

  /*!
   * \brief JThread entry point.
   * \param jvmti [in] JVMTI environment object.
   * \param jni   [in] JNI environment object.
   * \param data  [in] Pointer of TDeadlockFinder.
   */
  static void JNICALL entryPoint(jvmtiEnv *jvmti, JNIEnv *jni, void *data);

  /*!
   * \brief DeadLock search (recursive call).
   * \param startId [in] Thread id of TDeadlockFinder caller.
   * \param monitor [in] Monitor object of thread contended.
   * \return Is found deadlock.
   */
  static FASTCALL bool findDeadLock(pid_t startId, jobject monitor);

  /*!
   * \brief Get threads which has occurred deadlock.
   * \param startId [in]  Thread id of TDeadlockFinder caller.
   * \param monitor [in]  Monitor object of thread contended.
   * \param list    [out] List of deadlock occurred threads.
   */
  static void getLockedThreads(pid_t startId, jobject monitor,
                               TDeadlockList **list);

 private:
  /*!
   * \brief Singleton instance of TDeadlockFinder.
   */
  static TDeadlockFinder *inst;

  /*!
   * \brief Callback for event on deadlock occurred.
   */
  TDeadlockEventFunc callFunc;

  /*!
   * \brief Time of finally occurred deadlock until now.
   */
  volatile jlong occurTime;

  /*!
   * \brief Queue of occurred deadlock datetime.
   */
  std::queue<jlong> timeList;
};

#endif  // _DEADLOCK_FINDER_H

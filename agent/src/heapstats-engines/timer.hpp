/*!
 * \file timer.hpp
 * \brief This file is used to take interval snapshot.
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

#ifndef TIMER_HPP
#define TIMER_HPP

#include <semaphore.h>

#include "util.hpp"
#include "agentThread.hpp"

/*!
 * \brief This type is callback to periodic calling by timer.
 * \param jvmti [in] JVMTI environment object.
 * \param env   [in] JNI environment object.
 * \param cause [in] Cause of invoke function.<br>Maybe "Interval".
 */
typedef void (*TTimerEventFunc)(jvmtiEnv *jvmti, JNIEnv *env,
                                TInvokeCause cause);

/*!
 * \brief This class is used to take interval snapshot.
 */
class TTimer : public TAgentThread {
 public:
  /*!
   * \brief TTimer constructor.
   * \param event [in] Callback is used by interval calling.
   * \param timerName [in] Unique name of timer.
   */
  TTimer(TTimerEventFunc event, const char *timerName);
  /*!
   * \brief TTimer destructor.
   */
  virtual ~TTimer(void);

  /*!
   * \brief Make and begin Jthread.
   * \param jvmti    [in] JVMTI environment object.
   * \param env      [in] JNI environment object.
   * \param interval [in] Interval of invoke timer event.
   */
  void start(jvmtiEnv *jvmti, JNIEnv *env, jlong interval);
  /*!
   * \brief Notify reset timing to this thread from other thread.
   */
  void notify(void);
  /*!
   * \brief Notify stop to this thread from other thread.
   */
  void stop(void);

 protected:
  /*!
   * \brief JThread entry point called by interval or nofity.
   * \param jvmti [in] JVMTI environment object.
   * \param jni   [in] JNI environment object.
   * \param data  [in] Pointer of TTimer.
   */
  static void JNICALL entryPoint(jvmtiEnv *jvmti, JNIEnv *jni, void *data);

  /*!
   * \brief JThread entry point called by notify only.
   * \param jvmti [in] JVMTI environment object.
   * \param jni   [in] JNI environment object.
   * \param data  [in] Pointer of TTimer.
   */
  static void JNICALL
      entryPointByCall(jvmtiEnv *jvmti, JNIEnv *jni, void *data);

  /*!
   * \brief Callback for periodic calling by timer.
   */
  TTimerEventFunc _eventFunc;
  /*!
   * \brief Interval of timer.
   */
  jlong timerInterval;

  /*!
   * \brief Timer interrupt flag.
   */
  volatile bool _isInterrupted;

  /*!
   * \brief Timer semphore.
   */
  sem_t timerSem;
};

#endif

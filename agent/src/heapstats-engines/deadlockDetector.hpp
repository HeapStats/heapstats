/*!
 * \file deadlockDetector.hpp
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

#ifndef _DEADLOCK_DETECTOR_H
#define _DEADLOCK_DETECTOR_H

#include <jvmti.h>
#include <jni.h>


#define SYMBOL_DEADLOCK_FINDER "jmm_FindMonitorDeadlockedThreads"


/*!
 * \brief Type of jmm_FindMonitorDeadlockedThreads()
 */
typedef jobjectArray (*Tjmm_FindMonitorDeadlockedThreads)(JNIEnv *env);


namespace dldetector {

  /*!
   * \brief Event handler of JVMTI MonitorContendedEnter for finding deadlock.
   * \param jvmti  [in] JVMTI environment.
   * \param env    [in] JNI environment of the event (current) thread.
   * \param thread [in] JNI local reference to the thread attempting to enter
   *                    the monitor.
   * \param object [in] JNI local reference to the monitor.
   */
  void JNICALL OnMonitorContendedEnter(jvmtiEnv *jvmti, JNIEnv *env,
                                       jthread thread, jobject object);

  /*!
   * \brief Deadlock detector initializer.
   * \param jvmti  [in]  JVMTI environment
   * \return Process result.
   * \warning This function MUST be called only once.
   */
  bool initialize(jvmtiEnv *jvmti);

  /*!
   * \brief Deadlock detector finalizer.
   *        This function unregisters JVMTI callback for deadlock detection.
   * \param jvmti [in]  JVMTI environment
   */
  void finalize(jvmtiEnv *jvmti);

};

#endif  // _DEADLOCK_DETECTOR_H

/*!
 * \file snapShotMain.hpp
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

#ifndef _SNAPSHOT_MAIN_HPP
#define _SNAPSHOT_MAIN_HPP

#include <jvmti.h>
#include <jni.h>

#include "util.hpp"

/* Define export function for calling from external. */
extern "C" {
  /*!
  * \brief Take a heap information snapshot.
  * \param jvmti [in] JVMTI environment object.
  * \param env   [in] JNI environment object.
  * \param cause [in] Cause of taking a snapshot.<br>
  *                   e.g. GC, DumpRequest or Interval.
  */
  void TakeSnapShot(jvmtiEnv *jvmti, JNIEnv *env, TInvokeCause cause);
};

/* Event for logging function. */

/*!
 * \brief Setting enable of JVMTI and extension events for snapshot function.
 * \param jvmti  [in] JVMTI environment object.
 * \param enable [in] Event notification is enable.
 * \return Setting process result.
 */
jint setEventEnableForSnapShot(jvmtiEnv *jvmti, bool enable);

/*!
 * \brief Setting enable of agent each threads for snapshot function.
 * \param jvmti  [in] JVMTI environment object.
 * \param env    [in] JNI environment object.
 * \param enable [in] Event notification is enable.
 */
void setThreadEnableForSnapShot(jvmtiEnv *jvmti, JNIEnv *env, bool enable);

/*!
 * \brief Clear current SnapShot.
 */
void clearCurrentSnapShot();

/*!
 * \brief JVM initialization event for snapshot function.
 * \param jvmti  [in] JVMTI environment object.
 * \param env    [in] JNI environment object.
 */
void onVMInitForSnapShot(jvmtiEnv *jvmti, JNIEnv *env);

/*!
 * \brief JVM finalization event for snapshot function.
 * \param jvmti [in] JVMTI environment object.
 * \param env   [in] JNI environment object.
 */
void onVMDeathForSnapShot(jvmtiEnv *jvmti, JNIEnv *env);

/*!
 * \brief Agent initialization for snapshot function.
 * \param jvmti [in] JVMTI environment object.
 * \return Initialize process result.
 */
jint onAgentInitForSnapShot(jvmtiEnv *jvmti);

/*!
 * \brief Agent finalization for snapshot function.
 * \param env [in] JNI environment object.
 */
void onAgentFinalForSnapShot(JNIEnv *env);

/* JVMTI events. */

/*!
 * \brief New class loaded event.
 * \param jvmti  [in] JVMTI environment object.
 * \param env    [in] JNI environment object.
 * \param thread [in] Java thread object.
 * \param klass  [in] Newly loaded class object.
 */
void JNICALL
    OnClassPrepare(jvmtiEnv *jvmti, JNIEnv *env, jthread thread, jclass klass);

/*!
 * \brief Class unload event.
 * \param jvmti  [in] JVMTI environment object.
 * \param env    [in] JNI environment object.
 * \param thread [in] Java thread object.
 * \param klass  [in] Unload class object.
 * \sa    from: hotspot/src/share/vm/prims/jvmti.xml
 */
void JNICALL
    OnClassUnload(jvmtiEnv *jvmti, JNIEnv *env, jthread thread, jclass klass);

/*!
 * \brief Before garbage collection event.
 * \param jvmti [in] JVMTI environment object.
 */
void JNICALL OnGarbageCollectionStart(jvmtiEnv *jvmti);

/*!
 * \brief After garbage collection event.
 * \param jvmti [in] JVMTI environment object.
 */
void JNICALL OnGarbageCollectionFinish(jvmtiEnv *jvmti);

/*!
 * \brief Event of before garbage collection by CMS collector.
 * \param jvmti [in] JVMTI environment object.
 */
void JNICALL OnCMSGCStart(jvmtiEnv *jvmti);

/*!
 * \brief Event of after garbage collection by CMS collector.
 * \param jvmti [in] JVMTI environment object.
 */
void JNICALL OnCMSGCFinish(jvmtiEnv *jvmti);

/*!
 * \brief Data dump request event for snapshot.
 * \param jvmti [in] JVMTI environment object.
 */
void JNICALL OnDataDumpRequestForSnapShot(jvmtiEnv *jvmti);

#endif  // _SNAPSHOT_MAIN_HPP

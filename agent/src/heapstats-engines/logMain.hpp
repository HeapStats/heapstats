/*!
 * \file logMain.hpp
 * \brief This file is used common logging process.
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

#ifndef _LOG_MAIN_HPP
#define _LOG_MAIN_HPP

#include <jvmti.h>
#include <jni.h>

/* Event for logging function. */

/*!
 * \brief Interval watching for log signal.
 * \param jvmti [in] JVMTI environment object.
 * \param env   [in] JNI environment object.
 */
void intervalSigProcForLog(jvmtiEnv *jvmti, JNIEnv *env);

/*!
 * \brief Setting enable of JVMTI and extension events for log function.
 * \param jvmti  [in] JVMTI environment object.
 * \param enable [in] Event notification is enable.
 * \return Setting process result.
 */
jint setEventEnableForLog(jvmtiEnv *jvmti, bool enable);

/*!
 * \brief Setting enable of agent each threads for log function.
 * \param jvmti  [in] JVMTI environment object.
 * \param env    [in] JNI environment object.
 * \param enable [in] Event notification is enable.
 */
void setThreadEnableForLog(jvmtiEnv *jvmti, JNIEnv *env, bool enable);

/*!
 * \brief JVM initialization event for log function.
 * \param jvmti  [in] JVMTI environment object.
 * \param env    [in] JNI environment object.
 */
void onVMInitForLog(jvmtiEnv *jvmti, JNIEnv *env);

/*!
 * \brief JVM finalization event for log function.
 * \param jvmti [in] JVMTI environment object.
 * \param env   [in] JNI environment object.
 */
void onVMDeathForLog(jvmtiEnv *jvmti, JNIEnv *env);

/*!
 * \brief Agent initialization for log function.
 * \return Initialize process result.
 */
jint onAgentInitForLog(void);

/*!
 * \brief Agent finalization for log function.
 * \param env [in] JNI environment object.
 */
void onAgentFinalForLog(JNIEnv *env);

/* JVMTI events. */

/*!
 * \brief JVM resource exhausted event.
 * \param jvmti       [in] JVMTI environment object.
 * \param env         [in] JNI environment object.
 * \param flags       [in] Resource information bit flag.
 * \param reserved    [in] Reserved variable.
 * \param description [in] Description about resource exhausted.
 */
void JNICALL OnResourceExhausted(jvmtiEnv *jvmti, JNIEnv *env, jint flags,
                                 const void *reserved, const char *description);

#endif  // _LOG_MAIN_HPP

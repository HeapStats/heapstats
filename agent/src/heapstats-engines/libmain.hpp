/*!
 * \file libmain.hpp
 * \brief This file is used to common works.<br>
 *        e.g. initialization, finalization, etc...
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

#ifndef LIBMAIN_HPP
#define LIBMAIN_HPP

/* Define export function for calling from external. */
extern "C" {
  /*!
   * \brief Agent attach entry points.
   * \param vm       [in] JavaVM object.
   * \param options  [in] Commandline arguments.
   * \param reserved [in] Reserved.
   * \return Attach initialization result code.
   */
  JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved);

  /*!
   * \brief Common agent unload entry points.
   * \param vm [in] JavaVM object.
   */
  JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm);

  /*!
   * \brief Ondemand attach's entry points.
   * \param vm       [in] JavaVM object.
   * \param options  [in] Commandline arguments.
   * \param reserved [in] Reserved.
   * \return Ondemand-attach initialization result code.
   */
  JNIEXPORT jint JNICALL
      Agent_OnAttach(JavaVM *vm, char *options, void *reserved);
};

/*!
 * \brief Abort JVM by force on illegal status.
 * \param jvmti    [in] JVMTI environment object.
 * \param env      [in] JNI environment object.
 * \param causeMsg [in] Message about cause of aborting JVM.
 * \warning This function is always no return.
 */
void forcedAbortJVM(jvmtiEnv *jvmti, JNIEnv *env, const char *causeMsg);

#endif  // LIBMAIN_HPP

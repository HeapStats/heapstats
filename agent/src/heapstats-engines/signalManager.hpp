/*!
 * \file signalManager.hpp
 * \brief This file is used by signal handling.
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

#ifndef _SIGNAL_MNGR_H
#define _SIGNAL_MNGR_H

#include "util.hpp"

/*!
 * \brief This type is signal callback by TSignalManager.
 * \param signo   [in] Number of received signal.
 * \param siginfo [in] Informantion of received signal.<br>
 *                     But this value is always null.
 * \param data    [in] Data of received signal.<br>
 *                     But this value is always null.
 * \sa Please see JDK source about "siginfo" and "data" is always null.<br>
 *     jdk/src/share/native/sun/misc/NativeSignalHandler.c
 * \warning Function must be signal-safe.
 */
typedef void (*TSignalHandler)(int signo, void *siginfo, void *data);

/*!
 * \brief This class is handling signal.
 */
class TSignalManager {
 public:
  /*!
   * \brief TSignalManager constructor.
   * \param env  [in] JNI environment object.
   * \param proc [in] Pointer of signal callback.
   */
  TSignalManager(JNIEnv *env, TSignalHandler proc);

  /*!
   * \brief TSignalManager destructor.
   */
  virtual ~TSignalManager(void);

  /*!
   * \brief Add signal which is handled.
   * \param env     [in] JNI environment object.
   * \param sigName [in] Name of signal.
   * \return Process is succeed, if value is true.<br>
   *         Process is failure, if value is false.
   */
  bool catchSignal(JNIEnv *env, char *sigName);

  /*!
   * \brief Specify signal to ignore.
   * \param env     [in] JNI environment object.
   * \param sigName [in] Name of signal.
   * \return Process is succeed, if value is true.<br>
   *         Process is failure, if value is false.
   */
  bool ignoreSignal(JNIEnv *env, char *sigName);

  /*!
   * \brief Terminate handling signal.
   * \param env [in] JNI environment object.
   */
  void terminate(JNIEnv *env);

  /*!
   * \brief Global initialization.
   * \param env [in] JNI environment object.
   * \return Process is succeed, if value is true.<br>
   *         Process is failure, if value is false.
   * \warning Please call only once from main thread.
   */
  static bool globalInitialize(JNIEnv *env);

  /*!
   * \brief Global finalize.
   * \param env [in] JNI environment object.
   * \return Process is succeed, if value is true.<br>
   *         Process is failure, if value is false.
   * \warning Please call only once from main thread.
   */
  static bool globalFinalize(JNIEnv *env);

 protected:
  /*!
   * \brief Setting signal handle function.
   * \param env     [in] JNI environment object.
   * \param sigName [in] Name of signal.
   * \param handler [in] Setting signal handler.
   * \return Process is succeed, if value is true.<br>
   *         Process is failure, if value is false.
   */
  virtual bool setSignalHandler(JNIEnv *env, char *sigName, jobject handler);

  /*!
   * \brief Convert string from C++ native to java.
   * \param env       [in] JNI environment object.
   * \param nativeStr [in] String in C++.
   * \param javaStr   [in,out] String object in java.
   * \return Process is succeed, if value is true.<br>
   *         Process is failure, if value is false.
   */
  bool createJString(JNIEnv *env, char *nativeStr, jstring *javaStr);

  /*!
   * \brief Getting default signal handler in JVM.
   * \param env [in] JNI environment object.
   * \return Process is succeed, if value is true.<br>
   *         Process is failure, if value is false.
   */
  static bool getDefaultSignalHandler(JNIEnv *env);

  RELEASE_ONLY(private :)

  /*!
   * \brief Object of "NativeSignalHandler" class information.
   */
  static jclass classNativeHandler;
  /*!
   * \brief Object of "Signal" class information.
   */
  static jclass classSignal;
  /*!
   * \brief Object of "String" class information.
   */
  static jclass classString;

  /*!
   * \brief Constructor of "NativeSignalHandler" class.
   */
  static jmethodID nativeHandler_init;
  /*!
   * \brief Constructor of "Signal" class.
   */
  static jmethodID signal_init;
  /*!
   * \brief Constructor of "String" class.
   */
  static jmethodID string_init;
  /*!
   * \brief Method "handle" of "Signal" class.
   */
  static jmethodID signal_handle;

  /*!
   * \brief Default signal handler.
   */
  static jobject signal_DFL;

  /*!
   * \brief Flag of load initialize data.
   */
  static bool loadFlag;

  /*!
   * \brief Orignal signal handler.
   */
  jobject insSignalHandler;
};

#endif  // _SIGNAL_MNGR_H

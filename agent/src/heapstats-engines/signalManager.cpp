/*!
 * \file signalManager.cpp
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

#include "globals.hpp"
#include "signalManager.hpp"

/* Class static variables. */

/*!
 * \brief Object of "NativeSignalHandler" class information.
 */
jclass TSignalManager::classNativeHandler = NULL;
/*!
 * \brief Object of "Signal" class information.
 */
jclass TSignalManager::classSignal = NULL;
/*!
 * \brief Object of "String" class information.
 */
jclass TSignalManager::classString = NULL;

/*!
 * \brief Constructor of "NativeSignalHandler" class.
 */
jmethodID TSignalManager::nativeHandler_init = NULL;
/*!
 * \brief Constructor of "Signal" class.
 */
jmethodID TSignalManager::signal_init = NULL;
/*!
 * \brief Constructor of "String" class.
 */
jmethodID TSignalManager::string_init = NULL;
/*!
 * \brief Method "handle" of "Signal" class.
 */
jmethodID TSignalManager::signal_handle = NULL;

/*!
 * \brief Default signal handler.
 */
jobject TSignalManager::signal_DFL = NULL;

/*!
 * \brief Flag of load initialize data.
 */
bool TSignalManager::loadFlag = false;

/* Class methods. */

/*!
 * \brief TSignalManager constructor.
 * \param env  [in] JNI environment object.
 * \param proc [in] Pointer of signal callback.
 */
TSignalManager::TSignalManager(JNIEnv *env, TSignalHandler proc) {
  /* If don't loaded data yet. */
  if (unlikely(!loadFlag)) {
    throw "Didn't initialize signal handler yet.";
  }

  /* Create signal handler on JVM. */
  jobject localSigHandler = env->NewObject(
      classNativeHandler, nativeHandler_init, (jlong)((ptrdiff_t)proc));

  /* If failure create signal handler. */
  if (unlikely(localSigHandler == NULL)) {
    handlePendingException(env);
    throw "Failure create signal handler.";
  }

  /* Get global referrence. */
  insSignalHandler = env->NewGlobalRef(localSigHandler);

  /* If failure get global referrence. */
  if (unlikely(insSignalHandler == NULL)) {
    handlePendingException(env);
    env->DeleteLocalRef(localSigHandler);
    throw "Failure create signal handler.";
  }

  env->DeleteLocalRef(localSigHandler);
}

/*!
 * \brief TSignalManager destructor.
 */
TSignalManager::~TSignalManager(void) { /* None. */ }

/*!
 * \brief Add signal which is handled.
 * \param env     [in] JNI environment object.
 * \param sigName [in] Name of signal.
 * \return Process is succeed, if value is true.<br>
 *         Process is failure, if value is false.
 */
bool TSignalManager::catchSignal(JNIEnv *env, char *sigName) {
  /* Set user's signal handler. */
  return setSignalHandler(env, sigName, insSignalHandler);
}

/*!
 * \brief Specify signal to ignore.
 * \param env     [in] JNI environment object.
 * \param sigName [in] Name of signal.
 * \return Process is succeed, if value is true.<br>
 *         Process is failure, if value is false.
 */
bool TSignalManager::ignoreSignal(JNIEnv *env, char *sigName) {
  /* Set default signal handler. */
  return setSignalHandler(env, sigName, signal_DFL);
}

/*!
 * \brief Terminate handling signal.
 * \param env [in] JNI environment object.
 */
void TSignalManager::terminate(JNIEnv *env) {
  /* Sanity check. */
  if (likely(insSignalHandler != NULL)) {
    /* Delete global reference. */
    env->DeleteGlobalRef(insSignalHandler);
    insSignalHandler = NULL;
  }
}

/*!
 * \brief Global initialization.
 * \param env [in] JNI environment object.
 * \return Process is succeed, if value is true.<br>
 *         Process is failure, if value is false.
 * \warning Please call only once from main thread.
 */
bool TSignalManager::globalInitialize(JNIEnv *env) {
  /* If already loaded data. */
  if (loadFlag) {
    logger->printWarnMsg("Already initialized signal manager.");
    return true;
  }
  loadFlag = true;

  /* Search class. */

  /* Search class list. */
  struct {
    jclass *pClass;
    char className[255];
    jmethodID *pClassInit;
    char initArgs[255];
  } loadClassList[] = {
        {&classNativeHandler, "Lsun/misc/NativeSignalHandler;",
         &nativeHandler_init, "(J)V"},
        {&classSignal, "Lsun/misc/Signal;", &signal_init,
         "(Ljava/lang/String;)V"},
        {&classString, "Ljava/lang/String;", &string_init, "([B)V"},
        {NULL, {0}, NULL, {0}} /* End flag. */
    };

  /* Search class and get global reference. */
  for (int i = 0; loadClassList[i].pClass != NULL; i++) {
    /* Find java class. */
    jclass localRefClass = env->FindClass(loadClassList[i].className);
    /* If not found common class. */
    if (unlikely(localRefClass == NULL)) {
      handlePendingException(env);

      logger->printWarnMsg("Couldn't get common class.");
      return false;
    }

    /* Get global reference. */
    (*loadClassList[i].pClass) = (jclass)env->NewGlobalRef(localRefClass);
    /* If failure getting global reference. */
    if (unlikely((*loadClassList[i].pClass) == NULL)) {
      handlePendingException(env);

      logger->printWarnMsg("Couldn't get global reference.");
      return false;
    }

    /* Get class constructor. */
    (*loadClassList[i].pClassInit) =
        env->GetMethodID(localRefClass, "<init>", loadClassList[i].initArgs);
    /* If failure getting constructor. */
    if (unlikely((*loadClassList[i].pClassInit) == NULL)) {
      handlePendingException(env);

      logger->printWarnMsg("Couldn't get constructor of common class.");
      return false;
    }

    /* Clearup. */
    env->DeleteLocalRef(localRefClass);
  }

  /* Search method. */

  /* Get "Signal" class "handle" method. */
  signal_handle = env->GetStaticMethodID(
      classSignal, "handle",
      "(Lsun/misc/Signal;Lsun/misc/SignalHandler;)Lsun/misc/SignalHandler;");
  /* If failure getting "handle" function. */
  if (unlikely(signal_handle == NULL)) {
    handlePendingException(env);

    logger->printWarnMsg("Couldn't get function of signal handling.");
    return false;
  }

  /* Search default signal handler. */
  return getDefaultSignalHandler(env);
}

/*!
 * \brief Global finalize.
 * \param env [in] JNI environment object.
 * \return Process is succeed, if value is true.<br>
 *         Process is failure, if value is false.
 * \warning Please call only once from main thread.
 */
bool TSignalManager::globalFinalize(JNIEnv *env) {
  /* Sanity check. */
  if (!loadFlag) {
    logger->printWarnMsg("Didn't initialize signal manager yet.");
    return false;
  }
  loadFlag = false;

  /* Delete common data's global reference. */
  if (likely(classNativeHandler != NULL)) {
    env->DeleteGlobalRef(classNativeHandler);
    classNativeHandler = NULL;
  }
  if (likely(classSignal != NULL)) {
    env->DeleteGlobalRef(classSignal);
    classSignal = NULL;
  }
  if (likely(classString != NULL)) {
    env->DeleteGlobalRef(classString);
    classString = NULL;
  }
  if (likely(signal_DFL != NULL)) {
    env->DeleteGlobalRef(signal_DFL);
    signal_DFL = NULL;
  }

  return true;
}

/*!
 * \brief Setting signal handle function.
 * \param env     [in] JNI environment object.
 * \param sigName [in] Name of signal.
 * \param handler [in] Setting signal handler.
 * \return Process is succeed, if value is true.<br>
 *         Process is failure, if value is false.
 */
bool TSignalManager::setSignalHandler(JNIEnv *env, char *sigName,
                                      jobject handler) {
  /* Sanity check. */
  if (unlikely(insSignalHandler == NULL)) {
    logger->printWarnMsg("Illegal signal handler.");
    return false;
  }

  jobject insSignal = NULL;
  jobject olderHandler = NULL;
  jstring javaSigName = NULL;

  /* Create string in java (We should cut SIG from sigName) */
  if (unlikely(!createJString(env, sigName + 3, &javaSigName))) {
    logger->printWarnMsg("Couldn't create java string.");
    return false;
  }

  /* Create signal object */
  insSignal = env->NewObject(classSignal, signal_init, javaSigName);
  if (unlikely(insSignal == NULL)) {
    handlePendingException(env);

    logger->printWarnMsg("Couldn't create signal object.");
    env->DeleteLocalRef(javaSigName);
    return false;
  }

  /* Setting signal handler. */
  olderHandler = env->CallStaticObjectMethod(classSignal, signal_handle,
                                             insSignal, handler);
  /* If failure setting signal handler. */
  if (unlikely(env->ExceptionOccurred() != NULL)) {
    handlePendingException(env);

    logger->printWarnMsg("Couldn't set signal handler.");
    env->DeleteLocalRef(javaSigName);
    env->DeleteLocalRef(insSignal);
    return false;
  }

  /* Cleanup. */
  env->DeleteLocalRef(olderHandler);
  env->DeleteLocalRef(javaSigName);
  env->DeleteLocalRef(insSignal);

  return true;
}

/*!
 * \brief Convert string from C++ native to java.
 * \param env       [in] JNI environment object.
 * \param nativeStr [in] String in C++.
 * \param javaStr   [in,out] String object in java.
 * \return Process is succeed, if value is true.<br>
 *         Process is failure, if value is false.
 */
bool TSignalManager::createJString(JNIEnv *env, char *nativeStr,
                                   jstring *javaStr) {
  /* Sanity check. */
  if (unlikely(nativeStr == NULL || strlen(nativeStr) == 0)) {
    logger->printWarnMsg("Illegal native string.");
    return false;
  }

  jbyteArray byteArray = NULL;
  size_t strLen = strlen(nativeStr);

  /* Creaet source byte array. */
  byteArray = env->NewByteArray(strLen);
  if (unlikely(byteArray == NULL)) {
    handlePendingException(env);

    logger->printWarnMsg("Couldn't create byte array.");
    return false;
  }

  /* Copy native string to byte array. */
  env->SetByteArrayRegion(byteArray, 0, strLen, (jbyte *)nativeStr);
  if (unlikely(env->ExceptionOccurred() != NULL)) {
    handlePendingException(env);

    logger->printWarnMsg("Couldn't copy to byte array.");
    env->DeleteLocalRef(byteArray);
    return false;
  }

  /* Create java string object. */
  *javaStr = (jstring)env->NewObject(classString, string_init, byteArray);
  /* if failure create java string. */
  if (unlikely(*javaStr == NULL)) {
    handlePendingException(env);

    logger->printWarnMsg("Couldn't create string object.");
    env->DeleteLocalRef(byteArray);
    return false;
  }

  /* Cleanup. */
  env->DeleteLocalRef(byteArray);

  return true;
}

/*!
 * \brief Getting default signal handler in JVM.
 * \param env [in] JNI environment object.
 * \return Process is succeed, if value is true.<br>
 *         Process is failure, if value is false.
 */
bool TSignalManager::getDefaultSignalHandler(JNIEnv *env) {
  /* Find "SignalHandler" class. */
  jclass classSignalHandler = env->FindClass("Lsun/misc/SignalHandler;");
  /* If failure signalhandler class. */
  if (unlikely(classSignalHandler == NULL)) {
    handlePendingException(env);

    logger->printWarnMsg("Failure getting class of default signal handler.");
    return false;
  }

  /* Find static field. */
  jfieldID sigDFL = env->GetStaticFieldID(classSignalHandler, "SIG_DFL",
                                          "Lsun/misc/SignalHandler;");
  /* If not found default signal handler. */
  if (unlikely(sigDFL == NULL)) {
    handlePendingException(env);

    logger->printWarnMsg("Couldn't get fieldID of default signal handler.");
    env->DeleteLocalRef(classSignalHandler);
    return false;
  }

  /* Get static field reference. */
  jobject localSigDFL = env->GetStaticObjectField(classSignalHandler, sigDFL);
  /* If failure getting field. */
  if (unlikely(localSigDFL == NULL)) {
    handlePendingException(env);

    logger->printWarnMsg("Couldn't get value of default signal handler.");
    env->DeleteLocalRef(classSignalHandler);
    return false;
  }

  /* Get global reference. */
  signal_DFL = env->NewGlobalRef(localSigDFL);
  if (unlikely(signal_DFL == NULL)) {
    handlePendingException(env);

    logger->printWarnMsg("Couldn't get global reference.");
    env->DeleteLocalRef(localSigDFL);
    env->DeleteLocalRef(classSignalHandler);
    return false;
  }

  /* Cleanup. */
  env->DeleteLocalRef(localSigDFL);
  env->DeleteLocalRef(classSignalHandler);

  return true;
}

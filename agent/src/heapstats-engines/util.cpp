/*!
 * \file util.cpp
 * \brief This file is utilities.
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
#include "util.hpp"

/* Extern variables. */

/*!
 * \brief Page size got from sysconf.
 */
long systemPageSize =
#ifdef _SC_PAGESIZE
    sysconf(_SC_PAGESIZE);
#else
#ifdef _SC_PAGE_SIZE
    sysconf(_SC_PAGE_SIZE);
#else
    /* Suppose system page size to be 4KiByte. */
    4 * 1024;
#endif
#endif

/* Functions. */

/*!
 * \brief JVMTI error detector.
 * \param jvmti [in] JVMTI envrionment object.
 * \param error [in] JVMTI error code.
 * \return Param "error" is error code?(true/false)
 */
bool isError(jvmtiEnv *jvmti, jvmtiError error) {
  /* If param "error" is error code. */
  if (unlikely(error != JVMTI_ERROR_NONE)) {
    /* Get and output error message. */
    char *errStr = NULL;
    jvmti->GetErrorName(error, &errStr);
    logger->printWarnMsg(errStr);
    jvmti->Deallocate((unsigned char *)errStr);

    return true;
  }

  return false;
}

/*!
 * \brief Get system information.
 * \param env [in] JNI envrionment.
 * \param key [in] System property key.
 * \return String of system property.
 */
char *GetSystemProperty(JNIEnv *env, const char *key) {
  /* Convert string. */
  jstring key_str = env->NewStringUTF(key);

  /* Search class. */
  jclass sysClass = env->FindClass("Ljava/lang/System;");

  /* if raised exception. */
  if (env->ExceptionOccurred()) {
    env->ExceptionDescribe();
    env->ExceptionClear();

    logger->printWarnMsg("Get system class failed !");
    return NULL;
  }

  /* Search method. */
  jmethodID System_getProperty = env->GetStaticMethodID(
      sysClass, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;");

  /* If raised exception. */
  if (env->ExceptionOccurred()) {
    env->ExceptionDescribe();
    env->ExceptionClear();

    logger->printWarnMsg("Get system method failed !");
    return NULL;
  }

  /* Call method in find class. */

  jstring ret_str = (jstring)env->CallStaticObjectMethod(
      sysClass, System_getProperty, key_str);

  /* If got nothing or raised exception. */
  if ((ret_str == NULL) || env->ExceptionOccurred()) {
    env->ExceptionDescribe();
    env->ExceptionClear();

    logger->printWarnMsg("Get system properties failed !");
    return NULL;
  }

  /* Get result and clean up. */

  const char *ret_utf8 = env->GetStringUTFChars(ret_str, NULL);
  char *ret = NULL;

  /* If raised exception. */
  if (env->ExceptionOccurred()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
  } else if (ret_utf8 != NULL) {
    /* Copy and free if got samething. */
    ret = strdup(ret_utf8);
    env->ReleaseStringUTFChars(ret_str, ret_utf8);
  }

  /* Cleanup. */
  env->DeleteLocalRef(key_str);
  return ret;
}

/*!
 * \brief Get ClassUnload event index.
 * \param jvmti [in] JVMTI envrionment object.
 * \return ClassUnload event index.
 * \sa     hotspot/src/share/vm/prims/jvmtiExport.cpp<br>
 *         hotspot/src/share/vm/prims/jvmtiEventController.cpp<br>
 *         hotspot/src/share/vm/prims/jvmti.xml<br>
 *         in JDK.
 */
jint GetClassUnloadingExtEventIndex(jvmtiEnv *jvmti) {
  jint count, ret = -1;
  jvmtiExtensionEventInfo *events;

  /* Get extension events. */
  if (isError(jvmti, jvmti->GetExtensionEvents(&count, &events))) {
    /* Failure get extension events. */
    logger->printWarnMsg("Get JVMTI Extension Event failed!");
    return -1;

  } else if (count <= 0) {
    /* If extension event is none. */
    logger->printWarnMsg("VM has no JVMTI Extension Event!");
    if (events != NULL) {
      jvmti->Deallocate((unsigned char *)events);
    }

    return -1;
  }

  /* Check extension events. */
  for (int Cnt = 0; Cnt < count; Cnt++) {
    if (strcmp(events[Cnt].id, "com.sun.hotspot.events.ClassUnload") == 0) {
      ret = events[Cnt].extension_event_index;
      break;
    }
  }

  /* Clean up. */

  jvmti->Deallocate((unsigned char *)events);
  return ret;
}

/*!
 * \brief Replace old string in new string on string.
 * \param str    [in] Process target string.
 * \param oldStr [in] Targer of replacing.
 * \param newStr [in] A string will replace existing string.
 * \return String invoked replace.<br>Don't forget deallocate.
 */
char *strReplase(char const *str, char const *oldStr, char const *newStr) {
  char *strPos = NULL;
  char *chrPos = NULL;
  int oldStrCnt = 0;
  int oldStrLen = 0;
  char *newString = NULL;
  int newStrLen = 0;

  /* Sanity check. */
  if (unlikely(str == NULL || oldStr == NULL || newStr == NULL ||
               strlen(str) == 0 || strlen(oldStr) == 0)) {
    logger->printWarnMsg("Illegal string replacing paramters.");
    return NULL;
  }

  oldStrLen = strlen(oldStr);
  strPos = (char *)str;
  /* Counting oldStr */
  while (true) {
    /* Find old string. */
    chrPos = strstr(strPos, oldStr);
    if (chrPos == NULL) {
      /* All old string is already found. */
      break;
    }

    /* Counting. */
    oldStrCnt++;
    /* Move next. */
    strPos = chrPos + oldStrLen;
  }

  newStrLen = strlen(newStr);
  /* Allocate result string memory. */
  newString = (char *)calloc(
      1, strlen(str) + (newStrLen * oldStrCnt) - (oldStrLen * oldStrCnt) + 1);
  /* If failure allocate result string. */
  if (unlikely(newString == NULL)) {
    logger->printWarnMsg("Failure allocate replaced string.");
    return NULL;
  }

  strPos = (char *)str;
  while (true) {
    /* Find old string. */
    chrPos = strstr(strPos, oldStr);
    if (unlikely(chrPos == NULL)) {
      /* all old string is replaced. */
      strcat(newString, strPos);
      break;
    }
    /* Copy from string exclude old string. */
    strncat(newString, strPos, (chrPos - strPos));
    /* Copy from new string. */
    strcat(newString, newStr);

    /* Move next. */
    strPos = chrPos + oldStrLen;
  }

  /* Succeed. */
  return newString;
}

/*!
 * \brief Get now date and time.
 * \return Mili-second elapsed time from 1970/1/1 0:00:00.
 */
jlong getNowTimeSec(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (jlong)tv.tv_sec * 1000 + (jlong)tv.tv_usec / 1000;
}

/*!
 * \brief A little sleep.
 * \param sec  [in] Second of sleep range.
 * \param nsec [in] Nano second of sleep range.
 */
void littleSleep(const long int sec, const long int nsec) {
  /* Timer setting variables. */
  int result = 0;
  struct timespec buf1;
  struct timespec buf2 = {sec, nsec};

  struct timespec *req = &buf1;
  struct timespec *rem = &buf2;

  /* Wait loop. */
  do {
    /* Reset and exchange time. */
    memset(req, 0, sizeof(timespec));

    struct timespec *tmp = req;
    req = rem;
    rem = tmp;

    errno = 0;

    /* Sleep. */
    result = nanosleep(req, rem);
  } while (result != 0 && errno == EINTR);
}

/*!
 * \brief Get thread information.
 * \param jvmti  [in]  JVMTI environment object.
 * \param env    [in]  JNI environment object.
 * \param thread [in]  Thread object in java.
 * \param info   [out] Record stored thread information.
 */
void getThreadDetailInfo(jvmtiEnv *jvmti, JNIEnv *env, jthread thread,
                         TJavaThreadInfo *info) {
  /* Get thread basic information. */
  jvmtiThreadInfo threadInfo = {0};
  if (unlikely(isError(jvmti, jvmti->GetThreadInfo(thread, &threadInfo)))) {
    /* Setting dummy information. */
    info->name = strdup("Unknown-Thread");
    info->isDaemon = false;
    info->priority = 0;
  } else {
    /* Setting information. */
    info->name = strdup(threadInfo.name);
    info->isDaemon = (threadInfo.is_daemon == JNI_TRUE);
    info->priority = threadInfo.priority;

    /* Cleanup. */
    jvmti->Deallocate((unsigned char *)threadInfo.name);
    env->DeleteLocalRef(threadInfo.thread_group);
    env->DeleteLocalRef(threadInfo.context_class_loader);
  }

  /* Get thread state. */
  jint state = 0;
  jvmti->GetThreadState(thread, &state);

  /* Set thread state. */
  switch (state & JVMTI_JAVA_LANG_THREAD_STATE_MASK) {
    case JVMTI_JAVA_LANG_THREAD_STATE_NEW:
      info->state = strdup("NEW");
      break;
    case JVMTI_JAVA_LANG_THREAD_STATE_TERMINATED:
      info->state = strdup("TERMINATED");
      break;
    case JVMTI_JAVA_LANG_THREAD_STATE_RUNNABLE:
      info->state = strdup("RUNNABLE");
      break;
    case JVMTI_JAVA_LANG_THREAD_STATE_BLOCKED:
      info->state = strdup("BLOCKED");
      break;
    case JVMTI_JAVA_LANG_THREAD_STATE_WAITING:
      info->state = strdup("WAITING");
      break;
    case JVMTI_JAVA_LANG_THREAD_STATE_TIMED_WAITING:
      info->state = strdup("TIMED_WAITING");
      break;
    default:
      info->state = NULL;
  }
}

/*!
 * \brief Get method information in designed stack frame.
 * \param jvmti [in]  JVMTI environment object.
 * \param env   [in]  JNI environment object.
 * \param frame [in]  method stack frame.
 * \param info  [out] Record stored method information in stack frame.
 */
void getMethodFrameInfo(jvmtiEnv *jvmti, JNIEnv *env, jvmtiFrameInfo frame,
                        TJavaStackMethodInfo *info) {
  char *tempStr = NULL;
  jclass declareClass = NULL;

  /* Get method class. */
  if (unlikely(isError(jvmti, jvmti->GetMethodDeclaringClass(frame.method,
                                                             &declareClass)))) {
    info->className = NULL;
    info->sourceFile = NULL;
  } else {
    /* Get class signature. */
    if (unlikely(isError(
            jvmti, jvmti->GetClassSignature(declareClass, &tempStr, NULL)))) {
      info->className = NULL;
    } else {
      info->className = strdup(tempStr);
      jvmti->Deallocate((unsigned char *)tempStr);
    }

    /* Get source filename. */
    if (unlikely(
            isError(jvmti, jvmti->GetSourceFileName(declareClass, &tempStr)))) {
      info->sourceFile = NULL;
    } else {
      info->sourceFile = strdup(tempStr);
      jvmti->Deallocate((unsigned char *)tempStr);
    }

    env->DeleteLocalRef(declareClass);
  }

  /* Get method name. */
  if (unlikely(isError(
          jvmti, jvmti->GetMethodName(frame.method, &tempStr, NULL, NULL)))) {
    info->methodName = NULL;
  } else {
    info->methodName = strdup(tempStr);
    jvmti->Deallocate((unsigned char *)tempStr);
  }

  /* Check method is native. */
  jboolean isNativeMethod = JNI_TRUE;
  jvmti->IsMethodNative(frame.method, &isNativeMethod);

  if (unlikely(isNativeMethod == JNI_TRUE)) {
    /* method is native. */
    info->isNative = true;
    info->lineNumber = -1;
  } else {
    /* method is java method. */
    info->isNative = false;

    /* Get source code line number. */
    jint entriyCount = 0;
    jvmtiLineNumberEntry *entries = NULL;
    if (unlikely(isError(jvmti, jvmti->GetLineNumberTable(
                                    frame.method, &entriyCount, &entries)))) {
      /* Method location is unknown. */
      info->lineNumber = -1;
    } else {
      /* Search running line. */
      jint lineIdx = 0;
      entriyCount--;
      for (; lineIdx < entriyCount; lineIdx++) {
        if (frame.location <= entries[lineIdx].start_location) {
          break;
        }
      }

      info->lineNumber = entries[lineIdx].line_number;
      jvmti->Deallocate((unsigned char *)entries);
    }
  }
}

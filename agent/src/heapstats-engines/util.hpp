/*!
 * \file util.hpp
 * \brief This file is utilities.
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

#ifndef UTIL_HPP
#define UTIL_HPP

#include <jvmti.h>
#include <jni.h>

#ifdef HAVE_ATOMIC
#include <atomic>
#else
#include <cstdatomic>
#endif

#include <stddef.h>
#include <errno.h>
#include <string.h>

/* Branch prediction. */

/*!
 * \brief Positive branch prediction.
 */
#define likely(x) __builtin_expect(!!(x), 1)

/*!
 * \brief Negative branch prediction.
 */
#define unlikely(x) __builtin_expect(!!(x), 0)

/* Critical section helper macro for pthread mutex. */

/*!
 * \brief Enter critical pthread section macro.
 */
#define ENTER_PTHREAD_SECTION(monitor)                \
  if (unlikely(pthread_mutex_lock((monitor)) != 0)) { \
    logger->printWarnMsg("Entering mutex failed!");   \
  } else {
/*!
 * \brief Exit critical pthread section macro.
 */
#define EXIT_PTHREAD_SECTION(monitor)                   \
  if (unlikely(pthread_mutex_unlock((monitor)) != 0)) { \
    logger->printWarnMsg("Exiting mutex failed!");      \
  }                                                     \
  }

/*!
 * \brief Calculate align macro.
 *        from hotspot/src/share/vm/utilities/globalDefinitions.hpp
 */
#define ALIGN_SIZE_UP(size, alignment) \
  (((size) + ((alignment)-1)) & ~((alignment)-1))

#ifdef __LP64__

/*!
 * \brief Format string for jlong.<br />
 *        At "LP64" java environment, jlong is defined as "long int".
 */
#define JLONG_FORMAT_STR "%ld"

#define JLONG_MAX LONG_MAX
#else

/*!
 * \brief Format string for jlong.<br />
 *        At most java environment, jlong is defined as "long long int".
 */
#define JLONG_FORMAT_STR "%lld"

#define JLONG_MAX LLONG_MAX
#endif

#ifdef DEBUG

/*!
 * \brief This macro for statement only debug.
 */
#define DEBUG_ONLY(statement) statement

/*!
 * \brief This macro for statement only release.
 */
#define RELEASE_ONLY(statement)
#else

/*!
 * \brief This macro for statement only debug.
 */
#define DEBUG_ONLY(statement)

/*!
 * \brief This macro for statement only release.
 */
#define RELEASE_ONLY(statement) statement
#endif

/*!
 * \brief Debug macro.
 */
#define STATEMENT_BY_MODE(debug_state, release_state) \
  DEBUG_ONLY(debug_state) RELEASE_ONLY(release_state)

/* Agent_OnLoad/JNICALL Agent_OnAttach return code. */

/*!
 * \brief Process is successed.
 */
#define SUCCESS 0x00

/*!
 * \brief Failure set capabilities.
 */
#define CAPABILITIES_SETTING_FAILED 0x01
/*!
 * \brief Failure set callbacks.
 */
#define CALLBACKS_SETTING_FAILED 0x02
/*!
 * \brief Failure initialize container.
 */
#define CLASSCONTAINER_INITIALIZE_FAILED 0x03
/*!
 * \brief Failure initialize agent thread.
 */
#define AGENT_THREAD_INITIALIZE_FAILED 0x04
/*!
 * \brief Failure create monitor.
 */
#define MONITOR_CREATION_FAILED 0x05
/*!
 * \brief Failure get environment.
 */
#define GET_ENVIRONMENT_FAILED 0x06
/*!
 * \brief Failure get low level information.<br>
 *        E.g. symbol, VMStructs, etc..
 */
#define GET_LOW_LEVEL_INFO_FAILED 0x07
/*!
 * \brief Configuration is not valid.
 */
#define INVALID_CONFIGURATION 0x07
/*!
 * \brief SNMP setup failed.
 */
#define SNMP_SETUP_FAILED 0x08
/*!
 * \brief Deadlock detector setup failed.
 */
#define DLDETECTOR_SETUP_FAILED 0x09

/*!
 * \brief This macro is notification catch signal to signal watcher thread.
 */
#define NOTIFY_CATCH_SIGNAL intervalSigTimer->notify();

/* Byte order mark. */
#ifdef WORDS_BIGENDIAN
/*!
 * \brief Big endian.<br>
 *          e.g. SPARC, PowerPC, etc...
 */
#define BOM 'B'
#else
/*!
 * \brief Little endian.<br>
 *          e.g. i386, AMD64, etc..
 */
#define BOM 'L'
#endif

/* Typedef. */

/*!
 * \brief Causes of function invoking.
 */
typedef enum {
  GC = 1,                /*!< Invoke by Garbage Collection.     */
  DataDumpRequest = 2,   /*!< Invoke by user dump request.      */
  Interval = 3,          /*!< Invoke by timer interval.         */
  Signal = 4,            /*!< Invoke by user's signal.          */
  AnotherSignal = 5,     /*!< Invoke by user's another signal.  */
  ResourceExhausted = 6, /*!< Invoke by JVM resource exhausted. */
  ThreadExhausted = 7,   /*!< Invoke by thread exhausted.       */
  OccurredDeadlock = 8   /*!< Invoke by occured deadlock.       */
} TInvokeCause;

/*!
 * \brief Java thread information structure.
 */
typedef struct {
  char *name;    /*!< Name of java thread.             */
  bool isDaemon; /*!< Flag of thread is daemon thread. */
  int priority;  /*!< Priority of thread in java.      */
  char *state;   /*!< String about thread state.       */
} TJavaThreadInfo;

/*!
 * \brief Java method information structure in stack frame.
 */
typedef struct {
  char *methodName; /*!< Name of class method.                               */
  char *className;  /*!< Name of class is declaring method.                  */
  bool isNative;    /*!< Flag of method is native.                           */
  char *sourceFile; /*!< Name of file name is declaring class.               */
  int lineNumber;   /*!< Line number of method's executing position in file. */
} TJavaStackMethodInfo;

/*!
 * \brief This type is common large size int.
 */
typedef unsigned long long int TLargeUInt;

/*!
 * \brief This type is used by store datetime is mili-second unit.
 */
typedef unsigned long long int TMSecTime;

/*!
 * \brief Interval of check signal by signal watch timer.(msec)
 */
const unsigned int SIG_WATCHER_INTERVAL = 0;

/* Export functions. */

/*!
 * \brief JVMTI error detector.
 * \param jvmti [in] JVMTI envrionment object.
 * \param error [in] JVMTI error code.
 * \return Param "error" is error code?(true/false)
 */
bool isError(jvmtiEnv *jvmti, jvmtiError error);

/*!
 * \brief Get system information.
 * \param env [in] JNI envrionment.
 * \param key [in] System property key.
 * \return String of system property.
 */
char *GetSystemProperty(JNIEnv *env, const char *key);

/*!
 * \brief Get ClassUnload event index.
 * \param jvmti [in] JVMTI envrionment object.
 * \return ClassUnload event index.
 * \sa     hotspot/src/share/vm/prims/jvmtiExport.cpp<br>
 *         hotspot/src/share/vm/prims/jvmtiEventController.cpp<br>
 *         hotspot/src/share/vm/prims/jvmti.xml<br>
 *         in JDK.
 */
jint GetClassUnloadingExtEventIndex(jvmtiEnv *jvmti);

/*!
 * \brief Replace old string in new string on string.
 * \param str    [in] Process target string.
 * \param oldStr [in] Targer of replacing.
 * \param newStr [in] A string will replace existing string.
 * \return String invoked replace.<br>Don't forget deallocate.
 */
char *strReplase(char const *str, char const *oldStr, char const *newStr);

/*!
 * \brief Get now date and time.
 * \return Mili-second elapsed time from 1970/1/1 0:00:00.
 */
jlong getNowTimeSec(void);

/*!
 * \brief A little sleep.
 * \param sec  [in] Second of sleep range.
 * \param nsec [in] Nano second of sleep range.
 */
void littleSleep(const long int sec, const long int nsec);

/*!
 * \brief Get thread information.
 * \param jvmti  [in]  JVMTI environment object.
 * \param env    [in]  JNI environment object.
 * \param thread [in]  Thread object in java.
 * \param info   [out] Record stored thread information.
 */
void getThreadDetailInfo(jvmtiEnv *jvmti, JNIEnv *env, jthread thread,
                         TJavaThreadInfo *info);

/*!
 * \brief Get method information in designed stack frame.
 * \param jvmti [in]  JVMTI environment object.
 * \param env   [in]  JNI environment object.
 * \param frame [in]  method stack frame.
 * \param info  [out] Record stored method information in stack frame.
 */
void getMethodFrameInfo(jvmtiEnv *jvmti, JNIEnv *env, jvmtiFrameInfo frame,
                        TJavaStackMethodInfo *info);

/*!
 * \brief Handling Java Exception if exists.
 * \param env [in] JNI environment object.
 */
inline void handlePendingException(JNIEnv *env) {
  /* If exists execption in java. */
  if (env->ExceptionOccurred() != NULL) {
    /* Clear execption in java. */
    env->ExceptionDescribe();
    env->ExceptionClear();
  }
};

/*!
 * \brief Add to address.
 * \param addr [in] Target address.
 * \param inc  [in] Incremental integer value.
 * \return The address added increment value.
 */
inline void *incAddress(void *addr, off_t inc) {
  return (void *)((ptrdiff_t)addr + inc);
};

/*!
 * \brief Wrapper function of strerror_r().
 * \param buf     [in] Buffer of error string.
 * \param buflen  [in] Length of buf.
 * \return Pointer to error string.
 */
inline char *strerror_wrapper(char *buf, size_t buflen) {
  char *output_message;

#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !_GNU_SOURCE
  /* XSI-compliant version of strerror_r() */
  strerror_r(errno, buf, buflen);
  output_message = errno_string;
#else
  /* GNU-specific version of strerror_r() */
  output_message = strerror_r(errno, buf, buflen);
#endif

  return output_message;
};

/* Classes. */

/*!
 * \brief Hasher class for std::tr1::unordered_map.
 *        This template class will be used when key is numeric type.
 *        (int, pointer, etc)
 */
template <typename T>
class TNumericalHasher {
 public:
  /*!
   * \brief Get hash value from designated value.
   *        This function always return convert to integer only.
   * \param val [in] Hash source data.
   * \return Hash value.
   */
  size_t operator()(const T &val) const { return (size_t)val; };
};

/*!
 * \brief Utility class for flag of processing.
 */
class TProcessMark {

  private:
    std::atomic_int &flag;

  public:
    TProcessMark(std::atomic_int &flg) : flag(flg) { flag++; }
    ~TProcessMark() { flag--; }

};

/* CPU Specific utilities. */
#ifdef AVX
#include "arch/x86/avx/util.hpp"
#elif defined(SSE2) || defined(SSE3) || defined(SSE4)
#include "arch/x86/sse2/util.hpp"
#elif defined(NEON)
#include "arch/arm/neon/util.hpp"
#else
inline void memcpy32(void *dest, const void *src) {
  memcpy(dest, src, 32);
};
#endif

#endif

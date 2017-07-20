/*!
 * \file jniCallbackRegister.hpp
 * \brief Handling JNI function callback.
 * Copyright (C) 2015-2017 Yasumasa Suenaga
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

#ifndef JNI_CALLBACK_REGISTER_HPP
#define JNI_CALLBACK_REGISTER_HPP

#include <jvmti.h>
#include <jni.h>

#include <pthread.h>

#include <list>

/* Type definition for JNI functions. */
typedef void (*TJVM_Sleep)(JNIEnv *env, jclass threadClass, jlong millis);
typedef void (*TUnsafe_Park)(JNIEnv *env, jobject unsafe, jboolean isAbsolute,
                             jlong time);

/* Import functions from libjvm.so */
extern "C" void JVM_Sleep(JNIEnv *env, jclass threadClass, jlong millis);

/*!
 * \brief Base class of callback for JNI functions.
 */
template <typename T>
class TJNICallbackRegister {
 protected:
  /*!
   * \brief Callback function list before calling JNI function.
   */
  static std::list<T> prologueCallbackList;

  /*!
   * \brief Callback function list after calling JNI function.
   */
  static std::list<T> epilogueCallbackList;

  /*!
   * \brief Read-Write lock for callback container.
   */
  static pthread_rwlock_t callbackLock;

 public:
  /*!
   * \brief Register callbacks for JNI function.
   *
   * \param prologue [in] Callback before calling JNI function.
   * \param epilogue [in] Callback after calling JNI function.
   */
  static void registerCallback(T prologue, T epilogue) {
    pthread_rwlock_wrlock(&callbackLock);
    {
      if (prologue != NULL) {
        prologueCallbackList.push_back(prologue);
      }

      if (epilogue != NULL) {
        epilogueCallbackList.push_back(epilogue);
      }
    }
    pthread_rwlock_unlock(&callbackLock);
  };

  /*!
   * \brief Unegister callbacks for JNI function.
   *
   * \param prologue [in] Callback before calling JNI function.
   * \param epilogue [in] Callback after calling JNI function.
   */
  static void unregisterCallback(T prologue, T epilogue) {
    pthread_rwlock_wrlock(&callbackLock);
    {
      if (prologue != NULL) {
        for (auto itr = prologueCallbackList.cbegin();
             itr != prologueCallbackList.cend(); itr++) {
          if (prologue == *itr) {
            prologueCallbackList.erase(itr);
            break;
          }
        }
      }

      if (epilogue != NULL) {
        for (auto itr = epilogueCallbackList.cbegin();
             itr != epilogueCallbackList.cend(); itr++) {
          if (epilogue == *itr) {
            epilogueCallbackList.erase(itr);
            break;
          }
        }
      }
    }
    pthread_rwlock_unlock(&callbackLock);
  };
};

/*!
 * \brief JNI callbacks for prologue.
 */
template <typename T>
std::list<T> TJNICallbackRegister<T>::prologueCallbackList;

/*!
 * \brief JNI callbacks for epilogue.
 */
template <typename T>
std::list<T> TJNICallbackRegister<T>::epilogueCallbackList;

/*!
 * \brief Read-Write lock for callback container.
 */
template <typename T>
pthread_rwlock_t TJNICallbackRegister<T>::callbackLock =
                                              PTHREAD_RWLOCK_INITIALIZER;

#define ITERATE_JNI_CALLBACK_CHAIN(type, originalFunc, ...)                   \
  std::list<type>::iterator itr;                                              \
                                                                              \
  pthread_rwlock_rdlock(&callbackLock);                                       \
  {                                                                           \
    for (itr = prologueCallbackList.begin();                                  \
         itr != prologueCallbackList.end(); itr++) {                          \
      (*itr)(__VA_ARGS__);                                                    \
    }                                                                         \
  }                                                                           \
  pthread_rwlock_unlock(&callbackLock);                                       \
                                                                              \
  originalFunc(__VA_ARGS__);                                                  \
                                                                              \
  pthread_rwlock_rdlock(&callbackLock);                                       \
  {                                                                           \
    for (itr = epilogueCallbackList.begin();                                  \
         itr != epilogueCallbackList.end(); itr++) {                          \
      (*itr)(__VA_ARGS__);                                                    \
    }                                                                         \
  }                                                                           \
  pthread_rwlock_unlock(&callbackLock);

/*!
 * \brief JVM_Sleep callback (java.lang.System#sleep())
 */
class TJVMSleepCallback : public TJNICallbackRegister<TJVM_Sleep> {
 public:
  static void JNICALL
      callbackStub(JNIEnv *env, jclass threadClass, jlong millis){
          ITERATE_JNI_CALLBACK_CHAIN(TJVM_Sleep, JVM_Sleep, env, threadClass,
                                     millis)};

  static bool switchCallback(JNIEnv *env, bool isEnable) {
    jclass threadClass = env->FindClass("java/lang/Thread");

    JNINativeMethod sleepMethod;
    sleepMethod.name = (char *)"sleep";
    sleepMethod.signature = (char *)"(J)V";
    sleepMethod.fnPtr = isEnable ? (void *)&callbackStub : (void *)&JVM_Sleep;

    return (env->RegisterNatives(threadClass, &sleepMethod, 1) == 0);
  }
};

/*!
 * \brief Unsafe_Park callback (sun.misc.Unsafe#park())
 */
class TUnsafeParkCallback : public TJNICallbackRegister<TUnsafe_Park> {
 public:
  static void JNICALL callbackStub(JNIEnv *env, jobject unsafe,
                                   jboolean isAbsolute, jlong time){
      ITERATE_JNI_CALLBACK_CHAIN(TUnsafe_Park,
                                 TVMFunctions::getInstance()->Unsafe_Park, env,
                                 unsafe, isAbsolute, time)};

  static bool switchCallback(JNIEnv *env, bool isEnable) {
    jclass unsafeClass = env->FindClass("sun/misc/Unsafe");

    JNINativeMethod parkMethod;
    parkMethod.name = (char *)"park";
    parkMethod.signature = (char *)"(ZJ)V";
    parkMethod.fnPtr =
        isEnable ? (void *)&callbackStub
                 : TVMFunctions::getInstance()->GetUnsafe_ParkPointer();

    return (env->RegisterNatives(unsafeClass, &parkMethod, 1) == 0);
  }
};

#endif  // JNI_CALLBACK_REGISTER_HPP

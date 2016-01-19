/*!
 * \file callbackRegister.hpp
 * \brief Handling JVMTI callback functions.
 * Copyright (C) 2015 Yasumasa Suenaga
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

#ifndef CALLBACK_REGISTER_HPP
#define CALLBACK_REGISTER_HPP

#include <jvmti.h>
#include <jni.h>

#include <list>

/*!
 * \brief Base class of JVMTI event callback.
 */
template <typename T, jvmtiEvent event>
class TJVMTIEventCallback {
 protected:
  /*!
   * \brief JVMTI callback function list.
   */
  static std::list<T> callbackList;

 public:
  /*!
   * \brief Register JVMTI callback function.
   *
   * \param callback [in] Callback function.
   */
  static void registerCallback(T callback) {
    callbackList.push_back(callback);
  };

  /*!
   * \brief Merge JVMTI capabilities for this JVMTI event.
   *
   * \param capabilities [out] Capabilities to merge.
   */
  static void mergeCapabilities(jvmtiCapabilities *capabilities){
      // Nothing to do in default.
  };

  /*!
   * \brief Merge JVMTI callback for this JVMTI event.
   *
   * \param callbacks [out] Callbacks to merge.
   */
  static void mergeCallback(jvmtiEventCallbacks *callbacks){
      // Nothing to do in deafault.
  };

  /*!
   * \brief Switch JVMTI event notification.
   *
   * \param jvmti [in] JVMTI environment.
   * \param mode [in] Switch of this JVMTI event.
   */
  static bool switchEventNotification(jvmtiEnv *jvmti, jvmtiEventMode mode) {
    return isError(jvmti, jvmti->SetEventNotificationMode(mode, event, NULL));
  }
};

/*!
 * \brief JVMTI callback list definition.
 */
template <typename T, jvmtiEvent event>
std::list<T> TJVMTIEventCallback<T, event>::callbackList;

#define ITERATE_CALLBACK_CHAIN(type, ...)                    \
  for (std::list<type>::iterator itr = callbackList.begin(); \
       itr != callbackList.end(); itr++) {                   \
    (*itr)(__VA_ARGS__);                                     \
  }

#define DEFINE_MERGE_CALLBACK(callbackName)                   \
  static void mergeCallback(jvmtiEventCallbacks *callbacks) { \
    if (callbackList.size() == 0) {                           \
      callbacks->callbackName = NULL;                         \
    } else if (callbackList.size() == 1) {                    \
      callbacks->callbackName = *callbackList.begin();        \
    } else {                                                  \
      callbacks->callbackName = &callbackStub;                \
    }                                                         \
  };

/*!
 * \brief JVMTI ClassPrepare callback.
 */
class TClassPrepareCallback
    : public TJVMTIEventCallback<jvmtiEventClassPrepare,
                                 JVMTI_EVENT_CLASS_PREPARE> {
 public:
  static void JNICALL
      callbackStub(jvmtiEnv *jvmti, JNIEnv *env, jthread thread, jclass klass){
          ITERATE_CALLBACK_CHAIN(jvmtiEventClassPrepare, jvmti, env, thread,
                                 klass)};

  DEFINE_MERGE_CALLBACK(ClassPrepare)
};

/*!
 * \brief JVMTI DumpRequest callback.
 */
class TDataDumpRequestCallback
    : public TJVMTIEventCallback<jvmtiEventDataDumpRequest,
                                 JVMTI_EVENT_DATA_DUMP_REQUEST> {
 public:
  static void JNICALL callbackStub(jvmtiEnv *jvmti){
      ITERATE_CALLBACK_CHAIN(jvmtiEventDataDumpRequest, jvmti)};

  DEFINE_MERGE_CALLBACK(DataDumpRequest)
};

/*!
 * \brief JVMTI GarbageCollectionStart callback.
 */
class TGarbageCollectionStartCallback
    : public TJVMTIEventCallback<jvmtiEventGarbageCollectionStart,
                                 JVMTI_EVENT_GARBAGE_COLLECTION_START> {
 public:
  static void JNICALL callbackStub(jvmtiEnv *jvmti){
      ITERATE_CALLBACK_CHAIN(jvmtiEventGarbageCollectionStart, jvmti)};

  DEFINE_MERGE_CALLBACK(GarbageCollectionStart)

  static void mergeCapabilities(jvmtiCapabilities *capabilities) {
    capabilities->can_generate_garbage_collection_events = 1;
  };
};

/*!
 * \brief JVMTI GarbageCollectionFinish callback.
 */
class TGarbageCollectionFinishCallback
    : public TJVMTIEventCallback<jvmtiEventGarbageCollectionFinish,
                                 JVMTI_EVENT_GARBAGE_COLLECTION_FINISH> {
 public:
  static void JNICALL callbackStub(jvmtiEnv *jvmti){
      ITERATE_CALLBACK_CHAIN(jvmtiEventGarbageCollectionFinish, jvmti)};

  DEFINE_MERGE_CALLBACK(GarbageCollectionFinish)

  static void mergeCapabilities(jvmtiCapabilities *capabilities) {
    capabilities->can_generate_garbage_collection_events = 1;
  };
};

/*!
 * \brief JVMTI ResourceExhausted callback.
 */
class TResourceExhaustedCallback
    : public TJVMTIEventCallback<jvmtiEventResourceExhausted,
                                 JVMTI_EVENT_RESOURCE_EXHAUSTED> {
 public:
  static void JNICALL callbackStub(jvmtiEnv *jvmti, JNIEnv *env, jint flags,
                                   const void *reserved,
                                   const char *description) {
    ITERATE_CALLBACK_CHAIN(jvmtiEventResourceExhausted, jvmti, env, flags,
                           reserved, description);
  };

  DEFINE_MERGE_CALLBACK(ResourceExhausted)

  static void mergeCapabilities(jvmtiCapabilities *capabilities) {
    capabilities->can_generate_resource_exhaustion_heap_events = 1;
    capabilities->can_generate_resource_exhaustion_threads_events = 1;
  };
};

/*!
 * \brief JVMTI MonitorContendedEnter callback.
 */
class TMonitorContendedEnterCallback
    : public TJVMTIEventCallback<jvmtiEventMonitorContendedEnter,
                                 JVMTI_EVENT_MONITOR_CONTENDED_ENTER> {
 public:
  static void JNICALL callbackStub(jvmtiEnv *jvmti, JNIEnv *env, jthread thread,
                                   jobject object) {
    ITERATE_CALLBACK_CHAIN(jvmtiEventMonitorContendedEnter, jvmti, env, thread,
                           object);
  };

  DEFINE_MERGE_CALLBACK(MonitorContendedEnter)

  static void mergeCapabilities(jvmtiCapabilities *capabilities) {
    capabilities->can_generate_monitor_events = 1;
  };
};

/*!
 * \brief JVMTI MonitorContendedEntered callback.
 */
class TMonitorContendedEnteredCallback
    : public TJVMTIEventCallback<jvmtiEventMonitorContendedEntered,
                                 JVMTI_EVENT_MONITOR_CONTENDED_ENTERED> {
 public:
  static void JNICALL callbackStub(jvmtiEnv *jvmti, JNIEnv *env, jthread thread,
                                   jobject object) {
    ITERATE_CALLBACK_CHAIN(jvmtiEventMonitorContendedEntered, jvmti, env,
                           thread, object);
  };

  DEFINE_MERGE_CALLBACK(MonitorContendedEntered)

  static void mergeCapabilities(jvmtiCapabilities *capabilities) {
    capabilities->can_generate_monitor_events = 1;
  };
};

/*!
 * \brief JVMTI VMInit callback.
 */
class TVMInitCallback
    : public TJVMTIEventCallback<jvmtiEventVMInit, JVMTI_EVENT_VM_INIT> {
 public:
  static void JNICALL
      callbackStub(jvmtiEnv *jvmti, JNIEnv *env, jthread thread) {
    ITERATE_CALLBACK_CHAIN(jvmtiEventVMInit, jvmti, env, thread);
  };

  DEFINE_MERGE_CALLBACK(VMInit)
};

/*!
 * \brief JVMTI VMDeath callback.
 */
class TVMDeathCallback
    : public TJVMTIEventCallback<jvmtiEventVMDeath, JVMTI_EVENT_VM_DEATH> {
 public:
  static void JNICALL callbackStub(jvmtiEnv *jvmti, JNIEnv *env) {
    ITERATE_CALLBACK_CHAIN(jvmtiEventVMDeath, jvmti, env);
  };

  DEFINE_MERGE_CALLBACK(VMDeath)
};

/*!
 * \brief JVMTI ThreadStart callback.
 */
class TThreadStartCallback
    : public TJVMTIEventCallback<jvmtiEventThreadStart,
                                 JVMTI_EVENT_THREAD_START> {
 public:
  static void JNICALL
      callbackStub(jvmtiEnv *jvmti, JNIEnv *env, jthread thread) {
    ITERATE_CALLBACK_CHAIN(jvmtiEventThreadStart, jvmti, env, thread);
  };

  DEFINE_MERGE_CALLBACK(ThreadStart)
};

/*!
 * \brief JVMTI ThreadEnd callback.
 */
class TThreadEndCallback
    : public TJVMTIEventCallback<jvmtiEventThreadEnd, JVMTI_EVENT_THREAD_END> {
 public:
  static void JNICALL
      callbackStub(jvmtiEnv *jvmti, JNIEnv *env, jthread thread) {
    ITERATE_CALLBACK_CHAIN(jvmtiEventThreadEnd, jvmti, env, thread);
  };

  DEFINE_MERGE_CALLBACK(ThreadEnd)
};

/*!
 * \brief JVMTI MonitorWait callback.
 */
class TMonitorWaitCallback
    : public TJVMTIEventCallback<jvmtiEventMonitorWait,
                                 JVMTI_EVENT_MONITOR_WAIT> {
 public:
  static void JNICALL callbackStub(jvmtiEnv *jvmti, JNIEnv *env, jthread thread,
                                   jobject object, jlong timeout) {
    ITERATE_CALLBACK_CHAIN(jvmtiEventMonitorWait, jvmti, env, thread, object,
                           timeout);
  };

  DEFINE_MERGE_CALLBACK(MonitorWait)

  static void mergeCapabilities(jvmtiCapabilities *capabilities) {
    capabilities->can_generate_monitor_events = 1;
  };
};

/*!
 * \brief JVMTI MonitorWaited callback.
 */
class TMonitorWaitedCallback
    : public TJVMTIEventCallback<jvmtiEventMonitorWaited,
                                 JVMTI_EVENT_MONITOR_WAITED> {
 public:
  static void JNICALL callbackStub(jvmtiEnv *jvmti, JNIEnv *env, jthread thread,
                                   jobject object, jboolean timeout) {
    ITERATE_CALLBACK_CHAIN(jvmtiEventMonitorWaited, jvmti, env, thread, object,
                           timeout);
  };

  DEFINE_MERGE_CALLBACK(MonitorWaited)

  static void mergeCapabilities(jvmtiCapabilities *capabilities) {
    capabilities->can_generate_monitor_events = 1;
  };
};

/*!
 * \brief Register JVMTI callbacks to JVM.
 *
 * \param jvmti [in] JVMTI environment.
 */
inline bool registerJVMTICallbacks(jvmtiEnv *jvmti) {
  jvmtiEventCallbacks callbacks = {0};

  TClassPrepareCallback::mergeCallback(&callbacks);
  TDataDumpRequestCallback::mergeCallback(&callbacks);
  TGarbageCollectionStartCallback::mergeCallback(&callbacks);
  TGarbageCollectionFinishCallback::mergeCallback(&callbacks);
  TResourceExhaustedCallback::mergeCallback(&callbacks);
  TMonitorContendedEnterCallback::mergeCallback(&callbacks);
  TMonitorContendedEnteredCallback::mergeCallback(&callbacks);
  TVMInitCallback::mergeCallback(&callbacks);
  TVMDeathCallback::mergeCallback(&callbacks);
  TThreadStartCallback::mergeCallback(&callbacks);
  TThreadEndCallback::mergeCallback(&callbacks);
  TMonitorWaitCallback::mergeCallback(&callbacks);
  TMonitorWaitedCallback::mergeCallback(&callbacks);

  return isError(
      jvmti, jvmti->SetEventCallbacks(&callbacks, sizeof(jvmtiEventCallbacks)));
}

#endif  // CALLBACK_REGISTER_HPP

/*!
 * \iile threadRecorder.cpp
 * \brief Recording thread status.
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

#include <jvmti.h>
#include <jni.h>

#include <stddef.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "globals.hpp"
#include "util.hpp"
#include "vmFunctions.hpp"
#include "callbackRegister.hpp"
#include "jniCallbackRegister.hpp"
#include "threadRecorder.hpp"

#if PROCESSOR_ARCH == X86
#include "arch/x86/lock.inline.hpp"
#elif PROCESSOR_ARCH == ARM
#include "arch/arm/lock.inline.hpp"
#endif

/* Static valiable */
TThreadRecorder *TThreadRecorder::inst = NULL;

/* variables */
jclass threadClass;
jmethodID currentThreadMethod;

/* JVMTI event handler */

/*!
 * \brief JVMTI callback for ThreadStart event.
 *
 * \param jvmti [in] JVMTI environment.
 * \param env [in] JNI environment.
 * \param thread [in] jthread object which is created.
 */
void JNICALL OnThreadStart(jvmtiEnv *jvmti, JNIEnv *env, jthread thread) {
  TThreadRecorder *recorder = TThreadRecorder::getInstance();
  recorder->registerNewThread(jvmti, thread);
  recorder->putEvent(thread, ThreadStart, 0);
}

/*!
 * \brief JVMTI callback for ThreadEnd event.
 *
 * \param jvmti [in] JVMTI environment.
 * \param env [in] JNI environment.
 * \param thread [in] jthread object which is created.
 */
void JNICALL OnThreadEnd(jvmtiEnv *jvmti, JNIEnv *env, jthread thread) {
  TThreadRecorder::getInstance()->putEvent(thread, ThreadEnd, 0);
}

/*!
 * \brief JVMTI callback for MonitorContendedEnter event.
 *
 * \param jvmti [in] JVMTI environment.
 * \param env [in] JNI environment.
 * \param thread [in] jthread object which is created.
 * \param object [in] Monitor object which is contended.
 */
void JNICALL
    OnMonitorContendedEnterForThreadRecording(jvmtiEnv *jvmti, JNIEnv *env,
                                              jthread thread, jobject object) {
  TThreadRecorder::getInstance()->putEvent(thread, MonitorContendedEnter, 0);
}

/*!
 * \brief JVMTI callback for MonitorContendedEntered event.
 *
 * \param jvmti [in] JVMTI environment.
 * \param env [in] JNI environment.
 * \param thread [in] jthread object which is created.
 * \param object [in] Monitor object which was contended.
 */
void JNICALL OnMonitorContendedEnteredForThreadRecording(jvmtiEnv *jvmti,
                                                         JNIEnv *env,
                                                         jthread thread,
                                                         jobject object) {
  TThreadRecorder::getInstance()->putEvent(thread, MonitorContendedEntered, 0);
}

/*!
 * \brief JVMTI callback for MonitorWait event.
 *
 * \param jvmti [in] JVMTI environment.
 * \param env [in] JNI environment.
 * \param thread [in] jthread object which is created.
 * \param object [in] Monitor object which is waiting.
 * \param timeout [in] Timeout of this monitor.
 */
void JNICALL OnMonitorWait(jvmtiEnv *jvmti, JNIEnv *jni_env, jthread thread,
                           jobject object, jlong timeout) {
  TThreadRecorder::getInstance()->putEvent(thread, MonitorWait, timeout);
}

/*!
 * \brief JVMTI callback for MonitorWait event.
 *
 * \param jvmti [in] JVMTI environment.
 * \param env [in] JNI environment.
 * \param thread [in] jthread object which is created.
 * \param object [in] Monitor object which is waiting.
 * \param timeout [in] Timeout of this monitor.
 */
void JNICALL OnMonitorWaited(jvmtiEnv *jvmti, JNIEnv *jni_env, jthread thread,
                             jobject object, jboolean timeout) {
  TThreadRecorder::getInstance()->putEvent(thread, MonitorWaited, timeout);
}

/*!
 * \brief JVMTI callback for DataDumpRequest to dump thread record data.
 *
 * \param jvmti [in] JVMTI environment.
 */
void JNICALL OnDataDumpRequestForDumpThreadRecordData(jvmtiEnv *jvmti) {
  TThreadRecorder::inst->dump(conf->ThreadRecordFileName()->get());
}

/* Support function. */

/*!
 * \brief Get current thread as pointer of JavaThread in HotSpot.
 *
 * \param env [in] JNI environment.
 * \return JavaThread instance of current thread.
 */
void *GetCurrentThread(JNIEnv *env) {
  jobject threadObj =
      env->CallStaticObjectMethod(threadClass, currentThreadMethod);
  return *(void **)threadObj;
}

/* JNI event handler */

/*!
 * \brief Prologue callback of JVM_Sleep.
 *
 * \param env [in] JNI environment.
 * \param threadClass [in] jclass of java.lang.Thread .
 * \param millis [in] Time of sleeping.
 */
void JNICALL JVM_SleepPrologue(JNIEnv *env, jclass threadClass, jlong millis) {
  void *javaThread = GetCurrentThread(env);
  TThreadRecorder::getInstance()->putEvent((jthread)&javaThread,
                                           ThreadSleepStart, millis);
}

/*!
 * \brief Epilogue callback of JVM_Sleep.
 *
 * \param env [in] JNI environment.
 * \param threadClass [in] jclass of java.lang.Thread .
 * \param millis [in] Time of sleeping.
 */
void JNICALL JVM_SleepEpilogue(JNIEnv *env, jclass threadClass, jlong millis) {
  void *javaThread = GetCurrentThread(env);
  TThreadRecorder::getInstance()->putEvent((jthread)&javaThread,
                                           ThreadSleepEnd, millis);
}

/*!
 * \brief Prologue callback of Unsafe_Park.
 *
 * \param env [in] JNI environment.
 * \param unsafe [in] Instance of Unsafe.
 * \param isAbsolute [in] Absolute time or not.
 * \param time [in] Park time.
 */
void JNICALL UnsafeParkPrologue(JNIEnv *env, jobject unsafe,
                                jboolean isAbsolute, jlong time) {
  void *javaThread = GetCurrentThread(env);
  TThreadRecorder::getInstance()->putEvent((jthread)&javaThread, Park, time);
}

/*!
 * \brief Epilogue callback of Unsafe_Park.
 *
 * \param env [in] JNI environment.
 * \param unsafe [in] Instance of Unsafe.
 * \param isAbsolute [in] Absolute time or not.
 * \param time [in] Park time.
 */
void JNICALL UnsafeParkEpilogue(JNIEnv *env, jobject unsafe,
                                jboolean isAbsolute, jlong time) {
  void *javaThread = GetCurrentThread(env);
  TThreadRecorder::getInstance()->putEvent((jthread)&javaThread, Unpark, time);
}

/* I/O tracer */

/*
 * Method:    socketReadBegin
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL IoTrace_socketReadBegin(JNIEnv *env, jclass cls) {
  void *javaThread = GetCurrentThread(env);
  TThreadRecorder::getInstance()->putEvent((jthread)&javaThread,
                                           SocketReadStart, 0);
  return NULL;
}

/*
 * Method:    socketReadEnd
 * Signature: (Ljava/lang/Object;Ljava/net/InetAddress;IIJ)V
 */
JNIEXPORT void JNICALL IoTrace_socketReadEnd(JNIEnv *env, jclass cls,
                                             jobject context, jobject address,
                                             jint port, jint timeout,
                                             jlong bytesRead) {
  void *javaThread = GetCurrentThread(env);
  TThreadRecorder::getInstance()->putEvent((jthread)&javaThread,
                                           SocketReadEnd, bytesRead);
}

/*
 * Method:    socketWriteBegin
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL IoTrace_socketWriteBegin(JNIEnv *env, jclass cls) {
  void *javaThread = GetCurrentThread(env);
  TThreadRecorder::getInstance()->putEvent((jthread)&javaThread,
                                           SocketWriteStart, 0);
  return NULL;
}

/*
 * Method:    socketWriteEnd
 * Signature: (Ljava/lang/Object;Ljava/net/InetAddress;IJ)V
 */
JNIEXPORT void JNICALL IoTrace_socketWriteEnd(JNIEnv *env, jclass cls,
                                              jobject context, jobject address,
                                              jint port, jlong bytesWritten) {
  void *javaThread = GetCurrentThread(env);
  TThreadRecorder::getInstance()->putEvent((jthread)&javaThread,
                                           SocketWriteEnd, bytesWritten);
}

/*
 * Method:    fileReadBegin
 * Signature: (Ljava/lang/String;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL
    IoTrace_fileReadBegin(JNIEnv *env, jclass cls, jstring path) {
  void *javaThread = GetCurrentThread(env);
  TThreadRecorder::getInstance()->putEvent((jthread)&javaThread,
                                           FileReadStart, 0);
  return NULL;
}

/*
 * Method:    fileReadEnd
 * Signature: (Ljava/lang/Object;J)V
 */
JNIEXPORT void JNICALL IoTrace_fileReadEnd(JNIEnv *env, jclass cls,
                                           jobject context, jlong bytesRead) {
  void *javaThread = GetCurrentThread(env);
  TThreadRecorder::getInstance()->putEvent((jthread)&javaThread,
                                           FileReadEnd, bytesRead);
}

/*
 * Method:    fileWriteBegin
 * Signature: (Ljava/lang/String;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL
    IoTrace_fileWriteBegin(JNIEnv *env, jclass cls, jstring path) {
  void *javaThread = GetCurrentThread(env);
  TThreadRecorder::getInstance()->putEvent((jthread)&javaThread,
                                           FileWriteStart, 0);
  return NULL;
}

/*
 * Method:    fileWriteEnd
 * Signature: (Ljava/lang/Object;J)V
 */
JNIEXPORT void JNICALL IoTrace_fileWriteEnd(JNIEnv *env, jclass cls,
                                            jobject context,
                                            jlong bytesWritten) {
  void *javaThread = GetCurrentThread(env);
  TThreadRecorder::getInstance()->putEvent((jthread)&javaThread,
                                           FileWriteEnd, bytesWritten);
}

/* Class member functions */

/*!
 * \brief Constructor of TThreadRecorder.
 *
 * \param buffer_size [in] Size of ring buffer.
 *                         Ring buffer size will be aligned to page size.
 */
TThreadRecorder::TThreadRecorder(size_t buffer_size) : threadIDMap() {
  aligned_buffer_size = ALIGN_SIZE_UP(buffer_size, systemPageSize);
  bufferLockVal = 0;
  idmapLockVal = 0;

  /* manpage of mmap(2):
   *
   * MAP_ANONYMOUS:
   *   The mapping is not backed by any iile;
   *   to zero.
   */
  record_buffer = mmap(NULL, aligned_buffer_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (record_buffer == MAP_FAILED) {
    throw errno;
  }

  top_of_buffer = (TEventRecord *)record_buffer;
  end_of_buffer =
      (TEventRecord *)((unsigned char *)record_buffer + aligned_buffer_size);
}

/*!
 * \brief Destructor of TThreadRecorder.
 */
TThreadRecorder::~TThreadRecorder() {
  munmap(record_buffer, aligned_buffer_size);

  /* Deallocate memory for thread name. */
  spinLockWait(&idmapLockVal);
  {
    for (std::tr1::unordered_map<jlong, char *,
                                 TNumericalHasher<jlong> >::iterator itr =
                                                            threadIDMap.begin();
         itr != threadIDMap.end(); itr++) {
      free(itr->second);
    }
  }
  spinLockRelease(&idmapLockVal);

}

/*!
 * \brief Register JVMTI hook point.
 *
 * \param jvmti [in] JVMTI environment.
 * \param env [in] JNI environment.
 */
void TThreadRecorder::registerHookPoint(jvmtiEnv *jvmti, JNIEnv *env) {
  /* Set JVMTI event capabilities. */
  jvmtiCapabilities capabilities = {0};
  TThreadStartCallback::mergeCapabilities(&capabilities);
  TThreadEndCallback::mergeCapabilities(&capabilities);
  TMonitorContendedEnterCallback::mergeCapabilities(&capabilities);
  TMonitorContendedEnteredCallback::mergeCapabilities(&capabilities);
  TMonitorWaitCallback::mergeCapabilities(&capabilities);
  TMonitorWaitedCallback::mergeCapabilities(&capabilities);
  TDataDumpRequestCallback::mergeCapabilities(&capabilities);

  if (isError(jvmti, jvmti->AddCapabilities(&capabilities))) {
    logger->printCritMsg(
        "Couldn't set event capabilities for Thread recording.");
    return;
  }

  /* Set JVMTI event callbacks. */
  TThreadStartCallback::registerCallback(&OnThreadStart);
  TThreadEndCallback::registerCallback(&OnThreadEnd);
  TMonitorContendedEnterCallback::registerCallback(
      &OnMonitorContendedEnterForThreadRecording);
  TMonitorContendedEnteredCallback::registerCallback(
      &OnMonitorContendedEnteredForThreadRecording);
  TMonitorWaitCallback::registerCallback(&OnMonitorWait);
  TMonitorWaitedCallback::registerCallback(&OnMonitorWaited);
  TDataDumpRequestCallback::registerCallback(
      &OnDataDumpRequestForDumpThreadRecordData);

  if (registerJVMTICallbacks(jvmti)) {
    logger->printCritMsg("Couldn't register normal event.");
    return;
  }
}

/*!
 * \brief Register JNI hook point.
 *
 * \param env [in] JNI environment.
 */
void TThreadRecorder::registerJNIHookPoint(JNIEnv *env) {
  /* Set JNI varibles */
  threadClass = env->FindClass("java/lang/Thread");
  currentThreadMethod = env->GetStaticMethodID(threadClass, "currentThread",
                                               "()Ljava/lang/Thread;");

  /* Set JNI function callbacks. */
  TJVMSleepCallback::registerCallback(&JVM_SleepPrologue, &JVM_SleepEpilogue);
  TUnsafeParkCallback::registerCallback(&UnsafeParkPrologue,
                                        &UnsafeParkEpilogue);
}

/*!
 * \brief Register I/O tracer.
 *
 * \param jvmti [in] JVMTI environment.
 * \param env [in] JNI environment.
 *
 * \return true if succeeded.
 */
bool TThreadRecorder::registerIOTracer(jvmtiEnv *jvmti, JNIEnv *env) {
  char *iotrace_classfile = conf->ThreadRecordIOTracer()->get();

  if (iotrace_classfile == NULL) {
    logger->printWarnMsg("thread_record_iotracer is not set.");
    logger->printWarnMsg("Turn off I/O recording.");
    return false;
  }

  jclass iotraceClass = env->FindClass("sun/misc/IoTrace");

  if (iotraceClass == NULL) {
    logger->printWarnMsg("Could not find sun.misc.IoTrace class.");
    logger->printWarnMsg("Turn off I/O recording.");
    env->ExceptionClear();  // It may occur NoClassDefFoumdError
    return false;
  }

  int fd = open(iotrace_classfile, O_RDONLY);

  if (fd == -1) {
    logger->printWarnMsgWithErrno("Could not open thread_record_iotracer: %s",
                                  iotrace_classfile);
    logger->printWarnMsg("Turn off I/O recording.");
    return false;
  }

  struct stat st;
  if (fstat(fd, &st) == -1) {
    logger->printWarnMsgWithErrno(
        "Could not get stat of thread_record_iotracer: %s", iotrace_classfile);
    logger->printWarnMsg("Turn off I/O recording.");
    close(fd);
    return false;
  }

  unsigned char *iotracer_bytecode = (unsigned char *)malloc(st.st_size);
  ssize_t read_bytes = read(fd, iotracer_bytecode, st.st_size);
  int saved_errno = errno;
  close(fd);

  if (read_bytes == -1) {
    errno = saved_errno;
    logger->printWarnMsgWithErrno(
        "Could not read bytecodes from thread_record_iotracer: %s",
        iotrace_classfile);
    logger->printWarnMsg("Turn off I/O recording.");
    return false;
  } else if (read_bytes != st.st_size) {
    errno = saved_errno;
    logger->printWarnMsg(
        "Could not read bytecodes from thread_record_iotracer: %s",
        iotrace_classfile);
    logger->printWarnMsg("Turn off I/O recording.");
    return false;
  }

  /* Redefine IoTrace */
  jvmtiClassDefinition classDef = {iotraceClass, (jint)st.st_size,
                                                         iotracer_bytecode};
  if (isError(jvmti, jvmti->RedefineClasses(1, &classDef))) {
    logger->printWarnMsg("Could not redefine sun.misc.IoTrace .");
    logger->printWarnMsg("Turn off I/O recording.");
    return false;
  }

  /* Register hook native methods. */
  JNINativeMethod methods[] = {
      {(char *)"socketReadBegin",
       (char *)"()Ljava/lang/Object;",
       (void *)&IoTrace_socketReadBegin},
      {(char *)"socketReadEnd",
       (char *)"(Ljava/lang/Object;Ljava/net/InetAddress;IIJ)V",
       (void *)&IoTrace_socketReadEnd},
      {(char *)"socketWriteBegin",
       (char *)"()Ljava/lang/Object;",
       (void *)&IoTrace_socketWriteBegin},
      {(char *)"socketWriteEnd",
       (char *)"(Ljava/lang/Object;Ljava/net/InetAddress;IJ)V",
       (void *)&IoTrace_socketWriteEnd},
      {(char *)"fileReadBegin",
       (char *)"(Ljava/lang/String;)Ljava/lang/Object;",
       (void *)&IoTrace_fileReadBegin},
      {(char *)"fileReadEnd",
       (char *)"(Ljava/lang/Object;J)V",
       (void *)&IoTrace_fileReadEnd},
      {(char *)"fileWriteBegin",
       (char *)"(Ljava/lang/String;)Ljava/lang/Object;",
       (void *)&IoTrace_fileWriteBegin},
      {(char *)"fileWriteEnd",
       (char *)"(Ljava/lang/Object;J)V",
       (void *)&IoTrace_fileWriteEnd}};

  env->RegisterNatives(iotraceClass, methods, 8);

  return true;
}

/*!
 * \brief Initialize HeapStats Thread Recorder.
 *
 * \param jvmti [in] JVMTI environment.
 * \param jni [in] JNI environment.
 * \param buf_sz [in] Size of ring buffer.
 */
void TThreadRecorder::initialize(jvmtiEnv *jvmti, JNIEnv *env, size_t buf_sz) {
  static bool isRegistered = false;

  if (likely(inst == NULL)) {
    inst = new TThreadRecorder(buf_sz);

    if (!isRegistered) {
      isRegistered = true;
      registerHookPoint(jvmti, env);
      registerJNIHookPoint(env);
      registerIOTracer(jvmti, env);
    }

    inst->registerAllThreads(jvmti);
  }

  /* Start to hook JVMTI event */
  TThreadStartCallback::switchEventNotification(jvmti, JVMTI_ENABLE);
  TThreadEndCallback::switchEventNotification(jvmti, JVMTI_ENABLE);
  TMonitorContendedEnterCallback::switchEventNotification(jvmti, JVMTI_ENABLE);
  TMonitorContendedEnteredCallback::switchEventNotification(jvmti,
                                                            JVMTI_ENABLE);
  TMonitorWaitCallback::switchEventNotification(jvmti, JVMTI_ENABLE);
  TMonitorWaitedCallback::switchEventNotification(jvmti, JVMTI_ENABLE);

  /* Start to hook JNI function */
  TJVMSleepCallback::switchCallback(env, true);
  TUnsafeParkCallback::switchCallback(env, true);
}

/*!
 * \brief Dump record data to file.
 *
 * \param fname [in] File name to dump record data.
 */
void TThreadRecorder::dump(const char *fname) {
  int fd = creat(fname, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (fd == -1) {
    logger->printWarnMsgWithErrno("Thread Recorder dump failed.");
    throw errno;
  }

  /* Write byte order mark. */
  char bom = BOM;
  write(fd, &bom, sizeof(char));

  spinLockWait(&idmapLockVal);
  {
    /* Dump thread list. */
    int threadIDMapSize = threadIDMap.size();
    write(fd, &threadIDMapSize, sizeof(int));

    for (std::tr1::unordered_map<jlong, char *,
                                 TNumericalHasher<jlong> >::iterator itr =
             threadIDMap.begin();
         itr != threadIDMap.end(); itr++) {
      jlong id = itr->first;
      int classname_length = strlen(itr->second);
      write(fd, &id, sizeof(jlong));
      write(fd, &classname_length, sizeof(int));
      write(fd, itr->second, classname_length);
    }

    /* Dump thread event. */
    write(fd, record_buffer, aligned_buffer_size);
  }
  spinLockRelease(&idmapLockVal);

  close(fd);
}

/*!
 * \brief Finalize HeapStats Thread Recorder.
 *
 * \param jvmti [in] JVMTI environment.
 * \param env [in] JNI environment.
 * \param fname [in] File name to dump record data.
 */
void TThreadRecorder::finalize(jvmtiEnv *jvmti, JNIEnv *env,
                               const char *fname) {
  /* Stop to hook JVMTI event */
  TThreadStartCallback::switchEventNotification(jvmti, JVMTI_DISABLE);
  TThreadEndCallback::switchEventNotification(jvmti, JVMTI_DISABLE);
  TMonitorContendedEnterCallback::switchEventNotification(jvmti, JVMTI_DISABLE);
  TMonitorContendedEnteredCallback::switchEventNotification(jvmti,
                                                            JVMTI_DISABLE);
  TMonitorWaitCallback::switchEventNotification(jvmti, JVMTI_DISABLE);
  TMonitorWaitedCallback::switchEventNotification(jvmti, JVMTI_DISABLE);

  /* Stop to hook JNI function */
  TJVMSleepCallback::switchCallback(env, false);
  TUnsafeParkCallback::switchCallback(env, false);

  /* Stop HeapStats Thread Recorder */
  inst->dump(fname);

  delete inst;
  inst = NULL;
}

/*!
 * \brief Set JVMTI capabilities to work HeapStats Thread Recorder.
 *
 * \param capabilities [out] JVMTI capabilities.
 */
void TThreadRecorder::setCapabilities(jvmtiCapabilities *capabilities) {
  /* For RedefineClass() */
  capabilities->can_redefine_classes = 1;
  capabilities->can_redefine_any_class = 1;
}

/*!
 * \brief Register new thread to thread id map.
 *
 * \param jvmti [in] JVMTI environment.
 * \param thread [in] jthread object of new thread.
 */
void TThreadRecorder::registerNewThread(jvmtiEnv *jvmti, jthread thread) {
  void *thread_oop = *(void **)thread;
  jlong id = TVMFunctions::getInstance()->GetThreadId(thread_oop);

  jvmtiThreadInfo threadInfo;
  jvmti->GetThreadInfo(thread, &threadInfo);

  spinLockWait(&idmapLockVal);
  {
    char *current_val = threadIDMap[id];

    if (unlikely(current_val != NULL)) {
      free(current_val);
    }

    threadIDMap[id] = strdup(threadInfo.name);
  }
  spinLockRelease(&idmapLockVal);

  jvmti->Deallocate((unsigned char *)threadInfo.name);
}

/*!
 * \brief Register all existed threads to thread id map.
 *
 * \param jvmti [in] JVMTI environment.
 */
void TThreadRecorder::registerAllThreads(jvmtiEnv *jvmti) {
  jint thread_count;
  jthread *threads;

  jvmti->GetAllThreads(&thread_count, &threads);

  for (int Cnt = 0; Cnt < thread_count; Cnt++) {
    registerNewThread(jvmti, threads[Cnt]);
  }

  jvmti->Deallocate((unsigned char *)threads);
}

/*!
 * \brief Enqueue new event.
 *
 * \param thread [in] jthread object which occurs new event.
 * \param event [in] New thread event.
 * \param additionalData [in] Additional data if exist.
 */
void TThreadRecorder::putEvent(jthread thread, TThreadEvent event,
                               jlong additionalData) {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  TEventRecord eventRecord __attribute__((aligned(32))); // for YMM vmovdqa

  eventRecord.time = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
  eventRecord.thread_id =
      TVMFunctions::getInstance()->GetThreadId(*(void **)thread);
  eventRecord.event = event;
  eventRecord.additionalData = additionalData;

  if (unlikely(top_of_buffer->event == ThreadEnd)) {
    spinLockWait(&idmapLockVal);
    {
      std::tr1::unordered_map<jlong, char *, TNumericalHasher<jlong> >
              ::iterator entry = threadIDMap.find(top_of_buffer->thread_id);

      if (likely(entry != threadIDMap.end())) {
        free(entry->second);
        threadIDMap.erase(entry);
      }
    }
    spinLockRelease(&idmapLockVal);
  }

  spinLockWait(&bufferLockVal);
  {
    memcpy32(top_of_buffer, &eventRecord);

    if (unlikely(++top_of_buffer == end_of_buffer)) {
      logger->printDebugMsg(
                    "Ring buffer for Thread Recorder has been rewinded.");
      top_of_buffer = (TEventRecord *)record_buffer;
    }
  }
  spinLockRelease(&bufferLockVal);
}


/*!
 * \file threadRecorder.hpp
 * \brief Recording thread status.
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

#ifndef THREAD_RECORDER_HPP
#define THREAD_RECORDER_HPP

#include <jvmti.h>
#include <jni.h>

#include <stddef.h>

#include <tr1/unordered_map>

/*!
 * \brief Header of recording data.
 */
typedef struct {
  jlong buffer_size;
  int thread_name_len;
  char *thread_name;
  void *record_buffer;
} TRecordHeader;

/*!
 * \brief Event record information.
 */
typedef struct {
  jlong time;
  jlong thread_id;
  jlong event;  // This value presents TThreadEvent.
  jlong additionalData;
} TEventRecord;

/*!
 * \brief Event definition.
 */
typedef enum {
  /* JVMTI event */
  ThreadStart = 1,
  ThreadEnd,
  MonitorWait,
  MonitorWaited,
  MonitorContendedEnter,
  MonitorContendedEntered,

  /* JNI function hook */
  ThreadSleepStart,
  ThreadSleepEnd,
  Park,
  Unpark,

  /* IoTrace */
  FileWriteStart,
  FileWriteEnd,
  FileReadStart,
  FileReadEnd,
  SocketWriteStart,
  SocketWriteEnd,
  SocketReadStart,
  SocketReadEnd,
} TThreadEvent;

/*!
 * \brief Implementation of HeapStats Thread Recorder.
 *        This instance must be singleton.
 */
class TThreadRecorder {
 private:
  /*!
   * \brief Page-aligned buffer size.
   */
  size_t aligned_buffer_size;

  /*!
   * \brief Pointer of record buffer.
   */
  void *record_buffer;

  /*!
   * \brief Pointer of record buffer.
   *        Record buffer is ring buffer. So this pointer must be increment
   *        in each writing event, and return to top of buffer when this
   *        pointer reaches end of buffer.
   */
  TEventRecord *top_of_buffer;

  /*!
   * \brief End of record buffer.
   */
  TEventRecord *end_of_buffer;

  /*!
   * \brief ThreadID-ThreadName map.
   *        Key is thread ID, Value is thread name.
   */
  std::tr1::unordered_map<jlong, char *, TNumericalHasher<jlong> > threadIDMap;

  /*!
   * \brief SpinLock variable for Ring Buffer operation.
   */
  volatile int bufferLockVal;

  /*!
   * \brief SpinLock variable for Thread ID map operation.
   */
  volatile int idmapLockVal;

  /*!
   * \brief Instance of TThreadRecorder.
   */
  static TThreadRecorder *inst;

 protected:
  /*!
   * \brief Constructor of TThreadRecorder.
   *
   * \param buffer_size [in] Size of ring buffer.
   *                         Ring buffer size will be aligned to page size.
   */
  TThreadRecorder(size_t buffer_size);

  /*!
   * \brief Register new thread to thread id map.
   *
   * \param jvmti [in] JVMTI environment.
   * \param thread [in] jthread object of new thread.
   */
  void registerNewThread(jvmtiEnv *jvmti, jthread thread);

  /*!
   * \brief Register all existed threads to thread id map.
   *
   * \param jvmti [in] JVMTI environment.
   */
  void registerAllThreads(jvmtiEnv *jvmti);

  /*!
   * \brief Register JVMTI hook point.
   *
   * \param jvmti [in] JVMTI environment.
   * \param env [in] JNI environment.
   */
  static void registerHookPoint(jvmtiEnv *jvmti, JNIEnv *env);

  /*!
   * \brief Register JNI hook point.
   *
   * \param env [in] JNI environment.
   */
  static void registerJNIHookPoint(JNIEnv *env);

  /*!
   * \brief Register I/O tracer.
   *
   * \param jvmti [in] JVMTI environment.
   * \param env [in] JNI environment.
   *
   * \return true if succeeded.
   */
  static bool registerIOTracer(jvmtiEnv *jvmti, JNIEnv *env);

 public:
  /*!
   * \brief JVMTI callback for ThreadStart event.
   *
   * \param jvmti [in] JVMTI environment.
   * \param env [in] JNI environment.
   * \param thread [in] jthread object which is created.
   */
  friend void JNICALL
      OnThreadStart(jvmtiEnv *jvmti, JNIEnv *env, jthread thread);

  /*!
   * \brief JVMTI callback for DataDumpRequest to dump thread record data.
   *
   * \param jvmti [in] JVMTI environment.
   */
  friend void JNICALL OnDataDumpRequestForDumpThreadRecordData(jvmtiEnv *jvmti);

  /*!
   * \brief Initialize HeapStats Thread Recorder.
   *
   * \param jvmti [in] JVMTI environment.
   * \param jni [in] JNI environment.
   * \param buf_sz [in] Size of ring buffer.
   */
  static void initialize(jvmtiEnv *jvmti, JNIEnv *jni, size_t buf_sz);

  /*!
   * \brief Finalize HeapStats Thread Recorder.
   *
   * \param jvmti [in] JVMTI environment.
   * \param env [in] JNI environment.
   * \param fname [in] File name to dump record data.
   */
  static void finalize(jvmtiEnv *jvmti, JNIEnv *env, const char *fname);

  /*!
   * \brief Set JVMTI capabilities to work HeapStats Thread Recorder.
   *
   * \param capabilities [out] JVMTI capabilities.
   */
  static void setCapabilities(jvmtiCapabilities *capabilities);

  /*!
   * \brief Destructor of TThreadRecorder.
   */
  virtual ~TThreadRecorder();

  /*!
   * \brief Dump record data to file.
   *
   * \param fname [in] File name to dump record data.
   */
  void dump(const char *fname);

  /*!
   * \brief Get singleton instance of TThreadRecorder.
   *
   * \return Instance of TThreadRecorder.
   */
  inline static TThreadRecorder *getInstance() { return inst; };

  /*!
   * \brief Enqueue new event.
   *
   * \param thread [in] jthread object which occurs new event.
   * \param event [in] New thread event.
   * \param additionalData [in] Additional data if exist.
   */
  inline void putEvent(jthread thread, TThreadEvent event,
                       jlong additionalData);
};

#endif  // THREAD_RECORDER_HPP

/*!
 * \file logManager.hpp
 * \brief This file is used collect log information.
 * Copyright (C) 2011-2016 Nippon Telegraph and Telephone Corporation
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

#ifndef _LOG_MANAGER_H
#define _LOG_MANAGER_H

#include <queue>

#include "cmdArchiver.hpp"
#include "jniZipArchiver.hpp"
#include "jvmSockCmd.hpp"
#include "jvmInfo.hpp"
#include "util.hpp"

/*!
 * \brief This structure is used to get and store machine cpu time.
 */
typedef struct {
  TLargeUInt usrTime;     /*!< Time used by user work.                     */
  TLargeUInt lowUsrTime;  /*!< Time used by user work under low priority.  */
  TLargeUInt sysTime;     /*!< Time used by kernel work.                   */
  TLargeUInt idleTime;    /*!< Time used by waiting task work.             */
  TLargeUInt iowaitTime;  /*!< Time used by waiting IO work.               */
  TLargeUInt sortIrqTime; /*!< Time used by sort intercept work.           */
  TLargeUInt irqTime;     /*!< Time used by intercept work.                */
  TLargeUInt stealTime;   /*!< Time used by other OS.                      */
  TLargeUInt guestTime;   /*!< Time used by guest OS under kernel control. */
} TMachineTimes;

/*!
 * \brief This class collect and make log.
 */
class TLogManager {
 public:
  /*!
   * \brief TLogManager constructor.
   * \param env  [in] JNI environment object.
   * \param info [in] JVM running performance information.
   */
  TLogManager(JNIEnv *env, TJvmInfo *info);

  /*!
   * \brief TLogManager destructor.
   */
  virtual ~TLogManager(void);

  /*!
   * \brief Collect log.
   * \param jvmti   [in] JVMTI environment object.
   * \param env     [in] JNI environment object.
   * \param cause   [in] Invoke function cause.<br>
   *                     E.g. ResourceExhausted, Signal, Interval.
   * \param nowTime [in] Log collect time.
   * \param description [in] Description of the event.
   * \return Value is zero, if process is succeed.<br />
   *         Value is error number a.k.a. "errno", if process is failure.
   */
  int collectLog(jvmtiEnv *jvmti, JNIEnv *env, TInvokeCause cause,
                 TMSecTime nowTime, const char *description);

 protected:
  /*!
   * \brief Collect normal log.
   * \param cause       [in] Invoke function cause.<br>
   *                         E.g. ResourceExhausted, Signal, Interval.
   * \param nowTime     [in] Log collect time.
   * \param archivePath [in] Archive file path.
   * \return Value is zero, if process is succeed.<br />
   *         Value is error number a.k.a. "errno", if process is failure.
   */
  virtual int collectNormalLog(TInvokeCause cause, TMSecTime nowTime,
                               char *archivePath);

  /*!
   * \brief Collect all log.
   * \param jvmti       [in]  JVMTI environment object.
   * \param env         [in]  JNI environment object.
   * \param cause       [in]  Invoke function cause.<br>
   *                          E.g. ResourceExhausted, Signal, Interval.
   * \param nowTime     [in]  Log collect time.
   * \param archivePath [out] Archive file path.
   * \param pathLen     [in]  Max size of paramter"archivePath".
   * \param description [in] Description of the event.
   * \return Value is zero, if process is succeed.<br />
   *         Value is error number a.k.a. "errno", if process is failure.
   */
  virtual int collectAllLog(jvmtiEnv *jvmti, JNIEnv *env, TInvokeCause cause,
                            TMSecTime nowTime, char *archivePath,
                            size_t pathLen, const char *description);

  RELEASE_ONLY(private :)

  /*!
   * \brief Create file about JVM running environment.
   * \param basePath [in] Path of directory put report file.
   * \param cause    [in] Invoke function cause.<br>
   *                      E.g. Signal, ResourceExhausted, Interval.
   * \param nowTime  [in] Log collect time.
   * \param description [in] Description of the event.
   * \return Value is zero, if process is succeed.<br />
   *         Value is error number a.k.a. "errno", if process is failure.
   */
  virtual int makeEnvironFile(char *basePath, TInvokeCause cause,
                              TMSecTime nowTime, const char *description);

  /*!
   * \brief Dump thread and stack information to stream.
   * \param jvmti     [in] JVMTI environment object.
   * \param env       [in] JNI environment object.
   * \param fd        [in] Output file descriptor.
   * \param stackInfo [in] Stack frame of java thread.
   * \return Value is zero, if process is succeed.<br />
   *         Value is error number a.k.a. "errno", if process is failure.
   */
  virtual int dumpThreadInformation(jvmtiEnv *jvmti, JNIEnv *env, int fd,
                                    jvmtiStackInfo stackInfo);

  /*!
   * \brief Create thread dump with JVMTI.
   * \param jvmti    [in] JVMTI environment object.
   * \param env      [in] JNI environment object.
   * \param filename [in] Path of thread dump file.
   * \param nowTime  [in] Log collect time.
   * \return Value is zero, if process is succeed.<br />
   *         Value is error number a.k.a. "errno", if process is failure.
   */
  virtual int makeJvmtiThreadDump(jvmtiEnv *jvmti, JNIEnv *env, char *filename,
                                  TMSecTime nowTime);

  /*!
   * \brief Create thread dump file.
   * \param jvmti    [in] JVMTI environment object.
   * \param env      [in] JNI environment object.
   * \param basePath [in] Path of directory put report file.
   * \param cause    [in] Invoke function cause.<br>
   *                      E.g. Signal, ResourceExhausted, Interval.
   * \param nowTime  [in] Log collect time.
   * \return Value is zero, if process is succeed.<br />
   *         Value is error number a.k.a. "errno", if process is failure.
   */
  virtual int makeThreadDumpFile(jvmtiEnv *jvmti, JNIEnv *env, char *basePath,
                                 TInvokeCause cause, TMSecTime nowTime);

  /*!
   * \brief Getting java process information.
   * \param systime [out] System used cpu time in java process.
   * \param usrtime [out] User and java used cpu time in java process.
   * \param vmsize  [out] Virtual memory size of java process.
   * \param rssize  [out] Memory size of java process on main memory.
   * \return Prosess is succeed or failure.
   */
  virtual bool getProcInfo(TLargeUInt *systime, TLargeUInt *usrtime,
                           TLargeUInt *vmsize, TLargeUInt *rssize);

  /*!
   * \brief Getting machine cpu times.
   * \param times [out] Machine cpu times information.
   * \return Prosess is succeed or failure.
   */
  virtual bool getSysTimes(TMachineTimes *times);

  /*!
   * \brief Send log archive trap.
   * \param cause       [in] Invoke function cause.<br>
   *                         E.g. Signal, ResourceExhausted, Interval.
   * \param nowTime     [in] Log collect time.
   * \param path        [in] Archive file or directory path.
   * \param isDirectory [in] Param "path" is directory.
   * \return Value is true, if process is succeed.
   */
  virtual bool sendLogArchiveTrap(TInvokeCause cause, TMSecTime nowTime,
                                  char *path, bool isDirectory);

  /*!
   * \brief Collect files about JVM enviroment.
   * \param basePath [in] Temporary working directory.
   * \return Value is zero, if process is succeed.<br />
   *         Value is error number a.k.a. "errno", if process is failure.
   */
  virtual int copyInfoFiles(char const *basePath);

  /*!
   * \brief Copy GC log file.
   * \param basePath [in] Path of temporary directory.
   * \return Value is zero, if process is succeed.<br />
   *         Value is error number a.k.a. "errno", if process is failure.
   */
  virtual int copyGCLogFile(char const *basePath);

  /*!
   * \brief Create file about using socket by JVM.
   * \param basePath [in] Path of directory put report file.
   * \return Value is zero, if process is succeed.<br />
   *         Value is error number a.k.a. "errno", if process is failure.
   */
  virtual int makeSocketOwnerFile(char const *basePath);

  /*!
   * \brief Create archive file path.
   * \param nowTime [in] Log collect time.
   * \return Return archive file name, if process is succeed.<br>
   *         Don't forget deallocate memory.<br>
   *         Process is failure, if value is null.
   */
  virtual char *createArchiveName(TMSecTime nowTime);

  /*!
   * \brief Mutex of collect normal log.
   */
  static pthread_mutex_t logMutex;

  /*!
   * \brief Mutex of archive file.
   */
  static pthread_mutex_t archiveMutex;

  /*!
   * \brief Archive file maker.
   */
  TCmdArchiver *arcMaker;

  /*!
   * \brief Archive file maker to use zip library in java.
   */
  TJniZipArchiver *jniArchiver;

  /*!
   * \brief Dump thread information object.
   */
  TJVMSockCmd *jvmCmd;

  /*!
   * \brief JVM running performance information.
   */
  TJvmInfo *jvmInfo;

  /*!
   * \brief Pointer of string of GC log file path.
   */
  char **gcLogFilename;
};

#endif  // _LOG_MANAGER_H

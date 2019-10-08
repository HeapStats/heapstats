/*!
 * \file logManager.cpp
 * \brief This file is used collect log information.
 * Copyright (C) 2011-2018 Nippon Telegraph and Telephone Corporation
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

#include <gnu/libc-version.h>
#include <dirent.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#include <queue>

#include "globals.hpp"
#include "fsUtil.hpp"
#include "logManager.hpp"

/* Static variables. */

/*!
 * \brief Mutex of collect normal log.
 */
pthread_mutex_t TLogManager::logMutex = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;

/*!
 * \brief Mutex of archive file.
 */
pthread_mutex_t TLogManager::archiveMutex =
    PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;

/* Macro defines. */

/*!
 * \brief Format string macro for inode number type(ino_t).
 */
#define INODENUM_FORMAT_STR "%lu"

/*!
 * \brief Separator string mcaro of between name and value for environment file.
 */
#define ENVIRON_VALUE_SEPARATOR "="

/*!
 * \brief Symbol string mcaro of GC log filename.
 */
#define GCLOG_FILENAME_SYMBOL "_ZN9Arguments16_gc_log_filenameE"

/* Util method for log manager. */

/*!
 * \brief Convert log cause to integer for file output.
 * \param cause [in] Log collect cause.
 * \return Number of log cause.
 */
inline int logCauseToInt(TInvokeCause cause) {
  /* Convert invoke function cause. */
  int logCause;
  switch (cause) {
    case ResourceExhausted:
    case ThreadExhausted:
      logCause = 1;
      break;

    case Signal:
    case AnotherSignal:
      logCause = 2;
      break;

    case Interval:
      logCause = 3;
      break;

    case OccurredDeadlock:
      logCause = 4;
      break;

    default:
      /* Illegal. */
      logCause = 0;
  }
  return logCause;
}

/* Class method. */

/*!
 * \brief TLogManager constructor.
 * \param env  [in] JNI environment object.
 * \param info [in] JVM running performance information.
 */
TLogManager::TLogManager(JNIEnv *env, TJvmInfo *info) {
  /* Sanity check. */
  if (unlikely(info == NULL)) {
    throw "TJvmInfo is NULL.";
  }
  jvmInfo = info;
  jvmCmd = NULL;
  arcMaker = NULL;
  jniArchiver = NULL;

  char *tempdirPath = NULL;
  /* Get temporary path of java */
  tempdirPath = GetSystemProperty(env, "java.io.tmpdir");
  try {
    /* Create JVM socket commander. */
    jvmCmd = new TJVMSockCmd(tempdirPath);

    /* Archive file maker. */
    arcMaker = new TCmdArchiver();

    /* Archive file maker to use zip library in java. */
    jniArchiver = new TJniZipArchiver();

    /* Get GC log filename pointer. */
    gcLogFilename = (char **)symFinder->findSymbol(GCLOG_FILENAME_SYMBOL);
    if (unlikely(gcLogFilename == NULL)) {
      throw 1;
    }
  } catch (...) {
    free(tempdirPath);
    delete jvmCmd;
    delete arcMaker;
    delete jniArchiver;

    throw "TLogManager initialize failed!";
  }
  free(tempdirPath);
}

/*!
 * \brief TLogManager destructor.
 */
TLogManager::~TLogManager(void) {
  /* Destroy instance. */
  delete jvmCmd;
  delete arcMaker;
  delete jniArchiver;
}

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
int TLogManager::collectLog(jvmtiEnv *jvmti, JNIEnv *env, TInvokeCause cause,
                            TMSecTime nowTime, const char *description) {
  int result = 0;
  /* Variable store archive file path. */
  char arcPath[PATH_MAX] = {0};

  /* Switch collecting log type by invoking cause. */
  switch (cause) {
    case ResourceExhausted:
    case ThreadExhausted:
    case AnotherSignal:
    case OccurredDeadlock: {
      /* Collect log about java running environment and etc.. */
      int returnCode =
          collectAllLog(jvmti, env, cause, nowTime,
                        (char *)arcPath, PATH_MAX, description);
      if (unlikely(returnCode != 0)) {
        /* Check and show disk full error. */
        result = returnCode;
        checkDiskFull(returnCode, "collect log");
      }
    }
    default: {
      /* Collect log about java and machine now status. */
      int returnCode = collectNormalLog(cause, nowTime, arcPath);
      if (unlikely(returnCode != 0)) {
        /* Check and show disk full error. */
        result = returnCode;
        checkDiskFull(returnCode, "collect log");
      }
    }
  }
  return result;
}

/*!
 * \brief Collect normal log.
 * \param cause       [in] Invoke function cause.<br>
 *                         E.g. ResourceExhausted, Signal, Interval.
 * \param nowTime     [in] Log collect time.
 * \param archivePath [in] Archive file path.
 * \return Value is zero, if process is succeed.<br />
 *         Value is error number a.k.a. "errno", if process is failure.
 */
int TLogManager::collectNormalLog(TInvokeCause cause, TMSecTime nowTime,
                                  char *archivePath) {
  int result = EINVAL;
  TLargeUInt systime = 0;
  TLargeUInt usrtime = 0;
  TLargeUInt vmsize = 0;
  TLargeUInt rssize = 0;
  TMachineTimes cpuTimes = {0};

  /* Get java process information. */
  if (unlikely(!getProcInfo(&systime, &usrtime, &vmsize, &rssize))) {
    logger->printWarnMsg("Failure getting java process information.");
  }

  /* Get machine cpu times. */
  if (unlikely(!getSysTimes(&cpuTimes))) {
    logger->printWarnMsg("Failure getting machine cpu times.");
  }

  /* Make write log line. */
  char logData[4097] = {0};
  snprintf(logData, 4096,
           "%lld,%d"              /* Format : Logging information.      */
           ",%llu,%llu,%llu,%llu" /* Format : Java process information. */
           ",%llu,%llu,%llu"      /* Format : Machine CPU times.        */
           ",%llu,%llu,%llu"
           ",%llu,%llu,%llu"
           ",%lld,%lld,%lld,%lld" /* Format : JVM running information.  */
           ",%s\n",               /* Format : Archive file name.        */
           /* Params : Logging information. */
           nowTime,
           logCauseToInt(cause),
           /* Params : Java process information. */
           usrtime, systime, vmsize, rssize,
           /* Params : Machine CPU times. */
           cpuTimes.usrTime, cpuTimes.lowUsrTime, cpuTimes.sysTime,
           cpuTimes.idleTime, cpuTimes.iowaitTime, cpuTimes.irqTime,
           cpuTimes.sortIrqTime, cpuTimes.stealTime, cpuTimes.guestTime,
           /* Params : JVM running information. */
           (long long int)jvmInfo->getSyncPark(),
           (long long int)jvmInfo->getSafepointTime(),
           (long long int)jvmInfo->getSafepoints(),
           (long long int)jvmInfo->getThreadLive(),
           /* Params : Archive file name. */
           archivePath);

  /* Get mutex. */
  ENTER_PTHREAD_SECTION(&logMutex) {

    /* Open log file. */
    int fd = open(conf->HeapLogFile()->get(), O_CREAT | O_WRONLY | O_APPEND,
                  S_IRUSR | S_IWUSR);

    /* If failure open file. */
    if (unlikely(fd < 0)) {
      result = errno;
      logger->printWarnMsgWithErrno("Could not open log file");
    } else {
      result = 0;

      /* Write line to log file. */
      if (unlikely(write(fd, logData, strlen(logData)) < 0)) {
        result = errno;
        logger->printWarnMsgWithErrno("Could not write to log file");
      }

      /* Cleanup. */
      if (unlikely(close(fd) < 0 && (result == 0))) {
        result = errno;
        logger->printWarnMsgWithErrno("Could not close log file");
      }
    }
  }
  /* Release mutex. */
  EXIT_PTHREAD_SECTION(&logMutex)
  return result;
}

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
int TLogManager::collectAllLog(jvmtiEnv *jvmti, JNIEnv *env, TInvokeCause cause,
                               TMSecTime nowTime, char *archivePath,
                               size_t pathLen, const char *description) {
  /* Variable for process result. */
  int result = 0;
  /* Working directory path. */
  char *basePath = NULL;
  /* Archive file path. */
  char *uniqArcName = NULL;

  /* Make directory. */
  result = createTempDir(&basePath, conf->LogDir()->get());
  if (unlikely(result != 0)) {
    logger->printWarnMsg("Failure create working directory.");
    return result;
  }

  try {
    /* Create enviroment report file. */
    result = makeEnvironFile(basePath, cause, nowTime, description);
    if (unlikely(result != 0)) {
      logger->printWarnMsg("Failure create enviroment file.");

      /* If raise disk full error. */
      if (unlikely(isRaisedDiskFull(result))) {
        throw 1;
      }
    }

    /* Copy many files. */
    result = copyInfoFiles(basePath);
    /* If raise disk full error. */
    if (unlikely(isRaisedDiskFull(result))) {
      throw 1;
    }

    /* Create thread dump file. */
    result = makeThreadDumpFile(jvmti, env, basePath, cause, nowTime);
    if (unlikely(result != 0)) {
      logger->printWarnMsg("Failure thread dumping.");

      /* If raise disk full error. */
      if (unlikely(isRaisedDiskFull(result))) {
        throw 1;
      }
    }

    /* Copy gc log file. */
    result = copyGCLogFile(basePath);
    if (unlikely(result != 0)) {
      logger->printWarnMsgWithErrno("Could not copy GC log.");

      /* If raise disk full error. */
      if (unlikely(isRaisedDiskFull(result))) {
        throw 1;
      }
    }

    /* Create socket owner file. */
    result = makeSocketOwnerFile(basePath);
    if (unlikely(result != 0)) {
      logger->printWarnMsgWithErrno("Could not create socket owner file.");

      /* If raise disk full error. */
      if (unlikely(isRaisedDiskFull(result))) {
        throw 1;
      }
    }
  } catch (...) {
    ; /* Failed collect files by disk full. */
  }

  if (likely(result == 0)) {
    /*
     * Set value mean failed to create archive,
     * For if failed to get "archiveMutex" mutex.
     */
    result = -1;

    /* Get mutex. */
    ENTER_PTHREAD_SECTION(&archiveMutex) {

      /* Create archive file name. */
      uniqArcName = createArchiveName(nowTime);
      if (unlikely(uniqArcName == NULL)) {
        /* Failure make archive uniq name. */
        logger->printWarnMsg("Failure create archive name.");
      } else {
        /* Execute archive. */
        arcMaker->setTarget(basePath);
        result = arcMaker->doArchive(env, uniqArcName);

        /* If failure create archive file. */
        if (unlikely(result != 0)) {
          /* Execute archive to use jniArchiver. */
          jniArchiver->setTarget(basePath);
          result = jniArchiver->doArchive(env, uniqArcName);
        }
      }
    }
    /* Release mutex. */
    EXIT_PTHREAD_SECTION(&archiveMutex)

    /* If failure create archive file yet. */
    if (unlikely(result != 0)) {
      logger->printWarnMsg("Failure create archive file.");
    }
  }

  char *sendPath = uniqArcName;
  bool flagDirectory = false;
  /* If archive process is succeed. */
  if (likely(result == 0)) {
    /* Search last path separator. */
    char *filePos = strrchr(uniqArcName, '/');
    if (filePos != NULL) {
      filePos++;
    } else {
      filePos = uniqArcName;
    }

    /* Copy archive file name without directory path. */
    strncpy(archivePath, filePos, pathLen);
  } else {
    /* If failure execute command. */

    /* Send working directory path to make up for archive file. */
    sendPath = basePath;
    flagDirectory = true;
  }

  /* Send log archive trap. */
  if (unlikely(!sendLogArchiveTrap(cause, nowTime, sendPath, flagDirectory))) {
    logger->printWarnMsg("Send SNMP log archive trap failed!");
  }

  /* If execute command is succeed. */
  if (likely(result == 0)) {
    /* Remove needless working directory. */
    removeTempDir(basePath);
  }

  /* If allocated archive file path. */
  if (likely(uniqArcName != NULL)) {
    free(uniqArcName);
  }

  /* Cleanup. */
  free(basePath);

  return result;
}

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
int TLogManager::makeEnvironFile(char *basePath, TInvokeCause cause,
                                 TMSecTime nowTime, const char *description) {
  int raisedErrNum = 0;

  /* Invoke OS version function. */
  struct utsname uInfo;
  memset(&uInfo, 0, sizeof(struct utsname));
  if (unlikely(uname(&uInfo) != 0)) {
    logger->printWarnMsgWithErrno("Could not get kernel information.");
  }

  /* Invoke glibc information function. */
  const char *glibcVersion = NULL;
  const char *glibcRelease = NULL;

  glibcVersion = gnu_get_libc_version();
  glibcRelease = gnu_get_libc_release();
  if (unlikely(glibcVersion == NULL || glibcRelease == NULL)) {
    logger->printWarnMsgWithErrno("Could not get glibc version.");
  }

  /* Create filename. */
  char *envInfoName = createFilename(basePath, "envInfo.txt");
  /* If failure create file name. */
  if (unlikely(envInfoName == NULL)) {
    raisedErrNum = errno;
    logger->printWarnMsgWithErrno(
        "Could not allocate memory for envInfo.txt .");
    return raisedErrNum;
  }

  /* Create envInfo.txt. */
  int fd = open(envInfoName, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
  if (unlikely(fd < 0)) {
    raisedErrNum = errno;
    logger->printWarnMsgWithErrno("Could not create envInfo.txt .");
    free(envInfoName);
    return raisedErrNum;
  }
  free(envInfoName);

  /* Output enviroment information. */
  try {
    /* Output list. */
    const struct {
      const char *colName;
      const char *valueFmt;
      TMSecTime valueData;
      const char *valuePtr;
      bool isString;
    } envColumnList[] = {
          {"CollectionDate", "%lld", nowTime, NULL, false},
          {"LogTrigger", "%d", (TMSecTime)logCauseToInt(cause), NULL, false},
          {"Description", "%s", 0, description, true},
          {"VmVersion", "%s", 0, jvmInfo->getVmVersion(), true},
          {"OsRelease", "%s", 0, uInfo.release, true},
          {"LibCVersion", "%s", 0, glibcVersion, true},
          {"LibCRelease", "%s", 0, glibcRelease, true},
          {"VmName", "%s", 0, jvmInfo->getVmName(), true},
          {"ClassPath", "%s", 0, jvmInfo->getClassPath(), true},
          {"EndorsedPath", "%s", 0, jvmInfo->getEndorsedPath(), true},
          {"JavaVersion", "%s", 0, jvmInfo->getJavaVersion(), true},
          {"JavaHome", "%s", 0, jvmInfo->getJavaHome(), true},
          {"BootClassPath", "%s", 0, jvmInfo->getBootClassPath(), true},
          {"VmArgs", "%s", 0, jvmInfo->getVmArgs(), true},
          {"VmFlags", "%s", 0, jvmInfo->getVmFlags(), true},
          {"JavaCmd", "%s", 0, jvmInfo->getJavaCommand(), true},
          {"VmTime", JLONG_FORMAT_STR, (TMSecTime)jvmInfo->getTickTime(), NULL, false},
          {NULL, NULL, 0, NULL, '\0'}};

    char lineBuf[PATH_MAX + 1] = {0};
    char EMPTY_LINE[] = "";
    for (int i = 0; envColumnList[i].colName != NULL; i++) {
      /* Write column and separator. */
      snprintf(lineBuf, PATH_MAX, "%s" ENVIRON_VALUE_SEPARATOR,
               envColumnList[i].colName);
      if (unlikely(write(fd, lineBuf, strlen(lineBuf)) < 0)) {
        throw 1;
      }

      /* Convert column value to string. */
      char const *columnStr = NULL;
      if (envColumnList[i].isString) {
        columnStr = envColumnList[i].valuePtr;
        if (unlikely(columnStr == NULL)) {
          columnStr = EMPTY_LINE;
        }
      } else {
        snprintf(lineBuf, PATH_MAX, envColumnList[i].valueFmt,
                 envColumnList[i].valueData);
        columnStr = lineBuf;
      }

      /* Write column value. */
      if (unlikely(write(fd, columnStr, strlen(columnStr)) < 0)) {
        throw 1;
      }

      /* Write line separator. */
      snprintf(lineBuf, PATH_MAX, "\n");
      if (unlikely(write(fd, lineBuf, strlen(lineBuf)) < 0)) {
        throw 1;
      }
    }
  } catch (...) {
    /* Stored error number to avoid overwriting by "close". */
    raisedErrNum = errno;
    logger->printWarnMsgWithErrno("Could not create environment file.");
  }

  /* Cleanup. */
  if (unlikely(close(fd) < 0 && raisedErrNum == 0)) {
    raisedErrNum = errno;
    logger->printWarnMsgWithErrno("Could not create environment file.");
  }

  return raisedErrNum;
}

/*!
 * \brief Dump thread and stack information to stream.
 * \param jvmti     [in] JVMTI environment object.
 * \param env       [in] JNI environment object.
 * \param fd        [in] Output file descriptor.
 * \param stackInfo [in] Stack frame of java thread.
 * \return Value is zero, if process is succeed.<br />
 *         Value is error number a.k.a. "errno", if process is failure.
 */
int TLogManager::dumpThreadInformation(jvmtiEnv *jvmti, JNIEnv *env, int fd,
                                       jvmtiStackInfo stackInfo) {
  /* Get JVMTI capabilities for getting monitor information. */
  jvmtiCapabilities capabilities;
  jvmti->GetCapabilities(&capabilities);
  bool hasCapability = capabilities.can_get_owned_monitor_stack_depth_info &&
                       capabilities.can_get_current_contended_monitor;

  /* Get thread information. */
  TJavaThreadInfo threadInfo = {0};
  getThreadDetailInfo(jvmti, env, stackInfo.thread, &threadInfo);

  char const EMPTY_STR[] = "";
  char lineBuf[PATH_MAX + 1] = {0};

  /* Output thread information. */

  try {
    /* Output thread name, thread type and priority. */
    snprintf(lineBuf, PATH_MAX, "\"%s\"%s prio=%d\n",
             ((threadInfo.name != NULL) ? threadInfo.name : EMPTY_STR),
             ((threadInfo.isDaemon) ? " daemon" : EMPTY_STR),
             threadInfo.priority);
    if (unlikely(write(fd, lineBuf, strlen(lineBuf)) < 0)) {
      throw 1;
    }

    /* Output thread state. */
    snprintf(lineBuf, PATH_MAX, "   java.lang.Thread.State: %s\n",
             ((threadInfo.state != NULL) ? threadInfo.state : EMPTY_STR));
    if (unlikely(write(fd, lineBuf, strlen(lineBuf)) < 0)) {
      throw 1;
    }
  } catch (...) {
    int raisedErrNum = errno;
    logger->printWarnMsgWithErrno("Could not create threaddump through JVMTI.");

    free(threadInfo.name);
    free(threadInfo.state);

    return raisedErrNum;
  }
  free(threadInfo.name);
  free(threadInfo.state);

  /* Get thread monitor stack information. */
  jint monitorCount = 0;
  jvmtiMonitorStackDepthInfo *monitorInfo = NULL;
  if (unlikely(
          hasCapability &&
          isError(jvmti, jvmti->GetOwnedMonitorStackDepthInfo(
                             stackInfo.thread, &monitorCount, &monitorInfo)))) {
    /* Agnet is failed getting monitor info. */
    monitorInfo = NULL;
  }

  int result = 0;
  try {
    /* Output thread flame trace. */
    for (jint flameIdx = 0, flameSize = stackInfo.frame_count;
         flameIdx < flameSize; flameIdx++) {
      jvmtiFrameInfo frameInfo = stackInfo.frame_buffer[flameIdx];
      TJavaStackMethodInfo methodInfo = {0};

      /* Get method information. */
      getMethodFrameInfo(jvmti, env, frameInfo, &methodInfo);

      /* Output method class, name and source file location. */

      char *srcLocation = NULL;
      if (!methodInfo.isNative) {
        char locationBuf[PATH_MAX + 1] = {0};
        if (likely(methodInfo.lineNumber >= 0)) {
          snprintf(locationBuf, PATH_MAX, "%d", methodInfo.lineNumber);
          srcLocation = strdup(locationBuf);
        }

        snprintf(locationBuf, PATH_MAX, "%s:%s",
                 (methodInfo.sourceFile != NULL) ? methodInfo.sourceFile
                                                 : "UnknownFile",
                 (srcLocation != NULL) ? srcLocation : "UnknownLine");
        free(srcLocation);
        srcLocation = strdup(locationBuf);
      }
      snprintf(lineBuf, PATH_MAX, "\tat %s.%s(%s)\n",
               (methodInfo.className != NULL) ? methodInfo.className
                                              : "UnknownClass",
               (methodInfo.methodName != NULL) ? methodInfo.methodName
                                               : "UnknownMethod",
               (srcLocation != NULL) ? srcLocation : "Native method");

      /* Cleanup. */
      free(methodInfo.className);
      free(methodInfo.methodName);
      free(methodInfo.sourceFile);
      free(srcLocation);

      if (unlikely(write(fd, lineBuf, strlen(lineBuf)) < 0)) {
        throw 1;
      }

      /* If current stack frame. */
      if (unlikely(hasCapability && flameIdx == 0)) {
        jobject jMonitor = NULL;
        jvmti->GetCurrentContendedMonitor(stackInfo.thread, &jMonitor);

        /* If contented monitor is existing now. */
        if (likely(jMonitor != NULL)) {
          jclass monitorClass = NULL;
          char *tempStr = NULL;
          char ownerThreadName[1025] = "UNKNOWN";
          char monitorClsName[1025] = "UNKNOWN";

          /* Get monitor class. */
          monitorClass = env->GetObjectClass(jMonitor);
          if (likely(monitorClass != NULL)) {
            /* Get class signature. */
            jvmti->GetClassSignature(monitorClass, &tempStr, NULL);

            if (likely(tempStr != NULL)) {
              snprintf(monitorClsName, 1024, "%s", tempStr);
              jvmti->Deallocate((unsigned char *)tempStr);
            }

            /* Cleanup. */
            env->DeleteLocalRef(monitorClass);
          }

          /* Get monitor owner. */
          jvmtiMonitorUsage monitorInfo = {0};
          if (unlikely(!isError(jvmti, jvmti->GetObjectMonitorUsage(
                                           jMonitor, &monitorInfo)))) {
            /* Get owner thread information. */
            getThreadDetailInfo(jvmti, env, monitorInfo.owner, &threadInfo);

            if (likely(threadInfo.name != NULL)) {
              strncpy(ownerThreadName, threadInfo.name, 1024);
            }

            /* Cleanup. */
            free(threadInfo.name);
            free(threadInfo.state);
          }
          env->DeleteLocalRef(jMonitor);

          /* Output contented monitor information. */
          snprintf(lineBuf, PATH_MAX, "\t- waiting to lock <owner:%s> (a %s)\n",
                   ownerThreadName, monitorClsName);
          if (unlikely(write(fd, lineBuf, strlen(lineBuf)) < 0)) {
            throw 1;
          }
        }
      }

      if (likely(monitorInfo != NULL)) {
        /* Search monitor. */
        jint monitorIdx = 0;
        for (; monitorIdx < monitorCount; monitorIdx++) {
          if (monitorInfo[monitorIdx].stack_depth == flameIdx) {
            break;
          }
        }

        /* If locked monitor is found. */
        if (likely(monitorIdx < monitorCount)) {
          jobject jMonitor = monitorInfo[monitorIdx].monitor;
          jclass monitorClass = NULL;
          char *tempStr = NULL;
          char monitorClsName[1025] = "UNKNOWN";

          /* Get object class. */
          monitorClass = env->GetObjectClass(jMonitor);
          if (likely(monitorClass != NULL)) {
            /* Get class signature. */
            jvmti->GetClassSignature(monitorClass, &tempStr, NULL);
            if (likely(tempStr != NULL)) {
              snprintf(monitorClsName, 1024, "%s", tempStr);
              jvmti->Deallocate((unsigned char *)tempStr);
            }
            env->DeleteLocalRef(monitorClass);
          }

          /* Output owned monitor information. */
          snprintf(lineBuf, PATH_MAX, "\t- locked (a %s)\n", monitorClsName);
          if (unlikely(write(fd, lineBuf, strlen(lineBuf)) < 0)) {
            throw 1;
          }
        }
      }
    }

    /* Output separator for each thread. */
    snprintf(lineBuf, PATH_MAX, "\n");
    if (unlikely(write(fd, lineBuf, strlen(lineBuf)) < 0)) {
      throw 1;
    }
  } catch (...) {
    result = errno;

    /* Raise write error. */
    logger->printWarnMsgWithErrno("Could not create threaddump through JVMTI.");
  }

  /* Cleanup. */
  if (likely(monitorInfo != NULL)) {
    for (jint monitorIdx = 0; monitorIdx < monitorCount; monitorIdx++) {
      env->DeleteLocalRef(monitorInfo[monitorIdx].monitor);
    }
    jvmti->Deallocate((unsigned char *)monitorInfo);
  }
  return result;
}

/*!
 * \brief Create thread dump with JVMTI.
 * \param jvmti    [in] JVMTI environment object.
 * \param env      [in] JNI environment object.
 * \param filename [in] Path of thread dump file.
 * \param nowTime  [in] Log collect time.
 * \return Value is zero, if process is succeed.<br />
 *         Value is error number a.k.a. "errno", if process is failure.
 */
int TLogManager::makeJvmtiThreadDump(jvmtiEnv *jvmti, JNIEnv *env,
                                     char *filename, TMSecTime nowTime) {
  int result = 0;
  /* Open thread dump file. */
  int fd = open(filename, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
  /* If failure open file. */
  if (unlikely(fd < 0)) {
    result = errno;
    logger->printWarnMsgWithErrno("Could not create %s", filename);
    return result;
  }

  const jint MAX_STACK_COUNT = 100;
  jvmtiStackInfo *stackList = NULL;
  jint threadCount = 0;
  /* If failure get all stack traces. */
  if (unlikely(
          isError(jvmti, jvmti->GetAllStackTraces(MAX_STACK_COUNT, &stackList,
                                                  &threadCount)))) {
    logger->printWarnMsg("Couldn't get thread stack trace.");
    close(fd);
    return -1;
  }

  /* Output stack trace. */
  for (jint i = 0; i < threadCount; i++) {
    jvmtiStackInfo stackInfo = stackList[i];

    /* Output thread information. */
    result = dumpThreadInformation(jvmti, env, fd, stackInfo);
    if (unlikely(result != 0)) {
      break;
    }
  }

  /* Cleanup. */
  if (unlikely(close(fd) < 0 && result == 0)) {
    result = errno;
    logger->printWarnMsgWithErrno("Could not create threaddump through JVMTI.");
  }

  jvmti->Deallocate((unsigned char *)stackList);

  return result;
}

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
int TLogManager::makeThreadDumpFile(jvmtiEnv *jvmti, JNIEnv *env,
                                    char *basePath, TInvokeCause cause,
                                    TMSecTime nowTime) {
  /* Dump thread information. */

  int result = -1;

  /* Create dump filename. */
  char *dumpName = createFilename((char *)basePath, "threaddump.txt");
  if (unlikely(dumpName == NULL)) {
    logger->printWarnMsg("Couldn't allocate thread dump file path.");
  } else {
    if (unlikely(cause == ThreadExhausted && !jvmCmd->isConnectable())) {
      /*
       * JVM is aborted when reserve signal SIGQUIT,
       * if JVM can't make new thread (e.g. RLIMIT_NPROC).
       * So we need avoid to send SIGQUIT signal.
       */
      ;
    } else {
      /* Create thread dump file. */
      result = jvmCmd->exec("threaddump", dumpName);
    }

    /* If disk isn't full. */
    if (unlikely(!isRaisedDiskFull(result))) {
      /* If need original thread dump. */
      if (unlikely((result != 0) && (jvmti != NULL))) {
        result = makeJvmtiThreadDump(jvmti, env, dumpName, nowTime);
      }
    }

    free(dumpName);
  }
  return result;
}

/*!
 * \brief Getting java process information.
 * \param systime [out] System used cpu time in java process.
 * \param usrtime [out] User and java used cpu time in java process.
 * \param vmsize  [out] Virtual memory size of java process.
 * \param rssize  [out] Memory size of java process on main memory.
 * \return Prosess is succeed or failure.
 */
bool TLogManager::getProcInfo(TLargeUInt *systime, TLargeUInt *usrtime,
                              TLargeUInt *vmsize, TLargeUInt *rssize) {
  /* Get page size. */
  long pageSize = 0;
  /* If failure get page size from sysconf. */
  if (unlikely(systemPageSize <= 0)) {
    /* Assume page size is 4 KiByte. */
    systemPageSize = 1 << 12;
    logger->printWarnMsg("System page size not found.");
  }
  /* Use sysconf return value. */
  pageSize = systemPageSize;

  /* Initialize. */
  *usrtime = -1;
  *systime = -1;
  *vmsize = -1;
  *rssize = -1;

  /* Make path of process status file. */
  char path[256] = {0};
  snprintf(path, 255, "/proc/%d/stat", getpid());
  /* Open process status file. */
  FILE *proc = fopen(path, "r");

  /* If failure open process status file. */
  if (unlikely(proc == NULL)) {
    logger->printWarnMsgWithErrno("Could not open process status.");
    return false;
  }

  char *line = NULL;
  size_t len = 0;
  /* Get line from process status file. */
  if (likely(getline(&line, &len, proc) > 0)) {
    /* Parse line. */
    if (likely(
            sscanf(
                line,
                "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u"
                " %*u %*u %*u %llu %llu %*d %*d %*d %*d %*d %*d %*u %llu %llu",
                usrtime, systime, vmsize, rssize) == 4)) {
      /* Convert real page count to real page size. */
      *rssize = *rssize * pageSize;
    } else {
      /* Kernel may be old or customized. */
      logger->printWarnMsg("Process data has shortage.");
    }
  } else {
    logger->printWarnMsg("Couldn't read process status.");
  }

  /* Cleanup. */
  if (likely(line != NULL)) {
    free(line);
  }
  fclose(proc);
  return true;
}

/*!
 * \brief Getting machine cpu times.
 * \param times [out] Machine cpu times information.
 * \return Prosess is succeed or failure.
 */
bool TLogManager::getSysTimes(TMachineTimes *times) {
  /* Initialize. */
  memset(times, 0, sizeof(TMachineTimes));

  /* Open machine status file. */
  FILE *proc = fopen("/proc/stat", "r");
  /* If failure open machine status file. */
  if (unlikely(proc == NULL)) {
    logger->printWarnMsgWithErrno("Could not open /proc/stat");
    return false;
  }

  char *line = NULL;
  size_t len = 0;
  bool flagFound = false;

  /* Loop for find "cpu" line. */
  while (likely(getline(&line, &len, proc) > 0)) {
    /* Check line. */
    if (unlikely(strncmp(line, "cpu ", 4) != 0)) {
      /* This line is non-target. */
      continue;
    }

    /* Parse cpu time information. */
    if (unlikely(sscanf(line,
                        "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                        &times->usrTime, &times->lowUsrTime, &times->sysTime,
                        &times->idleTime, &times->iowaitTime,
                        &times->sortIrqTime, &times->irqTime, &times->stealTime,
                        &times->guestTime) != 9)) {
      /* Maybe the kernel is old version. */
      logger->printWarnMsg("CPU status data has shortage.");
    }

    flagFound = true;
    break;
  }

  /* If not found cpu information. */
  if (unlikely(!flagFound)) {
    /* Maybe the kernel is modified. */
    logger->printWarnMsg("Not found cpu status data.");
  }

  /* Cleanup. */
  if (likely(line != NULL)) {
    free(line);
  }
  fclose(proc);
  return flagFound;
}

/*!
 * \brief Send log archive trap.
 * \param cause       [in] Invoke function cause.<br>
 *                         E.g. Signal, ResourceExhausted, Interval.
 * \param nowTime     [in] Log collect time.
 * \param path        [in] Archive file or directory path.
 * \param isDirectory [in] Param "path" is directory.
 * \return Value is true, if process is succeed.
 */
bool TLogManager::sendLogArchiveTrap(TInvokeCause cause, TMSecTime nowTime,
                                     char *path, bool isDirectory) {
  /* Return value. */
  bool result = true;

  /* Get full path. */
  char realSendPath[PATH_MAX] = {0};
  if (unlikely(realpath(path, realSendPath) == NULL)) {
    logger->printWarnMsgWithErrno("Could not get real path of archive file");
    return false;
  }

  /* If path is directory, then append "/" to path. */
  if (isDirectory) {
    size_t pathSize = strlen(realSendPath);
    pathSize = (pathSize >= PATH_MAX) ? PATH_MAX - 1 : pathSize;
    realSendPath[pathSize] = '/';
  }

  /* Output archive file path. */
  logger->printInfoMsg("Collecting log has been completed: %s", realSendPath);

  if (conf->SnmpSend()->get()) {
    /* Trap OID. */
    char trapOID[50] = OID_LOGARCHIVE;
    oid OID_LOG_PATH[] = {SNMP_OID_LOGARCHIVE, 1};
    oid OID_TROUBLE_DATE[] = {SNMP_OID_LOGARCHIVE, 2};
    /* Temporary buffer. */
    char buff[256] = {0};

    try {
      /* Send resource trap. */
      TTrapSender sender;

      /* Setting sysUpTime */
      sender.setSysUpTime();

      /* Setting trapOID. */
      sender.setTrapOID(trapOID);

      /* Set log archive path. */
      sender.addValue(OID_LOG_PATH, OID_LENGTH(OID_LOG_PATH), realSendPath,
                      SNMP_VAR_TYPE_STRING);

      /* Set date which JVM resource exhausted. */
      switch (cause) {
        case ResourceExhausted:
        case ThreadExhausted:
        case OccurredDeadlock:
          /* Collect archive by resource or deadlock. */
          snprintf(buff, 255, "%llu", nowTime);
          break;
        default:
          /* Collect archive by signal. */
          snprintf(buff, 255, "%llu", (TMSecTime)0LL);
      }

      sender.addValue(OID_TROUBLE_DATE, OID_LENGTH(OID_TROUBLE_DATE), buff,
                      SNMP_VAR_TYPE_COUNTER64);

      /* Send trap. */
      result = (sender.sendTrap() == SNMP_PROC_SUCCESS);
      if (unlikely(!result)) {
        /* Clean up. */
        sender.clearValues();
      }
    } catch (...) {
      result = false;
    }
  }

  return result;
}

/*!
 * \brief Collect files about JVM enviroment.
 * \param basePath [in] Temporary working directory.
 * \return Value is zero, if process is succeed.<br />
 *         Value is error number a.k.a. "errno", if process is failure.
 */
int TLogManager::copyInfoFiles(char const *basePath) {
  int result = 0;

  /* Copy distribution file list. */
  const char distFileList[][255] = {/* Distribution release. */
                                    "/etc/redhat-release",
                                    /* For other distribution. */
                                    "/etc/sun-release",
                                    "/etc/mandrake-release",
                                    "/etc/SuSE-release",
                                    "/etc/turbolinux-release",
                                    "/etc/gentoo-release",
                                    "/etc/debian_version",
                                    "/etc/ltib-release",
                                    "/etc/angstrom-version",
                                    "/etc/fedora-release",
                                    "/etc/vine-release",
                                    "/etc/issue",
                                    /* End flag. */
                                    {0}};
  bool flagCopyedDistFile = false;

  /* Copy distribution file. */
  for (int i = 0; strlen(distFileList[i]) > 0; i++) {
    result = copyFile(distFileList[i], basePath);

    if ((result == 0) || isRaisedDiskFull(result)) {
      flagCopyedDistFile = (result == 0);
      break;
    }
  }

  /* If failure copy distribution file. */
  if (unlikely(!flagCopyedDistFile)) {
    logger->printWarnMsgWithErrno("Could not copy distribution release file.");
  }

  /* If disk is full. */
  if (unlikely(isRaisedDiskFull(result))) {
    return result;
  }

  /* Copy file list. */
  const char copyFileList[][255] = {/* Process information. */
                                    "/proc/self/smaps",
                                    "/proc/self/limits",
                                    "/proc/self/cmdline",
                                    "/proc/self/status",
                                    /* Netstat infomation. */
                                    "/proc/net/tcp",
                                    "/proc/net/tcp6",
                                    "/proc/net/udp",
                                    "/proc/net/udp6",
                                    /* End flag. */
                                    {0}};

  /* Copy files in list. */
  for (int i = 0; strlen(copyFileList[i]) > 0; i++) {
    /* Copy file. */
    result = copyFile(copyFileList[i], basePath);
    if (unlikely(result != 0)) {
      logger->printWarnMsg("Could not copy file: %s", copyFileList[i]);

      /* If disk is full. */
      if (unlikely(isRaisedDiskFull(result))) {
        return result;
      }
    }
  }

  /* Collect Syslog or Systemd-Journald */
  /*
   * Try to copy syslog at first, because journal daemon will forward all
   * received
   * log messages to a traditional syslog by default.
   */
  result = copyFile("/var/log/messages", basePath);
  if (unlikely(result != 0)) {
    errno = result;
    logger->printWarnMsgWithErrno("Could not copy /var/log/messages");
    /* If disk is full. */
    if (unlikely(isRaisedDiskFull(result))) {
      return result;
    }
  }

  /* Collect systemd-journald instead of syslog when failed to copy syslog, */
  if (result != 0) {
    /*
     * Select vfork() to run journalctl in a separate process. Unlikely
     * system(),
     * this function does not block SIGINT, et al. Unlikely fork(), this
     * function
     * does not copy but shares all memory with its parent, including the stack.
     */
    pid_t child = vfork();
    if (child == 0) {
      /* Child process */
      /* logfile name shows what command is used to output it.*/
      char logfile[PATH_MAX];
      sprintf(logfile, "%s%s", basePath,
              "/journalctl_-q_--all_--this-boot_--no-pager_-o_verbose.log");
      /* Redirect child process' stdout/stderr to logfile */
      int fd = open(logfile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
      if (dup2(fd, 1) < 0) {
        close(fd);
        _exit(errno);
      }
      if (dup2(fd, 2) < 0) {
        close(fd);
        _exit(errno);
      }
      close(fd);
      /* Use execve() for journalctl to prevent command injection */
      const char *argv[] = {"journalctl", "-q", "--all",   "--this-boot",
                            "--no-pager", "-o", "verbose", NULL};
      extern char **environ;
      execve("/bin/journalctl", (char *const *)argv, environ);
      /* if execve returns, it has failed */
      _exit(errno);
    } else if (child < 0) {
      /* vfork failed */
      result = errno;
      logger->printWarnMsgWithErrno(
          "Could not collect systemd-journald log by vfork().");
    } else {
      /* Parent process */
      int status;
      if (waitpid(child, &status, 0) < 0) {
        /* The child process failed. */
        result = errno;
        logger->printWarnMsgWithErrno(
            "Could not collect systemd-journald log by process error.");
        /* If disk is full. */
        if (unlikely(isRaisedDiskFull(result))) {
          return result;
        }
      }
      if (WIFEXITED(status)) {
        /* The child exited normally, get the returns as result. */
        result = WEXITSTATUS(status);
        if (result != 0) {
          logger->printWarnMsg("Could not collect systemd-journald log.");
        }
      } else {
        /* The child exited with a signal or unknown exit code. */
        logger->printWarnMsg(
            "Could not collect systemd-journald log by signal or unknown exit "
            "code.");
      }
    }
  }

  /* Copy file descriptors, i.e. stdout and stderr, as avoid double work. */
  const int fdNum = 2;
  const char streamList[fdNum][255] = {/* Standard streams */
                                       "/proc/self/fd/1", "/proc/self/fd/2",
  };

  const char fdFile[fdNum][10] = {"fd1", "fd2"};
  char fdPath[fdNum][PATH_MAX];
  struct stat fdStat[fdNum];

  for (int i = 0; i < fdNum; i++) {
    realpath(streamList[i], fdPath[i]);

    if (unlikely(stat(fdPath[i], &fdStat[i]) != 0)) {
      /* Failure get file information. */
      result = errno;
      logger->printWarnMsgWithErrno("Could not get file information (stat).");
    } else if (i == 1 && result == 0 && fdStat[0].st_ino == fdStat[1].st_ino) {
      /* If stdout and stderr are redirected to same file, no need to copy the
       * file again. */
      break;
    } else {
      result = copyFile(streamList[i], basePath, fdFile[i]);
    }

    /* If catch a failure during the copy process, show warn messages. */
    if (unlikely(result != 0)) {
      errno = result;
      logger->printWarnMsgWithErrno("Could not copy file: %s", streamList[i]);

      /* If disk is full. */
      if (unlikely(isRaisedDiskFull(result))) {
        return result;
      }
    }
  }

  return result;
}

/*!
 * \brief Copy GC log file.
 * \param basePath [in] Path of temporary directory.
 * \return Value is zero, if process is succeed.<br />
 *         Value is error number a.k.a. "errno", if process is failure.
 */
int TLogManager::copyGCLogFile(char const *basePath) {
  char *aGCLogFile = (*gcLogFilename);

  /* If GC log filename isn't specified. */
  if (unlikely(aGCLogFile == NULL)) {
    return 0;
  }

  /* Copy file. */
  return copyFile(aGCLogFile, basePath);
}

/*!
 * \brief Create file about using socket by JVM.
 * \param basePath [in] Path of directory put report file.
 * \return Value is zero, if process is succeed.<br />
 *         Value is error number a.k.a. "errno", if process is failure.
 */
int TLogManager::makeSocketOwnerFile(char const *basePath) {
  int result = 0;

  /* Create filename. */
  char *sockOwnName = createFilename(basePath, "sockowner");
  /* If failure create file name. */
  if (unlikely(sockOwnName == NULL)) {
    result = errno;
    logger->printWarnMsg("Couldn't allocate filename.");
    return result;
  }

  /* Create sockowner. */
  int fd = open(sockOwnName, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
  free(sockOwnName);
  if (unlikely(fd < 0)) {
    result = errno;
    logger->printWarnMsgWithErrno("Could not open socket owner file.");

    return result;
  }

  /* Open directory. */
  DIR *dir = opendir("/proc/self/fd");

  /* If failure open directory. */
  if (unlikely(dir == NULL)) {
    result = errno;
    logger->printWarnMsgWithErrno("Could not open directory: /proc/self/fd");
    close(fd);
    return result;
  }

  /* Read file in directory. */
  struct dirent *entry = NULL;
  while ((entry = readdir(dir)) != NULL) {
    /* Check file name. */
    if (strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0) {
      continue;
    }

    /* Get maybe socket file full path. */
    char *socketPath = createFilename("/proc/self/fd", entry->d_name);
    if (unlikely(socketPath == NULL)) {
      result = errno;
      logger->printWarnMsg("Failure allocate working memory.");
      break;
    }

    /* Get file information. */
    struct stat st = {0};
    if (unlikely(stat(socketPath, &st) != 0)) {
      free(socketPath);
      continue;
    }
    free(socketPath);

    /* If this file is socket file. */
    if ((st.st_mode & S_IFMT) == S_IFSOCK) {
      /* Output i-node number of a socket file. */
      char buf[256] = {0};
      snprintf(buf, 255, INODENUM_FORMAT_STR "\n", st.st_ino);
      if (unlikely(write(fd, buf, strlen(buf)) < 0)) {
        result = errno;
        logger->printWarnMsgWithErrno("Could not write to socket owner.");
        break;
      }
    }
  }

  /* If failure in read directory. */
  if (unlikely(close(fd) != 0 && result == 0)) {
    result = errno;
    logger->printWarnMsgWithErrno("Could not close socket owner.");
  }

  /* Cleanup. */
  closedir(dir);

  return result;
}

/*!
 * \brief Create archive file path.
 * \param nowTime [in] Log collect time.
 * \return Return archive file name, if process is succeed.<br>
 *         Don't forget deallocate memory.<br>
 *         Process is failure, if value is null.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
char *TLogManager::createArchiveName(TMSecTime nowTime) {
  time_t nowTimeSec = 0;
  struct tm time_struct = {0};
  char time_str[20] = {0};
  char arcName[PATH_MAX] = {0};
  char extPart[PATH_MAX] = {0};
  char namePart[PATH_MAX] = {0};

  /* Search extension. */
  char *archiveFileName = conf->ArchiveFile()->get();
  char *extPos = strrchr(archiveFileName, '.');
  if (likely(extPos != NULL)) {
    /* Path and extension store each other. */
    strncpy(extPart, extPos, PATH_MAX);
    strncpy(namePart, archiveFileName, (extPos - archiveFileName));
  } else {
    /* Not found extension in path. */
    strncpy(namePart, archiveFileName, PATH_MAX);
  }

  /* Get now datetime and convert to string. */
  nowTimeSec = (time_t)(nowTime / 1000);
  localtime_r((const time_t *)&nowTimeSec, &time_struct);
  strftime(time_str, 20, "%y%m%d%H%M%S", &time_struct);

  /* Create file name. */
  int ret = snprintf(arcName, PATH_MAX, "%s%s%s", namePart, time_str, extPart);
  if (ret >= PATH_MAX) {
    logger->printCritMsg("Archive name is too long: %s", arcName);
    return NULL;
  }

  /* Create unique file name. */
  return createUniquePath(arcName, false);
}
#pragma GCC diagnostic pop

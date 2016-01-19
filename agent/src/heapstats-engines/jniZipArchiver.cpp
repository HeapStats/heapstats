/*!
 * \file jniZipArchiver.cpp
 * \brief This file is used create archive file to use java zip library.
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

#include <dirent.h>
#include <fcntl.h>

#include "globals.hpp"
#include "fsUtil.hpp"
#include "jniZipArchiver.hpp"

/*!
 * \brief Refference of "BufferedOutputStream" class data.
 */
jclass TJniZipArchiver::clsBuffOutStrm = NULL;

/*!
 * \brief Refference of "BufferedOutputStream" class constructer.
 */
jmethodID TJniZipArchiver::clsBuffOutStrm_init = NULL;

/*!
 * \brief Refference of "FileOutputStream" class data.
 */
jclass TJniZipArchiver::clsFileOutStrm = NULL;

/*!
 * \brief Refference of "FileOutputStream" class constructer.
 */
jmethodID TJniZipArchiver::clsFileOutStrm_init = NULL;

/*!
 * \brief Refference of "ZipOutputStream" class data.
 */
jclass TJniZipArchiver::clsZipOutStrm = NULL;

/*!
 * \brief Refference of "ZipOutputStream" class constructer.
 */
jmethodID TJniZipArchiver::clsZipOutStrm_init = NULL;

/*!
 * \brief Refference of "ZipEntry" class data.
 */
jclass TJniZipArchiver::clsZipEntry = NULL;

/*!
 * \brief Refference of "ZipEntry" class constructer.
 */
jmethodID TJniZipArchiver::clsZipEntry_init = NULL;

/*!
 * \brief Refference of "ZipOutputStream" class method "close".
 */
jmethodID TJniZipArchiver::clsZipOutStrm_close = NULL;

/*!
 * \brief Refference of "ZipOutputStream" class method "closeEntry".
 */
jmethodID TJniZipArchiver::clsZipOutStrm_closeEntry = NULL;

/*!
 * \brief Refference of "ZipOutputStream" class method "putNextEntry".
 */
jmethodID TJniZipArchiver::clsZipOutStrm_putNextEntry = NULL;

/*!
 * \brief Refference of "ZipOutputStream" class method "write".
 */
jmethodID TJniZipArchiver::clsZipOutStrm_write = NULL;

/*!
 * \brief Refference of "ZipOutputStream" class method "flush".
 * \sa FilterOutputStream::flush
 */
jmethodID TJniZipArchiver::clsZipOutStrm_flush = NULL;

/*!
 * \brief Flag of load initialize data.
 */
bool TJniZipArchiver::loadFlag = false;

/*!
 * \brief Global initialization.
 * \param env [in] JNI environment object.
 * \return Process is succeed, if value is true.<br>
 *         Process is failure, if value is false.
 * \warning Please call only once from main thread.
 */
bool TJniZipArchiver::globalInitialize(JNIEnv *env) {
  /* If already loaded data. */
  if (loadFlag) {
    logger->printWarnMsg("Already initialized jni archiver.");
    return true;
  }
  loadFlag = true;

  /* Search class and constructor method. */

  /* Search class list. */
  const struct {
    jclass *pClass;
    char className[255];
    jmethodID *pClassInit;
    char initArgs[255];
  } loadClassList[] = {
        {&clsBuffOutStrm, "Ljava/io/BufferedOutputStream;",
         &clsBuffOutStrm_init, "(Ljava/io/OutputStream;)V"},
        {&clsFileOutStrm, "Ljava/io/FileOutputStream;", &clsFileOutStrm_init,
         "(Ljava/lang/String;)V"},
        {&clsZipOutStrm, "Ljava/util/zip/ZipOutputStream;", &clsZipOutStrm_init,
         "(Ljava/io/OutputStream;)V"},
        {&clsZipEntry, "Ljava/util/zip/ZipEntry;", &clsZipEntry_init,
         "(Ljava/lang/String;)V"},
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
      env->DeleteLocalRef(localRefClass);
      logger->printWarnMsg("Couldn't get global reference.");
      return false;
    }

    /* Get class constructor. */
    (*loadClassList[i].pClassInit) =
        env->GetMethodID(localRefClass, "<init>", loadClassList[i].initArgs);
    /* If failure getting constructor. */
    if (unlikely((*loadClassList[i].pClassInit) == NULL)) {
      handlePendingException(env);
      env->DeleteLocalRef(localRefClass);
      logger->printWarnMsg("Couldn't get constructor of common class.");
      return false;
    }

    /* Clearup. */
    env->DeleteLocalRef(localRefClass);
  }

  /* Search other method. */

  /* Search method list. */
  const struct {
    jclass pClass;
    jmethodID *pMethod;
    char methodName[20];
    char methodParam[50];
  } methodList[] = {
        {clsZipOutStrm, &clsZipOutStrm_close, "close", "()V"},
        {clsZipOutStrm, &clsZipOutStrm_closeEntry, "closeEntry", "()V"},
        {clsZipOutStrm, &clsZipOutStrm_putNextEntry, "putNextEntry",
         "(Ljava/util/zip/ZipEntry;)V"},
        {clsZipOutStrm, &clsZipOutStrm_write, "write", "([BII)V"},
        {clsZipOutStrm, &clsZipOutStrm_flush, "flush", "()V"},
        {NULL, NULL, {0}, {0}} /* End flag. */
    };

  /* Search class method. */
  for (int i = 0; methodList[i].pMethod != NULL; i++) {
    jmethodID methodID = NULL;

    /* Search method in designated class. */
    methodID = env->GetMethodID(methodList[i].pClass, methodList[i].methodName,
                                methodList[i].methodParam);

    /* If failure getting function. */
    if (unlikely(methodID == NULL)) {
      handlePendingException(env);
      logger->printWarnMsg("Couldn't get function of jni zip archive.");
      return false;
    }

    (*methodList[i].pMethod) = methodID;
  }

  return true;
}

/*!
 * \brief Global finalize.
 * \param env [in] JNI environment object.
 * \return Process is succeed, if value is true.<br>
 *         Process is failure, if value is false.
 * \warning Please call only once from main thread.
 */
bool TJniZipArchiver::globalFinalize(JNIEnv *env) {
  /* Sanity check. */
  if (!loadFlag) {
    logger->printWarnMsg("Didn't initialize jni archiver yet.");
    return false;
  }
  loadFlag = false;

  /* Delete common data's global reference. */
  if (likely(clsBuffOutStrm != NULL)) {
    env->DeleteGlobalRef(clsBuffOutStrm);
    clsBuffOutStrm = NULL;
  }

  if (likely(clsFileOutStrm != NULL)) {
    env->DeleteGlobalRef(clsFileOutStrm);
    clsFileOutStrm = NULL;
  }

  if (likely(clsZipOutStrm != NULL)) {
    env->DeleteGlobalRef(clsZipOutStrm);
    clsZipOutStrm = NULL;
  }

  if (likely(clsZipEntry != NULL)) {
    env->DeleteGlobalRef(clsZipEntry);
    clsZipEntry = NULL;
  }

  return true;
}

/*!
 * \brief TJniZipArchiver constructor.
 */
TJniZipArchiver::TJniZipArchiver(void) : TArchiveMaker() {
  /* If don't loaded data yet. */
  if (unlikely(!loadFlag)) {
    throw "Didn't initialize jni archiver yet.";
  }
}

/*!
 * \brief TJniZipArchiver destructor.
 */
TJniZipArchiver::~TJniZipArchiver(void) { /* Do Nothing. */ }

/*!
 * \brief Do file archive and create archive file.
 * \param env         [in] JNI environment object.
 * \param archiveFile [in] archive file name.
 * \return Response code of execute commad line.
 */
int TJniZipArchiver::doArchive(JNIEnv *env, char const *archiveFile) {
  /* Sanity check. */
  if (unlikely(strlen(this->getTarget()) == 0 || archiveFile == NULL ||
               strlen(archiveFile) == 0)) {
    logger->printWarnMsg("Illegal archive paramter.");
    clear();
    return -1;
  }

  /* Exec command. */
  int result = execute(env, archiveFile);

  /* Cleanup. */
  clear();

  /* Succeed. */
  return result;
}

/*!
 * \brief Execute archive.
 * \param env         [in] JNI environment object.
 * \param archiveFile [in] archive file name.
 * \return Response code of execute archive.
 */
int TJniZipArchiver::execute(JNIEnv *env, char const *archiveFile) {
  /* Variable to store return code. */
  int result = 0;
  /* Variable to store java object. */
  jstring jArcFile = NULL;
  jobject jFileOutStrm = NULL;
  jobject jBuffOutStrm = NULL;
  jobject jZipOutStrm = NULL;

  /* Create archive file name. */
  jArcFile = env->NewStringUTF(archiveFile);

  /* If failure any process. */
  if (unlikely(jArcFile == NULL)) {
    logger->printWarnMsg("Could not allocate jni zip archive name");
    handlePendingException(env);
    return -1;
  }

  /* Create java object to make archive file. */
  jFileOutStrm = env->NewObject(clsFileOutStrm, clsFileOutStrm_init, jArcFile);
  if (unlikely(jFileOutStrm == NULL)) {
    result = -1;
  } else {
    jBuffOutStrm =
        env->NewObject(clsBuffOutStrm, clsBuffOutStrm_init, jFileOutStrm);
    if (unlikely(jBuffOutStrm == NULL)) {
      result = -1;
    } else {
      jZipOutStrm =
          env->NewObject(clsZipOutStrm, clsZipOutStrm_init, jBuffOutStrm);
      if (unlikely(jZipOutStrm == NULL)) {
        result = -1;
      }
    }
  }

  /* Cleanup. */
  env->DeleteLocalRef(jArcFile);

  /* If creation stream object is successe. */
  if (unlikely(result == 0)) {
    /* Write file data to java zip stream. */
    result = writeFiles(env, jZipOutStrm);

    /* Close zip stream. */
    env->CallVoidMethod(jZipOutStrm, clsZipOutStrm_close);
    if (unlikely(env->ExceptionOccurred() != NULL && result == 0)) {
      result = (errno != 0) ? errno : -1;
      logger->printWarnMsgWithErrno("Could not write to jni zip archive");
    }
  }

  /* If failure create stream object. */
  if (unlikely(result != 0)) {
    env->ExceptionClear();
  }

  /* Cleanup. */
  if (likely(jZipOutStrm != NULL)) {
    env->DeleteLocalRef(jZipOutStrm);
  }
  if (likely(jBuffOutStrm != NULL)) {
    env->DeleteLocalRef(jBuffOutStrm);
  }
  if (likely(jFileOutStrm != NULL)) {
    env->DeleteLocalRef(jFileOutStrm);
  }

  /* If failure any process. */
  if (unlikely(result != 0)) {
    /* Remove file. Because it's maybe broken. */
    unlink(archiveFile);
  }

  return result;
}

/*!
 * \brief Write files to archive.
 * \param env        [in] JNI environment object.
 * \param jZipStream [in] JNI zip archive stream object.
 * \return Response code of execute archive.
 */
int TJniZipArchiver::writeFiles(JNIEnv *env, jobject jZipStream) {
  /* Variable to store return code. */
  int result = 0;
  /* Variable to operate entry in directory. */
  DIR *dir = NULL;
  struct dirent *entry = NULL;
  /* Variable for write file data. */
  ssize_t readSize = 0;
  char buff[255];
  jbyteArray jByteArray = NULL;

  /* Open directory. */
  dir = opendir(this->getTarget());
  /* Creaet byte array. */
  jByteArray = env->NewByteArray(sizeof(buff));

  /* If failure any process. */
  if (unlikely(dir == NULL || jByteArray == NULL)) {
    int raisedErrNum = errno;
    if (likely(dir != NULL)) {
      closedir(dir);
    }
    if (likely(jByteArray != NULL)) {
      env->DeleteLocalRef(jByteArray);
    }
    return ((raisedErrNum != 0) ? raisedErrNum : -1);
  }

  int raisedErrNum = 0;
  try {
    /* Loop for file in designed working directory. */
    while ((entry = readdir(dir)) != NULL) {
      char *filePath = NULL;
      jstring jTargetFile = NULL;
      jobject jZipEntry = NULL;
      int fd = -1;

      /* Check file name. */
      if (strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0) {
        continue;
      }

      /* Create source file path. */
      filePath = createFilename(this->getTarget(), entry->d_name);
      if (unlikely(filePath == NULL)) {
        raisedErrNum = errno;
        throw 1;
      }

      /* Create zip entry name without directory path. */
      jTargetFile = env->NewStringUTF(entry->d_name);
      if (unlikely(jTargetFile == NULL)) {
        raisedErrNum = errno;
        free(filePath);
        throw 1;
      }

      /* Create zip entry and cleanup. */
      jZipEntry = env->NewObject(clsZipEntry, clsZipEntry_init, jTargetFile);
      raisedErrNum = errno;
      env->DeleteLocalRef(jTargetFile);

      /* If failure create zip entry. */
      if (unlikely(jZipEntry == NULL)) {
        free(filePath);
        throw 1;
      }

      /* Setting zip entry and cleanup. */
      env->CallVoidMethod(jZipStream, clsZipOutStrm_putNextEntry, jZipEntry);
      raisedErrNum = errno;
      env->DeleteLocalRef(jZipEntry);

      /* If failure setting zip entry. */
      if (unlikely(env->ExceptionOccurred() != NULL)) {
        free(filePath);
        throw 1;
      }

      /* Open source file. */
      fd = open(filePath, O_RDONLY, S_IRUSR);
      free(filePath);

      if (unlikely(fd >= 0)) {
        raisedErrNum = 0;

        /* Write source file to zip archive. */
        while ((readSize = read(fd, buff, sizeof(buff))) > 0) {
          /* Copy native byte array to java byte array. */
          env->SetByteArrayRegion(jByteArray, 0, readSize, (jbyte *)buff);
          if (unlikely(env->ExceptionOccurred() != NULL)) {
            raisedErrNum = errno;
            break;
          }

          /* Write data. */
          env->CallVoidMethod(jZipStream, clsZipOutStrm_write, jByteArray,
                              (jint)0, (jint)readSize);
          if (unlikely(env->ExceptionOccurred() != NULL)) {
            raisedErrNum = errno;
            break;
          }
        }

        if (unlikely(close(fd) < 0 || raisedErrNum != 0)) {
          raisedErrNum = (raisedErrNum == 0) ? errno : raisedErrNum;
          throw 1;
        }
      } else {
        result = errno;
        /* Open file process is Failed. */
        logger->printWarnMsgWithErrno("Could not open jni zip source file");
        break;
      }

      /* Close zip entry. */
      env->CallVoidMethod(jZipStream, clsZipOutStrm_closeEntry);
      if (unlikely(env->ExceptionOccurred() != NULL)) {
        raisedErrNum = errno;
        throw 1;
      }
    }

    /* Flush zip data. */
    env->CallVoidMethod(jZipStream, clsZipOutStrm_flush);
    if (unlikely(env->ExceptionOccurred() != NULL)) {
      raisedErrNum = errno;
      throw 1;
    }
  } catch (...) {
    result = (raisedErrNum != 0) ? raisedErrNum : -1;
    logger->printWarnMsgWithErrno("Could not write to jni zip archive.");
    env->ExceptionClear();
  }

  /* Cleanup. */
  closedir(dir);
  env->DeleteLocalRef(jByteArray);

  return result;
}

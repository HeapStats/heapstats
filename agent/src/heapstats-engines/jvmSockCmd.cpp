/*!
 * \file jvmSockCmd.cpp
 * \brief This file is used by thread dump.
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

#include <fcntl.h>
#include <signal.h>

#include "globals.hpp"
#include "util.hpp"
#include "jvmSockCmd.hpp"

/*!
 * \brief JVM socket command protocol version.
 */
const char JVM_CMD_VERSION[2] = "1";

/*!
 * \brief TJVMSockCmd constructor.
 * \param temporaryPath [in] Temporary path of JVM.
 */
TJVMSockCmd::TJVMSockCmd(char const* temporaryPath) {
  /* Initialize fields. */
  memset(socketPath, 0, sizeof(socketPath));
  memset(tempPath, 0, sizeof(tempPath));

  /* Copy. */
  if (likely(temporaryPath != NULL)) {
    strncpy(tempPath, temporaryPath, sizeof(tempPath) - 1);
  } else {
    /* Set default temporary path. */
    strcpy(tempPath, "/tmp");
  }

  /* Create socket file to JVM. */
  createJvmSock();
}

/*!
 * \brief TJVMSockCmd destructor.
 */
TJVMSockCmd::~TJVMSockCmd(void) { /* None. */ }

/*!
 * \brief Execute command without params, and save response.
 * \param cmd      [in] Execute command string.
 * \param filename [in] Output file path.
 * \return Response code of execute commad line.<br>
 *         Execute command is succeed, if value is 0.<br>
 *         Value is error code, if failure execute command.<br>
 *         Even so the file was written response data, if failure.
 */
int TJVMSockCmd::exec(char const* cmd, char const* filename) {
  /* Empty paramters. */
  const TJVMSockCmdArgs conf = {{0}, {0}, {0}};

  return execute(cmd, conf, filename);
}

/*!
 * \brief Execute command, and save response.
 * \param cmd      [in] Execute command string.
 * \param conf     [in] Execute command arguments.
 * \param filename [in] Output file path.
 * \return Response code of execute commad line.<br>
 *         Execute command is succeed, if value is 0.<br>
 *         Value is error code, if failure execute command.<br>
 *         Even so the file was written response data, if failure.
 */
int TJVMSockCmd::execute(char const* cmd, const TJVMSockCmdArgs conf,
                         char const* filename) {
  /* If don't open socket yet. */
  if (unlikely(!isConnectable())) {
    /* If failure open JVM socket. */
    if (unlikely(!createJvmSock())) {
      logger->printWarnMsg("Failure open socket.");
      return -1;
    }
  }

  /* Open socket. */
  int socketFD = openJvmSock(socketPath);
  /* Socket isn't open yet. */
  if (unlikely(socketFD < 0)) {
    logger->printWarnMsg("Socket isn't open yet.");
    return -1;
  }

  int returnCode = 0;
  /* Create response file. */
  int fd = open(filename, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
  if (unlikely(fd < 0)) {
    returnCode = errno;
    logger->printWarnMsgWithErrno("Could not create threaddump file");
    close(socketFD);
    return returnCode;
  }

  /*
   * About JVM socket command
   * format:
   *    ~version~\0~command~\0~param1~\0~param2~\0~param3~\0
   * version:
   *    Always version is "1".
   * command:
   *    "threaddump"  Dump information of threads on JVM.
   *    "inspectheap" Seems to like "PrintClassHistogram".
   *    "datadump"    Seems to like when press CTRL and \ key.
   *    "heapdump"    Dump heap to file like "jhat".
   *    etc..
   * param1~3:
   *    paramter is fixed three param.
   *    If no need paramter , but should write '\0'.
   * return:
   *    '0'   Succeed in command and return result data.
   *    other Raised error and return error messages.
   */

  /* Result code variable. */
  char result = '\0';
  /* Wait socket response. */
  const int WAIT_LIMIT = 1000;
  int waitCount = 0;
  try {
    /* Write command protocol version. */
    if (unlikely(write(socketFD, JVM_CMD_VERSION, strlen(JVM_CMD_VERSION) + 1) <
                 0)) {
      returnCode = errno;
      logger->printWarnMsgWithErrno("Could not send threaddump command to JVM");
      throw 1;
    }

    /* Write command string. */
    if (unlikely(write(socketFD, cmd, strlen(cmd) + 1) < 0)) {
      returnCode = errno;
      logger->printWarnMsgWithErrno("Could not send threaddump command to JVM");
      throw 2;
    }

    /* Write three params */
    for (int i = 0; i < 3; i++) {
      if (unlikely(write(socketFD, conf[i], strlen(conf[i]) + 1) < 0)) {
        returnCode = errno;
        logger->printWarnMsgWithErrno(
            "Could not send threaddump command to JVM");
        throw 3;
      }
    }

    for (; waitCount <= WAIT_LIMIT; waitCount++) {
      /* Sleep for 1 milli-second. */
      littleSleep(0, 1000000);

      /* Check response. */
      if (read(socketFD, &result, sizeof(result)) > 0) {
        break;
      }
    }

    /* Read respone and Write to file. */
    char buff[255];
    ssize_t readByte = 0;
    while ((readByte = read(socketFD, buff, sizeof(buff))) > 0) {
      if (unlikely(write(fd, buff, readByte) < 0)) {
        returnCode = errno;
        logger->printWarnMsgWithErrno("Could not receive threaddump from JVM");
        throw 4;
      }
    }
  } catch (...) {
    ; /* Failed to write file. */
  }

  /* Cleanup. */
  close(socketFD);
  if (unlikely(close(fd) < 0 && returnCode == 0)) {
    returnCode = errno;
    logger->printWarnMsgWithErrno("Could not close socket to JVM");
  }

  /* Check command execute result. */
  if (unlikely(waitCount > WAIT_LIMIT)) {
    /* Maybe JVM is busy. */
    logger->printWarnMsg("AttachListener does not respond.");
    return -1;

  } else if (unlikely(result != '0')) {
    /* Illegal command or other error. */
    logger->printWarnMsg("Failure execute socket command.");
    return result;
  }

  /* Succeed or file error. */
  return returnCode;
}

/*!
 * \brief Create JVM socket file.
 * \return Process result.
 */
bool TJVMSockCmd::createJvmSock(void) {
  /* Socket file path. */
  char sockPath[PATH_MAX + 1] = {0};

  /* Search jvm socket file. */
  if (!findJvmSock((char*)&sockPath, PATH_MAX)) {
    /* Attach file path. */
    char attachPath[PATH_MAX + 1] = {0};

    /* No exists so create socket file. */
    if (unlikely(!createAttachFile((char*)&attachPath, PATH_MAX))) {
      /* Failure create attach-file or send signal. */
      logger->printWarnMsg("Failure create socket.");
      if (strlen(attachPath) != 0) {
        unlink(attachPath);
      }
      return false;

    } else {
      /* Wait for JVM socket file. */
      bool isFoundSocket = false;
      for (int waitCount = 0; waitCount < 1000; waitCount++) {
        /* Sleep for 1 milli-second. */
        littleSleep(0, 1000000);

        /* Recheck socket exists. */
        if (findJvmSock((char*)&sockPath, PATH_MAX)) {
          isFoundSocket = true;
          break;
        }
      }

      unlink(attachPath);
      if (unlikely(!isFoundSocket)) {
        /* Not found socket file. */
        logger->printWarnMsg("Failure find JVM socket.");
        return false;
      }
    }
  }

  /* Copy socket path. */
  strncpy(socketPath, sockPath, PATH_MAX);

  /* Succeed. */
  return true;
}

/*!
 * \brief Open socket to JVM.
 * \param path [in] Path of socket file.
 * \return Socket file descriptor.
 */
int TJVMSockCmd::openJvmSock(char* path) {
  /* Sanity check. */
  if (unlikely(path == NULL || strlen(path) == 0)) {
    return -1;
  }

  /* Socket structure. */
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;

  /* Create socket. */
  int fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (unlikely(fd < 0)) {
    /* Failure create socket. */
    return -1;
  }

  /* Copy socket path. */
  strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

  /* Connect socket. */
  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    /* Cleanup. */
    close(fd);
    return -1;
  }

  /* Return socket file descriptor. */
  return fd;
}

/*!
 * \brief Find JVM socket file and get path.
 * \param path    [out] Path of socket file.
 * \param pathLen [in]  Max length of param "path".
 * \return Search result.
 */
bool TJVMSockCmd::findJvmSock(char* path, int pathLen) {
  /* Sanity check. */
  if (unlikely(path == NULL || pathLen <= 0)) {
    return false;
  }

  /*
   * Memo
   *  Socket file is different path by JDK version.
   *
   * JDK: Most OracleJDK, OpenJDK.
   * Path: "/tmp".
   *
   * JDK: OracleJDK u23/u24, Same OpenJDK.
   * Path: System.getProperity("java.io.tmpdir").
   *
   * JDK: Few OracleJDK, OpenJDK.
   * Path: CWD or "/tmp".
   */

  char sockPath[PATH_MAX] = {0};
  pid_t selfPid = getpid();

  /* Open socket at normally temporary directory. */
  snprintf(sockPath, PATH_MAX, "/tmp/.java_pid%d", selfPid);
  int fd = openJvmSock(sockPath);

  if (fd < 0) {
    /* Open socket at designated temporary directory. */
    snprintf(sockPath, PATH_MAX, "%s/.java_pid%d", tempPath, selfPid);
    fd = openJvmSock(sockPath);
  }

  if (fd < 0) {
    /* Open socket at CWD. */
    snprintf(sockPath, PATH_MAX, "/proc/%d/cwd/.java_pid%d", selfPid, selfPid);
    fd = openJvmSock(sockPath);
  }

  /* If no exists sock. */
  if (unlikely(fd < 0)) {
    return false;
  }

  /* Cleanup. */
  close(fd);

  /* Copy path. */
  strncpy(path, sockPath, pathLen);

  return true;
}

/*!
 * \brief Create attach file.
 * \param path    [out] Path of created attach file.
 * \param pathLen [in]  Max length of param "path".
 * \return Process result.
 */
bool TJVMSockCmd::createAttachFile(char* path, int pathLen) {
  /* Sanity check. */
  if (unlikely(path == NULL || pathLen <= 0)) {
    return false;
  }

  /*
   * Memo
   *  Attach file is also different path by JDK version.
   *  JVM dosen't recognize attach file,
   *  if make attach file under incorrect path.
   *
   * JDK: Most OracleJDK, OpenJDK
   * Path: "/tmp" and CWD.
   *
   * JDK: OracleJDK u23/u24, Same OpenJDK
   * Path: System.getProperity("java.io.tmpdir") and CWD.
   */

  char attachPath[PATH_MAX] = {0};
  pid_t selfPid = getpid();
  int fd = 0;

  /* Create attach-file at current directory. */
  snprintf(attachPath, PATH_MAX, "/proc/%d/cwd/.attach_pid%d", selfPid,
           selfPid);
  fd = open(attachPath, O_CREAT | O_WRONLY,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (unlikely(fd < 0)) {
    /* Create attach-file at designated temporary directory. */
    snprintf(attachPath, PATH_MAX, "%s/.attach_pid%d", tempPath, selfPid);
    fd = open(attachPath, O_CREAT | O_WRONLY,
              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  }

  if (unlikely(fd < 0)) {
    /* Create attach-file at normally temporary directory. */
    snprintf(attachPath, PATH_MAX, "/tmp/.attach_pid%d", selfPid);
    fd = open(attachPath, O_CREAT | O_WRONLY,
              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  }

  /* If failure create attach-file. */
  if (unlikely(fd < 0)) {
    logger->printWarnMsgWithErrno("Could not create attach file");
    return false;
  }

  /* Cleanup and copy attach file path. */
  close(fd);
  strncpy(path, attachPath, pathLen);

  /* Notification means "please create and listen socket" to JVM. */
  if (unlikely(kill(selfPid, SIGQUIT) != 0)) {
    logger->printWarnMsgWithErrno("Could not send SIGQUIT to JVM");
    return false;
  }

  return true;
}

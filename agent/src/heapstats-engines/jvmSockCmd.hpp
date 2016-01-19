/*!
 * \file jvmSockCmd.hpp
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

#ifndef _JVM_SOCK_CMD_H
#define _JVM_SOCK_CMD_H

#include <limits.h>
#include <string.h>

/*!
 * \brief Structure of command arguments information.
 */
typedef char TJVMSockCmdArgs[3][255];

/*!
 * \brief This class execute command in JVM socket.
 */
class TJVMSockCmd {
 public:
  /*!
   * \brief TJVMSockCmd constructor.
   * \param temporaryPath [in] Temporary path of JVM.
   */
  TJVMSockCmd(char const* temporaryPath);

  /*!
   * \brief TJVMSockCmd destructor.
   */
  virtual ~TJVMSockCmd(void);

  /*!
   * \brief Execute command without params, and save response.
   * \param cmd      [in] Execute command string.
   * \param filename [in] Output file path.
   * \return Response code of execute commad line.<br>
   *         Execute command is succeed, if value is 0.<br>
   *         Value is error code, if failure execute command.<br>
   *         Even so the file was written response data, if failure.
   */
  int exec(char const* cmd, char const* filename);

  /*!
   * \brief Get connectable socket to JVM.
   * \return Is connectable socket.
   */
  inline bool isConnectable(void) { return strlen(socketPath) > 0; };

 protected:
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
  virtual int execute(char const* cmd, const TJVMSockCmdArgs conf,
                      char const* filename);

  /*!
   * \brief Create JVM socket file.
   * \return Process result.
   */
  virtual bool createJvmSock(void);

  /*!
   * \brief Open socket to JVM.
   * \param path [in] Path of socket file.
   * \return Socket file descriptor.
   */
  virtual int openJvmSock(char* path);

  /*!
   * \brief Find JVM socket file and get path.
   * \param path    [out] Path of socket file.
   * \param pathLen [in]  Max length of param "path".
   * \return Search result.
   */
  virtual bool findJvmSock(char* path, int pathLen);

  /*!
   * \brief Create attach file.
   * \param path    [out] Path of created attach file.
   * \param pathLen [in]  Max length of param "path".
   * \return Process result.
   */
  virtual bool createAttachFile(char* path, int pathLen);

 private:
  /*!
   * \brief Socket file path.
   */
  char socketPath[PATH_MAX];

  /*!
   * \brief Temporary directory path.
   */
  char tempPath[PATH_MAX];
};

#endif  // _JVM_SOCK_CMD_H

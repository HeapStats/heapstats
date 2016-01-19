/*!
 * \file fsUtil.hpp
 * \brief This file is utilities to access file system.
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
#ifndef _FS_UTIL_H
#define _FS_UTIL_H

#include <errno.h>

/*!
 * \brief Copy data as avoid overwriting.
 * \param sourceFile [in] Path of source file.
 * \param destPath   [in] Path of directory put duplicated file.
 * \param destName   [in] Name of duplicated file.<br>
 *                        Don't rename, if value is null.
 * \return Value is zero, if process is succeed.<br />
 *         Value is error number a.k.a. "errno", if process is failure.
 */
int copyFile(char const* sourceFile, char const* destPath,
             char const* destName = NULL);

/*!
 * \brief Create filename in expected path.
 * \param basePath [in] Path of directory.
 * \param filename [in] filename.
 * \return Create string.<br>
 *         Process is failure, if return is null.<br>
 *         Need call "free", if return is not null.
 */
char* createFilename(char const* basePath, char const* filename);

/*!
 * \brief Resolve canonicalized path path and check regular file or not.
 * \param path [in]  Path of the target file.
 * \param rpath [out] Real path which is pointed at "path".
 *        Buffer size must be enough to store path (we recommend PATH_MAX).
 * \return If "path" is copiable, this function returns true.
 */
bool isCopiablePath(const char* path, char* rpath);

/*!
 * \brief Create temporary directory in designated directory.
 * \param basePath   [out] Path of temporary directory.<br>
 *                         Process is failure, if return is null.<br>
 *                         Need call "free", if return is not null.
 * \param wishesName [in]  Name of one's wishes directory name.
 * \return Value is zero, if process is succeed.<br />
 *         Value is error number a.k.a. "errno", if process is failure.
 */
int createTempDir(char** basePath, char const* wishesName);

/*!
 * \brief Remove temporary directory.
 * \param basePath [in] Path of temporary directory.
 */
void removeTempDir(char const* basePath);

/*!
 * \brief Create unique path.
 * \param path        [in] Path.
 * \param isDirectory [in] Path is directory.
 * \return Unique path.<br>Don't forget deallocate.
 */
char* createUniquePath(char* path, bool isDirectory);

/*!
 * \brief Get parent directory path.
 * \param path [in] Path which file or directory.
 * \return Parent directory path.<br>Don't forget deallocate.
 */
char* getParentDirectoryPath(char const* path);

/*!
 * \brief Get accessible of directory.
 * \param path      [in] Directory path.
 * \param needRead  [in] Accessible flag about file read.
 * \param needWrite [in] Accessible flag about file write.
 * \return Value is zero, if process is succeed.<br />
 *         Value is error number a.k.a. "errno", if process is failure.
 */
int isAccessibleDirectory(char const* path, bool needRead, bool needWrite);

/*!
 * \brief Check that path is accessible.
 * \param path [in] A path to file.
 * \return Path is accessible.
 */
bool isValidPath(const char* path);

/*!
 * \brief Check disk full error.<br />
 *        If error is disk full, then print alert message.
 * \param aErrorNum [in] Number of error code.
 * \return Is designated error number means disk full.
 */
inline bool isRaisedDiskFull(const int aErrorNum) {
  return (aErrorNum == ENOSPC);
}

/*!
 * \brief Check disk full error.<br />
 *        If error is disk full, then print alert message.
 * \param aErrorNum [in] Number of error code.
 * \param workName  [in] Name of process when found disk full error.
 * \return Is designated error number means disk full.
 */
inline bool checkDiskFull(const int aErrorNum, char const* workName) {
  bool result = isRaisedDiskFull(aErrorNum);
  if (unlikely(result)) {
    /* Output alert message. */
    logger->printWarnMsg(
        "ALERT(DISKFULL): Designated disk is full"
        " for file output. work:\"%s\"",
        workName);
  }
  return result;
}

#endif  // _FS_UTIL_H

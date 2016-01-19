/*!
 * \file archiveMaker.hpp
 * \brief This file is used create archive file.
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

#ifndef _ARCHIVE_MAKER_H
#define _ARCHIVE_MAKER_H

#include <jvmti.h>
#include <jni.h>

#include <limits.h>

/*!
 * \brief This class create archive file.
 */
class TArchiveMaker {
 public:
  /*!
   * \brief TArchiveMaker constructor.
   */
  TArchiveMaker(void);
  /*!
   * \brief TArchiveMaker destructor.
   */
  virtual ~TArchiveMaker(void);

  /*!
   * \brief Setting target file path.
   * \param targetPath [in] Path of archive target file.
   */
  void setTarget(char const* targetPath);

  /*!
   * \brief Do file archive and create archive file.
   * \param env         [in] JNI environment object.
   * \param archiveFile [in] archive file name.
   * \return Response code of execute commad line.
   */
  virtual int doArchive(JNIEnv* env, char const* archiveFile) = 0;

  /*!
   * \brief Clear archive file data.
   */
  void clear(void);

 protected:
  /*!
   * \brief Getting target file path.
   * \return Path of archive target file.
   */
  char const* getTarget(void);

 private:
  /*!
   * \brief Path of archive source file.
   */
  char sourcePath[PATH_MAX + 1];
};

#endif  // _ARCHIVE_MAKER_H

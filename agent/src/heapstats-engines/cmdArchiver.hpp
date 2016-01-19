/*!
 * \file cmdArchiver.hpp
 * \brief This file is used create archive file by command line.
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

#ifndef _CMD_ARCHIVER_H
#define _CMD_ARCHIVER_H

#include "archiveMaker.hpp"
#include "cmdArchiver.hpp"

/*!
 * \brief This class create archive file.
 */
class TCmdArchiver : public TArchiveMaker {
 public:
  /*!
   * \brief TCmdArchiver constructor.
   */
  TCmdArchiver(void);
  /*!
   * \brief TCmdArchiver destructor.
   */
  ~TCmdArchiver(void);

  /*!
   * \brief Do file archive and create archive file.
   * \param env         [in] JNI environment object.
   * \param archiveFile [in] archive file name.
   * \return Process result code of create archive.
   */
  int doArchive(JNIEnv *env, char const *archiveFile);

 protected:
  /*!
   * \brief Convert string separated by space to string array.
   * \param str [in] Separated string by space.
   * \return Arrayed string.<br>Don't forget deallocate.<br>
   *         Value is null, if process failure or param is illegal.
   */
  char **strToArray(char *str);
  /*!
   * \brief Execute command line.
   * \param cmdline [in] String of execute command.
   * \return Response code of execute commad line.
   */
  virtual int execute(char *cmdline);
};

#endif  // _CMD_ARCHIVER_H

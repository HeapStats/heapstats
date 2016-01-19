/*!
 * \file archiveMaker.cpp
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

#include <string.h>

#include "archiveMaker.hpp"

/*!
 * \brief TArchiveMaker constructor.
 */
TArchiveMaker::TArchiveMaker(void) {
  /* Clear target path. */
  memset(sourcePath, 0, sizeof(sourcePath));
}

/*!
 * \brief TArchiveMaker destructor.
 */
TArchiveMaker::~TArchiveMaker(void) { /* Do Nothing. */ }

/*!
 * \brief Setting target file path.
 * \param targetPath [in] Path of archive target file.
 */
void TArchiveMaker::setTarget(char const* targetPath) {
  strncpy(sourcePath, targetPath, PATH_MAX);
}

/*!
 * \brief Clear archive file data.
 */
void TArchiveMaker::clear(void) { memset(sourcePath, 0, sizeof(sourcePath)); }

/*!
 * \brief Getting target file path.
 * \return Path of archive target file.
 */
char const* TArchiveMaker::getTarget(void) { return sourcePath; }

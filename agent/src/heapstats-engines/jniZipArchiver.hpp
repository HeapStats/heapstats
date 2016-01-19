/*!
 * \file jniZipArchiver.hpp
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

#ifndef _JNI_ZIP_ARCHIVER_H
#define _JNI_ZIP_ARCHIVER_H

#include <jvmti.h>
#include <jni.h>

#include "archiveMaker.hpp"

/*!
 * \brief This class create archive file.
 */
class TJniZipArchiver : public TArchiveMaker {
 public:
  /*!
   * \brief TJniZipArchiver constructor.
   */
  TJniZipArchiver(void);
  /*!
   * \brief TJniZipArchiver destructor.
   */
  ~TJniZipArchiver(void);

  /*!
   * \brief Do file archive and create archive file.
   * \param env         [in] JNI environment object.
   * \param archiveFile [in] archive file name.
   * \return Response code of execute commad line.
   */
  int doArchive(JNIEnv* env, char const* archiveFile);

  /*!
   * \brief Global initialization.
   * \param env [in] JNI environment object.
   * \return Process is succeed, if value is true.<br>
   *         Process is failure, if value is false.
   * \warning Please call only once from main thread.
   */
  static bool globalInitialize(JNIEnv* env);

  /*!
   * \brief Global finalize.
   * \param env [in] JNI environment object.
   * \return Process is succeed, if value is true.<br>
   *         Process is failure, if value is false.
   * \warning Please call only once from main thread.
   */
  static bool globalFinalize(JNIEnv* env);

 protected:
  /*!
   * \brief Execute archive.
   * \param env         [in] JNI environment object.
   * \param archiveFile [in] archive file name.
   * \return Response code of execute archive.
   */
  virtual int execute(JNIEnv* env, char const* archiveFile);

  /*!
   * \brief Write files to archive.
   * \param env        [in] JNI environment object.
   * \param jZipStream [in] JNI zip archive stream object.
   * \return Response code of execute archive.
   */
  virtual int writeFiles(JNIEnv* env, jobject jZipStream);

 private:
  /*!
   * \brief Refference of "BufferedOutputStream" class data.
   */
  static jclass clsBuffOutStrm;

  /*!
   * \brief Refference of "BufferedOutputStream" class constructer.
   */
  static jmethodID clsBuffOutStrm_init;

  /*!
   * \brief Refference of "FileOutputStream" class data.
   */
  static jclass clsFileOutStrm;

  /*!
   * \brief Refference of "FileOutputStream" class constructer.
   */
  static jmethodID clsFileOutStrm_init;

  /*!
   * \brief Refference of "ZipOutputStream" class data.
   */
  static jclass clsZipOutStrm;

  /*!
   * \brief Refference of "ZipOutputStream" class constructer.
   */
  static jmethodID clsZipOutStrm_init;

  /*!
   * \brief Refference of "ZipEntry" class data.
   */
  static jclass clsZipEntry;

  /*!
   * \brief Refference of "ZipEntry" class constructer.
   */
  static jmethodID clsZipEntry_init;

  /*!
   * \brief Refference of "ZipOutputStream" class method "close".
   */
  static jmethodID clsZipOutStrm_close;

  /*!
   * \brief Refference of "ZipOutputStream" class method "closeEntry".
   */
  static jmethodID clsZipOutStrm_closeEntry;

  /*!
   * \brief Refference of "ZipOutputStream" class method "putNextEntry".
   */
  static jmethodID clsZipOutStrm_putNextEntry;

  /*!
   * \brief Refference of "ZipOutputStream" class method "write".
   */
  static jmethodID clsZipOutStrm_write;

  /*!
   * \brief Refference of "ZipOutputStream" class method "flush".
   * \sa FilterOutputStream::flush
   */
  static jmethodID clsZipOutStrm_flush;

  /*!
   * \brief Flag of load initialize data.
   */
  static bool loadFlag;
};

#endif  // _JNI_ZIP_ARCHIVER_H

/*!
 * \file heapstatsMBean.hpp
 * \brief JNI implementation for HeapStatsMBean.
 * Copyright (C) 2014 Yasumasa Suenaga
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

#ifndef HEAPSTATSMBEAN_HPP
#define HEAPSTATSMBEAN_HPP

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

  JNIEXPORT void JNICALL RegisterHeapStatsNative(JNIEnv *env, jclass cls);
  JNIEXPORT jstring JNICALL GetHeapStatsVersion(JNIEnv *env, jobject obj);
  JNIEXPORT jobject JNICALL
      GetConfiguration(JNIEnv *env, jobject obj, jstring key);
  JNIEXPORT jobject JNICALL GetConfigurationList(JNIEnv *env, jobject obj);
  JNIEXPORT jboolean JNICALL
      ChangeConfiguration(JNIEnv *env, jobject obj, jstring key, jobject value);
  JNIEXPORT jboolean JNICALL InvokeLogCollection(JNIEnv *env, jobject obj);
  JNIEXPORT jboolean JNICALL InvokeAllLogCollection(JNIEnv *env, jobject obj);

#ifdef __cplusplus
}
#endif

#endif  // HEAPSTATSMBEAN_HPP

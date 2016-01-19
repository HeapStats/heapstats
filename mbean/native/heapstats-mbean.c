/*!
 * \file heapstats-mbean.c
 * \brief JNI implementation of jp.co.ntt.oss.heapstats.mbean.HeapStats .
 * Copyright (C) 2015 Yasumasa Suenaga
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

#include <jni.h>

#include "heapstats-mbean.h"

typedef void (*TRegisterHeapStatsNatives)(JNIEnv *env, jclass cls);


/*
 * Class:     jp_co_ntt_oss_heapstats_mbean_HeapStats
 * Method:    registerNatives
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_jp_co_ntt_oss_heapstats_mbean_HeapStats_registerNatives
                                                 (JNIEnv *env, jclass cls){
  TRegisterHeapStatsNatives func = (TRegisterHeapStatsNatives)(*env)->reserved0;

  if(func == NULL){
    jclass ex_cls = (*env)->FindClass(env, "java/lang/UnsatisfiedLinkError");
    (*env)->ThrowNew(env, ex_cls,
                          "Could not get native function for HeapStatsMBean");
  }
  else{
    func(env, cls);
  }

}


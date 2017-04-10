/*!
 * Copyright (C) 2017 Nippon Telegraph and Telephone Corporation
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

#include <pthread.h>
#include <string.h>

#include <jni.h>

#include "run-libjvm.hpp"

pthread_t RunLibJVMTest::java_driver;

void RunLibJVMTest::GetJVM(JavaVM **vm, JNIEnv **env){
  jsize numVMs;

  if(JNI_GetCreatedJavaVMs(vm, 1, &numVMs) != JNI_OK){
    throw "Could not get JavaVMs.";
  }

  if(numVMs < 1){
    throw "Could not get JavaVMs.";
  }

  (*vm)->AttachCurrentThread((void **)env, NULL);
}

void *RunLibJVMTest::java_driver_main(void *data){
  JavaVM *vm;
  JNIEnv *env;
  GetJVM(&vm, &env);

  jclass LongSleepClass = env->FindClass("LongSleep");
  if(LongSleepClass == NULL){
    throw "Could not find LongSleep class.";
  }

  jmethodID mainMethod = env->GetStaticMethodID(LongSleepClass, "main",
                                                      "([Ljava/lang/String;)V");
  if(mainMethod == NULL){
    throw "Could not find LongSleep#main() .";
  }

  env->CallStaticVoidMethod(LongSleepClass, mainMethod, NULL);

  vm->DetachCurrentThread();
  return NULL;
}

void RunLibJVMTest::SetUpTestCase(){
  java_driver = 0;

  if(pthread_create(&java_driver, NULL, &java_driver_main, NULL) != 0){
    throw "Could not create Java driver thread!";
  }

}

void RunLibJVMTest::TearDownTestCase(){
  JavaVM *vm;
  JNIEnv *env;
  GetJVM(&vm, &env);

  if(java_driver != 0){
    jclass LongSleepClass = env->FindClass("LongSleep");
    jfieldID lockObjField = env->GetStaticFieldID(LongSleepClass,
                                                  "lock", "Ljava/lang/Object;");
    if(lockObjField == NULL){
      throw "Could not find LongSleep#lock .";
    }

    jobject lockObj = env->GetStaticObjectField(
                                           LongSleepClass, lockObjField);
    if(lockObj == NULL){
      throw "Could not find lock object.";
    }

    jclass objClass = env->FindClass("java/lang/Object");
    jmethodID notifyMethod = env->GetMethodID(objClass, "notifyAll", "()V");

    env->MonitorEnter(lockObj);
    env->CallVoidMethod(lockObj, notifyMethod);
    env->MonitorExit(lockObj);

    if(env->ExceptionOccurred()){
      env->ExceptionDescribe();
      env->ExceptionClear();
    }

    pthread_join(java_driver, NULL);
  }

  if(vm != NULL){
    vm->DetachCurrentThread();
  }

}

char *RunLibJVMTest::GetSystemProperty(const char *key){
  JavaVM *vm;
  JNIEnv *env;
  GetJVM(&vm, &env);

  jstring key_str = env->NewStringUTF(key);
  jclass sysClass = env->FindClass("Ljava/lang/System;");
  jmethodID getProperty = env->GetStaticMethodID(sysClass,
                       "getProperty", "(Ljava/lang/String;)Ljava/lang/String;");

  jstring value = (jstring)env->CallStaticObjectMethod(sysClass,
                                                  getProperty, key_str);
  const char *ret_utf8 = env->GetStringUTFChars(value, NULL);
  char *ret = strdup(ret_utf8);

  env->ReleaseStringUTFChars(value, ret_utf8);
  vm->DetachCurrentThread();
  return ret;
}


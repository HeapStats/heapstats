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

#include <gtest/gtest.h>
#include <jni.h>

#include <heapstats-engines/globals.hpp>

#include "heapStatsEnvironment.hpp"

#define CLASSPATH "./stub"

void HeapStatsEnvironment::SetUp(){
  logger = new TLogger();

  JavaVMInitArgs args;
  args.version = JNI_VERSION_1_6;
  JNI_GetDefaultJavaVMInitArgs(&args);

  JavaVMOption opt;
  opt.optionString = (char *)"-Djava.class.path=" CLASSPATH;
  args.nOptions = 1;
  args.options = &opt;

  JavaVM *vm;
  JNIEnv *env;

  if(JNI_CreateJavaVM(&vm, (void **)&env, &args) != JNI_OK){
    throw "Could not create JavaVM";
  }

}

void HeapStatsEnvironment::TearDown(){
  JavaVM *vm;
  jsize numVMs;

  if(JNI_GetCreatedJavaVMs(&vm, 1, &numVMs) == JNI_OK){
    vm->DestroyJavaVM();
  }

}


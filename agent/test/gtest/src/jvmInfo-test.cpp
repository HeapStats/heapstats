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

#include <jvmti.h>
#include <gtest/gtest.h>

#include <heapstats-engines/jvmInfo.hpp>

#include "run-libjvm.hpp"

class JvmInfoTest : public RunLibJVMTest{
  private:
    static jvmtiEnv *jvmti;

  protected:
    static TJvmInfo info;
    static void SetUpTestCase();
};

jvmtiEnv *JvmInfoTest::jvmti;
TJvmInfo JvmInfoTest::info;

void JvmInfoTest::SetUpTestCase(){
  RunLibJVMTest::SetUpTestCase();

  JavaVM *vm;
  JNIEnv *env;
  GetJVM(&vm, &env);
  vm->GetEnv((void **)&jvmti, JVMTI_VERSION_1);

  info.setHSVersion(jvmti);
  info.detectInfoAddress(env);
  info.detectDelayInfoAddress();

  vm->DetachCurrentThread();
}

TEST_F(JvmInfoTest, GetMaxMemory){
  JavaVM *vm;
  JNIEnv *env;
  GetJVM(&vm, &env);

  ASSERT_NE(info.getMaxMemory(), -1);

  vm->DetachCurrentThread();
}

TEST_F(JvmInfoTest, GetTotalMemory){
  JavaVM *vm;
  JNIEnv *env;
  GetJVM(&vm, &env);

  ASSERT_NE(info.getTotalMemory(), -1);

  vm->DetachCurrentThread();
}

TEST_F(JvmInfoTest, GetNewAreaSize){
  ASSERT_NE(info.getNewAreaSize(), -1);
}

TEST_F(JvmInfoTest, GetOldAreaSize){
  ASSERT_NE(info.getOldAreaSize(), -1);
}

TEST_F(JvmInfoTest, GetMetaspaceUsage){
  ASSERT_NE(info.getMetaspaceUsage(), -1);
}

TEST_F(JvmInfoTest, GetMetaspaceCapacity){
  ASSERT_NE(info.getMetaspaceCapacity(), -1);
}

TEST_F(JvmInfoTest, GetFGCCount){
  ASSERT_NE(info.getFGCCount(), -1);
}

TEST_F(JvmInfoTest, GetYGCCount){
  ASSERT_NE(info.getYGCCount(), -1);
}

TEST_F(JvmInfoTest, GetGCCause){
  ASSERT_STRNE(info.getGCCause(), NULL);
}

TEST_F(JvmInfoTest, GetGCWorktime){
  info.getGCWorktime();
}

TEST_F(JvmInfoTest, GetSyncPark){
  ASSERT_NE(info.getSyncPark(), -1);
}

TEST_F(JvmInfoTest, GetThreadLive){
  ASSERT_NE(info.getThreadLive(), -1);
}

TEST_F(JvmInfoTest, GetSafepointTime){
  ASSERT_NE(info.getSafepointTime(), -1);
}

TEST_F(JvmInfoTest, GetSafepoints){
  ASSERT_NE(info.getSafepoints(), -1);
}

TEST_F(JvmInfoTest, GetVmVersion){
  ASSERT_STREQ(info.getVmVersion(), GetSystemProperty("java.vm.version"));
}

TEST_F(JvmInfoTest, GetVmName){
  ASSERT_STREQ(info.getVmName(), GetSystemProperty("java.vm.name"));
}

TEST_F(JvmInfoTest, GetClassPath){
  ASSERT_STREQ(info.getClassPath(), GetSystemProperty("java.class.path"));
}

TEST_F(JvmInfoTest, GetEndorsedPath){
  ASSERT_STREQ(info.getEndorsedPath(), GetSystemProperty("java.endorsed.dirs"));
}

TEST_F(JvmInfoTest, GetJavaVersion){
  ASSERT_STREQ(info.getJavaVersion(), GetSystemProperty("java.version"));
}

TEST_F(JvmInfoTest, GetBootClassPath){
  ASSERT_STREQ(info.getBootClassPath(),
               GetSystemProperty("sun.boot.class.path"));
}

TEST_F(JvmInfoTest, GetVmArgs){
  ASSERT_STRNE(info.getVmArgs(), NULL);
}

TEST_F(JvmInfoTest, GetVmFlags){
  ASSERT_STRNE(info.getVmFlags(), NULL);
}

TEST_F(JvmInfoTest, GetJavaCommand){
  ASSERT_STRNE(info.getJavaCommand(), NULL);
}

TEST_F(JvmInfoTest, GetTickTime){
  ASSERT_NE(info.getTickTime(), -1);
}

TEST_F(JvmInfoTest, LoadGCCause){
  info.loadGCCause();
}


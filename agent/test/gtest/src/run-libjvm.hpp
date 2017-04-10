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
#include <jni.h>

#include <gtest/gtest.h>

class RunLibJVMTest : public testing::Test{
  private:
    static pthread_t java_driver;

  protected:
    static void GetJVM(JavaVM **vm, JNIEnv **env);
    static void SetUpTestCase();
    static void TearDownTestCase();

  public:
    static void *java_driver_main(void *data);
    char *GetSystemProperty(const char *key);
};


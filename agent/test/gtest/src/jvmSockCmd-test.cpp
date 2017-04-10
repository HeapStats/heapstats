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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <heapstats-engines/jvmSockCmd.hpp>

#include "run-libjvm.hpp"

#define RESULT_FILE "results/jvmsockcmd-results.txt"

class JVMSockCmdTest : public RunLibJVMTest{};

TEST_F(JVMSockCmdTest, runAttachListener){
  char *tmpdir = GetSystemProperty("java.io.tmpdir");
  TJVMSockCmd jvmCmd(tmpdir);
  free(tmpdir);

  ASSERT_EQ(jvmCmd.exec("properties", RESULT_FILE), 0);

  struct stat buf;
  int ret = stat(RESULT_FILE, &buf);
  ASSERT_EQ(ret, 0);
  ASSERT_TRUE(S_ISREG(buf.st_mode));
}


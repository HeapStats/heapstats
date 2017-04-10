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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <openssl/md5.h>
#include <errno.h>
#include <stdlib.h>

#include <heapstats-engines/globals.hpp>
#include <heapstats-engines/fsUtil.hpp>


#define RESULT_DIR "results"
#define COPY_SRC "heapstats-test"


class FSUtilTest : public testing::Test{

  protected:

    bool GetMD5(const char *fname, unsigned char *md){
      int fd = open(fname, O_RDONLY);
      if(fd == -1){
        return false;
      }

      struct stat st;
      fstat(fd, &st);
      void *data = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);

      MD5_CTX c;

      MD5_Init(&c);
      MD5_Update(&c, data, st.st_size);
      MD5_Final(md, &c);

      munmap(data, st.st_size);
      close(fd);

      return true;
    }

};


TEST_F(FSUtilTest, copyFile){
  ASSERT_EQ(copyFile(COPY_SRC, RESULT_DIR), 0);

  unsigned char src_md5[MD5_DIGEST_LENGTH];
  unsigned char dst_md5[MD5_DIGEST_LENGTH];
  GetMD5(COPY_SRC, src_md5);
  ASSERT_TRUE(GetMD5(RESULT_DIR "/" COPY_SRC, dst_md5));

  ASSERT_EQ(0, memcmp(src_md5, dst_md5, MD5_DIGEST_LENGTH));
}

TEST_F(FSUtilTest, copyFileWithRename){
  ASSERT_EQ(0, copyFile(COPY_SRC, RESULT_DIR, "renamed_copy"));

  unsigned char src_md5[MD5_DIGEST_LENGTH];
  unsigned char dst_md5[MD5_DIGEST_LENGTH];
  GetMD5(COPY_SRC, src_md5);
  ASSERT_TRUE(GetMD5(RESULT_DIR "/renamed_copy", dst_md5));

  ASSERT_EQ(0, memcmp(src_md5, dst_md5, MD5_DIGEST_LENGTH));
}

TEST_F(FSUtilTest, tempDir){
  char *tmp = NULL;
  ASSERT_EQ(0, createTempDir(&tmp, "heapstats-test-tmp"));
  ASSERT_FALSE(tmp == NULL);

  struct stat st;
  ASSERT_EQ(0, stat(tmp, &st));

  ASSERT_TRUE(S_ISDIR(st.st_mode));

  removeTempDir(tmp);
  errno = 0;
  stat(tmp, &st);
  ASSERT_EQ(errno, ENOENT);

  free(tmp);
}

TEST_F(FSUtilTest, getParentDirectoryPath){
  char *result;

  result = getParentDirectoryPath("test");
  ASSERT_STREQ("./", result);
  free(result);

  result = getParentDirectoryPath("/test");
  ASSERT_STREQ("/", result);
  free(result);

  result = getParentDirectoryPath("./test");
  ASSERT_STREQ(".", result);
  free(result);

  result = getParentDirectoryPath("./path/to/file");
  ASSERT_STREQ("./path/to", result);
  free(result);

  result = getParentDirectoryPath("/path/to/file");
  ASSERT_STREQ("/path/to", result);
  free(result);

  result = getParentDirectoryPath("path/to/file");
  ASSERT_STREQ("path/to", result);
  free(result);
}

TEST_F(FSUtilTest, isAccessibleDirectory){
  const char *dirname = RESULT_DIR "/access_test";
  mkdir(dirname, S_IRUSR | S_IWUSR);

  ASSERT_EQ(0, isAccessibleDirectory(dirname, true, true));
  ASSERT_EQ(0, isAccessibleDirectory(dirname, true, false));
  ASSERT_EQ(0, isAccessibleDirectory(dirname, false, true));

  chmod(dirname, S_IRUSR);
  ASSERT_NE(0, isAccessibleDirectory(dirname, true, true));
  ASSERT_EQ(0, isAccessibleDirectory(dirname, true, false));
  ASSERT_NE(0, isAccessibleDirectory(dirname, false, true));

  chmod(dirname, S_IWUSR);
  ASSERT_NE(0, isAccessibleDirectory(dirname, true, true));
  ASSERT_NE(0, isAccessibleDirectory(dirname, true, false));
  ASSERT_EQ(0, isAccessibleDirectory(dirname, false, true));

  rmdir(dirname);
}

TEST_F(FSUtilTest, isValidPath){
  ASSERT_TRUE(isValidPath("./"));
  ASSERT_TRUE(isValidPath(COPY_SRC));
  ASSERT_FALSE(isValidPath("./does/not/exist"));
}

TEST_F(FSUtilTest, checkDiskFull){
  ASSERT_TRUE(checkDiskFull(ENOSPC, "testcase"));
  ASSERT_FALSE(checkDiskFull(0, "testcase"));
}


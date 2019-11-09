/*!
 * \file fsUtil.cpp
 * \brief This file is utilities to access file system.
 * Copyright (C) 2011-2019 Nippon Telegraph and Telephone Corporation
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

#include <fcntl.h>
#include <sys/sendfile.h>
#include <dirent.h>

#include "globals.hpp"
#include "fsUtil.hpp"

/*!
 * \brief Mutex of working directory.
 */
pthread_mutex_t directoryMutex = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;

/*!
 * \brief Copy data as avoid overwriting.
 * \param sourceFile [in] Path of source file.
 * \param destPath   [in] Path of directory put duplicated file.
 * \param destName   [in] Name of duplicated file.<br>
 *                        Don't rename, if value is null.
 * \return Value is zero, if process is succeed.<br />
 *         Value is error number a.k.a. "errno", if process is failure.
 */
int copyFile(char const *sourceFile, char const *destPath,
             char const *destName) {
  char rpath[PATH_MAX];

  if (unlikely(!isCopiablePath(sourceFile, rpath))) {
    return EINVAL;
  }

  int result = 0;
  /* Make file name. */
  char *newFile = destName == NULL
                      ? createFilename(destPath, (char *)sourceFile)
                      : createFilename(destPath, (char *)destName);

  /* Failure make filename. */
  if (unlikely(newFile == NULL)) {
    logger->printWarnMsg("Couldn't allocate copy source file path.");

    /*
     * Because exist below two pattern when "createFilename" return NULL.
     * 1, Illegal argument. "errno" don't change.
     * 2, No usable memory. "errno" maybe "ENOMEM".
     */
    return (errno != 0) ? errno : EINVAL;
  }

  /* Get uniq file name. */
  char *destFile = createUniquePath(newFile, false);
  if (unlikely(destFile == NULL)) {
    logger->printWarnMsg("Couldn't allocate unique destination file name.");
    free(newFile);
    return ENOMEM;
  }

  /* Open copy source file. */
  int sourceFd = open(sourceFile, O_RDONLY);
  if (unlikely(sourceFd < 0)) {
    result = errno;
    logger->printWarnMsgWithErrno("Couldn't open copy source file.");
    free(newFile);
    free(destFile);
    return result;
  }

  /* Get source file size */
  struct stat st;
  if (unlikely(fstat(sourceFd, &st) != 0)) {
    result = errno;
    logger->printWarnMsgWithErrno("Couldn't open copy destination file.");
    free(newFile);
    free(destFile);
    close(sourceFd);
    return result;
  }

  /* Open destination file. */
  int destFd = open(destFile, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);
  if (unlikely(destFd < 0)) {
    result = errno;
    logger->printWarnMsgWithErrno("Couldn't open copy destination file.");
    free(newFile);
    free(destFile);
    close(sourceFd);
    return result;
  }

  /* Copy data */
  if (st.st_size > 0) {
    if (unlikely(sendfile64(destFd, sourceFd, NULL, st.st_size) == -1)) {
      result = errno;
      logger->printWarnMsgWithErrno("Couldn't copy file.");
    }
  } else { /* This route is for files in procfs */
    char buf[1024];
    ssize_t read_size;

    while ((read_size = read(sourceFd, buf, 1024)) > 0) {
      if (write(destFd, buf, (size_t)read_size) == -1) {
        read_size = -1;
        break;
      }
    }

    if (read_size == -1) {
      result = errno;
      logger->printWarnMsgWithErrno("Couldn't copy file.");
    }
  }

  /* Clean up */
  close(sourceFd);

  if (unlikely((close(destFd) != 0) && (result == 0))) {
    result = errno;
    logger->printWarnMsgWithErrno("Couldn't write copy file data.");
  }

  free(newFile);
  free(destFile);

  return result;
}

/*!
 * \brief Create filename in expected path.
 * \param basePath [in] Path of directory.
 * \param filename [in] filename.
 * \return Create string.<br>
 *         Process is failure, if return is null.<br>
 *         Need call "free", if return is not null.
 */
char *createFilename(char const *basePath, char const *filename) {
  /* Path separator. */
  const char PATH_SEP = '/';
  /* Sanity check. */
  if (unlikely(basePath == NULL || filename == NULL || strlen(basePath) == 0 ||
               strlen(filename) == 0)) {
    return NULL;
  }

  /* Find file name head. so exclude path. */
  char *headPos = strrchr((char *)filename, PATH_SEP);
  /* If not found separator. */
  if (headPos == NULL) {
    /* Copy all. */
    headPos = (char *)filename;
  } else {
    /* Skip separator. */
    headPos++;
  }

  /* Check separator existing. */
  bool needSeparator = (basePath[strlen(basePath) - 1] != PATH_SEP);
  size_t strSize = strlen(basePath) + strlen(headPos) + 1;
  if (needSeparator) {
    strSize += 1;
  }

  char *newPath = (char *)malloc(strSize);
  /* If failure allocate result string. */
  if (unlikely(newPath == NULL)) {
    return NULL;
  }

  /* Add base directory path. */
  strcpy(newPath, basePath);
  /* Insert separator. */
  if (needSeparator) {
    strcat(newPath, "/");
  }
  /* Add file name. */
  strcat(newPath, headPos);

  /* Return generate file path. */
  return newPath;
}

/*!
 * \brief Resolve canonicalized path path and check regular file or not.
 * \param path [in]  Path of the target file.
 * \param rpath [out] Real path which is pointed at "path".
 *        Buffer size must be enough to store path (we recommend PATH_MAX).
 * \return If "path" is copiable, this function returns true.
 */
bool isCopiablePath(const char *path, char *rpath) {
  realpath(path, rpath);

  struct stat st;
  if (unlikely(stat(rpath, &st) != 0)) {
    /* Failure get file information. */
    logger->printWarnMsgWithErrno("Failure get file information (stat).");
    return false;
  }

  if ((st.st_mode & S_IFMT) != S_IFREG) {
    /* File isn't regular file. This route is not error. */
    logger->printDebugMsg("Couldn't copy file. Not a regular file: %s", path);
    return false;
  }

  return true;
}

/*!
 * \brief Create temporary directory in designated directory.
 * \param basePath   [out] Path of temporary directory.<br>
 *                         Process is failure, if return is null.<br>
 *                         Need call "free", if return is not null.
 * \param wishesName [in]  Name of one's wishes directory name.
 * \return Value is zero, if process is succeed.<br />
 *         Value is error number a.k.a. "errno", if process is failure.
 */
int createTempDir(char **basePath, char const *wishesName) {
  char *uniqName = NULL;

  /* Sanity check. */
  if (unlikely(basePath == NULL || wishesName == NULL)) {
    logger->printWarnMsg("Illegal directory path.");
    return -1;
  }

  int raisedErrNum = -1;
  {
    TMutexLocker locker(&directoryMutex);

    /* Create unique directory path. */
    uniqName = createUniquePath((char *)wishesName, true);
    if (unlikely(uniqName == NULL)) {
      raisedErrNum = errno;
    } else {
      /* If failed to create temporary directory. */
      if (unlikely(mkdir(uniqName, S_IRUSR | S_IWUSR | S_IXUSR) != 0)) {
        raisedErrNum = errno;
      } else {
        raisedErrNum = 0;
      }
    }
  }

  /* If failed to create temporary directory. */
  if (unlikely(raisedErrNum != 0)) {
    free(uniqName);
    uniqName = NULL;
  }

  /* Copy directory path. */
  (*basePath) = uniqName;
  return raisedErrNum;
}

/*!
 * \brief Remove temporary directory.
 * \param basePath [in] Path of temporary directory.
 */
void removeTempDir(char const *basePath) {
  /* If failure open directory. */
  if (unlikely(basePath == NULL)) {
    logger->printWarnMsg("Illegal directory path.");
    return;
  }

  /* Open directory. */
  DIR *dir = opendir(basePath);

  /* If failure open directory. */
  if (unlikely(dir == NULL)) {
    logger->printWarnMsgWithErrno("Couldn't open directory.");
    return;
  }

  /* Store error number. */
  int oldErrNum = errno;
  /* Read file in directory. */
  struct dirent *entry = NULL;
  while ((entry = readdir(dir)) != NULL) {
    /* Check file name. */
    if (strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0) {
      continue;
    }

    /* Get remove file's full path. */
    char *removePath = createFilename(basePath, entry->d_name);

    /* All file need remove. */
    if (unlikely(removePath == NULL || unlink(removePath) != 0)) {
      /* Skip file remove. */
      if (removePath == NULL) {
        errno = ENOMEM;  // This error may be caused by ENOMEM because malloc()
                         // failed.
      }
      char *printPath = (removePath != NULL) ? removePath : entry->d_name;
      logger->printWarnMsgWithErrno("Failure remove file. path: \"%s\"",
                                    printPath);
    }

    /* Cleanup. */
    free(removePath);
    oldErrNum = errno;
  }

  /* If failure in read directory. */
  if (unlikely(errno != oldErrNum)) {
    logger->printWarnMsgWithErrno("Failure search file in directory.");
  }

  /* Cleanup. */
  closedir(dir);

  {
    TMutexLocker locker(&directoryMutex);

    /* Remove directory. */
    if (unlikely(rmdir(basePath) != 0)) {
      /* Skip directory remove. */
      logger->printWarnMsgWithErrno("Failure remove directory.");
    }

  }
}

/*!
 * \brief Create unique path.
 * \param path        [in] Path.
 * \param isDirectory [in] Path is directory.
 * \return Unique path.<br>Don't forget deallocate.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
char *createUniquePath(char *path, bool isDirectory) {
  /* Variable for temporary path. */
  char tempPath[PATH_MAX] = {0};
  /* Variables for file system. */
  FILE *file = NULL;
  DIR *dir = NULL;
  /* Variable for file path and extensition. */
  char ext[PATH_MAX] = {0};
  char tempName[PATH_MAX] = {0};

  /* Sanity check. */
  if (unlikely(path == NULL || strlen(path) == 0)) {
    logger->printWarnMsg("Illegal unique path paramters.");
    return NULL;
  }

  /* Search extension. */
  char *extPos = strrchr(path, '.');

  /* If create path for file and exists extension. */
  if (!isDirectory && extPos != NULL) {
    int pathSize = (extPos - path);
    pathSize = (pathSize > PATH_MAX) ? PATH_MAX : pathSize;

    /* Path and extension store each other. */
    strncpy(ext, extPos, PATH_MAX);
    strncpy(tempName, path, pathSize);
  } else {
    /* Ignore extension if exists. */
    strncpy(tempName, path, PATH_MAX);
  }

  /* Copy default path. */
  strncpy(tempPath, path, PATH_MAX);

  /* Try make unique path loop. */
  const unsigned long int MAX_RETRY_COUNT = 1000000;
  unsigned long int loopCount = 0;
  for (; loopCount <= MAX_RETRY_COUNT; loopCount++) {
    /* Usable path flag. */
    bool noExist = false;

    /* Reset error number. */
    errno = 0;

    /* Open and check exists. */
    if (isDirectory) {
      /* Check directory. */
      dir = opendir(tempPath);
      noExist = (dir == NULL && errno == ENOENT);

      if (dir != NULL) {
        closedir(dir);
      }
    } else {
      /* Check file. */
      file = fopen(tempPath, "r");
      noExist = (file == NULL && errno == ENOENT);

      if (file != NULL) {
        fclose(file);
      }
    }

    /* If path is usable. */
    if (noExist) {
      break;
    }

    /* Make new path insert sequence number between 000000 and 999999. */
    int ret = snprintf(tempPath, PATH_MAX, "%s_%06lu%s",
                       tempName, loopCount, ext);
    if (ret >= PATH_MAX) {
      logger->printCritMsg("Temp path is too long: %s", tempPath);
      return NULL;
    }
  }

  /* If give up to try unique naming by number. */
  if (unlikely(loopCount > MAX_RETRY_COUNT)) {
    /* Get random value. */
    long int rand = random();
    const int randSize = 6;
    char randStr[randSize + 1] = {0};

    for (int i = 0; i < randSize; i++) {
      randStr[i] = (rand % 26) + 'A';
      rand /= 26;
    }

    /* Uniquely naming by random string. */
    int ret = snprintf(tempPath, PATH_MAX, "%s_%6s%s", tempName, randStr, ext);
    if (ret >= PATH_MAX) {
      logger->printCritMsg("Temp path is too long: %s", tempPath);
      return NULL;
    }
    logger->printWarnMsg("Not found unique name. So used random string.");
  }

  /* Copy path. */
  char *result = strdup(tempPath);
  if (unlikely(result == NULL)) {
    logger->printWarnMsg("Couldn't copy unique path.");
  }

  return result;
}
#pragma GCC diagnostic pop

/*!
 * \brief Get parent directory path.
 * \param path [in] Path which file or directory.
 * \return Parent directory path.<br>Don't forget deallocate.
 */
char *getParentDirectoryPath(char const *path) {
  char *sepPos = NULL;
  char *result = NULL;

  /* Search last separator. */
  sepPos = strrchr((char *)path, '/');
  if (sepPos == NULL) {
    /* Path is current directory. */
    result = strdup("./");

  } else if (unlikely(sepPos == path)) {
    /* Path is root. */
    result = strdup("/");

  } else {
    /* Copy path exclude last entry. */
    result = (char *)calloc(1, sepPos - path + 1);
    if (likely(result != NULL)) {
      strncpy(result, path, sepPos - path);
    }
  }

  /* If failure allocate path. */
  if (unlikely(result == NULL)) {
    logger->printWarnMsg("Couldn't copy parent directory path.");
  }

  return result;
}

/*!
 * \brief Get accessible of directory.
 * \param path      [in] Directory path.
 * \param needRead  [in] Accessible flag about file read.
 * \param needWrite [in] Accessible flag about file write.
 * \return Value is zero, if process is succeed.<br />
 *         Value is error number a.k.a. "errno", if process is failure.
 */
int isAccessibleDirectory(char const *path, bool needRead, bool needWrite) {
  struct stat st = {0};

  /* Sanity check. */
  if (unlikely(path == NULL || strlen(path) == 0 ||
               (!needRead && !needWrite))) {
    logger->printWarnMsg("Illegal accessible paramter.");
    return -1;
  }

  /* If failure get directory information. */
  if (unlikely(stat(path, &st) != 0)) {
    int raisedErrNum = errno;
    logger->printWarnMsgWithErrno("Failure get directory information.");
    return raisedErrNum;
  }

  /* if path isn't directory. */
  if (unlikely(!S_ISDIR(st.st_mode))) {
    logger->printWarnMsg("Illegal directory path.");
    return ENOTDIR;
  }

  /* if execute by administrator. */
  if (unlikely(geteuid() == 0)) {
    return 0;
  }

  bool result = false;
  bool isDirOwner = (st.st_uid == geteuid());
  bool isGroupUser = (st.st_gid == getegid());

  /* Check access permition as file owner. */
  if (isDirOwner) {
    result |= (!needRead || (st.st_mode & S_IRUSR) > 0) &&
              (!needWrite || (st.st_mode & S_IWUSR) > 0);
  }

  /* Check access permition as group user. */
  if (!result && isGroupUser) {
    result |= (!needRead || (st.st_mode & S_IRGRP) > 0) &&
              (!needWrite || (st.st_mode & S_IWGRP) > 0);
  }

  /* Check access permition as other. */
  if (!result) {
    result |= (!needRead || (st.st_mode & S_IROTH) > 0) &&
              (!needWrite || (st.st_mode & S_IWOTH) > 0);
  }

  return (result) ? 0 : EACCES;
}

/*!
 * \brief Check that path is accessible.
 * \param path [in] A path to file.
 * \return Path is accessible.
 */
bool isValidPath(const char *path) {
  /* Check archive file path. */
  if (strlen(path) == 0) {
    throw "Invalid file path";
  }
  char *dir = getParentDirectoryPath(path);
  if (dir == NULL) {
    throw "Cannot get parent directory";
  }

  errno = 0;
  int ret = isAccessibleDirectory(dir, true, true);
  free(dir);
  if (ret == -1) {
    throw "Illegal parameter was passed to isAccessibleDirectory()";
  } else if (errno != 0) {
    return false;
  }

  struct stat st;
  if (stat(path, &st) == -1) {
    if (errno == ENOENT) {
      return true;
    } else {
      throw errno;
    }
  }

  bool result = false;
  bool isOwner = (st.st_uid == geteuid());
  bool isGroupUser = (st.st_gid == getegid());

  /* Check access permition as file owner. */
  if (isOwner) {
    result |= (st.st_mode & S_IRUSR) && (st.st_mode & S_IWUSR);
  }

  /* Check access permition as group user. */
  if (!result && isGroupUser) {
    result |= (st.st_mode & S_IRGRP) && (st.st_mode & S_IWGRP);
  }

  /* Check access permition as other. */
  if (!result) {
    result |= (st.st_mode & S_IROTH) && (st.st_mode & S_IWOTH);
  }

  if (!result) {
    errno = EPERM;
  }

  return result;
}

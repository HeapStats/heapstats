/*!
 * \file cmdArchiver.cpp
 * \brief This file is used create archive file by command line.
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

#include <string.h>
#include <sys/wait.h>

#include "globals.hpp"
#include "util.hpp"
#include "cmdArchiver.hpp"

/*!
 * \brief TCmdArchiver constructor.
 */
TCmdArchiver::TCmdArchiver(void) : TArchiveMaker() { /* Do Nothing. */ }

/*!
 * \brief TCmdArchiver destructor.
 */
TCmdArchiver::~TCmdArchiver(void) { /* Do Nothing. */ }

/*!
 * \brief Do file archive and create archive file.
 * \param env         [in] JNI environment object.
 * \param archiveFile [in] archive file name.
 * \return Process result code of create archive.
 */
int TCmdArchiver::doArchive(JNIEnv *env, char const *archiveFile) {
  /* If Source file is empty. */
  if (unlikely(strlen(this->getTarget()) == 0 || archiveFile == NULL ||
               strlen(archiveFile) == 0 ||
               conf->ArchiveCommand()->get() == NULL)) {
    logger->printWarnMsg("Illegal archive paramter.");
    clear();
    return -1;
  }

  /* Copy command line. */
  char *cmdline = strdup(conf->ArchiveCommand()->get());
  if (unlikely(cmdline == NULL)) {
    /* Failure copy string. */
    logger->printWarnMsg("Couldn't allocate command line memory.");
    clear();
    return -1;
  }

  /* Setting replace rule. */
  const int ruleCount = 2;
  const char *sourceRule[ruleCount] = {"%logdir%", "%archivefile%"};
  char *destRule[ruleCount] = {NULL, NULL};

  destRule[0] = (char *)this->getTarget();
  destRule[1] = (char *)archiveFile;

  /* Replace paramters. */
  for (int i = 0; i < ruleCount; i++) {
    /* Replace with rule. */
    char *afterStr = NULL;
    afterStr = strReplase(cmdline, (char *)sourceRule[i], destRule[i]);
    if (unlikely(afterStr == NULL)) {
      /* Failure replace string. */
      logger->printWarnMsg("Failure parse command line.");
      free(cmdline);
      clear();
      return -1;
    }

    /* Cleanup and swap. */
    free(cmdline);
    cmdline = afterStr;
  }

  /* Exec command. */
  int result = execute(cmdline);

  /* Check archive file. */
  struct stat st = {0};
  if (unlikely(stat(archiveFile, &st) != 0)) {
    /* If process is succeed but archive isn't exists. */
    if (unlikely(result == 0)) {
      /* Accept as failure. */
      result = -1;
    }
  } else {
    /* If process is failed but archive is still existing. */
    if (unlikely(result != 0)) {
      /* Remove file. Because it's maybe broken. */
      unlink(archiveFile);
    }
  }

  /* Cleanup. */
  free(cmdline);
  clear();

  /* Succeed. */
  return result;
}

/*!
 * \brief Execute command line.
 * \param cmdline [in] String of execute command.
 * \return Response code of execute commad line.
 */
int TCmdArchiver::execute(char *cmdline) {
  /* Variable for result code. */
  int result = 0;
  /* Enviroment variable for execve. */
  char *envParam[] = {NULL};

  /* Convert string to array. */
  char **argArray = strToArray(cmdline);
  if (unlikely(argArray == NULL)) {
    /* Failure string to array string. */
    logger->printWarnMsg("Couldn't allocate command line memory.");
    return -1;
  }

  /* Fork process without copy. */
  pid_t forkPid = vfork();

  switch (forkPid) {
    case -1:
      /* Error vfork. */

      result = errno;
      logger->printWarnMsgWithErrno("Could not fork child process.");
      break;

    case 0:
      /* Child process. */

      /* Exec commad line. */
      if (unlikely(execve(argArray[0], argArray, envParam) < 0)) {
        /* Failure execute command line. */
        _exit(errno);
      }

      /* Stopper for illegal route. */
      _exit(-99);

    default:
      /* Parent Process. */

      /* Wait child process. */
      int st = 0;
      if (unlikely(waitpid(forkPid, &st, 0) < 0)) {
        result = errno;
        logger->printWarnMsgWithErrno("Could not wait command process.");
      }

      /* If process is failure. */
      if (unlikely(!WIFEXITED(st) || WEXITSTATUS(st) != 0)) {
        /* Failure execve, catch signal, etc.. */

        if (WIFEXITED(st)) {
          result = WEXITSTATUS(st);
        } else {
          result = -1;
        }
        logger->printWarnMsg("Failure execute archive command.");
      }
  }

  /* Cleanup. */
  free(argArray[0]);
  free(argArray);

  return result;
}

/*!
 * \brief Convert string separated by space to string array.
 * \param str [in] Separated string by space.
 * \return Arrayed string.<br>Don't forget deallocate.<br>
 *         Value is null, if process failure or param is illegal.
 */
char **TCmdArchiver::strToArray(char *str) {
  /* String separator. */
  const char SEPARATOR_CHAR = ' ';

  /* Sanity check. */
  if (unlikely(str == NULL || strlen(str) == 0)) {
    return NULL;
  }

  /* Count arguments. */
  size_t argCount = 0;
  size_t newStrSize = 0;
  size_t oldStrLen = strlen(str);
  for (size_t i = 0; i < oldStrLen;) {
    size_t strSize = 0;

    /* Skip string. */
    while (i < oldStrLen && str[i] != SEPARATOR_CHAR) {
      i++;
      strSize++;
    }

    /* If exists string, then count separate string. */
    if (strSize > 0) {
      argCount++;
      newStrSize += strSize + 1;
    }

    /* Skip a string separator sequence. */
    while (i < oldStrLen && str[i] == SEPARATOR_CHAR) {
      i++;
    }
  }

  /* If string is separator char only. */
  if (unlikely(argCount == 0)) {
    return NULL;
  }

  /* Allocate string. */
  char *tempStr = (char *)malloc(newStrSize + 1);
  if (unlikely(tempStr == NULL)) {
    return NULL;
  }

  /* Allocate string array. */
  char **argArray = (char **)malloc(sizeof(char *) * (argCount + 1));
  if (unlikely(argArray == NULL)) {
    free(tempStr);
    return NULL;
  }

  /* Parse string line to array. */
  size_t argIdx = 0;
  for (size_t i = 0; i < oldStrLen;) {
    size_t oldIdx = i;
    size_t strSize = 0;

    /* Skip string. */
    while (i < oldStrLen && str[i] != SEPARATOR_CHAR) {
      i++;
      strSize++;
    }

    /* If exists string, then separate string. */
    if (strSize > 0) {
      /* Copy string. */
      strncpy(tempStr, &str[oldIdx], strSize);
      tempStr[strSize] = '\0';
      argArray[argIdx] = tempStr;

      /* Move next. */
      tempStr += strSize + 1;
      argIdx++;
    }

    /* Skip and replace a string separator sequence. */
    while (i < oldStrLen && str[i] == SEPARATOR_CHAR) {
      i++;
    }
  }
  /* Add end flag. */
  argArray[argCount] = NULL;

  return argArray;
}

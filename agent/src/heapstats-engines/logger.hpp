/*!
 * \file logger.hpp
 * \brief Logger class to stdout/err .
 * Copyright (C) 2014-2015 Yasumasa Suenaga
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

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "util.hpp"

/*!
 * \brief Logging Level.
 */
typedef enum {
  CRIT = 1,
  /*!< Level for  FATAL   error log. This's displayed by all means. */
  WARN = 2,
  /*!< Level for runnable error log. e.g. Heap-Alert                */
  INFO = 3,
  /*!< Level for  normal  info  log. e.g. Heap-Ranking              */
  DEBUG = 4
  /*!< Level for  debug  info  log.                                 */
} TLogLevel;

class TLogger {
 private:
  FILE *out;
  FILE *err;
  TLogLevel logLevel;

  /*!
   * \brief Print message to stream.
   * \param stream: Output stream.
   * \param header: Message header. This value should be loglevel.
   * \param isNewLine: If this value sets to true, this function will add
   *                   newline character at end of message.
   * \param format: String format to print.
   * \param ap: va_list for "format".
   */
  inline void printMessageInternal(FILE *stream, const char *header,
                                   bool isNewLine, const char *format,
                                   va_list ap) {
    fprintf(stream, "heapstats %s: ", header);
    vfprintf(stream, format, ap);
    if (isNewLine) {
      fputc('\n', stream);
    }
  }

 public:
  TLogger() : logLevel(INFO) {
    out = stdout;
    err = stderr;
  }
  TLogger(TLogLevel level) : logLevel(level) {
    out = stdout;
    err = stderr;
  }

  virtual ~TLogger() {
    flush();
    if (out != stdout && out != NULL) {
      fclose(out);
    }
  }

  /*!
   * \brief Flush stdout and stderr.
   */
  inline void flush() {
    fflush(out);
    if (err != out) {
      fflush(err);
    }
  }

  /*!
   * \brief Setter of logLevel.
   * \param level: Log level.
   */
  inline void setLogLevel(TLogLevel level) { this->logLevel = level; }

  /*!
   * \brief Setter for logFile.
   * \param logfile: Console log filename.
   */
  inline void setLogFile(char *logfile) {
    if (logfile != NULL && strlen(logfile) != 0) {
      out = fopen(logfile, "a");
      if (unlikely(out == NULL)) {
        // If cannot open file.
        this->printWarnMsgWithErrno(
            "Could not open console log file: %s. Agent always output to "
            "console.",
            logfile);
        out = stdout;
      } else {
        err = out;
      }
    }
  }

  /*!
   * \brief Print critical error message.
   *        This function treats arguments as printf(3).
   */
  inline void printCritMsg(const char *format, ...) {
    if (this->logLevel >= CRIT) {
      va_list ap;
      va_start(ap, format);
      this->printMessageInternal(err, "CRIT", true, format, ap);
      va_end(ap);
    }
  }

  /*!
   * \brief Print warning error message.
   *        This function treats arguments as printf(3).
   */
  inline void printWarnMsg(const char *format, ...) {
    if (this->logLevel >= WARN) {
      va_list ap;
      va_start(ap, format);
      this->printMessageInternal(err, "WARN", true, format, ap);
      va_end(ap);
    }
  }

  /*!
   * \brief Print warning error message with current errno.
   *        This function treats arguments as printf(3).
   */
  inline void printWarnMsgWithErrno(const char *format, ...) {
    if (this->logLevel >= WARN) {
      char error_string[1024];
      char *output_message = strerror_wrapper(error_string, 1024);

      va_list ap;
      va_start(ap, format);
      this->printMessageInternal(err, "WARN", false, format, ap);
      va_end(ap);
      fprintf(err, " cause: %s\n", output_message);
    }
  }

  /*!
   * \brief Print information message.
   *        This function treats arguments as printf(3).
   */
  inline void printInfoMsg(const char *format, ...) {
    if (this->logLevel >= INFO) {
      va_list ap;
      va_start(ap, format);
      this->printMessageInternal(out, "INFO", true, format, ap);
      va_end(ap);
    }
  }

  /*!
   * \brief Print debug message.
   *        This function treats arguments as printf(3).
   */
  inline void printDebugMsg(const char *format, ...) {
    if (this->logLevel >= DEBUG) {
      va_list ap;
      va_start(ap, format);
      this->printMessageInternal(out, "DEBUG", true, format, ap);
      va_end(ap);
    }
  }
};

#endif  // LOGGER_HPP

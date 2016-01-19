/*!
 * \file elapsedTimer.hpp
 * \brief This file is used to measure elapsed time.
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

#ifndef ELAPSED_TIMER_H
#define ELAPSED_TIMER_H

#include <sys/times.h>

/*!
 * \brief This class measure elapsed time by processed working.
 */
class TElapsedTimer {
 public:
  /*!
   * \brief TElapsedTimer's clock frequency.<br>
   *        Please set this value from external.<br>
   *        e.g. "sysconf(_SC_CLK_TCK)"
   */
  static long clock_ticks;

  /*!
   * \brief TElapsedTimer constructor.
   * \param label [in] Working process name.
   */
  TElapsedTimer(const char *label) {
    /* Stored process start time. */
    this->_start_clock = times(&this->_start_tms);
    this->_label = label;
  }

  /*!
   * \brief TElapsedTimer destructor.
   */
  ~TElapsedTimer() {
    /* Get process end time. */
    struct tms _end_tms;
    clock_t _end_clock = times(&_end_tms);

    if (this->_label != NULL) {
      logger->printInfoMsg(
          "Elapsed Time (in %s): %f sec (user = %f, sys = %f)", this->_label,
          this->calcClockToSec(this->_start_clock, _end_clock),
          this->calcClockToSec(this->_start_tms.tms_utime, _end_tms.tms_utime),
          this->calcClockToSec(this->_start_tms.tms_stime, _end_tms.tms_stime));
    } else {
      logger->printInfoMsg(
          "Elapsed Time: %f sec (user = %f, sys = %f)",
          this->calcClockToSec(this->_start_clock, _end_clock),
          this->calcClockToSec(this->_start_tms.tms_utime, _end_tms.tms_utime),
          this->calcClockToSec(this->_start_tms.tms_stime, _end_tms.tms_stime));
    }
  }

 private:
  /*!
   * \brief Working process name.
   */
  const char *_label;
  /*!
   * \brief Process start simple time.
   */
  clock_t _start_clock;
  /*!
   * \brief Process start detail time.
   */
  struct tms _start_tms;

  /*!
   * \brief Calculate time between start and end.
   * \param start [in] Time of process begin.
   * \param end   [in] Time of process finished.
   * \return Elapsed time.
   */
  inline double calcClockToSec(const clock_t start, const clock_t end) {
    return (double)(end - start) / (double)clock_ticks;
  }
};

#endif

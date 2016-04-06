/*!
 * \file signalManager.hpp
 * \brief This file is used by signal handling.
 * Copyright (C) 2011-2016 Nippon Telegraph and Telephone Corporation
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

#ifndef _SIGNAL_MNGR_H
#define _SIGNAL_MNGR_H

#include <jni.h>
#include <signal.h>

#include "util.hpp"

/*!
 * \brief This type is signal callback by TSignalManager.
 * \param signo   [in] Number of received signal.
 * \param siginfo [in] Informantion of received signal.<br>
 *                     But this value is always null.
 * \param data    [in] Data of received signal.<br>
 *                     But this value is always null.
 * \warning Function must be signal-safe.
 */
typedef void (*TSignalHandler)(int signo, siginfo_t *siginfo, void *data);

typedef struct structSignalHandlerChain {
  TSignalHandler handler;
  structSignalHandlerChain *next;
} TSignalHandlerChain;

typedef struct {
  int sig;
  const char *name;
} TSignalMap;


/*!
 * \brief This class is handling signal.
 */
class TSignalManager {

  private:
    /*!
     * \brief Signal number for this instance.
     */
    int signal;

    /*!
     * \brief SR_Handler in HotSpot VM
     */
    static TSignalHandler SR_Handler;

  public:
    /*!
     * \brief Find signal number from name.
     * \param sig Signal name.
     * \return Signal number. Return -1 if not found.
     */
    static int findSignal(const char *sig);

    /*!
     * \brief TSignalManager constructor.
     * \param sig Signal string.
     */
    TSignalManager(const char *sig);

    /*!
     * \brief TSignalManager destructor.
     */
    virtual ~TSignalManager(void);

    /*!
     * \brief Add signal handler.
     * \param handler [in] Function pointer for signal handler.
     * \return true if new signal handler is added.
     */
    bool addHandler(TSignalHandler handler);
};

#endif  // _SIGNAL_MNGR_H

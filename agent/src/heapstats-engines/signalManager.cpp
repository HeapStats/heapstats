/*!
 * \file signalManager.cpp
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

#include <signal.h>

#include "globals.hpp"
#include "vmFunctions.hpp"
#include "signalManager.hpp"


static TSignalHandlerChain *sighandlers[NSIG] = {0};

static TSignalMap signalMap[] = 
  {
    {SIGHUP,   "SIGHUP"},  {SIGALRM,   "SIGALRM"},  {SIGUSR1,   "SIGUSR1"},
    {SIGUSR2,  "SIGUSR2"}, {SIGTSTP,   "SIGTSTP"},  {SIGTTIN,   "SIGTTIN"},
    {SIGTTOU,  "SIGTTOU"}, {SIGPOLL,   "SIGPOLL"},  {SIGVTALRM, "SIGVTALRM"},
    {SIGIOT,   "SIGIOT"},  {SIGWINCH,  "SIGWINCH"}
  };


void SignalHandlerStub(int signo, siginfo_t *siginfo, void *data) {
  TSignalHandlerChain *chain = sighandlers[signo];
  TVMFunctions *vmFunc = TVMFunctions::getInstance();

  while (chain != NULL) {
    if ((void *)chain->handler == vmFunc->GetSRHandlerPointer()) {
      /* SR handler should be called in HotSpot Thread context. */
      if (vmFunc->GetThread() != NULL) {
        chain->handler(signo, siginfo, data);
      }
    } else if (((void *)chain->handler == vmFunc->GetUserHandlerPointer()) &&
               (signo == SIGHUP)) {
      // This might SHUTDOWN1_SIGNAL handler which is defined in
      // java.lang.Terminator . (Unix class)
      // We should ignore this signal.

      // Do nothing.
    } else if ((void *)chain->handler != SIG_IGN) {
      chain->handler(signo, siginfo, data);
    }

    chain = chain->next;
  }
}


/* Class methods. */

/*!
 * \brief Find signal number from name.
 * \param sig Signal name.
 * \return Signal number. Return -1 if not found.
 */
int TSignalManager::findSignal(const char *sig) {
  for (size_t idx = 0; idx < (sizeof(signalMap) / sizeof(TSignalMap)); idx++) {
    if (strcmp(signalMap[idx].name, sig) == 0){
      return signalMap[idx].sig;
    }
  }

  return -1;
}

/*!
 * \brief TSignalManager constructor.
 * \param sig Signal string.
 */
TSignalManager::TSignalManager(const char *sig) {
  this->signal = findSignal(sig);

  if (this->signal == -1) {
    static char errstr[1024];
    snprintf(errstr, 1024, "Invalid signal: %s", sig);
    throw errstr;
  }
}

/*!
 * \brief TSignalManager destructor.
 */
TSignalManager::~TSignalManager() {
  TSignalHandlerChain *chain = sighandlers[this->signal];
  JVM_RegisterSignal(this->signal, (void *)chain->handler);

  while (chain != NULL) {
    TSignalHandlerChain *next = chain->next;
    free(chain);
    chain = next;
  }
}

/*!
 * \brief Add signal handler.
 * \param handler [in] Function pointer for signal handler.
 * \return true if new signal handler is added.
 */
bool TSignalManager::addHandler(TSignalHandler handler) {
  TSignalHandlerChain *chain = sighandlers[this->signal];

  if (chain == NULL) {
    TSignalHandler oldHandler = (TSignalHandler)JVM_RegisterSignal(
                                     this->signal, (void *)&SignalHandlerStub);

    if ((ptrdiff_t)oldHandler == -1L) { // Cannot set
      return false;
    } else if ((ptrdiff_t)oldHandler == 1L) { // Ignore signal
      oldHandler = (TSignalHandler)SIG_IGN;
    } else if ((ptrdiff_t)oldHandler == 2L) { // User Handler in HotSpot
      oldHandler =
           (TSignalHandler)TVMFunctions::getInstance()->GetUserHandlerPointer();
    }

    chain = (TSignalHandlerChain *)calloc(1, sizeof(TSignalHandlerChain));

    if (chain == NULL) {
      throw errno;
    }

    chain->handler = oldHandler;
    sighandlers[this->signal] = chain;
  }

  while (chain->next != NULL) {
    chain = chain->next;
  }

  chain->next = (TSignalHandlerChain *)calloc(1, sizeof(TSignalHandlerChain));

  if (chain->next == NULL) {
    throw errno;
  }

  chain->next->handler = handler;
  return true;
}


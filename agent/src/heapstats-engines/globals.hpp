/*!
 * \file globals.hpp
 * \brief Definitions of global variables.
 * Copyright (C) 2015-2017 Yasumasa Suenaga
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

#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <pthread.h>

#include "jvmInfo.hpp"
extern TJvmInfo *jvmInfo;

#include "configuration.hpp"
extern TConfiguration *conf;

#include "logger.hpp"
extern TLogger *logger;

#include "signalManager.hpp"
extern TSignalManager *reloadSigMngr;
extern TSignalManager *logSignalMngr;
extern TSignalManager *logAllSignalMngr;

#include "timer.hpp"
extern TTimer *intervalSigTimer;
extern TTimer *logTimer;
extern TTimer *timer;

#include "symbolFinder.hpp"
extern TSymbolFinder *symFinder;

#include "vmStructScanner.hpp"
extern TVMStructScanner *vmScanner;

#include "logManager.hpp"
extern TLogManager *logManager;

#include "classContainer.hpp"
extern TClassContainer *clsContainer;

#include "snapShotProcessor.hpp"
extern TSnapShotProcessor *snapShotProcessor;

#include "gcWatcher.hpp"
extern TGCWatcher *gcWatcher;

/*!
 * \brief Mutex of working directory.
 */
extern pthread_mutex_t directoryMutex;

/*!
 * \brief Page size got from sysconf.
 */
extern long systemPageSize;

#endif  // GLOBALS_HPP

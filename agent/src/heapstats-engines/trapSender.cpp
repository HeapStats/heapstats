/*!
 * \file trapSender.cpp
 * \brief This file is used to send SNMP trap.
 * Copyright (C) 2016 Yasumasa Suenaga
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

#include "trapSender.hpp"


/*!
 * \brief Datetime of agent initialization.
 */
unsigned long int TTrapSender::initializeTime;

/*!
 * \brief Mutex for TTrapSender.<br>
 * <br>
 * This mutex used in below process.<br>
 *   - TTrapSender::TTrapSender @ trapSender.hpp<br>
 *     To initialize SNMP trap section.<br>
 *   - TTrapSender::~TTrapSender @ trapSender.hpp<br>
 *     To finalize SNMP trap section.<br>
 *   - TTrapSender::sendTrap @ trapSender.hpp<br>
 *     To change SNMP trap section and send SNMP trap.<br>
 */
pthread_mutex_t TTrapSender::senderMutex =
                                PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;


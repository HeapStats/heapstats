/*!
 * \file trapSender.hpp
 * \brief This file is used to send SNMP trap.
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

#ifndef _TRAP_SENDER_H
#define _TRAP_SENDER_H

#include <set>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <pthread.h>


/* Process return code. */

/*!
 * \brief Failure SNMP process.
 */
#define SNMP_PROC_FAILURE -1
/*!
 * \brief SNMP process is succeed.
 */
#define SNMP_PROC_SUCCESS 0

/* SNMP variable types. */

/*!
 * \brief 32bit signed Integer.
 */
#define SNMP_VAR_TYPE_INTEGER 'i'
/*!
 * \brief 32bit unsigned Integer.
 */
#define SNMP_VAR_TYPE_UNSIGNED 'u'
/*!
 * \brief 32bit Counter.
 */
#define SNMP_VAR_TYPE_COUNTER32 'c'
/*!
 * \brief 64bit Counter.
 */
#define SNMP_VAR_TYPE_COUNTER64 'C'
/*!
 * \brief String.
 */
#define SNMP_VAR_TYPE_STRING 's'
/*!
 * \brief Null object.
 */
#define SNMP_VAR_TYPE_NULL 'n'
/*!
 * \brief OID.
 */
#define SNMP_VAR_TYPE_OID 'o'
/*!
 * \brief Time-tick.
 */
#define SNMP_VAR_TYPE_TIMETICK 't'

/* SNMP OID. */

/*!
 * \brief OID of SysUpTimes.
 */
#define SNMP_OID_SYSUPTIME 1, 3, 6, 1, 2, 1, 1, 3
/*!
 * \brief OID of SnmpTrapOid.
 */
#define SNMP_OID_TRAPOID 1, 3, 6, 1, 6, 3, 1, 1, 4, 1
/*!
 * \brief OID of Enterpirce.
 */
#define SNMP_OID_ENTERPRICE 1, 3, 6, 1, 4, 1

/*!
 * \brief OID of heapstats's PEN.
 */
#define SNMP_OID_PEN SNMP_OID_ENTERPRICE, 45156

/*!
 * \brief OID of heap alert trap.
 */
#define SNMP_OID_HEAPALERT SNMP_OID_PEN, 1
/*!
 * \brief OID of resource exhasted alert trap.
 */
#define SNMP_OID_RESALERT SNMP_OID_PEN, 2
/*!
 * \brief OID of log archive trap.
 */
#define SNMP_OID_LOGARCHIVE SNMP_OID_PEN, 3
/*!
 * \brief OID of deadlock alert trap.
 */
#define SNMP_OID_DEADLOCKALERT SNMP_OID_PEN, 4
/*!
 * \brief OID of java heap usage alert trap.
 */
#define SNMP_OID_JAVAHEAPALERT SNMP_OID_PEN, 5
/*!
 * \brief OID of metaspace usage alert trap.
 */
#define SNMP_OID_METASPACEALERT SNMP_OID_PEN, 6

/*!
 * \brief OID string of enterprice.
 */
#define OID_PEN "1.3.6.1.4.1.45156"

/*!
 * \brief OID string of heap alert trap.
 */
#define OID_HEAPALERT OID_PEN ".1.0"
/*!
 * \brief OID string of resource exhasted alert trap.
 */
#define OID_RESALERT OID_PEN ".2.0"
/*!
 * \brief OID string of log archive trap.
 */
#define OID_LOGARCHIVE OID_PEN ".3.0"
/*!
 * \brief OID string of deadlock alert trap.
 */
#define OID_DEADLOCKALERT OID_PEN ".4.0"
/*!
 * \brief OID string of java heap usage alert trap.
 */
#define OID_JAVAHEAPALERT OID_PEN ".5.0"
/*!
 * \brief OID string of java heap usage alert trap.
 */
#define OID_METASPACEALERT OID_PEN ".6.0"

/*!
 * \brief This class is send trap to SNMP Manager.
 */
class TTrapSender {
 public:
  /*!
   * \brief Datetime of agent initialization.
   */
  static unsigned long int initializeTime;

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
  static pthread_mutex_t senderMutex;

  /*!
   * \brief TrapSender constructor.
   * \param snmp      [in] SNMP version.
   * \param pPeer     [in] Target of SNMP trap.
   * \param pCommName [in] Community name use for SNMP.
   * \param port      [in] Port used by SNMP trap.
   */
  TTrapSender(int snmp, char *pPeer, char *pCommName, int port);

  /*!
   * \brief TrapSender destructor.
   */
  ~TTrapSender(void);

  /*!
   * \brief Add agent running time from initialize to trap.
   */
  void setSysUpTime(void);

  /*!
   * \brief Add trapOID to send information by trap.
   * \param trapOID[] [in] Identifier of trap.
   */
  void setTrapOID(const char *trapOID);

  /*!
   * \brief Add variable as send information by trap.
   * \param id[]   [in] Identifier of variable.
   * \param len    [in] Length of param "id".
   * \param pValue [in] Variable's data.
   * \param type   [in] Kind of a variable.
   * \return Return process result code.
   */
  int addValue(oid id[], int len, const char *pValue, char type);

  /*!
   * \brief Add variable as send information by trap.
   * \return Return process result code.
   */
  int sendTrap(void);

  /*!
   * \brief Clear PDU and allocated strings.
   */
  void clearValues(void);

  /*!
   * \brief Get SNMP session information.
   * \return Session information.
   */
  netsnmp_session getSession(void) { return this->session; }

  /*!
   * \brief Get SNMP trap variable count.
   * \return Trap variable count.
   */
  int getInfoCount(void) { return this->strSet.size(); }

  /*!
   * \brief Get SNMP PDU information.
   * \return Pdu object.
   */
  netsnmp_pdu *getPdu(void) { return this->pPdu; }

 private:
  /*!
   * \brief SNMP session information.
   */
  netsnmp_session session;
  /*!
   * \brief SNMP PDU information.
   */
  netsnmp_pdu *pPdu;

  /*!
   * \brief Allocated string set for PDU.
   */
  std::set<char *> strSet;
};

#endif  //_TRAP_SENDER_H

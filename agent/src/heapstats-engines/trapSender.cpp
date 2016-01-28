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

#include <set>
#include <sys/time.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <pthread.h>
#include <dlfcn.h>

#include "globals.hpp"
#include "util.hpp"
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

/*!
 * \brief Flags whether libnetsnmp.so is loaded.
 */
bool TTrapSender::is_netsnmp_loaded = false;

/*!
 * \brief Library handle of libnetsnmp.so .
 */
void * TTrapSender::libnetsnmp_handle = NULL;

/*!
 * \brief Functions in NET-SNMP client library.
 */
TNetSNMPFunctions TTrapSender::netSnmpFuncs;


/*!
 * \brief TTrapSender initialization.
 * \return true if succeeded.
 */
bool TTrapSender::initialize(void) {
  /* Get now date and set time as agent init-time. */
  struct timeval tv;
  gettimeofday(&tv, NULL);
  TTrapSender::initializeTime =
      (jlong)tv.tv_sec * 100 + (jlong)tv.tv_usec / 10000;

  /* Load functions from libnetsnmp */
  if (!getProcAddressFromNetSNMPLib()) {
    return false;
  }

  return true;
}

/*!
 * \brief TTrapSender global finalization.
 */
void TTrapSender::finalize(void) {
  /* Unload library */
  if (libnetsnmp_handle != NULL) {
    dlclose(libnetsnmp_handle);
  }
}

/*!
 * \brief TrapSender constructor.
 * \param snmp      [in] SNMP version.
 * \param pPeer     [in] Target of SNMP trap.
 * \param pCommName [in] Community name use for SNMP.
 * \param port      [in] Port used by SNMP trap.
 */
TTrapSender::TTrapSender(int snmp, char *pPeer, char *pCommName, int port) {
  /* Lock to use in multi-thread. */
  ENTER_PTHREAD_SECTION(&senderMutex) {
    /* Disable NETSNMP logging. */
    netSnmpFuncs.netsnmp_register_loghandler(
                               NETSNMP_LOGHANDLER_NONE, LOG_EMERG);

    /* If snmp target is illegal. */
    if (pPeer == NULL) {
      logger->printWarnMsg("Illegal SNMP target.");
      pPdu = NULL;
    } else {
      /* Initialize session. */
      memset(&session, 0, sizeof(netsnmp_session));
      netSnmpFuncs.snmp_sess_init(&session);
      session.version = snmp;
      session.peername = strdup(pPeer);
      session.remote_port = port;
      session.community = (u_char *)strdup(pCommName);
      session.community_len = (pCommName != NULL) ? strlen(pCommName) : 0;

      /* Make a PDU */
      pPdu = netSnmpFuncs.snmp_pdu_create(SNMP_MSG_TRAP2);
    }
  }
  /* Unlock to use in multi-thread. */
  EXIT_PTHREAD_SECTION(&senderMutex)
}

/*!
 * \brief TrapSender destructor.
 */
TTrapSender::~TTrapSender(void) {
  /* Lock to use in multi-thread. */
  ENTER_PTHREAD_SECTION(&senderMutex) {
    /* Clear Allocated str. */
    clearValues();

    /* Free SNMP pdu. */
    if (pPdu != NULL) {
      netSnmpFuncs.snmp_free_pdu(pPdu);
    }
    /* Close and free SNMP session. */
    netSnmpFuncs.snmp_close(&session);
  }
  /* Unlock to use in multi-thread. */
  EXIT_PTHREAD_SECTION(&senderMutex)
}

/*!
 * \brief Add agent running time from initialize to trap.
 */
void TTrapSender::setSysUpTime(void) {
  /* OID and buffer. */
  oid OID_SYSUPTIME[] = {SNMP_OID_SYSUPTIME, 0};
  char buff[255] = {0};

  /* Get agent uptime. */
  unsigned long int agentUpTime = 0;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  agentUpTime = ((jlong)tv.tv_sec * 100 + (jlong)tv.tv_usec / 10000) -
                TTrapSender::initializeTime;

  /* Uptime to String. */
  sprintf(buff, "%lu", agentUpTime);

  /* Setting sysUpTime. */
  if (addValue(OID_SYSUPTIME, OID_LENGTH(OID_SYSUPTIME), buff,
               SNMP_VAR_TYPE_TIMETICK) == SNMP_PROC_FAILURE) {
    logger->printWarnMsg("Couldn't append SysUpTime.");
  }
}

/*!
 * \brief Add trapOID to send information by trap.
 * \param trapOID[] [in] Identifier of trap.
 */
void TTrapSender::setTrapOID(const char *trapOID) {
  /* Setting trapOID. */
  oid OID_TRAPOID[] = {SNMP_OID_TRAPOID, 0};

  /* Add snmpTrapOID. */
  if (addValue(OID_TRAPOID, OID_LENGTH(OID_TRAPOID), trapOID,
               SNMP_VAR_TYPE_OID) == SNMP_PROC_FAILURE) {
    logger->printWarnMsg("Couldn't append TrapOID.");
  }
}

/*!
 * \brief Add variable as send information by trap.
 * \param id[]   [in] Identifier of variable.
 * \param len    [in] Length of param "id".
 * \param pValue [in] Variable's data.
 * \param type   [in] Kind of a variable.
 * \return Return process result code.
 */
int TTrapSender::addValue(oid id[], int len, const char *pValue, char type) {
  /* Check param. */
  if (id == NULL || len <= 0 || pValue == NULL || type < 'A' || type > 'z' ||
      (type > 'Z' && type < 'a') || pPdu == NULL) {
    logger->printWarnMsg("Illegal SNMP trap parameter!");
    return SNMP_PROC_FAILURE;
  }

  /* Allocate string memory. */
  char *pStr = strdup(pValue);

  /* If failure allocate value string. */
  if (unlikely(pStr == NULL)) {
    logger->printWarnMsg("Couldn't allocate variable string memory!");
    return SNMP_PROC_FAILURE;
  }

  /* Append variable. */
  int error = netSnmpFuncs.snmp_add_var(pPdu, id, len, type, pStr);

  /* Failure append variables. */
  if (error) {
    free(pStr);
    logger->printWarnMsg("Couldn't append variable list!");
    return SNMP_PROC_FAILURE;
  }

  /* Insert allocated string set. */
  strSet.insert(pStr);

  /* Return success code. */
  return SNMP_PROC_SUCCESS;
}

/*!
 * \brief Add variable as send information by trap.
 * \return Return process result code.
 */
int TTrapSender::sendTrap(void) {
  /* If snmp target is illegal. */
  if (pPdu == NULL) {
    logger->printWarnMsg("Illegal SNMP target.");
    return SNMP_PROC_FAILURE;
  }

  /* If failure lock to use in multi-thread. */
  if (unlikely(pthread_mutex_lock(&senderMutex) != 0)) {
    logger->printWarnMsg("Entering mutex failed!");
    return SNMP_PROC_FAILURE;
  }

  SOCK_STARTUP;

  /* Open session. */
#ifdef HAVE_NETSNMP_TRANSPORT_OPEN_CLIENT
  netsnmp_session *sess = netSnmpFuncs.snmp_add(
        &session, netSnmpFuncs.netsnmp_transport_open_client(
                                        "snmptrap", session.peername),
                                                                  NULL, NULL);
#else
  char target[256];
  snprintf(target, sizeof(target), "%s:%d", session.peername,
             session.remote_port);
  netsnmp_session *sess = netSnmpFuncs.snmp_add(
        &session, netSnmpFuncs.netsnmp_tdomain_transport(target, 0, "udp"),
                                                                  NULL, NULL);
#endif

  /* If failure open session. */
  if (sess == NULL) {
    logger->printWarnMsg("Failure open SNMP trap session.");
    SOCK_CLEANUP;

    /* Unlock to use in multi-thread. */
    pthread_mutex_unlock(&senderMutex);

    return SNMP_PROC_FAILURE;
  }

  /* Send trap. */
  int success = netSnmpFuncs.snmp_send(sess, pPdu);

  /* Clean up after send trap. */
  netSnmpFuncs.snmp_close(sess);

  /* If failure send trap. */
  if (!success) {
    /* Free PDU. */
    netSnmpFuncs.snmp_free_pdu(pPdu);
    logger->printWarnMsg("Send SNMP trap failed!");
  }

  /* Clean up. */
  SOCK_CLEANUP;

  /* Unlock to use in multi-thread. */
  pthread_mutex_unlock(&senderMutex);

  clearValues();

  return (success) ? SNMP_PROC_SUCCESS : SNMP_PROC_FAILURE;
}

/*!
 * \brief Clear PDU and allocated strings.
 */
void TTrapSender::clearValues(void) {
  /* If snmp target is illegal. */
  if (pPdu == NULL) {
    return;
  }

  /* Free allocated strings. */
  std::set<char *>::iterator it = strSet.begin();
  while (it != strSet.end()) {
    free((*it));
    ++it;
  }
  strSet.clear();

  /* Make a PDU. */
  pPdu = netSnmpFuncs.snmp_pdu_create(SNMP_MSG_TRAP2);
}

/*!
 * \brief Get function address from libnetsnmp.
 * \return true if succeeded.
 */
bool TTrapSender::getProcAddressFromNetSNMPLib(void) {
  libnetsnmp_handle = dlopen(conf->SnmpLibPath()->get(), RTLD_NOW);

  if (libnetsnmp_handle == NULL) {
    logger->printCritMsg("Could not load libnetsnmp: %s", dlerror());
    return false;
  }

  netSnmpFuncs.netsnmp_register_loghandler =
                   (Tnetsnmp_register_loghandler)dlsym(
                             libnetsnmp_handle, "netsnmp_register_loghandler");
  netSnmpFuncs.snmp_sess_init = (Tsnmp_sess_init)dlsym(
                                           libnetsnmp_handle, "snmp_sess_init");
  netSnmpFuncs.snmp_pdu_create = (Tsnmp_pdu_create)dlsym(
                                          libnetsnmp_handle, "snmp_pdu_create");
  netSnmpFuncs.snmp_free_pdu = (Tsnmp_free_pdu)dlsym(
                                            libnetsnmp_handle, "snmp_free_pdu");
  netSnmpFuncs.snmp_close = (Tsnmp_close)dlsym(libnetsnmp_handle, "snmp_close");
  netSnmpFuncs.snmp_add_var = (Tsnmp_add_var)dlsym(
                                             libnetsnmp_handle, "snmp_add_var");
  netSnmpFuncs.snmp_add = (Tsnmp_add)dlsym(libnetsnmp_handle, "snmp_add");
  netSnmpFuncs.netsnmp_transport_open_client =
                    (Tnetsnmp_transport_open_client)dlsym(
                            libnetsnmp_handle, "netsnmp_transport_open_client");
  netSnmpFuncs.netsnmp_tdomain_transport = (Tnetsnmp_tdomain_transport)dlsym(
                                libnetsnmp_handle, "netsnmp_tdomain_transport");
  netSnmpFuncs.snmp_send = (Tsnmp_send)dlsym(libnetsnmp_handle, "snmp_send");

  if ((netSnmpFuncs.netsnmp_register_loghandler == NULL) ||
      (netSnmpFuncs.snmp_sess_init == NULL) ||
      (netSnmpFuncs.snmp_pdu_create == NULL) ||
      (netSnmpFuncs.snmp_free_pdu == NULL) ||
      (netSnmpFuncs.snmp_close == NULL) ||
      (netSnmpFuncs.snmp_add_var == NULL) ||
      (netSnmpFuncs.snmp_add == NULL) ||
      (netSnmpFuncs.netsnmp_transport_open_client == NULL) ||
      (netSnmpFuncs.snmp_send == NULL)) {
    logger->printCritMsg("Could not load function(s) from libnetsnmp");
    return false;
  }

  return true;
}


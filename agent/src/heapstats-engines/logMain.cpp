/*!
 * \file logmain.cpp
 * \brief This file is used common logging process.
 * Copyright (C) 2011-2017 Nippon Telegraph and Telephone Corporation
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
#include <sched.h>

#ifdef HAVE_ATOMIC
#include <atomic>
#else
#include <cstdatomic>
#endif

#include "globals.hpp"
#include "elapsedTimer.hpp"
#include "util.hpp"
#include "libmain.hpp"
#include "callbackRegister.hpp"
#include "logMain.hpp"

/*!
 * \brief Manager of log information.
 */
TLogManager *logManager = NULL;
/*!
 * \brief Timer for interval collect log.
 */
TTimer *logTimer = NULL;
/*!
 * \brief Signal manager to collect normal log by signal.
 */
TSignalManager *logSignalMngr = NULL;
/*!
 * \brief Signal manager to collect all log by signal.
 */
TSignalManager *logAllSignalMngr = NULL;
/*!
 * \brief Mutex to avoid conflict in OnResourceExhausted.<br>
 * <br>
 * This mutex used in below process.<br>
 *   - OnResourceExhausted @ logMain.cpp<br>
 *     To read and write to flag mean already collect log
 *     on resource exhasuted.<br>
 */
pthread_mutex_t errMutex = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;

/*!
 * \brief Flag of collect normal log for signal.
 */
volatile sig_atomic_t flagLogSignal;
/*!
 * \brief Flag of collect all log for signal.
 */
volatile sig_atomic_t flagAllLogSignal;

/*!
 * \brief processing flag
 */
static std::atomic_int processing(0);


/*!
 * \brief Take log information.
 * \param jvmti   [in] JVMTI environment object.
 * \param env     [in] JNI environment object.
 * \param cause   [in] Cause of taking a snapshot.<br>
 *                     e.g. ResourceExhausted, Signal or Interval.
 * \param nowTime [in] Mili-second elapsed from 1970/1/1 0:00:00.<br>
 *                     This value express time of call log function.
 * \param description [in] Description of the event.
 * \return Value is true, if process is succeed.
 */
inline bool TakeLogInfo(jvmtiEnv *jvmti, JNIEnv *env, TInvokeCause cause,
                        TMSecTime nowTime, const char *description) {
  /* Count working time. */
  static const char *label = "Take LogInfo";
  TElapsedTimer elapsedTime(label);

  /* Collect log. */
  return (logManager->collectLog(jvmti, env, cause, nowTime, description) == 0);
}

/*!
 * \brief Interval collect log.
 * \param jvmti [in] JVMTI environment object.
 * \param env   [in] JNI environment object.
 * \param cause [in] Cause of taking a snapshot.<br>
 *                   This value is always Interval.
 */
void intervalLogProc(jvmtiEnv *jvmti, JNIEnv *env, TInvokeCause cause) {
  TProcessMark mark(processing);

  /* Call collect log by interval. */
  if (unlikely(!TakeLogInfo(jvmti, env, cause,
                            (TMSecTime)getNowTimeSec(), ""))) {
    logger->printWarnMsg("Failure interval collect log.");
  }
}

/*!
 * \brief Handle signal express user wanna collect normal log.
 * \param signo   [in] Number of received signal.
 * \param siginfo [in] Information of received signal.
 * \param data    [in] Data of received signal.
 */
void normalLogProc(int signo, siginfo_t *siginfo, void *data) {
  /* Enable flag. */
  flagLogSignal = 1;
  NOTIFY_CATCH_SIGNAL;
}

/*!
 * \brief Handle signal express user wanna collect all log.
 * \param signo   [in] Number of received signal.
 * \param siginfo [in] Information of received signal.
 * \param data    [in] Data of received signal.
 */
void anotherLogProc(int signo, siginfo_t *siginfo, void *data) {
  /* Enable flag. */
  flagAllLogSignal = 1;
  NOTIFY_CATCH_SIGNAL;
}

/*!
 * \brief Interval watching for log signal.
 * \param jvmti [in] JVMTI environment object.
 * \param env   [in] JNI environment object.
 */
void intervalSigProcForLog(jvmtiEnv *jvmti, JNIEnv *env) {
  TProcessMark mark(processing);

  /* If catch normal log signal. */
  if (unlikely(flagLogSignal != 0)) {
    TMSecTime nowTime = (TMSecTime)getNowTimeSec();
    flagLogSignal = 0;

    if (unlikely(!TakeLogInfo(jvmti, env, Signal, nowTime, ""))) {
      logger->printWarnMsg("Failure collect log by normal log signal.");
    }
  }

  /* If catch all log signal. */
  if (unlikely(flagAllLogSignal != 0)) {
    TMSecTime nowTime = (TMSecTime)getNowTimeSec();
    flagAllLogSignal = 0;

    if (unlikely(!TakeLogInfo(jvmti, env, AnotherSignal, nowTime, ""))) {
      logger->printWarnMsg("Failure collect log by all log signal.");
    }
  }
}

/*!
 * \brief JVM resource exhausted event.
 * \param jvmti       [in] JVMTI environment object.
 * \param env         [in] JNI environment object.
 * \param flags       [in] Resource information bit flag.
 * \param reserved    [in] Reserved variable.
 * \param description [in] Description about resource exhausted.
 */
void JNICALL OnResourceExhausted(jvmtiEnv *jvmti, JNIEnv *env, jint flags,
                                 const void *reserved,
                                 const char *description) {
  TProcessMark mark(processing);

  /* Raise alert. */
  logger->printCritMsg("ALERT(RESOURCE): resource was exhausted. info:\"%s\"",
                       description);

  /* Get now date and time. */
  TMSecTime nowTime = (TMSecTime)getNowTimeSec();

  if (conf->SnmpSend()->get()) {
    /* Trap OID. */
    char trapOID[50] = OID_RESALERT;
    oid OID_RES_FLAG[] = {SNMP_OID_RESALERT, 1};
    oid OID_RES_DESC[] = {SNMP_OID_RESALERT, 2};
    char buff[256] = {0};

    try {
      /* Send resource trap. */
      TTrapSender sender;

      /* Setting sysUpTime */
      sender.setSysUpTime();

      /* Setting trapOID. */
      sender.setTrapOID(trapOID);

      /* Set resource inforomation flags. */
      snprintf(buff, 255, "%d", flags);
      sender.addValue(OID_RES_FLAG, OID_LENGTH(OID_RES_FLAG), buff,
                      SNMP_VAR_TYPE_INTEGER);

      /* Set resource decsription. */
      strncpy(buff, description, 255);
      sender.addValue(OID_RES_DESC, OID_LENGTH(OID_RES_DESC), buff,
                      SNMP_VAR_TYPE_STRING);

      /* Send trap. */
      if (unlikely(sender.sendTrap() != SNMP_PROC_SUCCESS)) {
        /* Ouput message and clean up. */
        sender.clearValues();
        throw 1;
      }
    } catch (...) {
      logger->printWarnMsg("Send SNMP resource exhausted trap failed!");
    }
  }

  bool isCollectLog = true;
  /* Lock to use in multi-thread. */
  ENTER_PTHREAD_SECTION(&errMutex) {

    /* If collected already and collect first only. */
    if (conf->FirstCollect()->get() && conf->isFirstCollected()) {
      /* Skip collect log. */
      logger->printWarnMsg("Skip collect all log on JVM resource exhausted.");
      isCollectLog = false;
    }
    conf->setFirstCollected(true);

  }
  /* Unlock to use in multi-thread. */
  EXIT_PTHREAD_SECTION(&errMutex)

  if (isCollectLog) {
    /* Setting collect log cause. */
    TInvokeCause cause = ResourceExhausted;
    if (unlikely((flags & JVMTI_RESOURCE_EXHAUSTED_THREADS) > 0)) {
      cause = ThreadExhausted;
    }

    /* Collect log. */
    if (unlikely(!TakeLogInfo(jvmti, env, cause, nowTime, description))) {
      logger->printWarnMsg("Failure collect log on resource exhausted.");
    }
  }

  /* If enable to abort JVM by force on resource exhausted. */
  if (unlikely(conf->KillOnError()->get())) {
    forcedAbortJVM(jvmti, env, "resource exhausted");
  }
}

/*!
 * \brief Setting enable of JVMTI and extension events for log function.
 * \param jvmti  [in] JVMTI environment object.
 * \param enable [in] Event notification is enable.
 * \return Setting process result.
 */
jint setEventEnableForLog(jvmtiEnv *jvmti, bool enable) {
  jvmtiEventMode mode = enable ? JVMTI_ENABLE : JVMTI_DISABLE;

  /* If collect log when JVM's resource exhausted. */
  if (conf->TriggerOnLogError()->get()) {
    /* Enable resource exhusted event. */
    TResourceExhaustedCallback::switchEventNotification(jvmti, mode);
  }

  return SUCCESS;
}

/*!
 * \brief Setting enable of agent each threads for log function.
 * \param jvmti  [in] JVMTI environment object.
 * \param env    [in] JNI environment object.
 * \param enable [in] Event notification is enable.
 */
void setThreadEnableForLog(jvmtiEnv *jvmti, JNIEnv *env, bool enable) {
  /* Start or suspend HeapStats agent threads. */
  try {
    /* Switch interval log timer. */
    if (conf->LogInterval()->get() > 0) {
      if (enable) {
        logTimer->start(jvmti, env, conf->LogInterval()->get() * 1000);
      } else {
        logTimer->stop();
      }
    }

    /* Reset signal flag even if non-processed signal is exist. */
    flagLogSignal = 0;
    flagAllLogSignal = 0;
  } catch (const char *errMsg) {
    logger->printWarnMsg(errMsg);
  }
}

/*!
 * \brief JVM initialization event for log function.
 * \param jvmti  [in] JVMTI environment object.
 * \param env    [in] JNI environment object.
 */
void onVMInitForLog(jvmtiEnv *jvmti, JNIEnv *env) {
  /* If failure jni archiver initialization. */
  if (unlikely(!TJniZipArchiver::globalInitialize(env))) {
    logger->printWarnMsg("Failure jni archiver initialization.");
  }

  /* Create instance for log function. */
  try {
    logManager = new TLogManager(env, jvmInfo);
  } catch (const char *errMsg) {
    logger->printWarnMsg(errMsg);
    conf->TriggerOnLogSignal()->set(false);
    conf->TriggerOnLogLock()->set(false);
    conf->TriggerOnLogError()->set(false);
    conf->LogInterval()->set(0);
  } catch (...) {
    logger->printWarnMsg("AgentThread start failed!");
    conf->TriggerOnLogSignal()->set(false);
    conf->TriggerOnLogLock()->set(false);
    conf->TriggerOnLogError()->set(false);
    conf->LogInterval()->set(0);
  }

  /* Initialize signal flag. */
  flagLogSignal = 0;
  flagAllLogSignal = 0;

  /* Signal handler setup */
  if (conf->LogSignalNormal()->get() != NULL) {
    try {
      logSignalMngr = new TSignalManager(conf->LogSignalNormal()->get());
      if (!logSignalMngr->addHandler(&normalLogProc)) {
        logger->printWarnMsg("Log normal signal handler setup is failed.");
        conf->LogSignalNormal()->set(NULL);
      }
    } catch (const char *errMsg) {
      logger->printWarnMsg(errMsg);
      conf->LogSignalNormal()->set(NULL);
    } catch (...) {
      logger->printWarnMsg("Log normal signal handler setup is failed.");
      conf->LogSignalNormal()->set(NULL);
    }
  }

  if (conf->LogSignalAll()->get() != NULL) {
    try {
      logAllSignalMngr = new TSignalManager(conf->LogSignalAll()->get());
      if (!logAllSignalMngr->addHandler(&anotherLogProc)) {
        logger->printWarnMsg("Log all signal handler setup is failed.");
        conf->LogSignalAll()->set(NULL);
      }
    } catch (const char *errMsg) {
      logger->printWarnMsg(errMsg);
      conf->LogSignalAll()->set(NULL);
    } catch (...) {
      logger->printWarnMsg("Log all signal handler setup is failed.");
      conf->LogSignalAll()->set(NULL);
    }
  }
}

/*!
 * \brief JVM finalization event for log function.
 * \param jvmti [in] JVMTI environment object.
 * \param env   [in] JNI environment object.
 */
void onVMDeathForLog(jvmtiEnv *jvmti, JNIEnv *env) {
  if (logSignalMngr != NULL) {
    delete logSignalMngr;
    logSignalMngr = NULL;
  }
  if (logAllSignalMngr != NULL) {
    delete logAllSignalMngr;
    logAllSignalMngr = NULL;
  }

  /*
   * ResourceExhausted, MonitorContendedEnter for Deadlock JVMTI event,
   * all callee of TakeLogInfo() will not be started at this point.
   * So we wait to finish all existed tasks.
   */
  while (processing > 0) {
    sched_yield();
  }
}

/*!
 * \brief Agent initialization for log function.
 * \return Initialize process result.
 */
jint onAgentInitForLog(void) {
  /* Create thread instances that controlled log trigger. */
  try {
    logTimer = new TTimer(&intervalLogProc, "HeapStats Log Timer");
  } catch (const char *errMsg) {
    logger->printCritMsg(errMsg);
    return AGENT_THREAD_INITIALIZE_FAILED;
  } catch (...) {
    logger->printCritMsg("AgentThread initialize failed!");
    return AGENT_THREAD_INITIALIZE_FAILED;
  }

  return SUCCESS;
}

/*!
 * \brief Agent finalization for log function.
 * \param env [in] JNI environment object.
 */
void onAgentFinalForLog(JNIEnv *env) {
  /* Destroy signal manager objects. */
  delete logSignalMngr;
  logSignalMngr = NULL;
  delete logAllSignalMngr;
  logAllSignalMngr = NULL;

  /* Destroy timer object. */
  delete logTimer;
  logTimer = NULL;

  /* Destroy log manager. */
  delete logManager;
  logManager = NULL;

  if (likely(env != NULL)) {
    /* Jni archiver finalization. */
    TJniZipArchiver::globalFinalize(env);
  }
}

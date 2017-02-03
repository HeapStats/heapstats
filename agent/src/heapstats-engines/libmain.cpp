/*!
 * \file libmain.cpp
 * \brief This file is used to common works.<br>
 *        e.g. initialization, finalization, etc...
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

#include <jni.h>
#include <jvmti.h>

#include <unistd.h>
#include <signal.h>

#include "globals.hpp"
#include "elapsedTimer.hpp"
#include "snapShotMain.hpp"
#include "logMain.hpp"
#include "deadlockFinder.hpp"
#include "callbackRegister.hpp"
#include "threadRecorder.hpp"
#include "heapstatsMBean.hpp"
#include "libmain.hpp"

/* Variables. */

/*!
 * \brief JVM running performance information.
 */
TJvmInfo *jvmInfo;
/*!
 * \brief Signal manager to reload configuration by signal.
 */
TSignalManager *reloadSigMngr = NULL;
/*!
 * \brief Reload configuration timer thread.
 */
TTimer *intervalSigTimer;
/*!
 * \brief Logger instance.
 */
TLogger *logger;
/*!
 * \brief HeapStats configuration.
 */
TConfiguration *conf;
/*!
 * \brief Flag of reload configuration for signal.
 */
volatile sig_atomic_t flagReloadConfig;

/*!
 * \brief Running flag.
 */
int flagRunning = 0;

/*!
 * \brief CPU clock ticks.
 */
long TElapsedTimer::clock_ticks = sysconf(_SC_CLK_TCK);

/*!
 * \brief Path of load configuration file at agent initialization.
 */
char *loadConfigPath = NULL;

/* Macros. */

/*!
 * \brief Check memory duplication macro.<br>
 *        If load a library any number of times,<br>
 *        Then the plural agents use single memory area.<br>
 *        E.g. "java -agentlib:A -agentlib:A -jar X.jar"
 */
#define CHECK_DOUBLING_RUN                         \
  if (flagRunning != 0) {                          \
    /* Already running another heapstats agent. */ \
    logger->printWarnMsg(                          \
        "HeapStats agent already run on this JVM." \
        " This agent is disabled.");               \
    return SUCCESS;                                \
  }                                                \
  /* Setting running flag. */                      \
  flagRunning = 1;

/* Functions. */

/*!
 * \brief Handle signal express user wanna reload config.
 * \param signo   [in] Number of received signal.
 * \param siginfo [in] Information of received signal.
 * \param data    [in] Data of received signal.
 */
void ReloadSigProc(int signo, siginfo_t *siginfo, void *data) {
  /* Enable flag. */
  flagReloadConfig = 1;
  NOTIFY_CATCH_SIGNAL;
}

/*!
 * \brief Setting enable of JVMTI and extension events.
 * \param jvmti  [in] JVMTI environment object.
 * \param enable [in] Event notification is enable.
 * \return Setting process result.
 */
jint SetEventEnable(jvmtiEnv *jvmti, bool enable) {
  jint result = SUCCESS;

  /* Set snapshot component enable. */
  result = setEventEnableForSnapShot(jvmti, enable);
  if (result == SUCCESS) {
    /* Set logging component enable. */
    result = setEventEnableForLog(jvmti, enable);
  }

  return result;
}

/*!
 * \brief Setting enable of agent each threads.
 * \param jvmti  [in] JVMTI environment object.
 * \param env    [in] JNI environment object.
 * \param enable [in] Event notification is enable.
 */
void SetThreadEnable(jvmtiEnv *jvmti, JNIEnv *env, bool enable) {
  /* Change thread enable for snapshot. */
  setThreadEnableForSnapShot(jvmti, env, enable);

  /* Change thread enable for log. */
  setThreadEnableForLog(jvmti, env, enable);
}

/*!
 * \brief Interval watching for reload config signal.
 * \param jvmti [in] JVMTI environment object.
 * \param env   [in] JNI environment object.
 */
void ReloadConfigProc(jvmtiEnv *jvmti, JNIEnv *env) {
  /* If catch reload config signal. */
  if (unlikely(flagReloadConfig != 0)) {
    /* Reload configuration. */

    /* If agent is attaching now. */
    if (likely(conf->Attach()->get())) {
      /* Suspend events. */
      SetEventEnable(jvmti, false);

      /* Suspend threads. */
      SetThreadEnable(jvmti, env, false);

      /* Suspend thread status logging. */
      if (conf->ThreadRecordEnable()->get()) {
        TThreadRecorder::finalize(jvmti, env,
                                  conf->ThreadRecordFileName()->get());
      }
    }

    /* If config file is designated at initialization. */
    if (unlikely(loadConfigPath == NULL)) {
      /* Make default configuration path. */
      char confPath[PATH_MAX + 1] = {0};
      snprintf(confPath, PATH_MAX, "%s/heapstats.conf", DEFAULT_CONF_DIR);

      /* Load default config file. */
      conf->loadConfiguration(confPath);
    } else {
      /* Reload designated config file. */
      conf->loadConfiguration(loadConfigPath);
    }

    if (!conf->validate()) {
      logger->printCritMsg(
          "Given configuration is invalid. Use default value.");
      delete conf;
      conf = new TConfiguration(jvmInfo);
      conf->validate();
    }

    /* If agent is attaching now. */
    if (likely(conf->Attach()->get())) {
      /* Ignore perfomance information during agent dettached. */
      jvmInfo->resumeGCinfo();

      /* Restart threads. */
      SetThreadEnable(jvmti, env, true);

      /* Suspend thread status logging. */
      if (conf->ThreadRecordEnable()->get()) {
        TThreadRecorder::initialize(
            jvmti, env, conf->ThreadRecordBufferSize()->get() * 1024 * 1024);
      }
    }

    logger->printInfoMsg("Reloaded configuration file.");
    /* Show setting information. */
    conf->printSetting();
    logger->flush();

    /* If agent is attaching now. */
    if (likely(conf->Attach()->get())) {
      /* Restart events. */
      SetEventEnable(jvmti, true);
    }

    /* Reset signal flag. */
    flagReloadConfig = 0;
  }
}

/*!
 * \brief Interval watching for signals.
 * \param jvmti [in] JVMTI environment object.
 * \param env   [in] JNI environment object.
 * \param cause [in] Cause of Invoking.<br>
 *                   This value is always Interval.
 */
void intervalSigProc(jvmtiEnv *jvmti, JNIEnv *env, TInvokeCause cause) {
  ReloadConfigProc(jvmti, env);

  /* If agent is attaching now and enable log signal. */
  if (likely(conf->Attach()->get() && conf->TriggerOnLogSignal()->get())) {
    intervalSigProcForLog(jvmti, env);
  }
}

/*!
 * \brief JVM initialization event.
 * \param jvmti  [in] JVMTI environment object.
 * \param env    [in] JNI environment object.
 * \param thread [in] Java thread object.
 */
void JNICALL OnVMInit(jvmtiEnv *jvmti, JNIEnv *env, jthread thread) {
/* Get GC information address. */
#ifdef USE_VMSTRUCTS
  jvmInfo->detectInfoAddress();
#else
  jvmInfo->detectInfoAddress(env);
#endif

  /* Get all values from HotSpot VM */
  if (!TVMVariables::getInstance()->getValuesAfterVMInit()) {
    logger->printCritMsg(
        "Cannot gather all values from HotSpot to work HeapStats");
    return;
  }

  if (!conf->validate()) {
    logger->printCritMsg("Given configuration is invalid. Use default value.");
    delete conf;
    conf = new TConfiguration(jvmInfo);
    conf->validate();
  }

  /* Initialize signal flag. */
  flagReloadConfig = 0;

  /* Start HeapStats agent threads. */
  if (conf->ReloadSignal()->get() != NULL) {
    try {
      reloadSigMngr = new TSignalManager(conf->ReloadSignal()->get());
      if (!reloadSigMngr->addHandler(&ReloadSigProc)) {
        logger->printWarnMsg("Reload signal handler setup is failed.");
        conf->ReloadSignal()->set(NULL);
      }
    } catch (const char *errMsg) {
      logger->printWarnMsg(errMsg);
      conf->ReloadSignal()->set(NULL);
    } catch (...) {
      logger->printWarnMsg("Reload signal handler setup is failed.");
      conf->ReloadSignal()->set(NULL);
    }
  }

  /* Calculate alert limit. */
  jlong MaxMemory = jvmInfo->getMaxMemory();
  if (MaxMemory == -1) {
    conf->setAlertThreshold(-1);
    conf->setHeapAlertThreshold(-1);
  } else {
    conf->setAlertThreshold(MaxMemory * conf->AlertPercentage()->get() / 100);
    conf->setHeapAlertThreshold(MaxMemory * conf->HeapAlertPercentage()->get() /
                                100);
  }

  /* Invoke JVM initialize event of snapshot function. */
  onVMInitForSnapShot(jvmti, env);
  /* Invoke JVM initialize event of log function. */
  onVMInitForLog(jvmti, env);

  /* If agent is attaching now. */
  if (likely(conf->Attach()->get())) {
    /* Start and enable each agent threads. */
    SetThreadEnable(jvmti, env, true);

    /* Set event enable in each function. */
    SetEventEnable(jvmti, true);

    /* Start thread status logging. */
    if (conf->ThreadRecordEnable()->get()) {
      TThreadRecorder::initialize(
          jvmti, env, conf->ThreadRecordBufferSize()->get() * 1024 * 1024);
    }
  }

  /* Getting class prepare events. */
  if (TClassPrepareCallback::switchEventNotification(jvmti, JVMTI_ENABLE)) {
    logger->printWarnMsg("HeapStats will be turned off.");

    SetEventEnable(jvmti, false);
    SetThreadEnable(jvmti, env, false);
    logger->flush();
    return;
  }

  /* Show setting information. */
  conf->printSetting();
  logger->flush();

  /* Start reload signal watcher. */
  try {
    intervalSigTimer->start(jvmti, env, SIG_WATCHER_INTERVAL);
  } catch (const char *errMsg) {
    logger->printWarnMsg(errMsg);
  }

  /* Set JNI function to register MBean native function. */
  jniNativeInterface *jniFuncs = NULL;
  if (isError(jvmti, jvmti->GetJNIFunctionTable(&jniFuncs))) {
    logger->printWarnMsg("Could not get JNI Function table.");
  } else if (jniFuncs->reserved0 != NULL) {
    logger->printWarnMsg("JNI Function table #0 is already set.");
  } else {
    jniFuncs->reserved0 = (void *)RegisterHeapStatsNative;

    if (isError(jvmti, jvmti->SetJNIFunctionTable(jniFuncs))) {
      logger->printWarnMsg("Could not set JNI Function table.");
    }
  }

  if (jniFuncs != NULL) {
    jvmti->Deallocate((unsigned char *)jniFuncs);
  }
}

/*!
 * \brief JVM finalization event.
 * \param jvmti [in] JVMTI environment object.
 * \param env   [in] JNI environment object.
 */
void JNICALL OnVMDeath(jvmtiEnv *jvmti, JNIEnv *env) {
  /* If agent is attaching now. */
  if (likely(conf->Attach()->get())) {
    /* Stop and disable event notify. */
    SetEventEnable(jvmti, false);
  }

  /*
   * Terminate signal watcher thread.
   * This thread is used by collect log and reload configuration.
   * So that, need to this thread stop before other all thread stopped.
   */
  intervalSigTimer->terminate();

  /* If reload log signal is enabled. */
  if (likely(reloadSigMngr != NULL)) {
    delete reloadSigMngr;
    reloadSigMngr = NULL;
  }

  /* Invoke JVM finalize event of snapshot function. */
  onVMDeathForSnapShot(jvmti, env);
  /* Invoke JVM finalize event of log function. */
  onVMDeathForLog(jvmti, env);

  /* If agent is attaching now. */
  if (likely(conf->Attach()->get())) {
    /* Stop and disable each thread. */
    SetThreadEnable(jvmti, env, false);

    if (conf->ThreadRecordEnable()->get()) {
      TThreadRecorder::finalize(jvmti, env,
                                conf->ThreadRecordFileName()->get());
    }
  }
}

/*!
 * \brief Abort JVM by force on illegal status.
 * \param jvmti    [in] JVMTI environment object.
 * \param env      [in] JNI environment object.
 * \param causeMsg [in] Message about cause of aborting JVM.
 * \warning This function is always no return.
 */
void forcedAbortJVM(jvmtiEnv *jvmti, JNIEnv *env, const char *causeMsg) {
  /* Terminate all event and thread. */
  OnVMDeath(jvmti, env);
  logger->flush();

  /* Output last message. */
  logger->printCritMsg("Aborting JVM by HeapStats. cause: %s", causeMsg);
  logger->flush();

  abort();
}

/*!
 * \brief Setting JVMTI and extension events.
 * \param jvmti    [in] JVMTI environment object.
 * \param isOnLoad [in] OnLoad phase or not (Live phase).
 * \return Setting process result.
 */
jint InitEventSetting(jvmtiEnv *jvmti, bool isOnLoad) {
  jvmtiCapabilities capabilities = {0};

  /* Set capability for object tagging. */
  capabilities.can_tag_objects = 1;

  /* Set capabilities for Deadlock detector. */
  TDeadlockFinder::setCapabilities(&capabilities, isOnLoad);

  /* Set capabilities for Thread Recording. */
  TThreadRecorder::setCapabilities(&capabilities);

  /* Setup ClassPrepare event. */
  TClassPrepareCallback::mergeCapabilities(&capabilities);
  TClassPrepareCallback::registerCallback(&OnClassPrepare);

  /* Setup DataDumpRequest event. */
  TDataDumpRequestCallback::mergeCapabilities(&capabilities);
  TDataDumpRequestCallback::registerCallback(&OnDataDumpRequestForSnapShot);

  /* Setup garbage collection event. */
  if (conf->TriggerOnFullGC()->get()) {
    TVMVariables *vmVal = TVMVariables::getInstance();

    /* FullGC on G1, we handle it at callbackForG1Full() */
    if (!vmVal->getUseG1()) {
      TGarbageCollectionStartCallback::mergeCapabilities(&capabilities);
      TGarbageCollectionFinishCallback::mergeCapabilities(&capabilities);

      if (vmVal->getUseCMS()) {
        TGarbageCollectionStartCallback::registerCallback(&OnCMSGCStart);
        TGarbageCollectionFinishCallback::registerCallback(&OnCMSGCFinish);
      } else {  // for Parallel GC
        TGarbageCollectionStartCallback::registerCallback(
            &OnGarbageCollectionStart);
        TGarbageCollectionFinishCallback::registerCallback(
            &OnGarbageCollectionFinish);
      }
    }
  }

  /* Setup ResourceExhausted event. */
  if (conf->TriggerOnLogError()->get()) {
    TResourceExhaustedCallback::mergeCapabilities(&capabilities);
    TResourceExhaustedCallback::registerCallback(&OnResourceExhausted);
  }

  /* Setup MonitorContendedEnter event. */
  if (conf->CheckDeadlock()->get()) {
    TMonitorContendedEnterCallback::mergeCapabilities(&capabilities);
    TMonitorContendedEnterCallback::registerCallback(
        &OnMonitorContendedEnterForDeadlock);
  }

  /* Setup VMInit event. */
  TVMInitCallback::mergeCapabilities(&capabilities);
  TVMInitCallback::registerCallback(&OnVMInit);

  /* Setup VMDeath event. */
  TVMDeathCallback::mergeCapabilities(&capabilities);
  TVMDeathCallback::registerCallback(&OnVMDeath);

  /* Set JVMTI event capabilities. */
  if (isError(jvmti, jvmti->AddCapabilities(&capabilities))) {
    logger->printCritMsg("Couldn't set event capabilities.");
    return CAPABILITIES_SETTING_FAILED;
  }

  /* Set JVMTI event callbacks. */
  if (registerJVMTICallbacks(jvmti)) {
    logger->printCritMsg("Couldn't register normal event.");
    return CALLBACKS_SETTING_FAILED;
  }

  /* Enable VMInit and VMDeath. */
  TVMInitCallback::switchEventNotification(jvmti, JVMTI_ENABLE);
  TVMDeathCallback::switchEventNotification(jvmti, JVMTI_ENABLE);

  return SUCCESS;
}

/*!
 * \brief Common initialization function.
 * \param vm    [in]  JavaVM object.
 * \param jvmti [out] JVMTI environment object.
 * \param options [in] Option string which is passed through
 *                     -agentpath/-agentlib.
 * \return Initialize process result.
 */
jint CommonInitialization(JavaVM *vm, jvmtiEnv **jvmti, char *options) {
  /* Initialize logger */
  logger = new TLogger();

  /* Get JVMTI environment object. */
  if (vm->GetEnv((void **)jvmti, JVMTI_VERSION_1) != JNI_OK) {
    logger->printCritMsg("Get JVMTI environment information failed!");
    return GET_ENVIRONMENT_FAILED;
  }

  /* Setup TJvmInfo */
  try {
    jvmInfo = new TJvmInfo();
  } catch (const char *errMsg) {
    logger->printCritMsg(errMsg);
    return GET_LOW_LEVEL_INFO_FAILED;
  }

  if (!jvmInfo->setHSVersion(*jvmti)) {
    return GET_LOW_LEVEL_INFO_FAILED;
  }

  /* Initialize configuration */
  conf = new TConfiguration(jvmInfo);

  /* Parse arguments. */
  if (options == NULL || strlen(options) == 0) {
    /* Make default configuration path. */
    char confPath[PATH_MAX + 1] = {0};
    snprintf(confPath, PATH_MAX, "%s/heapstats.conf", DEFAULT_CONF_DIR);

    conf->loadConfiguration(confPath);
  } else {
    conf->loadConfiguration(options);
    loadConfigPath = strdup(options);
  }

  logger->setLogLevel(conf->LogLevel()->get());
  logger->setLogFile(conf->LogFile()->get());

  /* Show package information. */
  logger->printInfoMsg(PACKAGE_STRING);
  logger->printInfoMsg(
      "Supported processor features:"
#ifdef SSE2
      " SSE2"
#endif
#ifdef SSE3
      " SSE3"
#endif
#ifdef SSE4
      " SSE4"
#endif
#ifdef AVX
      " AVX"
#endif
#ifdef NEON
      " NEON"
#endif
#if (!defined NEON) && (!defined AVX) && (!defined SSE4) && (!defined SSE3) && \
    (!defined SSE2)
      " None"
#endif
      );

  logger->flush();

  if (conf->SnmpSend()->get() &&
      !TTrapSender::initialize(SNMP_VERSION_2c, conf->SnmpTarget()->get(),
                               conf->SnmpComName()->get(), 162)) {
    return SNMP_SETUP_FAILED;
  }

  /* Create thread instances that controlled snapshot trigger. */
  try {
    intervalSigTimer = new TTimer(&intervalSigProc, "HeapStats Signal Watcher");
  } catch (const char *errMsg) {
    logger->printCritMsg(errMsg);
    return AGENT_THREAD_INITIALIZE_FAILED;
  } catch (...) {
    logger->printCritMsg("AgentThread initialize failed!");
    return AGENT_THREAD_INITIALIZE_FAILED;
  }

  /* Invoke agent initialize of each function. */
  jint result;
  result = onAgentInitForSnapShot(*jvmti);
  if (result == SUCCESS) {
    result = onAgentInitForLog();
  }

  return result;
}

/*!
 * \brief Agent attach entry points.
 * \param vm       [in] JavaVM object.
 * \param options  [in] Commandline arguments.
 * \param reserved [in] Reserved.
 * \return Attach initialization result code.
 */
JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
  /* Check memory duplication run. */
  CHECK_DOUBLING_RUN;

  /* JVMTI Event Setup. */
  jvmtiEnv *jvmti;
  int result = CommonInitialization(vm, &jvmti, options);
  if (result != SUCCESS) {
    /* Failure event setup. */
    return result;
  }

  /* Call common initialize. */
  return InitEventSetting(jvmti, true);
}

/*!
 * \brief Common agent unload entry points.
 * \param vm [in] JavaVM object.
 */
JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm) {
  JNIEnv *env = NULL;
  /* Get JNI environment object. */
  vm->GetEnv((void **)&env, JNI_VERSION_1_6);

  /* Invoke agent finalize of snapshot function. */
  onAgentFinalForSnapShot(env);
  /* Invoke agent finalize of log function. */
  onAgentFinalForLog(env);

  /* Destroy object is JVM running informations. */
  delete jvmInfo;
  jvmInfo = NULL;

  /* Destroy object reload signal watcher timer. */
  delete intervalSigTimer;
  intervalSigTimer = NULL;

  /* Delete logger */
  delete logger;

  /* Free allocated configuration file path string. */
  free(loadConfigPath);
  loadConfigPath = NULL;

  /* Cleanup TTrapSender. */
  if (conf->SnmpSend()->get()) {
    TTrapSender::finalize();
  }

  /* Delete configuration */
  delete conf;
}

/*!
 * \brief Ondemand attach's entry points.
 * \param vm       [in] JavaVM object.
 * \param options  [in] Commandline arguments.
 * \param reserved [in] Reserved.
 * \return Ondemand-attach initialization result code.
 */
JNIEXPORT jint JNICALL
    Agent_OnAttach(JavaVM *vm, char *options, void *reserved) {
  /* Check memory duplication run. */
  CHECK_DOUBLING_RUN;

  /* Call common process. */
  jvmtiEnv *jvmti;
  jint result = CommonInitialization(vm, &jvmti, options);
  if (result != SUCCESS) {
    return result;
  }

  /* JVMTI Event Setup. */
  InitEventSetting(jvmti, false);

  /* Get JNI environment object. */
  JNIEnv *env;
  if (vm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK) {
    logger->printCritMsg("Get JNI environment information failed!");
    return GET_ENVIRONMENT_FAILED;
  }

  /* Call live step initialization. */
  OnVMInit(jvmti, env, NULL);

  jvmInfo->detectDelayInfoAddress();

  return SUCCESS;
}

/*!
 * \file configuration.cpp
 * \brief This file treats HeapStats configuration.
 * Copyright (C) 2014-2016 Yasumasa Suenaga
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

#include "globals.hpp"
#include "fsUtil.hpp"
#include "signalManager.hpp"
#include "configuration.hpp"

#if USE_PCRE
#include "pcreRegex.hpp"
#else
#include "cppRegex.hpp"
#endif

/* Macro define. */

/*!
 * \brief Macro of return code length.<br />
 *        E.g. linux is 1(LF), mac is 1(CR), windows is 2 (CRLF).
 */
#define RETURN_CODE_LEN 1

/*!
 * \brief Constructor of TConfiguration.
 */
TConfiguration::TConfiguration(TJvmInfo *info) {
  jvmInfo = info;
  isLoaded = false;
  firstCollected = false; /* Don't collected yet, */

  /* Initialize each configurations. */
  initializeConfig(NULL);
}

/*!
 * \brief Copy constructor of TConfiguration.
 */
TConfiguration::TConfiguration(const TConfiguration &src) {
  jvmInfo = src.jvmInfo;
  isLoaded = false; // Set to false to load configuration from src.
  alertThreshold = src.alertThreshold;
  heapAlertThreshold = src.heapAlertThreshold;

  /* Initialize each configurations. */
  initializeConfig(&src);

  isLoaded = src.isLoaded;
}

/*!
 * \brief Initialize each configurations.
 *
 * \param src Source operand of configuration.
 *            If this value is not NULL, each configurations are copied from
 *            src.
 */
void TConfiguration::initializeConfig(const TConfiguration *src) {
  configs.clear();

  if (src == NULL) {
    attach = new TBooleanConfig(this, "attach", true);
    fileName =
        new TStringConfig(this, "file", (char *)"heapstats_snapshot.dat",
                          &ReadStringValue, (TStringConfig::TFinalizer) & free);
    heapLogFile =
        new TStringConfig(this, "heaplogfile", (char *)"heapstats_log.csv",
                          &ReadStringValue, (TStringConfig::TFinalizer) & free);
    archiveFile =
        new TStringConfig(this, "archivefile", (char *)"heapstats_analyze.zip",
                          &ReadStringValue, (TStringConfig::TFinalizer) & free);
    logFile = new TStringConfig(this, "logfile", (char *)"", &ReadStringValue,
                                (TStringConfig::TFinalizer) & free);
    reduceSnapShot = new TBooleanConfig(this, "reduce_snapshot", true);
    collectRefTree = new TBooleanConfig(this, "collect_reftree", true);
    triggerOnFullGC = new TBooleanConfig(this, "trigger_on_fullgc", true,
                                         &setOnewayBooleanValue);
    triggerOnDump = new TBooleanConfig(this, "trigger_on_dump", true,
                                       &setOnewayBooleanValue);
    checkDeadlock = new TBooleanConfig(this, "check_deadlock", true,
                                       &setOnewayBooleanValue);
    triggerOnLogError = new TBooleanConfig(this, "trigger_on_logerror", true,
                                           &setOnewayBooleanValue);
    triggerOnLogSignal = new TBooleanConfig(this, "trigger_on_logsignal", true,
                                            &setOnewayBooleanValue);
    triggerOnLogLock = new TBooleanConfig(this, "trigger_on_loglock", true);
    rankLevel = new TIntConfig(this, "rank_level", 5);
    logLevel = new TLogLevelConfig(this, "loglevel", INFO, &setLogLevel);
    order = new TRankOrderConfig(this, "rank_order", DELTA);
    alertPercentage = new TIntConfig(this, "alert_percentage", 50);
    heapAlertPercentage = new TIntConfig(this, "javaheap_alert_percentage", 95);
    metaspaceThreshold = new TLongConfig(this, "metaspace_alert_threshold", 0);
    timerInterval = new TLongConfig(this, "snapshot_interval", 0);
    logInterval = new TLongConfig(this, "log_interval", 300);
    firstCollect = new TBooleanConfig(this, "first_collect", true);
    logSignalNormal =
        new TStringConfig(this, "logsignal_normal", NULL, &setSignalValue,
                          (TStringConfig::TFinalizer) & free);
    logSignalAll =
        new TStringConfig(this, "logsignal_all", (char *)"SIGUSR2",
                          &setSignalValue, (TStringConfig::TFinalizer) & free);
    reloadSignal =
        new TStringConfig(this, "signal_reload", (char *)"SIGHUP",
                          &setSignalValue, (TStringConfig::TFinalizer) & free);
    threadRecordEnable =
        new TBooleanConfig(this, "thread_record_enable", false);
    threadRecordBufferSize =
        new TLongConfig(this, "thread_record_buffer_size", 100);
    threadRecordFileName = new TStringConfig(
        this, "thread_record_filename", (char *)"heapstats-thread-records.htr",
        &ReadStringValue, (TStringConfig::TFinalizer) & free);
    threadRecordIOTracer = new TStringConfig(
        this, "thread_record_iotracer",
        (char *)DEFAULT_CONF_DIR "/IoTrace.class",
        &ReadStringValue, (TStringConfig::TFinalizer) & free);
    snmpSend =
        new TBooleanConfig(this, "snmp_send", false, &setOnewayBooleanValue);
    snmpTarget =
        new TStringConfig(this, "snmp_target", (char *)"localhost",
                          &setSnmpTarget, (TStringConfig::TFinalizer) & free);
    snmpComName =
        new TStringConfig(this, "snmp_comname", (char *)"public",
                          &setSnmpComName, (TStringConfig::TFinalizer) & free);
    snmpLibPath =
        new TStringConfig(this, "snmp_libpath", (char *)LIBNETSNMP_PATH,
                          &setSnmpLibPath, (TStringConfig::TFinalizer) & free);
    logDir = new TStringConfig(this, "logdir", (char *)"./tmp",
                               &ReadStringValue,
                               (TStringConfig::TFinalizer) & free);
    archiveCommand = new TStringConfig(
        this, "archive_command",
        (char *)"/usr/bin/zip %archivefile% -jr %logdir%",
        &ReadStringValue, (TStringConfig::TFinalizer) & free);
    killOnError = new TBooleanConfig(this, "kill_on_error", false);
  } else {
    attach = new TBooleanConfig(*src->attach);
    fileName = new TStringConfig(*src->fileName);
    heapLogFile = new TStringConfig(*src->heapLogFile);
    archiveFile = new TStringConfig(*src->archiveFile);
    logFile = new TStringConfig(*src->logFile);
    reduceSnapShot = new TBooleanConfig(*src->reduceSnapShot);
    collectRefTree = new TBooleanConfig(*src->collectRefTree);
    triggerOnFullGC = new TBooleanConfig(*src->triggerOnFullGC);
    triggerOnDump = new TBooleanConfig(*src->triggerOnDump);
    checkDeadlock = new TBooleanConfig(*src->checkDeadlock);
    triggerOnLogError = new TBooleanConfig(*src->triggerOnLogError);
    triggerOnLogSignal = new TBooleanConfig(*src->triggerOnLogSignal);
    triggerOnLogLock = new TBooleanConfig(*src->triggerOnLogLock);
    rankLevel = new TIntConfig(*src->rankLevel);
    logLevel = new TLogLevelConfig(*src->logLevel);
    order = new TRankOrderConfig(*src->order);
    alertPercentage = new TIntConfig(*src->alertPercentage);
    heapAlertPercentage = new TIntConfig(*src->heapAlertPercentage);
    metaspaceThreshold = new TLongConfig(*src->metaspaceThreshold);
    timerInterval = new TLongConfig(*src->timerInterval);
    logInterval = new TLongConfig(*src->logInterval);
    firstCollect = new TBooleanConfig(*src->firstCollect);
    logSignalNormal = new TStringConfig(*src->logSignalNormal);
    logSignalAll = new TStringConfig(*src->logSignalAll);
    reloadSignal = new TStringConfig(*src->reloadSignal);
    threadRecordEnable = new TBooleanConfig(*src->threadRecordEnable);
    threadRecordBufferSize = new TLongConfig(*src->threadRecordBufferSize);
    threadRecordFileName = new TStringConfig(*src->threadRecordFileName);
    threadRecordIOTracer = new TStringConfig(*src->threadRecordIOTracer);
    snmpSend = new TBooleanConfig(*src->snmpSend);
    snmpTarget = new TStringConfig(*src->snmpTarget);
    snmpComName = new TStringConfig(*src->snmpComName);
    snmpLibPath = new TStringConfig(*src->snmpLibPath);
    logDir = new TStringConfig(*src->logDir);
    archiveCommand = new TStringConfig(*src->archiveCommand);
    killOnError = new TBooleanConfig(*src->killOnError);
  }

  configs.push_back(attach);
  configs.push_back(fileName);
  configs.push_back(heapLogFile);
  configs.push_back(archiveFile);
  configs.push_back(logFile);
  configs.push_back(reduceSnapShot);
  configs.push_back(collectRefTree);
  configs.push_back(triggerOnFullGC);
  configs.push_back(triggerOnDump);
  configs.push_back(checkDeadlock);
  configs.push_back(triggerOnLogError);
  configs.push_back(triggerOnLogSignal);
  configs.push_back(triggerOnLogLock);
  configs.push_back(rankLevel);
  configs.push_back(logLevel);
  configs.push_back(order);
  configs.push_back(alertPercentage);
  configs.push_back(heapAlertPercentage);
  configs.push_back(metaspaceThreshold);
  configs.push_back(timerInterval);
  configs.push_back(logInterval);
  configs.push_back(firstCollect);
  configs.push_back(logSignalNormal);
  configs.push_back(logSignalAll);
  configs.push_back(reloadSignal);
  configs.push_back(threadRecordEnable);
  configs.push_back(threadRecordBufferSize);
  configs.push_back(threadRecordFileName);
  configs.push_back(threadRecordIOTracer);
  configs.push_back(snmpSend);
  configs.push_back(snmpTarget);
  configs.push_back(snmpComName);
  configs.push_back(snmpLibPath);
  configs.push_back(logDir);
  configs.push_back(archiveCommand);
  configs.push_back(killOnError);
}

/*!
 * \brief Destructor of TConfiguration.
 */
TConfiguration::~TConfiguration() {
  for (std::list<TConfigElementSuper *>::iterator itr = configs.begin();
       itr != configs.end(); itr++) {
    delete *itr;
  }
}

/*!
 * \brief Read boolean value from configuration.
 * \param value [in] Value of this configuration.
 * \return value which is represented by boolean.
 */
bool TConfiguration::ReadBooleanValue(const char *value) {
  if (strcmp(value, "true") == 0) {
    return true;
  } else if (strcmp(value, "false") == 0) {
    return false;
  } else {
    throw "Illegal boolean value";
  }
}

/*!
 * \brief Read string value from configuration.
 * \param value [in] Value of this configuration.
 * \param dest [in] [out] Destination of this configuration.
 */
void TConfiguration::ReadStringValue(TConfiguration *inst, char *value,
                                     char **dest) {
  if (*dest != NULL) {
    free(*dest);
  }

  *dest = value == NULL ? NULL : strdup(value);
}

/*!
 * \brief Read string value for signal from configuration.
 * \param value [in] Value of this configuration.
 * \param dest [in] [out] Destination of this configuration.
 */
void TConfiguration::ReadSignalValue(const char *value, char **dest) {
  if ((value == NULL) || (value[0] == '\0')) {
    if (*dest != NULL) {
      free(*dest);
    }

    *dest = NULL;
  } else if (TSignalManager::findSignal(value) != -1) {
    if (*dest != NULL) {
      free(*dest);
    }

    *dest = strdup(value);
  } else {
    throw "Illegal signal name";
  }
}

/*!
 * \brief Read long/int value from configuration.
 * \param value [in] Value of this configuration.
 * \param max_val [in] Max value of this parameter.
 * \return value which is represented by long.
 */
long TConfiguration::ReadLongValue(const char *value, const jlong max_val) {
  /* Convert to number from string. */
  char *done = NULL;
  errno = 0;
  jlong temp = strtoll(value, &done, 10);

  /* If all string is able to convert to number. */
  if ((*done == '\0') && (temp <= max_val) && (temp >= 0)) {
    return temp;
  } else if (((temp == LLONG_MIN) || (temp == LLONG_MAX)) && (errno != 0)) {
    throw errno;
  } else {
    throw "Illegal number";
  }
}

/*!
 * \brief Read order value from configuration.
 * \param value [in] Value of this configuration.
 * \return value which is represented by TRankOrder.
 */
TRankOrder TConfiguration::ReadRankOrderValue(const char *value) {
  if (strcmp(value, "usage") == 0) {
    return USAGE;
  } else if (strcmp(value, "delta") == 0) {
    return DELTA;
  } else {
    throw "Illegal order";
  }
}

/*!
 * \brief Read log level value from configuration.
 * \param value [in] Value of this configuration.
 * \return value which is represented by TLogLevel.
 */
TLogLevel TConfiguration::ReadLogLevelValue(const char *value) {
  if (strcmp(value, "CRIT") == 0) {
    return CRIT;
  } else if (strcmp(value, "WARN") == 0) {
    return WARN;
  } else if (strcmp(value, "INFO") == 0) {
    return INFO;
  } else if (strcmp(value, "DEBUG") == 0) {
    return DEBUG;
  } else {
    throw "Illegal level";
  }
}

/*!
 * \brief Load configuration from file.
 * \param filename [in] Read configuration file path.
 */
void TConfiguration::loadConfiguration(const char *filename) {
  /* Check filename. */
  if (filename == NULL || strlen(filename) == 0) {
    return;
  }

  /* Open file. */
  FILE *conf = fopen(filename, "r");
  if (unlikely(conf == NULL)) {
    logger->printWarnMsgWithErrno("Could not open configuration file: %s",
                                  filename);
    return;
  }

#if USE_PCRE
  TPCRERegex confRegex("^\\s*(\\S+?)\\s*=\\s*(\\S+)?\\s*$", 9);
#else
  TCPPRegex confRegex("^\\s*(\\S+?)\\s*=\\s*(\\S+)?\\s*$");
#endif

  /* Get string line from configure file. */
  long lineCnt = 0;
  char *lineBuff = NULL;
  size_t lineBuffLen = 0;

  /* Read line. */
  while (likely(getline(&lineBuff, &lineBuffLen, conf) > 0)) {
    lineCnt++;
    /* If this line is empty. */
    if (unlikely(strlen(lineBuff) <= RETURN_CODE_LEN)) {
      /* skip this line. */
      continue;
    }

    /* Remove comments */
    char *comment = strchr(lineBuff, '#');
    if (comment != NULL) {
      *comment = '\0';
    }

    /* Check matched pair. */
    if (confRegex.find(lineBuff)) {
      /* Key and value variables. */
      char *key = confRegex.group(1);
      char *value;
      try {
        value = confRegex.group(2);
      } catch (const char *errStr) {
        logger->printDebugMsg(errStr);
        value = (char *)calloc(1, sizeof(char));
      }

      /* Check key name. */
      try {
        applyConfig(key, value);
      } catch (const char *errStr) {
        logger->printWarnMsg("Configuration error(key=%s, value=%s): %s", key,
                             value, errStr);
      } catch (int err) {
        errno = err;
        logger->printWarnMsgWithErrno("Configuration error(key=%s, value=%s) ",
                                      key, value);
      }

      /* Cleanup after param setting. */
      free(key);
      free(value);
    }
  }

  /* Cleanup after load file. */
  if (likely(lineBuff != NULL)) {
    free(lineBuff);
  }
  fclose(conf);

  isLoaded = true;
  firstCollected = false;
}

/*!
 * \brief Print setting information.
 */
void TConfiguration::printSetting(void) {
  /* Agent attach state. */
  logger->printInfoMsg("Agent Attach Enable = %s",
                       attach->get() ? "true" : "false");

  /* Output filenames. */
  logger->printInfoMsg("SnapShot FileName = %s", fileName->get());
  logger->printInfoMsg("Heap Log FileName = %s", heapLogFile->get());
  logger->printInfoMsg("Archive FileName = %s", archiveFile->get());
  logger->printInfoMsg(
      "Console Log FileName = %s",
      strlen(logFile->get()) > 0 ? logFile->get() : "None (output to console)");

  /* Output log-level. */
  logger->printInfoMsg("LogLevel = %s", getLogLevelAsString());

  /* Output about reduce SnapShot. */
  logger->printInfoMsg("ReduceSnapShot = %s",
                       reduceSnapShot->get() ? "true" : "false");

  /* Output whether collecting reftree. */
  logger->printInfoMsg("CollectRefTree = %s",
                       collectRefTree->get() ? "true" : "false");

  /* Output status of snapshot triggers. */
  logger->printInfoMsg("Trigger on FullGC = %s",
                       triggerOnFullGC->get() ? "true" : "false");
  logger->printInfoMsg("Trigger on DumpRequest = %s",
                       triggerOnDump->get() ? "true" : "false");

  /* Output status of deadlock check. */
  logger->printInfoMsg("Deadlock check = %s",
                       checkDeadlock->get() ? "true" : "false");

  /* Output status of logging triggers. */
  logger->printInfoMsg("Log trigger on Error = %s",
                       triggerOnLogError->get() ? "true" : "false");
  logger->printInfoMsg("Log trigger on Signal = %s",
                       triggerOnLogSignal->get() ? "true" : "false");
  logger->printInfoMsg("Log trigger on Deadlock = %s",
                       triggerOnLogLock->get() ? "true" : "false");

  /* Output about ranking. */
  logger->printInfoMsg("RankingOrder = %s", getRankOrderAsString());
  logger->printInfoMsg("RankLevel = %d", rankLevel->get());

  /* Output about heap alert. */
  if (alertThreshold <= 0) {
    logger->printInfoMsg("HeapAlert is DISABLED.");
  } else {
    logger->printInfoMsg("AlertPercentage = %d ( %lu bytes )",
                         alertPercentage->get(), alertThreshold);
  }

  /* Output about heap alert. */
  if (heapAlertThreshold <= 0) {
    logger->printInfoMsg("Java heap usage alert is DISABLED.");
  } else {
    logger->printInfoMsg("Java heap usage alert percentage = %d ( %lu MB )",
                         heapAlertPercentage->get(),
                         heapAlertThreshold / 1024 / 1024);
  }

  /* Output about metaspace alert. */
  const char *label = jvmInfo->isAfterCR6964458() ? "Metaspace" : "PermGen";

  if (metaspaceThreshold <= 0) {
    logger->printInfoMsg("%s usage alert is DISABLED.", label);
  } else {
    logger->printInfoMsg("%s usage alert threshold %lu MB", label,
                         metaspaceThreshold->get() / 1024 / 1024);
  }

  /* Output about interval snapshot. */
  if (timerInterval == 0) {
    logger->printInfoMsg("Interval SnapShot is DISABLED.");
  } else {
    logger->printInfoMsg("SnapShot interval = %d sec", timerInterval->get());
  }

  /* Output about interval logging. */
  if (logInterval->get() == 0) {
    logger->printInfoMsg("Interval Logging is DISABLED.");
  } else {
    logger->printInfoMsg("Log interval = %d sec", logInterval->get());
  }

  logger->printInfoMsg("First collect log = %s",
                       firstCollect->get() ? "true" : "false");

  /* Output logging signal name. */
  char *normalSig = logSignalNormal->get();
  if (normalSig == NULL || strlen(normalSig) == 0) {
    logger->printInfoMsg("Signal for normal logging is DISABLED.");
  } else {
    logger->printInfoMsg("Signal for normal logging = %s", normalSig);
  }

  char *allSig = logSignalAll->get();
  if (allSig == NULL || strlen(allSig) == 0) {
    logger->printInfoMsg("Signal for all logging is DISABLED.");
  } else {
    logger->printInfoMsg("Signal for all logging = %s", allSig);
  }

  char *reloadSig = reloadSignal->get();
  if (reloadSig == NULL || strlen(reloadSig) == 0) {
    logger->printInfoMsg("Signal for config reloading is DISABLED.");
  } else {
    logger->printInfoMsg("Signal for config reloading = %s", reloadSig);
  }

  /* Thread recorder. */
  logger->printInfoMsg("Thread recorder = %s",
                       threadRecordEnable->get() ? "true" : "false");
  logger->printInfoMsg("Buffer size of thread recorder = %ld MB",
                       threadRecordBufferSize->get());
  logger->printInfoMsg("Thread record file name = %s",
                       threadRecordFileName->get());
  logger->printInfoMsg("Thread record I/O tracer = %s",
                       threadRecordIOTracer->get());

  /* Output about SNMP trap. */
  logger->printInfoMsg("Send SNMP Trap = %s",
                       snmpSend->get() ? "true" : "false");
  logger->printInfoMsg("SNMP target = %s", snmpTarget->get());
  logger->printInfoMsg("SNMP community = %s", snmpComName->get());
  logger->printInfoMsg("NET-SNMP client library path = %s", snmpLibPath->get());

  /* Output temporary log directory path. */
  logger->printInfoMsg("Temporary log directory = %s", logDir->get());

  /* Output archive command. */
  logger->printInfoMsg("Archive command = \"%s\"", archiveCommand->get());

  /* Output about force killing JVM. */
  logger->printInfoMsg("Kill on Error = %s",
                       killOnError->get() ? "true" : "false");
}

/*!
 * \brief Validate this configuration..
 * \return Return true if all configuration is valid.
 */
bool TConfiguration::validate(void) {
  bool result = true;

  /* File check */
  TStringConfig *filenames[] = {fileName, heapLogFile, archiveFile,
                                logFile,  logDir,      NULL};
  for (TStringConfig **elmt = filenames; *elmt != NULL; elmt++) {
    if (strlen((*elmt)->get()) == 0) {
      // "" means "disable", not a file path like "./".
      continue;
    }
    try {
      if (!isValidPath((*elmt)->get())) {
        throw "Permission denied";
      }
    } catch (const char *message) {
      logger->printWarnMsg("%s: %s = %s", message, (*elmt)->getConfigName(),
                           (*elmt)->get());
      result = false;
    } catch (int errnum) {
      logger->printWarnMsgWithErrno("Configuration error: %s = %s",
                                    (*elmt)->getConfigName(), (*elmt)->get());
      result = false;
    }
  }

  /* Range check */
  TIntConfig *percentages[] = {alertPercentage, heapAlertPercentage, NULL};
  for (TIntConfig **percentage = percentages; *percentage != NULL;
       percentage++) {
    if (((*percentage)->get() < 0) || ((*percentage)->get() > 100)) {
      logger->printWarnMsg("Out of range: %s = %d",
                           (*percentage)->getConfigName(),
                           (*percentage)->get());
      result = false;
    }
  }

  /* Set alert threshold. */
  jlong maxMem = this->jvmInfo->getMaxMemory();
  alertThreshold =
      (maxMem == -1) ? -1 : (maxMem * alertPercentage->get() / 100);
  heapAlertThreshold =
      (maxMem == -1) ? -1 : (maxMem * heapAlertPercentage->get() / 100);

  /* Signal check */
  char *reloadSig = reloadSignal->get();
  char *normalSig = logSignalNormal->get();
  char *allSig = logSignalAll->get();
  if (reloadSig != NULL) {
    if ((normalSig != NULL) && (strcmp(normalSig, reloadSig) == 0)) {
      logger->printWarnMsg(
          "Cannot set same signal: logsignal_normal & signal_reload");
      result = false;
    }

    if ((allSig != NULL) && (strcmp(allSig, reloadSig) == 0)) {
      logger->printWarnMsg(
          "Cannot set same signal: logsignal_all & signal_reload");
      result = false;
    }
  }

  if ((normalSig != NULL) && (allSig != NULL) &&
      (strcmp(normalSig, allSig) == 0)) {
    logger->printWarnMsg(
        "Cannot set same signal: logsignal_normal & logsignal_all");
    result = false;
  }

  /* Thread recorder check */
  if (threadRecordEnable->get()) {
    if (threadRecordBufferSize <= 0) {
      logger->printWarnMsg("Invalid value: thread_record_buffer_size = %ld",
                           threadRecordBufferSize->get());
      result = false;
    } else if (!isValidPath(threadRecordFileName->get())) {
      logger->printWarnMsg("Permission denied: thread_record_filename = %s",
                           threadRecordFileName->get());
      result = false;
    }
  }

  /* SNMP check */
  if (snmpSend->get()) {
    if (snmpLibPath->get() == NULL) {
      logger->printWarnMsg("snmp_libpath must be set when snmp_send is set.");
      result = false;
    }

    if ((snmpTarget->get() == NULL) || (strlen(snmpTarget->get()) == 0)) {
      logger->printWarnMsg("snmp_target have to be set when snmp_send is set");
      result = false;
    }

    if ((snmpComName->get() == NULL) || (strlen(snmpComName->get()) == 0)) {
      logger->printWarnMsg("snmp_comname have to be set when snmp_send is set");
      result = false;
    }
  }

  return result;
}

/*!
 * \brief Merge configuration from others.
 * \param src Pointer of source configuration.
 */
void TConfiguration::merge(TConfiguration *src) {
  attach->set(src->attach->get());
  fileName->set(src->fileName->get());
  heapLogFile->set(src->heapLogFile->get());
  archiveFile->set(src->archiveFile->get());
  logFile->set(src->logFile->get());
  rankLevel->set(src->rankLevel->get());
  logLevel->set(src->logLevel->get());
  reduceSnapShot->set(src->reduceSnapShot->get());
  collectRefTree->set(src->collectRefTree->get());
  triggerOnFullGC->set(triggerOnFullGC->get() && src->triggerOnFullGC->get());
  triggerOnDump->set(triggerOnDump->get() && src->triggerOnDump->get());
  checkDeadlock->set(checkDeadlock->get() && src->checkDeadlock->get());
  triggerOnLogError->set(triggerOnLogError->get() &&
                         src->triggerOnLogError->get());
  triggerOnLogSignal->set(triggerOnLogSignal->get() &&
                          src->triggerOnLogSignal->get());
  triggerOnLogLock->set(triggerOnLogLock->get() &&
                        src->triggerOnLogLock->get());
  order->set(src->order->get());
  alertPercentage->set(src->alertPercentage->get());
  heapAlertPercentage->set(src->heapAlertPercentage->get());
  metaspaceThreshold->set(src->metaspaceThreshold->get());
  timerInterval->set(src->timerInterval->get());
  logInterval->set(src->logInterval->get());
  firstCollect->set(src->firstCollect->get());
  threadRecordFileName->set(src->threadRecordFileName->get());
  snmpSend->set(snmpSend->get() & src->snmpSend->get());
  logDir->set(src->logDir->get());
  archiveCommand->set(src->archiveCommand->get());
  killOnError->set(src->killOnError->get());
}

/*!
 * \brief Apply value to configuration which is presented by key.
 * \param key Key of configuration.
 * \param value New value.
 * \exception Throws const char * or int which presents error.
 */
void TConfiguration::applyConfig(char *key, char *value) {
  for (std::list<TConfigElementSuper *>::iterator itr = configs.begin();
       itr != configs.end(); itr++) {
    if (strcmp((*itr)->getConfigName(), key) == 0) {
      switch ((*itr)->getConfigDataType()) {
        case BOOLEAN:
          ((TBooleanConfig *)*itr)->set(ReadBooleanValue(value));
          break;
        case INTEGER:
          ((TIntConfig *)*itr)->set(ReadLongValue(value, INT_MAX));
          break;
        case LONG:
          ((TLongConfig *)*itr)->set(ReadLongValue(value, JLONG_MAX));
          break;
        case STRING:
          ((TStringConfig *)*itr)->set(value);
          break;
        case LOGLEVEL:
          ((TLogLevelConfig *)*itr)->set(ReadLogLevelValue(value));
          break;
        case RANKORDER:
          ((TRankOrderConfig *)*itr)->set(ReadRankOrderValue(value));
          break;
      }
    }
  }
}

void TConfiguration::setLogLevel(TConfiguration *inst,
                                 TLogLevel val, TLogLevel *dest) {
  *dest = val;
  logger->setLogLevel(val);
}


/*!
 * \file configuration.hpp
 * \brief This file treats HeapStats configuration.
 * Copyright (C) 2014-2015 Yasumasa Suenaga
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

#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <jni.h>

#include <list>

#include "jvmInfo.hpp"
#include "logger.hpp"

/*!
 * \brief Ranking Order.<br>
 *        This order affects heap alert.
 */
typedef enum {
  DELTA, /*!< Ranking is sorted by delta from before snapshot. */
  USAGE  /*!< Ranking is sorted by heap using size.            */
} TRankOrder;

/*!
 * \brief Configuration data types.
 */
typedef enum {
  BOOLEAN,
  INTEGER,
  LONG,
  STRING,
  LOGLEVEL,
  RANKORDER
} TConfigDataType;

/* Forward declaration. */
class TConfiguration;

/*!
 * \brief Super class of configuration holder.
 */
class TConfigElementSuper {
 public:
  virtual ~TConfigElementSuper(){};
  virtual TConfigDataType getConfigDataType() = 0;
  virtual const char *getConfigName() = 0;
};

/*!
 * \brief Implementation of configuration.
 *
 * \param T C++ data type.
 * \param V Configuration data type.
 */
template <typename T, TConfigDataType V>
class TConfigElement : public TConfigElementSuper {
 public:
  typedef void (*TSetter)(TConfiguration *inst, T value, T *dest);
  typedef void (*TFinalizer)(T value);

 private:
  /*!
   * \brief Instance of TConfiguration which is related to this config.
   */
  TConfiguration *config;

  /*!
   * \brief Configuration name in heapstats.conf .
   */
  const char *configName;

  /*!
   * \brief Configuration value.
   */
  T value;

  /*!
   * \brief Setter of this configuration.
   *        If this value is NULL, configuration will be stored directly.
   */
  TSetter setter;

  /*!
   * \brief Finalizer of this configuration.
   *        If this value is not NULL, it will be called at destructor
   *        of this class.
   *        If you can use C++ pointer type configuration, you can set
   *        free() to this value.
   */
  TFinalizer finalizer;

 public:
  TConfigElement(TConfiguration *cnf, const char *name, T initVal,
                 TSetter sttr = NULL, TFinalizer fnlzr = NULL) {
    config = cnf;
    configName = strdup(name);
    value = (T)0;
    setter = sttr;
    finalizer = fnlzr;

    set(initVal);
  }

  TConfigElement(const TConfigElement<T, V> &src) {
    config = src.config;
    configName = strdup(src.configName);
    value = (T)0;
    setter = src.setter;
    finalizer = src.finalizer;

    set(src.value);
  }

  virtual ~TConfigElement() {
    if (finalizer != NULL) {
      finalizer(value);
    }
  }

  /*!
   * \brief Get configuration data type.
   * \return Data type of configuration.
   */
  TConfigDataType getConfigDataType() { return V; }

  /*!
   * \brief Get configuration name.
   * \return Configuration name in heapstats.conf .
   */
  const char *getConfigName() { return configName; }

  /*!
   * \brief Getter of value in this configuration.
   * \return Current configuration value in this configuration.
   */
  inline T get() { return value; }

  /*!
   * \brief Setter of value in this configuration.
   *        If you set setter function, configuration value will be set
   *        through its function.
   */
  inline void set(T val) {
    if (setter == NULL) {
      value = val;
    } else {
      setter(config, val, &value);
    }
  }
};

/* Configuration definition */
typedef TConfigElement<bool, BOOLEAN> TBooleanConfig;
typedef TConfigElement<int, INTEGER> TIntConfig;
typedef TConfigElement<jlong, LONG> TLongConfig;
typedef TConfigElement<char *, STRING> TStringConfig;
typedef TConfigElement<TLogLevel, LOGLEVEL> TLogLevelConfig;
typedef TConfigElement<TRankOrder, RANKORDER> TRankOrderConfig;

/*!
 * \brief Configuration holder.
 *        This class contains current configuration value.
 */
class TConfiguration {
 private:
  /*!< Flag of agent attach enable. */
  TBooleanConfig *attach;

  /*!< Output snapshot file name. */
  TStringConfig *fileName;

  /*!< Output common log file name. */
  TStringConfig *heapLogFile;

  /*!< Output archive log file name. */
  TStringConfig *archiveFile;

  /*!< Output console log file name. */
  TStringConfig *logFile;

  /*!< Is reduced snapshot. */
  TBooleanConfig *reduceSnapShot;

  /*!< Whether collecting reftree. */
  TBooleanConfig *collectRefTree;

  /*!< Make snapshot is triggered by Full GC. */
  TBooleanConfig *triggerOnFullGC;

  /*!< Make snapshot is triggered by dump request. */
  TBooleanConfig *triggerOnDump;

  /*!< Is deadlock finder enabled? */
  TBooleanConfig *checkDeadlock;

  /*!< Logging on JVM error(Resoure exhausted). */
  TBooleanConfig *triggerOnLogError;

  /*!< Logging on received signal. */
  TBooleanConfig *triggerOnLogSignal;

  /*!< Logging on occurred deadlock. */
  TBooleanConfig *triggerOnLogLock;

  /*!< Count of show ranking. */
  TIntConfig *rankLevel;

  /*!< Output log level. */
  TLogLevelConfig *logLevel;

  /*!< Order of ranking. */
  TRankOrderConfig *order;

  /*!< Percentage of trigger alert in heap. */
  TIntConfig *alertPercentage;

  /*!< Alert percentage of javaHeapAlert. */
  TIntConfig *heapAlertPercentage;

  /*!< Trigger usage for javaMetaspaceAlert. */
  TLongConfig *metaspaceThreshold;

  /*!< Interval of periodic snapshot. */
  TLongConfig *timerInterval;

  /*!< Interval of periodic logging. */
  TLongConfig *logInterval;

  /*!< Logging on JVM error only first time. */
  TBooleanConfig *firstCollect;

  /*!< Name of signal logging about process. */
  TStringConfig *logSignalNormal;

  /*!< Name of signal logging about environment. */
  TStringConfig *logSignalAll;

  /*!< Name of signal reload configuration file. */
  TStringConfig *reloadSignal;

  /*!< Flag of thread recorder enable. */
  TBooleanConfig *threadRecordEnable;

  /*!< Buffer size of thread recorder. */
  TLongConfig *threadRecordBufferSize;

  /*!< Thread record filename. */
  TStringConfig *threadRecordFileName;

  /*!< Class file for I/O tracing. */
  TStringConfig *threadRecordIOTracer;

  /*!< Flag of SNMP trap send enable. */
  TBooleanConfig *snmpSend;

  /*!< SNMP trap send target. */
  TStringConfig *snmpTarget;

  /*!< SNMP trap community name. */
  TStringConfig *snmpComName;

  /*!< Path of working directory for log archive. */
  TStringConfig *logDir;

  /*!< Command was execute to making log archive. */
  TStringConfig *archiveCommand;

  /*!< Abort JVM on resoure exhausted or deadlock. */
  TBooleanConfig *killOnError;

  /*!< Array of all configurations. */
  std::list<TConfigElementSuper *> configs;

  /*!< Configs are already loaded. */
  bool isLoaded;

  /*!< Trigger usage for javaHeapAlert. */
  jlong heapAlertThreshold;

  /*!< Size of trigger alert in heap. */
  jlong alertThreshold;

  /*!< Flag of already logged on JVM error. */
  bool firstCollected;

  /*!< JVM statistics. */
  TJvmInfo *jvmInfo;

 protected:
  /*!
   * \brief Check signal is supported by JVM.
   * \param sigName [in] Name of signal.
   * \return Signal is supported, if value is true.
   *         Otherwise, value is false.
   * \sa Please see below JDK source about JVM supported signal.<br>
   *     hotspot/src/os/linux/vm/jvm_linux.cpp
   */
  bool isSupportSignal(char const *sigName);

  /*!
   * \brief Read string value from configuration.
   * \param value [in] Value of this configuration.
   * \param dest [in] [out] Destination of this configuration.
   */
  static void ReadStringValue(TConfiguration *inst, char *value, char **dest);

  /*!
   * \brief Read string value for signal from configuration.
   * \param value [in] Value of this configuration.
   * \param dest [in] [out] Destination of this configuration.
   */
  void ReadSignalValue(const char *value, char **dest);

 public:
  TConfiguration(TJvmInfo *info);

  TConfiguration(const TConfiguration &src);

  virtual ~TConfiguration();

  /*!
   * \brief Read boolean value from configuration.
   * \param value [in] Value of this configuration.
   * \return value which is represented by boolean.
   */
  bool ReadBooleanValue(const char *value);

  /*!
   * \brief Read long/int value from configuration.
   * \param value [in] Value of this configuration.
   * \param max_val [in] Max value of this parameter.
   * \return value which is represented by long.
   */
  long ReadLongValue(const char *value, const jlong max_val);

  /*!
   * \brief Read order value from configuration.
   * \param value [in] Value of this configuration.
   * \return value which is represented by TRankOrder.
   */
  TRankOrder ReadRankOrderValue(const char *value);

  /*!
   * \brief Read log level value from configuration.
   * \param value [in] Value of this configuration.
   * \return value which is represented by TLogLevel.
   */
  TLogLevel ReadLogLevelValue(const char *value);

  /*!
   * \brief Load configuration from file.
   * \param filename [in] Read configuration file path.
   */
  void loadConfiguration(const char *filename);

  /*!
   * \brief Print setting information.
   */
  void printSetting(void);

  /*!
   * \brief Validate this configuration.
   * \return Return true if all configuration is valid.
   */
  bool validate(void);

  /*!
   * \brief Merge configuration from others.
   * \param src Pointer of source configuration.
   */
  void merge(TConfiguration *src);

  /*!
   * \brief Apply value to configuration which is presented by key.
   * \param key Key of configuration.
   * \param value New value.
   * \exception Throws const char * or int which presents error.
   */
  void applyConfig(char *key, char *value);

  /* Accessors */
  TBooleanConfig *Attach() { return attach; }
  TStringConfig *FileName() { return fileName; }
  TStringConfig *HeapLogFile() { return heapLogFile; }
  TStringConfig *ArchiveFile() { return archiveFile; }
  TStringConfig *LogFile() { return logFile; }
  TBooleanConfig *ReduceSnapShot() { return reduceSnapShot; }
  TBooleanConfig *CollectRefTree() { return collectRefTree; }
  TBooleanConfig *TriggerOnFullGC() { return triggerOnFullGC; }
  TBooleanConfig *TriggerOnDump() { return triggerOnDump; }
  TBooleanConfig *CheckDeadlock() { return checkDeadlock; }
  TBooleanConfig *TriggerOnLogError() { return triggerOnLogError; }
  TBooleanConfig *TriggerOnLogSignal() { return triggerOnLogSignal; }
  TBooleanConfig *TriggerOnLogLock() { return triggerOnLogLock; }
  TIntConfig *RankLevel() { return rankLevel; }
  TLogLevelConfig *LogLevel() { return logLevel; }
  TRankOrderConfig *Order() { return order; }
  TIntConfig *AlertPercentage() { return alertPercentage; }
  TIntConfig *HeapAlertPercentage() { return heapAlertPercentage; }
  TLongConfig *MetaspaceThreshold() { return metaspaceThreshold; }
  TLongConfig *TimerInterval() { return timerInterval; }
  TLongConfig *LogInterval() { return logInterval; }
  TBooleanConfig *FirstCollect() { return firstCollect; }
  TStringConfig *LogSignalNormal() { return logSignalNormal; }
  TStringConfig *LogSignalAll() { return logSignalAll; }
  TStringConfig *ReloadSignal() { return reloadSignal; }
  TBooleanConfig *ThreadRecordEnable() { return threadRecordEnable; }
  TLongConfig *ThreadRecordBufferSize() { return threadRecordBufferSize; }
  TStringConfig *ThreadRecordFileName() { return threadRecordFileName; }
  TStringConfig *ThreadRecordIOTracer() { return threadRecordIOTracer; }
  TBooleanConfig *SnmpSend() { return snmpSend; }
  TStringConfig *SnmpTarget() { return snmpTarget; }
  TStringConfig *SnmpComName() { return snmpComName; }
  TStringConfig *LogDir() { return logDir; }
  TStringConfig *ArchiveCommand() { return archiveCommand; }
  TBooleanConfig *KillOnError() { return killOnError; }

  jlong getHeapAlertThreshold() { return heapAlertThreshold; }
  void setHeapAlertThreshold(jlong val) { heapAlertThreshold = val; }
  jlong getAlertThreshold() { return alertThreshold; }
  void setAlertThreshold(jlong val) { alertThreshold = val; }
  bool isFirstCollected() { return firstCollected; }
  void setFirstCollected(bool val) { firstCollected = val; }
  std::list<TConfigElementSuper *> getConfigs() { return configs; }

  /*!
   * \brief Get current log level as string.
   * \return String of current log level.
   */
  const char *getLogLevelAsString() {
    const char *loglevel_str[] = {"Unknown", "CRIT", "WARN", "INFO", "DEBUG"};
    return loglevel_str[logLevel->get()];
  }

  /*!
   * \brief Get current rank order as string.
   * \return String of current rank order.
   */
  const char *getRankOrderAsString() {
    const char *rankorder_str[] = {"delta", "usage"};
    return rankorder_str[order->get()];
  }

  /*!
   * \brief Initialize each configurations.
   *
   * \param src Source operand of configuration.
   *            If this value is not NULL, each configurations are copied
   *            from  src.
   */
  void initializeConfig(const TConfiguration *src);

  /* Setters */
  static void setOnewayBooleanValue(TConfiguration *inst, bool val,
                                    bool *dest) {
    if (inst->isLoaded && !(*dest) && val) {
      throw "Cannot set to true";
    } else {
      *dest = val;
    }
  }

  static void setSignalValue(TConfiguration *inst, char *value, char **dest) {
    if (inst->isLoaded &&
        ((value != NULL) && (*dest != NULL) && (strlen(value) > 3) &&
         strcmp(value + 3, *dest) != 0)) {
      throw "Cannot change signal value";
    } else {
      inst->ReadSignalValue(value, dest);
    }
  }

  static void setSnmpTarget(TConfiguration *inst, char *val, char **dest) {
    if (inst->isLoaded &&
        ((val != NULL) && (*dest != NULL) && (strcmp(val, *dest) != 0))) {
      throw "Cannot set snmp_target";
    } else {
      inst->ReadStringValue(inst, val, dest);
    }
  }

  static void setSnmpComName(TConfiguration *inst, char *val, char **dest) {
    if (inst->isLoaded &&
        ((val != NULL) && (*dest != NULL) && (strcmp(val, *dest) != 0))) {
      throw "Cannot set snmp_comname";
    } else {
      if (*dest != NULL) {
        free(*dest);
      }

      /* If set empty value to community name. */
      *dest = (strcmp(val, "(NULL)") == 0) ? strdup("") : strdup(val);
    }
  }
};

#endif  // CONFIGURATION_HPP

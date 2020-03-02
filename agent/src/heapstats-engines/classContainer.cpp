/*!
 * \file classContainer.cpp
 * \brief This file is used to add up using size every class.
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

#include <fcntl.h>

#include "globals.hpp"
#include "vmFunctions.hpp"
#include "classContainer.hpp"

/*!
 * \brief SNMP variable Identifier of raise heap-alert date.
 */
static oid OID_ALERT_DATE[] = {SNMP_OID_HEAPALERT, 1};
/*!
 * \brief SNMP variable Identifier of class-name is cause of heap-alert.
 */
static oid OID_ALERT_CLS_NAME[] = {SNMP_OID_HEAPALERT, 2};
/*!
 * \brief SNMP variable Identifier of kind of class-size.
 */
static oid OID_ALERT_SIZE_KIND[] = {SNMP_OID_HEAPALERT, 3};
/*!
 * \brief SNMP variable Identifier of class-size.
 */
static oid OID_ALERT_CLS_SIZE[] = {SNMP_OID_HEAPALERT, 4};
/*!
 * \brief SNMP variable Identifier of instance count.
 */
static oid OID_ALERT_CLS_COUNT[] = {SNMP_OID_HEAPALERT, 5};

/*!
 * \brief SNMP variable Identifier of raise Java heap alert date.
 */
static oid OID_JAVAHEAPALERT_DATE[] = {SNMP_OID_JAVAHEAPALERT, 1};
/*!
 * \brief SNMP variable Identifier of Java heap usage.
 */
static oid OID_JAVAHEAPALERT_USAGE[] = {SNMP_OID_JAVAHEAPALERT, 2};
/*!
 * \brief SNMP variable Identifier of Java heap max capacity.
 */
static oid OID_JAVAHEAPALERT_MAX_CAPACITY[] = {SNMP_OID_JAVAHEAPALERT, 3};

/*!
 * \brief SNMP variable Identifier of raise metaspace alert date.
 */
static oid OID_METASPACEALERT_DATE[] = {SNMP_OID_METASPACEALERT, 1};
/*!
 * \brief SNMP variable Identifier of Java metaspace usage.
 */
static oid OID_METASPACEALERT_USAGE[] = {SNMP_OID_METASPACEALERT, 2};
/*!
 * \brief SNMP variable Identifier of Java heap max capacity.
 */
static oid OID_METASPACEALERT_MAX_CAPACITY[] = {SNMP_OID_METASPACEALERT, 3};

/*!
 * \brief String of USAGE order.
 */
static char ORDER_USAGE[6] = "USAGE";

static TClassInfoSet unloadedList;

/*!
 * \briefString of DELTA order.
 */
static char ORDER_DELTA[6] = "DELTA";

/*!
 * \brief TClassContainer constructor.
 */
TClassContainer::TClassContainer(void) : classMap(), updatedClassList() {
  /* Create trap sender. */
  pSender = conf->SnmpSend()->get() ? new TTrapSender() : NULL;
}

/*!
 * \brief TClassContainer destructor.
 */
TClassContainer::~TClassContainer(void) {
  /*
   * Cleanup class information.
   * clear() is thread unsafe function, but this d'tor would not be called concurrently.
   */
  classMap.clear();

  /* Cleanup instances. */
  delete pSender;
}

/*!
 * \brief Append new-class to container.
 * \param klassOop [in] New class oop.
 * \return New-class data.
 */
std::shared_ptr<TObjectDataA> TClassContainer::pushNewClass(void *klassOop) {
  TObjectDataA *cur = NULL;

  /* Class info setting. */

  try {
    cur = new TObjectDataA(klassOop);
  } catch (...) {
    logger->printWarnMsg("Couldn't allocate new TObjectData for %p!", klassOop);
    return NULL;
  }

  std::shared_ptr<TObjectDataA> cur_ptr(cur);

  void *clsLoader = getClassLoader(klassOop, cur_ptr->OopType());
  std::shared_ptr<TObjectDataA> clsLoaderData;
  /* If class loader isn't system bootstrap class loader. */
  if (clsLoader != NULL) {
    void *clsLoaderKlsOop = getKlassOopFromOop(clsLoader);
    if (clsLoaderKlsOop != NULL) {
      /* Search classloader's class. */
      clsLoaderData = this->findClass(clsLoaderKlsOop);
      if (unlikely(clsLoaderData == NULL)) {
        /* Register classloader's class. */
        clsLoaderData = this->pushNewClass(clsLoaderKlsOop);
      }
    }
  }
  cur_ptr->setClassLoader(clsLoader, (clsLoaderData != NULL) ? clsLoaderData->Tag() : 0);

  // Return corresponding TObjectData.
  // If TObjectData which relates to klassOop is already exists,
  // returns existed TObjectData. Then cur_ptr and TObjectData
  // which is wrapped in it would be released automatically because
  // cur_ptr is shared_ptr and it is just refered at this point.
  return this->pushNewClass(klassOop, cur_ptr);
}

/*!
 * \brief Append new-class to container.
 * \param klassOop [in] New class oop.
 * \param objData  [in] Add new class data.
 * \return New-class data.<br />
 *         This value isn't equal param "objData",
 *         if already registered equivalence class.
 */
std::shared_ptr<TObjectDataA> TClassContainer::pushNewClass(void *klassOop,
                                                            std::shared_ptr<TObjectDataA> objData) {
  /*
   * Jvmti extension event "classUnload" is loose once in a while.
   * The event forget callback occasionally when class unloading.
   * So we need to check klassOop that was doubling.
   */
  TClassMap::accessor acc;
  if (!classMap.insert(acc, {klassOop, objData})) {
    std::shared_ptr<TObjectDataA> expectData = acc->second;
    if (likely(expectData != NULL)) {
      /* If adding class data for another class is already exists. */
      if (unlikely((expectData->ClassName() == NULL) ||
                   (strcmp(objData->ClassName(), expectData->ClassName()) != 0) ||
                   (objData->ClassLoaderId() != expectData->ClassLoaderId()))) {
        acc->second = objData;
        unloadedList.push(expectData);
      }
    }
  }

  return acc->second;
}

/*!
 * \brief Remove class from container.
 * \param target [in] Remove class data.
 */
void TClassContainer::removeClass(std::shared_ptr<TObjectDataA> target) {
  classMap.erase(target->KlassOop());
}

/*!
 * \brief Comparator for sort by usage order.
 * \param *arg1 [in] Compare target A.
 * \param *arg2 [in] Compare target B.
 * \return Compare result.
 */
int HeapUsageCmp(const void *arg1, const void *arg2) {
  jlong cmp = ((THeapDelta *)arg2)->usage - ((THeapDelta *)arg1)->usage;
  if (cmp > 0) {
    /* arg2 is bigger than arg1. */
    return -1;
  } else if (cmp < 0) {
    /* arg1 is bigger than arg2. */
    return 1;
  } else {
    /* arg2 is equal arg1. */
    return 0;
  }
}

/*!
 * \brief Comparator for sort by delta order.
 * \param *arg1 [in] Compare target A.
 * \param *arg2 [in] Compare target B.
 * \return Compare result.
 */
int HeapDeltaCmp(const void *arg1, const void *arg2) {
  jlong cmp = ((THeapDelta *)arg2)->delta - ((THeapDelta *)arg1)->delta;
  if (cmp > 0) {
    /* arg2 is bigger than arg1. */
    return -1;
  } else if (cmp < 0) {
    /* arg1 is bigger than arg2. */
    return 1;
  } else {
    /* arg2 is equal arg1. */
    return 0;
  }
}

/*!
 * \brief Output snapshot header information to file.
 * \param fd     [in] Target file descriptor.
 * \param header [in] Snapshot file information.
 * \return Value is zero, if process is succeed.<br />
 *         Value is error number a.k.a. "errno", if process is failure.
 */
inline int writeHeader(const int fd, TSnapShotFileHeader header) {
  int result = 0;
  try {
    /* Write header param before GC-cause. */
    if (unlikely(write(fd, &header, offsetof(TSnapShotFileHeader, gcCause)) <
                 0)) {
      throw 1;
    }

    /* Write GC-cause. */
    if (unlikely(write(fd, header.gcCause, header.gcCauseLen) < 0)) {
      throw 1;
    }

    /* Write header param after gccause. */
    if (unlikely(write(fd, &header.FGCCount,
                       sizeof(TSnapShotFileHeader) -
                           offsetof(TSnapShotFileHeader, FGCCount)) < 0)) {
      throw 1;
    }
  } catch (...) {
    result = errno;
  }
  return result;
}

/*!
 * \brief Send memory usage information by SNMP trap.
 * \param pSender       [in] SNMP trap sender.
 * \param type          [in] This alert type.
 * \param occurred_time [in] Datetime of this alert is occurred.
 * \param usage         [in] Java heap usage.
 * \param maxCapacity   [in] Max capacity of Java heap.
 * \return Process result.
 */
inline bool sendMemoryUsageAlertTrap(TTrapSender *pSender,
                                     TMemoryUsageAlertType type,
                                     jlong occurred_time, jlong usage,
                                     jlong maxCapacity) {
  /* Setting trap information. */
  char paramStr[256];
  const char *trapOID;
  oid *alert_date;
  oid *alert_usage;
  oid *alert_capacity;
  int oid_len = 0;

  switch (type) {
    case ALERT_JAVA_HEAP:
      trapOID = OID_JAVAHEAPALERT;
      alert_date = OID_JAVAHEAPALERT_DATE;
      alert_usage = OID_JAVAHEAPALERT_USAGE;
      alert_capacity = OID_JAVAHEAPALERT_MAX_CAPACITY;
      oid_len = OID_LENGTH(OID_JAVAHEAPALERT_DATE);
      break;

    case ALERT_METASPACE:
      trapOID = OID_METASPACEALERT;
      alert_date = OID_METASPACEALERT_DATE;
      alert_usage = OID_METASPACEALERT_USAGE;
      alert_capacity = OID_METASPACEALERT_MAX_CAPACITY;
      oid_len = OID_LENGTH(OID_METASPACEALERT_DATE);
      break;

    default:
      logger->printCritMsg("Unknown alert type!");
      return false;
  }

  bool result = true;

  try {
    /* Setting sysUpTime */
    pSender->setSysUpTime();

    /* Setting trapOID. */
    pSender->setTrapOID(trapOID);

    /* Set time of occurring this alert. */
    sprintf(paramStr, JLONG_FORMAT_STR, occurred_time);
    pSender->addValue(alert_date, oid_len, paramStr, SNMP_VAR_TYPE_COUNTER64);

    /* Set java heap usage. */
    sprintf(paramStr, JLONG_FORMAT_STR, usage);
    pSender->addValue(alert_usage, oid_len, paramStr, SNMP_VAR_TYPE_COUNTER64);

    /* Set max capacity of java heap. */
    sprintf(paramStr, JLONG_FORMAT_STR, maxCapacity);
    pSender->addValue(alert_capacity, oid_len, paramStr,
                      SNMP_VAR_TYPE_COUNTER64);

    /* Send trap. */
    if (unlikely(pSender->sendTrap() != SNMP_PROC_SUCCESS)) {
      /* Clean up. */
      pSender->clearValues();
      throw 1;
    }

  } catch (...) {
    result = false;
  }

  return result;
}

/*!
 * \brief Send class information by SNMP trap.
 * \param pSender       [in] SNMP trap sender.
 * \param heapUsage     [in] Heap usage information of sending target class.
 * \param className     [in] Name of sending target class(JNI format string).
 * \param instanceCount [in] Number of instance of sending target class.
 * \return Process result.
 */
inline bool sendHeapAlertTrap(TTrapSender *pSender, THeapDelta heapUsage,
                              char *className, jlong instanceCount) {
  /* Setting trap information. */
  char paramStr[256];
  /* Trap OID. */
  char trapOID[50] = OID_HEAPALERT;

  bool result = true;
  try {
    /* Setting sysUpTime */
    pSender->setSysUpTime();

    /* Setting trapOID. */
    pSender->setTrapOID(trapOID);

    if (conf->Order()->get() == USAGE) {
      /* Set size kind. */
      pSender->addValue(OID_ALERT_SIZE_KIND, OID_LENGTH(OID_ALERT_SIZE_KIND),
                        ORDER_USAGE, SNMP_VAR_TYPE_STRING);

      sprintf(paramStr, JLONG_FORMAT_STR, heapUsage.usage);
    } else {
      /* Set size kind. */
      pSender->addValue(OID_ALERT_SIZE_KIND, OID_LENGTH(OID_ALERT_SIZE_KIND),
                        ORDER_DELTA, SNMP_VAR_TYPE_STRING);

      sprintf(paramStr, JLONG_FORMAT_STR, heapUsage.delta);
    }

    /* Set alert size. */
    pSender->addValue(OID_ALERT_CLS_SIZE, OID_LENGTH(OID_ALERT_CLS_SIZE),
                      paramStr, SNMP_VAR_TYPE_COUNTER64);

    /* Set now time. */
    sprintf(paramStr, JLONG_FORMAT_STR, getNowTimeSec());
    pSender->addValue(OID_ALERT_DATE, OID_LENGTH(OID_ALERT_DATE), paramStr,
                      SNMP_VAR_TYPE_COUNTER64);

    /* Set class-name. */
    pSender->addValue(OID_ALERT_CLS_NAME, OID_LENGTH(OID_ALERT_CLS_NAME),
                      className, SNMP_VAR_TYPE_STRING);

    /* Set instance count. */
    sprintf(paramStr, JLONG_FORMAT_STR, instanceCount);
    pSender->addValue(OID_ALERT_CLS_COUNT, OID_LENGTH(OID_ALERT_CLS_COUNT),
                      paramStr, SNMP_VAR_TYPE_COUNTER64);

    /* Send trap. */
    if (unlikely(pSender->sendTrap() != SNMP_PROC_SUCCESS)) {
      /* Clean up. */
      pSender->clearValues();
      throw 1;
    }
  } catch (...) {
    result = false;
  }
  return result;
}

/*!
 * \brief Output class information to file.
 * \param fd      [in] Target file descriptor.
 * \param objData [in] The class information.
 * \param cur     [in] The class size counter.
 * \param snapshot[in] SnapShot container.
 * \return Value is zero, if process is succeed.<br />
 *         Value is error number a.k.a. "errno", if process is failure.
 */
inline int writeClassData(const int fd, std::shared_ptr<TObjectDataA> objData,
                          TClassCounter *cur, TSnapShotContainer *snapshot) {
  int result = 0;
  /* Output class-information. */
  try {
    /* Write TObjectData */
    if (unlikely(objData->writeObjectData(fd) == -1)) {
      throw 1;
    }

    /* Output class instance count and heap usage. */
    if (unlikely(write(fd, cur->counter, sizeof(TObjectCounter)) < 0)) {
      throw 1;
    }

    /* Output children-class-information. */
    if (conf->CollectRefTree()->get()) {
      TChildClassCounter *childCounter = cur->child;

      while (childCounter != NULL) {
        /* If do output child class. */
        if (likely(!conf->ReduceSnapShot()->get() ||
                   (childCounter->counter->total_size > 0))) {
          /* Output child class tag. */
          jlong childClsTag = reinterpret_cast<jlong>(childCounter->objData.get());
          if (unlikely(write(fd, &childClsTag, sizeof(jlong)) < 0)) {
            throw 1;
          }

          /* Output child class instance count and heap usage. */
          if (unlikely(
              write(fd, childCounter->counter, sizeof(TObjectCounter)) < 0)) {
            throw 1;
          }
        }

        /* Move next child class. */
        childCounter = childCounter->next;
      }

      /* Output end-marker of children-class-information. */
      const jlong childClsEndMarker[] = {-1, -1, -1};
      if (unlikely(
          write(fd, childClsEndMarker, sizeof(childClsEndMarker)) < 0)) {
        throw 1;
      }

    }

  } catch (...) {
    result = errno;
  }

  return result;
}

/*!
 * \brief Output all-class information to file.
 * \param snapshot [in]  Snapshot instance.
 * \param rank     [out] Sorted-class information.
 * \return Value is zero, if process is succeed.<br />
 *         Value is error number a.k.a. "errno", if process is failure.
 */
int TClassContainer::afterTakeSnapShot(TSnapShotContainer *snapshot,
                                       TSorter<THeapDelta> **rank) {
  /* Sanity check. */
  if (unlikely(snapshot == NULL || rank == NULL)) {
    return 0;
  }

  /* Copy header. */
  TSnapShotFileHeader hdr;
  memcpy(&hdr, (const void *)snapshot->getHeader(),
         sizeof(TSnapShotFileHeader));

  /* Set safepoint time. */
  hdr.safepointTime = jvmInfo->getSafepointTime();
  hdr.magicNumber |= EXTENDED_SAFEPOINT_TIME;

  /* If java heap usage alert is enable. */
  if (conf->getHeapAlertThreshold() > 0) {
    jlong usage = hdr.newAreaSize + hdr.oldAreaSize;

    if (usage > conf->getHeapAlertThreshold()) {
      /* Raise alert. */
      logger->printWarnMsg(
          "ALERT: Java heap usage exceeded the threshold (%ld MB)",
          usage / 1024 / 1024);

      /* If need send trap. */
      if (conf->SnmpSend()->get()) {
        if (unlikely(!sendMemoryUsageAlertTrap(pSender, ALERT_JAVA_HEAP,
                                               hdr.snapShotTime, usage,
                                               jvmInfo->getMaxMemory()))) {
          logger->printWarnMsg("SNMP trap send failed!");
        }
      }
    }
  }

  /* If metaspace usage alert is enable. */
  if ((conf->MetaspaceThreshold()->get() > 0) &&
      ((conf->MetaspaceThreshold()->get() * 1024 * 1024) <
       hdr.metaspaceUsage)) {
    const char *label = jvmInfo->isAfterCR6964458() ? "Metaspace" : "PermGen";

    /* Raise alert. */
    logger->printWarnMsg("ALERT: %s usage exceeded the threshold (%ld MB)",
                         label, hdr.metaspaceUsage / 1024 / 1024);

    /* If need send trap. */
    if (conf->SnmpSend()->get()) {
      if (unlikely(!sendMemoryUsageAlertTrap(
                       pSender, ALERT_METASPACE, hdr.snapShotTime,
                       hdr.metaspaceUsage, hdr.metaspaceCapacity))) {
        logger->printWarnMsg("SNMP trap send failed!");
      }
    }
  }

  /* Class map used snapshot output. */
  auto workClsMap(classMap);

  /* Allocate return array. */
  jlong rankCnt = workClsMap.size();
  rankCnt =
      (rankCnt < conf->RankLevel()->get()) ? rankCnt : conf->RankLevel()->get();

  /* Make controller to sort. */
  register TRankOrder order = conf->Order()->get();
  TSorter<THeapDelta> *sortArray;
  try {
    sortArray = new TSorter<THeapDelta>(
        rankCnt,
        (TComparator)((order == DELTA) ? &HeapDeltaCmp : &HeapUsageCmp));
  } catch (...) {
    int raisedErrNum = errno;
    logger->printWarnMsgWithErrno("Couldn't allocate working memory!");
    return raisedErrNum;
  }

  /* Open file and seek EOF. */

  int fd = open(conf->FileName()->get(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
  /* If failure open file. */
  if (unlikely(fd < 0)) {
    int raisedErrNum = errno;
    logger->printWarnMsgWithErrno("Could not open %s", conf->FileName()->get());
    delete sortArray;
    return raisedErrNum;
  }

  off_t oldFileOffset = -1;
  try {
    /* Move position to EOF. */
    oldFileOffset = lseek(fd, 0, SEEK_END);
    /* If failure seek. */
    if (unlikely(oldFileOffset < 0)) {
      throw 1;
    }

    /* Frist, Output each classes information. Secondly output header. */
    if (unlikely(lseek(fd, sizeof(TSnapShotFileHeader) - sizeof(char[80]) +
                               hdr.gcCauseLen,
                       SEEK_CUR) < 0)) {
      throw 1;
    }
  } catch (...) {
    int raisedErrNum = errno;
    logger->printWarnMsg("Could not write snapshot");
    close(fd);
    delete sortArray;
    return raisedErrNum;
  }

  /* Output class information. */

  THeapDelta result;
  jlong numEntries = 0L;
  int raiseErrorCode = 0;
  register jlong AlertThreshold = conf->getAlertThreshold();

  /* Loop each class. */
  for (auto it = workClsMap.begin(); it != workClsMap.end(); it++) {
    std::shared_ptr<TObjectDataA> objData = it->second;
    TClassCounter *cur = snapshot->findClass(objData);
    /* If don't registed class yet. */
    if (unlikely(cur == NULL)) {
      cur = snapshot->pushNewClass(objData);
      if (unlikely(cur == NULL)) {
        raiseErrorCode = errno;
        logger->printWarnMsgWithErrno("Couldn't allocate working memory!");
        delete sortArray;
        sortArray = NULL;
        break;
      }
    }

    /* Calculate uasge and delta size. */
    result.usage = cur->counter->total_size;
    result.delta = cur->counter->total_size - objData->OldTotalSize();
    result.tag = objData->Tag();
    objData->SetOldTotalSize(result.usage);

    /* If do output class. */
    if (!conf->ReduceSnapShot()->get() || (result.usage > 0)) {
      /* Output class-information. */
      if (likely(raiseErrorCode == 0)) {
        raiseErrorCode = writeClassData(fd, objData, cur, snapshot);
      }

      numEntries++;
    }

    /* Ranking sort. */
    sortArray->push(result);

    /* If alert is enable. */
    if (AlertThreshold > 0) {
      /* Variable for send trap. */
      int sendFlag = 0;

      /* If size is bigger more limit size. */
      if ((order == DELTA) && (AlertThreshold <= result.delta)) {
        /* Raise alert. */
        logger->printWarnMsg(
            "ALERT(DELTA): \"%s\" exceeded the threshold (%ld bytes)",
            objData->ClassName(), result.delta);
        /* Set need send trap flag. */
        sendFlag = 1;
      } else if ((order == USAGE) && (AlertThreshold <= result.usage)) {
        /* Raise alert. */
        logger->printWarnMsg(
            "ALERT(USAGE): \"%s\" exceeded the threshold (%ld bytes)",
            objData->ClassName(), result.usage);
        /* Set need send trap flag. */
        sendFlag = 1;
      }

      /* If need send trap. */
      if (conf->SnmpSend()->get() && sendFlag != 0) {
        if (unlikely(!sendHeapAlertTrap(pSender, result, objData->ClassName(),
                                        cur->counter->count))) {
          logger->printWarnMsg("Send SNMP trap failed!");
        }
      }
    }
  }

  /* Set output entry count. */
  hdr.size = numEntries;
  /* Stored error number to avoid overwriting by "truncate" and etc.. */
  int raisedErrNum = 0;
  try {
    /* If already failed in processing to write snapshot. */
    if (unlikely(raiseErrorCode != 0)) {
      errno = raiseErrorCode;
      raisedErrNum = raiseErrorCode;
      throw 1;
    }

    /* If fail seeking to header position. */
    if (unlikely(lseek(fd, oldFileOffset, SEEK_SET) < 0)) {
      raisedErrNum = errno;
      throw 2;
    }

    raisedErrNum = writeHeader(fd, hdr);
    /* If failed to write a snapshot header. */
    if (unlikely(raisedErrNum != 0)) {
      throw 3;
    }
  } catch (...) {
    ; /* Failed to write file. */
  }

  /* Clean up. */
  if (unlikely(close(fd) != 0 && raisedErrNum == 0)) {
    errno = raisedErrNum;
    logger->printWarnMsgWithErrno("Could not write snapshot");
  }

  /* If need rollback snapshot. */
  if (unlikely(raisedErrNum != 0)) {
    if (unlikely(truncate(conf->FileName()->get(), oldFileOffset) < 0)) {
      logger->printWarnMsgWithErrno("Could not rollback snapshot");
    }
  }

  /* Cleanup. */
  (*rank) = sortArray;
  return raisedErrNum;
}

/*!
 * \brief Class unload event. Unloaded class will be added to unloaded list.
 * \param jvmti  [in] JVMTI environment object.
 * \param env    [in] JNI environment object.
 * \param thread [in] Java thread object.
 * \param klass  [in] Unload class object.
 * \sa    from: hotspot/src/share/vm/prims/jvmti.xml
 */
void JNICALL
    OnClassUnload(jvmtiEnv *jvmti, JNIEnv *env, jthread thread, jclass klass) {
  /*
   * This function does not require to check whether at safepoint because
   * Class Unloading always executes at safepoint.
   */

  /* Get klassOop. */
  void *mirror = *(void **)klass;
  void *klassOop = TVMFunctions::getInstance()->AsKlassOop(mirror);

  if (likely(klassOop != NULL)) {
    /* Search class. */
    std::shared_ptr<TObjectDataA> counter = clsContainer->findClass(klassOop);
    if (likely(counter != NULL)) {
      /*
       * This function will be called at safepoint and be called by VMThread
       * because class unloading is single-threaded process.
       * So we can lock-free access.
       */
      unloadedList.push(counter);
    }
  }
}

/*!
 * \brief GarbageCollectionFinish JVMTI event to release memory for unloaded
 *        TObjectData.
 *        This function will be called at safepoint.
 *        All GC worker and JVMTI agent threads for HeapStats will not work
 *        at this point.
 *        So we can lock-free access.
 */
void JNICALL OnGarbageCollectionFinishForUnload(jvmtiEnv *jvmti) {
  if (!unloadedList.empty()) {
    /* Remove targets from snapshot container. */
    TSnapShotContainer::removeObjectDataFromAllSnapShots(unloadedList);

    /* Remove targets from class container. */
    std::shared_ptr<TObjectDataA> objData;
    while (unloadedList.try_pop(objData)) {
      clsContainer->removeClass(objData);
      objData.reset();
    }
  }

  /* Clear updated data */
  clsContainer->removeBeforeUpdatedData();
}


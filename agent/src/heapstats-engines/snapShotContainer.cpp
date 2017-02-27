/*!
 * \file snapshotContainer.cpp
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

#include "globals.hpp"
#include "snapShotContainer.hpp"

/*!
 * \brief Pthread mutex for instance control.<br>
 * <br>
 * This mutex used in below process.<br>
 *   - TSnapShotContainer::getInstance @ snapShotContainer.cpp<br>
 *     To get older snapShotContainer instance from stockQueue.<br>
 *   - TSnapShotContainer::releaseInstance @ snapShotContainer.cpp<br>
 *     To add used snapShotContainer instance to stockQueue.<br>
 */
pthread_mutex_t TSnapShotContainer::instanceLocker =
    PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;

/*!
 * \brief Snapshot container instance stock queue.
 */
TSnapShotQueue *TSnapShotContainer::stockQueue = NULL;

/*!
 * \brief Set of active TSnapShotContainer set
 */
TActiveSnapShots TSnapShotContainer::activeSnapShots;

/*!
 * \brief Initialize snapshot caontainer class.
 * \return Is process succeed.
 * \warning Please call only once from main thread.
 */
bool TSnapShotContainer::globalInitialize(void) {
  try {
    /* Create snapshot container storage. */
    stockQueue = new TSnapShotQueue();
  } catch (...) {
    logger->printWarnMsg("Failure initialize snapshot container.");
    return false;
  }

  return true;
}

/*!
 * \brief Finalize snapshot caontainer class.
 * \warning Please call only once from main thread.
 */
void TSnapShotContainer::globalFinalize(void) {
  if (likely(stockQueue != NULL)) {
    /* Clear snapshot in queue. */
    while (!stockQueue->empty()) {
      TSnapShotContainer *item = stockQueue->front();

      /* Deallocate snapshot instance. */
      delete item;
      stockQueue->pop();
    }

    /* Deallocate stock queue. */
    delete stockQueue;
    stockQueue = NULL;
  }
}

/*!
 * \brief Get snapshot container instance.
 * \return Snapshot container instance.
 * \warning Don't deallocate instance getting by this function.<br>
 *          Please call "releaseInstance" method.
 */
TSnapShotContainer *TSnapShotContainer::getInstance(void) {
  TSnapShotContainer *result = NULL;

  ENTER_PTHREAD_SECTION(&instanceLocker) {
    if (!stockQueue->empty()) {
      /* Reuse snapshot container instance. */
      result = stockQueue->front();
      stockQueue->pop();
    }

    /* If need create new instance. */
    if (result == NULL) {
      /* Create new snapshot container instance. */
      try {
        result = new TSnapShotContainer();
        activeSnapShots.insert(result);
      } catch (...) {
        result = NULL;
      }
    }
  }
  EXIT_PTHREAD_SECTION(&instanceLocker)

  return result;
}

/*!
 * \brief Release snapshot container instance..
 * \param instance [in] Snapshot container instance.
 * \warning Don't access instance after called this function.
 */
void TSnapShotContainer::releaseInstance(TSnapShotContainer *instance) {
  /* Sanity check. */
  if (unlikely(instance == NULL)) {
    return;
  }

  bool existStockSpace = false;
  ENTER_PTHREAD_SECTION(&instanceLocker) {
    existStockSpace = (stockQueue->size() < MAX_STOCK_COUNT);
  }
  EXIT_PTHREAD_SECTION(&instanceLocker)

  if (likely(existStockSpace)) {
    /*
     * We reset this flag.
     * Because we need deallocating if failed to store to stock.
     * E.g. Failed to get mutex at "pthread_mutex_lock/unlock"
     *      or no more memory at "std::queue<T>::push()".
     */
    existStockSpace = false;

    /* Clear data. */
    instance->clear(false);

    ENTER_PTHREAD_SECTION(&instanceLocker) {
      try {
        /* Store instance. */
        stockQueue->push(instance);

        existStockSpace = true;
      } catch (...) {
        /* Maybe faield to allocate memory. So we release instance. */
      }
    }
    EXIT_PTHREAD_SECTION(&instanceLocker)
  }

  ENTER_PTHREAD_SECTION(&instanceLocker)
  {
    if (unlikely(!existStockSpace)) {
      /* Deallocate instance. */
      activeSnapShots.erase(instance);
      delete instance;
    }
  }
  EXIT_PTHREAD_SECTION(&instanceLocker)
}

/*!
 * \brief TSnapshotContainer constructor.
 */
TSnapShotContainer::TSnapShotContainer(bool isParent)
    : counterMap(), containerMap() {
  /* Header setting. */
  this->_header.magicNumber = conf->CollectRefTree()->get()
                                ? EXTENDED_REFTREE_SNAPSHOT
                                : EXTENDED_SNAPSHOT;
  this->_header.byteOrderMark = BOM;
  this->_header.snapShotTime = 0;
  this->_header.size = 0;
  memset((void *)&this->_header.gcCause[0], 0, 80);

  /* Initialize each field. */
  lockval = 0;
  isParentContainer = isParent;

  /* Create thread storage key. */
  if (unlikely(isParent &&
               pthread_key_create(&snapShotContainerKey, NULL) != 0)) {
    throw "Failed to create pthread key";
  }

  this->isCleared = true;
}

/*!
 * \brief TSnapshotContainer destructor.
 */
TSnapShotContainer::~TSnapShotContainer(void) {
  /* Cleanup elements on counter map. */
  for (TSizeMap::iterator it = counterMap.begin(); it != counterMap.end();
       ++it) {
    TClassCounter *clsCounter = (*it).second;
    if (unlikely(clsCounter == NULL)) {
      continue;
    }

    /* Deallocate field block cache. */
    free(clsCounter->offsets);

    /* Deallocate children class list. */
    TChildClassCounter *counter = clsCounter->child;
    while (counter != NULL) {
      TChildClassCounter *aCounter = counter;
      counter = counter->next;

      /* Deallocate TChildClassCounter. */
      free(aCounter->counter);
      free(aCounter);
    }

    /* Deallocate TClassCounter. */
    free(clsCounter->counter);
    free(clsCounter);
  }

  /* Cleanup elements on snapshot container map. */
  for (TLocalSnapShotContainer::iterator it = containerMap.begin();
       it != containerMap.end(); ++it) {
    delete (*it).second;
  }

  /* Clean maps. */
  counterMap.clear();
  containerMap.clear();

  if (isParentContainer) {
    /* Clean thread storage key. */
    pthread_key_delete(snapShotContainerKey);
  }
}

/*!
 * \brief Append new-class to container.
 * \param objData [in] New-class key object.
 * \return New-class data.
 */
TClassCounter *TSnapShotContainer::pushNewClass(TObjectData *objData) {
  TClassCounter *cur = NULL;

  cur = (TClassCounter *)calloc(1, sizeof(TClassCounter));
  /* If failure allocate counter data. */
  if (unlikely(cur == NULL)) {
    /* Adding empty to list is deny. */
    logger->printWarnMsg("Couldn't allocate counter memory!");
    return NULL;
  }
  cur->offsetCount = -1;

  int ret = posix_memalign(
      (void **)&cur->counter, 16,
      sizeof(TObjectCounter) /* sizeof(TObjectCounter) == 16. */);
  /* If failure allocate counter. */
  if (unlikely(ret != 0)) {
    /* Adding empty to list is deny. */
    logger->printWarnMsg("Couldn't allocate counter memory!");
    free(cur);
    return NULL;
  }

  this->clearObjectCounter(cur->counter);

  try {
    /* Set counter map. */
    counterMap[objData] = cur;
  } catch (...) {
    /*
     * Maybe failed to allocate memory at "std::map::operator[]".
     */
    free(cur->counter);
    free(cur);
    cur = NULL;
  }

  return cur;
}

/*!
 * \brief Append new-child-class to container.
 * \param clsCounter [in] Parent class counter object.
 * \param objData    [in] New-child-class key object.
 * \return New-class data.
 */
TChildClassCounter *TSnapShotContainer::pushNewChildClass(
    TClassCounter *clsCounter, TObjectData *objData) {
  TChildClassCounter *newCounter =
      (TChildClassCounter *)calloc(1, sizeof(TChildClassCounter));
  /* If failure allocate child class counter data. */
  if (unlikely(newCounter == NULL)) {
    return NULL;
  }

  int ret = posix_memalign(
      (void **)&newCounter->counter, 16,
      sizeof(TObjectCounter) /* sizeof(TObjectCounter) == 16. */);
  /* If failure allocate child class counter. */
  if (unlikely(ret != 0)) {
    free(newCounter);
    return NULL;
  }

  this->clearObjectCounter(newCounter->counter);
  newCounter->objData = objData;

  /* Chain children list. */
  TChildClassCounter *counter = clsCounter->child;
  if (unlikely(counter == NULL)) {
    clsCounter->child = newCounter;
  } else {
    /* Get last counter. */
    while (counter->next != NULL) {
      counter = counter->next;
    }
    counter->next = newCounter;
  }

  return newCounter;
}

/*!
 * \brief Set JVM performance info to header.
 * \param info [in] JVM running performance information.
 */
void TSnapShotContainer::setJvmInfo(TJvmInfo *info) {
  /* Sanity check. */
  if (unlikely(info == NULL)) {
    logger->printWarnMsg("Couldn't get GC Information!");
    return;
  }

  /* If GC cause is need. */
  if (this->_header.cause == GC) {
    /* Copy GC cause. */
    strcpy((char *)this->_header.gcCause, info->getGCCause());
    this->_header.gcCauseLen = strlen((char *)this->_header.gcCause);

    /* Setting GC work time. */
    this->_header.gcWorktime = info->getGCWorktime();
  } else {
    /* Clear GC cause. */
    this->_header.gcCauseLen = 1;
    this->_header.gcCause[0] = '\0';

    /* GC no work. */
    this->_header.gcWorktime = 0;
  }

  /* Setting header info from TJvmInfo.
   * Total memory (JVM_TotalMemory) should be called from outside of GC.
   * Comment of VM_ENTRY_BASE macro says as following:
   *   ENTRY routines may lock, GC and throw exceptions
   * So we set TSnapShotFileHeader.totalHeapSize in TakeSnapShot() .
   */
  this->_header.FGCCount = info->getFGCCount();
  this->_header.YGCCount = info->getYGCCount();
  this->_header.newAreaSize = info->getNewAreaSize();
  this->_header.oldAreaSize = info->getOldAreaSize();
  this->_header.metaspaceUsage = info->getMetaspaceUsage();
  this->_header.metaspaceCapacity = info->getMetaspaceCapacity();
}

/*!
 * \brief Clear snapshot data.
 */
void TSnapShotContainer::clear(bool isForce) {
  if (!isForce && this->isCleared) {
    return;
  }

  /* Get snapshot container's spin lock. */
  spinLockWait(&lockval);
  {
    /* Clean heap usage information. */
    for (TSizeMap::iterator it = counterMap.begin(); it != counterMap.end();
         ++it) {
      TClassCounter *clsCounter = (*it).second;
      if (unlikely(clsCounter == NULL)) {
        continue;
      }

      /* Deallocate field block cache. */
      free(clsCounter->offsets);
      clsCounter->offsets = NULL;
      clsCounter->offsetCount = -1;

      /* Reset counters. */
      this->clearChildClassCounters(clsCounter);
    }

    /* Clean local snapshots. */
    for (TLocalSnapShotContainer::iterator it = containerMap.begin();
         it != containerMap.end(); ++it) {
      (*it).second->clear(true);
    }

    this->isCleared = true;
  }
  /* Release snapshot container's spin lock. */
  spinLockRelease(&lockval);
}

/*!
 * \brief Output GC statistics information.
 */
void TSnapShotContainer::printGCInfo(void) {
  logger->printInfoMsg("GC Statistics Information:");

  /* GC cause and GC worktime show only raised GC. */
  if (this->_header.cause == GC) {
    logger->printInfoMsg(
        "GC Cause: %s,  GC Worktime: " JLONG_FORMAT_STR " msec",
        (char *)this->_header.gcCause, this->_header.gcWorktime);
  }

  /* Output GC count. */
  logger->printInfoMsg("GC Count:  FullGC: " JLONG_FORMAT_STR
                       " / Young GC: " JLONG_FORMAT_STR,
                       this->_header.FGCCount, this->_header.YGCCount);

  /* Output heap size status. */
  logger->printInfoMsg("Area using size:  New: " JLONG_FORMAT_STR
                       " bytes"
                       " / Old: " JLONG_FORMAT_STR
                       " bytes"
                       " / Total: " JLONG_FORMAT_STR " bytes",
                       this->_header.newAreaSize, this->_header.oldAreaSize,
                       this->_header.totalHeapSize);

  /* Output metaspace size status. */
  const char *label =
      jvmInfo->isAfterCR6964458() ? "Metaspace usage: " : "PermGen usage: ";
  logger->printInfoMsg("%s " JLONG_FORMAT_STR
                       " bytes"
                       ", capacity: " JLONG_FORMAT_STR "  bytes",
                       label, this->_header.metaspaceUsage,
                       this->_header.metaspaceCapacity);
}

/*!
 * \brief Merge children data.
 */
void TSnapShotContainer::mergeChildren(void) {
  /* Get snapshot container's spin lock. */
  spinLockWait(&lockval);
  {
    /* Loop each local snapshot container. */
    for (TLocalSnapShotContainer::iterator it = this->containerMap.begin();
         it != this->containerMap.end(); it++) {
      /* Loop each class in snapshot container. */
      TSizeMap *srcCounterMap = &(*it).second->counterMap;
      for (TSizeMap::iterator it2 = srcCounterMap->begin();
           it2 != srcCounterMap->end(); it2++) {
        TClassCounter *srcClsCounter = (*it2).second;

        /* Search or register class. */
        TClassCounter *clsCounter = this->findClass((*it2).first);
        if (unlikely(clsCounter == NULL)) {
          clsCounter = this->pushNewClass((*it2).first);

          /* If failed to search and register class. */
          if (unlikely(clsCounter == NULL)) {
            continue; /* Skip merge this class. */
          }
        }

        /* Marge class heap usage. */
        this->addInc(clsCounter->counter, srcClsCounter->counter);

        /* Loop each children class. */
        TChildClassCounter *counter = srcClsCounter->child;
        while (counter != NULL) {
          TObjectData *objData = counter->objData;

          /* Search child class. */
          TChildClassCounter *childClsData =
                      this->findChildClass(clsCounter, objData->klassOop);

          /* Register class as child class. */
          if (unlikely(childClsData == NULL)) {
            childClsData = this->pushNewChildClass(clsCounter, objData);
          }

          if (likely(childClsData != NULL)) {
            /* Marge children class heap usage. */
            this->addInc(childClsData->counter, counter->counter);
          }

          counter = counter->next;
        }
      }
    }
  }
  /* Release snapshot container's spin lock. */
  spinLockRelease(&lockval);
}

/*!
 * \brief Remove unloaded TObjectData in this snapshot container.
 *        This function should be called at safepoint.
 * \param unloadedList Set of unloaded TObjectData.
 */
void TSnapShotContainer::removeObjectData(TClassInfoSet &unloadedList) {
  TSizeMap::iterator itr;

  /* Remove the target from parent container. */
  for (TClassInfoSet::iterator target = unloadedList.begin();
       target != unloadedList.end(); target++) {
    itr = counterMap.find(*target);
    if (itr != counterMap.end()) {
      TClassCounter *clsCounter = itr->second;
      TChildClassCounter *childCounter = clsCounter->child;

      while (childCounter != NULL) {
        TChildClassCounter *nextCounter = childCounter->next;
        free(childCounter->counter);
        free(childCounter);
        childCounter = nextCounter;
      }

      free(clsCounter->counter);
      free(clsCounter);
      counterMap.erase(itr);
    }
  }

  /* Remove the target from all children in counterMap. */
  for (itr = counterMap.begin(); itr != counterMap.end(); itr++) {
    TClassCounter *clsCounter = itr->second;
    TChildClassCounter *childCounter = clsCounter->child;
    TChildClassCounter *prevChildCounter = NULL;

    while (childCounter != NULL) {
      TChildClassCounter *nextCounter = childCounter->next;

      if (unloadedList.find(childCounter->objData) != unloadedList.end()) {
        free(childCounter->counter);
        free(childCounter);
        if (prevChildCounter == NULL) {
          clsCounter->child = nextCounter;
        } else {
          prevChildCounter->next = nextCounter;
        }
      } else {
        prevChildCounter = childCounter;
      }

      childCounter = nextCounter;
    }
  }

  /* Remove the target from local containers. */
  for (TLocalSnapShotContainer::iterator container = containerMap.begin();
       container != containerMap.end(); container++) {
    container->second->removeObjectData(unloadedList);
  }
}

/*!
 * \brief Remove unloaded TObjectData all active snapshot container.
 * \param unloadedList Set of unloaded TObjectData.
 */
void TSnapShotContainer::removeObjectDataFromAllSnapShots(
                                                 TClassInfoSet &unloadedList) {
  ENTER_PTHREAD_SECTION(&instanceLocker)
  {
    for (TActiveSnapShots::iterator itr = activeSnapShots.begin();
         itr != activeSnapShots.end(); itr++) {
      (*itr)->removeObjectData(unloadedList);
    }
  }
  EXIT_PTHREAD_SECTION(&instanceLocker)
}


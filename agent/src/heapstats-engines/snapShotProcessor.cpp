/*!
 * \file snapShotProcessor.cpp
 * \brief This file is used to output ranking and call snapshot function.
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
#include "elapsedTimer.hpp"
#include "fsUtil.hpp"
#include "snapShotProcessor.hpp"

/*!
 * \brief TSnapShotProcessor constructor.
 * \param clsContainer [in] Class container object.
 * \param info         [in] JVM running performance information.
 */
TSnapShotProcessor::TSnapShotProcessor(TClassContainer *clsContainer,
                                       TJvmInfo *info)
    : TAgentThread("HeapStats SnapShot Processor"), snapQueue() {
  /* Sanity check. */
  if (clsContainer == NULL) {
    throw "TClassContainer is NULL.";
  }

  if (info == NULL) {
    throw "TJvmInfo is NULL.";
  }

  /* Setting parameter. */
  this->_container = clsContainer;
  this->jvmInfo = info;
}

/*!
 * \brief TSnapShotProcessor destructor.
 */
TSnapShotProcessor::~TSnapShotProcessor(void) { /* Do Nothing. */ }

/*!
 * \brief Parallel work function by JThread.
 * \param jvmti [in] JVMTI environment information.
 * \param jni   [in] JNI environment information.
 * \param data  [in] Monitor-object for class-container.
 */
void JNICALL
    TSnapShotProcessor::entryPoint(jvmtiEnv *jvmti, JNIEnv *jni, void *data) {
  /* Ranking pointer. */
  TSorter<THeapDelta> *ranking = NULL;

  /* Get self. */
  TSnapShotProcessor *controller = (TSnapShotProcessor *)data;
  /* Change running state. */
  controller->_isRunning = true;

  bool existRemainder = false;
  /* Loop for agent run or remaining work exist. */
  while (!controller->_terminateRequest || existRemainder) {
    TSnapShotContainer *snapshot = NULL;
    /* Is notify flag. */
    bool needProcess = false;

    ENTER_PTHREAD_SECTION(&controller->mutex) {
      if (likely(controller->_numRequests == 0)) {
        /* Wait for notification or termination. */
        pthread_cond_wait(&controller->mutexCond, &controller->mutex);
      }

      /* If get notification means output. */
      if (likely(controller->_numRequests > 0)) {
        controller->_numRequests--;
        needProcess = true;
        controller->snapQueue.try_pop(snapshot);
      }

      /* Check remaining work. */
      existRemainder = (controller->_numRequests > 0);
    }
    EXIT_PTHREAD_SECTION(&controller->mutex)

    /* If waiting is finished by notification. */
    if (needProcess && (snapshot != NULL)) {
      int result = 0;
      {
        /* Count working time. */
        static const char *label = "Write SnapShot and calculation";
        TElapsedTimer elapsedTime(label);

        /* Output class-data. */
        result = controller->_container->afterTakeSnapShot(snapshot, &ranking);
      }

      /* If raise disk full error. */
      if (unlikely(isRaisedDiskFull(result))) {
        checkDiskFull(result, "snapshot");
      }

      /* Output snapshot infomartion. */
      snapshot->printGCInfo();

      /* If output failure. */
      if (likely(ranking != NULL)) {
        /* Show class ranking. */
        if (conf->RankLevel()->get() > 0) {
          controller->showRanking(snapshot->getHeader(), ranking);
        }
      }

      /* Clean up. */
      TSnapShotContainer::releaseInstance(snapshot);
      delete ranking;
      ranking = NULL;
    }
  }

  /* Change running state. */
  controller->_isRunning = false;
}

/*!
 * \brief Start parallel work by JThread.
 * \param jvmti [in] JVMTI environment information.
 * \param env   [in] JNI environment information.
 */
void TSnapShotProcessor::start(jvmtiEnv *jvmti, JNIEnv *env) {
  /* Start JThread. */
  TAgentThread::start(jvmti, env, TSnapShotProcessor::entryPoint, this,
                      JVMTI_THREAD_MIN_PRIORITY);
}

/*!
 * \brief Notify output snapshot to this thread from other thread.
 * \param snapshot [in] Output snapshot instance.
 */
void TSnapShotProcessor::notify(TSnapShotContainer *snapshot) {
  /* Sanity check. */
  if (unlikely(snapshot == NULL)) {
    return;
  }

  bool raiseException = true;
  /* Send notification and count notify. */
  ENTER_PTHREAD_SECTION(&this->mutex) {
    try {
      /* Store and count data. */
      snapQueue.push(snapshot);
      this->_numRequests++;

      /* Notify occurred deadlock. */
      pthread_cond_signal(&this->mutexCond);

      /* Reset exception flag. */
      raiseException = false;
    } catch (...) {
      /*
       * Maybe faield to allocate memory at "std:queue<T>::push()".
       * So we throw exception again at after release lock.
       */
    }
  }
  EXIT_PTHREAD_SECTION(&this->mutex)

  if (unlikely(raiseException)) {
    throw "Failed to TSnapShotProcessor notify";
  }
}

/*!
 * \brief Show ranking.
 * \param hdr  [in] Snapshot file information.
 * \param data [in] All class-data.
 */
void TSnapShotProcessor::showRanking(const TSnapShotFileHeader *hdr,
                                     TSorter<THeapDelta> *data) {
  /* Get now datetime. */
  char time_str[20];
  struct tm time_struct;

  /* snapshot time is "msec" */
  jlong snapDate = hdr->snapShotTime / 1000;
  /* "time_t" is defined as type has 8byte size in 32bit. */
  /* But jlong is defined as type has 16byte size in 32bit and 64bit. */
  /* So this part need cast-code for 32bit. */
  localtime_r((const time_t *)&snapDate, &time_struct);
  strftime(time_str, 20, "%F %T", &time_struct);

  /* Output ranking header. */
  switch (hdr->cause) {
    case GC:
      logger->printInfoMsg("Heap Ranking at %s (caused by GC, GCCause: %s)",
                           time_str, hdr->gcCause);
      break;

    case DataDumpRequest:
      logger->printInfoMsg("Heap Ranking at %s (caused by DataDumpRequest)",
                           time_str);
      break;

    case Interval:
      logger->printInfoMsg("Heap Ranking at %s (caused by Interval)");
      break;

    default:
      /* Illegal value. */
      logger->printInfoMsg("Heap Ranking at %s (caused by UNKNOWN)");
      logger->printWarnMsg("Illegal snapshot cause!");
      return;
  }

  /* Output ranking column. */
  logger->printInfoMsg("Rank    usage(byte)    increment(byte)  Class name");
  logger->printInfoMsg("----  ---------------  ---------------  ----------");

  /* Output high-rank class information. */
  int rankCnt = data->getCount();
  Node<THeapDelta> *aNode = data->lastNode();
  for (int Cnt = 0; Cnt < rankCnt && aNode != NULL;
       Cnt++, aNode = aNode->prev) {
/* Output header. */
#ifdef LP64
    logger->printInfoMsg("%4d  %15ld  %15ld  %s",
#else
    logger->printInfoMsg("%4d  %15lld  %15lld  %s",
#endif
                         Cnt + 1, aNode->value.usage, aNode->value.delta,
                         ((TObjectData *)aNode->value.tag)->className);
  }

  /* Clean up after ranking output. */
  logger->flush();
}

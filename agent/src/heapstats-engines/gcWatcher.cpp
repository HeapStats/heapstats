/*!
 * \file gcWatcher.cpp
 * \brief This file is used to take snapshot when finish garbage collection.
 * Copyright (C) 2011-2015 Nippon Telegraph and Telephone Corporation
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
#include "gcWatcher.hpp"

/*!
 * \brief TGCWatcher constructor.
 * \param postGCFunc [in] Callback is called after GC.
 * \param info       [in] JVM running performance information.
 */
TGCWatcher::TGCWatcher(TPostGCFunc postGCFunc, TJvmInfo *info)
    : TAgentThread("HeapStats GC Watcher") {
  /* Sanity check. */
  if (postGCFunc == NULL) {
    throw "Event callback is NULL.";
  }

  if (info == NULL) {
    throw "TJvmInfo is NULL.";
  }

  /* Setting parameter. */
  this->_postGCFunc = postGCFunc;
  this->pJvmInfo = info;
  this->_FGC = 0;
}

/*!
 * \brief TGCWatcher destructor.
 */
TGCWatcher::~TGCWatcher(void) { /* Do nothing. */ }

/*!
 * \brief JThread entry point.
 * \param jvmti [in] JVMTI environment object.
 * \param jni   [in] JNI environment object.
 * \param data  [in] Pointer of user data.
 */
void JNICALL TGCWatcher::entryPoint(jvmtiEnv *jvmti, JNIEnv *jni, void *data) {
  /* Get self. */
  TGCWatcher *controller = (TGCWatcher *)data;
  /* Change running state. */
  controller->_isRunning = true;

  /* Loop for agent run. */
  while (!controller->_terminateRequest) {
    /* Variable for notification flag. */
    bool needProcess = false;

    ENTER_PTHREAD_SECTION(&controller->mutex) {
      /* If no exists request. */
      if (likely(controller->_numRequests == 0)) {
        /* Wait for notification or termination. */
        pthread_cond_wait(&controller->mutexCond, &controller->mutex);
      }

      /* If waiting finished by notification. */
      if (likely(controller->_numRequests > 0)) {
        controller->_numRequests--;
        needProcess = true;
      }
    }
    EXIT_PTHREAD_SECTION(&controller->mutex)

    /* If waiting finished by notification. */
    if (needProcess) {
      /* Call event callback. */
      (*controller->_postGCFunc)(jvmti, jni, GC);
    }
  }

  /* Change running state. */
  controller->_isRunning = false;
}

/*!
 * \brief Make and begin Jthread.
 * \param jvmti [in] JVMTI environment object.
 * \param env   [in] JNI environment object.
 */
void TGCWatcher::start(jvmtiEnv *jvmti, JNIEnv *env) {
  /* Check JVM information. */
  if (this->pJvmInfo->getFGCCount() < 0) {
    logger->printWarnMsg("All GC accept as youngGC.");
  } else {
    /* Reset now full gc count. */
    this->_FGC = this->pJvmInfo->getFGCCount();
  }

  /* Start jThread. */
  TAgentThread::start(jvmti, env, TGCWatcher::entryPoint, this,
                      JVMTI_THREAD_MAX_PRIORITY);
}

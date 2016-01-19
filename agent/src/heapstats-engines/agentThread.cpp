/*!
 * \file agentThread.cpp
 * \brief This file is used to work on Jthread.
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
#include "util.hpp"
#include "agentThread.hpp"

/*!
 * \brief TAgentThread constructor.
 * \param name  [in] Thread name.
 */
TAgentThread::TAgentThread(const char *name) {
  /* Sanity check. */
  if (unlikely(name == NULL)) {
    throw "AgentThread name is illegal.";
  }

  /* Create mutex and condition. */
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&mutexCond, NULL);

  /* Initialization. */
  this->_numRequests = 0;
  this->_terminateRequest = false;
  this->_isRunning = false;
  this->threadName = strdup(name);
}

/*!
 * \brief TAgentThread destructor.
 */
TAgentThread::~TAgentThread(void) {
  /* Destroy mutex and condition. */
  pthread_cond_destroy(&mutexCond);
  pthread_mutex_destroy(&mutex);

  /* Cleanup. */
  free(this->threadName);
}

/*!
 * \brief Make and begin Jthread.
 * \param jvmti      [in] JVMTI environment object.
 * \param env        [in] JNI environment object.
 * \param entryPoint [in] Entry point for jThread.
 * \param conf       [in] Pointer of user data.
 * \param prio       [in] Jthread priority.
 */
void TAgentThread::start(jvmtiEnv *jvmti, JNIEnv *env,
                         TThreadEntryPoint entryPoint, void *conf, jint prio) {
  /* Sanity check. */
  if (this->_isRunning) {
    logger->printWarnMsg("AgentThread already started.");
    return;
  }

  /* Find jThread class. */
  jclass threadClass = env->FindClass("Ljava/lang/Thread;");
  if (unlikely(threadClass == NULL)) {
    throw "Couldn't get java thread class.";
  }

  /* Find jThread class's constructor. */
  jmethodID constructorID =
      env->GetMethodID(threadClass, "<init>", "(Ljava/lang/String;)V");
  if (unlikely(constructorID == NULL)) {
    throw "Couldn't get java thread class's constructor.";
  }

  /* Create new jThread name. */
  jstring jThreadName = env->NewStringUTF(threadName);
  if (unlikely(jThreadName == NULL)) {
    throw "Couldn't generate AgentThread name.";
  }

  /* Create new jThread object. */
  jthread thread = env->NewObject(threadClass, constructorID, jThreadName);
  if (unlikely(thread == NULL)) {
    throw "Couldn't generate AgentThread instance!";
  }

  /* Run thread. */
  if (isError(jvmti, jvmti->RunAgentThread(thread, entryPoint, conf, prio))) {
    throw "Couldn't start AgentThread!";
  }
}

/*!
 * \brief Notify timing to this thread from other thread.
 */
void TAgentThread::notify(void) {
  /* Send notification and count notify. */
  ENTER_PTHREAD_SECTION(&this->mutex) {
    this->_numRequests++;
    pthread_cond_signal(&this->mutexCond);
  }
  EXIT_PTHREAD_SECTION(&this->mutex)
}

/*!
 * \brief Notify stopping to this thread from other thread.
 */
void TAgentThread::stop(void) {
  /* Sanity check. */
  if (!this->_isRunning) {
    logger->printWarnMsg("AgentThread already finished.");
    return;
  }

  /* Send notification and count notify. */
  ENTER_PTHREAD_SECTION(&this->mutex) {
    this->_terminateRequest = true;
    pthread_cond_signal(&this->mutexCond);
  }
  EXIT_PTHREAD_SECTION(&this->mutex)

  /* SpinLock for AgentThread termination. */
  while (this->_isRunning) {
    ; /* none. */
  }

  /* Clean termination flag. */
  this->_terminateRequest = false;
}

/*!
 * \brief Notify termination to this thread from other thread.
 */
void TAgentThread::terminate(void) {
  /* Sanity check. */
  if (this->_isRunning) {
    stop();
  }
}

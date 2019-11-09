/*!
 * \file timer.cpp
 * \brief This file is used to take interval snapshot.
 * Copyright (C) 2011-2019 Nippon Telegraph and Telephone Corporation
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
#include "timer.hpp"

/*!
 * \brief TTimer constructor.
 * \param event     [in] Callback is used by interval calling.
 * \param timerName [in] Unique name of timer.
 */
TTimer::TTimer(TTimerEventFunc event, const char *timerName)
    : TAgentThread(timerName) {
  /* Sanity check. */
  if (event == NULL) {
    throw "Event callback is NULL.";
  }

  /* Setting param. */
  this->timerInterval = 0;
  this->_eventFunc = event;
  this->_isInterrupted = false;

  /* Create semphore. */
  if (unlikely(sem_init(&this->timerSem, 0, 0))) {
    throw "Couldn't create semphore.";
  }
}

/*!
 * \brief TTimer destructor.
 */
TTimer::~TTimer(void) {
  /* Destroy semphore. */
  sem_destroy(&this->timerSem);
}

/*!
 * \brief JThread entry point called by interval or nofity.
 * \param jvmti [in] JVMTI environment object.
 * \param jni   [in] JNI environment object.
 * \param data  [in] Pointer of TTimer.
 */
void JNICALL TTimer::entryPoint(jvmtiEnv *jvmti, JNIEnv *jni, void *data) {
  /* Get self. */
  TTimer *controller = (TTimer *)data;

  /* Create increment time. */
  struct timespec incTs = {0};
  incTs.tv_sec = controller->timerInterval / 1000;
  incTs.tv_nsec = (controller->timerInterval % 1000) * 1000;

  /* Change running state. */
  controller->_isRunning = true;

  /* Loop for agent run. */
  while (true) {
    /* Reset timer interrupt flag. */
    controller->_isInterrupted = false;

    {
      TMutexLocker locker(&controller->mutex);

      if (unlikely(controller->_terminateRequest)) {
        break;
      }

      /* Create limit datetime. */
      struct timespec limitTs = {0};
      struct timeval nowTv = {0};
      gettimeofday(&nowTv, NULL);
      TIMEVAL_TO_TIMESPEC(&nowTv, &limitTs);
      limitTs.tv_sec += incTs.tv_sec;
      limitTs.tv_nsec += incTs.tv_nsec;

      /* Wait for notification, termination or timeout. */
      pthread_cond_timedwait(&controller->mutexCond, &controller->mutex,
                             &limitTs);
    }

    /* If waiting finished by timeout. */
    if (!controller->_isInterrupted) {
      /* Call event callback. */
      (*controller->_eventFunc)(jvmti, jni, Interval);
    }
  }

  /* Change running state */
  controller->_isRunning = false;
}

/*!
 * \brief JThread entry point called by notify only.
 * \param jvmti [in] JVMTI environment object.
 * \param jni   [in] JNI environment object.
 * \param data  [in] Pointer of TTimer.
 */
void JNICALL
    TTimer::entryPointByCall(jvmtiEnv *jvmti, JNIEnv *jni, void *data) {
  /* Get self. */
  TTimer *controller = (TTimer *)data;

  /* Change running state. */
  controller->_isRunning = true;

  /* Loop for agent run. */
  while (!controller->_terminateRequest) {
    /* Reset timer interrupt flag. */
    controller->_isInterrupted = false;

    /* Wait notify or terminate. */
    sem_wait(&controller->timerSem);

    /* If waiting finished by timeout. */
    if (!controller->_isInterrupted) {
      /* Call event callback. */
      (*controller->_eventFunc)(jvmti, jni, Interval);
    }
  }

  /* Change running state */
  controller->_isRunning = false;
}

/*!
 * \brief Make and begin Jthread.
 * \param jvmti    [in] JVMTI environment object.
 * \param env      [in] JNI environment object.
 * \param interval [in] Interval of invoke timer event.
 */
void TTimer::start(jvmtiEnv *jvmti, JNIEnv *env, jlong interval) {
  /* Set timer interval. */
  this->timerInterval = interval;

  if (interval == 0) {
    /* Notify process mode. Using semaphore. */
    TAgentThread::start(jvmti, env, TTimer::entryPointByCall, this,
                        JVMTI_THREAD_MAX_PRIORITY);
  } else {
    /* Interval process mode. Using pthread monitor. */
    TAgentThread::start(jvmti, env, TTimer::entryPoint, this,
                        JVMTI_THREAD_MAX_PRIORITY);
  }
}

/*!
 * \brief Notify reset timing to this thread from other thread.
 */
void TTimer::notify(void) {
  if (this->timerInterval == 0) {
    /* Notify process to waiting thread. */
    sem_post(&this->timerSem);

  } else {
    /* Set interrupt flag and notify. */
    TMutexLocker locker(&this->mutex);

    this->_isInterrupted = true;
    pthread_cond_signal(&this->mutexCond);
  }
}

/*!
 * \brief Notify stop to this thread from other thread.
 */
void TTimer::stop(void) {
  /* Terminate follow entry callback type. */
  if (this->timerInterval == 0) {
    /* Sanity check. */
    if (!this->_isRunning) {
      logger->printWarnMsg("AgentThread already finished.");
      return;
    }

    /* Send notification and count notify. */
    this->_isInterrupted = true;
    this->_terminateRequest = true;
    sem_post(&this->timerSem);

    /* SpinLock for AgentThread termination. */
    while (this->_isRunning) {
      ; /* none. */
    }

    /* Clean termination flag. */
    this->_terminateRequest = false;
  } else {
    /* Set interrupt flag and notify termination. */
    this->_isInterrupted = true;
    TAgentThread::stop();
  }
}

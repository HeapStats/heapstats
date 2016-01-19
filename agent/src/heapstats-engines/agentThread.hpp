/*!
 * \file agentThread.hpp
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

#ifndef AGENT_THREAD_HPP
#define AGENT_THREAD_HPP

#include <jvmti.h>
#include <jni.h>

#include <pthread.h>

/*!
 * \brief Entry point function type for jThread.
 * \param jvmti    [in] JVMTI environment information.
 * \param name     [in] Raw Monitor name.
 * \param userData [in] Pointer of user data.
 */
typedef void(JNICALL *TThreadEntryPoint)(jvmtiEnv *jvmti, JNIEnv *jni,
                                         void *userData);

/*!
* \brief This class is base class for parallel work using by jthread.
*/
class TAgentThread {
 public:
  /*!
   * \brief TAgentThread constructor.
   * \param name  [in] Thread name.
   */
  TAgentThread(const char *name);
  /*!
   * \brief TAgentThread destructor.
   */
  virtual ~TAgentThread(void);

  /*!
   * \brief Make and begin Jthread.
   * \param jvmti      [in] JVMTI environment object.
   * \param env        [in] JNI environment object.
   * \param entryPoint [in] Entry point for jThread.
   * \param conf       [in] Pointer of user data.
   * \param prio       [in] Jthread priority.
   */
  virtual void start(jvmtiEnv *jvmti, JNIEnv *env, TThreadEntryPoint entryPoint,
                     void *conf, jint prio);
  /*!
   * \brief Notify timing to this thread from other thread.
   */
  virtual void notify(void);
  /*!
   * \brief Notify stopping to this thread from other thread.
   */
  virtual void stop(void);
  /*!
   * \brief Notify termination to this thread from other thread.
   */
  virtual void terminate(void);

 protected:
  /*!
   * \brief Number of request.
   */
  volatile int _numRequests;
  /*!
   * \brief Flag of exists termination request.
   */
  volatile bool _terminateRequest;
  /*!
   * \brief Flag of jThread flag.
   */
  volatile bool _isRunning;
  /*!
   * \brief Pthread mutex for thread.
   */
  pthread_mutex_t mutex;
  /*!
   * \brief Pthread mutex condition for thread.
   */
  pthread_cond_t mutexCond;
  /*!
   * \brief Thread name for java thread.
   */
  char *threadName;
};

#endif

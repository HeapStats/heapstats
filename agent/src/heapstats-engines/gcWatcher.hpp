/*!
 * \file gcWatcher.hpp
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

#ifndef GCWATCHER_HPP
#define GCWATCHER_HPP

#include <jvmti.h>
#include <jni.h>

#include "util.hpp"
#include "jvmInfo.hpp"
#include "agentThread.hpp"

/*!
 * \brief This type is callback to notify finished gc by gcWatcher.
 * \param jvmti [in] JVMTI environment object.
 * \param env   [in] JNI environment object.
 * \param cause [in] Cause of invoke function.<br>Maybe "GC".
 */
typedef void (*TPostGCFunc)(jvmtiEnv *jvmti, JNIEnv *env, TInvokeCause cause);

/*!
 * \brief This class is used to take snapshot when finished GC.
 */
class TGCWatcher : public TAgentThread {
 public:
  /*!
   * \brief TGCWatcher constructor.
   * \param postGCFunc [in] Callback is called after GC.
   * \param info       [in] JVM running performance information.
   */
  TGCWatcher(TPostGCFunc postGCFunc, TJvmInfo *info);
  /*!
   * \brief TGCWatcher destructor.
   */
  virtual ~TGCWatcher(void);

  /*!
   * \brief What's gc kind.
   * \return is full-GC? (true/false)
   */
  inline bool needToStartGCTrigger(void) {
    bool isFullGC;
    /* Check full-GC count. */
    if (this->pJvmInfo->getFGCCount() > this->_FGC) {
      /* Refresh full GC count. */
      this->_FGC = this->pJvmInfo->getFGCCount();
      /* GC kind is full-GC. */
      isFullGC = true;
    } else {
      /* GC kind is young-GC. */
      isFullGC = false;
    }

    /* Set GC kind flag. */
    this->pJvmInfo->setFullGCFlag(isFullGC);
    return isFullGC;
  };

  /*!
   * \brief Make and begin Jthread.
   * \param jvmti [in] JVMTI environment object.
   * \param env   [in] JNI environment object.
   */
  void start(jvmtiEnv *jvmti, JNIEnv *env);

 protected:
  /*!
   * \brief JThread entry point.
   * \param jvmti [in] JVMTI environment object.
   * \param jni   [in] JNI environment object.
   * \param data  [in] Pointer of user data.
   */
  static void JNICALL entryPoint(jvmtiEnv *jvmti, JNIEnv *jni, void *data);

  RELEASE_ONLY(private :)
  /*!
   * \brief Treated full-GC count.
   */
  jlong _FGC;
  /*!
   * \brief Callback for notify finished gc by gcWatcher.
   */
  TPostGCFunc _postGCFunc;

  /*!
   * \brief JVM running performance information.
   */
  TJvmInfo *pJvmInfo;
};

#endif

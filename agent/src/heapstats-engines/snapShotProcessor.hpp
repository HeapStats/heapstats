/*!
 * \file snapShotProcessor.hpp
 * \brief This file is used to output ranking and call snapshot function.
 * Copyright (C) 2011-2016 Nippon Telegraph and Telephone Corporation
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

#ifndef SNAPSHOT_PROCESSOR_HPP
#define SNAPSHOT_PROCESSOR_HPP

#include <jvmti.h>
#include <jni.h>

#include "agentThread.hpp"
#include "snapShotContainer.hpp"
#include "classContainer.hpp"

/*!
 * \brief This class control take snapshot and show ranking.
 */
class TSnapShotProcessor : public TAgentThread {
 public:
  /*!
   * \brief TSnapShotProcessor constructor.
   * \param clsContainer [in] Class container object.
   * \param info         [in] JVM running performance information.
   */
  TSnapShotProcessor(TClassContainer *clsContainer, TJvmInfo *info);

  /*!
   * \brief TSnapShotProcessor destructor.
   */
  virtual ~TSnapShotProcessor(void);

  using TAgentThread::start;

  /*!
   * \brief Start parallel work by JThread.
   * \param jvmti [in] JVMTI environment information.
   * \param env   [in] JNI environment information.
   */
  void start(jvmtiEnv *jvmti, JNIEnv *env);

  using TAgentThread::notify;

  /*!
   * \brief Notify output snapshot to this thread from other thread.
   * \param snapshot [in] Output snapshot instance.
   */
  virtual void notify(TSnapShotContainer *snapshot);

 protected:
  /*!
   * \brief Parallel work function by JThread.
   * \param jvmti [in] JVMTI environment information.
   * \param jni   [in] JNI environment information.
   * \param data  [in] Monitor-object for class-container.
   */
  static void JNICALL entryPoint(jvmtiEnv *jvmti, JNIEnv *jni, void *data);

  /*!
   * \brief Show ranking.
   * \param hdr  [in] Snapshot file information.
   * \param data [in] All class-data.
   */
  virtual void showRanking(const TSnapShotFileHeader *hdr,
                           TSorter<THeapDelta> *data);

  RELEASE_ONLY(private :)
  /*!
   * \brief Class counter container.
   */
  TClassContainer *_container;

  /*!
   * \brief JVM running performance information.
   */
  TJvmInfo *jvmInfo;

  /*!
   * \brief Unprocessed snapshot queue.
   */
  TSnapShotQueue snapQueue;
};

#endif

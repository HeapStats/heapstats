/*!
 * \file classContainer.hpp
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

#ifndef CLASS_CONTAINER_HPP
#define CLASS_CONTAINER_HPP

#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>
#include <algorithm>

#include "sorter.hpp"
#include "trapSender.hpp"
#include "objectData.hpp"
#include "oopUtil.hpp"

#if PROCESSOR_ARCH == X86
#include "arch/x86/lock.inline.hpp"
#elif PROCESSOR_ARCH == ARM
#include "arch/arm/lock.inline.hpp"
#endif


/*!
 * \brief Forward declaration in snapShotContainer.hpp
 */
class TSnapShotContainer;

/*!
 * \brief This structure stored size of a class used in heap.
 */
typedef struct {
  jlong tag;   /*!< Pointer of TObjectData.                */
  jlong usage; /*!< Class using total size.                */
  jlong delta; /*!< Class delta size from before snapshot. */
} THeapDelta;

/*!
 * \brief Memory usage alert types.
 */
typedef enum { ALERT_JAVA_HEAP, ALERT_METASPACE } TMemoryUsageAlertType;

/*!
 * \brief Type is for unloaded class information.
 */
typedef tbb::concurrent_queue<TObjectData *> TClassInfoSet;

/*!
 * \brief Type is for storing class information.
 */
typedef tbb::concurrent_hash_map<PKlassOop, TObjectData *,
                                 TPointerHasher<PKlassOop> > TClassMap;
/*!
 * \brief This class is stored class information.<br>
 *        e.g. class-name, class instance count, size, etc...
 */
class TClassContainer {
 public:
  /*!
   * \brief TClassContainer constructor.
   */
  TClassContainer(void);
  /*!
   * \brief TClassContainer destructor.
   */
  virtual ~TClassContainer(void);

  /*!
   * \brief Append new-class to container.
   * \param klassOop [in] New class oop.
   * \return New-class data.
   */
  virtual TObjectData *pushNewClass(PKlassOop klassOop);

  /*!
   * \brief Append new-class to container.
   * \param klassOop [in] New class oop.
   * \param objData  [in] Add new class data.
   * \return New-class data.<br />
   *         This value isn't equal param "objData",
   *         if already registered equivalence class.
   */
  virtual TObjectData *pushNewClass(PKlassOop klassOop, TObjectData *objData);

  /*!
   * \brief Remove class from container.
   * \param target [in] Remove class data.
   */
  virtual void removeClass(TObjectData *target);

  /*!
   * \brief Search class from container.
   * \param klassOop [in] Target class oop.
   * \return Class data of target class.
   */
  inline TObjectData *findClass(PKlassOop klassOop) {
    TClassMap::const_accessor acc;
    return classMap.find(acc, klassOop) ? acc->second : NULL;
  }

  /*!
   * \brief Update class oop.
   * \param oldKlassOop [in] Target old class oop.
   * \param newKlassOop [in] Target new class oop.
   * \return Class data of target class.
   */
  inline void updateClass(PKlassOop oldKlassOop, PKlassOop newKlassOop) {
    TClassMap::const_accessor acc;
    if (classMap.find(acc, oldKlassOop)) {
      TObjectData *cur = acc->second;
      acc.release();

      cur->replaceKlassOop(newKlassOop);
      TClassMap::accessor new_acc;
      classMap.insert(new_acc, std::make_pair(newKlassOop, cur));
      new_acc.release();

      updatedClassList.push(oldKlassOop);
    }
  }

  /*!
   * \brief Get class entries count.
   * \return Entries count of class information.
   */
  inline size_t getContainerSize(void) {
    return classMap.size();
  }

  /*!
   * \brief Remove all-class from container.
   */
  virtual void allClear(void);
  /*!
   * \brief Output all-class information to file.
   * \param snapshot [in]  Snapshot instance.
   * \param rank     [out] Sorted-class information.
   * \return Value is zero, if process is succeed.<br />
   *         Value is error number a.k.a. "errno", if process is failure.
   */
  virtual int afterTakeSnapShot(TSnapShotContainer *snapshot,
                                TSorter<THeapDelta> **rank);

  void removeBeforeUpdatedData(void) {
    PKlassOop klass;
    while (updatedClassList.try_pop(klass)) {
      classMap.erase(klass);
    }
  }

 private:
  /*!
   * \brief SNMP trap sender.
   */
  TTrapSender *pSender;

  /*!
   * \brief Maps of class counting record.
   */
  TClassMap classMap;

  /*!
   * \brief Updated class list.
   */
  tbb::concurrent_queue<PKlassOop> updatedClassList;
};

/*!
 * \brief Class unload event. Unloaded class will be added to unloaded list.
 * \param jvmti  [in] JVMTI environment object.
 * \param env    [in] JNI environment object.
 * \param thread [in] Java thread object.
 * \param klass  [in] Unload class object.
 * \sa    from: hotspot/src/share/vm/prims/jvmti.xml
 */
void JNICALL
     OnClassUnload(jvmtiEnv *jvmti, JNIEnv *env, jthread thread, jclass klass);

/*!
 * \brief GarbageCollectionFinish JVMTI event to release memory for unloaded
 *        TObjectData.
 *        This function will be called at safepoint.
 *        All GC worker and JVMTI agent threads for HeapStats will not work
 *        at this point.
 */
void JNICALL OnGarbageCollectionFinishForUnload(jvmtiEnv *jvmti);

#endif  // CLASS_CONTAINER_HPP

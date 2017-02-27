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

#include <tr1/unordered_map>
#include <deque>

#include "snapShotContainer.hpp"
#include "sorter.hpp"
#include "trapSender.hpp"

#if PROCESSOR_ARCH == X86
#include "arch/x86/lock.inline.hpp"
#elif PROCESSOR_ARCH == ARM
#include "arch/arm/lock.inline.hpp"
#endif

/*!
 * \brief This structure stored size of a class used in heap.
 */
typedef struct {
  jlong tag;   /*!< Pointer of TObjectData.                */
  jlong usage; /*!< Class using total size.                */
  jlong delta; /*!< Class delta size from before snapshot. */
} THeapDelta;

/*!
 * \brief This type is for map stored class information.
 */
typedef std::tr1::unordered_map<void *, TObjectData *,
                                TNumericalHasher<void *> > TClassMap;

/*!
 * \brief Memory usage alert types.
 */
typedef enum { ALERT_JAVA_HEAP, ALERT_METASPACE } TMemoryUsageAlertType;

/*!
 * \brief This class is stored class information.<br>
 *        e.g. class-name, class instance count, size, etc...
 */
class TClassContainer;

/*!
 * \brief This type is for TClassContainer in Thread-Local-Storage.
 */
typedef std::deque<TClassContainer *> TLocalClassContainer;

/*!
 * \brief This class is stored class information.<br>
 *        e.g. class-name, class instance count, size, etc...
 */
class TClassContainer {
 public:
  /*!
   * \brief TClassContainer constructor.
   * \param base      [in] Parent class container instance.
   * \param needToClr [in] Flag of deallocate all data on destructor.
   */
  TClassContainer(TClassContainer *base = NULL, bool needToClr = true);
  /*!
   * \brief TClassContainer destructor.
   */
  virtual ~TClassContainer(void);

  /*!
   * \brief Append new-class to container.
   * \param klassOop [in] New class oop.
   * \return New-class data.
   */
  virtual TObjectData *pushNewClass(void *klassOop);

  /*!
   * \brief Append new-class to container.
   * \param klassOop [in] New class oop.
   * \param objData  [in] Add new class data.
   * \return New-class data.<br />
   *         This value isn't equal param "objData",
   *         if already registered equivalence class.
   */
  virtual TObjectData *pushNewClass(void *klassOop, TObjectData *objData);

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
  inline TObjectData *findClass(void *klassOop) {
    /* Search class data. */
    TObjectData *result = NULL;

    /* Get class container's spin lock. */
    spinLockWait(&lockval);
    {
      /* Search class data. */
      TClassMap::iterator it = classMap->find(klassOop);
      if (it != classMap->end()) {
        result = (*it).second;
      }
    }
    /* Release class container's spin lock. */
    spinLockRelease(&lockval);

    return result;
  }

  /*!
   * \brief Update class oop.
   * \param oldKlassOop [in] Target old class oop.
   * \param newKlassOop [in] Target new class oop.
   * \return Class data of target class.
   */
  inline void updateClass(void *oldKlassOop, void *newKlassOop) {
    /* Get class container's spin lock. */
    spinLockWait(&lockval);
    {
      /* Search class data. */
      TClassMap::iterator it = classMap->find(oldKlassOop);
      if (it != classMap->end()) {
        TObjectData *cur = (*it).second;

        /* Remove old klassOop. */
        classMap->erase(it);

        try {
          /* Update class data. */
          (*classMap)[newKlassOop] = cur;
          cur->klassOop = newKlassOop;
        } catch (...) {
          /*
           * Maybe failed to allocate memory
           * at "std::map::operator[]".
           */
        }
      }
    }
    /* Release class container's spin lock. */
    spinLockRelease(&lockval);

    /* Get spin lock of containers queue. */
    spinLockWait(&queueLock);
    {
      TLocalClassContainer::iterator it = localContainers.begin();
      /* Broadcast to each local container. */
      for (; it != localContainers.end(); it++) {
        (*it)->updateClass(oldKlassOop, newKlassOop);
      }
    }
    /* Release spin lock of containers queue. */
    spinLockRelease(&queueLock);
  }

  /*!
   * \brief Get class entries count.
   * \return Entries count of class information.
   */
  inline size_t getContainerSize(void) {
    size_t result = 0;

    /* Get class container's spin lock. */
    spinLockWait(&lockval);
    { result = this->classMap->size(); }
    /* Release class container's spin lock. */
    spinLockRelease(&lockval);
    return result;
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

  /*!
   * \brief Get local class container with each threads.
   * \return Local class container instance for this thread.
   */
  inline TClassContainer *getLocalContainer(void) {
    /* Get container for this thread. */
    TClassContainer *result =
        (TClassContainer *)pthread_getspecific(clsContainerKey);

    /* If container isn't exists yet. */
    if (unlikely(result == NULL)) {
      try {
        result = new TClassContainer(this, false);
      } catch (...) {
        /* Maybe raised badalloc exception. */
        return NULL;
      }
      pthread_setspecific(clsContainerKey, result);

      bool isFailure = false;
      /* Get spin lock of containers queue. */
      spinLockWait(&queueLock);
      {
        try {
          localContainers.push_back(result);
        } catch (...) {
          /* Maybe failed to add queue. */
          isFailure = true;
        }
      }
      /* Release spin lock of containers queue. */
      spinLockRelease(&queueLock);

      if (unlikely(isFailure)) {
        delete result;
        result = NULL;
      }
    }

    return result;
  }

 protected:
  /*!
   * \brief ClassContainer in TLS of each threads.
   */
  TLocalClassContainer localContainers;

  RELEASE_ONLY(private :)
  /*!
   * \brief SNMP trap sender.
   */
  TTrapSender *pSender;

  /*!
   * \brief Maps of class counting record.
   */
  TClassMap *classMap;

  /*!
   * \brief The thread storage key for each local class container.
   */
  pthread_key_t clsContainerKey;

  /*!
   * \brief SpinLock variable for class container instance.
   */
  volatile int lockval;

  /*!
   * \brief SpinLock variable for queue of local class containers.
   */
  volatile int queueLock;

  /*!
   * \brief Do we need to clear at destructor?
   */
  bool needToClear;
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

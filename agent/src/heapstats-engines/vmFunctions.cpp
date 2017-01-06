/*!
 * \file vmFunctions.cpp
 * \brief This file includes functions in HotSpot VM.<br>
 * Copyright (C) 2014-2016 Yasumasa Suenaga
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
#include "vmFunctions.hpp"

/* Variables */
TVMFunctions *TVMFunctions::inst = NULL;

/*!
 * \brief VTable list which should be hooked.
 */
void *VTableForTypeArrayOopClosure[2] __attribute__((aligned(16)));

/*!
 * \brief Function pointer for "is_in_permanent".
 */
THeap_IsIn is_in_permanent = NULL;

/* Internal function */

/*!
 * \brief This dummy function for "is_in_permanent".<br>
 * \param thisptr [in] Heap object instance.
 * \param oop     [in] Java object.
 * \return Always value is "false".
 */
bool dummyIsInPermanent(void *thisptr, void *oop) { return false; }

/**********************************************************/

/*!
 * \brief Instance initialier.
 * \param sym [in] Symbol finder of libjvm.so .
 * \return Singleton instance of TVMFunctions.
 */
TVMFunctions *TVMFunctions::initialize(TSymbolFinder *sym) {
  inst = new TVMFunctions(sym);
  if (inst == NULL) {
    logger->printCritMsg("Cannot initialize TVMFunctions.");
    return NULL;
  }

  if (inst->getFunctionsFromSymbol()) {
    return inst;
  } else {
    logger->printCritMsg("Cannot initialize TVMFunctions.");
    delete inst;
    return NULL;
  }
}

/*!
 * \brief Get HotSpot functions through symbol table.
 * \return Result of this function.
 */
bool TVMFunctions::getFunctionsFromSymbol(void) {
  TVMVariables *vmVal = TVMVariables::getInstance();
  if (vmVal == NULL) {
    logger->printCritMsg(
        "TVMVariables should be initialized before TVMFunction call.");
    return false;
  }

  /* Search "is_in_permanent" function symbol. */
  if (jvmInfo->isAfterCR6964458()) {
    /* Perm gen does not exist. */
    is_in_permanent = (THeap_IsIn)&dummyIsInPermanent;
  } else if (vmVal->getUseParallel() || vmVal->getUseParOld()) {
    is_in_permanent = (THeap_IsIn) this->symFinder->findSymbol(
        IS_IN_PERM_ON_PARALLEL_GC_SYMBOL);
  } else {
    is_in_permanent = (THeap_IsIn) this->symFinder->findSymbol(
        IS_IN_PERM_ON_OTHER_GC_SYMBOL);
  }

  if (unlikely(is_in_permanent == NULL)) {
    logger->printCritMsg("is_in_permanent() not found.");
    return false;
  }

  /* Search "is_in" function symbol for ParNew GC when CMS GC is worked. */
  if (vmVal->getUseCMS()) {
    is_in = (THeap_IsIn) this->symFinder->findSymbol(IS_IN_SYMBOL);
    if (unlikely(is_in == NULL)) {
      logger->printCritMsg("is_in() not found.");
      return false;
    }
  }

  /* Search "GetObjectSize" function symbol. */
  getObjectSize = (TJvmtiEnv_GetObjectSize) this->symFinder->findSymbol(
      SYMBOL_GETOBJCTSIZE);
  if (unlikely(getObjectSize == NULL)) {
    logger->printCritMsg("GetObjectSize() not found.");
    return false;
  }

  /* Search "as_klassOop" function symbol. */
  asKlassOop = (TJavaLangClass_AsKlassOop) this->symFinder->findSymbol(
      (jvmInfo->isAfterCR6964458()) ? AS_KLASS_SYMBOL : AS_KLASSOOP_SYMBOL);
  if (unlikely(asKlassOop == NULL)) {
    logger->printWarnMsg("as_klassOop() not found.");
    return false;
  }

  /* Search symbol of function getting classloader. */
  if (jvmInfo->isAfterCR8004883()) {
    getClassLoaderForInstanceKlass = (TGetClassLoader)symFinder->findSymbol(
        CR8004883_GET_CLSLOADER_FOR_INSTANCE_SYMBOL);
    getClassLoaderForObjArrayKlass = (TGetClassLoader)symFinder->findSymbol(
        CR8004883_GET_CLSLOADER_FOR_OBJARY_SYMBOL);
  } else {
    getClassLoaderForInstanceKlass = (TGetClassLoader)symFinder->findSymbol(
        GET_CLSLOADER_FOR_INSTANCE_SYMBOL);
    getClassLoaderForObjArrayKlass =
        (TGetClassLoader)symFinder->findSymbol(GET_CLSLOADER_FOR_OBJARY_SYMBOL);
  }

  if (unlikely(getClassLoaderForInstanceKlass == NULL ||
               getClassLoaderForObjArrayKlass == NULL)) {
    logger->printCritMsg("get_classloader not found.");
    return false;
  }

  /* Search "thread_id" function symbol. */
  getThreadId =
      (TGetThreadId) this->symFinder->findSymbol(GET_THREAD_ID_SYMBOL);
  if (unlikely(getThreadId == NULL)) {
    logger->printWarnMsg("java_lang_Thread::thread_id() not found.");
    return false;
  }

  /* Search "Unsafe_Park" function symbol. */
  unsafePark = (TUnsafe_Park) this->symFinder->findSymbol(UNSAFE_PARK_SYMBOL);
  if (unlikely(unsafePark == NULL)) {
    logger->printWarnMsg("Unsafe_Park() not found.");
    return false;
  }

  /* Search "get_thread" function symbol. */
  get_thread = (TGet_thread) this->symFinder->findSymbol(GET_THREAD_SYMBOL);
  if (get_thread == NULL) { // for JDK 9
    /* Search "ThreadLocalStorage::thread" function symbol. */
    get_thread = (TGet_thread) this->symFinder->findSymbol(
                                              THREADLOCALSTORAGE_THREAD_SYMBOL);
    if (unlikely(get_thread == NULL)) {
      logger->printWarnMsg("ThreadLocalStorage::thread() not found.");
      return false;
    }
  }

  /* Search "UserHandler" function symbol. */
  userHandler = (TUserHandler) this->symFinder->findSymbol(USERHANDLER_SYMBOL);
  if (unlikely(userHandler == NULL)) {
    logger->printWarnMsg("UserHandler() not found.");
    return false;
  }

  /* Search "SR_handler" function symbol. */
  sr_handler = (TSR_Handler) this->symFinder->findSymbol(SR_HANDLER_SYMBOL);
  if (sr_handler == NULL) { // for OpenJDK
    sr_handler = (TSR_Handler) this->symFinder->findSymbol(
                                                    SR_HANDLER_SYMBOL_FALLBACK);
    if (sr_handler == NULL) {
      logger->printWarnMsg("SR_handler() not found.");
      return false;
    }
  }

  /* Search "ObjectSynchronizer::get_lock_owner()" function symbol. */
  getLockOwner = (TGetLockOwner) this->symFinder->findSymbol(
                                                           GETLOCKOWNER_SYMBOL);
  if (unlikely(getLockOwner == NULL)) {
    logger->printWarnMsg("ObjectSynchronizer::get_lock_owner() not found.");
    return false;
  }

  /* Search "ThreadSafepointState::create()" function symbol. */
  threadSafepointStateCreate = (TVMThreadFunction) this->symFinder->findSymbol(
                                            THREADSAFEPOINTSTATE_CREATE_SYMBOL);
  if (unlikely(threadSafepointStateCreate == NULL)) {
    logger->printWarnMsg("ThreadSafepointState::create() not found.");
    return false;
  }

  /* Search "ThreadSafepointState::destroy()" function symbol. */
  threadSafepointStateDestroy = (TVMThreadFunction) this->symFinder->findSymbol(
                                           THREADSAFEPOINTSTATE_DESTROY_SYMBOL);
  if (unlikely(threadSafepointStateDestroy == NULL)) {
    logger->printWarnMsg("ThreadSafepointState::destroy() not found.");
    return false;
  }

  /* Search "Monitor::lock()" function symbol. */
  monitor_lock = (TVMMonitorFunction) this->symFinder->findSymbol(
                                                           MONITOR_LOCK_SYMBOL);
  if (unlikely(monitor_lock == NULL)) {
    logger->printWarnMsg("Monitor::lock() not found.");
    return false;
  }

  /* Search "Monitor::lock_without_safepoint_check()" function symbol. */
  monitor_lock_without_safepoint_check =
                    (TVMMonitorFunction) this->symFinder->findSymbol(
                                   MONITOR_LOCK_WTIHOUT_SAFEPOINT_CHECK_SYMBOL);
  if (unlikely(monitor_lock_without_safepoint_check == NULL)) {
    logger->printWarnMsg("Monitor::lock_without_safepoint_check() not found.");
    return false;
  }

  /* Search "Monitor::unlock()" function symbol. */
  monitor_unlock = (TVMMonitorFunction) this->symFinder->findSymbol(
                                                         MONITOR_UNLOCK_SYMBOL);
  if (unlikely(monitor_unlock == NULL)) {
    logger->printWarnMsg("Monitor::unlock() not found.");
    return false;
  }

  /* Search "Monitor::owned_by_self()" function symbol. */
  monitor_owned_by_self = (TOwnedBySelf) this->symFinder->findSymbol(
                                                  MONITOR_OWNED_BY_SELF_SYMBOL);
  if (unlikely(monitor_owned_by_self == NULL)) {
    logger->printWarnMsg("Monitor::owned_by_self() not found.");
    return false;
  }

  if (vmVal->getUseG1()) {
    inst->getG1VTableFromSymbol();
  }

  return true;
}

/*!
 * \brief Get vtable through symbol table which is related to G1.
 * \return Result of this function.
 */
bool TVMFunctions::getG1VTableFromSymbol(void) {
  /* Add vtable offset */
#ifdef __LP64__
  VTableForTypeArrayOopClosure[0] =
    incAddress(symFinder->findSymbol("_ZTV14G1CMOopClosure"), 16);
  VTableForTypeArrayOopClosure[1] =
    incAddress(symFinder->findSymbol("_ZTV23G1RootRegionScanClosure"), 16);
#else
  VTableForTypeArrayOopClosure[0] =
    incAddress(symFinder->findSymbol("_ZTV14G1CMOopClosure"), 8);
  VTableForTypeArrayOopClosure[1] =
    incAddress(symFinder->findSymbol("_ZTV23G1RootRegionScanClosure"), 8);
#endif

  if (unlikely(VTableForTypeArrayOopClosure[0] == NULL ||
               VTableForTypeArrayOopClosure[1] == NULL)) {
    logger->printCritMsg("Cannot get vtables which are related to G1.");
    return false;
  }

  return true;
}

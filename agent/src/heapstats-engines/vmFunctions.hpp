/*!
 * \file vmFunctions.hpp
 * \brief This file includes functions in HotSpot VM.
 * Copyright (C) 2014-2017 Yasumasa Suenaga
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

#ifndef VMFUNCTIONS_H
#define VMVARIABLES_H

#include <jni.h>

#include "symbolFinder.hpp"

/* Macros for symbol */

/*!
 * \brief String of symbol which is "is_in_permanent" function on parallel GC.
 */
#define IS_IN_PERM_ON_PARALLEL_GC_SYMBOL \
  "_ZNK20ParallelScavengeHeap15is_in_permanentEPKv"

/*!
 * \brief String of symbol which is "is_in_permanent" function on other GC.
 */
#define IS_IN_PERM_ON_OTHER_GC_SYMBOL "_ZNK10SharedHeap15is_in_permanentEPKv"

/*!
 * \brief Symbol of Generation::is_in()
 */
#define IS_IN_SYMBOL "_ZNK10Generation5is_inEPKv"

/*!
 * \brief Symbol of "JvmtiEnv::GetObjectSize" macro.
 */
#ifdef __x86_64__
#define SYMBOL_GETOBJCTSIZE "_ZN8JvmtiEnv13GetObjectSizeEP8_jobjectPl"
#else
#define SYMBOL_GETOBJCTSIZE "_ZN8JvmtiEnv13GetObjectSizeEP8_jobjectPx"
#endif

/*!
 * \brief String of symbol which is "java.lang.Class.as_klassOop" function.
 */
#define AS_KLASSOOP_SYMBOL "_ZN15java_lang_Class11as_klassOopEP7oopDesc"

/*!
 * \brief String of symbol which is "java.lang.Class.as_klass" function.<br />
 *        This function is for after CR6964458.
 */
#define AS_KLASS_SYMBOL "_ZN15java_lang_Class8as_KlassEP7oopDesc"

/*!
 * \brief String of symbol which is function get class loader for instance.
 */
#define GET_CLSLOADER_FOR_INSTANCE_SYMBOL "_ZNK13instanceKlass12class_loaderEv"

/*!
 * \brief String of symbol which is function get class loader for instance
 *        after CR#8004883.
 */
#define CR8004883_GET_CLSLOADER_FOR_INSTANCE_SYMBOL \
  "_ZNK13InstanceKlass12klass_holderEv"

/*!
 * \brief String of symbol which is function get class loader for object array.
 */
#define GET_CLSLOADER_FOR_OBJARY_SYMBOL "_ZNK13objArrayKlass12class_loaderEv"

/*!
 * \brief String of symbol which is function get class loader for object array
 *        after CR#8004883.
 */
#define CR8004883_GET_CLSLOADER_FOR_OBJARY_SYMBOL "_ZNK5Klass12klass_holderEv"

/*!
 * \brief Symbol of java_lang_Thread::thread_id()
 */
#define GET_THREAD_ID_SYMBOL "_ZN16java_lang_Thread9thread_idEP7oopDesc"

/*!
 * \brief Symbol of Unsafe_Park()
 */
#define UNSAFE_PARK_SYMBOL "Unsafe_Park"

/*!
 * \brief Symbol of get_thread()
 */
#define GET_THREAD_SYMBOL "get_thread"

/*!
 * \brief Symbol of ThreadLocalStorage::thread()
 */
#define THREADLOCALSTORAGE_THREAD_SYMBOL "_ZN18ThreadLocalStorage6threadEv"

/*!
 * \brief Symbol of UserHandler()
 */
#define USERHANDLER_SYMBOL "_ZL11UserHandleriPvS_"
#define USERHANDLER_SYMBOL_JDK6 "_Z11UserHandleriPvS_"

/*!
 * \brief Symbol of SR_handler()
 */
#define SR_HANDLER_SYMBOL           "_ZL10SR_handleriP7siginfoP8ucontext"
#define SR_HANDLER_SYMBOL_FALLBACK  "_ZL10SR_handleriP9siginfo_tP8ucontext"
#define SR_HANDLER_SYMBOL_JDK6      "_Z10SR_handleriP7siginfoP8ucontext"
#define SR_HANDLER_SYMBOL_FALLBACK2 "_ZL10SR_handleriP9siginfo_tP10ucontext_t"

/*!
 * \brief Symbol of ObjectSynchronizer::get_lock_owner().
 */
#define GETLOCKOWNER_SYMBOL "_ZN18ObjectSynchronizer14get_lock_ownerE6Handleb"

/*!
 * \brief Symbol of ThreadSafepointState::create().
 */
#define THREADSAFEPOINTSTATE_CREATE_SYMBOL \
  "_ZN20ThreadSafepointState6createEP10JavaThread"

/*!
 * \brief Symbol of ThreadSafepointState::destroy().
 */
#define THREADSAFEPOINTSTATE_DESTROY_SYMBOL \
  "_ZN20ThreadSafepointState7destroyEP10JavaThread"

/*!
 * \brief Symbol of Monitor::lock().
 */
#define MONITOR_LOCK_SYMBOL "_ZN7Monitor4lockEv"

/*!
 * \brief Symbol of Monitor::lock_without_safepoint_check().
 */
#define MONITOR_LOCK_WTIHOUT_SAFEPOINT_CHECK_SYMBOL \
  "_ZN7Monitor28lock_without_safepoint_checkEv"

/*!
 * \brief Symbol of Monitor::unlock().
 */
#define MONITOR_UNLOCK_SYMBOL "_ZN7Monitor6unlockEv"

/*!
 * \brief Symbol of Monitor::owned_by_self().
 */
#define MONITOR_OWNED_BY_SELF_SYMBOL "_ZNK7Monitor13owned_by_selfEv"


/* Function type definition */

/*!
 * \brief This function is C++ heap class member.<br>
 *        So 1st arg must be set instance.
 * \param thisptr [in] Instance of Java memory region.
 * \param oop     [in] Java object.
 * \return true if oop is in this region.
 */
typedef bool (*THeap_IsIn)(const void *thisptr, const void *oop);

/*!
 * \brief This function is C++ JvmtiEnv class member.<br>
 *        So 1st arg must be set instance.<br>
 *        jvmtiError JvmtiEnv::GetObjectSize(jobject object, jlong* size_ptr)
 * \param thisptr  [in]  JvmtiEnv object instance.
 * \param object   [in]  Java object.
 * \param size_ptr [out] Pointer of java object's size.
 * \return Java object's size.
 */
typedef int (*TJvmtiEnv_GetObjectSize)(void *thisptr, jobject object,
                                       jlong *size_ptr);

/*!
 * \brief This function is java_lang_Class class member.<br>
 *        void *java_lang_Class::as_klassOop(void *mirror);
 * \param mirror [in] Java object mirror.
 * \return KlassOop of java object mirror.
 */
typedef void *(*TJavaLangClass_AsKlassOop)(void *mirror);

/*!
 * \brief This function is for get classloader.<br>
 *        E.g. instanceKlass::class_loader()<br>
 *             objArrayKlass::class_loader()<br>
 * \param klassOop [in] Pointer of java class object(KlassOopDesc).
 * \return Java heap object which is class loader load expected the class.
 */
typedef void *(*TGetClassLoader)(void *klassOop);

/*!
 * \brief Get thread ID (Thread#getId()) from oop.
 * \param oop [in] oop of java.lang.Thread .
 * \return Thread ID
 */
typedef jlong (*TGetThreadId)(void *oop);

/*!
 * \brief Get JavaThread from oop.
 * \param oop [in] oop of java.lang.Thread .
 * \return Pointer of JavaThread
 */
typedef void *(*TGetThread)(void *oop);

/*!
 * \brief JNI function of sun.misc.Unsafe#park() .
 * \param env [in] JNI environment.
 * \param unsafe [in] Unsafe object.
 * \param isAbsolute [in] absolute.
 * \param time [in] Park time.
 */
typedef void (*TUnsafe_Park)(JNIEnv *env, jobject unsafe, jboolean isAbsolute,
                             jlong time);

/*!
 * \brief JNI function of sun.misc.Unsafe#park() .
 * \param env [in] JNI environment.
 * \param unsafe [in] Unsafe object.
 * \param isAbsolute [in] absolute.
 * \param time [in] Park time.
 */
typedef void (*TUnsafe_Park)(JNIEnv *env, jobject unsafe, jboolean isAbsolute,
                             jlong time);

/*!
 * \brief Get C++ Thread instance.
 * \return C++ Thread instance of this thread context.
 */
typedef void *(*TGet_thread)();

/*!
 * \brief User signal handler for HotSpot.
 * \param sig Signal number.
 * \param siginfo Signal information.
 * \param context Thread context.
 */
typedef void (*TUserHandler)(int sig, void *siginfo, void *context);

/*!
 * \brief Thread suspend/resume signal handler in HotSpot.
 * \param sig Signal number.
 * \param siginfo Signal information.
 * \param context Thread context.
 */
typedef void (*TSR_Handler)(int sig, siginfo_t *siginfo, ucontext_t *context);

/*!
 * \brief function type of
 * "JavaThread* ObjectSynchronizer::get_lock_owner(Handle h_obj, bool doLock)".
 * \param monitor_oop [in] Target monitor oop.
 * \param doLock      [in] Enable oop lock.
 * \return Monitor owner thread oop.<br>
 *         Value is NULL, if owner is none.
 */
typedef void *(*TGetLockOwner)(void *monitor_oop, bool doLock);

/*!
 * \brief function type of common thread operation.
 * \param thread [in] Target thread object is inner JVM class instance.
 */
typedef void (*TVMThreadFunction)(void *thread);

/*!
 * \brief function type of common monitor operation.
 * \param monitor_oop [in] Target monitor oop.
 */
typedef void (*TVMMonitorFunction)(void *monitor_oop);

/*!
 * \brief function type of common monitor operation.
 * \param monitor_oop [in] Target monitor oop.
 * \return Thread is owned monitor.
 */
typedef bool (*TOwnedBySelf)(void *monitor_oop);


/* Exported function in libjvm.so */
extern "C" void *JVM_RegisterSignal(jint sig, void *handler);


/* extern variables */
extern "C" void *VTableForTypeArrayOopClosure[2];
extern "C" THeap_IsIn is_in_permanent;

/*!
 * \brief This class gathers/provides functions in HotSpot VM.
 */
class TVMFunctions {
 private:
  /*!
   * \brief Function pointer for "JvmtiEnv::GetObjectSize".
   */
  TJvmtiEnv_GetObjectSize getObjectSize;

  /*!
   * \brief Function pointer for "Generation::is_in".
   */
  THeap_IsIn is_in;

  /*!
   * \brief Function pointer for "java_lang_Class::as_klassOop".
   */
  TJavaLangClass_AsKlassOop asKlassOop;

  /*!
   * \brief Function pointer for "instanceKlass::class_loader()".
   */
  TGetClassLoader getClassLoaderForInstanceKlass;

  /*!
   * \brief Function pointer for "objArrayKlass::class_loader()".
   */
  TGetClassLoader getClassLoaderForObjArrayKlass;

  /*!
   * \brief Function pointer for "java_lang_Thread::thread_id()".
   */
  TGetThreadId getThreadId;

  /*!
   * \brief Function pointer for "Unsafe_Park()".
   */
  TUnsafe_Park unsafePark;

  /*!
   * \brief Function pointer for "get_thread()".
   */
  TGet_thread get_thread;

  /*!
   * \brief Function pointer for "UserHandler".
   */
  TUserHandler userHandler;

  /*!
   * \brief Function pointer for "SR_handler".
   */
  TSR_Handler sr_handler;

  /*!
   * \brief Function pointer for "ObjectSynchronizer::get_lock_owner()".
   */
  TGetLockOwner getLockOwner;

  /*!
   * \brief Function pointer for "ThreadSafepointState::create()".
   */
  TVMThreadFunction threadSafepointStateCreate;

  /*!
   * \brief Function pointer for "ThreadSafepointState::destroy()".
   */
  TVMThreadFunction threadSafepointStateDestroy;

  /*!
   * \brief Function pointer for "Monitor::lock()"
   */
  TVMMonitorFunction monitor_lock;

  /*!
   * \brief Function pointer for "Monitor::lock_without_safepoint_check()".
   */
  TVMMonitorFunction monitor_lock_without_safepoint_check;

  /*!
   * \brief Function pointer for "Monitor::unlock()".
   */
  TVMMonitorFunction monitor_unlock;

  /*!
   * \brief Function pointer for "Monitor::owned_by_self()".
   */
  TOwnedBySelf monitor_owned_by_self;

  /* Class of HeapStats for scanning variables in HotSpot VM */
  TSymbolFinder *symFinder;

  /*!
   * \brief Singleton instance of TVMFunctions.
   */
  static TVMFunctions *inst;

 protected:
  /*!
   * \brief Get HotSpot functions through symbol table.
   * \return Result of this function.
   */
  bool getFunctionsFromSymbol(void);

  /*!
   * \brief Get vtable through symbol table which is related to G1.
   * \return Result of this function.
   */
  bool getG1VTableFromSymbol(void);

 public:
  /*
   * Constructor of TVMFunctions.
   * \param sym [in] Symbol finder of libjvm.so .
   */
  TVMFunctions(TSymbolFinder *sym) : symFinder(sym){};

  /*!
   * \brief Instance initializer.
   * \param sym [in] Symbol finder of libjvm.so .
   * \return Singleton instance of TVMFunctions.
   */
  static TVMFunctions *initialize(TSymbolFinder *sym);

  /*!
   * \brief Get singleton instance of TVMFunctions.
   * \return Singleton instance of TVMFunctions.
   */
  static TVMFunctions *getInstance() { return inst; };

  /* Delegators to HotSpot VM */

  inline int GetObjectSize(void *thisptr, jobject object, jlong *size_ptr) {
    return getObjectSize(thisptr, object, size_ptr);
  }

  inline bool IsInYoung(const void *oop) {
    return is_in(TVMVariables::getInstance()->getYoungGen(), oop);
  }

  inline void *AsKlassOop(void *mirror) { return asKlassOop(mirror); }

  inline void *GetClassLoaderForInstanceKlass(void *klassOop) {
    return getClassLoaderForInstanceKlass(klassOop);
  }

  inline void *GetClassLoaderForObjArrayKlass(void *klassOop) {
    return getClassLoaderForObjArrayKlass(klassOop);
  }

  inline jlong GetThreadId(void *oop) { return getThreadId(oop); }

  inline void Unsafe_Park(JNIEnv *env, jobject unsafe, jboolean isAbsolute,
                          jlong time) {
    return unsafePark(env, unsafe, isAbsolute, time);
  }

  inline void *GetUnsafe_ParkPointer(void) { return (void *)unsafePark; }

  inline void *GetThread(void) {
    return get_thread();
  }

  inline void *GetUserHandlerPointer(void) { return (void *)userHandler; }

  inline void *GetSRHandlerPointer(void) { return (void *)sr_handler; }

  inline void *GetLockOwner(void *monitor_oop, bool doLock) {
    return getLockOwner(monitor_oop, doLock);
  }

  inline void ThreadSafepointStateCreate(void *thread) {
    threadSafepointStateCreate(thread);
  }

  inline void ThreadSafepointStateDestroy(void *thread) {
    threadSafepointStateDestroy(thread);
  }

  inline void MonitorLock(void *monitor_oop) {
    monitor_lock(monitor_oop);
  }

  inline void MonitorLockWithoutSafepointCheck(void *monitor_oop) {
    monitor_lock_without_safepoint_check(monitor_oop);
  }

  inline void MonitorUnlock(void *monitor_oop) {
    monitor_unlock(monitor_oop);
  }

  /*!
   * \brief Function pointer for "Monitor::owned_by_self()".
   */
  inline bool MonitorOwnedBySelf(void *monitor_oop) {
    return monitor_owned_by_self(monitor_oop);
  }
};

#endif  // VMFUNCTIONS_H

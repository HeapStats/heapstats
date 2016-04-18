/*!
 * \file overrider.hpp
 * \brief Controller of overriding functions in HotSpot VM.
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

#ifndef OVERRIDER_H
#define OVERRIDER_H

#include <stddef.h>

/*!
 * \brief Override function information structure.
 */
typedef struct {
  const char *vtableSymbol; /*!< Symbol string of target class.            */
  void **vtable;            /*!< Pointer of C++ vtable.                    */
  const char *funcSymbol;   /*!< Symbol string of target function.         */
  void *overrideFunc;       /*!< Pointer of override function.             */
  void *originalFunc;       /*!< Original callback target.                 */
  void **originalFuncPtr;   /*!< Pointer of original callback target.      */
  void *enterFunc;          /*!< Enter event for hook target.              */
  void **enterFuncPtr;      /*!< Pointer of enter event for hook target.   */
  bool isVTableWritable;    /*!< Is vtable already writable?               */
} THookFunctionInfo;

/*!
 * \brief This structure is expressing garbage collector state.
 */
typedef enum {
  gcStart = 1,  /*!< Garbage collector is start.                    */
  gcFinish = 2, /*!< Garbage collector is finished.                 */
  gcLast = 3,   /*!< Garbage collector is tarminating on JVM death. */
} TGCState;

/* Variable for CMS collector state. */

/*!
 * \brief CMS collector is idling.
 */
#define CMS_IDLING 2

/*!
 * \brief CMS collector is initial-marking phase.
 */
#define CMS_INITIALMARKING 3

/*!
 * \brief CMS collector is marking phase.
 */
#define CMS_MARKING 4

/*!
 * \brief CMS collector is final-making phase.
 */
#define CMS_FINALMARKING 7

/*!
 * \brief CMS collector is sweep phase.
 */
#define CMS_SWEEPING 8

/* Macro to define overriding functions. */

/*!
 * \brief Convert to THookFunctionInfo macro.
 */
#define HOOK_FUNC(prefix, num, vtableSym, funcSym, enterFunc)      \
  {                                                                \
    vtableSym, NULL, funcSym, &prefix##_override_func_##num, NULL, \
        &prefix##_original_func_##num, (void *)enterFunc,          \
        &prefix##_enter_hook_##num, false                          \
  }

/*!
 * \brief Enf flag for THookFunctionInfo macro.
 */
#define HOOK_FUNC_END \
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false }

#define DEFINE_OVERRIDE_FUNC_N(prefix, num)      \
  extern "C" void *prefix##_override_func_##num; \
  extern "C" {                                   \
    void *prefix##_original_func_##num;          \
    void *prefix##_enter_hook_##num;             \
  };

#define DEFINE_OVERRIDE_FUNC_1(prefix) DEFINE_OVERRIDE_FUNC_N(prefix, 0)

#define DEFINE_OVERRIDE_FUNC_2(prefix) \
  DEFINE_OVERRIDE_FUNC_N(prefix, 0)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 1)

#define DEFINE_OVERRIDE_FUNC_3(prefix) \
  DEFINE_OVERRIDE_FUNC_N(prefix, 0)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 1)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 2)

#define DEFINE_OVERRIDE_FUNC_4(prefix) \
  DEFINE_OVERRIDE_FUNC_N(prefix, 0)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 1)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 2)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 3)

#define DEFINE_OVERRIDE_FUNC_5(prefix) \
  DEFINE_OVERRIDE_FUNC_N(prefix, 0)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 1)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 2)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 3)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 4)

#define DEFINE_OVERRIDE_FUNC_8(prefix) \
  DEFINE_OVERRIDE_FUNC_N(prefix, 0)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 1)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 2)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 3)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 4)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 5)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 6)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 7)

#define DEFINE_OVERRIDE_FUNC_9(prefix) \
  DEFINE_OVERRIDE_FUNC_N(prefix, 0)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 1)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 2)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 3)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 4)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 5)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 6)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 7)    \
  DEFINE_OVERRIDE_FUNC_N(prefix, 8)

#define DEFINE_OVERRIDE_FUNC_11(prefix) \
  DEFINE_OVERRIDE_FUNC_N(prefix, 0)     \
  DEFINE_OVERRIDE_FUNC_N(prefix, 1)     \
  DEFINE_OVERRIDE_FUNC_N(prefix, 2)     \
  DEFINE_OVERRIDE_FUNC_N(prefix, 3)     \
  DEFINE_OVERRIDE_FUNC_N(prefix, 4)     \
  DEFINE_OVERRIDE_FUNC_N(prefix, 5)     \
  DEFINE_OVERRIDE_FUNC_N(prefix, 6)     \
  DEFINE_OVERRIDE_FUNC_N(prefix, 7)     \
  DEFINE_OVERRIDE_FUNC_N(prefix, 8)     \
  DEFINE_OVERRIDE_FUNC_N(prefix, 9)     \
  DEFINE_OVERRIDE_FUNC_N(prefix, 10)

/*!
 * \brief Macro to select override function with CR.
 */
#define SELECT_HOOK_FUNCS(prefix)              \
  if (jvmInfo->isAfterJDK9()) {                \
    prefix##_hook = jdk9_##prefix##_hook;      \
  } else if (jvmInfo->isAfterCR8049421()) {    \
    prefix##_hook = CR8049421_##prefix##_hook; \
  } else if (jvmInfo->isAfterCR8027746()) {    \
    prefix##_hook = CR8027746_##prefix##_hook; \
  } else if (jvmInfo->isAfterCR8000213()) {    \
    prefix##_hook = CR8000213_##prefix##_hook; \
  } else if (jvmInfo->isAfterCR6964458()) {    \
    prefix##_hook = CR6964458_##prefix##_hook; \
  } else {                                     \
    prefix##_hook = default_##prefix##_hook;   \
  }

/* Macro for deciding class type. */

/*!
 * \brief Header macro for array array class. Chars of "[[".
 */
#define HEADER_KLASS_ARRAY_ARRAY 0x5b5b

/*!
 * \brief Header macro for object array class. Chars of "[L".
 */
#ifdef WORDS_BIGENDIAN
#define HEADER_KLASS_OBJ_ARRAY 0x5b4c
#else
#define HEADER_KLASS_OBJ_ARRAY 0x4c5b
#endif

/*!
 * \brief Header macro for array class. Chars of "[".
 */
#define HEADER_KLASS_ARRAY 0x5b

/* Callback function type. */

/*!
 * \brief This function is for heap object callback.
 * \param oop  [in] Java heap object(Inner class format).
 * \param data [in] User expected data. Always this value is NULL.
 */
typedef void (*THeapObjectCallback)(void *oop, void *data);

/*!
 * \brief This function is for class oop adjust callback by GC.
 * \param oldOop [in] Old pointer of java class object(KlassOopDesc).
 * \param newOop [in] New pointer of java class object(KlassOopDesc).
 */
typedef void (*TKlassAdjustCallback)(void *oldOop, void *newOop);

/*!
 * \brief This function type is for common callback.
 */
typedef void (*TCommonCallback)(void);

/*!
 * \brief This function is for get classloader.<br>
 *        E.g. instanceKlass::class_loader()<br>
 *             objArrayKlass::class_loader()<br>
 * \param klassOop [in] Pointer of java class object(KlassOopDesc).
 * \return Java heap object which is class loader load expected the class.
 */
typedef void *(*TGetClassLoader)(void *klassOop);

/* extern functions (for overriding) */
extern "C" void callbackForParallel(void *oop);
extern "C" void callbackForParallelWithMarkCheck(void *oop);
extern "C" void callbackForParOld(void *oop);
extern "C" void callbackForDoOop(void **oop);
extern "C" void callbackForDoNarrowOop(unsigned int *narrowOop);
extern "C" void callbackForIterate(void *oop);
extern "C" void callbackForSweep(void *oop);
extern "C" void callbackForAdjustPtr(void *oop);
extern "C" void callbackForDoAddr(void *oop);
extern "C" void callbackForUpdatePtr(void *oop);
extern "C" void callbackForJvmtiIterate(void *oop);
extern "C" void callbackForG1Cleanup(void);
extern "C" void callbackForG1Full(bool isFull);
extern "C" void callbackForG1FullReturn(bool isFull);
extern "C" void callbackForInnerGCStart(void);
extern "C" void callbackForWatcherThreadRun(void);

/* Functions to be provided from this file. */

/*!
 * \brief Initialize override functions.
 * \return Process result.
 */
bool initOverrider(void);

/*!
 * \brief Cleanup override functions.
 */
void cleanupOverrider(void);

/*!
 * \brief Setup hooking.
 * \warning Please this function call at after Agent_OnLoad.
 * \param funcOnGC     [in] Pointer of GC callback function.
 * \param funcOnCMS    [in] Pointer of CMSGC callback function.
 * \param funcOnJVMTI  [in] Pointer of JVMTI callback function.
 * \param funcOnAdjust [in] Pointer of adjust class callback function.
 * \param funcOnG1GC   [in] Pointer of event callback on G1GC finished.
 * \param maxMemSize   [in] Allocatable maximum memory size of JVM.
 * \return Process result.
 */
bool setupHook(THeapObjectCallback funcOnGC, THeapObjectCallback funcOnCMS,
               THeapObjectCallback funcOnJVMTI,
               TKlassAdjustCallback funcOnAdjust, TCommonCallback funcOnG1GC,
               size_t maxMemSize);

/*!
 * \brief Setup hooking for inner GC event.
 * \warning Please this function call at after Agent_OnLoad.
 * \param enableHook     [in] Flag of enable hooking to function.
 * \param interruptEvent [in] Event callback on many times GC interupt.
 * \return Process result.
 */
bool setupHookForInnerGCEvent(bool enableHook, TCommonCallback interruptEvent);

/*!
 * \brief Setup override funtion.
 * \param list      [in]  List of hooking information.
 * \return Process result.
 */
bool setupOverrideFunction(THookFunctionInfo *list);

/*!
 * \brief Setting GC hooking enable.
 * \param enable [in] Is GC hook enable.
 * \return Process result.
 */
bool setGCHookState(bool enable);

/*!
 * \brief Setting JVMTI hooking enable.
 * \param enable [in] Is JVMTI hook enable.
 * \return Process result.
 */
bool setJvmtiHookState(bool enable);

/*!
 * \brief Switching override function enable state.
 * \param list   [in] List of hooking information.
 * \param enable [in] Enable of override function.
 * \return Process result.
 */
bool switchOverrideFunction(THookFunctionInfo *list, bool enable);

/*!
 * \brief Check CMS garbage collector state.
 * \param state        [in]  State of CMS garbage collector.
 * \param needSnapShot [out] Is need snapshot now.
 * \return CMS collector state.
 * \warning Please don't forget call on JVM death.
 */
int checkCMSState(TGCState state, bool *needSnapShot);

#endif  // OVERRIDER_H

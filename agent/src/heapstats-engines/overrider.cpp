/*!
 * \file overrider.cpp
 * \brief Controller of overriding functions in HotSpot VM.
 * Copyright (C) 2014-2018 Yasumasa Suenaga
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

#include <pthread.h>
#include <sys/mman.h>

#include "globals.hpp"
#include "vmVariables.hpp"
#include "vmFunctions.hpp"
#include "overrider.hpp"
#include "snapShotMain.hpp"

#if PROCESSOR_ARCH == X86
#include "arch/x86/x86BitMapMarker.hpp"

#ifdef AVX
#include "arch/x86/avx/avxBitMapMarker.hpp"
#endif

#if (defined SSE2) || (defined SSE4)
#include "arch/x86/sse2/sse2BitMapMarker.hpp"
#endif

#elif PROCESSOR_ARCH == ARM

#ifdef NEON
#include "arch/arm/neon/neonBitMapMarker.hpp"
#else
#include "arch/arm/armBitMapMarker.hpp"
#endif

#endif

/* These functions is defined in override.S. */

/*!
 * \brief Override function for heap object on parallelGC.
 */
DEFINE_OVERRIDE_FUNC_4(par);
DEFINE_OVERRIDE_FUNC_5(par_6964458);
DEFINE_OVERRIDE_FUNC_5(par_jdk9);
DEFINE_OVERRIDE_FUNC_7(par_jdk10);

/*!
 * \brief Override function for heap object on parallelOldGC.
 */
DEFINE_OVERRIDE_FUNC_4(parOld);
DEFINE_OVERRIDE_FUNC_5(parOld_6964458);
DEFINE_OVERRIDE_FUNC_5(parOld_jdk9);


/*!
 * \brief Override function for sweep at old gen on CMSGC.
 */
DEFINE_OVERRIDE_FUNC_1(cms_sweep);

/*!
 * \brief Override function for heap object at new gen on CMSGC.
 */
DEFINE_OVERRIDE_FUNC_4(cms_new);
DEFINE_OVERRIDE_FUNC_5(cms_new_6964458);
DEFINE_OVERRIDE_FUNC_5(cms_new_jdk9);

/*!
 * \brief Override function for heap object on G1GC.
 */
DEFINE_OVERRIDE_FUNC_9(g1);
DEFINE_OVERRIDE_FUNC_11(g1_6964458);
DEFINE_OVERRIDE_FUNC_12(g1_jdk9);
DEFINE_OVERRIDE_FUNC_19(g1_jdk10);

/*!
 * \brief Override function for cleanup and System.gc() event on G1GC.
 */
DEFINE_OVERRIDE_FUNC_3(g1Event);

/*!
 * \brief Override function for adjust klassOop.
 */
DEFINE_OVERRIDE_FUNC_8(adj);

/*!
 * \brief Override function for JVMTI function.
 */
DEFINE_OVERRIDE_FUNC_1(jvmti);

/*!
 * \brief Override function for inner GC function.
 */
DEFINE_OVERRIDE_FUNC_3(innerStart);

/*!
 * \brief Override function for WatcherThread.
 */
DEFINE_OVERRIDE_FUNC_1(watcherThread);

/*!
 * \brief Bitmap object for CMS and G1 GC.
 */
TBitMapMarker *checkObjectMap = NULL;

/* Variables for override information. */

/*!
 * \brief Pointer of hook information on parallelGC for default.
 */
THookFunctionInfo default_par_hook[] = {
    HOOK_FUNC(par, 0, "_ZTV13instanceKlass",
              "_ZN13instanceKlass19oop_follow_contentsEP7oopDesc",
              &callbackForParallel),
    HOOK_FUNC(par, 1, "_ZTV13objArrayKlass",
              "_ZN13objArrayKlass19oop_follow_contentsEP7oopDesc",
              &callbackForParallel),
    HOOK_FUNC(par, 2, "_ZTV14typeArrayKlass",
              "_ZN14typeArrayKlass19oop_follow_contentsEP7oopDesc",
              &callbackForParallel),
    HOOK_FUNC(par, 3, "_ZTV16instanceRefKlass",
              "_ZN16instanceRefKlass19oop_follow_contentsEP7oopDesc",
              &callbackForParallel),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on parallelGC for after CR6964458.
 */
THookFunctionInfo CR6964458_par_hook[] = {
    HOOK_FUNC(par_6964458, 0, "_ZTV13InstanceKlass",
              "_ZN13InstanceKlass19oop_follow_contentsEP7oopDesc",
              &callbackForParallel),
    HOOK_FUNC(par_6964458, 1, "_ZTV13objArrayKlass",
              "_ZN13objArrayKlass19oop_follow_contentsEP7oopDesc",
              &callbackForParallel),
    HOOK_FUNC(par_6964458, 2, "_ZTV14typeArrayKlass",
              "_ZN14typeArrayKlass19oop_follow_contentsEP7oopDesc",
              &callbackForParallel),
    HOOK_FUNC(par_6964458, 3, "_ZTV16InstanceRefKlass",
              "_ZN16InstanceRefKlass19oop_follow_contentsEP7oopDesc",
              &callbackForParallel),
    HOOK_FUNC(par_6964458, 4, "_ZTV24InstanceClassLoaderKlass",
              "_ZN24InstanceClassLoaderKlass19oop_follow_contentsEP7oopDesc",
              &callbackForParallel),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on parallelGC for after CR8000213.
 */
THookFunctionInfo CR8000213_par_hook[] = {
    HOOK_FUNC(par_6964458, 0, "_ZTV13InstanceKlass",
              "_ZN13InstanceKlass19oop_follow_contentsEP7oopDesc",
              &callbackForParallel),
    HOOK_FUNC(par_6964458, 1, "_ZTV13ObjArrayKlass",
              "_ZN13ObjArrayKlass19oop_follow_contentsEP7oopDesc",
              &callbackForParallel),
    HOOK_FUNC(par_6964458, 2, "_ZTV14TypeArrayKlass",
              "_ZN14TypeArrayKlass19oop_follow_contentsEP7oopDesc",
              &callbackForParallel),
    HOOK_FUNC(par_6964458, 3, "_ZTV16InstanceRefKlass",
              "_ZN16InstanceRefKlass19oop_follow_contentsEP7oopDesc",
              &callbackForParallel),
    HOOK_FUNC(par_6964458, 4, "_ZTV24InstanceClassLoaderKlass",
              "_ZN24InstanceClassLoaderKlass19oop_follow_contentsEP7oopDesc",
              &callbackForParallel),
    HOOK_FUNC_END};

/*!
 * \brief Pointers of hook information on parallelGC for several CRs.<br>
 *        These CRs have no impact on parallelGC, so refer to the last.
 */
#define CR8027746_par_hook CR8000213_par_hook
#define CR8049421_par_hook CR8000213_par_hook

/*!
 * \brief Pointer of hook information on parallelGC for after jdk 9.
 */
THookFunctionInfo jdk9_par_hook[] = {
    HOOK_FUNC(par_jdk9, 0, "_ZTV13InstanceKlass",
              "_ZN13InstanceKlass22oop_ms_adjust_pointersEP7oopDesc",
              &callbackForParallelWithMarkCheck),
    HOOK_FUNC(par_jdk9, 1, "_ZTV13ObjArrayKlass",
              "_ZN13ObjArrayKlass22oop_ms_adjust_pointersEP7oopDesc",
              &callbackForParallelWithMarkCheck),
    HOOK_FUNC(par_jdk9, 2, "_ZTV14TypeArrayKlass",
              "_ZN14TypeArrayKlass22oop_ms_adjust_pointersEP7oopDesc",
              &callbackForParallelWithMarkCheck),
    HOOK_FUNC(par_jdk9, 3, "_ZTV16InstanceRefKlass",
              "_ZN16InstanceRefKlass22oop_ms_adjust_pointersEP7oopDesc",
              &callbackForParallelWithMarkCheck),
    HOOK_FUNC(par_jdk9, 4, "_ZTV24InstanceClassLoaderKlass",
              "_ZN24InstanceClassLoaderKlass22oop_ms_adjust_pointersEP7oopDesc",
              &callbackForParallelWithMarkCheck),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on parallelGC for after jdk 10.
 */
THookFunctionInfo jdk10_par_hook[] = {
    HOOK_FUNC(par_jdk10, 0, "_ZTV20AdjustPointerClosure",
              "_ZN20AdjustPointerClosure6do_oopEPP7oopDesc",
              &callbackForDoOopWithMarkCheck),
    HOOK_FUNC(par_jdk10, 1, "_ZTV20AdjustPointerClosure",
              "_ZN20AdjustPointerClosure6do_oopEPj",
              &callbackForDoNarrowOopWithMarkCheck),
    HOOK_FUNC(par_jdk10, 2, "_ZTV13InstanceKlass",
              "_ZN13InstanceKlass18oop_oop_iterate_nvEP7oopDescP18MarkAndPushClosure",
              &callbackForParallelWithMarkCheck),
    HOOK_FUNC(par_jdk10, 3, "_ZTV13ObjArrayKlass",
              "_ZN13ObjArrayKlass18oop_oop_iterate_nvEP7oopDescP18MarkAndPushClosure",
              &callbackForParallelWithMarkCheck),
    HOOK_FUNC(par_jdk10, 4, "_ZTV14TypeArrayKlass",
              "_ZN14TypeArrayKlass18oop_oop_iterate_nvEP7oopDescP18MarkAndPushClosure",
              &callbackForParallelWithMarkCheck),
    HOOK_FUNC(par_jdk10, 5, "_ZTV16InstanceRefKlass",
              "_ZN16InstanceRefKlass18oop_oop_iterate_nvEP7oopDescP18MarkAndPushClosure",
              &callbackForParallelWithMarkCheck),
    HOOK_FUNC(par_jdk10, 6, "_ZTV24InstanceClassLoaderKlass",
              "_ZN24InstanceClassLoaderKlass18oop_oop_iterate_nvEP7oopDescP18MarkAndPushClosure",
              &callbackForParallelWithMarkCheck),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on parallelGC.
 */
THookFunctionInfo *par_hook = NULL;

/*!
 * \brief Pointer of hook information on parallelOldGC for default.
 */
THookFunctionInfo default_parOld_hook[] = {
    HOOK_FUNC(parOld, 0, "_ZTV13instanceKlass",
              "_ZN13instanceKlass19oop_follow_"
              "contentsEP20ParCompactionManagerP7oopDesc",
              &callbackForParOld),
    HOOK_FUNC(parOld, 1, "_ZTV13objArrayKlass",
              "_ZN13objArrayKlass19oop_follow_"
              "contentsEP20ParCompactionManagerP7oopDesc",
              &callbackForParOld),
    HOOK_FUNC(parOld, 2, "_ZTV14typeArrayKlass",
              "_ZN14typeArrayKlass19oop_follow_"
              "contentsEP20ParCompactionManagerP7oopDesc",
              &callbackForParOld),
    HOOK_FUNC(parOld, 3, "_ZTV16instanceRefKlass",
              "_ZN16instanceRefKlass19oop_follow_"
              "contentsEP20ParCompactionManagerP7oopDesc",
              &callbackForParOld),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on parallelOldGC for after CR6964458.
 */
THookFunctionInfo CR6964458_parOld_hook[] = {
    HOOK_FUNC(parOld_6964458, 0, "_ZTV13InstanceKlass",
              "_ZN13InstanceKlass19oop_follow_"
              "contentsEP20ParCompactionManagerP7oopDesc",
              &callbackForParOld),
    HOOK_FUNC(parOld_6964458, 1, "_ZTV13objArrayKlass",
              "_ZN13objArrayKlass19oop_follow_"
              "contentsEP20ParCompactionManagerP7oopDesc",
              &callbackForParOld),
    HOOK_FUNC(parOld_6964458, 2, "_ZTV14typeArrayKlass",
              "_ZN14typeArrayKlass19oop_follow_"
              "contentsEP20ParCompactionManagerP7oopDesc",
              &callbackForParOld),
    HOOK_FUNC(parOld_6964458, 3, "_ZTV16InstanceRefKlass",
              "_ZN16InstanceRefKlass19oop_follow_"
              "contentsEP20ParCompactionManagerP7oopDesc",
              &callbackForParOld),
    HOOK_FUNC(parOld_6964458, 4, "_ZTV24InstanceClassLoaderKlass",
              "_ZN24InstanceClassLoaderKlass19oop_follow_"
              "contentsEP20ParCompactionManagerP7oopDesc",
              &callbackForParOld),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on parallelOldGC for after CR8000213.
 */
THookFunctionInfo CR8000213_parOld_hook[] = {
    HOOK_FUNC(parOld_6964458, 0, "_ZTV13InstanceKlass",
              "_ZN13InstanceKlass19oop_follow_"
              "contentsEP20ParCompactionManagerP7oopDesc",
              &callbackForParOld),
    HOOK_FUNC(parOld_6964458, 1, "_ZTV13ObjArrayKlass",
              "_ZN13ObjArrayKlass19oop_follow_"
              "contentsEP20ParCompactionManagerP7oopDesc",
              &callbackForParOld),
    HOOK_FUNC(parOld_6964458, 2, "_ZTV14TypeArrayKlass",
              "_ZN14TypeArrayKlass19oop_follow_"
              "contentsEP20ParCompactionManagerP7oopDesc",
              &callbackForParOld),
    HOOK_FUNC(parOld_6964458, 3, "_ZTV16InstanceRefKlass",
              "_ZN16InstanceRefKlass19oop_follow_"
              "contentsEP20ParCompactionManagerP7oopDesc",
              &callbackForParOld),
    HOOK_FUNC(parOld_6964458, 4, "_ZTV24InstanceClassLoaderKlass",
              "_ZN24InstanceClassLoaderKlass19oop_follow_"
              "contentsEP20ParCompactionManagerP7oopDesc",
              &callbackForParOld),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on parallelOldGC for after jdk 9.
 */
THookFunctionInfo jdk9_parOld_hook[] = {
    HOOK_FUNC(parOld_jdk9, 0, "_ZTV13InstanceKlass",
              "_ZN13InstanceKlass22oop_pc_follow_contentsEP7oopDescP20ParCompactionManager",
              &callbackForParOld),
    HOOK_FUNC(parOld_jdk9, 1, "_ZTV13ObjArrayKlass",
              "_ZN13ObjArrayKlass22oop_pc_follow_contentsEP7oopDescP20ParCompactionManager",
              &callbackForParOld),
    HOOK_FUNC(parOld_jdk9, 2, "_ZTV14TypeArrayKlass",
              "_ZN14TypeArrayKlass22oop_pc_follow_contentsEP7oopDescP20ParCompactionManager",
              &callbackForParOld),
    HOOK_FUNC(parOld_jdk9, 3, "_ZTV16InstanceRefKlass",
              "_ZN16InstanceRefKlass22oop_pc_follow_contentsEP7oopDescP20ParCompactionManager",
              &callbackForParOld),
    HOOK_FUNC(parOld_jdk9, 4, "_ZTV24InstanceClassLoaderKlass",
              "_ZN24InstanceClassLoaderKlass22oop_pc_follow_contentsEP7oopDescP20ParCompactionManager",
              &callbackForParOld),
    HOOK_FUNC_END};

/*!
 * \brief Pointers of hook information on parallelOldGC for several CRs.<br>
 *        These CRs have no impact on parallelOldGC, so refer to the last.
 */
#define CR8027746_parOld_hook CR8000213_parOld_hook
#define CR8049421_parOld_hook CR8000213_parOld_hook
#define jdk10_parOld_hook jdk9_parOld_hook

/*!
 * \brief Pointer of hook information on parallelOldGC.
 */
THookFunctionInfo *parOld_hook = NULL;

/*!
 * \brief Pointer of hook information on CMSGC for default.
 */
THookFunctionInfo default_cms_sweep_hook[] = {
    HOOK_FUNC(cms_sweep, 0, "_ZTV12SweepClosure",
              "_ZN12SweepClosure14do_blk_carefulEP8HeapWord",
              &callbackForSweep),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on CMSGC.
 */
THookFunctionInfo *cms_sweep_hook = default_cms_sweep_hook;

/*!
 * \brief Pointer of hook information on CMSGC for default.
 */
THookFunctionInfo default_cms_new_hook[] = {
    HOOK_FUNC(cms_new, 0, "_ZTV13instanceKlass",
              "_ZN13instanceKlass18oop_oop_iterate_nvEP7oopDescP30Par_"
              "MarkRefsIntoAndScanClosure",
              &callbackForIterate),
    HOOK_FUNC(cms_new, 1, "_ZTV13objArrayKlass",
              "_ZN13objArrayKlass18oop_oop_iterate_nvEP7oopDescP30Par_"
              "MarkRefsIntoAndScanClosure",
              &callbackForIterate),
    HOOK_FUNC(cms_new, 2, "_ZTV14typeArrayKlass",
              "_ZN14typeArrayKlass15oop_oop_iterateEP7oopDescP10OopClosure",
              &callbackForIterate),
    HOOK_FUNC(cms_new, 3, "_ZTV16instanceRefKlass",
              "_ZN16instanceRefKlass18oop_oop_iterate_nvEP7oopDescP30Par_"
              "MarkRefsIntoAndScanClosure",
              &callbackForIterate),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on CMSGC for after CR6964458.
 */
THookFunctionInfo CR6964458_cms_new_hook[] = {
    HOOK_FUNC(cms_new_6964458, 0, "_ZTV13InstanceKlass",
              "_ZN13InstanceKlass18oop_oop_iterate_nvEP7oopDescP30Par_"
              "MarkRefsIntoAndScanClosure",
              &callbackForIterate),
    HOOK_FUNC(cms_new_6964458, 1, "_ZTV13objArrayKlass",
              "_ZN13objArrayKlass18oop_oop_iterate_nvEP7oopDescP30Par_"
              "MarkRefsIntoAndScanClosure",
              &callbackForIterate),
    HOOK_FUNC(
        cms_new_6964458, 2, "_ZTV14typeArrayKlass",
        "_ZN14typeArrayKlass15oop_oop_iterateEP7oopDescP18ExtendedOopClosure",
        &callbackForIterate),
    HOOK_FUNC(cms_new_6964458, 3, "_ZTV16InstanceRefKlass",
              "_ZN16InstanceRefKlass18oop_oop_iterate_nvEP7oopDescP30Par_"
              "MarkRefsIntoAndScanClosure",
              &callbackForIterate),
    HOOK_FUNC(cms_new_6964458, 4, "_ZTV24InstanceClassLoaderKlass",
              "_ZN24InstanceClassLoaderKlass18oop_oop_iterate_nvEP7oopDescP30"
              "Par_MarkRefsIntoAndScanClosure",
              &callbackForIterate),
    HOOK_FUNC_END};

/*!
* \brief Pointer of hook information on CMSGC for after CR8000213.
*/
THookFunctionInfo CR8000213_cms_new_hook[] = {
    HOOK_FUNC(cms_new_6964458, 0, "_ZTV13InstanceKlass",
              "_ZN13InstanceKlass18oop_oop_iterate_nvEP7oopDescP30Par_"
              "MarkRefsIntoAndScanClosure",
              &callbackForIterate),
    HOOK_FUNC(cms_new_6964458, 1, "_ZTV13ObjArrayKlass",
              "_ZN13ObjArrayKlass18oop_oop_iterate_nvEP7oopDescP30Par_"
              "MarkRefsIntoAndScanClosure",
              &callbackForIterate),
    HOOK_FUNC(
        cms_new_6964458, 2, "_ZTV14TypeArrayKlass",
        "_ZN14TypeArrayKlass15oop_oop_iterateEP7oopDescP18ExtendedOopClosure",
        &callbackForIterate),
    HOOK_FUNC(cms_new_6964458, 3, "_ZTV16InstanceRefKlass",
              "_ZN16InstanceRefKlass18oop_oop_iterate_nvEP7oopDescP30Par_"
              "MarkRefsIntoAndScanClosure",
              &callbackForIterate),
    HOOK_FUNC(cms_new_6964458, 4, "_ZTV24InstanceClassLoaderKlass",
              "_ZN24InstanceClassLoaderKlass18oop_oop_iterate_nvEP7oopDescP30"
              "Par_MarkRefsIntoAndScanClosure",
              &callbackForIterate),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on CMSGC for JDK 9.
 */
THookFunctionInfo jdk9_cms_new_hook[] = {
    HOOK_FUNC(cms_new_jdk9, 0, "_ZTV13InstanceKlass",
              "_ZN13InstanceKlass17oop_oop_iterate_vEP7oopDescP18ExtendedOopClosure",
              &callbackForIterate),
    HOOK_FUNC(cms_new_jdk9, 1, "_ZTV13ObjArrayKlass",
              "_ZN13ObjArrayKlass17oop_oop_iterate_vEP7oopDescP18ExtendedOopClosure",
              &callbackForIterate),
    HOOK_FUNC(
        cms_new_jdk9, 2, "_ZTV14TypeArrayKlass",
        "_ZN14TypeArrayKlass17oop_oop_iterate_vEP7oopDescP18ExtendedOopClosure",
        &callbackForIterate),
    HOOK_FUNC(cms_new_jdk9, 3, "_ZTV16InstanceRefKlass",
              "_ZN16InstanceRefKlass17oop_oop_iterate_vEP7oopDescP18ExtendedOopClosure",
              &callbackForIterate),
    HOOK_FUNC(cms_new_jdk9, 4, "_ZTV24InstanceClassLoaderKlass",
              "_ZN24InstanceClassLoaderKlass17oop_oop_iterate_vEP7oopDescP18ExtendedOopClosure",
              &callbackForIterate),
    HOOK_FUNC_END};

/*!
* \brief Pointers of hook information on CMSGC for several CRs.<br>
*        These CRs have no impact on CMSGC, so refer to the last.
*/
#define CR8027746_cms_new_hook CR8000213_cms_new_hook
#define CR8049421_cms_new_hook CR8000213_cms_new_hook
#define jdk10_cms_new_hook jdk9_cms_new_hook

/*!
 * \brief Pointer of hook information on CMSGC.
 */
THookFunctionInfo *cms_new_hook = NULL;

/*!
 * \brief Pointer of hook information on G1GC for default.
 */
THookFunctionInfo default_g1_hook[] = {
    HOOK_FUNC(g1, 0, "_ZTV16G1ParCopyClosureILb0EL9G1Barrier0ELb1EE",
              "_ZN16G1ParCopyClosureILb0EL9G1Barrier0ELb1EE6do_oopEPP7oopDesc",
              &callbackForDoOop),
    HOOK_FUNC(g1, 1, "_ZTV16G1ParCopyClosureILb0EL9G1Barrier0ELb1EE",
              "_ZN16G1ParCopyClosureILb0EL9G1Barrier0ELb1EE6do_oopEPj",
              &callbackForDoNarrowOop),
    HOOK_FUNC(g1, 2, "_ZTV13instanceKlass",
              "_ZN13instanceKlass18oop_oop_iterate_"
              "nvEP7oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(g1, 3, "_ZTV13objArrayKlass",
              "_ZN13objArrayKlass18oop_oop_iterate_"
              "nvEP7oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(g1, 4, "_ZTV16instanceRefKlass",
              "_ZN16instanceRefKlass18oop_oop_iterate_"
              "nvEP7oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(
        g1, 5, "_ZTV13instanceKlass",
        "_ZN13instanceKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1, 6, "_ZTV13objArrayKlass",
        "_ZN13objArrayKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(g1, 7, "_ZTV14typeArrayKlass",
              "_ZN14typeArrayKlass15oop_oop_iterateEP7oopDescP10OopClosure",
              &callbackForIterate),
    HOOK_FUNC(
        g1, 8, "_ZTV16instanceRefKlass",
        "_ZN16instanceRefKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on G1GC for after CR6964458.
 */
THookFunctionInfo CR6964458_g1_hook[] = {
    HOOK_FUNC(g1_6964458, 0, "_ZTV16G1ParCopyClosureILb0EL9G1Barrier0ELb1EE",
              "_ZN16G1ParCopyClosureILb0EL9G1Barrier0ELb1EE6do_oopEPP7oopDesc",
              &callbackForDoOop),
    HOOK_FUNC(g1_6964458, 1, "_ZTV16G1ParCopyClosureILb0EL9G1Barrier0ELb1EE",
              "_ZN16G1ParCopyClosureILb0EL9G1Barrier0ELb1EE6do_oopEPj",
              &callbackForDoNarrowOop),
    HOOK_FUNC(g1_6964458, 2, "_ZTV13InstanceKlass",
              "_ZN13InstanceKlass18oop_oop_iterate_"
              "nvEP7oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(g1_6964458, 3, "_ZTV13objArrayKlass",
              "_ZN13objArrayKlass18oop_oop_iterate_"
              "nvEP7oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(g1_6964458, 4, "_ZTV16InstanceRefKlass",
              "_ZN16InstanceRefKlass18oop_oop_iterate_"
              "nvEP7oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(g1_6964458, 5, "_ZTV24InstanceClassLoaderKlass",
              "_ZN24InstanceClassLoaderKlass18oop_oop_iterate_nvEP7"
              "oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 6, "_ZTV13InstanceKlass",
        "_ZN13InstanceKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 7, "_ZTV13objArrayKlass",
        "_ZN13objArrayKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 8, "_ZTV14typeArrayKlass",
        "_ZN14typeArrayKlass15oop_oop_iterateEP7oopDescP18ExtendedOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 9, "_ZTV16InstanceRefKlass",
        "_ZN16InstanceRefKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 10, "_ZTV24InstanceClassLoaderKlass",
        "_ZN24InstanceClassLoaderKlass18oop_oop_iterate_nvEP7"
        "oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on G1GC for after CR8000213.
 */
THookFunctionInfo CR8000213_g1_hook[] = {
    HOOK_FUNC(g1_6964458, 0, "_ZTV16G1ParCopyClosureILb0EL9G1Barrier0ELb1EE",
              "_ZN16G1ParCopyClosureILb0EL9G1Barrier0ELb1EE6do_oopEPP7oopDesc",
              &callbackForDoOop),
    HOOK_FUNC(g1_6964458, 1, "_ZTV16G1ParCopyClosureILb0EL9G1Barrier0ELb1EE",
              "_ZN16G1ParCopyClosureILb0EL9G1Barrier0ELb1EE6do_oopEPj",
              &callbackForDoNarrowOop),
    HOOK_FUNC(g1_6964458, 2, "_ZTV13InstanceKlass",
              "_ZN13InstanceKlass18oop_oop_iterate_"
              "nvEP7oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(g1_6964458, 3, "_ZTV13ObjArrayKlass",
              "_ZN13ObjArrayKlass18oop_oop_iterate_"
              "nvEP7oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(g1_6964458, 4, "_ZTV16InstanceRefKlass",
              "_ZN16InstanceRefKlass18oop_oop_iterate_"
              "nvEP7oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(g1_6964458, 5, "_ZTV24InstanceClassLoaderKlass",
              "_ZN24InstanceClassLoaderKlass18oop_oop_iterate_nvEP7"
              "oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 6, "_ZTV13InstanceKlass",
        "_ZN13InstanceKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 7, "_ZTV13ObjArrayKlass",
        "_ZN13ObjArrayKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 8, "_ZTV14TypeArrayKlass",
        "_ZN14TypeArrayKlass15oop_oop_iterateEP7oopDescP18ExtendedOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 9, "_ZTV16InstanceRefKlass",
        "_ZN16InstanceRefKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 10, "_ZTV24InstanceClassLoaderKlass",
        "_ZN24InstanceClassLoaderKlass18oop_oop_iterate_nvEP7"
        "oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on G1GC for after CR8027746.
 */
THookFunctionInfo CR8027746_g1_hook[] = {
    HOOK_FUNC(g1_6964458, 0, "_ZTV16G1ParCopyClosureIL9G1Barrier0ELb1EE",
              "_ZN16G1ParCopyClosureIL9G1Barrier0ELb1EE6do_oopEPP7oopDesc",
              &callbackForDoOop),
    HOOK_FUNC(g1_6964458, 1, "_ZTV16G1ParCopyClosureIL9G1Barrier0ELb1EE",
              "_ZN16G1ParCopyClosureIL9G1Barrier0ELb1EE6do_oopEPj",
              &callbackForDoNarrowOop),
    HOOK_FUNC(g1_6964458, 2, "_ZTV13InstanceKlass",
              "_ZN13InstanceKlass18oop_oop_iterate_"
              "nvEP7oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(g1_6964458, 3, "_ZTV13ObjArrayKlass",
              "_ZN13ObjArrayKlass18oop_oop_iterate_"
              "nvEP7oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(g1_6964458, 4, "_ZTV16InstanceRefKlass",
              "_ZN16InstanceRefKlass18oop_oop_iterate_"
              "nvEP7oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(g1_6964458, 5, "_ZTV24InstanceClassLoaderKlass",
              "_ZN24InstanceClassLoaderKlass18oop_oop_iterate_nvEP7"
              "oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 6, "_ZTV13InstanceKlass",
        "_ZN13InstanceKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 7, "_ZTV13ObjArrayKlass",
        "_ZN13ObjArrayKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 8, "_ZTV14TypeArrayKlass",
        "_ZN14TypeArrayKlass15oop_oop_iterateEP7oopDescP18ExtendedOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 9, "_ZTV16InstanceRefKlass",
        "_ZN16InstanceRefKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 10, "_ZTV24InstanceClassLoaderKlass",
        "_ZN24InstanceClassLoaderKlass18oop_oop_iterate_nvEP7"
        "oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on G1GC for after JDK-8049421.
 */
THookFunctionInfo CR8049421_g1_hook[] = {
    HOOK_FUNC(
        g1_6964458, 0, "_ZTV16G1ParCopyClosureIL9G1Barrier0EL6G1Mark1EE",
        "_ZN16G1ParCopyClosureIL9G1Barrier0EL6G1Mark1EE6do_oopEPP7oopDesc",
        &callbackForDoOop),
    HOOK_FUNC(g1_6964458, 1, "_ZTV16G1ParCopyClosureIL9G1Barrier0EL6G1Mark1EE",
              "_ZN16G1ParCopyClosureIL9G1Barrier0EL6G1Mark1EE6do_oopEPj",
              &callbackForDoNarrowOop),
    HOOK_FUNC(g1_6964458, 2, "_ZTV13InstanceKlass",
              "_ZN13InstanceKlass18oop_oop_iterate_"
              "nvEP7oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(g1_6964458, 3, "_ZTV13ObjArrayKlass",
              "_ZN13ObjArrayKlass18oop_oop_iterate_"
              "nvEP7oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(g1_6964458, 4, "_ZTV16InstanceRefKlass",
              "_ZN16InstanceRefKlass18oop_oop_iterate_"
              "nvEP7oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(g1_6964458, 5, "_ZTV24InstanceClassLoaderKlass",
              "_ZN24InstanceClassLoaderKlass18oop_oop_iterate_nvEP7"
              "oopDescP23G1RootRegionScanClosure",
              &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 6, "_ZTV13InstanceKlass",
        "_ZN13InstanceKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 7, "_ZTV13ObjArrayKlass",
        "_ZN13ObjArrayKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 8, "_ZTV14TypeArrayKlass",
        "_ZN14TypeArrayKlass15oop_oop_iterateEP7oopDescP18ExtendedOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 9, "_ZTV16InstanceRefKlass",
        "_ZN16InstanceRefKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_6964458, 10, "_ZTV24InstanceClassLoaderKlass",
        "_ZN24InstanceClassLoaderKlass18oop_oop_iterate_nvEP7"
        "oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on G1GC for after JDK 9.
 */
THookFunctionInfo jdk9_g1_hook[] = {
    HOOK_FUNC(
        g1_jdk9, 0, "_ZTV16G1ParCopyClosureIL9G1Barrier0EL6G1Mark1ELb0EE",
        "_ZN16G1ParCopyClosureIL9G1Barrier0EL6G1Mark1ELb0EE6do_oopEPP7oopDesc",
        &callbackForDoOop),
    HOOK_FUNC(
        g1_jdk9, 1, "_ZTV16G1ParCopyClosureIL9G1Barrier0EL6G1Mark1ELb0EE",
        "_ZN16G1ParCopyClosureIL9G1Barrier0EL6G1Mark1ELb0EE6do_oopEPj",
        &callbackForDoNarrowOop),
    HOOK_FUNC(
        g1_jdk9, 2, "_ZTV13InstanceKlass",
        "_ZN13InstanceKlass18oop_oop_iterate_nvEP7oopDescP23G1RootRegionScanClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk9, 3, "_ZTV13ObjArrayKlass",
        "_ZN13ObjArrayKlass18oop_oop_iterate_nvEP7oopDescP23G1RootRegionScanClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk9, 4, "_ZTV14TypeArrayKlass",
        "_ZN14TypeArrayKlass18oop_oop_iterate_nvEP7oopDescP23G1RootRegionScanClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk9, 5, "_ZTV16InstanceRefKlass",
        "_ZN16InstanceRefKlass18oop_oop_iterate_nvEP7oopDescP23G1RootRegionScanClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk9, 6, "_ZTV24InstanceClassLoaderKlass",
        "_ZN24InstanceClassLoaderKlass18oop_oop_iterate_nvEP7oopDescP23G1RootRegionScanClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk9, 7, "_ZTV13InstanceKlass",
        "_ZN13InstanceKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk9, 8, "_ZTV13ObjArrayKlass",
        "_ZN13ObjArrayKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk9, 9, "_ZTV14TypeArrayKlass",
        "_ZN14TypeArrayKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk9, 10, "_ZTV16InstanceRefKlass",
        "_ZN16InstanceRefKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk9, 11, "_ZTV24InstanceClassLoaderKlass",
        "_ZN24InstanceClassLoaderKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on G1GC for after JDK 10.
 */
THookFunctionInfo jdk10_g1_hook[] = {
    HOOK_FUNC(
        g1_jdk10, 0, "_ZTV16G1ParCopyClosureIL9G1Barrier0EL6G1Mark1ELb0EE",
        "_ZN16G1ParCopyClosureIL9G1Barrier0EL6G1Mark1ELb0EE6do_oopEPP7oopDesc",
        &callbackForDoOop),
    HOOK_FUNC(
        g1_jdk10, 1, "_ZTV16G1ParCopyClosureIL9G1Barrier0EL6G1Mark1ELb0EE",
        "_ZN16G1ParCopyClosureIL9G1Barrier0EL6G1Mark1ELb0EE6do_oopEPj",
        &callbackForDoNarrowOop),
    HOOK_FUNC(
        g1_jdk10, 2, "_ZTV13InstanceKlass",
        "_ZN13InstanceKlass18oop_oop_iterate_nvEP7oopDescP23G1RootRegionScanClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk10, 3, "_ZTV13ObjArrayKlass",
        "_ZN13ObjArrayKlass18oop_oop_iterate_nvEP7oopDescP23G1RootRegionScanClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk10, 4, "_ZTV14TypeArrayKlass",
        "_ZN14TypeArrayKlass18oop_oop_iterate_nvEP7oopDescP23G1RootRegionScanClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk10, 5, "_ZTV16InstanceRefKlass",
        "_ZN16InstanceRefKlass18oop_oop_iterate_nvEP7oopDescP23G1RootRegionScanClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk10, 6, "_ZTV24InstanceClassLoaderKlass",
        "_ZN24InstanceClassLoaderKlass18oop_oop_iterate_nvEP7oopDescP23G1RootRegionScanClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk10, 7, "_ZTV13InstanceKlass",
        "_ZN13InstanceKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk10, 8, "_ZTV13ObjArrayKlass",
        "_ZN13ObjArrayKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk10, 9, "_ZTV14TypeArrayKlass",
        "_ZN14TypeArrayKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk10, 10, "_ZTV16InstanceRefKlass",
        "_ZN16InstanceRefKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk10, 11, "_ZTV24InstanceClassLoaderKlass",
        "_ZN24InstanceClassLoaderKlass18oop_oop_iterate_nvEP7oopDescP14G1CMOopClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk10, 12, "_ZTV20G1MarkAndPushClosure",
        "_ZN20G1MarkAndPushClosure6do_oopEPP7oopDesc",
        &callbackForDoOop),
    HOOK_FUNC(
        g1_jdk10, 13, "_ZTV20G1MarkAndPushClosure",
        "_ZN20G1MarkAndPushClosure6do_oopEPj",
        &callbackForDoNarrowOop),
    HOOK_FUNC(
        g1_jdk10, 14, "_ZTV13InstanceKlass",
        "_ZN13InstanceKlass18oop_oop_iterate_nvEP7oopDescP20G1MarkAndPushClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk10, 15, "_ZTV13ObjArrayKlass",
        "_ZN13ObjArrayKlass18oop_oop_iterate_nvEP7oopDescP20G1MarkAndPushClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk10, 16, "_ZTV14TypeArrayKlass",
        "_ZN14TypeArrayKlass18oop_oop_iterate_nvEP7oopDescP20G1MarkAndPushClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk10, 17, "_ZTV16InstanceRefKlass",
        "_ZN16InstanceRefKlass18oop_oop_iterate_nvEP7oopDescP20G1MarkAndPushClosure",
        &callbackForIterate),
    HOOK_FUNC(
        g1_jdk10, 18, "_ZTV24InstanceClassLoaderKlass",
        "_ZN24InstanceClassLoaderKlass18oop_oop_iterate_nvEP7oopDescP20G1MarkAndPushClosure",
        &callbackForIterate),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on G1GC.
 */
THookFunctionInfo *g1_hook = NULL;

/*!
 * \brief Pointer of hook information on G1GC cleanup event for default.
 */
THookFunctionInfo default_g1Event_hook[] = {
    HOOK_FUNC(g1Event, 0, "_ZTV9CMCleanUp", "_ZN9CMCleanUp7do_voidEv",
              &callbackForG1Cleanup),
    HOOK_FUNC(g1Event, 1, "_ZTV16VM_G1CollectFull",
              "_ZN15VM_GC_Operation13doit_prologueEv", &callbackForG1Full),
    HOOK_FUNC(g1Event, 2, "_ZTV16VM_G1CollectFull",
              "_ZN15VM_GC_Operation13doit_epilogueEv", &callbackForG1FullReturn),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook information on G1GC cleanup event.
 */
THookFunctionInfo *g1Event_hook = default_g1Event_hook;

/*!
 * \brief Pointer of hook change and adjust oop with GC for default.
 */
THookFunctionInfo default_adj_hook[] = {
    HOOK_FUNC(adj, 0, "_ZTV18instanceKlassKlass",
              "_ZN18instanceKlassKlass19oop_adjust_pointersEP7oopDesc",
              &callbackForAdjustPtr),
    HOOK_FUNC(adj, 1, "_ZTV18objArrayKlassKlass",
              "_ZN18objArrayKlassKlass19oop_adjust_pointersEP7oopDesc",
              &callbackForAdjustPtr),
    HOOK_FUNC(adj, 2, "_ZTV15arrayKlassKlass",
              "_ZN15arrayKlassKlass19oop_adjust_pointersEP7oopDesc",
              &callbackForAdjustPtr),

    HOOK_FUNC(adj, 3, "_ZTV20MoveAndUpdateClosure",
#ifdef __LP64__
              "_ZN20MoveAndUpdateClosure7do_addrEP8HeapWordm",
#else
              "_ZN20MoveAndUpdateClosure7do_addrEP8HeapWordj",
#endif
              &callbackForDoAddr),
    HOOK_FUNC(adj, 4, "_ZTV17UpdateOnlyClosure",
#ifdef __LP64__
              "_ZN17UpdateOnlyClosure7do_addrEP8HeapWordm",
#else
              "_ZN17UpdateOnlyClosure7do_addrEP8HeapWordj",
#endif
              &callbackForDoAddr),

    HOOK_FUNC(adj, 5, "_ZTV18instanceKlassKlass",
              "_ZN18instanceKlassKlass19oop_update_"
              "pointersEP20ParCompactionManagerP7oopDesc",
              &callbackForUpdatePtr),
    HOOK_FUNC(adj, 6, "_ZTV18objArrayKlassKlass",
              "_ZN18objArrayKlassKlass19oop_update_"
              "pointersEP20ParCompactionManagerP7oopDesc",
              &callbackForUpdatePtr),
    HOOK_FUNC(adj, 7, "_ZTV15arrayKlassKlass",
              "_ZN15arrayKlassKlass19oop_update_"
              "pointersEP20ParCompactionManagerP7oopDesc",
              &callbackForUpdatePtr),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook change and adjust oop with GC.
 */
THookFunctionInfo *adj_hook = default_adj_hook;

/*!
 * \brief Pointer of hook oop iterate with JVMTI HeapOverIterate for default.
 */
THookFunctionInfo default_jvmti_hook[] = {
    HOOK_FUNC(jvmti, 0, "_ZTV28IterateOverHeapObjectClosure",
              "_ZN28IterateOverHeapObjectClosure9do_objectEP7oopDesc",
              &callbackForJvmtiIterate),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook oop iterate with JVMTI HeapOverIterate.
 */
THookFunctionInfo *jvmti_hook = default_jvmti_hook;

/*!
 * \brief Pointer of hook inner GC function for default.
 */
THookFunctionInfo default_innerStart_hook[] = {
    HOOK_FUNC(innerStart, 0, "_ZTV20ParallelScavengeHeap",
              "_ZN20ParallelScavengeHeap31accumulate_statistics_all_tlabsEv",
              &callbackForInnerGCStart),
    HOOK_FUNC(innerStart, 1, "_ZTV13CollectedHeap",
              "_ZN13CollectedHeap31accumulate_statistics_all_tlabsEv",
              &callbackForInnerGCStart),
    HOOK_FUNC(innerStart, 2, "_ZTV16GenCollectedHeap",
              "_ZN16GenCollectedHeap11gc_prologueEb", &callbackForInnerGCStart),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook inner GC function.
 */
THookFunctionInfo *innerStart_hook = default_innerStart_hook;

/*!
 * \brief Pointer of WatcherThread hook.
 */
THookFunctionInfo default_watcherThread_hook[] = {
    HOOK_FUNC(watcherThread, 0, "_ZTV13WatcherThread",
              "_ZN13WatcherThread3runEv", &callbackForWatcherThreadRun),
    HOOK_FUNC_END};

/*!
 * \brief Pointer of hook WatcherThread.
 */
THookFunctionInfo *watcherThread_hook = default_watcherThread_hook;

/* Event callback for outter. */

/*!
 * \brief Callback function for general GC function hooking.
 */
THeapObjectCallback gcCallbackFunc = NULL;

/*!
 * \brief Callback function for CMSGC function hooking.
 */
THeapObjectCallback cmsCallbackFunc = NULL;

/*!
 * \brief Callback function for JVMTI iterate hooking.
 */
THeapObjectCallback jvmtiIteCallbackFunc = NULL;

/*!
 * \brief Callback function for adjust oop function hooking.
 */
TKlassAdjustCallback adjustCallbackFunc = NULL;

/*!
 * \brief Callback function for finished G1GC.
 */
TCommonCallback g1FinishCallbackFunc = NULL;

/*!
 * \brief Callback function for interrupt GC.
 */
TCommonCallback gcInterruptCallbackFunc = NULL;

/* Variable for inner work. */

/*!
 * \brief flag of snapshot with CMS phase.
 */
bool needSnapShotByCMSPhase = false;

/*!
 * \brief flag of GC hooking enable.
 */
bool isEnableGCHooking = false;

/*!
 * \brief flag of JVMTI hooking enable.
 */
bool isEnableJvmtiHooking = false;

/*!
 * \brief Pthread key to store old klassOop.
 */
pthread_key_t oldKlassOopKey;

/*!
 * \brief flag of inner GC function is called many times.
 */
bool isInvokeGCManyTimes = false;

/*!
 * \brief Flag of called parallel gc on using CMS gc.
 */
bool isInvokedParallelGC = false;

/* Function prototypes */
/****************************************************************/
void pThreadKeyDestructor(void *data);
bool setupForParallel(void);
bool setupForParallelOld(void);
bool setupForCMS(void);
bool setupForG1(size_t maxMemSize);

/* Utility functions for overriding */
/****************************************************************/

/*!
 * \brief Initialize override functions.
 * \return Process result.
 */
bool initOverrider(void) {
  bool result = true;

  try {
    if (TVMVariables::getInstance()->getUseG1()) {
      if (unlikely(!jvmInfo->isAfterCR7046558())) {
        /* Heapstats agent is unsupported G1GC on JDK6. */
        logger->printCritMsg("G1GC isn't supported in this version.");
        logger->printCritMsg("You should use HotSpot >= 22.0-b03");
        throw 1;
      }

      if (conf->TimerInterval()->get() > 0) {
        logger->printWarnMsg(
            "Interval SnapShot is not supported with G1GC. Turn off.");
        conf->TimerInterval()->set(0);
      }

      if (conf->TriggerOnDump()->get()) {
        logger->printWarnMsg(
            "SnapShot trigger on dump request is not supported with G1GC. "
            "Turn off.");
        conf->TriggerOnDump()->set(false);
      }
    }

    /* Create pthred key for klassOop. */
    if (unlikely(pthread_key_create(&oldKlassOopKey, pThreadKeyDestructor) !=
                 0)) {
      logger->printCritMsg("Cannot create pthread key.");
      throw 2;
    }

  } catch (...) {
    result = false;
  }

  return result;
}

/*!
 * \brief Cleanup override functions.
 */
void cleanupOverrider(void) {
  /* Cleanup. */
  TVMVariables *vmVal = TVMVariables::getInstance();
  if (vmVal->getUseCMS() || vmVal->getUseG1()) {
    delete checkObjectMap;
    checkObjectMap = NULL;
  }

  symFinder = NULL;
  pthread_key_delete(oldKlassOopKey);
}

/*!
 * \brief Callback for pthread key destructor.
 * \param data [in] Data related to pthread key.
 */
void pThreadKeyDestructor(void *data) {
  if (likely(data != NULL)) {
    /* Cleanup allocated memory on "callbackForDoAddr". */
    free(data);
  }
}

/*!
 * \brief Check CMS garbage collector state.
 * \param state        [in]  State of CMS garbage collector.
 * \param needSnapShot [out] Is need snapshot now.
 * \return CMS collector state.
 * \warning Please don't forget call on JVM death.
 */
int checkCMSState(TGCState state, bool *needSnapShot) {
  /* Sanity check. */
  if (unlikely(needSnapShot == NULL)) {
    logger->printCritMsg("Illegal paramter!");
    return -1;
  }

  TVMVariables *vmVal = TVMVariables::getInstance();
  static int cmsStateAtStart;
  *needSnapShot = false;
  switch (state) {
    case gcStart:
      /* Call on JVMTI GC start. */

      if (vmVal->getCMS_collectorState() <= CMS_INITIALMARKING) {
        /* Set stored flag of passed sweep phase. */
        *needSnapShot = needSnapShotByCMSPhase;
        needSnapShotByCMSPhase = false;
      } else if (vmVal->getCMS_collectorState() == CMS_FINALMARKING) {
        checkObjectMap->clear();

        /* switch hooking for CMS new generation. */
        switchOverrideFunction(cms_new_hook, true);
      }

      cmsStateAtStart = vmVal->getCMS_collectorState();
      isInvokedParallelGC = false;
      break;

    case gcFinish:
      /* Call on JVMTI GC finished. */
      switch (vmVal->getCMS_collectorState()) {
        case CMS_MARKING:
          /* CMS marking phase. */
          break;

        case CMS_SWEEPING:
          /* CMS sweep phase. */
          needSnapShotByCMSPhase = true;
          break;

        default:
          /* If called parallel gc. */
          if (isInvokedParallelGC) {
            /* GC invoke by user or service without CMS. */
            (*needSnapShot) = true;
            needSnapShotByCMSPhase = false;
            break;
          }
      }

      /* If enable cms new gen hook. */
      if (cmsStateAtStart == CMS_FINALMARKING) {
        /* switch hooking for CMS new generation. */
        switchOverrideFunction(cms_new_hook, false);
      }

      break;

    case gcLast:
      /* Call on JVM is terminating. */

      /* Set stored flag of passed sweep phase. */
      *needSnapShot = needSnapShotByCMSPhase;
      needSnapShotByCMSPhase = false;
      break;

    default:
      /* Not reached here. */
      logger->printWarnMsg("Illegal GC state.");
  }

  return vmVal->getCMS_collectorState();
}

/*!
 * \brief Is marked object on CMS bitmap.
 * \param oop [in] Java heap object(Inner class format).
 * \return Value is true, if object is marked.
 */
inline bool isMarkedObject(void *oop) {
  TVMVariables *vmVal = TVMVariables::getInstance();

  size_t idx = (((ptrdiff_t)oop - (ptrdiff_t)vmVal->getCmsBitMap_startWord()) >>
                ((unsigned)vmVal->getLogHeapWordSize() +
                 (unsigned)vmVal->getCmsBitMap_shifter()));
  size_t mask = (size_t)1 << (idx & vmVal->getBitsPerWordMask());
  return (((size_t *)vmVal->getCmsBitMap_startAddr())[(
              idx >> vmVal->getLogBitsPerWord())] &
          mask) != 0;
}

/*!
 * \brief Callback function for klassOop.
 * \param oldOop [in] Old java class object.
 * \param newOop [in] New java class object.
 * \warning Param "oop" isn't usable for JVMTI and JNI.
 */
inline void callbackForAdjustKlass(void *oldOop, void *newOop) {
  if (likely(adjustCallbackFunc != NULL)) {
    adjustCallbackFunc(oldOop, newOop);
  }
}

/* Override controller. */
/****************************************************************/

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
               size_t maxMemSize) {
  /* Set function. */
  gcCallbackFunc = funcOnGC;
  cmsCallbackFunc = funcOnCMS;
  jvmtiIteCallbackFunc = funcOnJVMTI;
  adjustCallbackFunc = funcOnAdjust;
  g1FinishCallbackFunc = funcOnG1GC;

  /* Setup for WatcherThread. */
  if (unlikely(!setupOverrideFunction(watcherThread_hook))) {
    logger->printWarnMsg("Cannot setup to override WatcherThread");
    return false;
  }
  switchOverrideFunction(watcherThread_hook, true);

  /* Setup for JVMTI. */
  if (unlikely(!setupOverrideFunction(jvmti_hook))) {
    logger->printWarnMsg("Cannot setup to override JVMTI GC");
    return false;
  }

  /* Setup for pointer adjustment. */
  if (!jvmInfo->isAfterCR6964458()) {
    if (unlikely(!setupOverrideFunction(adj_hook))) {
      logger->printWarnMsg("Cannot setup to override Class adjuster");
      return false;
    }
  }

  /* Setup by using GC type. */

  /* Setup parallel GC and GC by user. */
  if (unlikely(!setupForParallel())) {
    return false;
  }

  /* Setup for inner GC function. */
  if (unlikely(!setupOverrideFunction(innerStart_hook))) {
    logger->printWarnMsg("Cannot setup to override innner-hook functions.");
    return false;
  }

  TVMVariables *vmVal = TVMVariables::getInstance();

  /* Setup each GC type. */
  bool result = vmVal->getUseParallel();
  if (vmVal->getUseParOld()) {
    result &= setupForParallelOld();
  } else if (vmVal->getUseCMS()) {
    result &= setupForCMS();
  } else if (vmVal->getUseG1()) {
    result &= setupForG1(maxMemSize);
  }

  return result;
}

/*!
 * \brief Setup override funtion.
 * \param list  [in]  List of hooking information.
 * \return Process result.
 */
bool setupOverrideFunction(THookFunctionInfo *list) {
  /* Variable for list pointer. */
  THookFunctionInfo *arr = list;

  /* Search class and function symbol. */
  for (int i = 0; arr->vtableSymbol != NULL; i++) {
    /* Search class(vtable) symbol. */
    arr->vtable = (void **)symFinder->findSymbol(arr->vtableSymbol);
    if (unlikely(arr->vtable == NULL)) {
      logger->printCritMsg("%s not found.", arr->vtableSymbol);
      return false;
    }

    /* Search function symbol. */
    arr->originalFunc = symFinder->findSymbol(arr->funcSymbol);
    if (unlikely(arr->originalFunc == NULL)) {
      logger->printCritMsg("%s not found.", arr->funcSymbol);
      return false;
    }

    /* Set function pointers to gobal variables through THookFunctionInfo. */
    *arr->originalFuncPtr = arr->originalFunc;
    *arr->enterFuncPtr = arr->enterFunc;

    /* Move next list item. */
    arr++;
  }

  return true;
}

/*!
 * \brief Setup GC hooking for parallel.
 * \return Process result.
 */
bool setupForParallel(void) {
  SELECT_HOOK_FUNCS(par);

  if (unlikely(!setupOverrideFunction(par_hook))) {
    logger->printCritMsg("Cannot setup to override ParallelGC.");
    return false;
  }

  return true;
}

/*!
 * \brief Setup GC hooking for parallel old GC.
 * \return Process result.
 */
bool setupForParallelOld(void) {
  SELECT_HOOK_FUNCS(parOld);

  if (unlikely(!setupOverrideFunction(parOld_hook))) {
    logger->printCritMsg("Cannot setup to override ParallelOldGC.");
    return false;
  }

  return true;
}

/*!
 * \brief Setup GC hooking for CMSGC.
 * \return Process result.
 */
bool setupForCMS(void) {
  SELECT_HOOK_FUNCS(cms_new);

  TVMVariables *vmVal = TVMVariables::getInstance();
  const void *startAddr = vmVal->getYoungGenStartAddr();
  const size_t youngSize = vmVal->getYoungGenSize();

/* Create bitmap to check object collected flag. */
#ifdef AVX
    checkObjectMap = new TAVXBitMapMarker(startAddr, youngSize);
#elif(defined SSE2) || (defined SSE4)
    checkObjectMap = new TSSE2BitMapMarker(startAddr, youngSize);
#elif PROCESSOR_ARCH == X86
    checkObjectMap = new TX86BitMapMarker(startAddr, youngSize);
#elif PROCESSOR_ARCH == ARM

#ifdef NEON
    checkObjectMap = new TNeonBitMapMarker(startAddr, youngSize);
#else
    checkObjectMap = new TARMBitMapMarker(startAddr, youngSize);
#endif

#endif

  if (unlikely(!setupOverrideFunction(cms_new_hook))) {
    logger->printCritMsg("Cannot setup to override CMS_new (ParNew GC).");
    return false;
  }

  if (unlikely(!setupOverrideFunction(cms_sweep_hook))) {
    logger->printCritMsg(
        "Cannot setup to override CMS_sweep (concurrent sweep).");
    return false;
  }

  return true;
}

/*!
 * \brief Setup GC hooking for G1GC
 * \return Process result.
 */
bool setupForG1(size_t maxMemSize) {
  TVMVariables *vmVal = TVMVariables::getInstance();
  SELECT_HOOK_FUNCS(g1);

  try {
    /* Sanity check. */
    if (unlikely(maxMemSize <= 0)) {
      logger->printCritMsg("G1 memory size should be > 1.");
      throw 1;
    }

/* Create bitmap to check object collected flag. */
#ifdef AVX
    checkObjectMap = new TAVXBitMapMarker(vmVal->getG1StartAddr(), maxMemSize);
#elif(defined SSE2) || (defined SSE4)
    checkObjectMap = new TSSE2BitMapMarker(vmVal->getG1StartAddr(), maxMemSize);
#elif PROCESSOR_ARCH == X86
    checkObjectMap = new TX86BitMapMarker(vmVal->getG1StartAddr(), maxMemSize);
#elif PROCESSOR_ARCH == ARM

#ifdef NEON
    checkObjectMap = new TNeonBitMapMarker(vmVal->getG1StartAddr(), maxMemSize);
#else
    checkObjectMap = new TARMBitMapMarker(vmVal->getG1StartAddr(), maxMemSize);
#endif

#endif

  } catch (...) {
    logger->printCritMsg("Cannot create object marker bitmap for G1.");
    return false;
  }

  /* If failure getting function information. */
  if (unlikely(!setupOverrideFunction(g1_hook) ||
               !setupOverrideFunction(g1Event_hook))) {
    logger->printCritMsg("Cannot setup to override G1GC.");
    return false;
  }

  return true;
}

/*!
 * \brief Setup hooking for inner GC event.
 * \warning Please this function call at after Agent_OnLoad.
 * \param enableHook     [in] Flag of enable hooking to function.
 * \param interruptEvent [in] Event callback on many times GC interupt.
 * \return Process result.
 */
bool setupHookForInnerGCEvent(bool enableHook, TCommonCallback interruptEvent) {
  isInvokeGCManyTimes = false;
  gcInterruptCallbackFunc = interruptEvent;

  return switchOverrideFunction(innerStart_hook, enableHook);
}

/*!
 * \brief Setting GC hooking enable.
 * \param enable [in] Is GC hook enable.
 * \return Process result.
 */
bool setGCHookState(bool enable) {
  TVMVariables *vmVal = TVMVariables::getInstance();

  /* Sanity check. */
  if (unlikely(isEnableGCHooking == enable)) {
    /* Already set state. */
    return true;
  }

  /* Change state. */
  isEnableGCHooking = enable;

  /* Setting hooking target. */
  THookFunctionInfo *list = NULL;
  if (vmVal->getUseParOld()) {
    list = parOld_hook;
  } else if (vmVal->getUseCMS()) {
    /* Switch CMS hooking at new generation. */
    switchOverrideFunction(cms_new_hook, enable);
    list = cms_sweep_hook;
    checkObjectMap->clear();
  } else if (vmVal->getUseG1()) {
    /* Switch G1GC event hooking. */
    switchOverrideFunction(g1Event_hook, enable);

    list = g1_hook;
    checkObjectMap->clear();
  }

  /* Switch common hooking. */
  if (unlikely(!switchOverrideFunction(par_hook, enable))) {
    logger->printCritMsg("Cannot switch override (ParallelGC)");
    return false;
  }

  /* Switch adjust pointer hooking. */
  if (!jvmInfo->isAfterCR6964458()) {
    if (unlikely(!switchOverrideFunction(adj_hook, enable))) {
      logger->printCritMsg("Cannot switch override (Class adjuster)");
      return false;
    }
  }

  if (list != NULL) {
    /* Switch hooking for each GC type. */
    if (unlikely(!switchOverrideFunction(list, enable))) {
      logger->printCritMsg("Cannot switch override GC");
      return false;
    }
  }

  return true;
}

/*!
 * \brief Setting JVMTI hooking enable.
 * \param enable [in] Is JVMTI hook enable.
 * \return Process result.
 */
bool setJvmtiHookState(bool enable) {
  /* Sanity check. */
  if (unlikely(isEnableJvmtiHooking == enable)) {
    /* Already set state. */
    return true;
  }

  /* Change state. */
  isEnableJvmtiHooking = enable;

  /* Switch JVMTI hooking. */
  if (unlikely(!switchOverrideFunction(jvmti_hook, enable))) {
    logger->printCritMsg("Cannot switch override JVMTI GC");
    return false;
  }

  return true;
}

/*!
 * \brief Switching override function enable state.
 * \param list   [in] List of hooking information.
 * \param enable [in] Enable of override function.
 * \return Process result.
 */
bool switchOverrideFunction(THookFunctionInfo *list, bool enable) {
  /* Variable for list pointer. */
  THookFunctionInfo *arr = list;
  /* Variable for list item count. */
  int listCnt = 0;
  /* Variable for succeed count. */
  int doneCnt = 0;

  /* Search class and function symbol. */
  for (; arr->originalFunc != NULL; listCnt++) {
    /* Variable for store vtable. */
    void **vtable = arr->vtable;
    /* Variable for store function pointer. */
    void *targetFunc = NULL;
    void *destFunc = NULL;

    /* Setting swap target and swap dest. */
    if (enable) {
      targetFunc = arr->originalFunc;
      destFunc = arr->overrideFunc;
    } else {
      targetFunc = arr->overrideFunc;
      destFunc = arr->originalFunc;
    }

    if (unlikely(vtable == NULL || targetFunc == NULL || destFunc == NULL)) {
      /* Skip illegal. */
      arr++;
      continue;
    }

    const int VTABLE_SKIP_LIMIT = 1000;
    for (int i = 0; i < VTABLE_SKIP_LIMIT && *vtable == NULL; i++) {
      /* Skip null entry. */
      vtable++;
    }

    while (*vtable != NULL) {
      /* If target function. */
      if (*vtable == targetFunc) {
        if (unlikely(!arr->isVTableWritable)) {
          /* Avoid memory protection for vtable
           *   libstdc++ may remove write permission from vtable
           *   http://gcc.gnu.org/ml/gcc-patches/2012-11/txt00001.txt
           */
          ptrdiff_t start_page = (ptrdiff_t)vtable & ~(systemPageSize - 1);
          ptrdiff_t end_page =
              ((ptrdiff_t)vtable + sizeof(void *)) & ~(systemPageSize - 1);
          size_t len = systemPageSize + (end_page - start_page);
          mprotect((void *)start_page, len, PROT_READ | PROT_WRITE);
          arr->isVTableWritable = true;
        }

        *vtable = destFunc;
        doneCnt++;
        break;
      }

      /* Move next vtable entry. */
      vtable++;
    }

    /* Move next list item. */
    arr++;
  }

  return (listCnt == doneCnt);
}

/* Callback functions. */
/****************************************************************/

/*!
 * \brief Callback function for parallel GC and user's GC.<br>
 *        E.g. System.gc() in java code, JVMTI and etc..
 * \param oop [in] Java heap object(OopDesc format).
 * \warning Param "oop" isn't usable for JVMTI and JNI.
 */
inline void callbackForParallelInternal(void *oop) {
  /* Invoke callback by GC. */
  gcCallbackFunc(oop, NULL);
  isInvokedParallelGC = true;
}

/*!
 * \brief Callback function for parallel GC and user's GC.<br>
 *        E.g. System.gc() in java code, JVMTI and etc..
 * \param oop [in] Java heap object(OopDesc format).
 * \warning Param "oop" isn't usable for JVMTI and JNI.
 */
void callbackForParallel(void *oop) {
  callbackForParallelInternal(oop);
}

/*!
 * \brief Callback function for parallel GC and user's GC.<br>
 *        E.g. System.gc() in java code, JVMTI and etc..
 *        This function checks markOop value.
 * \param oop [in] Java heap object(OopDesc format).
 * \warning Param "oop" isn't usable for JVMTI and JNI.
 */
void callbackForParallelWithMarkCheck(void *oop) {
  TVMVariables *vmVal = TVMVariables::getInstance();
  unsigned long markOop = *(ptrdiff_t *)incAddress(oop,
                                                   vmVal->getOfsMarkAtOop());
  uint64_t markValue = markOop & vmVal->getLockMaskInPlaceMarkOop();

  if (markValue == vmVal->getMarkedValue()) {
    callbackForParallelInternal(oop);
  }
}

/*!
 * \brief Callback function for parallel Old GC.
 * \param oop [in] Java heap object(OopDesc format).
 * \warning Param "oop" isn't usable for JVMTI and JNI.
 */
void callbackForParOld(void *oop) {
  /* Invoke callback by GC. */
  gcCallbackFunc(oop, NULL);
  isInvokedParallelGC = true;
}

/*!
 * \brief Callback function for OopClosure::do_oop(oop *).<br>
 *        This function is targeted for G1ParScanAndMarkExtRootClosure.
 *        (initial-mark for G1)
 * \param oop [in] Java heap object(OopDesc format).
 */
void callbackForDoOop(void **oop) {
  if ((oop == NULL) || (*oop == NULL) || is_in_permanent(collectedHeap, *oop)) {
    return;
  }

  if ((checkObjectMap != NULL) && checkObjectMap->checkAndMark(*oop)) {
    /* Object is already collected by G1GC collector. */
    return;
  }

  /* Invoke snapshot callback. */
  gcCallbackFunc(*oop, NULL);
}

/*!
 * \brief Callback function for OopClosure::do_oop(oop *).<br>
 *        This function checks whether the oop is marked.
 * \param oop [in] Java heap object(OopDesc format).
 */
void callbackForDoOopWithMarkCheck(void **oop) {
  if ((oop == NULL) || (*oop == NULL) || is_in_permanent(collectedHeap, *oop)) {
    return;
  }

  TVMVariables *vmVal = TVMVariables::getInstance();
  unsigned long markOop = *(ptrdiff_t *)incAddress(oop,
                                                   vmVal->getOfsMarkAtOop());
  uint64_t markValue = markOop & vmVal->getLockMaskInPlaceMarkOop();

  if (markValue == vmVal->getMarkedValue()) {
    gcCallbackFunc(*oop, NULL);
  }
}

/*!
 * \brief Callback function for OopClosure::do_oop(narrowOop *).<br>
 *        This function is targeted for G1ParScanAndMarkExtRootClosure.
 *        (initial-mark for G1)
 * \param oop [in] Java heap object(OopDesc format).
 */
void callbackForDoNarrowOop(unsigned int *narrowOop) {
  void *oop = getWideOop(*narrowOop);
  callbackForDoOop(&oop);
}

/*!
 * \brief Callback function for OopClosure::do_oop(narrowOop *).<br>
 *        This function checks whether the oop is marked.
 * \param oop [in] Java heap object(OopDesc format).
 */
void callbackForDoNarrowOopWithMarkCheck(unsigned int *narrowOop) {
  void *oop = getWideOop(*narrowOop);
  callbackForDoOopWithMarkCheck(&oop);
}

/*!
 * \brief Callback function for CMS GC and G1 GC.<br>
 *        This function is targeted for Young Gen only.
 * \param oop [in] Java heap object(OopDesc format).
 * \warning Param "oop" isn't usable for JVMTI and JNI.
 * \sa CMSParRemarkTask::do_young_space_rescan()<br>
 *     ContiguousSpace::par_oop_iterate()
 */
void callbackForIterate(void *oop) {
  TVMVariables *vmVal = TVMVariables::getInstance();

  if (vmVal->getUseCMS()) {
    /* If object is in CMS gen.  */
    if ((ptrdiff_t)oop >= (ptrdiff_t)vmVal->getCmsBitMap_startWord()) {
      /* Skip. because collect object in CMS gen by callbackForSweep. */
      return;
    }

    if (checkObjectMap->checkAndMark(oop)) {
      /* Object is in young gen and already marked. */
      return;
    }

    /* Invoke callback by CMS GC. */
    cmsCallbackFunc(oop, NULL);
  } else if (vmVal->getUseG1()) {
    if (checkObjectMap->checkAndMark(oop)) {
      /* Object is already collected by G1GC collector. */
      return;
    }

    /* Invoke callback by GC. */
    gcCallbackFunc(oop, NULL);
    isInvokedParallelGC = true;
  }
}

/*!
 * \brief Callback function for CMS sweep.<br>
 *        This function is targeted for CMS Gen only.
 * \param oop [in] Java heap object(OopDesc format).
 * \warning Param "oop" isn't usable for JVMTI and JNI.
 */
void callbackForSweep(void *oop) {
  /* If object isn't GC target. */
  if (likely(isMarkedObject(oop))) {
    /* Invoke callback by CMS GC. */
    cmsCallbackFunc(oop, NULL);
  }
}

/*!
 * \brief Callback function for oop_adjust_pointers.
 * \param oop [in] Java heap object(OopDesc format).
 * \warning Param "oop" isn't usable for JVMTI and JNI.
 */
void callbackForAdjustPtr(void *oop) {
  /* Get new class oop. */
  void *newOop = getForwardAddr(oop);

  if (newOop != NULL) {
    /* Invoke callback. */
    callbackForAdjustKlass(oop, newOop);
  }
}

/*!
 * \brief Callback function for do_addr.
 * \param oop [in] Java heap object(OopDesc format).
 * \warning Param "oop" isn't usable for JVMTI and JNI.
 */
void callbackForDoAddr(void *oop) {
  /* Get store memory. */
  void **oldOop = (void **)pthread_getspecific(oldKlassOopKey);
  if (unlikely(oldOop == NULL)) {
    /* Allocate and set store memory. */
    oldOop = (void **)malloc(sizeof(void *));
    pthread_setspecific(oldKlassOopKey, oldOop);
  }

  /* Store old klassOop. */
  if (likely(oldOop != NULL)) {
    (*oldOop) = oop;
  }
}

/*!
 * \brief Callback function for oop_update_pointers.
 * \param oop [in] Java heap object(OopDesc format).
 * \warning Param "oop" isn't usable for JVMTI and JNI.
 */
void callbackForUpdatePtr(void *oop) {
  /* Get old class oop. */
  void **oldOop = (void **)pthread_getspecific(oldKlassOopKey);

  if (likely(oldOop != NULL && (*oldOop) != NULL)) {
    /* Invoke callback. */
    callbackForAdjustKlass((*oldOop), oop);

    /* Cleanup old data. */
    (*oldOop) = NULL;
  }
}

/*!
 * \brief Callback function for snapshot with data dump or interval.
 * \param oop [in] Java heap object(OopDesc format).
 * \warning Param "oop" isn't usable for JVMTI and JNI.
 */
void callbackForJvmtiIterate(void *oop) {
  /* Invoke callback by JVMTI. */
  jvmtiIteCallbackFunc(oop, NULL);
}

/*!
 * \brief Callback function for before G1 GC cleanup.
 * \param thisptr [in] this pointer of caller C++ instance.
 */
void callbackForG1Cleanup(void *thisptr) {
  if (likely(g1FinishCallbackFunc != NULL)) {
    /* Invoke callback. */
    g1FinishCallbackFunc();
  }

  /* Clear bitmap. */
  checkObjectMap->clear();
}

/*!
 * \brief Callback function for before System.gc() on using G1GC.
 * \param thisptr [in] this pointer of caller C++ instance.
 */
void callbackForG1Full(void *thisptr) {
  /*
   * Disable G1 callback function:
   *  OopClosure for typeArrayKlass is called by G1 FullCollection.
   */
  if (!jvmInfo->isAfterJDK10()) {
    switchOverrideFunction(g1_hook, false);
  }

  /* Discard existed snapshot data */
  clearCurrentSnapShot();
  checkObjectMap->clear();
}

/*!
 * \brief Callback function for after System.gc() on using G1GC.
 * \param thisptr [in] this pointer of caller C++ instance.
 */
void callbackForG1FullReturn(void *thisptr) {
  /* Restore G1 callback. */
  if (!jvmInfo->isAfterJDK10()) {
    switchOverrideFunction(g1_hook, true);
  }

  if (likely(g1FinishCallbackFunc != NULL)) {
    /* Invoke callback. */
    g1FinishCallbackFunc();
  }

  /* Clear bitmap. */
  checkObjectMap->clear();
}

/*!
 * \brief Callback function for starting inner GC.
 */
void callbackForInnerGCStart(void) {
  /* If call GC function more one. */
  if (unlikely(isInvokeGCManyTimes)) {
    /* Invoke GC interrupt callback. */
    if (likely(gcInterruptCallbackFunc != NULL)) {
      gcInterruptCallbackFunc();
    }
  } else {
    /* Update state. */
    isInvokeGCManyTimes = true;
  }

  /* Get current GCCause. */
  jvmInfo->loadGCCause();
}

/*!
 * \brief Callback function for WatcherThread.
 */
void callbackForWatcherThreadRun(void) { jvmInfo->detectDelayInfoAddress(); }

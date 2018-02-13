/*!
 * \file overrideFunc.S
 * \brief This file is used to override JVM inner function.<br>
 *        The function defined this file, used with v-table hook.<br>
 *        So in this file, all function is written by only assembler and macro.
 * Copyright (C) 2011-2018 Nippon Telegraph and Telephone Corporation
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


/* Override functions of "do_oop" families. */
    .file "overrideFunc.S"

#ifdef __amd64__
#include "arch/x86/overrideFunc.amd64.S"
#elif defined(__i686)

/*
 * This statement is for avoiding illegal conversion by preprocceor.
 * before : "call __i686.get_pc_thunk.bx;"
 * after  : "call 1.get_pc_thunk.bx;"
 */
#undef __i686
#define AVOID__i686

#include "arch/x86/overrideFunc.i686.S"

#elif defined __arm__
#include "arch/arm/overrideFunc.arm.S"
#else
#error overrideFunc supports only AMD64 or i686 or ARM.
#endif


/* Override definition. */

/* For Parallel. */

/* instanceKlass::oop_follow_contents(oopDesc*) */
OVERRIDE_DEFINE(par, 0, 2)
/* objArrayKlass::oop_follow_contents(oopDesc*) */
OVERRIDE_DEFINE(par, 1, 2)
/* typeArrayKlass::oop_follow_contents(oopDesc*) */
OVERRIDE_DEFINE(par, 2, 2)
/* instanceRefKlass::oop_follow_contents(oopDesc*) */
OVERRIDE_DEFINE(par, 3, 2)

/* instanceKlass::oop_follow_contents(oopDesc*) */
OVERRIDE_DEFINE(par_6964458, 0, 2)
/* objArrayKlass::oop_follow_contents(oopDesc*) */
OVERRIDE_DEFINE(par_6964458, 1, 2)
/* typeArrayKlass::oop_follow_contents(oopDesc*) */
OVERRIDE_DEFINE(par_6964458, 2, 2)
/* instanceRefKlass::oop_follow_contents(oopDesc*) */
OVERRIDE_DEFINE(par_6964458, 3, 2)
/* InstanceClassLoaderKlass::oop_follow_contents(oopDesc*) */
OVERRIDE_DEFINE(par_6964458, 4, 2)

/* For ParallelOld. */

/* instanceKlass::oop_follow_contents(ParCompactionManager*, oopDesc*) */
OVERRIDE_DEFINE(parOld, 0, 3)
/* objArrayKlass::oop_follow_contents(ParCompactionManager*, oopDesc*) */
OVERRIDE_DEFINE(parOld, 1, 3)
/* typeArrayKlass::oop_follow_contents(ParCompactionManager*, oopDesc*) */
OVERRIDE_DEFINE(parOld, 2, 3)
/* instanceRefKlass::oop_follow_contents(ParCompactionManager*, oopDesc*) */
OVERRIDE_DEFINE(parOld, 3, 3)

/* instanceKlass::oop_follow_contents(ParCompactionManager*, oopDesc*) */
OVERRIDE_DEFINE(parOld_6964458, 0, 3)
/* objArrayKlass::oop_follow_contents(ParCompactionManager*, oopDesc*) */
OVERRIDE_DEFINE(parOld_6964458, 1, 3)
/* typeArrayKlass::oop_follow_contents(ParCompactionManager*, oopDesc*) */
OVERRIDE_DEFINE(parOld_6964458, 2, 3)
/* instanceRefKlass::oop_follow_contents(ParCompactionManager*, oopDesc*) */
OVERRIDE_DEFINE(parOld_6964458, 3, 3)
/* InstanceClassLoaderKlass::oop_follow_contents(ParCompactionManager*, oopDesc*) */
OVERRIDE_DEFINE(parOld_6964458, 4, 3)

/* For CMS. */
/*
 * We want to hook sweep phase
 * and around "ContiguousSpace::par_oop_iterate()".
 */

/* SweepClosure::do_blk_careful(HeapWord*) */
OVERRIDE_DEFINE(cms_sweep, 0, 2)

/* instanceKlass::oop_oop_iterate_nv(oopDesc*, Par_MarkRefsIntoAndScanClosure*) */
OVERRIDE_DEFINE(cms_new, 0, 2)
/* objArrayKlass::oop_oop_iterate_nv(oopDesc*, Par_MarkRefsIntoAndScanClosure*) */
OVERRIDE_DEFINE(cms_new, 1, 2)
/* typeArrayKlass::oop_oop_iterate(oopDesc*, OopClosure*) */
OVERRIDE_DEFINE(cms_new, 2, 2)
/* instanceRefKlass::oop_oop_iterate_nv(oopDesc*, Par_MarkRefsIntoAndScanClosure*) */
OVERRIDE_DEFINE(cms_new, 3, 2)

/* instanceKlass::oop_oop_iterate_nv(oopDesc*, Par_MarkRefsIntoAndScanClosure*) */
OVERRIDE_DEFINE(cms_new_6964458, 0, 2)
/* objArrayKlass::oop_oop_iterate_nv(oopDesc*, Par_MarkRefsIntoAndScanClosure*) */
OVERRIDE_DEFINE(cms_new_6964458, 1, 2)
/* typeArrayKlass::oop_oop_iterate(oopDesc*, OopClosure*) */
OVERRIDE_DEFINE(cms_new_6964458, 2, 2)
/* instanceRefKlass::oop_oop_iterate_nv(oopDesc*, Par_MarkRefsIntoAndScanClosure*) */
OVERRIDE_DEFINE(cms_new_6964458, 3, 2)
/* InstanceClassLoaderKlass::oop_oop_iterate_nv(oopDesc*, Par_MarkRefsIntoAndScanClosure*) */
OVERRIDE_DEFINE(cms_new_6964458, 4, 2)

/* For G1. */

/* initial-mark */
/* G1ParScanAndMarkExtRootClosure::do_oop(oopDesc**) */
/* G1ParCopyClosure<false, (G1Barrier)0, false>::do_oop(oopDesc**) */
OVERRIDE_DEFINE(g1, 0, 2)
/* G1ParScanAndMarkExtRootClosure::do_oop(unsigned int*) */
/* G1ParCopyClosure<false, (G1Barrier)0, false>::do_oop(unsigned int*) */
OVERRIDE_DEFINE(g1, 1, 2)

/* concurrent-root-region-scan */
/* instanceKlass::oop_oop_iterate_nv(oopDesc*, G1RootRegionScanClosure*) */
OVERRIDE_DEFINE(g1, 2, 2)
/* objArrayKlass::oop_oop_iterate_nv(oopDesc*, G1RootRegionScanClosure*) */
OVERRIDE_DEFINE(g1, 3, 2)
/* instanceRefKlass::oop_oop_iterate_nv(oopDesc*, G1RootRegionScanClosure*) */
OVERRIDE_DEFINE(g1, 4, 2)

/* concurrent-mark / remark */
/* instanceKlass::oop_oop_iterate_nv(oopDesc*, G1CMOopClosure*) */
OVERRIDE_DEFINE(g1, 5, 2)
/* objArrayKlass::oop_oop_iterate_nv(oopDesc*, G1CMOopClosure*) */
OVERRIDE_DEFINE(g1, 6, 2)
/* typeArrayKlass::oop_oop_iterate(oopDesc*, OopClosure*) */
OVERRIDE_DEFINE_WITH_CALLER_CHECK(g1, 7, 2, VTableForTypeArrayOopClosure)
/* instanceRefKlass::oop_oop_iterate_nv(oopDesc*, G1CMOopClosure*) */
OVERRIDE_DEFINE(g1, 8, 2)

/* initial-mark */
/* G1ParScanAndMarkExtRootClosure::do_oop(oopDesc**) */
/* G1ParCopyClosure<false, (G1Barrier)0, false>::do_oop(oopDesc**) */
OVERRIDE_DEFINE(g1_6964458, 0, 2)
/* G1ParScanAndMarkExtRootClosure::do_oop(unsigned int*) */
/* G1ParCopyClosure<false, (G1Barrier)0, false>::do_oop(unsigned int*) */
OVERRIDE_DEFINE(g1_6964458, 1, 2)

/* concurrent-root-region-scan */
/* instanceKlass::oop_oop_iterate_nv(oopDesc*, G1RootRegionScanClosure*) */
OVERRIDE_DEFINE(g1_6964458, 2, 2)
/* objArrayKlass::oop_oop_iterate_nv(oopDesc*, G1RootRegionScanClosure*) */
OVERRIDE_DEFINE(g1_6964458, 3, 2)
/* instanceRefKlass::oop_oop_iterate_nv(oopDesc*, G1RootRegionScanClosure*) */
OVERRIDE_DEFINE(g1_6964458, 4, 2)
/* InstanceClassLoaderKlass::oop_oop_iterate_nv(oopDesc*, G1RootRegionScanClosure*) */
OVERRIDE_DEFINE(g1_6964458, 5, 2)

/* concurrent-mark / remark */
/* instanceKlass::oop_oop_iterate_nv(oopDesc*, G1CMOopClosure*) */
OVERRIDE_DEFINE(g1_6964458, 6, 2)
/* objArrayKlass::oop_oop_iterate_nv(oopDesc*, G1CMOopClosure*) */
OVERRIDE_DEFINE(g1_6964458, 7, 2)
/* typeArrayKlass::oop_oop_iterate(oopDesc*, OopClosure*) */
OVERRIDE_DEFINE_WITH_CALLER_CHECK(g1_6964458, 8, 2, VTableForTypeArrayOopClosure)
/* instanceRefKlass::oop_oop_iterate_nv(oopDesc*, G1CMOopClosure*) */
OVERRIDE_DEFINE(g1_6964458, 9, 2)
/* InstanceClassLoaderKlass::oop_oop_iterate_nv(oopDesc*, G1CMOopClosure*) */
OVERRIDE_DEFINE(g1_6964458, 10, 2)

/* CMCleanUp::do_void() */
OVERRIDE_DEFINE_WITHOUT_PERMCHECK(g1Event, 0, 1)
/* VM_G1CollectFull::doit_prologue() */
OVERRIDE_DEFINE_WITHOUT_PERMCHECK(g1Event, 1, 1)
/* VM_G1CollectFull::doit_epilogue() */
OVERRIDE_DEFINE_WITHOUT_PERMCHECK(g1Event, 2, 1)

/* for Klass relocation. */

/* instanceKlassKlass::oop_adjust_pointers(oopDesc*) */
OVERRIDE_ADJ_DEFINE(adj, 0, 2)
/* objArrayKlassKlass::oop_adjust_pointers(oopDesc*) */
OVERRIDE_ADJ_DEFINE(adj, 1, 2)
/* arrayKlassKlass::oop_adjust_pointers(oopDesc*) */
OVERRIDE_ADJ_DEFINE(adj, 2, 2)

/* for Klass relocation if use ParallelOldGC. */

/* MoveAndUpdateClosure::do_addr(HeapWord*, unsigned long) */
OVERRIDE_ADJ_DEFINE(adj, 3, 2)
/* UpdateOnlyClosure::do_addr(HeapWord*, size_t) */
OVERRIDE_ADJ_DEFINE(adj, 4, 2)

/* instanceKlassKlass::oop_update_pointers(ParCompactionManager*, oopDesc*) */
OVERRIDE_ADJ_DEFINE(adj, 5, 3)
/* objArrayKlassKlass::oop_update_pointers(ParCompactionManager*, oopDesc*) */
OVERRIDE_ADJ_DEFINE(adj, 6, 3)
/* arrayKlassKlass::oop_update_pointers(ParCompactionManager*, oopDesc*) */
OVERRIDE_ADJ_DEFINE(adj, 7, 3)

/* for JVMTI IterateOverHeap. */

/* IterateOverHeapObjectClosure::do_object(oop o) */
OVERRIDE_DEFINE(jvmti, 0, 2)

/* For inner GC start. */

/* ParallelScavengeHeap::accumulate_statistics_all_tlabs(void) */
OVERRIDE_DEFINE_WITHOUT_PERMCHECK(innerStart, 0, 1)
/* CollectedHeap::accumulate_statistics_all_tlabs(void) */
OVERRIDE_DEFINE_WITHOUT_PERMCHECK(innerStart, 1, 1)
/* GenCollectedHeap::gc_prologue(bool) */
OVERRIDE_DEFINE_WITHOUT_PERMCHECK(innerStart, 2, 2)

/* For WatcherThread */
/* WatcherThread::run() */
OVERRIDE_DEFINE_WITHOUT_PERMCHECK(watcherThread, 0, 1)


/* For JDK 9 Serial / Parallel GC hook */

/* InstanceKlass::oop_ms_adjust_pointers(oopDesc*) */
OVERRIDE_DEFINE(par_jdk9, 0, 2)
/* ObjArrayKlass::oop_ms_adjust_pointers(oopDesc*) */
OVERRIDE_DEFINE(par_jdk9, 1, 2)
/* TypeArrayKlass::oop_ms_adjust_pointers(oopDesc*) */
OVERRIDE_DEFINE(par_jdk9, 2, 2)
/* InstanceRefKlass::oop_ms_adjust_pointers(oopDesc*) */
OVERRIDE_DEFINE(par_jdk9, 3, 2)
/* InstanceClassLoaderKlass::oop_ms_adjust_pointers(oopDesc*) */
OVERRIDE_DEFINE(par_jdk9, 4, 2)

/* For JDK 10 Serial / Parallel GC hook */

/* AdjustPointerClosure::do_oop(oopDesc**) */
OVERRIDE_DEFINE(par_jdk10, 0, 2)
/* AdjustPointerClosure::do_oop(unsigned int*) */
OVERRIDE_DEFINE(par_jdk10, 1, 2)

/* For JDK 9 ParallelOld GC hook */

/* InstanceKlass::oop_pc_follow_contents(oopDesc*, ParCompactionManager*) */
OVERRIDE_DEFINE(parOld_jdk9, 0, 2)
/* ObjArrayKlass::oop_pc_follow_contents(oopDesc*, ParCompactionManager*) */
OVERRIDE_DEFINE(parOld_jdk9, 1, 2)
/* TypeArrayKlass::oop_pc_follow_contents(oopDesc*, ParCompactionManager*) */
OVERRIDE_DEFINE(parOld_jdk9, 2, 2)
/* InstanceRefKlass::oop_pc_follow_contents(oopDesc*, ParCompactionManager*) */
OVERRIDE_DEFINE(parOld_jdk9, 3, 2)
/* InstanceClassLoaderKlass::oop_pc_follow_contents(oopDesc*, ParCompactionManager*) */
OVERRIDE_DEFINE(parOld_jdk9, 4, 2)

/* For JDK 9 ParNew GC hook */

/* InstanceKlass::oop_oop_iterate_v(oopDesc*, ExtendedOopClosure*) */
OVERRIDE_DEFINE(cms_new_jdk9, 0, 2)
/* ObjArrayKlass::oop_oop_iterate_v(oopDesc*, ExtendedOopClosure*) */
OVERRIDE_DEFINE(cms_new_jdk9, 1, 2)
/* TypeArrayKlass::oop_oop_iterate_v(oopDesc*, ExtendedOopClosure*) */
OVERRIDE_DEFINE(cms_new_jdk9, 2, 2)
/* InstanceRefKlass::oop_oop_iterate_v(oopDesc*, ExtendedOopClosure*) */
OVERRIDE_DEFINE(cms_new_jdk9, 3, 2)
/* InstanceClassLoaderKlass::oop_oop_iterate_v(oopDesc*, ExtendedOopClosure*) */
OVERRIDE_DEFINE(cms_new_jdk9, 4, 2)

/* For JDK 9 G1 GC hook */

/* initial-mark */
/* G1ParCopyClosure<(G1Barrier)0, (G1Mark)1, false>::do_oop(oopDesc**) */
OVERRIDE_DEFINE(g1_jdk9, 0, 2)
/* G1ParCopyClosure<(G1Barrier)0, (G1Mark)1, false>::do_oop(unsigned int*) */
OVERRIDE_DEFINE(g1_jdk9, 1, 2)

/* concurrent-root-region-scan */
/* InstanceKlass::oop_oop_iterate_nv(oopDesc*, G1RootRegionScanClosure*) */
OVERRIDE_DEFINE(g1_jdk9, 2, 2)
/* ObjArrayKlass::oop_oop_iterate_nv(oopDesc*, G1RootRegionScanClosure*) */
OVERRIDE_DEFINE(g1_jdk9, 3, 2)
/* TypeArrayKlass::oop_oop_iterate_nv(oopDesc*, G1RootRegionScanClosure*) */
OVERRIDE_DEFINE(g1_jdk9, 4, 2)
/* InstanceRefKlass::oop_oop_iterate_nv(oopDesc*, G1RootRegionScanClosure*) */
OVERRIDE_DEFINE(g1_jdk9, 5, 2)
/* InstanceClassLoaderKlass::oop_oop_iterate_nv(oopDesc*, G1RootRegionScanClosure*) */
OVERRIDE_DEFINE(g1_jdk9, 6, 2)

/* concurrent-mark / remark */
/* InstanceKlass::oop_oop_iterate_nv(oopDesc*, G1CMOopClosure*) */
OVERRIDE_DEFINE(g1_jdk9, 7, 2)
/* ObjArrayKlass::oop_oop_iterate_nv(oopDesc*, G1CMOopClosure*) */
OVERRIDE_DEFINE(g1_jdk9, 8, 2)
/* TypeArrayKlass::oop_oop_iterate_nv(oopDesc*, G1CMOopClosure*) */
OVERRIDE_DEFINE(g1_jdk9, 9, 2)
/* InstanceRefKlass::oop_oop_iterate_nv(oopDesc*, G1CMOopClosure*) */
OVERRIDE_DEFINE(g1_jdk9, 10, 2)
/* InstanceClassLoaderKlass::oop_oop_iterate_nv(oopDesc*, G1CMOopClosure*) */
OVERRIDE_DEFINE(g1_jdk9, 11, 2)


#ifdef AVOID__i686
/* Restore definition. */
#undef AVOID__i686
#define __i686
#endif


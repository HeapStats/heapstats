/*!
 * \file heapstats_md_x86.cpp
 * \brief Proxy library for HeapStats backend.
 *        This file implements x86 specific code for loading backend library.
 * Copyright (C) 2014 Yasumasa Suenaga
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <link.h>
#include <libgen.h>
#include <stdio.h>
#include <dlfcn.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "heapstats_md.hpp"
#include "heapstats_md_x86.hpp"

/*!
 * \brief Check CPU instruction.
 * \return Max level of instruction.
 */
static const char *checkInstructionSet(void) {
  int cFlag = 0;
  int dFlag = 0;

#ifdef __amd64__
  asm volatile("cpuid;" : "=c"(cFlag), "=d"(dFlag) : "a"(1) : "cc", "%ebx");
#else  // i386
  /* Evacuate EBX because EBX is PIC register. */
  asm volatile(
    "pushl %%ebx;"
    "cpuid;"
    "popl %%ebx;"
    : "=c"(cFlag), "=d"(dFlag)
    : "a"(1)
    : "cc"
  );
#endif

#ifdef AVX
  if ((cFlag >> 28) & 1) {
    return OPTIMIZE_AVX;
  } else
#endif
#ifdef SSE4
      if (((cFlag >> 20) & 1) || ((cFlag >> 19) & 1)) {
    return OPTIMIZE_SSE4;
  } else
#endif
#ifdef SSE3
      if (((cFlag >> 9) & 1) || (cFlag & 1)) {
    return OPTIMIZE_SSE3;
  } else
#endif
#ifdef SSE2
      if ((dFlag >> 26) & 1) {
    return OPTIMIZE_SSE2;
  } else
#endif
  {
    return OPTIMIZE_NONE;
  }
}

/*!
 * \brief Callback function for dl_iterate_phdr to find HeapStats library.
 */
static int findHeapStatsCallback(struct dl_phdr_info *info, size_t size,
                                 void *data) {
  ptrdiff_t this_func_addr = (ptrdiff_t)&findHeapStatsCallback;

  for (int idx = 0; idx < info->dlpi_phnum; idx++) {
    ptrdiff_t base_addr = info->dlpi_addr + info->dlpi_phdr[idx].p_vaddr;

    if ((this_func_addr >= base_addr) &&
        (this_func_addr <= base_addr +
                               (ptrdiff_t)info->dlpi_phdr[idx].p_memsz)) {
      strcpy((char *)data, dirname((char *)info->dlpi_name));
      return 1;
    }
  }

  return 0;
}

/*!
 * \brief Load HeapStats engine.
 * \return Library handle which is used in dlsym(). NULL if error occurred.
 */
void *loadHeapStatsEngine(void) {
  char engine_path[PATH_MAX];
  char heapstats_path[PATH_MAX];
  int found = dl_iterate_phdr(&findHeapStatsCallback, heapstats_path);

  if (found == 0) {
    fprintf(stderr, "HeapStats shared library is not found!\n");
    return NULL;
  }

  sprintf(engine_path,
          "%s/heapstats-engines/libheapstats-engine-%s-" HEAPSTATS_MAJOR_VERSION
          ".so",
          heapstats_path, checkInstructionSet());

  void *hEngine = dlopen(engine_path, RTLD_NOW);
  if (hEngine == NULL) {
    fprintf(stderr,
            "HeapStats engine could not be loaded: library: %s, cause: %s\n",
            engine_path, dlerror());
    return NULL;
  }

  return hEngine;
}

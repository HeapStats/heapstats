/*!
 * \file heapstats_md_arm.cpp
 * \brief Proxy library for HeapStats backend.
 *        This file implements ARM specific code for loading backend library.
 * Copyright (C) 2015 Yasumasa Suenaga
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
#include <unistd.h>
#include <asm/hwcap.h>

#ifdef HAVE_SYS_AUXV_H
#include <sys/auxv.h>
#else
#include <fcntl.h>
#include <elf.h>
#endif

#include "heapstats_md.hpp"

/*!
 * \brief Callback function for dl_iterate_phdr to find HeapStats library.
 */
static int findHeapStatsCallback(struct dl_phdr_info *info, size_t size,
                                 void *data) {
  ptrdiff_t this_func_addr = (ptrdiff_t)&findHeapStatsCallback;

  for (int idx = 0; idx < info->dlpi_phnum; idx++) {
    ptrdiff_t base_addr = info->dlpi_addr + info->dlpi_phdr[idx].p_vaddr;

    if ((this_func_addr >= base_addr) &&
        (this_func_addr <= base_addr + info->dlpi_phdr[idx].p_memsz)) {
      strcpy((char *)data, dirname((char *)info->dlpi_name));
      return 1;
    }
  }

  return 0;
}

#ifdef HAVE_SYS_AUXV_H
/*!
 * \brief check NEON support.
 *        http://community.arm.com/groups/android-community/blog/2014/10/10/runtime-detection-of-cpu-features-on-an-armv8-a-cpu
 * \return true if NEON is supported.
 */
static bool checkNEON(void) { return getauxval(AT_HWCAP) & HWCAP_NEON; }
#else
static bool checkNEON(void) {
  int fd = open("/proc/self/auxv", O_RDONLY);
  if (fd == -1) {
    return false;
  }

  Elf32_auxv_t aux;
  bool result = false;

  while (read(fd, &aux, sizeof(Elf32_auxv_t)) > 0) {
    if ((aux.a_type == AT_HWCAP) && ((aux.a_un.a_val & HWCAP_NEON) != 0)) {
      result = true;
      break;
    }
  }

  close(fd);
  return result;
}
#endif

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

  sprintf(engine_path, "%s/heapstats-engines/libheapstats-engine-%s-%s.so",
          heapstats_path, checkNEON() ? "neon" : "none",
          HEAPSTATS_MAJOR_VERSION);

  void *hEngine = dlopen(engine_path, RTLD_NOW);
  if (hEngine == NULL) {
    fprintf(stderr,
            "HeapStats engine could not be loaded: library: %s, cause: %s\n",
            engine_path, dlerror());
    return NULL;
  }

  return hEngine;
}

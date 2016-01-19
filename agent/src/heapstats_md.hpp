/*!
 * \file heapstats_md.hpp
 * \brief Proxy library for HeapStats backend.
 *        This file defines machine-dependent (md) types/functions.
 *        Each md source in "arch" should be implemented functions
 *        in this file.
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

#ifndef HEAPSTATS_MD_HPP
#define HEAPSTATS_MD_HPP

#include <jni.h>
#include <jvmti.h>

/* Function pointer prototypes */
typedef jint (*TAgentOnLoadFunc)(JavaVM *vm, char *options, void *reserved);
typedef jint (*TAgentOnAttachFunc)(JavaVM *vm, char *options, void *reserved);
typedef jint (*TAgentOnUnloadFunc)(JavaVM *vm);

/*!
 * \brief Load HeapStats engine.
 * \return Library handle which is used in dlsym(). NULL if error occurred.
 */
void *loadHeapStatsEngine(void);

#endif  // HEAPSTATS_MD_HPP

/*!
 * \file heapstats.cpp
 * \brief Proxy library for HeapStats backend.
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

#include <jni.h>
#include <jvmti.h>
#include <link.h>
#include <libgen.h>
#include <stdio.h>
#include <dlfcn.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "heapstats.hpp"
#include "heapstats_md.hpp"

/*!
 * \brief Handle of HeapStats engine.
 */
static void *hEngine = NULL;

/*!
 * \brief Agent attach entry points.
 * \param vm       [in] JavaVM object.
 * \param options  [in] Commandline arguments.
 * \param reserved [in] Reserved.
 * \return Attach initialization result code.
 */
JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
  hEngine = loadHeapStatsEngine();
  if (hEngine == NULL) {
    return JNI_ERR;
  }

  TAgentOnLoadFunc onLoad = (TAgentOnLoadFunc)dlsym(hEngine, "Agent_OnLoad");
  if (onLoad == NULL) {
    fprintf(stderr, "Could not get Agent_OnLoad() from backend library.\n");
    dlclose(hEngine);
    return JNI_ERR;
  }

  return onLoad(vm, options, reserved);
}

/*!
 * \brief Common agent unload entry points.
 * \param vm [in] JavaVM object.
 */
JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm) {
  TAgentOnUnloadFunc onUnload =
      (TAgentOnUnloadFunc)dlsym(hEngine, "Agent_OnUnload");
  if (onUnload == NULL) {
    fprintf(stderr, "Could not get Agent_OnUnload() from backend library.\n");
  } else {
    onUnload(vm);
  }

  dlclose(hEngine);
}

/*!
 * \brief Ondemand attach's entry points.
 * \param vm       [in] JavaVM object.
 * \param options  [in] Commandline arguments.
 * \param reserved [in] Reserved.
 * \return Ondemand-attach initialization result code.
 */
JNIEXPORT jint JNICALL
    Agent_OnAttach(JavaVM *vm, char *options, void *reserved) {
  hEngine = loadHeapStatsEngine();
  if (hEngine == NULL) {
    return JNI_ERR;
  }

  TAgentOnAttachFunc onAttach =
      (TAgentOnAttachFunc)dlsym(hEngine, "Agent_OnAttach");
  if (onAttach == NULL) {
    fprintf(stderr, "Could not get Agent_OnAttach() from backend library.\n");
    dlclose(hEngine);
    return JNI_ERR;
  }

  return onAttach(vm, options, reserved);
}

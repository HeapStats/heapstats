/*!
 * \file jvmInfo.cpp
 * \brief This file is used to get JVM performance information.
 * Copyright (C) 2011-2016 Nippon Telegraph and Telephone Corporation
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

#include <dlfcn.h>
#include <stdlib.h>

#ifndef USE_VMSTRUCTS
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#endif

#ifdef USE_VMSTRUCTS
#include "vmStructScanner.hpp"
#endif

#include "globals.hpp"
#include "util.hpp"
#include "jvmInfo.hpp"

#include "cppRegex.hpp"


/*!
 * \brief TJvmInfo constructor.
 */
TJvmInfo::TJvmInfo(void) {
  /* Initialization. */
  _nowFGC = NULL;
  _nowYGC = NULL;
  _edenSize = NULL;
  _sur0Size = NULL;
  _sur1Size = NULL;
  _oldSize = NULL;
  _metaspaceUsage = NULL;
  _metaspaceCapacity = NULL;
  _FGCTime = NULL;
  _YGCTime = NULL;
  _freqTime = NULL;
  _gcCause = NULL;
  perfAddr = 0;
  elapFGCTime = 0;
  elapYGCTime = 0;
  fullgcFlag = false;
  loadLogFlag = false;
  loadDelayLogFlag = false;

  /* Logging information. */
  _syncPark = NULL;
  _threadLive = NULL;
  _safePointTime = NULL;
  _safePoints = NULL;
  _vmVersion = NULL;
  _vmName = NULL;
  _classPath = NULL;
  _endorsedPath = NULL;
  _javaVersion = NULL;
  _javaHome = NULL;
  _bootClassPath = NULL;
  _vmArgs = NULL;
  _vmFlags = NULL;
  _javaCommand = NULL;
  _tickTime = NULL;

  /* Allocate GC cause container. */
  if (unlikely(posix_memalign((void **)&gcCause, 32, /* for vmovdqa. */
                              MAXSIZE_GC_CAUSE) != 0)) {
    throw "Couldn't allocate gc-cause memory!";
  }

  /* If failure allocate GC cause container. */
  if (unlikely(gcCause == NULL)) {
    throw "Couldn't allocate gc-cause memory!";
  }

  /* Initialize. */
  memcpy(this->gcCause, UNKNOWN_GC_CAUSE, 16);

  /* Get memory function address. */
  this->maxMemFunc = (TGetMemoryFunc)dlsym(RTLD_DEFAULT, "JVM_MaxMemory");
  this->totalMemFunc = (TGetMemoryFunc)dlsym(RTLD_DEFAULT, "JVM_TotalMemory");

  /* Check memory function address. */
  if (this->maxMemFunc == NULL) {
    logger->printWarnMsg("Couldn't get memory function: JVM_MaxMemory");
  }

  if (this->totalMemFunc == NULL) {
    logger->printWarnMsg("Couldn't get memory function: JVM_TotalMemory");
  }
}

/*!
 * \brief TJvmInfo destructor.
 */
TJvmInfo::~TJvmInfo() {
  /* Free allcated GC cause container. */
  free(gcCause);
}

/*!
 * \brief Set JVM version from system property (java.vm.version) .
 * \param jvmti  [in]  JVMTI Environment.
 * \return Process result.
 */
bool TJvmInfo::setHSVersion(jvmtiEnv *jvmti) {
  /* Setup HotSpot version. */
  char *versionStr = NULL;
  if (unlikely(isError(
          jvmti, jvmti->GetSystemProperty("java.vm.version", &versionStr)))) {
    logger->printCritMsg(
        "Cannot get JVM version from \"java.vm.version\" property.");
    return false;
  } else {
    logger->printDebugMsg("HotSpot version: %s", versionStr);

    /*
     * Version-String Scheme has changed since JDK 9 (JEP223).
     * afterJDK9 means whether this version is after JDK 9(1) or not(0).
     * major means hotspot version until JDK 8, jdk major version since JDK 9.
     * minor means jdk minor version in every JDK.
     * build means build version in every JDK.
     * security means security version which has introduced since JDK 9.
     *
     * NOT support Early Access because the binaries do not archive.
     * See also: https://bugs.openjdk.java.net/browse/JDK-8061493
     */
    unsigned char afterJDK9 = 0;
    unsigned char major = 0;
    unsigned char minor = 0;
    unsigned char build = 0;
    unsigned char security = 0;
    int result = 0;

    /* Parse version string of JDK 8 and older. */
    result = sscanf(versionStr, "%hhu.%hhu-b%hhu", &major, &minor, &build);
    if (result != 3) {
      /*
       * Parse version string of JDK 9+ GA.
       * GA release does not include minor and security.
       */
      result = sscanf(versionStr, "%hhu+%hhu", &major, &build);

      if (likely(result != 2)) {
        /* Parse version string of JDK 9 GA (Minor #1) and later */
        result = sscanf(versionStr, "%hhu.%hhu.%hhu+%hhu", &major, &minor,
            &security, &build);
        if (unlikely(result != 4)) {
          logger->printCritMsg("Unsupported JVM version: %s", versionStr);
          jvmti->Deallocate((unsigned char *)versionStr);
          return false;
        }
      }
      afterJDK9 = 1;
    }

    jvmti->Deallocate((unsigned char *)versionStr);
    _hsVersion = MAKE_HS_VERSION(afterJDK9, major, minor, security, build);
  }

  return true;
}

#ifdef USE_VMSTRUCTS

/*!
 * \brief Get JVM performance data address.<br>
 *        Use external VM structure.
 * \return JVM performance data address.
 */
ptrdiff_t TJvmInfo::getPerfMemoryAddress(void) {
  /* Symbol "gHotSpotVMStructs" is included in "libjvm.so". */
  extern VMStructEntry *gHotSpotVMStructs;
  /* If don't exist gHotSpotVMStructs. */
  if (gHotSpotVMStructs == NULL) {
    return 0;
  }

  /* Search perfomance data in gHotSpotVMStructs. */
  int Cnt = 0;
  ptrdiff_t addr = 0;
  while (gHotSpotVMStructs[Cnt].typeName != NULL) {
    /* Check each entry. */
    if ((strncmp(gHotSpotVMStructs[Cnt].typeName, "PerfMemory", 10) == 0) &&
        (strncmp(gHotSpotVMStructs[Cnt].fieldName, "_start", 6) == 0)) {
      /* Found. */
      addr = (ptrdiff_t) * (char **)gHotSpotVMStructs[Cnt].address;
      break;
    }

    /* Go next entry. */
    Cnt++;
  }

  return addr;
}

#else

/*!
 * \brief Search JVM performance data address in memory.
 * \param path [in] Path of hsperfdata.
 * \return JVM performance data address.
 */
ptrdiff_t TJvmInfo::findPerfMemoryAddress(char *path) {
  /* Search entries in performance file. */
  ptrdiff_t addr = 0;
  ptrdiff_t start_addr;
  char mapPath[101];
  char file[PATH_MAX + 1];
  char *line = NULL;
  char lineFmt[101];
  size_t len = 0;

  /* Decode link in path. */
  char target[PATH_MAX + 1] = {0};
  if (unlikely(realpath(path, target) == NULL)) {
    return 0;
  }

  /* Open process map. */
  snprintf(mapPath, 100, "/proc/%d/maps", getpid());
  FILE *maps = fopen(mapPath, "r");

  /* If failure open file. */
  if (unlikely(maps == NULL)) {
    return 0;
  }

  snprintf(lineFmt, 100, "%%tx-%%*x %%*s %%*x %%*x:%%*x %%*u %%%ds", PATH_MAX);
  /* Repeats until EOF. */
  while (getline(&line, &len, maps) != -1) {
    /*
     * From Linux Kernel source.
     * fs/proc/task_mmu.c: static void show_map_vma()
     */
    if (sscanf(line, lineFmt, &start_addr, file) == 2) {
      /* If found load address. */
      if (strcmp(file, target) == 0) {
        addr = start_addr;
        break;
      }
    }
  }

  /* Clean up after. */
  if (line != NULL) {
    free(line);
  }

  /* Close map. */
  fclose(maps);
  return addr;
}

/*!
 * \brief Get JVM performance data address.<br>
 *        Use performance data file.
 * \param env [in] JNI environment object.
 * \return JVM performance data address.
 */
ptrdiff_t TJvmInfo::getPerfMemoryAddress(JNIEnv *env) {
  /* Get perfomance file path. */

  char perfPath[PATH_MAX];
  char tempPath[PATH_MAX];
  char *tmpdir = NULL;
  char *username = NULL;
  char *separator = NULL;
  ptrdiff_t addr = 0;

  /* GetSystemProperty() on JVMTI gets value from Arguments::_system_properties
   * ONLY. */
  /* Arguments::_system_properties differents from
   * java.lang.System.getProperties().  */
  /* So, this function uses java.lang.System.getProperty(String). */
  tmpdir = GetSystemProperty(env, "java.io.tmpdir");
  username = GetSystemProperty(env, "user.name");
  separator = GetSystemProperty(env, "file.separator");

  /* Value check. */
  if ((tmpdir == NULL) || (username == NULL) || (separator == NULL)) {
    /* Free allocated memories. */
    if (tmpdir != NULL) {
      free(tmpdir);
    }
    if (username != NULL) {
      free(username);
    }
    if (separator != NULL) {
      free(separator);
    }
    return 0;
  }

  /* Check and fix path. e.g "/usr/local/". */
  if (strlen(tmpdir) > 0 && tmpdir[strlen(tmpdir) - 1] == '/') {
    tmpdir[strlen(tmpdir) - 1] = '\0';
  }

  /* Make performance file path. */
  sprintf(perfPath, "%s%shsperfdata_%s%s%d", tmpdir, separator, username,
          separator, getpid());
  sprintf(tempPath, "%stmp%shsperfdata_%s%s%d", separator, separator, username,
          separator, getpid());

  /* Clean up after get performance file path. */
  free(tmpdir);
  free(username);
  free(separator);

  /* Search entries in performance file. */

  addr = findPerfMemoryAddress(perfPath);
  if (addr == 0) {
    addr = findPerfMemoryAddress(tempPath);
  }

  return addr;
}

#endif

/*!
 * \brief Search GC-informaion address.
 * \param entries [in,out] List of search target entries.
 * \param count   [in]     Count of search target list.
 */
void TJvmInfo::SearchInfoInVMStruct(VMStructSearchEntry *entries, int count) {
  /* Copy performance data's header. */
  PerfDataPrologue prologue;
  memcpy(&prologue, (void *)perfAddr, sizeof(PerfDataPrologue));

/* Check performance data's header. */
#ifdef WORDS_BIGENDIAN
  if (unlikely(prologue.magic != (jint)0xcafec0c0)) {
#else
  if (unlikely(prologue.magic != (jint)0xc0c0feca)) {
#endif
    logger->printWarnMsg("Performance data's magic is broken.");
    this->perfAddr = 0;
    return;
  }

  ptrdiff_t ofs = 0;
  int name_len = 0;
  char *name = NULL;
  PerfDataEntry entry;
  /* Seek entries head. */
  void *readPtr = (void *)(perfAddr + prologue.entry_offset);

  /* Search target entries. */
  for (int Cnt = 0; Cnt < prologue.num_entries; Cnt++) {
    /* Copy entry data. */
    ofs = (ptrdiff_t)readPtr;
    memcpy(&entry, readPtr, sizeof(PerfDataEntry));

    /* Get entry name. */
    name_len = entry.data_offset - entry.name_offset;
    name = (char *)calloc(name_len + 1, sizeof(char));
    if (unlikely(name == NULL)) {
      logger->printWarnMsg("Couldn't allocate entry memory!");
      break;
    }

    /* Get entry name and check target entries. */
    readPtr = (void *)(ofs + entry.name_offset);
    memcpy(name, readPtr, name_len);
    for (int i = 0; i < count; i++) {
      /* If target entry. */
      if ((strcmp(entries[i].entryName, name) == 0) &&
          (entry.data_type == entries[i].entryType)) {
        /* Store found entry. */
        (*entries[i].entryValue) = (void *)(ofs + entry.data_offset);
      }
    }

    /* Clean up and move next entry. */
    free(name);
    readPtr = (void *)(ofs + entry.entry_length);
  }

  /* Verify necessary information. */
  for (int i = 0; i < count; i++) {
    /* If failure get target entry. */
    if (unlikely((*entries[i].entryValue) == NULL)) {
      /* JEP 220: Modular Run-Time Images */
      if (!isAfterJDK9() && (
          (strcmp(entries[i].entryName,
                 "java.property.java.endorsed.dirs") == 0) ||
          (strcmp(entries[i].entryName,
                 "sun.property.sun.boot.class.path") == 0))) {
        logger->printWarnMsg(
             "Necessary information isn't found in performance data. Entry: %s",
             entries[i].entryName);
      }
    }
  }
}

#ifdef USE_VMSTRUCTS

/*!
 * \brief Detect GC-informaion address.<br>
 *        Use external VM structure.
 * \sa    "jdk/src/share/classes/sun/tools/jstat/resources/jstat_options"
 */
void TJvmInfo::detectInfoAddress(void) {
  /* Get performance address. */
  this->perfAddr = this->getPerfMemoryAddress();
#else

/*!
 * \brief Detect GC-informaion address.<br>
 *        Use performance data file.
 * \param env [in] JNI environment object.
 * \sa    "jdk/src/share/classes/sun/tools/jstat/resources/jstat_options"
 */
void TJvmInfo::detectInfoAddress(JNIEnv *env) {
  /* Get performance address. */
  this->perfAddr = this->getPerfMemoryAddress(env);
#endif

  /* Check result. */
  if (unlikely(this->perfAddr == 0)) {
#ifdef USE_VMSTRUCTS
    logger->printWarnMsg("Necessary information isn't found in libjvm.so.");
#else
    logger->printWarnMsg(
        "Necessary information isn't found in performance file.");
#endif
    return;
  }

  VMStructSearchEntry entries[] = {
      {"sun.gc.collector.0.invocations", 'J', (void **)&this->_nowYGC},
      {"sun.gc.collector.1.invocations", 'J', (void **)&this->_nowFGC},
      {"sun.gc.generation.0.space.0.used", 'J', (void **)&this->_edenSize},
      {"sun.gc.generation.0.space.1.used", 'J', (void **)&this->_sur0Size},
      {"sun.gc.generation.0.space.2.used", 'J', (void **)&this->_sur1Size},
      {"sun.gc.generation.1.space.0.used", 'J', (void **)&this->_oldSize},
      {"sun.gc.collector.0.time", 'J', (void **)&this->_YGCTime},
      {"sun.gc.collector.1.time", 'J', (void **)&this->_FGCTime},
      {"sun.gc.cause", 'B', (void **)&this->_gcCause},
      {"sun.rt._sync_Parks", 'J', (void **)&this->_syncPark},
      {"java.threads.live", 'J', (void **)&this->_threadLive},
      {"sun.rt.safepointTime", 'J', (void **)&this->_safePointTime},
      {"sun.rt.safepoints", 'J', (void **)&this->_safePoints}};
  int entryCount = sizeof(entries) / sizeof(VMStructSearchEntry);
  SearchInfoInVMStruct(entries, entryCount);

  /* Refresh log data load flag. */
  loadLogFlag = (_syncPark != NULL) && (_threadLive != NULL) &&
                (_safePointTime != NULL) && (_safePoints != NULL);

  /* Get pointers of PermGen or Metaspace usage / capacity. */
  VMStructSearchEntry metaspaceEntry[] = {
      {"sun.gc.metaspace.used", 'J', (void **)&this->_metaspaceUsage},
      {"sun.gc.metaspace.maxCapacity", 'J',
       (void **)&this->_metaspaceCapacity}};
  VMStructSearchEntry permGenEntry[] = {
      {"sun.gc.generation.2.space.0.used", 'J',
       (void **)&this->_metaspaceUsage},
      {"sun.gc.generation.2.space.0.maxCapacity", 'J',
       (void **)&this->_metaspaceCapacity}};

  SearchInfoInVMStruct(this->isAfterCR6964458() ? metaspaceEntry : permGenEntry,
                       2);
}

/*!
 * \brief Detect delayed GC-informaion address.
 */
void TJvmInfo::detectDelayInfoAddress(void) {
  /* Check performance data's address. */
  if (unlikely(this->perfAddr == 0)) {
    return;
  }

  const int entryCount = 12;
  VMStructSearchEntry entries[entryCount] = {
      {"sun.os.hrt.frequency", 'J', (void **)&this->_freqTime},
      {"java.property.java.vm.version", 'B', (void **)&this->_vmVersion},
      {"java.property.java.vm.name", 'B', (void **)&this->_vmName},
      {"java.property.java.class.path", 'B', (void **)&this->_classPath},
      {"java.property.java.endorsed.dirs", 'B', (void **)&this->_endorsedPath},
      {"java.property.java.version", 'B', (void **)&this->_javaVersion},
      {"java.property.java.home", 'B', (void **)&this->_javaHome},
      {"sun.property.sun.boot.class.path", 'B', (void **)&this->_bootClassPath},
      {"java.rt.vmArgs", 'B', (void **)&this->_vmArgs},
      {"java.rt.vmFlags", 'B', (void **)&this->_vmFlags},
      {"sun.rt.javaCommand", 'B', (void **)&this->_javaCommand},
      {"sun.os.hrt.ticks", 'J', (void **)&this->_tickTime}};
  SearchInfoInVMStruct(entries, entryCount);

  /* Refresh delay log data load flag. */
  loadDelayLogFlag = (_freqTime != NULL) && (_vmVersion != NULL) &&
                     (_vmName != NULL) && (_classPath != NULL) &&
                     (_javaVersion != NULL) && (_javaHome != NULL) &&
                     (_vmArgs != NULL) && (_vmFlags != NULL) &&
                     (_javaCommand != NULL) && (_tickTime != NULL);

  /* JEP 220: Modular Run-Time Images */
  if (!isAfterJDK9()) {
    loadDelayLogFlag &= (_endorsedPath != NULL) && (_bootClassPath != NULL);
  }
}

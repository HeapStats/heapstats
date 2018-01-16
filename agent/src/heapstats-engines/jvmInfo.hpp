/*!
 * \file jvmInfo.hpp
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

#ifndef JVMINFO_H
#define JVMINFO_H

#include <jvmti.h>
#include <jni.h>

#include <stddef.h>

/*!
 * \brief Make HotSpot version
 */
#define MAKE_HS_VERSION(afterJDK9, major, minor, micro, build) \
  (((afterJDK9) << 30) | ((major) << 24) | ((minor) << 16) | ((micro) << 8) | (build))

/*!
 * \brief JVM performance header info.
 */
typedef struct {
  jint magic;           /*!< Magic number. - 0xcafec0c0 (bigEndian)      */
  jbyte byte_order;     /*!< Byte order of the buffer.                   */
  jbyte major_version;  /*!< Major version numbers.                      */
  jbyte minor_version;  /*!< Minor version numbers.                      */
  jbyte accessible;     /*!< Ready to access.                            */
  jint used;            /*!< Number of PerfData memory bytes used.       */
  jint overflow;        /*!< Number of bytes of overflow.                */
  jlong mod_time_stamp; /*!< Time stamp of last structural modification. */
  jint entry_offset;    /*!< Offset of the first PerfDataEntry.          */
  jint num_entries;     /*!< Number of allocated PerfData entries.       */
} PerfDataPrologue;

/*!
 * \brief Entry of JVM performance informations.
 */
typedef struct {
  jint entry_length;      /*!< Entry length in bytes.                       */
  jint name_offset;       /*!< Offset of the data item name.                */
  jint vector_length;     /*!< Length of the vector. If 0, then scalar.     */
  jbyte data_type;        /*!< Type of the data item. -
                               'B','Z','J','I','S','C','D','F','V','L','['  */
  jbyte flags;            /*!< Flags indicating misc attributes.            */
  jbyte data_units;       /*!< Unit of measure for the data type.           */
  jbyte data_variability; /*!< Variability classification of data type.     */
  jint data_offset;       /*!< Offset of the data item.                     */
} PerfDataEntry;

/*!
 * \brief Search entry of JVM performance informations in vmstruct.
 */
typedef struct {
  const char *entryName; /*!< Entry name string.         */
  const char entryType;  /*!< Entry type char.           */
  void **entryValue;     /*!< Pointer of store variable. */
} VMStructSearchEntry;

/*!
 * \brief Function type definition for JVM internal functions.<br>
 *        JVM_MaxMemory() / JVM_TotalMemory() / etc...
 * \return Memory size.
 */
typedef jlong(JNICALL *TGetMemoryFunc)(void);

/*!
 * \brief Value of gc cause when failure get GC cause.
 */
const static char UNKNOWN_GC_CAUSE[16]
    __attribute__((aligned(16))) = "unknown GCCause";

/*!
 * \brief Size of GC cause.
 */
const static int MAXSIZE_GC_CAUSE = 80;

/*!
 * \brief This class is used to get JVM performance information.
 */
class TJvmInfo {
 public:
  /*!
   * \brief TJvmInfo constructor.
   */
  TJvmInfo(void);
  /*!
   * \brief TJvmInfo destructor.
   */
  virtual ~TJvmInfo();

  /*!
   * \brief   Get heap memory size (New + Old)
   *          through the JVM internal function.
   * \return  Heap memory size.
   */
  inline jlong getMaxMemory(void) {
    return (this->maxMemFunc == NULL) ? -1 : (*this->maxMemFunc)();
  }

  /*!
   * \brief   Get total allocated heap memory size.
   * \return  Heap memory size.
   * \warning CAUTION!!: This function must be called
   *          after "VMInit" JVMTI event!<br>
   *          JVM_TotalMemory() is used "JavaThread" object.
   */
  inline jlong getTotalMemory(void) {
    return (this->totalMemFunc == NULL) ? -1 : (*this->totalMemFunc)();
  }

  /*!
   * \brief Get new generation size.
   * \return Size of new generation of heap.
   */
  inline jlong getNewAreaSize(void) {
    return (this->_edenSize != NULL && this->_sur0Size != NULL &&
            this->_sur1Size != NULL)
               ? *this->_edenSize + *this->_sur0Size + *this->_sur1Size
               : -1;
  };

  /*!
   * \brief Get tenured generation size.
   * \return Size of tenured generation of heap.
   */
  inline jlong getOldAreaSize(void) {
    return (this->_oldSize != NULL) ? *this->_oldSize : -1;
  };

  /*!
   * \brief Get PermGen or Metaspace usage.
   * \return Usage of PermGen or Metaspace.
   */
  inline jlong getMetaspaceUsage(void) {
    return (this->_metaspaceUsage != NULL) ? *this->_metaspaceUsage : -1;
  };

  /*!
   * \brief Get PermGen or Metaspace capacity.
   * \return Max capacity of PermGen or Metaspace.
   */
  inline jlong getMetaspaceCapacity(void) {
    return (this->_metaspaceCapacity != NULL) ? *this->_metaspaceCapacity : -1;
  };

  /*!
   * \brief Get raised full-GC count.
   * \return Raised full-GC count.
   */
  inline jlong getFGCCount(void) {
    return (this->_nowFGC != NULL) ? *this->_nowFGC : -1;
  };

  /*!
   * \brief Get raised young-GC count.
   * \return Raised young-GC count.
   */
  inline jlong getYGCCount(void) {
    return (this->_nowYGC != NULL) ? *this->_nowYGC : -1;
  };

  /*!
   * \brief Load string of GC cause.
   */
  void loadGCCause(void);

  /*!
   * \brief Reset stored gc information.
   */
  inline void resumeGCinfo(void) {
    /* Reset gc cause string. */
    this->SetUnknownGCCause();
    /* Refresh elapsed GC work time. */
    elapFGCTime = (this->_FGCTime != NULL) ? *this->_FGCTime : -1;
    elapYGCTime = (this->_YGCTime != NULL) ? *this->_YGCTime : -1;
    /* Reset full-GC flag. */
    this->fullgcFlag = false;
  };

  /*!
   * \brief Get GC cause string.
   * \return GC cause string.
   */
  inline char *getGCCause(void) { return this->gcCause; };

  /*!
   * \brief Get last GC worktime(msec).
   * \return Last GC work time.
   */
  inline jlong getGCWorktime(void) {
    jlong nowTime;
    jlong result = 0;
    /* Get time tick frequency. */
    jlong freqTime = (this->_freqTime != NULL) ? (*this->_freqTime / 1000) : 1;

    /* Check GC type. */
    if (fullgcFlag) {
      nowTime = (this->_FGCTime != NULL) ? *this->_FGCTime : 0;
      result = (nowTime >= 0) ? nowTime - elapFGCTime : 0;
    } else {
      nowTime = (this->_YGCTime != NULL) ? *this->_YGCTime : 0;
      result = (nowTime >= 0) ? nowTime - elapYGCTime : 0;
    }

    /* Calculate work time. */
    return (result / freqTime);
  };

  /*!
   * \brief Set full-GC flag.
   * \param isFullGC [in] Is raised full-GC ?
   */
  inline void setFullGCFlag(bool isFullGC) { this->fullgcFlag = isFullGC; }

  /*!
   * \brief Detect delayed GC-informaion address.
   */
  void detectDelayInfoAddress(void);

  /*!
   * \brief Get finished conflict count.
   * \return It's count of finished conflict of monitor.
   */
  inline jlong getSyncPark(void) {
    return ((_syncPark == NULL) ? -1 : *_syncPark);
  }

  /*!
   * \brief Get live thread count.
   * \return Now living thread count.
   */
  inline jlong getThreadLive(void) {
    return ((_threadLive == NULL) ? -1 : *_threadLive);
  }

  /*!
   * \brief Get safepoint time.
   * \return Total work time in safepoint.
   */
  inline jlong getSafepointTime(void) {
    /* Get time tick frequency. */
    jlong freqTime = (this->_freqTime != NULL) ? (*this->_freqTime / 1000) : 1;

    return ((_safePointTime == NULL) ? -1 : *_safePointTime / freqTime);
  }

  /*!
   * \brief Get safepoint count.
   * \return Count of work in safepoint.
   */
  inline jlong getSafepoints(void) {
    return ((_safePoints == NULL) ? -1 : *_safePoints);
  }

  /*!
   * \brief Get JVM version.
   * \return JVM version.
   */
  inline char *getVmVersion(void) { return this->_vmVersion; }

  /*!
   * \brief Set JVM version from system property (java.vm.version) .
   * \param jvmti  [in]  JVMTI Environment.
   * \return Process result.
   */
  bool setHSVersion(jvmtiEnv *jvmti);

  /*!
   * \brief Get JVM version as uint value.
   */
  inline unsigned int getHSVersion(void) { return this->_hsVersion; }

  /*!
   * \brief Get JVM name.
   * \return JVM name.
   */
  inline char *getVmName(void) { return this->_vmName; }
  /*!
   * \brief Get JVM class path.
   * \return JVM class path.
   */
  inline char *getClassPath(void) { return this->_classPath; }
  /*!
   * \brief Get JVM endorsed path.
   * \return JVM endorsed path.
   */
  inline char *getEndorsedPath(void) { return this->_endorsedPath; }

  /*!
   * \brief Decision for JDK-7046558: G1: concurrent marking optimizations.
   * https://bugs.openjdk.java.net/browse/JDK-7046558
   * http://hg.openjdk.java.net/hsx/hotspot-gc/hotspot/rev/842b840e67db
   */
  inline bool isAfterCR7046558(void) {
    // hs22.0-b03
    return (this->_hsVersion >= MAKE_HS_VERSION(0, 22, 0, 0, 3));
  }

  /*!
   * \brief Decision for JDK-7017732: move static fields into Class to prepare
   * for perm gen removal
   * https://bugs.openjdk.java.net/browse/JDK-7017732
   */
  inline bool isAfterCR7017732(void) {
    // hs21.0-b06
    return (this->_hsVersion >= MAKE_HS_VERSION(0, 21, 0, 0, 6));
  }

  /*!
   * \brief Decision for JDK-6964458: Reimplement class meta-data storage to use
   * native memory
   *        (PermGen Removal)
   * https://bugs.openjdk.java.net/browse/JDK-6964458
   * http://hg.openjdk.java.net/hsx/hotspot-rt-gate/hotspot/rev/da91efe96a93
   */
  inline bool isAfterCR6964458(void) {
    // hs25.0-b01
    return (this->_hsVersion >= MAKE_HS_VERSION(0, 25, 0, 0, 1));
  }

  /*!
   * \brief Decision for JDK-8000213: NPG: Should have renamed arrayKlass and
   * typeArrayKlass
   * https://bugs.openjdk.java.net/browse/JDK-8000213
   * http://hg.openjdk.java.net/hsx/hotspot-rt/hotspot/rev/d8ce2825b193
   */
  inline bool isAfterCR8000213(void) {
    // hs25.0-b04
    return (this->_hsVersion >= MAKE_HS_VERSION(0, 25, 0, 0, 4));
  }

  /*!
   * \brief Decision for JDK-8027746: Remove do_gen_barrier template parameter
   * in G1ParCopyClosure
   * https://bugs.openjdk.java.net/browse/JDK-8027746
   */
  inline bool isAfterCR8027746(void) {
    // hs25.20-b02
    return (this->_hsVersion >= MAKE_HS_VERSION(0, 25, 20, 0, 2));
  }

  /*!
   * \brief Decision for JDK-8049421: G1 Class Unloading after completing a
   * concurrent mark cycle
   * https://bugs.openjdk.java.net/browse/JDK-8049421
   * http://hg.openjdk.java.net/jdk8u/jdk8u/hotspot/rev/2c6ef90f030a
   */
  inline bool isAfterCR8049421(void) {
    // hs25.40-b05
    return (this->_hsVersion >= MAKE_HS_VERSION(0, 25, 40, 0, 5));
  }

  /*!
   * \brief Decision for JDK-8004883:  NPG: clean up anonymous class fix
   * https://bugs.openjdk.java.net/browse/JDK-8004883
   */
  inline bool isAfterCR8004883(void) {
    // hs25.0-b14
    return (this->_hsVersion >= MAKE_HS_VERSION(0, 25, 0, 0, 14));
  }

  /*!
   * \brief Decision for JDK-8003424: Enable Class Data Sharing for
   * CompressedOops
   * https://bugs.openjdk.java.net/browse/JDK-8003424
   * http://hg.openjdk.java.net/hsx/hotspot-rt/hotspot/rev/740e263c80c6
   */
  inline bool isAfterCR8003424(void) {
    // hs25.0-b46
    return (this->_hsVersion >= MAKE_HS_VERSION(0, 25, 0, 0, 46));
  }

  /*!
   * \brief Decision for JDK-8015107: NPG: Use consistent naming for metaspace
   * concepts
   * https://bugs.openjdk.java.net/browse/JDK-8015107
   */
  inline bool isAfterCR8015107(void) {
    // hs25.0-b51
    return (this->_hsVersion >= MAKE_HS_VERSION(0, 25, 0, 0, 51));
  }

  /*!
   * \brief Running on JDK 9 or not.
   */
  inline bool isAfterJDK9(void) {
    return (this->_hsVersion >= MAKE_HS_VERSION(1, 9, 0, 0, 0));
  }

  /*!
   * \brief Get Java version.
   * \return Java version.
   */
  inline char *getJavaVersion(void) { return this->_javaVersion; }
  /*!
   * \brief Get JAVA_HOME.
   * \return JAVA_HOME.
   */
  inline char *getJavaHome(void) { return this->_javaHome; }
  /*!
   * \brief Get JVM boot class path.
   * \return JVM boot class path.
   */
  inline char *getBootClassPath(void) { return this->_bootClassPath; }
  /*!
   * \brief Get JVM arguments.
   * \return JVM arguments.
   */
  inline char *getVmArgs(void) { return this->_vmArgs; }
  /*!
   * \brief Get JVM flags.
   * \return JVM flags.
   */
  inline char *getVmFlags(void) { return this->_vmFlags; }
  /*!
   * \brief Get Java application arguments.
   * \return Java application arguments.
   */
  inline char *getJavaCommand(void) { return this->_javaCommand; }
  /*!
   * \brief Get JVM running timetick.
   * \return JVM running timetick.
   */
  inline jlong getTickTime(void) {
    /* Get time tick frequency. */
    jlong freqTime = (this->_freqTime != NULL) ? (*this->_freqTime / 1000) : 1;

    return ((_tickTime == NULL) ? -1 : *_tickTime / freqTime);
  }

#ifdef USE_VMSTRUCTS

  /*!
   * \brief Detect GC-informaion address.<br>
   *        Use external VM structure.
   * \sa "jdk/src/share/classes/sun/tools/jstat/resources/jstat_options"
   */
  void detectInfoAddress(void);
#else

  /*!
   * \brief Detect GC-informaion address.<br>
   *        Use performance data file.
   * \param env [in] JNI environment object.
   * \sa "jdk/src/share/classes/sun/tools/jstat/resources/jstat_options"
   */
  void detectInfoAddress(JNIEnv *env);
#endif

  void SetUnknownGCCause(void);

 protected:
  /*!
   * \brief Address of java.lang.getMaxMemory().
   */
  TGetMemoryFunc maxMemFunc;

  /*!
   * \brief Address of java.lang.getTotalMemory().
   */
  TGetMemoryFunc totalMemFunc;

#ifdef USE_VMSTRUCTS
  /*!
   * \brief Get JVM performance data address.<br>
   *        Use external VM structure.
   * \return JVM performance data address.
   */
  virtual ptrdiff_t getPerfMemoryAddress(void);
#else

  /*!
   * \brief Search JVM performance data address in memory.
   * \param path [in] Path of hsperfdata.
   * \return JVM performance data address.
   */
  ptrdiff_t findPerfMemoryAddress(char *path);

  /*!
   * \brief Get JVM performance data address.<br>
   *        Use performance data file.
   * \param env [in] JNI environment object.
   * \return JVM performance data address.
   */
  virtual ptrdiff_t getPerfMemoryAddress(JNIEnv *env);
#endif

  /*!
   * \brief Search GC-informaion address.
   * \param entries [in,out] List of search target entries.
   * \param count   [in]     Count of search target list.
   */
  virtual void SearchInfoInVMStruct(VMStructSearchEntry *entries, int count);

 private:
  /*!
   * \brief JVM running performance data pointer.
   */
  ptrdiff_t perfAddr;
  /*!
   * \brief Now Full-GC count.
   */
  jlong *_nowFGC;
  /*!
   * \brief Now Young-GC count.
   */
  jlong *_nowYGC;
  /*!
   * \brief Now eden space size.
   */
  jlong *_edenSize;
  /*!
   * \brief Now survivor 0 space size.
   */
  jlong *_sur0Size;
  /*!
   * \brief Now survivor 1 space size.
   */
  jlong *_sur1Size;
  /*!
   * \brief Tenured generation space size.
   */
  jlong *_oldSize;
  /*!
   * \brief PermGen or Metaspace usage.
   */
  jlong *_metaspaceUsage;
  /*!
   * \brief PermGen or Metaspace capacity.
   */
  jlong *_metaspaceCapacity;
  /*!
   * \brief Total Full-GC work time.
   */
  jlong *_FGCTime;
  /*!
   * \brief Total Young-GC work time.
   */
  jlong *_YGCTime;
  /*!
   * \brief Elapsed Full-GC work time.
   */
  jlong elapFGCTime;
  /*!
   * \brief Elapsed Young-GC work time.
   */
  jlong elapYGCTime;
  /*!
   * \brief Frequency timetick.
   */
  jlong *_freqTime;
  /*!
   * \brief GC cause.
   */
  char *_gcCause;
  /*!
   * \brief Stored GC cause.
   */
  char *gcCause;
  /*!
   * \brief Full-GC flag. Used by getGCWorktime().
   */
  bool fullgcFlag;
  /*!
   * \brief Log data loaded flag.
   */
  bool loadLogFlag;
  /*!
   * \brief Delay log data loaded flag.
   */
  bool loadDelayLogFlag;

  /*!
   * \brief It's count of finished conflict of monitor.
   */
  jlong *_syncPark;
  /*!
   * \brief Now living thread count.
   */
  jlong *_threadLive;
  /*!
   * \brief Total work time in safepoint.
   */
  jlong *_safePointTime;
  /*!
   * \brief Count of work in safepoint.
   */
  jlong *_safePoints;

  /*!
   * \brief JVM version as uint
   */
  unsigned int _hsVersion;

  /*!
   * \brief JVM version.
   */
  char *_vmVersion;
  /*!
   * \brief JVM name.
   */
  char *_vmName;
  /*!
   * \brief JVM class path.
   */
  char *_classPath;
  /*!
   * \brief JVM endorsed path.
   */
  char *_endorsedPath;
  /*!
   * \brief Java version.
   */
  char *_javaVersion;
  /*!
   * \brief JAVA_HOME.
   */
  char *_javaHome;
  /*!
   * \brief JVM boot class path.
   */
  char *_bootClassPath;
  /*!
   * \brief JVM arguments.
   */
  char *_vmArgs;
  /*!
   * \brief JVM flags.
   */
  char *_vmFlags;
  /*!
   * \brief Java application arguments.
   */
  char *_javaCommand;
  /*!
   * \brief JVM running timetick.
   */
  jlong *_tickTime;
};

/* Include optimized inline functions. */
#ifdef AVX
#include "arch/x86/avx/jvmInfo.inline.hpp"
#elif defined(SSE4) || defined(SSE3)
#include "arch/x86/sse3/jvmInfo.inline.hpp"
#elif defined(SSE2)
#include "arch/x86/sse2/jvmInfo.inline.hpp"
#elif defined(NEON)
#include "arch/arm/neon/jvmInfo.inline.hpp"
#else

inline void TJvmInfo::loadGCCause(void) {
  __builtin_memcpy(this->gcCause, this->_gcCause, MAXSIZE_GC_CAUSE);
};

inline void TJvmInfo::SetUnknownGCCause(void) {
  __builtin_memcpy(this->gcCause, UNKNOWN_GC_CAUSE, 16);
}

#endif

#endif  // JVMINFO_H

/*!
 * \file vmVariables.cpp
 * \brief This file includes variables in HotSpot VM.<br>
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

#include "globals.hpp"
#include "vmVariables.hpp"

/* Variables */
TVMVariables *TVMVariables::inst = NULL;

/*!
 * \brief Pointer of "_collectedHeap" field in Universe.<br>
 *        Use to calling "is_in_permanent" function.
 */
void *collectedHeap;

/*!
 * \brief Constructor of TVMVariables
 * \param sym   [in] Symbol finder of libjvm.so .
 * \param scan  [in] VMStruct scanner.
 */
TVMVariables::TVMVariables(TSymbolFinder *sym, TVMStructScanner *scan)
    : symFinder(sym), vmScanner(scan) {
  /* Member initialization */
  isCOOP = false;
  useParallel = false;
  useParOld = false;
  useCMS = false;
  useG1 = false;
  CMS_collectorState = NULL;
  collectedHeap = NULL;
  clsSizeOopDesc = 0;
  clsSizeKlassOop = 0;
  clsSizeNarrowOop = 0;
  clsSizeKlass = 0;
  clsSizeInstanceKlass = 0;
  clsSizeArrayOopDesc = 0;
  ofsKlassAtOop = -1;
  ofsCoopKlassAtOop = -1;
  ofsMarkAtOop = -1;
  ofsNameAtKlass = -1;
  ofsLengthAtSymbol = -1;
  ofsBodyAtSymbol = -1;
  ofsVTableSizeAtInsKlass = -1;
  ofsITableSizeAtInsKlass = -1;
  ofsStaticFieldSizeAtInsKlass = -1;
  ofsNonstaticOopMapSizeAtInsKlass = -1;
  ofsKlassOffsetInBytesAtOopDesc = -1;
  narrowOffsetBase = 0;
  narrowOffsetShift = 0;
  narrowKlassOffsetBase = 0;
  narrowKlassOffsetShift = 0;
  lockMaskInPlaceMarkOop = 0;
  marked_value = 0;
  cmsBitMap_startWord = NULL;
  cmsBitMap_shifter = 0;
  cmsBitMap_startAddr = NULL;
  BitsPerWordMask = 0;
  safePointState = NULL;
  g1StartAddr = NULL;
  ofsJavaThreadOsthread = -1;
  ofsJavaThreadThreadObj = -1;
  ofsJavaThreadThreadState = -1;
  ofsThreadCurrentPendingMonitor = -1;
  ofsOSThreadThreadId = -1;
  ofsObjectMonitorObject = -1;
  threads_lock = NULL;
  youngGen = NULL;
  youngGenStartAddr = NULL;
  youngGenSize = 0;

#ifdef __LP64__
  HeapWordSize = 8;
  LogHeapWordSize = 3;
  HeapWordsPerLong = 1;
  LogBitsPerWord = 6;
  BitsPerWord = 64;
#else
  HeapWordSize = 4;
  LogHeapWordSize = 2;
  HeapWordsPerLong = 2;
  LogBitsPerWord = 5;
  BitsPerWord = 32;
#endif
}

/*!
 * \brief Instance initializer.
 * \param sym   [in] Symbol finder of libjvm.so .
 * \param scan  [in] VMStruct scanner.
 * \return Singleton instance of TVMVariables.
 */
TVMVariables *TVMVariables::initialize(TSymbolFinder *sym,
                                       TVMStructScanner *scan) {
  /* Create instance */
  try {
    inst = new TVMVariables(sym, scan);
  } catch (...) {
    logger->printCritMsg("Cannot initialize TVMVariables.");
    return NULL;
  }

  bool result = inst->getUnrecognizedOptions() &&
                inst->getValuesFromVMStructs() && inst->getValuesFromSymbol();

  return result ? inst : NULL;
}

/*!
 * \brief Get unrecognized options (-XX)
 * \return Result of this function.
 */
bool TVMVariables::getUnrecognizedOptions(void) {
  /* Search basically unrecognized flags. */
  struct {
    const char *symbolStr;
    bool *flagPtr;
  } flagList[] = {
#ifdef __LP64__
        {"UseCompressedOops", &this->isCOOP},
#endif
        {"UseParallelGC", &this->useParallel},
        {"UseParallelOldGC", &this->useParOld},
        {"UseConcMarkSweepGC", &this->useCMS},
        {"UseG1GC", &this->useG1},
        /* End marker. */
        {NULL, NULL}};

  for (int i = 0; flagList[i].flagPtr != NULL; i++) {
    bool *tempPtr = NULL;

    /* Search symbol. */
    tempPtr = (bool *)symFinder->findSymbol(flagList[i].symbolStr);

    if (unlikely(tempPtr == NULL)) {
      logger->printCritMsg("%s not found.", flagList[i].symbolStr);
      return false;
    }

    *(flagList[i].flagPtr) = *tempPtr;
  }

  /* Search "Threads_lock" symbol. */
  threads_lock = symFinder->findSymbol("Threads_lock");
  if (unlikely(threads_lock == NULL)) {
    logger->printCritMsg("Threads_lock not found.");
    return false;
  }

#ifdef __LP64__
  if (jvmInfo->isAfterCR6964458()) {
    bool *tempPtr = NULL;
    const char *target_sym = jvmInfo->isAfterCR8015107()
                                 ? "UseCompressedClassPointers"
                                 : "UseCompressedKlassPointers";

    /* Search symbol. */
    tempPtr = (bool *)symFinder->findSymbol(target_sym);

    if (unlikely(tempPtr == NULL)) {
      logger->printCritMsg("%s not found.", target_sym);
      return false;
    } else {
      isCOOP = *tempPtr;
    }
  }
#endif

  logger->printDebugMsg("Compressed Class = %s", isCOOP ? "true" : "false");

  return true;
}

/*!
 * \brief Get HotSpot values through VMStructs.
 * \return Result of this function.
 */
bool TVMVariables::getValuesFromVMStructs(void) {
  TOffsetNameMap ofsMap[] = {
      {"oopDesc", "_metadata._klass", &ofsKlassAtOop, NULL},
      {"oopDesc", "_metadata._compressed_klass", &ofsCoopKlassAtOop, NULL},
      {"oopDesc", "_mark", &ofsMarkAtOop, NULL},
      {"Klass", "_name", &ofsNameAtKlass, NULL},
      {"JavaThread", "_osthread", &ofsJavaThreadOsthread, NULL},
      {"JavaThread", "_threadObj", &ofsJavaThreadThreadObj, NULL},
      {"JavaThread", "_thread_state", &ofsJavaThreadThreadState, NULL},
      {"Thread", "_current_pending_monitor", &ofsThreadCurrentPendingMonitor,
       NULL},
      {"OSThread", "_thread_id", &ofsOSThreadThreadId, NULL},
      {"ObjectMonitor", "_object", &ofsObjectMonitorObject, NULL},

      /*
       * For CR6990754.
       * Use native memory and reference counting to implement SymbolTable.
       */
      {"symbolOopDesc", "_length", &ofsLengthAtSymbol, NULL},
      {"symbolOopDesc", "_body", &ofsBodyAtSymbol, NULL},
      {"Symbol", "_length", &ofsLengthAtSymbol, NULL},
      {"Symbol", "_body", &ofsBodyAtSymbol, NULL},

      {"instanceKlass", "_vtable_len", &ofsVTableSizeAtInsKlass, NULL},
      {"instanceKlass", "_itable_len", &ofsITableSizeAtInsKlass, NULL},
      {"instanceKlass", "_static_field_size", &ofsStaticFieldSizeAtInsKlass,
       NULL},
      {"instanceKlass", "_nonstatic_oop_map_size",
       &ofsNonstaticOopMapSizeAtInsKlass, NULL},

      /* For CR6964458 */
      {"InstanceKlass", "_vtable_len", &ofsVTableSizeAtInsKlass, NULL},
      {"InstanceKlass", "_itable_len", &ofsITableSizeAtInsKlass, NULL},
      {"InstanceKlass", "_static_field_size", &ofsStaticFieldSizeAtInsKlass,
       NULL},
      {"InstanceKlass", "_nonstatic_oop_map_size",
       &ofsNonstaticOopMapSizeAtInsKlass, NULL},

      /* For JDK-8148047 */
      {"Klass", "_vtable_len", &ofsVTableSizeAtInsKlass, NULL},

      /* End marker. */
      {NULL, NULL, NULL, NULL}};

  this->vmScanner->GetDataFromVMStructs(ofsMap);

  if (unlikely(ofsKlassAtOop == -1 || ofsCoopKlassAtOop == -1 ||
               ofsNameAtKlass == -1 || ofsLengthAtSymbol == -1 ||
               ofsJavaThreadOsthread == -1 || ofsJavaThreadThreadObj == -1 ||
               ofsJavaThreadThreadState == -1 ||
               ofsThreadCurrentPendingMonitor == -1 ||
               ofsOSThreadThreadId == -1 || ofsObjectMonitorObject == -1 ||
               ofsBodyAtSymbol == -1 || ofsVTableSizeAtInsKlass == -1 ||
               ofsITableSizeAtInsKlass == -1 ||
               (!jvmInfo->isAfterCR7017732() &&
                (ofsStaticFieldSizeAtInsKlass == -1)) ||
               ofsNonstaticOopMapSizeAtInsKlass == -1)) {
    logger->printCritMsg("Cannot get values from VMStructs.");
    return false;
  }

  TTypeSizeMap typeMap[] = {/*
                             * After CR6964458.
                             * Not exists class "klassOopDesc".
                             */
                            {"klassOopDesc", &clsSizeKlassOop},
                            {"oopDesc", &clsSizeOopDesc},
                            {"instanceKlass", &clsSizeInstanceKlass},
                            {"InstanceKlass", &clsSizeInstanceKlass},
                            {"arrayOopDesc", &clsSizeArrayOopDesc},
                            {"narrowOop", &clsSizeNarrowOop},
                            /* End marker. */
                            {NULL, NULL}};

  vmScanner->GetDataFromVMTypes(typeMap);

  if (unlikely((!jvmInfo->isAfterCR6964458() && clsSizeKlassOop == 0) ||
               clsSizeOopDesc == 0 || clsSizeInstanceKlass == 0 ||
               clsSizeArrayOopDesc == 0 || clsSizeNarrowOop == 0)) {
    logger->printCritMsg("Cannot get values from VMTypes.");
    return false;
  }

  TLongConstMap longMap[] = {
      {"markOopDesc::lock_mask_in_place", &lockMaskInPlaceMarkOop},
      {"markOopDesc::marked_value", &marked_value},
      /* End marker. */
      {NULL, NULL}};

  vmScanner->GetDataFromVMLongConstants(longMap);

  if (unlikely((lockMaskInPlaceMarkOop == 0) || (marked_value == 0))) {
    logger->printCritMsg("Cannot get values from VMLongConstants.");
    return false;
  }

  TIntConstMap intMap[] = {{"HeapWordSize", &HeapWordSize},
                           {"LogHeapWordSize", &LogHeapWordSize},
                           /* End marker. */
                           {NULL, NULL}};

  vmScanner->GetDataFromVMIntConstants(intMap);

  return true;

}

/*!
 * \brief Get values which are decided after VMInit JVMTI event.
 *        This function should be called after JVMTI VMInit event.
 * \return Process result.
 */
bool TVMVariables::getValuesAfterVMInit(void) {
  int *narrowOffsetShiftBuf = NULL;
  int *narrowKlassOffsetShiftBuf = NULL;

  TOffsetNameMap ofsMap[] = {
      {"Universe", "_collectedHeap", NULL, &collectedHeap},
      {"Universe", "_narrow_oop._base", NULL, (void **)&narrowOffsetBase},
      {"Universe", "_narrow_oop._shift", NULL, (void **)&narrowOffsetShiftBuf},
      {"Universe", "_narrow_klass._base", NULL,
       (void **)&narrowKlassOffsetBase},
      {"Universe", "_narrow_klass._shift", NULL,
       (void **)&narrowKlassOffsetShiftBuf},

      /* End marker. */
      {NULL, NULL, NULL, NULL}};

  this->vmScanner->GetDataFromVMStructs(ofsMap);

  if (unlikely(collectedHeap == NULL || narrowOffsetBase == 0 ||
               narrowOffsetShiftBuf == NULL)) {
    logger->printCritMsg("Cannot get values from VMStructs.");
    return false;
  }

  narrowOffsetShift = *narrowOffsetShiftBuf;

  if (!jvmInfo->isAfterCR8003424()) {
    narrowKlassOffsetBase = narrowOffsetBase;
    narrowKlassOffsetShift = narrowOffsetShift;
  } else {
    if (unlikely(narrowKlassOffsetShiftBuf == NULL)) {
      logger->printCritMsg("Cannot get values from VMStructs.");
      return false;
    }

    narrowKlassOffsetShift = *narrowKlassOffsetShiftBuf;
  }

  collectedHeap = (*(void **)collectedHeap);
  narrowOffsetBase = (ptrdiff_t) * (void **)narrowOffsetBase;
  narrowKlassOffsetBase = (ptrdiff_t) * (void **)narrowKlassOffsetBase;

  bool result = true;

  if (this->useCMS) {
    result =
        this->getCMSValuesFromVMStructs() && this->getCMSValuesFromSymbol();
  } else if (this->useG1) {
    result = this->getG1ValuesFromVMStructs();
  }

  return result;
}

/*!
 * \brief Get HotSpot values through VMStructs which is related to CMS.
 * \return Result of this function.
 */
bool TVMVariables::getCMSValuesFromVMStructs(void) {
  void *cmsCollector = NULL;
  off_t offsetLowAtVirtualSpace = -1;
  off_t offsetCmsStartWord = -1;
  off_t offsetCmsShifter = -1;
  off_t offsetCmsMapAtCollector = -1;
  off_t offsetCmsVirtualSpace = -1;
  off_t offsetGens = -1;
  off_t offsetYoungGen = -1;
  off_t offsetYoungGenReserved = -1;
  off_t offsetMemRegionStart = -1;
  off_t offsetMemRegionWordSize = -1;
  TOffsetNameMap ofsMap[] = {
      {"CMSBitMap", "_virtual_space", &offsetCmsVirtualSpace, NULL},
      {"CMSBitMap", "_bmStartWord", &offsetCmsStartWord, NULL},
      {"CMSBitMap", "_shifter", &offsetCmsShifter, NULL},
      {"CMSCollector", "_markBitMap", &offsetCmsMapAtCollector, NULL},
      {"ConcurrentMarkSweepThread", "_collector", NULL, &cmsCollector},
      {"VirtualSpace", "_low", &offsetLowAtVirtualSpace, NULL},
      {"GenCollectedHeap", "_gens", &offsetGens, NULL},
      {"GenCollectedHeap", "_young_gen", &offsetYoungGen, NULL},
      {"Generation", "_reserved", &offsetYoungGenReserved, NULL},
      {"MemRegion", "_start", &offsetMemRegionStart, NULL},
      {"MemRegion", "_word_size", &offsetMemRegionWordSize, NULL},
      /* End marker. */
      {NULL, NULL, NULL, NULL}};

  this->vmScanner->GetDataFromVMStructs(ofsMap);

  /* "CMSBitMap::_bmStartWord" is not found in vmstruct. */
  if (jvmInfo->isAfterCR6964458()) {
    off_t bmWordSizeOfs = -1;
    TOffsetNameMap bmWordSizeMap[] = {
        {"CMSBitMap", "_bmWordSize", &bmWordSizeOfs, NULL},
        {NULL, NULL, NULL, NULL}};

    this->vmScanner->GetDataFromVMStructs(bmWordSizeMap);

    if (likely(bmWordSizeOfs != -1)) {
      /*
       * CMSBitMap::_bmStartWord is appeared before _bmWordSize.
       * See CMSBitMap definition in
       * hotspot/src/share/vm/gc_implementation/concurrentMarkSweep/
       * concurrentMarkSweepGeneration.hpp
       */
      offsetCmsStartWord = bmWordSizeOfs - sizeof(void *);
    }
  }

  if (unlikely(offsetCmsVirtualSpace == -1 || offsetCmsStartWord == -1 ||
               offsetCmsShifter == -1 || offsetCmsMapAtCollector == -1 ||
               cmsCollector == NULL || offsetLowAtVirtualSpace == -1 ||
               (offsetGens == -1 && offsetYoungGen == -1) ||
               offsetYoungGenReserved == -1 || offsetMemRegionStart == -1 ||
               offsetMemRegionWordSize == -1 ||
               *(void **)cmsCollector == NULL)) {
    logger->printCritMsg("Cannot get CMS values from VMStructs.");
    return false;
  }

  /* Calculate about CMS bitmap information. */
  cmsCollector = *(void **)cmsCollector;
  ptrdiff_t cmsBitmapPtr = (ptrdiff_t)cmsCollector + offsetCmsMapAtCollector;
  cmsBitMap_startWord = *(void **)(cmsBitmapPtr + offsetCmsStartWord);
  cmsBitMap_shifter = *(int *)(cmsBitmapPtr + offsetCmsShifter);

  ptrdiff_t virtSpace = cmsBitmapPtr + offsetCmsVirtualSpace;
  cmsBitMap_startAddr = *(size_t **)(virtSpace + offsetLowAtVirtualSpace);

  youngGen = (offsetGens == -1) // JDK-8061802
                ? *(void **)incAddress(collectedHeap, offsetYoungGen)
                : *(void **)incAddress(collectedHeap, offsetGens);
  void *youngGenReserved = incAddress(youngGen, offsetYoungGenReserved);
  youngGenStartAddr =
               *(void **)incAddress(youngGenReserved, offsetMemRegionStart);
  size_t youngGenWordSize =
               *(size_t *)incAddress(youngGenReserved, offsetMemRegionWordSize);
  youngGenSize = youngGenWordSize * HeapWordSize;

  if (unlikely((cmsBitMap_startWord == NULL) ||
               (cmsBitMap_startAddr == NULL) ||
               (youngGen == NULL) || (youngGenStartAddr == NULL))) {
    logger->printCritMsg("Cannot calculate CMS values.");
    return false;
  }

  BitsPerWordMask = BitsPerWord - 1;
  return true;
}

/*!
 * \brief Get HotSpot values through symbol table which is related to CMS.
 * \return Result of this function.
 */
bool TVMVariables::getCMSValuesFromSymbol(void) {
  /* Search symbol for CMS state. */
  CMS_collectorState =
      (int *)symFinder->findSymbol("_ZN12CMSCollector15_collectorStateE");

  bool result = (CMS_collectorState != NULL);
  if (unlikely(!result)) {
    logger->printCritMsg("CollectorState not found.");
  }

  return result;
}

/*!
 * \brief Get HotSpot values through VMStructs which is related to G1.
 * \return Result of this function.
 */
bool TVMVariables::getG1ValuesFromVMStructs(void) {
  off_t offsetReserved = -1;
  off_t offsetMemRegionStart = -1;
  TOffsetNameMap ofsMap[] = {
      {"CollectedHeap", "_reserved", &offsetReserved, NULL},
      {"MemRegion", "_start", &offsetMemRegionStart, NULL},
      /* End marker. */
      {NULL, NULL, NULL, NULL}};

  vmScanner->GetDataFromVMStructs(ofsMap);

  if (unlikely(offsetReserved == -1 || offsetMemRegionStart == -1 ||
               collectedHeap == NULL)) {
    logger->printCritMsg("Cannot get G1 values from VMStructs.");
    return false;
  }

  /* Calculate about G1GC memory information. */
  void *reservedRegion = *(void **)incAddress(collectedHeap, offsetReserved);
  this->g1StartAddr = incAddress(reservedRegion, offsetMemRegionStart);
  if (unlikely(reservedRegion == NULL || g1StartAddr == NULL)) {
    logger->printCritMsg("Cannot calculate G1 values.");
    return false;
  }

  return true;
}

/*!
 * \brief Get HotSpot values through symbol table.
 * \return Result of this function.
 */
bool TVMVariables::getValuesFromSymbol(void) {
  /* Search symbol for integer information. */
  struct {
    const char *symbol;
    const char *subSymbol;
    int *pointer;
  } intSymList[] = {
        {"HeapWordsPerLong", "_ZL16HeapWordsPerLong", &HeapWordsPerLong},
        {"LogBitsPerWord", "_ZL14LogBitsPerWord", &LogBitsPerWord},
        {"BitsPerWord", "_ZL11BitsPerWord", &BitsPerWord},
        /* End marker. */
        {NULL, NULL, NULL}};

  for (int i = 0; intSymList[i].pointer != NULL; i++) {
    int *intPos = NULL;
    intPos = (int *)symFinder->findSymbol(intSymList[i].symbol);

    if (unlikely(intPos == NULL)) {
      intPos = (int *)symFinder->findSymbol(intSymList[i].subSymbol);
    }

    if (unlikely(intPos == NULL)) {
      logger->printDebugMsg("%s not found. Use default value.",
                            intSymList[i].symbol);
    } else {
      (*intSymList[i].pointer) = (*intPos);
    }
  }

  /* Search symbol of variable stored safepoint state. */
  safePointState = (int *)this->symFinder->findSymbol(SAFEPOINT_STATE_SYMBOL);
  if (unlikely(safePointState == NULL)) {
    logger->printWarnMsg("safepoint_state not found.");
    return false;
  }

  return true;
}

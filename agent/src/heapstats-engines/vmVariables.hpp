/*!
 * \file vmVariables.hpp
 * \brief This file includes variables in HotSpot VM.
 * Copyright (C) 2014-2015 Yasumasa Suenaga
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

#ifndef VMVARIABLES_H
#define VMVARIABLES_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#include "symbolFinder.hpp"
#include "vmStructScanner.hpp"

/* Macros for symbol */

/*!
 * \brief String of symbol which is safepoint state variable.
 */
#define SAFEPOINT_STATE_SYMBOL "_ZN20SafepointSynchronize6_stateE"

/* extern variables */
extern "C" void *collectedHeap;

/*!
 * \brief This class gathers/provides variables from HotSpot VM.
 */
class TVMVariables {
 private:
  /* Unrecognized option (-XX) */

  /*!
   * \brief Value of "-XX:UseCompressedOops" or
   * "-XX:UseCompressedClassPointers".
   */
  bool isCOOP;

  /*!
   * \brief Value of "-XX:UseParallelGC".
   */
  bool useParallel;

  /*!
   * \brief Value of "-XX:UseParallelOldGC".
   */
  bool useParOld;

  /*!
   * \brief Value of "-XX:UseConcMarkSweepGC".
   */
  bool useCMS;

  /*!
   * \brief Value of "-XX:UseG1GC".
   */
  bool useG1;

  /* Internal value in HotSpot VM */

  /*!
   * \brief CMS collector state pointer.
   * \sa concurrentMarkSweepGeneration.hpp<br>
   *     at hotspot/src/share/vm/gc_implementation/concurrentMarkSweep/<br>
   *     enum CollectorState
   */
  int *CMS_collectorState;

  /*!
   * \brief Size of "oopDesc" class in JVM.
   */
  uint64_t clsSizeOopDesc;

  /*!
   * \brief Size of "klassOopDesc" class in JVM.
   */
  uint64_t clsSizeKlassOop;

  /*!
   * \brief Size of "narrowOop" class in JVM.
   */
  uint64_t clsSizeNarrowOop;

  /*!
   * \brief Size of "Klass" class in JVM.
   */
  uint64_t clsSizeKlass;

  /*!
   * \brief Size of "instanceKlass" class in JVM.
   */
  uint64_t clsSizeInstanceKlass;

  /*!
   * \brief Size of "arrayOopDesc" class in JVM.
   */
  uint64_t clsSizeArrayOopDesc;

  /*!
   * \brief Offset of "oopDesc" class's "_metadata._klass" field.
   *        This field is stored class object when JVM don't using COOP.
   */
  off_t ofsKlassAtOop;

  /*!
   * \brief Offset of "oopDesc" class's "_metadata._compressed_klass" field.
   *        This field is stored class object when JVM is using COOP.
   */
  off_t ofsCoopKlassAtOop;

  /*!
   * \brief Offset of "oopDesc" class's "_mark" field.
   *        This field is stored bit flags of lock information.
   */
  off_t ofsMarkAtOop;

  /*!
   * \brief Offset of "Klass" class's "_name" field.
   *        This field is stored symbol object that designed class name.
   */
  off_t ofsNameAtKlass;

  /*!
   * \brief Offset of "Symbol" class's "_length" field.
   *        This field is stored string size of class name.
   */
  off_t ofsLengthAtSymbol;

  /*!
   * \brief Offset of "Symbol" class's "_body" field.
   *        This field is stored string of class name.
   */
  off_t ofsBodyAtSymbol;

  /*!
   * \brief Offset of "instanceKlass" class's "_vtable_len" field.
   *        This field is stored vtable size.
   */
  off_t ofsVTableSizeAtInsKlass;

  /*!
   * \brief Offset of "instanceKlass" class's "_itable_len" field.
   *        This field is stored itable size.
   */
  off_t ofsITableSizeAtInsKlass;

  /*!
   * \brief Offset of "instanceKlass" class's "_static_field_size" field.
   *        This field is stored static field size.
   */
  off_t ofsStaticFieldSizeAtInsKlass;

  /*!
   * \brief Offset of "instanceKlass" class's "_nonstatic_oop_map_size" field.
   *        This field is stored static field size.
   */
  off_t ofsNonstaticOopMapSizeAtInsKlass;

  /*!
   * \brief Offset of "oopDesc" class's "klass_offset_in_bytes" field.
   *        This field is stored static field size.
   */
  off_t ofsKlassOffsetInBytesAtOopDesc;

  /*!
   * \brief Pointer of COOP base address.
   */
  ptrdiff_t narrowOffsetBase;

  /*!
   * \brief Value of COOP shift bits.
   */
  int narrowOffsetShift;

  /*!
   * \brief Pointer of Klass COOP base address.
   */
  ptrdiff_t narrowKlassOffsetBase;

  /*!
   * \brief Value of Klass COOP shift bits.
   */
  int narrowKlassOffsetShift;

  /*!
   * \brief Lock bit mask.<br>
   *        Value of "markOopDesc" class's "lock_mask_in_place" constant value.
   */
  uint64_t lockMaskInPlaceMarkOop;

  /*!
   * \brief Pointer of CMS marking bitmap start word.
   */
  void *cmsBitMap_startWord;

  /*!
   * \brief Value of CMS bitmap shifter.
   */
  int cmsBitMap_shifter;

  /*!
   * \brief Pointer to CMS Marking bitmap start memory address.
   */
  size_t *cmsBitMap_startAddr;

  /*!
   * \brief Size of integer(regular integer register) on now environment.<br>
   *        BitsPerWord is used by CMS GC mark bitmap.
   */
  int32_t HeapWordSize;

  /*!
   * \brief Number of bit to need expressing integer size of "HeapWordSize".
   *        HeapWordSize = 2 ^ LogHeapWordSize.<br>
   *        BitsPerWord is used by CMS GC mark bitmap.
   */
  int32_t LogHeapWordSize;

  /*!
   * \brief It's integer's number to need for expressing 64bit integer
   *        on now environment.<br>
   *        BitsPerWord is used by CMS GC mark bitmap.
   */
  int HeapWordsPerLong;

  /*!
   * \brief Number of bit to need expressing a integer register.<br>
   *        LogBitsPerWord = 2 ^ LogBitsPerWord.<br>
   *        BitsPerWord is used by CMS GC mark bitmap.
   */
  int LogBitsPerWord;

  /*!
   * \brief Number of bit to expressable max integer by single register
   *        on now environment.<br>
   *        LogBitsPerWord = sizeof(long) * 8.<br>
   *        BitsPerWord is used by CMS GC mark bitmap.
   */
  int BitsPerWord;

  /*!
   * \brief Mask bit data of BitsPerWord.<br>
   *        Used by CMSGC("isMarkedObject") process.
   */
  int BitsPerWordMask;

  /*!
   * \brief JVM safepoint state pointer.
   */
  int *safePointState;

  /*!
   * \brief G1 start address.
   */
  void *g1StartAddr;

  /* Class of HeapStats for scanning variables in HotSpot VM */
  TSymbolFinder *symFinder;
  TVMStructScanner *vmScanner;

  /*!
   * \brief Singleton instance of TVMVariables.
   */
  static TVMVariables *inst;

  /*!
   * \brief Get HotSpot values through VMStructs which is related to CMS.
   * \return Result of this function.
   */
  bool getCMSValuesFromVMStructs(void);

  /*!
   * \brief Get HotSpot values through symbol table which is related to CMS.
   * \return Result of this function.
   */
  bool getCMSValuesFromSymbol(void);

  /*!
   * \brief Get HotSpot values through VMStructs which is related to G1.
   * \return Result of this function.
   */
  bool getG1ValuesFromVMStructs(void);

 protected:
  /*!
   * \brief Get unrecognized options (-XX)
   * \return Result of this function.
   */
  bool getUnrecognizedOptions(void);

  /*!
   * \brief Get HotSpot values through VMStructs.
   * \return Result of this function.
   */
  bool getValuesFromVMStructs(void);

  /*!
   * \brief Get HotSpot values through symbol table.
   * \return Result of this function.
   */
  bool getValuesFromSymbol(void);

 public:
  /*
   * Constructor of TVMVariables.
   * \param sym   [in] Symbol finder of libjvm.so .
   * \param scan  [in] VMStruct scanner.
   */
  TVMVariables(TSymbolFinder *sym, TVMStructScanner *scan);

  /*!
   * \brief Instance initializer.
   * \param sym   [in] Symbol finder of libjvm.so .
   * \param scan  [in] VMStruct scanner.
   * \return Singleton instance of TVMVariables.
   */
  static TVMVariables *initialize(TSymbolFinder *sym, TVMStructScanner *scan);

  /*!
   * \brief Get singleton instance of TVMVariables.
   * \return Singleton instance of TVMVariables.
   */
  static TVMVariables *getInstance() { return inst; };

  /*!
   * \brief Get values which are decided after VMInit JVMTI event.
   *        This function should be called after JVMTI VMInit event.
   * \return Process result.
   */
  bool getValuesAfterVMInit(void);

  /* Getters */
  inline bool getIsCOOP() { return isCOOP; };
  inline bool getUseParallel() { return useParallel; };
  inline bool getUseParOld() { return useParOld; };
  inline bool getUseCMS() { return useCMS; };
  inline bool getUseG1() { return useG1; };
  inline int getCMS_collectorState() { return *CMS_collectorState; };
  inline uint64_t getClsSizeOopDesc() { return clsSizeOopDesc; };
  inline uint64_t getClsSizeKlassOop() { return clsSizeKlassOop; };
  inline uint64_t getClsSizeNarrowOop() { return clsSizeNarrowOop; };
  inline uint64_t getClsSizeKlass() { return clsSizeKlass; };
  inline uint64_t getClsSizeInstanceKlass() { return clsSizeInstanceKlass; };
  inline uint64_t getClsSizeArrayOopDesc() { return clsSizeArrayOopDesc; };
  inline off_t getOfsKlassAtOop() { return ofsKlassAtOop; };
  inline off_t getOfsCoopKlassAtOop() { return ofsCoopKlassAtOop; };
  inline off_t getOfsMarkAtOop() { return ofsMarkAtOop; };
  inline off_t getOfsNameAtKlass() { return ofsNameAtKlass; };
  inline off_t getOfsLengthAtSymbol() { return ofsLengthAtSymbol; };
  inline off_t getOfsBodyAtSymbol() { return ofsBodyAtSymbol; };
  inline off_t getOfsVTableSizeAtInsKlass() { return ofsVTableSizeAtInsKlass; };
  inline off_t getOfsITableSizeAtInsKlass() { return ofsITableSizeAtInsKlass; };
  inline off_t getOfsStaticFieldSizeAtInsKlass() {
    return ofsStaticFieldSizeAtInsKlass;
  };
  inline off_t getOfsNonstaticOopMapSizeAtInsKlass() {
    return ofsNonstaticOopMapSizeAtInsKlass;
  };
  inline off_t getOfsKlassOffsetInBytesAtOopDesc() {
    return ofsKlassOffsetInBytesAtOopDesc;
  };
  inline ptrdiff_t getNarrowOffsetBase() { return narrowOffsetBase; };
  inline int getNarrowOffsetShift() { return narrowOffsetShift; };
  inline ptrdiff_t getNarrowKlassOffsetBase() { return narrowKlassOffsetBase; };
  inline int getNarrowKlassOffsetShift() { return narrowKlassOffsetShift; };
  inline uint64_t getLockMaskInPlaceMarkOop() {
    return lockMaskInPlaceMarkOop;
  };
  inline void *getCmsBitMap_startWord() { return cmsBitMap_startWord; };
  inline int getCmsBitMap_shifter() { return cmsBitMap_shifter; };
  inline size_t *getCmsBitMap_startAddr() { return cmsBitMap_startAddr; };
  inline int32_t getHeapWordSize() { return HeapWordSize; };
  inline int32_t getLogHeapWordSize() { return LogHeapWordSize; };
  inline int getHeapWordsPerLong() { return HeapWordsPerLong; };
  inline int getLogBitsPerWord() { return LogBitsPerWord; };
  inline int getBitsPerWord() { return BitsPerWord; };
  inline int getBitsPerWordMask() { return BitsPerWordMask; };
  inline int getSafePointState() { return *safePointState; };
  inline void *getG1StartAddr() { return g1StartAddr; };
};

#endif  // VMVARIABLES_H

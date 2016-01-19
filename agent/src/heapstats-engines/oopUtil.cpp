/*!
 * \file oopUtil.cpp
 * \brief This file is used to getting information inner JVM.<br>
 * Copyright (C) 2011-2015 Nippon Telegraph and Telephone Corporation
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
#include "vmFunctions.hpp"
#include "oopUtil.hpp"

/* Variable for class object. */

TSymbolFinder *symFinder = NULL;
TVMStructScanner *vmScanner = NULL;

/* Macros. */

/*!
 * \brief Calculate pointer align macro.
 */
#define ALIGN_POINTER_OFFSET(size) \
  (ALIGN_SIZE_UP((size), TVMVariables::getInstance()->getHeapWordsPerLong()))

/* Function. */

/* Function for utilities. */

/*!
 * \brief Convert COOP(narrowKlass) to wide Klass(normally Klass).
 * \param narrowKlass [in] Java Klass object(compressed Klass pointer).
 * \return Wide Klass object.
 */
inline void *getWideKlass(unsigned int narrowKlass) {
  TVMVariables *vmVal = TVMVariables::getInstance();

  /*
   * narrowKlass decoding is defined in
   * inline Klass* Klass::decode_klass_not_null(narrowKlass v)
   * hotspot/src/share/vm/oops/klass.inline.hpp
   */

  return (
      void *)(vmVal->getNarrowKlassOffsetBase() +
              ((ptrdiff_t)narrowKlass << vmVal->getNarrowKlassOffsetShift()));
}

/*!
 * \brief Getting oop's class information(It's "Klass", not "KlassOop").
 * \param klassOop [in] Java heap object(Inner "KlassOop" class).
 * \return Class information object(Inner "Klass" class).
 */
void *getKlassFromKlassOop(void *klassOop) {
  if (unlikely(klassOop == NULL || jvmInfo->isAfterCR6964458())) {
    return klassOop;
  }
  return incAddress(klassOop,
                    TVMVariables::getInstance()->getClsSizeKlassOop());
}

/*!
 * \brief Getting oop's class information(It's "Klass", not "KlassOop").
 * \param oop [in] Java heap object(Inner class format).
 * \return Class information object.
 * \sa oopDesc::klass()<br>
 *     at hotspot/src/share/vm/oops/oop.inline.hpp in JDK.
 */
void *getKlassOopFromOop(void *oop) {
  TVMVariables *vmVal = TVMVariables::getInstance();

  /* Sanity check. */
  if (unlikely(oop == NULL)) {
    return NULL;
  }

  void *tempAddr = NULL;

  /* If JVM use "C"ompressed "OOP". */
  if (vmVal->getIsCOOP()) {
    /* Get oop's klassOop from "_compressed_klass" field. */
    unsigned int *testAddr =
        (unsigned int *)((ptrdiff_t)oop + vmVal->getOfsCoopKlassAtOop());

    if (likely(testAddr != NULL)) {
      /* Decode COOP. */
      tempAddr = getWideKlass(*testAddr);
    }

  } else {
    /* Get oop's klassOop from "_klass" field. */
    void **testAddr = (void **)incAddress(oop, vmVal->getOfsKlassAtOop());

    if (likely(testAddr != NULL)) {
      tempAddr = (*testAddr);
    }
  }

  return tempAddr;
}

/*!
 * \brief Getting class's name form java inner class.
 * \param klass [in] Java class object(Inner class format).
 * \return String of object class name.<br>
 *         Don't forget deallocate if value isn't null.
 */
char *getClassName(void *klass) {
  TVMVariables *vmVal = TVMVariables::getInstance();

  /* Sanity check. */
  if (unlikely(klass == NULL)) {
    return NULL;
  }

  /* Get class symbol information. */
  void **klassSymbol = (void **)incAddress(klass, vmVal->getOfsNameAtKlass());
  if (unlikely(klassSymbol == NULL || (*klassSymbol) == NULL)) {
    return NULL;
  }

  /* Get class name pointer. */
  char *name = (char *)incAddress((*klassSymbol), vmVal->getOfsBodyAtSymbol());
  if (unlikely(name == NULL)) {
    return NULL;
  }

  bool isInstanceClass = (*name != '[');

  /* Get class name size. */
  unsigned short len = *(unsigned short *)incAddress(
                           (*klassSymbol), vmVal->getOfsLengthAtSymbol());

  char *str = NULL;
  /* If class is instance class. */
  if (isInstanceClass) {
    /* Add length of "L" and ";". */
    str = (char *)malloc(len + 3);
  } else {
    /* Get string memory. */
    str = (char *)malloc(len + 1);
  }

  if (likely(str != NULL)) {
    /* Get class name. */
    if (isInstanceClass) {
      /* Copy class name if class is instance class. */
      /* As like "instanceKlass::signature_name()".  */
      str[0] = 'L';

      // memcpy(&str[1], name, len);
      __builtin_memcpy(&str[1], name, len);

      str[len + 1] = ';';
      str[len + 2] = '\0';
    } else {
      /* Copy class name if class is other. */
      // memcpy(str, name, len);
      __builtin_memcpy(str, name, len);

      str[len] = '\0';
    }
  }

  return str;
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
 * \brief Get first field object block for "InstanceKlass".
 * \param klassOop [in] Class information object(Inner "Klass" class).
 * \return Oop's field information inner class object("OopMapBlock" class).
 * \sa hotspot/src/share/vm/oops/instanceKlass.hpp
 *     InstanceKlass::start_of_static_fields()
 */
inline void *getBeginBlock(void *klassOop) {
  TVMVariables *vmVal = TVMVariables::getInstance();

  /*
   * Memory map of "klass(InstanceKlass)" class.
   * - Klass object head pointer -
   * - OopDesc                   -
   * - InstanceKlass             -
   * - Java's v-table            -
   * - Java's i-table            -
   * - Static field oops(Notice) -
   * - Non-Static field oops     -
   * Notice - By CR7017732, this area is removed.
   */

  void *klass = getKlassFromKlassOop(klassOop);
  if (unlikely(klass == NULL)) {
    return NULL;
  }

  /* Move to v-table head. */
  off_t ofsStartVTable =
      jvmInfo->isAfterCR6964458()
          ? ALIGN_POINTER_OFFSET(vmVal->getClsSizeInstanceKlass() /
                                 vmVal->getHeapWordSize())
          : ALIGN_POINTER_OFFSET(
                (vmVal->getClsSizeOopDesc() / vmVal->getHeapWordSize()) +
                (vmVal->getClsSizeInstanceKlass() / vmVal->getHeapWordSize()));

  void *startVTablePtr = (intptr_t *)klassOop + ofsStartVTable;
  int vTableLen =
      *(int *)incAddress(klass, vmVal->getOfsVTableSizeAtInsKlass());

  /* Increment v-table size. */
  void *startITablePtr =
      (intptr_t *)startVTablePtr + ALIGN_POINTER_OFFSET(vTableLen);
  int iTableLen =
      *(int *)incAddress(klass, vmVal->getOfsITableSizeAtInsKlass());

  /* Increment i-table size. */
  void *startStaticFieldPtr =
      (intptr_t *)startITablePtr + ALIGN_POINTER_OFFSET(iTableLen);

  if (jvmInfo->isAfterCR7017732()) {
    return startStaticFieldPtr;
  }

  /* Increment static field size. */
  int staticFieldSize =
      *(int *)incAddress(klass, vmVal->getOfsStaticFieldSizeAtInsKlass());
  return (intptr_t *)startStaticFieldPtr + staticFieldSize;
}

/*!
 * \brief Get number of field object blocks for "InstanceKlass".
 * \param instanceKlass [in] Class information object about "InstanceKlass".
 *                           (Inner "Klass" class)
 * \return Oop's field information inner class object("OopMapBlock" class).
 * \sa hotspot/src/share/vm/oops/instanceKlass.hpp
 *     InstanceKlass::nonstatic_oop_map_count()
 */
inline int getBlockCount(void *instanceKlass) {
  TVMVariables *vmVal = TVMVariables::getInstance();

  /* Get total field size. */
  int nonstaticFieldSize =
      *(int *)incAddress(instanceKlass,
                         vmVal->getOfsNonstaticOopMapSizeAtInsKlass());
  /* Get single field size. */
  int sizeInWord =
      ALIGN_SIZE_UP(sizeof(TOopMapBlock), vmVal->getHeapWordSize()) >>
      vmVal->getLogHeapWordSize();
  /* Calculate number of fields. */
  return nonstaticFieldSize / sizeInWord;
}

/*!
 * \brief Get class loader that loaded expected class as KlassOop.
 * \param klassName [in] String of target class name (JNI class format).
 * \return Java heap object which is class loader load expected the class.
 */
TOopType getClassType(const char *klassName) {
  TOopType result = otIllegal;

  if (likely(klassName != NULL)) {
    if (*(unsigned short *)klassName == HEADER_KLASS_ARRAY_ARRAY) {
      result = otArrayArray;
    } else if (*(unsigned short *)klassName == HEADER_KLASS_OBJ_ARRAY) {
      result = otObjArarry;
    } else if (*klassName == HEADER_KLASS_ARRAY) {
      result = otArray;
    } else {
      /* Expected class name is instance class. */
      result = otInstance;
    }
  }
  return result;
}

/*!
 * \brief Get class loader that loaded expected class as KlassOop.
 * \param klassOop [in] Class information object(Inner "Klass" class).
 * \param type     [in] KlassOop type.
 * \return Java heap object which is class loader load expected the class.
 */
void *getClassLoader(void *klassOop, const TOopType type) {
  TVMFunctions *vmFunc = TVMFunctions::getInstance();

  void *classLoader = NULL;
  void *klass = getKlassFromKlassOop(klassOop);
  /* Sanity check. */
  if (unlikely(klassOop == NULL || klass == NULL)) {
    return NULL;
  }

  switch (type) {
    case otObjArarry:
      classLoader = vmFunc->GetClassLoaderForObjArrayKlass(klass);
      break;
    case otInstance:
      classLoader = vmFunc->GetClassLoaderForInstanceKlass(klass);
      break;
    case otArray:
    case otArrayArray:
    /*
     * This type oop has no class loader.
     * Because such oop is loaded by system bootstrap class loader.
     * Or the class is loaded at system initialize (e.g. "Universe").
     */
    default:
      ;
  }

  return classLoader;
}

/* Function for callback. */

/*!
 * \brief Follow oop field block.
 * \param event     [in] Callback function.
 * \param fieldOops [in] Pointer of field block head.
 * \param count     [in] Count of field block.
 * \param data      [in] User expected data.
 */
inline void followFieldBlock(THeapObjectCallback event, void **fieldOops,
                             const unsigned int count, void *data) {
  TVMVariables *vmVal = TVMVariables::getInstance();

  /* Follow oop at each block. */
  for (unsigned int idx = 0; idx < count; idx++) {
    void *childOop = NULL;

    /* Sanity check. */
    if (unlikely(fieldOops == NULL)) {
      continue;
    }

    /* Get child oop and move next array item. */
    if (vmVal->getIsCOOP()) {
      /* If this field is not null. */
      if (likely((*(unsigned int *)fieldOops) != 0)) {
        childOop = getWideOop(*(unsigned int *)fieldOops);
      }

      fieldOops = (void **)((unsigned int *)fieldOops + 1);
    } else {
      childOop = *fieldOops++;
    }

    /* Check oops isn't in permanent generation. */
    if (childOop != NULL && !is_in_permanent(collectedHeap, childOop)) {
      /* Invoke callback. */
      event(childOop, data);
    }
  }
}

/*!
 * \brief Generate oop field offset cache.
 * \param klassOop [in]  Target class object(klassOop format).
 * \param oopType  [in]  Type of inner "Klass" type.
 * \param offsets  [out] Field offset array.<br />
 *                       Please don't forget deallocate this value.
 * \param count    [out] Count of field offset array.
 */
void generateIterateFieldOffsets(void *klassOop, TOopType oopType,
                                 TOopMapBlock **offsets, int *count) {
  TVMVariables *vmVal = TVMVariables::getInstance();

  /* Sanity check. */
  if (unlikely(klassOop == NULL || offsets == NULL || count == NULL)) {
    return;
  }

  void *klass = getKlassFromKlassOop(klassOop);
  if (unlikely(klass == NULL)) {
    return;
  }

  TOopMapBlock *block = NULL;
  /* Get field blocks. */
  switch (oopType) {
    case otInstance: {
      int blockCount = getBlockCount(klass);
      TOopMapBlock *mapBlock = (TOopMapBlock *)getBeginBlock(klassOop);

      /* Allocate block offset records. */
      block = (TOopMapBlock *)malloc(sizeof(TOopMapBlock) * blockCount);
      if (likely(mapBlock != NULL && block != NULL && blockCount > 0)) {
        /* Follow each block. */
        for (int i = 0; i < blockCount; i++) {
          /* Store block information. */
          block[i].offset = mapBlock->offset;
          block[i].count = mapBlock->count;

          /* Go next block. */
          mapBlock++;
        }

        (*offsets) = block;
        (*count) = blockCount;
      } else {
        /* If class has no field. */
        if (blockCount == 0) {
          (*offsets) = NULL;
          (*count) = 0;
        }

        /* Deallocate memory. */
        free(block);
      }

      break;
    }

    case otObjArarry:
      /* Allocate a block offset record. */
      block = (TOopMapBlock *)malloc(sizeof(TOopMapBlock));
      if (likely(block != NULL)) {
        /*
         * Get array's length field and array field offset.
         * But such fields isn't define by C++. So be cafeful.
         * Please see below source file about detail implementation.
         * hostpost/src/share/vm/oops/arrayOop.hpp
         */
        block->count =
            vmVal->getIsCOOP()
                ? (vmVal->getOfsKlassAtOop() + vmVal->getClsSizeNarrowOop())
                : vmVal->getClsSizeArrayOopDesc();
        block->offset =
            ALIGN_SIZE_UP(block->count + sizeof(int), vmVal->getHeapWordSize());

        (*offsets) = block;
        (*count) = 1;
      }

      break;

    default:
      /* The oop has no field.  */
      ;
  }
}

/*!
 * \brief Iterate oop's field oops.
 * \param event       [in]     Callback function.
 * \param oop         [in]     Itearate target object(OopDesc format).
 * \param oopType     [in]     Type of oop's class.
 * \param ofsData     [in,out] Cache data for iterate oop fields.<br />
 *                             If value is null, then create and set cache data.
 * \param ofsDataSize [in,out] Cache data count.<br />
 *                             If value is null, then set count of "ofsData".
 * \param data        [in,out] User expected data for callback.
 */
void iterateFieldObject(THeapObjectCallback event, void *oop, TOopType oopType,
                        TOopMapBlock **ofsData, int *ofsDataSize, void *data) {
  /* Sanity check. */
  if (unlikely(oop == NULL || event == NULL || ofsData == NULL ||
               ofsDataSize == NULL || (*ofsData) == NULL ||
               (*ofsDataSize) <= 0)) {
    return;
  }

  /* If oop has no field. */
  if (unlikely(!hasOopField(oopType))) {
    return;
  }

  /* Use expected data by function calling arguments. */
  TOopMapBlock *offsets = (*ofsData);
  int offsetCount = (*ofsDataSize);

  if (oopType == otInstance) {
    /* Iterate each oop field block as "Instance klass". */
    for (TOopMapBlock *endOfs = offsets + offsetCount; offsets < endOfs;
         offsets++) {
      followFieldBlock(event, (void **)incAddress(oop, offsets->offset),
                       offsets->count, data);
    }
  } else {
    /* Iterate each oop field block as "ObjArray klass". */
    followFieldBlock(event, (void **)incAddress(oop, offsets->offset),
                     *(int *)incAddress(oop, offsets->count), data);
  }
}

/*!
 * \brief Initialization of this util.
 * \param jvmti [in] JVMTI environment object.
 * \return Process result.
 */
bool oopUtilInitialize(jvmtiEnv *jvmti) {
  /* Search symbol in libjvm. */
  char *libPath = NULL;
  jvmti->GetSystemProperty("sun.boot.library.path", &libPath);
  try {
    /* Create symbol finder instance. */
    symFinder = new TSymbolFinder();

    if (unlikely(!symFinder->loadLibrary(libPath, "libjvm.so"))) {
      throw 1;
    }

    /* Cretae VMStruct scanner instance. */
    vmScanner = new TVMStructScanner(symFinder);
  } catch (...) {
    logger->printCritMsg("Cannot initialize symbol finder (libjvm).");
    jvmti->Deallocate((unsigned char *)libPath);
    delete symFinder;
    symFinder = NULL;
    return false;
  }

  jvmti->Deallocate((unsigned char *)libPath);

  bool result = true;
  try {
    /* Initialize TVMVariables */
    TVMVariables *vmVal = TVMVariables::initialize(symFinder, vmScanner);
    if (vmVal == NULL) {
      logger->printCritMsg("Cannot get TVMVariables instance.");
      throw 3;
    }

    /* Initialize TVMFunctions */
    TVMFunctions *vmFunc = TVMFunctions::initialize(symFinder);
    if (vmFunc == NULL) {
      logger->printCritMsg("Cannot get TVMFunctions instance.");
      throw 4;
    }

    /* Initialize overrider */
    if (!initOverrider()) {
      logger->printCritMsg("Cannot initialize HotSpot overrider.");
      throw 5;
    }

  } catch (...) {
    /* Deallocate memory. */
    delete symFinder;
    symFinder = NULL;

    TVMVariables *vmVal = TVMVariables::getInstance();
    if (vmVal != NULL) delete vmVal;

    TVMFunctions *vmFunc = TVMFunctions::getInstance();
    if (vmFunc != NULL) delete vmFunc;

    result = false;
  }

  return result;
}

/*!
 * \brief Finailization of this util.
 */
void oopUtilFinalize(void) {
  delete symFinder;
  symFinder = NULL;
  delete vmScanner;
  vmScanner = NULL;
}

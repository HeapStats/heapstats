/*!
 * \file oopUtil.hpp
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

#ifndef _OOP_UTIL_H
#define _OOP_UTIL_H

#include "overrider.hpp"
#include "vmVariables.hpp"

/*!
 * \brief This structure is expressing java heap object type.
 */
typedef enum {
  otIllegal = 0,    /*!< The Object is unknown class type.    */
  otInstance = 1,   /*!< The Object is object instance class. */
  otArray = 2,      /*!< The Object is single array class.    */
  otObjArarry = 3,  /*!< The Object is object array class.    */
  otArrayArray = 4, /*!< The Object is multi array class.     */
} TOopType;

/*!
 * \brief This structure is inner "OopMapBlock" class stub.<br/>
 *        This struct is used by get following class information.
 * \sa hotspot/src/share/vm/oops/instanceKlass.hpp
 */
typedef struct {
  int offset;         /*!< Byte offset of the first oop mapped. */
  unsigned int count; /*!< Count of oops mapped in this block.  */
} TOopMapBlock;

/* Function for init/final. */

/*!
 * \brief Initialization of this util.
 * \param jvmti [in] JVMTI environment object.
 * \return Process result.
 */
bool oopUtilInitialize(jvmtiEnv *jvmti);

/*!
 * \brief Finailization of this util.
 */
void oopUtilFinalize(void);

/* Function for oop util. */

/*!
 * \brief Getting oop's class information(It's "Klass", not "KlassOop").
 * \param klassOop [in] Java heap object(Inner "KlassOop" class).
 * \return Class information object(Inner "Klass" class).
 */
void *getKlassFromKlassOop(void *klassOop);

/*!
 * \brief Getting oop's class information(It's "Klass", not "KlassOop").
 * \param oop [in] Java heap object(Inner class format).
 * \return Class information object.
 * \sa oopDesc::klass()<br>
 *     at hotspot/src/share/vm/oops/oop.inline.hpp in JDK.
 */
void *getKlassOopFromOop(void *oop);

/*!
 * \brief Getting class's name form java inner class.
 * \param klass [in] Java class object(Inner class format).
 * \return String of object class name.<br>
 *         Don't forget deallocate if value isn't null.
 */
char *getClassName(void *klass);

/*!
 * \brief Get class loader that loaded expected class as KlassOop.
 * \param klassName [in] String of target class name (JNI class format).
 * \return Java heap object which is class loader load expected the class.
 */
TOopType getClassType(const char *klassName);

/*!
 * \brief Get class loader that loaded expected class as KlassOop.
 * \param klassOop [in] Class information object(Inner "Klass" class).
 * \param type     [in] KlassOop type.
 * \return Java heap object which is class loader load expected the class.
 */
void *getClassLoader(void *klassOop, const TOopType type);

/*!
 * \brief Generate oop field offset cache.
 * \param klassOop [in]  Target class object(klassOop format).
 * \param oopType  [in]  Type of inner "Klass" type.
 * \param offsets  [out] Field offset array.<br />
 *                       Please don't forget deallocate this value.
 * \param count    [out] Count of field offset array.
 */
void generateIterateFieldOffsets(void *klassOop, TOopType oopType,
                                 TOopMapBlock **offsets, int *count);

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
                        TOopMapBlock **ofsData, int *ofsDataSize, void *data);

/* Function for other. */

/*!
 * \brief Convert COOP(narrowOop) to wide Oop(normally Oop).
 * \param narrowOop [in] Java heap object(compressed format Oop).
 * \return Wide OopDesc object.
 */
inline void *getWideOop(unsigned int narrowOop) {
  TVMVariables *vmVal = TVMVariables::getInstance();

  /*
   * narrow oop decoding is defined in
   * inline oop oopDesc::decode_heap_oop_not_null(narrowOop v)
   * hotspot/src/share/vm/oops/oop.inline.hpp
   */

  return (void *)(vmVal->getNarrowOffsetBase() +
                  ((ptrdiff_t)narrowOop << vmVal->getNarrowOffsetShift()));
}

/*!
 * \brief Get oop forward address.
 * \param oop [in] Java heap object.
 * \return Wide OopDesc object.
 */
inline void *getForwardAddr(void *oop) {
  TVMVariables *vmVal = TVMVariables::getInstance();

  /* Sanity check. */
  if (unlikely(oop == NULL)) {
    return NULL;
  }

  /* Get OopDesc::_mark field. */
  ptrdiff_t markOop = *(ptrdiff_t *)incAddress(oop, vmVal->getOfsMarkAtOop());
  return (void *)(markOop & ~vmVal->getLockMaskInPlaceMarkOop());
}

/*!
 * \brief Get oop field exists.
 * \return Does oop have oop field.
 */
inline bool hasOopField(const TOopType oopType) {
  return (oopType == otInstance || oopType == otObjArarry);
}

#endif  // _OOP_UTIL_H

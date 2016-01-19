/*!
 * \file vmStructScanner.hpp
 * \brief This file is used to getting JVM structure information.<br>
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

#ifndef _VMSCANNER_HPP
#define _VMSCANNER_HPP

#include <stdint.h>
#include <sys/types.h>

#include "symbolFinder.hpp"

/* Type define. */

/* Structure for VMStruct. */
/* from hotspot/src/share/vm/runtime/vmStructs.hpp */

/*!
 * \brief Hotspot JVM information structure.
 */
typedef struct {
  const char *typeName;
  /*!< The type name containing the given field. (example: "Klass") */
  const char *fieldName;
  /*!< The field name within the type. (example: "_name") */
  const char *typeString;
  /*!< Quoted name of the type of this field. (example: "symbolOopDesc*")<br>
       Parsed in Java to ensure type correctness. */
  int32_t isStatic;
  /*!< Indicates whether following field is an offset or an address. */
  uint64_t offset;
  /*!< Offset of field within structure; only used for nonstatic fields. */
  void *address;
  /*!< Address of field; only used for static fields. <br>
       ("offset" can not be reused because of apparent SparcWorks compiler bug
       in generation of initializer data) */
} VMStructEntry;

/*!
 * \brief Hotspot's inner class information structure.
 */
typedef struct {
  const char *typeName;
  /*!< Type name. (example: "methodOopDesc") */
  const char *superclassName;
  /*!< Superclass name, or null if none. (example: "oopDesc") */
  int32_t isOopType;
  /*!< Does this type represent an oop typedef. <br>
       (i.e., "methodOop" or "klassOop", but NOT "methodOopDesc") */
  int32_t isIntegerType;
  /*!< Does this type represent an integer type. (of arbitrary size) */
  int32_t isUnsigned;
  /*!< If so, is it unsigned. */
  uint64_t size;
  /*!< Size, in bytes, of the type. */
} VMTypeEntry;

/*!
 * \brief Hotspot constant integer information structure.
 */
typedef struct {
  const char *name;
  /*!< Name of constant. (example: "_thread_in_native") */
  int32_t value;
  /*!< Value of constant. */
} VMIntConstantEntry;

/*!
 * \brief Hotspot constant long integer information structure.
 */
typedef struct {
  const char *name;
  /*!< Name of constant. (example: "_thread_in_native") */
  uint64_t value;
  /*!< Value of constant. */
} VMLongConstantEntry;

/* Structure for TVMStructScanner. */

/*!
 * \brief Field offset/pointer information structure.
 */
typedef struct {
  const char *className;
  /*!< String of target class name. */
  const char *fieldName;
  /*!< String of target class's field name. */
  off_t *ofs;
  /*!< Offset of target field.(If target field is dynamic) */
  void **addr;
  /*!< Pointer of target field.(If target field is static) */
} TOffsetNameMap;

/*!
 * \brief Type size structure.
 */
typedef struct {
  const char *typeName;
  /*!< String of target class name. */
  uint64_t *size;
  /*!< Size of target class type. */
} TTypeSizeMap;

/*!
 * \brief Constant integer structure.
 */
typedef struct {
  const char *name;
  /*!< String of target integer constant name. */
  int32_t *value;
  /*!< Value of interger constant. */
} TIntConstMap;

/*!
 * \brief Constant long integer structure.
 */
typedef struct {
  const char *name;
  /*!< String of target long integer constant name. */
  uint64_t *value;
  /*!< Value of long interger constant. */
} TLongConstMap;

/* Class define. */

/*!
 * \brief This class is used to search JVM inner information.<br>
 *        E.g. VMStruct, inner-class, etc...
 */
class TVMStructScanner {
 public:
  /*!
   * \brief TVMStructScanner constructor.
   * \param finder [in] Symbol search object.
   */
  TVMStructScanner(TSymbolFinder *finder);

  /*!
   * \brief Get offset data from JVM inner struct.
   * \param ofsMap [out] Map of search offset target.
   */
  void GetDataFromVMStructs(TOffsetNameMap *ofsMap);
  /*!
   * \brief Get type size data from JVM inner struct.
   * \param typeMap [out] Map of search constant target.
   */
  void GetDataFromVMTypes(TTypeSizeMap *typeMap);
  /*!
   * \brief Get int constant data from JVM inner struct.
   * \param constMap [out] Map of search constant target.
   */
  void GetDataFromVMIntConstants(TIntConstMap *constMap);
  /*!
   * \brief Get long constant data from JVM inner struct.
   * \param constMap [out] Map of search constant target.
   */
  void GetDataFromVMLongConstants(TLongConstMap *constMap);

 protected:
  /*!
   * \brief Pointer of Hotspot JVM information.
   */
  static VMStructEntry **vmStructEntries;

  /*!
   * \brief Pointer of hotspot's inner class information.
   */
  static VMTypeEntry **vmTypeEntries;

  /*!
   * \brief Pointer of hotspot constant integer information.
   */
  static VMIntConstantEntry **vmIntConstEntries;

  /*!
   * \brief Pointer of hotspot constant long integer information.
   */
  static VMLongConstantEntry **vmLongConstEntries;
};

#endif  // _VMSCANNER_HPP

/*!
 * \file vmStructScanner.cpp
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

#include <dlfcn.h>

#include "vmStructScanner.hpp"

/* Macros. */

/*!
 * \brief Symbol of VMStruct entry macro.
 */
#define SYMBOL_VMSTRUCTS "gHotSpotVMStructs"

/*!
 * \brief Symbol of VMTypes entry macro.
 */
#define SYMBOL_VMTYPES "gHotSpotVMTypes"

/*!
 * \brief Symbol of VMIntConstants entry macro.
 */
#define SYMBOL_VMINTCONSTANTS "gHotSpotVMIntConstants"

/*!
 * \brief Symbol of VMLongConstants entry macro.
 */
#define SYMBOL_VMLONGCONSTANTS "gHotSpotVMLongConstants"

/* Hotspot JVM information. */

/*!
 * \brief Pointer of Hotspot JVM information.
 */
VMStructEntry **TVMStructScanner::vmStructEntries = NULL;

/*!
 * \brief Pointer of hotspot's inner class information.
 */
VMTypeEntry **TVMStructScanner::vmTypeEntries = NULL;

/*!
 * \brief Pointer of hotspot constant integer information.
 */
VMIntConstantEntry **TVMStructScanner::vmIntConstEntries = NULL;

/*!
 * \brief Pointer of hotspot constant long integer information.
 */
VMLongConstantEntry **TVMStructScanner::vmLongConstEntries = NULL;

/* Class function. */

/*!
 * \brief TVMStructScanner constructor.
 * \param finder [in] Symbol search object.
 */
TVMStructScanner::TVMStructScanner(TSymbolFinder *finder) {
  if (unlikely(vmStructEntries == NULL)) {
    /* Get VMStructs. */
    vmStructEntries = (VMStructEntry **)dlsym(RTLD_DEFAULT, SYMBOL_VMSTRUCTS);
    if (unlikely(vmStructEntries == NULL)) {
      vmStructEntries = (VMStructEntry **)finder->findSymbol(SYMBOL_VMSTRUCTS);
    }

    if (unlikely(vmStructEntries == NULL)) {
      throw "Not found java symbol. symbol:" SYMBOL_VMSTRUCTS;
    }
  }

  if (unlikely(vmTypeEntries == NULL)) {
    /* Get VMTypes. */
    vmTypeEntries = (VMTypeEntry **)dlsym(RTLD_DEFAULT, SYMBOL_VMTYPES);
    if (unlikely(vmTypeEntries == NULL)) {
      vmTypeEntries = (VMTypeEntry **)finder->findSymbol(SYMBOL_VMTYPES);
    }

    if (unlikely(vmTypeEntries == NULL)) {
      throw "Not found java symbol. symbol:" SYMBOL_VMTYPES;
    }
  }

  if (unlikely(vmIntConstEntries == NULL)) {
    /* Get VMIntConstants. */
    vmIntConstEntries =
        (VMIntConstantEntry **)dlsym(RTLD_DEFAULT, SYMBOL_VMINTCONSTANTS);
    if (unlikely(vmIntConstEntries == NULL)) {
      vmIntConstEntries =
          (VMIntConstantEntry **)finder->findSymbol(SYMBOL_VMINTCONSTANTS);
    }

    if (unlikely(vmIntConstEntries == NULL)) {
      throw "Not found java symbol. symbol:" SYMBOL_VMINTCONSTANTS;
    }
  }

  if (unlikely(vmLongConstEntries == NULL)) {
    /* Get VMLongConstants. */
    vmLongConstEntries =
        (VMLongConstantEntry **)dlsym(RTLD_DEFAULT, SYMBOL_VMLONGCONSTANTS);
    if (unlikely(vmLongConstEntries == NULL)) {
      vmLongConstEntries =
          (VMLongConstantEntry **)finder->findSymbol(SYMBOL_VMLONGCONSTANTS);
    }

    if (unlikely(vmLongConstEntries == NULL)) {
      throw "Not found java symbol. symbol:" SYMBOL_VMLONGCONSTANTS;
    }
  }
}

/*!
 * \brief Get offset data from JVM inner struct.
 * \param ofsMap [out] Map of search offset target.
 */
void TVMStructScanner::GetDataFromVMStructs(TOffsetNameMap *ofsMap) {
  /* Sanity check. */
  if (unlikely((ofsMap == NULL) || (this->vmStructEntries == NULL))) {
    return;
  }

  /* Check map entry. */
  for (TOffsetNameMap *ofs = ofsMap; ofs->className != NULL; ofs++) {
    /* Search JVM inner struct entry. */
    for (VMStructEntry *entry = *this->vmStructEntries; entry->typeName != NULL;
         entry++) {
      if ((strcmp(entry->typeName, ofs->className) == 0) &&
          (strcmp(entry->fieldName, ofs->fieldName) == 0)) {
        /* If entry is expressing static field. */
        if (entry->isStatic) {
          *ofs->addr = entry->address;
        } else {
          *ofs->ofs = entry->offset;
        }
        break;
      }
    }
  }
}

/*!
 * \brief Get type size data from JVM inner struct.
 * \param typeMap [out] Map of search constant target.
 */
void TVMStructScanner::GetDataFromVMTypes(TTypeSizeMap *typeMap) {
  /* Sanity check. */
  if (unlikely((typeMap == NULL) || (this->vmTypeEntries == NULL))) {
    return;
  }

  /* Search JVM inner struct entry. */
  for (TTypeSizeMap *types = typeMap; types->typeName != NULL; types++) {
    /* Check map entry. */
    for (VMTypeEntry *entry = *this->vmTypeEntries; entry->typeName != NULL;
         entry++) {
      if (strcmp(entry->typeName, types->typeName) == 0) {
        *types->size = entry->size;
        break;
      }
    }
  }
}

/*!
 * \brief Get int constant data from JVM inner struct.
 * \param constMap [out] Map of search constant target.
 */
void TVMStructScanner::GetDataFromVMIntConstants(TIntConstMap *constMap) {
  /* Sanity check. */
  if (unlikely((constMap == NULL) || (this->vmIntConstEntries == NULL))) {
    return;
  }

  /* Search JVM inner struct entry. */
  for (TIntConstMap *consts = constMap; consts->name != NULL; consts++) {
    /* Check map entry. */
    for (VMIntConstantEntry *entry = *this->vmIntConstEntries;
         entry->name != NULL; entry++) {
      if (strcmp(entry->name, consts->name) == 0) {
        *consts->value = entry->value;
        break;
      }
    }
  }
}

/*!
 * \brief Get long constant data from JVM inner struct.
 * \param constMap [out] Map of search constant target.
 */
void TVMStructScanner::GetDataFromVMLongConstants(TLongConstMap *constMap) {
  /* Sanity check. */
  if (unlikely((constMap == NULL) || (this->vmLongConstEntries == NULL))) {
    return;
  }

  /* Search JVM inner struct entry. */
  for (TLongConstMap *consts = constMap; consts->name != NULL; consts++) {
    /* Check map entry. */
    for (VMLongConstantEntry *entry = *this->vmLongConstEntries;
         entry->name != NULL; entry++) {
      if (strcmp(entry->name, consts->name) == 0) {
        *consts->value = entry->value;
        break;
      }
    }
  }
}

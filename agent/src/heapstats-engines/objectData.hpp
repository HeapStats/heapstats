/*!
 * \file objectData.hpp
 * \brief This file is used to add up using size every class.
 * Copyright (C) 2020 Yasumasa Suenaga
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

#ifndef OBJECT_DATA_HPP
#define OBJECT_DATA_HPP

#include <jni.h>

#include "oopUtil.hpp"


/*!
 * \brief Pointer type of klassOop.
 */
typedef void* PKlassOop;

/*!
 * \brief This structure stored class information.
 */
class TObjectData {

  private:
    jlong _tag;          /*!< Class tag.                                 */
    jlong _classNameLen; /*!< Class name.                                */
    char *_className;    /*!< Class name length.                         */
    PKlassOop _klassOop; /*!< Java inner class object.                   */
    jlong _oldTotalSize; /*!< Class old total use size.                  */
    TOopType _oopType;   /*!< Type of class.                             */
    jlong _clsLoaderId;  /*!< Class loader instance id.                  */
    jlong _clsLoaderTag; /*!< Class loader class tag.                    */
    jlong _instanceSize; /*!< Class size if this class is instanceKlass. */

  public:
    TObjectData(PKlassOop koop);
    ~TObjectData();

    void setClassLoader(void *classLoaderOop, jlong classLoaderTag);
    int writeObjectData(int fd);

    inline void replaceKlassOop(PKlassOop newKlassOop) { _klassOop = newKlassOop; };

    inline jlong Tag() { return _tag; }
    inline jlong ClassNameLen() { return _classNameLen; }
    inline char *ClassName() { return _className; }
    inline PKlassOop KlassOop() { return _klassOop; }
    inline jlong OldTotalSize() { return _oldTotalSize; }
    inline TOopType OopType() { return _oopType; }
    inline jlong ClassLoaderId() { return _clsLoaderId; }
    inline jlong ClassLoaderTag() { return _clsLoaderTag; }
    inline void SetInstanceSize(jlong size) { _instanceSize = size; }
    inline void SetOldTotalSize(jlong size) { _oldTotalSize = size; }
    inline jlong InstanceSize() { return _instanceSize; }

    inline bool is_instance() { return _oopType == otInstance; }
};

#endif //OBJECT_DATA_HPP

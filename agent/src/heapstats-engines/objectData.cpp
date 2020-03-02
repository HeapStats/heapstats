/*!
 * \file objectData.cpp
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

#include <jni.h>
#include <stdlib.h>
#include <unistd.h>

#include "objectData.hpp"
#include "oopUtil.hpp"


TObjectDataA::TObjectDataA(PKlassOop koop) : _klassOop(koop),
                                           _oldTotalSize(0L),
                                           _clsLoaderId(-1L),
                                           _clsLoaderTag(-1L),
                                           _instanceSize(0L) {
  _tag = reinterpret_cast<jlong>(this);

  _className = getClassName(getKlassFromKlassOop(_klassOop));
  if (unlikely(_className == NULL)) {
    // Class name should be gotten from KlassOop
    throw 1;
  }
  _classNameLen = strlen(_className);

  _oopType = getClassType(_className);
}

TObjectDataA::~TObjectDataA() {
  free(_className);
}

void TObjectDataA::setClassLoader(void *classLoaderOop, jlong classLoaderTag) {
  _clsLoaderId = reinterpret_cast<jlong>(classLoaderOop);
  _clsLoaderTag = classLoaderTag;
}

int TObjectDataA::writeObjectData(int fd) {
  int ret;

  /* Write TObjectData.tag & TObjectData.classNameLen. */
  if ((ret = write(fd, &_tag, sizeof(jlong) << 1)) != -1) {
    /* Write class name. */
    if ((ret = write(fd, _className, _classNameLen)) != -1) {
      /* Write class loader's instance id and class tag. */
      ret = write(fd, &_clsLoaderId, sizeof(jlong) << 1);
    }
  }

  return ret;
}

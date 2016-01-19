/*!
 * \file armBitMapMarker.hpp
 * \brief This file is used to store and control of bit map.
 * Copyright (C) 2015 Yasumasa Suenaga
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

#ifndef ARMBITMAPMARKER_HPP
#define ARMBITMAPMARKER_HPP

#include <stddef.h>

#include "../../bitMapMarker.hpp"

/*!
 * \brief This class is stored bit express flag in pointer range.
 */
class TARMBitMapMarker : public TBitMapMarker {
 public:
  /*!
   * \brief TBitMapMarker constructor.
   * \param startAddr [in] Start address of Java heap.
   * \param size      [in] Max Java heap size.
   */
  TARMBitMapMarker(void *startAddr, size_t size)
      : TBitMapMarker(startAddr, size){};

  /*!
   * \brief TBitMapMarker destructor.
   */
  virtual ~TARMBitMapMarker(){};

  /*!
   * \brief Mark GC-marked address in this bitmap.
   * \param addr [in] Oop address.
   */
  virtual void setMark(void *addr);

  /*!
   * \brief Check address which is already marked and set mark.
   * \param addr [in] Oop address.
   * \return Designated pointer is marked.
   */
  virtual bool checkAndMark(void *addr);
};

#endif  // ARMBITMAPMARKER_HPP

/*!
 * \file x86BitMapMarker.hpp
 * \brief This file is used to store and control of bit map.
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

#ifndef X86BITMAPMARKER_HPP
#define X86BITMAPMARKER_HPP

#include "../../bitMapMarker.hpp"

/*!
 * \brief This class is stored bit express flag in pointer range.
 */
class TX86BitMapMarker : public TBitMapMarker {
 public:
  /*!
   * \brief TBitMapMarker constructor.
   * \param startAddr [in] Start address of Java heap.
   * \param size      [in] Max Java heap size.
   */
  TX86BitMapMarker(void *startAddr, size_t size)
      : TBitMapMarker(startAddr, size){};

  /*!
   * \brief TBitMapMarker destructor.
   */
  virtual ~TX86BitMapMarker(){};

  /*!
   * \brief Mark GC-marked address in this bitmap.
   * \param addr [in] Oop address.
   */
  virtual void setMark(void *addr);

  /*!
   * \brief Get marked flag of designated pointer.
   * \param addr [in] Targer pointer.
   * \return Designated pointer is marked.
   */
  virtual bool isMarked(void *addr);

  /*!
   * \brief Check address which is already marked and set mark.
   * \param addr [in] Oop address.
   * \return Designated pointer is marked.
   */
  virtual bool checkAndMark(void *addr);
};

#endif  // X86BITMAPMARKER_HPP

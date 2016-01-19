/*!
 * \file neonBitMapMarker.hpp
 * \brief Storeing and Controlling G1 marking bitmap.
 *        This source is optimized for NEON instruction set.
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

#ifndef _NEONBITMAPMARKER_HPP
#define _NEONBITMAPMARKER_HPP

#include "arch/arm/armBitMapMarker.hpp"

/*!
 * \brief This class is stored bit express flag in pointer range NEON version.
 */
class TNeonBitMapMarker : public TARMBitMapMarker {
 public:
  /*!
   * \brief TNeonBitMapMarker constructor.
   * \param startAddr [in] Start address of Java heap.
   * \param size      [in] Max Java heap size.
   */
  TNeonBitMapMarker(void *startAddr, size_t size)
      : TARMBitMapMarker(startAddr, size){};

  /*!
   * \brief TNeonBitMapMarker destructor.
   */
  virtual ~TNeonBitMapMarker(){};

  /*!
   * \brief Clear bitmap flag.
   */
  virtual void clear(void);
};

#endif  // _NEONBITMAPMARKER_HPP

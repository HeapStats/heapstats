/*!
 * \file sse2BitMapMarker.hpp
 * \brief Storeing and Controlling G1 marking bitmap.
 *        This source is optimized for SSE2 instruction set.
 * Copyright (C) 2014-2016 Yasumasa Suenaga
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

#ifndef _SSE2BITMAPMARKER_HPP
#define _SSE2BITMAPMARKER_HPP

#include "arch/x86/x86BitMapMarker.hpp"

/*!
 * \brief This class is stored bit express flag in pointer range SSE2 version.
 */
class TSSE2BitMapMarker : public TX86BitMapMarker {
 public:
  /*!
   * \brief TSSE2BitMapMarker constructor.
   * \param startAddr [in] Start address of Java heap.
   * \param size      [in] Max Java heap size.
   */
  TSSE2BitMapMarker(const void *startAddr, const size_t size)
      : TX86BitMapMarker(startAddr, size){};

  /*!
   * \brief TSSE2BitMapMarker destructor.
   */
  virtual ~TSSE2BitMapMarker(){};

  /*!
   * \brief Clear bitmap flag.
   */
  virtual void clear(void);
};

#endif  // _SSE2BITMAPMARKER_HPP

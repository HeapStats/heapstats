/*!
 * \file neonBitMapMarker.cpp
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

#include <stddef.h>
#include <sys/mman.h>

#include "neonBitMapMarker.hpp"

/*!
 * \brief Clear bitmap flag with NEON instruction set.
 */
void TNeonBitMapMarker::clear(void) {
  /*
   * Linux memory page size = 4KB.
   * So top address of _bitmap is 16byte aligned.
   * _size is ditto.
   *
   * memset() in glibc sets 128bytes per loop.
   */

  ptrdiff_t end_addr = (ptrdiff_t) this->bitmapAddr + this->bitmapSize;

  /* Temporary advise to zero-clear with sequential access. */
  madvise(this->bitmapAddr, this->bitmapSize, MADV_SEQUENTIAL);

  asm volatile(
    "vbic.I64 %%q0, %%q0, %%q0;"
    "1:"
    "  vst1.8 {%%q0}, [%0]!;"
    "  vst1.8 {%%q0}, [%0]!;"
    "  vst1.8 {%%q0}, [%0]!;"
    "  vst1.8 {%%q0}, [%0]!;"
    "  vst1.8 {%%q0}, [%0]!;"
    "  vst1.8 {%%q0}, [%0]!;"
    "  vst1.8 {%%q0}, [%0]!;"
    "  vst1.8 {%%q0}, [%0]!;"
    "  cmp %0, %1;"
    "  bne 1b;"
    :
    : "r"(this->bitmapAddr), "r"(end_addr)
    : "cc"
  );

  /* Reset advise. */
  madvise(this->bitmapAddr, this->bitmapSize, MADV_RANDOM);
}

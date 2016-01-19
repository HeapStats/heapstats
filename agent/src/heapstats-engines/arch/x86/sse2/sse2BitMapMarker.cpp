/*!
 * \file sse2BitMapMarker.cpp
 * \brief Storeing and Controlling G1 marking bitmap.
 *        This source is optimized for SSE2 instruction set.
 * Copyright (C) 2014 Yasumasa Suenaga
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

#include <sys/mman.h>

#include "sse2BitMapMarker.hpp"

/*!
 * \brief Clear bitmap flag.
 */
void TSSE2BitMapMarker::clear(void) {
  /*
   * Linux memory page size = 4KB.
   * So top address of _bitmap is 16byte aligned.
   * _size is ditto.
   *
   * memset() in glibc sets 128bytes per loop.
   *
   *******************************************************
   * Intel 64 and IA-32 Architectures Optimization Reference Manual
   *   Assembler/Compiler Coding Rule 12:
   *     All branch targets should be 16-byte aligned.
   */

  /* Temporary advise to zero-clear with sequential access. */
  madvise(this->bitmapAddr, this->bitmapSize, MADV_SEQUENTIAL);

  asm volatile(
    "pxor %%xmm0, %%xmm0;"
    ".align 16;"
    ".LSSE2_LOOP:" /* memset 128bytes per LOOP. */
    "movntdq %%xmm0, -0x80(%1, %0, 1);"
    "movntdq %%xmm0, -0x70(%1, %0, 1);"
    "movntdq %%xmm0, -0x60(%1, %0, 1);"
    "movntdq %%xmm0, -0x50(%1, %0, 1);"
    "movntdq %%xmm0, -0x40(%1, %0, 1);"
    "movntdq %%xmm0, -0x30(%1, %0, 1);"
    "movntdq %%xmm0, -0x20(%1, %0, 1);"
    "movntdq %%xmm0, -0x10(%1, %0, 1);"
#ifdef __amd64__
    "subq $0x80, %0;"
#else  // __i386__
    "subl $0x80, %0;"
#endif
    "jnz .LSSE2_LOOP;"
    :
    : "r"(this->bitmapSize), "r"(this->bitmapAddr)
    : "cc", "%xmm0"
  );

  /* Reset advise. */
  madvise(this->bitmapAddr, this->bitmapSize, MADV_RANDOM);
}

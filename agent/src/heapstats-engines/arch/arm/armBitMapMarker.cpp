/*!
 * \file armBitMapMarker.cpp
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

#include "util.hpp"
#include "armBitMapMarker.hpp"

/*!
 * \brief Mark GC-marked address in this bitmap.
 * \param addr [in] Oop address.
 */
void TARMBitMapMarker::setMark(void *addr) {
  /* Sanity check. */
  if (unlikely(!this->isInZone(addr))) {
    return;
  }

  /*
   * Why I use "ptrdiff_t" ?
   *  ptrdiff_t is integer representation of pointer. ptrdiff_t size equals
   *  pointer size. Pointer size equals CPU register size.
   *  Thus, we use CPU register more effective.
   */
  ptrdiff_t *bitmapBlock;
  ptrdiff_t bitmapMask;
  this->getBlockAndMask(addr, &bitmapBlock, &bitmapMask);

  asm volatile(
    "1:"
    "  ldrex %%r0, [%0];"
    "  orr   %%r0, %1;"
    "  strex %%r1, %%r0, [%0];"
    "  tst   %%r1, %%r1;"
    "  bne   1b;"
    :
    : "r"(bitmapBlock), "r"(bitmapMask)
    : "cc", "memory", "%r0", "%r1"
  );
}

/*!
 * \brief Check address which is already marked and set mark.
 * \param addr [in] Oop address.
 * \return Designated pointer is marked.
 */
bool TARMBitMapMarker::checkAndMark(void *addr) {
  /* Sanity check. */
  if (unlikely(!this->isInZone(addr))) {
    return false;
  }

  /* Get block and mask for getting mark flag. */
  ptrdiff_t *bitmapBlock;
  ptrdiff_t bitmapMask;
  this->getBlockAndMask(addr, &bitmapBlock, &bitmapMask);

  bool result = false;
  asm volatile(
    "mov %%r1, #1;"
    "1:"
    "  ldrex   %%r0, [%1];"
    "  tst     %%r0, %2;"
    "  movne   %0,   #1;"
    "  orrne   %%r0, %2;"
    "  strexne %%r1, %%r0, [%1];"
    "  tst     %%r1, %%r1;"
    "  bne     1b;"
    : "+r"(result)
    : "r"(bitmapBlock), "r"(bitmapMask)
    : "cc", "memory", "%r0", "%r1"
  );

  return result;
}

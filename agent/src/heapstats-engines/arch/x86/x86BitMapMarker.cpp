/*!
 * \file x86BitMapMarker.cpp
 * \brief This file is used to store and control of bit map.
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

#include "util.hpp"
#include "x86BitMapMarker.hpp"

/*!
 * \brief Mark GC-marked address in this bitmap.
 * \param addr [in] Oop address.
 */
void TX86BitMapMarker::setMark(void *addr) {
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

  /* Atomic set mark bit flag. */
  asm volatile(
#ifdef __LP64__
    "lock orq %1, %0;"
#else  // __ILP32__
    "lock orl %1, %0;"
#endif
    : "+m"(*bitmapBlock)
    : "r"(bitmapMask)
    : "cc", "memory"
  );
}

/*!
 * \brief Get marked flag of designated pointer.
 * \param addr [in] Targer pointer.
 * \return Designated pointer is marked.
 */
bool TX86BitMapMarker::isMarked(void *addr) {
  /* Sanity check. */
  if (unlikely(!this->isInZone(addr))) {
    return false;
  }

  /* Get block and mask for getting mark flag. */
  ptrdiff_t *bitmapBlock;
  ptrdiff_t bitmapMask;
  this->getBlockAndMask(addr, &bitmapBlock, &bitmapMask);

  /* Atomic get mark bit flag. */
  register bool result asm("al");

  /*
   * Intel 64 and IA-32 Architectures Software Developer's Manual.
   * Volume 3A: System Programming Guide, Part1
   *  8.2.3.8 Locked Instructions Have a Total Order:
   *    The memory-ordering model ensures that all processors agree
   *    on a single execution order of all locked instructions,
   *    including those that are larger than 8 bytes
   *    or are not naturally aligned.
   */
  asm volatile(
    "test %1, %2;"
    "setneb %0;"
    : "=r"(result)
    : "m"(*bitmapBlock), "r"(bitmapMask)
    : "cc", "memory"
  );

  return result;
}

/*!
 * \brief Check address which is already marked and set mark.
 * \param addr [in] Oop address.
 * \return Designated pointer is marked.
 */
bool TX86BitMapMarker::checkAndMark(void *addr) {
  /* Sanity check. */
  if (unlikely(!this->isInZone(addr))) {
    return false;
  }

  /* Get block and mask for getting mark flag. */
  ptrdiff_t *bitmapBlock;
  ptrdiff_t bitmapMask;
  this->getBlockAndMask(addr, &bitmapBlock, &bitmapMask);

  /* Get and set mark. */
  register bool result asm("al");
  asm volatile(
#ifdef __amd64__
    "bsfq %1, %%rbx;"
    "lock btsq %%rbx, %2;"
#else  // __i386__
    "bsfl %1, %%edx;"
    "lock btsl %%edx, %2;"
#endif
    "setbb %0;"
    : "=r"(result)
    : "r"(bitmapMask), "m"(*bitmapBlock)
    :
#ifdef __amd64__
    "%rbx",
#else  // __i386__
    "%edx",
#endif
    "cc", "memory"
  );

  return result;
}

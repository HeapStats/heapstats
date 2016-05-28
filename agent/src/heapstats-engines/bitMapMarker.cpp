/*!
 * \file bitMapMarker.cpp
 * \brief This file is used to store and control of bit map.
 * Copyright (C) 2011-2016 Nippon Telegraph and Telephone Corporation
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

#include "globals.hpp"
#include "util.hpp"
#include "bitMapMarker.hpp"

/*!
 * \brief TBitMapMarker constructor.
 * \param startAddr [in] Start address of Java heap.
 * \param size      [in] Max Java heap size.
 */
TBitMapMarker::TBitMapMarker(const void *startAddr, const size_t size) {
  /* Sanity check. */
  if (unlikely(startAddr == NULL || size <= 0)) {
    throw - 1;
  }

  /* Initialize setting. */
  size_t alignedSize = ALIGN_SIZE_UP(size, systemPageSize);

  this->beginAddr = const_cast<void *>(startAddr);
  this->endAddr = incAddress(this->beginAddr, alignedSize);
  this->bitmapSize = ALIGN_SIZE_UP(alignedSize >> MEMALIGN_BIT, systemPageSize);
  this->bitmapAddr = mmap(NULL, this->bitmapSize, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  /* If failure allocate bitmap memory. */
  if (unlikely(this->bitmapAddr == MAP_FAILED)) {
    throw errno;
  }

  /* Advise the kernel that this memory will be random access. */
  madvise(this->bitmapAddr, this->bitmapSize, POSIX_MADV_RANDOM);

  /* Initialize bitmap (clear bitmap). */
  this->clear();
}

/*!
 * \brief TBitMapMarker destructor.
 */
TBitMapMarker::~TBitMapMarker() {
  /* Release memory map. */
  munmap(this->bitmapAddr, this->bitmapSize);
}

/*!
 * \brief Get marked flag of designated pointer.
 * \param addr [in] Targer pointer.
 * \return Designated pointer is marked.
 */
bool TBitMapMarker::isMarked(const void *addr) {
  /* Sanity check. */
  if (unlikely(!this->isInZone(addr))) {
    return false;
  }

  /* Get block and mask for getting mark flag. */
  ptrdiff_t *bitmapBlock;
  ptrdiff_t bitmapMask;
  this->getBlockAndMask(addr, &bitmapBlock, &bitmapMask);

  return (*bitmapBlock & bitmapMask);
}

/*!
 * \brief Clear bitmap flag.
 */
void TBitMapMarker::clear(void) {
  /* Temporary advise to zero-clear with sequential access. */
  madvise(this->bitmapAddr, this->bitmapSize, MADV_SEQUENTIAL);
  memset(this->bitmapAddr, 0, this->bitmapSize);
  madvise(this->bitmapAddr, this->bitmapSize, MADV_RANDOM);
}

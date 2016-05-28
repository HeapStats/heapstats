/*!
 * \file bitMapMarker.hpp
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

#ifndef _BITMAPMARKER_HPP
#define _BITMAPMARKER_HPP

#include <stddef.h>

#ifdef __LP64__

/*!
 * \brief Memory align bit count.
 *        Pointer is aligned 8byte,
 *        If allocate pointer as like "void*" in 64bit.
 *        So, 3bits under allocated pointer is always 0.
 */
#define MEMALIGN_BIT 3

/*!
 * \brief Bit size of pointer.
 *        E.g. "sizeof(void*) << 3".
 */
#define PTR_BIT_WIDTH 64

/*!
 * \brief Log. 2 of allocated pointer size.
 *        E.g. "log 2 sizeof(void*)".
 */
#define LOG2_PTR_WIDTH 6

#else  // __ILP32__

/*!
 * \brief Memory align bit count.
 *        Pointer is aligned 4byte,
 *        If allocate pointer as like "void*" in 32bit.
 *        So, 2bits under allocated pointer is always 0.
 */
#define MEMALIGN_BIT 2

/*!
 * \brief Bit size of pointer.
 *        E.g. "sizeof(void*) << 3".
 */
#define PTR_BIT_WIDTH 32

/*!
 * \brief Log. 2 of allocated pointer size.
 *        E.g. "log 2 sizeof(void*)".
 */
#define LOG2_PTR_WIDTH 5

#endif

/*!
 * \brief This class is stored bit express flag in pointer range.
 */
class TBitMapMarker {
 public:
  /*!
   * \brief TBitMapMarker constructor.
   * \param startAddr [in] Start address of Java heap.
   * \param size      [in] Max Java heap size.
   */
  TBitMapMarker(const void *startAddr, const size_t size);
  /*!
   * \brief TBitMapMarker destructor.
   */
  virtual ~TBitMapMarker();

  /*!
   * \brief Check address which is covered by this bitmap.
   * \param addr [in] Oop address.
   * \return Designated pointer is existing in bitmap.
   */
  inline bool isInZone(const void *addr) {
    return (this->beginAddr <= addr) && (this->endAddr >= addr);
  }

  /*!
   * \brief Mark GC-marked address in this bitmap.
   * \param addr [in] Oop address.
   */
  virtual void setMark(const void *addr) = 0;

  /*!
   * \brief Get marked flag of designated pointer.
   * \param addr [in] Targer pointer.
   * \return Designated pointer is marked.
   */
  virtual bool isMarked(const void *addr);

  /*!
   * \brief Check address which is already marked and set mark.
   * \param addr [in] Oop address.
   * \return Designated pointer is marked.
   */
  virtual bool checkAndMark(const void *addr) = 0;

  /*!
   * \brief Clear bitmap flag.
   */
  virtual void clear(void);

 protected:
  /*!
   * \brief Pointer of memory stored bitmap flag.
   */
  void *bitmapAddr;

  /*!
   * \brief Real allocated bitmap range size for mmap.
   */
  size_t bitmapSize;

  /*!
   * \brief Get a byte data and mask expressing target pointer.
   * \param addr  [in]  Oop address.
   * \param block [out] A byte data including bit flag.
   *                    The bit flag is expressing target pointer.
   * \param mask  [out] A bitmask for getting only flag of target pointer.
   */
  inline void getBlockAndMask(const void *addr,
                              ptrdiff_t **block, ptrdiff_t *mask) {
    /*
     * Calculate bitmap offset.
     * offset = (address offset from top of JavaHeap) / (pointer size)
     */
    ptrdiff_t bitmapOfs =
        ((ptrdiff_t)addr - (ptrdiff_t) this->beginAddr) >> MEMALIGN_BIT;

    /*
     * Calculate bitmap block.
     * block = (top of bitmap) + ((bitmap offset) / (pointer size(bit)))
     */
    *block = (ptrdiff_t *)this->bitmapAddr + (bitmapOfs >> LOG2_PTR_WIDTH);

    /*
     * Calculate bitmask.
     * mask = 1 << ((bitmap offset) % (pointer size(bit)))
     */
    *mask = (ptrdiff_t)1 << (bitmapOfs % PTR_BIT_WIDTH);
  }

 private:
  /*!
   * \brief Begin of Java heap.
   */
  void *beginAddr;
  /*!
   * \brief End of Java heap.
   */
  void *endAddr;
};

#endif  // _BITMAPMARKER_HPP

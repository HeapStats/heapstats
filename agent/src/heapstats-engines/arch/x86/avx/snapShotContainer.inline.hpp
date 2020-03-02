/*!
 * \file snapShotContainer.inline.hpp
 * \brief This file is used to add up using size every class.
 *        This source is optimized for AVX instruction set.
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

#ifndef AVX_SNAPSHOTCONTAINER_INLINE_H
#define AVX_SNAPSHOTCONTAINER_INLINE_H

/*!
 * \brief Increment instance count and using size.
 * \param counter [in] Increment target class.
 * \param operand [in] Right-hand operand (SRC operand).
 *                     This value must be aligned 16bytes.
 */
inline void TSnapShotContainer::addInc(TObjectCounter *counter,
                                       TObjectCounter *operand) {
  asm volatile(
    "vmovntdqa (%1),   %%xmm0;"
    "vpaddq    (%0),   %%xmm0, %%xmm0;"
    "vmovdqa   %%xmm0, (%0);"
    :
    : "r"(counter), "r"(operand)
    : "%xmm0"
  );
}

/*!
 * \brief Zero clear to TObjectCounter.
 * \param counter TObjectCounter to clear.
 */
inline void TSnapShotContainer::clearObjectCounter(TObjectCounter *counter) {
  asm volatile(
    "vpxor   %%xmm0, %%xmm0, %%xmm0;"
    "vmovdqa %%xmm0, (%0);"
    :
    : "r"(counter)
    : "%xmm0"
  );
}

/*!
 * \brief Zero clear to TClassCounter and its children counter.
 * \param counter TClassCounter to clear.
 */
inline void TSnapShotContainer::clearChildClassCounters(
    TClassCounter *counter) {
#ifdef __amd64__
  asm volatile(
    "vpxor %%xmm0, %%xmm0, %%xmm0;"
    "movq   8(%0), %%rax;" /* clsCounter->child */

    ".align 16;"
    "1:"
    "  testq       %%rax,   %%rax;"
    "  jz 2f;"
    "  movq      (%%rax),   %%rbx;" /* child->counter */
    "  vmovdqa    %%xmm0, (%%rbx);"
    "  movq     8(%%rax),   %%rax;" /* child->next */
    "  jmp 1b;"
    "2:"
    "  movq      (%0),  %%rbx;" /* clsCounter->counter */
    "  vmovdqa %%xmm0, (%%rbx);"
    :
    : "r"(counter)
    : "%xmm0", "%rax", "%rbx", "cc"
  );
#else  // i386
  asm volatile(
    "vpxor %%xmm0, %%xmm0, %%xmm0;"
    "movl   4(%0), %%eax;" /* clsCounter->child */

    ".align 16;"
    "1:"
    "  testl      %%eax,   %%eax;"
    "  jz 2f;"
    "  movl     (%%eax),   %%ecx;" /* child->counter */
    "  vmovdqa   %%xmm0, (%%ecx);"
    "  movl    4(%%eax),   %%eax;" /* child->next */
    "  jmp 1b;"
    "2:"
    "  movl      (%0),  %%ecx;" /* clsCounter->counter */
    "  vmovdqa %%xmm0, (%%ecx);"
    :
    : "r"(counter)
    : "%xmm0", "%eax", "%ecx", "cc"
  );
#endif
}

#endif  // AVX_SNAPSHOTCONTAINER_INLINE_H

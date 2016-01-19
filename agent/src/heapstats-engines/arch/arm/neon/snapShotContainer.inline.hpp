/*!
 * \file snapShotContainer.inline.hpp
 * \brief This file is used to add up using size every class.
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

#ifndef NEON_SNAPSHOTCONTAINER_INLINE_H
#define NEON_SNAPSHOTCONTAINER_INLINE_H

/*!
 * \brief Increment instance count and using size.
 * \param counter [in] Increment target class.
 * \param operand [in] Right-hand operand (SRC operand).
 */
inline void TSnapShotContainer::addInc(TObjectCounter *counter,
                                       TObjectCounter *operand) {
  asm volatile(
    "vld1.8   {%%q0}, [%1];"
    "vld1.8   {%%q1}, [%0];"
    "vadd.I64 %%q2,   %%q0, %%q1;"
    "vst1.8   {%%q2}, [%0];"
    :
    : "r"(counter), "r"(operand)
    : "%q0", "%q1", "%q2"
  );
}

/*!
 * \brief Zero clear to TObjectCounter.
 * \param counter TObjectCounter to clear.
 */
inline void TSnapShotContainer::clearObjectCounter(TObjectCounter *counter) {
  asm volatile(
    "vbic.I64 %%q0,   %%q0, %%q0;"
    "vst1.8   {%%q0}, [%0];"
    :
    : "r"(counter)
    : "%q0"
  );
}

/*!
 * \brief Zero clear to TClassCounter and its children counter.
 * \param counter TClassCounter to clear.
 */
inline void TSnapShotContainer::clearChildClassCounters(
    TClassCounter *counter) {
  asm volatile(
    "vbic.I64 %%q0, %%q0, %%q0;"
    "ldr      %%r0, [%0, #4];" /* clsCounter->child */
    "1:"
    "  tst    %%r0, %%r0;"
    "  beq    2f;"
    "  ldr    %%r1,   [%%r0];" /* child->counter */
    "  vst1.8 {%%q0}, [%%r1];"
    "  ldr    %%r0,   [%%r0, #8];" /* child->next */
    "  b      1b;"
    "2:"
    "  ldr    %%r0,   [%0];" /* clsCounter->counter */
    "  vst1.8 {%%q0}, [%%r0];"
    :
    : "r"(counter)
    : "cc", "%q0", "%r0", "%r1"
  );
}

#endif  // NEON_SNAPSHOTCONTAINER_INLINE_H

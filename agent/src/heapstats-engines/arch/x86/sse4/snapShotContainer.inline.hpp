/*!
 * \file snapshotContainer.inline.hpp
 * \brief This file is used to add up using size every class.
 *        This source is optimized for SSE4 instruction set.
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

#ifndef SSE4_SNAPSHOT_CONTAINER_INLINE_HPP
#define SSE4_SNAPSHOT_CONTAINER_INLINE_HPP

/*!
 * \brief Increment instance count and using size.
 * \param counter [in] Increment target class.
 * \param operand [in] Right-hand operand (SRC operand).
 *                     This value must be aligned 16bytes.
 */
inline void TSnapShotContainer::addInc(TObjectCounter *counter,
                                       TObjectCounter *operand) {
  asm volatile(
    "movntdqa (%1), %%xmm0;"
    "paddq (%0), %%xmm0;"
    "movdqa %%xmm0, (%0);"
    :
    : "r"(counter), "r"(operand)
    : "%xmm0"
  );
}

/* Include other optimized inline functions from SSE2. */
#include "arch/x86/sse2/snapShotContainer.inline.hpp"

#endif  // SSE4_SNAPSHOT_CONTAINER_INLINE_HPP

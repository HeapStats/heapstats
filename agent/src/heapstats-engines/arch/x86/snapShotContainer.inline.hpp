/*!
 * \file snapShotContainer.inline.hpp
 * \brief This file is used to add up using size every class.
 *        This source is optimized for x86 instruction set.
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

#ifndef X86_SNAPSHOTCONTAINER_INLINE_HPP
#define X86_SNAPSHOTCONTAINER_INLINE_HPP

/*!
 * \brief Increment instance count and using size atomically.
 * \param counter [in] Increment target class.
 * \param size    [in] Increment object size.
 */
inline void TSnapShotContainer::Inc(TObjectCounter *counter, jlong size) {
#ifdef __amd64__
  asm volatile(
    "lock addq $1, %0;"
    "lock addq %2, %1;"
    : "+m"(counter->count), "+m"(counter->total_size)
    : "r"(size)
    : "cc"
  );
#else  // __i386__
  asm volatile(
    "lock addl    $1,    %0;" /* Lower 32bit */
    "lock adcl    $0,  4+%0;" /* Upper 32bit */
    "     movl    %2, %%eax;"
    "lock addl %%eax,    %1;" /* Lower 32bit */
    "     movl  4+%2, %%eax;"
    "lock adcl %%eax,  4+%1;" /* Upper 32bit */
    : "+o"(counter->count), "+o"(counter->total_size)
    : "o"(size)
    : "%eax", "cc"
  );
#endif
}

#endif  // X86_SNAPSHOTCONTAINER_INLINE_HPP

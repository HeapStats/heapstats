/*!
 * \file snapShotContainer.inline.hpp
 * \brief This file is used to add up using size every class.
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

#ifndef SNAPSHOTCONTAINER_INLINE_HPP
#define SNAPSHOTCONTAINER_INLINE_HPP

/*!
 * \brief Increment instance count and using size.
 * \param counter [in] Increment target class.
 * \param size    [in] Increment object size.
 */
inline void TSnapShotContainer::Inc(TObjectCounter *counter, jlong size) {
#ifdef WORDS_BIGENDIAN
  asm volatile(
    "1:"
    "  ldrex %%r0, [%0, #4];"
    "  adds  %%r0, %%r0, #1;"
    "  strex %%r1, %%r0, [%r0, #4];"
    "  tst   %%r1, %%r1;"
    "  bne   1b;"
    "2:"
    "  ldrex %%r0, [%0];"
    "  adc   %%r0, %%r0, #0;"
    "  strex %%r1, %%r0, [%0];"
    "  tst   %%r1, %%r1;"
    "  bne   2b;"
    :
    : "r"(&counter->count)
    : "cc", "memory", "%r0", "%r1"
  );

  asm volatile(
    "ldr     %%r2, [%1, #4];"
    "1:"
    "  ldrex %%r0, [%0, #4];"
    "  adds  %%r0, %%r0, %%r2;"
    "  strex %%r1, %%r0, [%r0, #4];"
    "  tst   %%r1, %%r1;"
    "  bne   1b;"
    "ldr     %%r2, [%1];"
    "2:"
    "  ldrex %%r0, [%0];"
    "  adc   %%r0, %%r0, %%r2;"
    "  strex %%r1, %%r0, [%0];"
    "  tst   %%r1, %%r1;"
    "  bne   2b;"
    :
    : "r"(&counter->count), "r"(&counter->total_size)
    : "cc", "memory", "%r0", "%r1", "%r2"
  );
#else
  asm volatile(
    "1:"
    "  ldrex %%r0, [%0];"
    "  adds  %%r0, %%r0, #1;"
    "  strex %%r1, %%r0, [%r0];"
    "  tst   %%r1, %%r1;"
    "  bne   1b;"
    "2:"
    "  ldrex %%r0, [%0, #4];"
    "  adc   %%r0, %%r0, #0;"
    "  strex %%r1, %%r0, [%0, #4];"
    "  tst   %%r1, %%r1;"
    "  bne   2b;"
    :
    : "r"(&counter->count)
    : "cc", "memory", "%r0", "%r1"
  );

  asm volatile(
    "ldr     %%r2, [%1];"
    "1:"
    "  ldrex %%r0, [%0];"
    "  adds  %%r0, %%r0, %%r2;"
    "  strex %%r1, %%r0, [%r0];"
    "  tst   %%r1, %%r1;"
    "  bne   1b;"
    "ldr     %%r2, [%1, #4];"
    "2:"
    "  ldrex %%r0, [%0, #4];"
    "  adc   %%r0, %%r0, %%r2;"
    "  strex %%r1, %%r0, [%0, #4];"
    "  tst   %%r1, %%r1;"
    "  bne   2b;"
    :
    : "r"(&counter->count), "r"(&counter->total_size)
    : "cc", "memory", "%r0", "%r1", "%r2"
  );
#endif
}

#endif  // SNAPSHOTCONTAINER_INLINE_HPP

/*!
 * \file util.inline.hpp
 * \brief Optimized utility functions for ARM processor.
 * Copyright (C) 2017 Yasumasa Suenaga
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

#ifndef ARM_UTIL_H
#define ARM_UTIL_H

inline void atomic_inc(int *target, int value) {
  asm volatile("1:"
               "  ldrex %%r0, [%0];"
               "  add %%r0, %%r0, %1;"
               "  strex %%r1, %%r0, [%0];"
               "  tst %%r1, %%r1;"
               "  bne 1b;"
               : : "r"(target), "r"(value) : "memory", "cc", "%r0", "%r1");
}

inline int atomic_get(int *target) {
  register int ret;
  asm volatile("ldrex %0, [%1]" : "=r"(ret) : "r"(target) : );

  return ret;
}

#endif  // ARM_UTIL_H

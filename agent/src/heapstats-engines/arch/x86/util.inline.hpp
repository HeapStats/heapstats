/*!
 * \file util.inline.hpp
 * \brief Optimized utility functions for x86 processor.
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

#ifndef X86_UTIL_H
#define X86_UTIL_H

inline void atomic_inc(int *target, int value) {
  asm volatile("lock addl %1, (%0)" :
                                    : "r"(target), "r"(value)
                                    : "memory", "cc");
}

/*
 * We can use MOV operation at this point.
 * LOCK'ed store operation will be occurred before load (MOV) operation.
 *
 * Intel (R)  64 and IA-32 Architectures Software Developer’s Manual
 *   Volume 3A: System Programming Guide, Part 1
 *     8.2.3.8 Locked Instructions Have a Total Order
 */
inline int atomic_get(int *target) {
  register int ret;
  asm volatile("movl (%1), %0" : "=r"(ret) : "r"(target) : );

  return ret;
}

#endif  // X86_UTIL_H

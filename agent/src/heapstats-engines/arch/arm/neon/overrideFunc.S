/*!
 * \file overrideFunc.S
 * \brief This file is used to override JVM inner function.<br>
 *        The function defined this file, used with v-table hook.<br>
 *        So in this file, all function is written by only assembler and macro.
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

#define CHECK_VTABLE(oop_reg, valid_caller) \
  ldr %r0, [oop_reg];       \
  vdup.32 %q0, %r0;         \
  ldr %r0, =valid_caller;   \
  vld1.8 {%q1}, [%r0];       \
  vceq.I32 %q1, %q0, %q1;   \
  vmrs    APSR_nzcv, FPSCR;

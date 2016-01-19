/*!
 * \file overrideFunc.S
 * \brief This file is used to override JVM inner function.<br>
 *        The function defined this file, used with v-table hook.<br>
 *        So in this file, all function is written by only assembler and macro.
 *        This source is optimized for SSE4 instruction set.
 * Copyright (C) 2014-2015 Yasumasa Suenaga
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


#ifdef __amd64__

  #define CHECK_VTABLE(oop_reg, valid_caller) \
    movq    (oop_reg), %xmm0;                    \
    shufpd         $0, %xmm0, %xmm1;             \
    mov     valid_caller##@GOTPCREL(%rip), %r11; \
    pcmpeqq    (%r11), %xmm1;                    \
    ptest       %xmm1, %xmm1;

#elif defined __i386__

  #define CHECK_VTABLE(oop_reg, valid_caller) \
    movd    (oop_reg), %xmm0;               \
    pshufd         $0, %xmm0, %xmm1;        \
    mov     valid_caller##@GOT(%ebx), %ecx; \
    movq       (%ecx), %xmm1;               \
    pcmpeqd     %xmm0, %xmm1;               \
    ptest       %xmm1, %xmm1;

#endif


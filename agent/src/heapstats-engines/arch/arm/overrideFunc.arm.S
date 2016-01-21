/*!
 * \file overrideFunc.arm.S
 * \brief This file is used to override JVM inner function for ARM.<br>
 *        The function defined this file, used with v-table hook.<br>
 *        So in this file, all function is written by only assembler and macro.
 * Copyright (C) 2015-2016 Yasumasa Suenaga
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


#ifdef NEON
#include "arch/arm/neon/overrideFunc.S"
#else
#define CHECK_VTABLE(oop_reg, valid_caller) \
  mov %r1, oop_reg;       \
  eor %r2, %r2, %r2;      \
  ldr %r0, =valid_caller; \
  mov %r3, %r0;           \
                          \
  ldr %r0, [%r3];         \
  cmp %r0, %r1;           \
  moveq %r2, #1;          \
  ldr %r0, [%r3, #4];     \
  cmp %r0, %r1;           \
  moveq %r2, #1;          \
  tst %r2, %r2;
#endif


/* Macro for function parts define. */

#define HASH(x) x

#define DO_SAVE_REG \
  push {%r2}; /* 3rd argument. */ \
  push {%r1}; /* 2nd argument. */ \
  push {%r0}; /* 1st argument. */

#define DO_LOAD_REG \
  pop {%r0}; /* 1st argument. */ \
  pop {%r1}; /* 2nd argument. */ \
  pop {%r2}; /* 3rd argument. */

#define DO_CHECK_PERM(oop_ofs) \
  ldr %r0, =is_in_permanent;             \
  ldr %r12, [%r0];                       \
  ldr %r0, =collectedHeap;               \
  ldr %r0, [%r0];                        \
  /* oop_ofs + lr + original_func - 1 */ \
  mov %r1, HASH(#)oop_ofs;               \
  lsl %r1, %r1, #2;                      \
  add %r1, #4;                           \
  ldr %r1, [%sp, %r1];                   \
  blx %r12;                              \
  tst %r0, %r0;

#define DO_JMP_TO_CALLBACK(header, ary_idx, oop_ofs) \
  ldr %r0, =header##_enter_hook_##ary_idx; \
  ldr %r12, [%r0];                         \
  /* oop_ofs + lr + original_func - 1 */   \
  mov %r0, HASH(#)oop_ofs;                 \
  lsl %r0, %r0, #2;                        \
  add %r0, #4;                             \
  ldr %r0, [%sp, %r0];                     \
  blx %r12;

#define OVERRIDE_FUNC_DEFINE(header, ary_idx) \
.global header##_override_func_##ary_idx ;         \
.type header##_override_func_##ary_idx, %function; \
header##_override_func_##ary_idx: ;                \
  /* Save argument registers. */ ;                 \
  DO_SAVE_REG                                      \
                                                   \
  /* Save link register. */ ;                      \
  push {%lr};                                      \
                                                   \
  /* Save pointer of original function. */ ;       \
  ldr %r0, =header##_original_func_##ary_idx;      \
  ldr %r0, [%r0];                                  \
  push {%r0};


/* Macro for override function define. */

#define OVERRIDE_DEFINE(header, ary_idx, oop_arg_idx) \
  OVERRIDE_FUNC_DEFINE(header, ary_idx)            \
                                                   \
  /* Check permanent object. */ ;                  \
  DO_CHECK_PERM(oop_arg_idx)                       \
  bne .LORIGINAL_FUNC;                             \
                                                   \
  /* Call callback function. */ ;                  \
  DO_JMP_TO_CALLBACK(header, ary_idx, oop_arg_idx) \
  b .LORIGINAL_FUNC;                               \
  .ltorg;

#define OVERRIDE_DEFINE_WITHOUT_PERMCHECK(header, ary_idx, oop_arg_idx) \
  OVERRIDE_FUNC_DEFINE(header, ary_idx)            \
                                                   \
  /* Call callback function. */ ;                  \
  DO_JMP_TO_CALLBACK(header, ary_idx, oop_arg_idx) \
  b .LORIGINAL_FUNC;

#define OVERRIDE_DEFINE_WITH_CALLER_CHECK(header, ary_idx, oop_arg_idx, valid_caller) \
  OVERRIDE_FUNC_DEFINE(header, ary_idx)            \
                                                   \
  /* Get 3rd arg (OopClosure) */                   \
  mov %r12, %r2;                                   \
                                                   \
  /* Check vtable of caller */                     \
  CHECK_VTABLE(%r12, valid_caller)                 \
  beq .LORIGINAL_FUNC;                             \
                                                   \
  /* Check permanent object. */ ;                  \
  DO_CHECK_PERM(oop_arg_idx)                       \
  bne .LORIGINAL_FUNC;                             \
                                                   \
  /* Call callback function. */ ;                  \
  DO_JMP_TO_CALLBACK(header, ary_idx, oop_arg_idx) \
  b .LORIGINAL_FUNC;

#define OVERRIDE_ADJ_DEFINE(header, ary_idx, oop_arg_idx) \
  OVERRIDE_FUNC_DEFINE(header, ary_idx)            \
                                                   \
  /* Check permanent object. */ ;                  \
  DO_CHECK_PERM(oop_arg_idx)                       \
  beq .LORIGINAL_FUNC;                             \
                                                   \
  /* Call callback function. */ ;                  \
  DO_JMP_TO_CALLBACK(header, ary_idx, oop_arg_idx) \
  b .LORIGINAL_FUNC;


/* Begin of .text section. */
    .text


.LORIGINAL_FUNC:
  pop {%r12};
  pop {%lr};
  DO_LOAD_REG
  mov %pc, %r12;


/*!
 * \file overrideFunc.amd64.S
 * \brief This file is used to override JVM inner function for AMD64.<br>
 *        The function defined this file, used with v-table hook.<br>
 *        So in this file, all function is written by only assembler and macro.
 * Copyright (C) 2014-2017 Yasumasa Suenaga
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


#ifdef AVX
#include "arch/x86/avx/overrideFunc.S"
#elif defined SSE4
#include "arch/x86/sse4/overrideFunc.S"
#else
  #define CHECK_VTABLE(oop_reg, valid_caller) \
    mov (oop_reg), oop_reg;                  \
    xor      %rax, %rax;                     \
    mov valid_caller##@GOTPCREL(%rip), %r11; \
    cmp    (%r11), oop_reg;                  \
    cmove    %r11, %rax;                     \
    cmp   8(%r11), oop_reg;                  \
    cmove    %r11, %rax;                     \
    test     %rax, %rax;
#endif


/* Macro for function parts define. */

#define DO_SAVE_REG \
  sub $24, %rsp;                          \
  mov %rdx, 16(%rsp); /* 3rd argument. */ \
  mov %rsi,  8(%rsp); /* 2nd argument. */ \
  mov %rdi,   (%rsp); /* 1st argument. */

#define DO_LOAD_REG \
  mov   (%rsp), %rdi; /* 1st argument. */ \
  mov  8(%rsp), %rsi; /* 2nd argument. */ \
  mov 16(%rsp), %rdx; /* 3rd argument. */ \
  add $24, %rsp;

#define DO_CHECK_PERM(oop_ofs) \
  mov is_in_permanent@GOTPCREL(%rip), %r11; \
  mov collectedHeap@GOTPCREL(%rip), %rdi;   \
  mov (%rdi), %rdi;                         \
  mov oop_ofs(%rsp), %rsi;                  \
  push %rbp;                                \
  mov %rsp, %rbp;                           \
  call *(%r11);                             \
  pop %rbp;                                 \
  test %al, %al;                            

#define DO_JMP_TO_CALLBACK(header, ary_idx, oop_ofs) \
  mov oop_ofs(%rsp), %rdi;                                  \
  mov header##_enter_hook_##ary_idx##@GOTPCREL(%rip), %r11; \
  push %rbp;                                                \
  mov %rsp, %rbp;                                           \
  call *(%r11);                                             \
  pop %rbp;

#define OVERRIDE_FUNC_DEFINE(header, ary_idx) \
.global header##_override_func_##ary_idx ;                     \
.type header##_override_func_##ary_idx, @function;             \
header##_override_func_##ary_idx: ;                            \
  /* Save argument registers. */ ;                             \
  DO_SAVE_REG                                                  \
                                                               \
  /* Set pointer of original function to stack. */ ;           \
  mov header##_original_func_##ary_idx##@GOTPCREL(%rip), %r11; \
  push (%r11);


/* Macro for override function define. */

#define OVERRIDE_DEFINE(header, ary_idx, oop_arg_idx) \
  OVERRIDE_FUNC_DEFINE(header, ary_idx)              \
                                                     \
  /* Check permanent object. */ ;                    \
  DO_CHECK_PERM(oop_arg_idx*8)                       \
  jne .LORIGINAL_FUNC;                               \
                                                     \
  /* Call callback function. */ ;                    \
  DO_JMP_TO_CALLBACK(header, ary_idx, oop_arg_idx*8) \
  jmp .LORIGINAL_FUNC                   

#define OVERRIDE_DEFINE_WITHOUT_PERMCHECK(header, ary_idx, oop_arg_idx) \
  OVERRIDE_FUNC_DEFINE(header, ary_idx)              \
                                                     \
  /* Call callback function. */ ;                    \
  DO_JMP_TO_CALLBACK(header, ary_idx, oop_arg_idx*8) \
  jmp .LORIGINAL_FUNC                   

#define OVERRIDE_DEFINE_WITH_CALLER_CHECK(header, ary_idx, oop_arg_idx, valid_caller) \
  OVERRIDE_FUNC_DEFINE(header, ary_idx)              \
                                                     \
  /* Check vtable of caller */                       \
  CHECK_VTABLE(%rdx, valid_caller)                   \
  jz .LORIGINAL_FUNC;                                \
                                                     \
  /* Check permanent object. */ ;                    \
  DO_CHECK_PERM(oop_arg_idx*8)                       \
  jne .LORIGINAL_FUNC;                               \
                                                     \
  /* Call callback function. */ ;                    \
  DO_JMP_TO_CALLBACK(header, ary_idx, oop_arg_idx*8) \
  jmp .LORIGINAL_FUNC

#define OVERRIDE_ADJ_DEFINE(header, ary_idx, oop_arg_idx) \
  OVERRIDE_FUNC_DEFINE(header, ary_idx)               \
                                                      \
  /* Check permanent object. */ ;                     \
  DO_CHECK_PERM(oop_arg_idx*8)                        \
  jz .LORIGINAL_FUNC;                                 \
                                                      \
  /* Call callback function. */ ;                     \
  DO_JMP_TO_CALLBACK(header, ary_idx, oop_arg_idx*8)  \
  jmp .LORIGINAL_FUNC                   


/* Begin of .text section. */
    .text


.LORIGINAL_FUNC:
  pop %r11;
  DO_LOAD_REG
  jmp *%r11;


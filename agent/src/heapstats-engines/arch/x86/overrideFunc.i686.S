/*!
 * \file overrideFunc.i686.S
 * \brief This file is used to override JVM inner function for i686.<br>
 *        The function defined this file, used with v-table hook.<br>
 *        So in this file, all function is written by only assembler and macro.
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


#ifdef AVX
#include "arch/x86/avx/overrideFunc.S"
#elif defined SSE4
#include "arch/x86/sse4/overrideFunc.S"
#else
  #define CHECK_VTABLE(oop_reg, valid_caller) \
    mov (oop_reg), oop_reg;             \
    xor      %edx, %edx;                \
    mov valid_caller##@GOT(%ebx), %ecx; \
    cmp    (%ecx), oop_reg;             \
    cmove    %ecx, %edx;                \
    cmp   4(%ecx), oop_reg;             \
    cmove    %ecx, %edx;                \
    test     %edx, %edx;
#endif


/* Macro for function parts define. */

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 6)
  #define GET_GOT_ADDR \
    call __x86.get_pc_thunk.bx;        \
    add $_GLOBAL_OFFSET_TABLE_, %ebx;
#else
  #define GET_GOT_ADDR \
    call __i686.get_pc_thunk.bx;       \
    add $_GLOBAL_OFFSET_TABLE_, %ebx;
#endif

#define DO_CHECK_PERM(oop_ofs) \
  /* EBP + (oop_ofs - 1) + evacuated_EBP(1) + return_addr(1)*/ \
  /*    = EBP + 4(ptr size) * (oop_ofs + 1) */                 \
  mov $##oop_ofs, %esi;                                        \
  lea (%ebp, %esi, 4), %edx;                                   \
  mov 4(%edx), %edx; /* oop_ofs begins from "1" */             \
  mov is_in_permanent@GOT(%ebx), %ecx;                         \
  mov collectedHeap@GOT(%ebx), %eax;                           \
  sub $8, %esp;                                                \
  mov %edx, 4(%esp); /* Set oop address. */                    \
  mov (%eax), %eax;                                            \
  mov %eax, (%esp);  /* Set "this" pointer. */                 \
  call *(%ecx);                                                \
  lea -16(%ebp), %esp; /* Restore stack. */                    \
  test %al, %al;

#define DO_JMP_TO_CALLBACK(header, ary_idx, oop_ofs) \
  /* EBP + (oop_ofs - 1) + evacuated_EBP(1) + return_addr(1)*/ \
  /*    = EBP + 4(ptr size) * (oop_ofs + 1) */                 \
  mov $##oop_ofs, %esi;                                        \
  lea (%ebp, %esi, 4), %edx;                                   \
  mov 4(%edx), %edx; /* oop_ofs begins from "1" */             \
  push %edx;                                                   \
  mov header##_enter_hook_##ary_idx##@GOT(%ebx), %edx;         \
  call *(%edx);                                                \
  lea -16(%ebp), %esp; /* Restore stack. */

#define OVERRIDE_FUNC_DEFINE(header, ary_idx) \
.global header##_override_func_##ary_idx ;                \
.type header##_override_func_##ary_idx, @function;        \
header##_override_func_##ary_idx: ;                       \
  push %ebp;                                              \
  mov %esp, %ebp;                                         \
                                                          \
  /* Evacuate callee-saved registers. */ ;                \
  sub $8, %esp;                                           \
  mov %ebx,  (%esp);                                      \
  mov %esi, 4(%esp);                                      \
                                                          \
  /* Get GOT base address. */ ;                           \
  GET_GOT_ADDR                                            \
                                                          \
  /* Set pointer of original function to stack. */ ;      \
  mov header##_original_func_##ary_idx##@GOT(%ebx), %edx; \
  push (%edx);


/* Macro for override function define. */

#define OVERRIDE_DEFINE(header, ary_idx, oop_arg_idx) \
  OVERRIDE_FUNC_DEFINE(header, ary_idx)              \
                                                     \
  /* Check permanent object. */ ;                    \
  DO_CHECK_PERM(oop_arg_idx)                         \
  jne .LORIGINAL_FUNC;                               \
                                                     \
  /* Call callback function. */ ;                    \
  DO_JMP_TO_CALLBACK(header, ary_idx, oop_arg_idx) \
  jmp .LORIGINAL_FUNC;

#define OVERRIDE_DEFINE_WITHOUT_PERMCHECK(header, ary_idx, oop_arg_idx) \
  OVERRIDE_FUNC_DEFINE(header, ary_idx)              \
                                                     \
  /* Call callback function. */ ;                    \
  DO_JMP_TO_CALLBACK(header, ary_idx, oop_arg_idx)   \
  jmp .LORIGINAL_FUNC;

#define OVERRIDE_DEFINE_WITH_CALLER_CHECK(header, ary_idx, oop_arg_idx, valid_caller) \
  OVERRIDE_FUNC_DEFINE(header, ary_idx)                    \
                                                           \
  /* Get 3rd arg (OopClosure) */                           \
  /* EBP + 2(3rd argument) + evacuate_ebp + return_addr */ \
  /*    = EBP + 4(num ptrs) * 4(ptr size) */               \
  mov 16(%ebp), %eax;                                      \
                                                           \
  /* Check vtable of caller */                             \
  CHECK_VTABLE(%eax, valid_caller)                         \
  jz .LORIGINAL_FUNC;                                      \
                                                           \
  /* Check permanent object. */ ;                          \
  DO_CHECK_PERM(oop_arg_idx)                               \
  jne .LORIGINAL_FUNC;                                     \
                                                           \
  /* Call callback function. */ ;                          \
  DO_JMP_TO_CALLBACK(header, ary_idx, oop_arg_idx)         \
  jmp .LORIGINAL_FUNC;

#define OVERRIDE_ADJ_DEFINE(header, ary_idx, oop_arg_idx) \
  OVERRIDE_FUNC_DEFINE(header, ary_idx)            \
  /* Check permanent object. */ ;                  \
  DO_CHECK_PERM(oop_arg_idx)                       \
  jz .LORIGINAL_FUNC;                              \
                                                   \
  /* Call callback function. */ ;                  \
  DO_JMP_TO_CALLBACK(header, ary_idx, oop_arg_idx) \
  jmp .LORIGINAL_FUNC;


/* Begin of .text section. */
    .text

.LORIGINAL_FUNC:
  mov %ebp, %esp;
  pop %ebp;

  /* Set original function pointer to ECX. */
  mov  -16(%esp), %ecx;

  /* Restore callee-saved registers. */
  mov  -12(%esp), %ebx;
  mov   -8(%esp), %esi;

  /* Jump to original function. */
  jmp *%ecx;


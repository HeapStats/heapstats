/*!
 * \file jvmInfo.inline.hpp
 * \brief This file is used to get JVM performance information.
 *        This source is optimized for SSE3 instruction set.
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

#ifndef SSE3_JVMINFO_INLINE_H
#define SSE3_JVMINFO_INLINE_H

/*!
 * \brief  memcpy for gccause
 */
inline void TJvmInfo::loadGCCause(void) {
  /* Strcpy with SSE3 (80 bytes).
   * Sandy Bridge can two load operations per cycle.
   * So we should execute sequentially 2-load / 1-store ops.
   *   Intel 64 and IA-32 Architectures Optimization Reference Manual
   *     3.6.1.1 Make Use of Load Bandwidth in Intel Microarchitecture
   *     Code Name Sandy Bridge
   */
  asm volatile(
    "lddqu    (%0), %%xmm0;"
    "lddqu  16(%0), %%xmm1;"
    "movdqa %%xmm0,   (%1);"
    "lddqu  32(%0), %%xmm0;"
    "lddqu  48(%0), %%xmm2;"
    "movdqa %%xmm1, 16(%1);"
    "lddqu  64(%0), %%xmm1;"
    "movdqa %%xmm0, 32(%1);"
    "movdqa %%xmm2, 48(%1);"
    "movdqa %%xmm1, 64(%1);"
    :
    : "d"(this->_gcCause), /* GC Cause address.   */
      "c"(this->gcCause)   /* GC Cause container. */
    : "cc", "%xmm0", "%xmm1", "%xmm2"
  );
}

/*!
 * \brief Set "Unknown GCCause" to gccause.
 */
inline void TJvmInfo::SetUnknownGCCause(void) {
  asm volatile(
    "movdqa   (%1), %%xmm0;"
    "movdqa %%xmm0,   (%0);"
    :
    : "a"(this->gcCause), "c"(UNKNOWN_GC_CAUSE)
    : "%xmm0"
  );
}

#endif  // SSE3_JVMINFO_INLINE_H

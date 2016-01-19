/*!
 * \file jvmInfo.inline.hpp
 * \brief Getting JVM performance information.
 *        This source is optimized for AVX instruction set.
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

#ifndef AVX_JVMINFO_INLINE_H
#define AVX_JVMINFO_INLINE_H

/*!
 * \brief  memcpy for gccause
 */
inline void TJvmInfo::loadGCCause(void) {
  /* Strcpy with AVX (80 bytes).
   * Sandy Bridge can two load operations per cycle.
   * So we should execute sequentially 2-load / 1-store ops.
   *   Intel 64 and IA-32 Architectures Optimization Reference Manual
   *     3.6.1.1 Make Use of Load Bandwidth in Intel Microarchitecture
   *     Code Name Sandy Bridge
   */
  asm volatile(
    "vlddqu    (%0), %%ymm0;"
    "vlddqu  32(%0), %%ymm1;"
    "vmovdqa %%ymm0,   (%1);"
    "vlddqu  64(%0), %%xmm0;"
    "vmovdqa %%ymm1, 32(%1);"
    "vmovdqa %%xmm0, 64(%1);"
    :
    : "d"(this->_gcCause), /* GC Cause address.   */
      "c"(this->gcCause)   /* GC Cause container. */
    : "cc", "%xmm0"        /*, "%ymm0", "%ymm1" */
  );
}

/*!
 * \brief Set "Unknown GCCause" to gccause.
 */
inline void TJvmInfo::SetUnknownGCCause(void) {
  asm volatile(
    "vmovdqa   (%1), %%xmm0;"
    "vmovdqa %%xmm0,   (%0);"
    :
    : "a"(this->gcCause), "c"(UNKNOWN_GC_CAUSE)
    : "%xmm0"
  );
}

#endif  // AVX_JVMINFO_INLINE_H

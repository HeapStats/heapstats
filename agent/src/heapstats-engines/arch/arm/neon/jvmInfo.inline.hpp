/*!
 * \file jvmInfo.inline.hpp
 * \brief This file is used to get JVM performance information.
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

#ifndef NEON_JVMINFO_INLINE_H
#define NEON_JVMINFO_INLINE_H

/*!
 * \brief  memcpy for gccause
 */
inline void TJvmInfo::loadGCCause(void) {
  /* Strcpy with NEON (80 bytes). */
  asm volatile(
    "vld1.8 {%%q0}, [%0]!;"
    "vld1.8 {%%q1}, [%0]!;"
    "vld1.8 {%%q2}, [%0]!;"
    "vld1.8 {%%q3}, [%0]!;"
    "vld1.8 {%%q4}, [%0];"
    "vst1.8 {%%q0}, [%1]!;"
    "vst1.8 {%%q1}, [%1]!;"
    "vst1.8 {%%q2}, [%1]!;"
    "vst1.8 {%%q3}, [%1]!;"
    "vst1.8 {%%q4}, [%1];"
    :
    : "r"(this->_gcCause), /* GC Cause address.   */
      "r"(this->gcCause)   /* GC Cause container. */
    : "cc", "%q0", "%q1", "%q2", "%q3", "q4"
  );
}

/*!
 * \brief Set "Unknown GCCause" to gccause.
 */
inline void TJvmInfo::SetUnknownGCCause(void) {
  asm volatile(
    "vld1.8 {%%q0}, [%1];"
    "vst1.8 {%%q0}, [%0];"
    :
    : "r"(this->gcCause), "r"(UNKNOWN_GC_CAUSE)
    : "%q0"
  );
}

#endif  // NEON_JVMINFO_INLINE_H

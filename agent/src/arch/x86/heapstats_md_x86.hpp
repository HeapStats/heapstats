/*!
 * \file heapstats_md_x86.hpp
 * \brief Proxy library for HeapStats backend.
 *        This file implements x86 specific code for loading backend library.
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

#ifndef HEAPSTATS_MD_X86_HPP
#define HEAPSTATS_MX_X86_HPP

#include "heapstats_md.hpp"

/* Instruction set definition */
#define OPTIMIZE_NONE "none"
#define OPTIMIZE_SSE2 "sse2"
#define OPTIMIZE_SSE3 "sse3"
#define OPTIMIZE_SSE4 "sse4"
#define OPTIMIZE_AVX "avx"

#endif  // HEAPSTATS_MD_X86_HPP

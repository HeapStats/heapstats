/*!
 * \file pcreRegex.cpp
 * \brief Regex implementation to use PCRE.
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

#include <string.h>
#include <pcre.h>

#include "pcreRegex.hpp"

TPCRERegex::TPCRERegex(const char *pattern, int ovecsz)
    : TRegexAdapter(pattern) {
  const char *errMsg;
  int errPos;
  pcreExpr = pcre_compile(pattern, PCRE_ANCHORED, &errMsg, &errPos, NULL);

  if (pcreExpr == NULL) {
    throw errMsg;
  }

  ovecsize = ovecsz;
  ovec = new int[ovecsize];
}

TPCRERegex::~TPCRERegex() {
  pcre_free(pcreExpr);
  delete[] ovec;
}

bool TPCRERegex::find(const char *str) {
  find_str = str;
  int result =
      pcre_exec(pcreExpr, NULL, str, strlen(str), 0, 0, ovec, ovecsize);
  return result >= 0;
}

char *TPCRERegex::group(int index) {
  const char *val = NULL;

  if (pcre_get_substring(find_str, ovec, ovecsize, index, &val) <= 0) {
    if (val != NULL) {
      pcre_free_substring(val);
    }

    throw "Could not get substring through PCRE.";
  }

  char *result = strdup(val);
  pcre_free_substring(val);
  return result;
}

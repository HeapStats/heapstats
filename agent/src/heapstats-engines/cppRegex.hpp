/*!
 * \file cppRegex.hpp
 * \brief Regex implementation to use regex in C++11.
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

#ifndef CPP_REGEX_HPP
#define CPP_REGEX_HPP

#include <regex>

#include "regexAdapter.hpp"

class TCPPRegex : TRegexAdapter {
 private:
  std::regex expr;
  std::cmatch mat;

 public:
  TCPPRegex(const char *pattern) : TRegexAdapter(pattern), expr(pattern){};
  virtual ~TCPPRegex(){};

  bool find(const char *str) { return std::regex_match(str, mat, expr); };

  char *group(int index) { return strdup(mat.str(index).c_str()); };
};

#endif  // CPP_REGEX_HPP

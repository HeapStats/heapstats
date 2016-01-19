/*!
 * \file regexAdapter.hpp
 * \brief C++ super class for regex.
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

#ifndef REGEXADAPTER_HPP
#define REGEXADAPTER_HPP

class TRegexAdapter {
 protected:
  const char *pattern_str;

 public:
  TRegexAdapter(const char *pattern) : pattern_str(pattern){};
  virtual ~TRegexAdapter(){};

  virtual bool find(const char *str) = 0;
  virtual char *group(int index) = 0;
};

#endif  // REGEXADAPTER_HPP

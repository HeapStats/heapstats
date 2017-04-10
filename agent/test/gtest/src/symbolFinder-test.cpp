/*!
 * Copyright (C) 2017 Nippon Telegraph and Telephone Corporation
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
#include <dlfcn.h>
#include <gtest/gtest.h>

#include <heapstats-engines/symbolFinder.hpp>

#define LIBJVM_PATH_PATTERN "/usr/lib/jvm/java-"
#define LIBJVM_NAME "libjvm.so"
#define FIND_SYMBOL "JNI_CreateJavaVM"

class SymbolFinderTest : public testing::Test{};

TEST_F(SymbolFinderTest, findSymbol){
  TSymbolFinder symFinder;

  ASSERT_TRUE(symFinder.loadLibrary(LIBJVM_PATH_PATTERN, LIBJVM_NAME));

  void *fromSymFinder = symFinder.findSymbol(FIND_SYMBOL);
  void *fromDlsym = dlsym(RTLD_DEFAULT, FIND_SYMBOL);

  ASSERT_TRUE(fromSymFinder == fromDlsym);
}


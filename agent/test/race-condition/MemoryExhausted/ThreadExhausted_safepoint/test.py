'''
Copyright (C) 2017 Nippon Telegraph and Telephone Corporation

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
'''

import sys, os, time
sys.path.append(os.pardir + "/../")

import common
import re

# from jvmti.h
JVMTI_RESOURCE_EXHAUSTED_JAVA_HEAP = 0x0002
JVMTI_RESOURCE_EXHAUSTED_THREADS = 0x0004

def flagToInt():
    m = re.search("\d+$", gdb.execute("p flags", False, True))
    return int(m.group(0))

def Cond_MemoryExhausted():
  return ((flagToInt() & JVMTI_RESOURCE_EXHAUSTED_JAVA_HEAP) != 0)

def Cond_ThreadExhausted():
  return ((flagToInt() & JVMTI_RESOURCE_EXHAUSTED_THREADS) != 0)

common.initialize("OnResourceExhausted", Cond_MemoryExhausted, "OnResourceExhausted", Cond_ThreadExhausted, True, at_safepoint=True, jcmd_for_safepoint=False)

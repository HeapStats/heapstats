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

common.initialize("OnResourceExhausted", Cond_MemoryExhausted, "OnResourceExhausted", Cond_ThreadExhausted, True, False, True)

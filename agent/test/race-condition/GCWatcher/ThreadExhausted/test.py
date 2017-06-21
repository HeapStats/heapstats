import sys, os, time
sys.path.append(os.pardir + "/../")

import common

common.initialize("TGCWatcher::entryPoint:RACE_COND_DEBUG_POINT", common.return_true, "OnResourceExhausted", common.return_true, True)

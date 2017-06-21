import sys, os, time
sys.path.append(os.pardir + "/../")

import common

common.initialize("OnResourceExhausted", common.return_true, "TGCWatcher::entryPoint:RACE_COND_DEBUG_POINT", common.return_true, False, True)

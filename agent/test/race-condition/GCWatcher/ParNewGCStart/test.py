import sys, os, time
sys.path.append(os.pardir + "/../")

import common

common.initialize("TGCWatcher::entryPoint:RACE_COND_DEBUG_POINT", common.return_true, "VM_GenCollectForAllocation::doit", common.return_true, False)

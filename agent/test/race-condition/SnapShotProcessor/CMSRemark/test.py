import sys, os, time
sys.path.append(os.pardir + "/../")

import common

common.initialize("TSnapShotProcessor::entryPoint:RACE_COND_DEBUG_POINT", common.return_true, "VM_CMS_Final_Remark::doit", common.return_true, False)

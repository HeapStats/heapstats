import sys, os, time
sys.path.append(os.pardir + "/../")

import common

def Cond_OnClassPrepare():
    gdb.newest_frame().older().select()
    symbol = gdb.execute("p (char *)klass->_name->_body", False, True)
    return (symbol.find("DynLoad") != -1)


common.initialize("OnClassPrepare", Cond_OnClassPrepare, "TSnapShotProcessor::entryPoint:RACE_COND_DEBUG_POINT", common.return_true, True, False, True)

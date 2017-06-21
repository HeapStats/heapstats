import sys, os
sys.path.append(os.pardir + "/../")

import common

def Cond_OnClassPrepare():
    gdb.newest_frame().older().select()
    symbol = gdb.execute("p (char *)klass->_name->_body", False, True)
    return (symbol.find("DynLoad") != -1)


common.initialize("OnClassPrepare", Cond_OnClassPrepare, "OnClassPrepare", Cond_OnClassPrepare, False, False, True)

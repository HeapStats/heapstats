import sys, os, time
sys.path.append(os.pardir + "/../")

import common

def Cond_OnClassPrepare():
    gdb.newest_frame().older().select()
    symbol = gdb.execute("p (char *)klass->_name->_body", False, True)
    return (symbol.find("DynLoad") != -1)


class BreakAtDataDump(gdb.Breakpoint):
    def __init__(self):
        super(BreakAtDataDump, self).__init__("data_dump")

    def stop(self):
        gdb.execute("set var ReduceSignalUsage=true")
        gdb.write("set true to ReduceSignalUsage\n")


class BreakAtShouldPostDataDump(gdb.Breakpoint):
    def __init__(self):
        super(BreakAtShouldPostDataDump, self).__init__("JvmtiExport::should_post_data_dump")

    def stop(self):
        gdb.execute("set var ReduceSignalUsage=false")
        gdb.write("set false to ReduceSignalUsage\n")


common.initialize("OnClassPrepare", Cond_OnClassPrepare, "OnDataDumpRequestForSnapShot", common.return_true, True)
BreakAtDataDump()
BreakAtShouldPostDataDump()

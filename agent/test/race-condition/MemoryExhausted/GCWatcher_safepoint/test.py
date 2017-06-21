import sys, os, time, threading
sys.path.append(os.pardir + "/../")

import common

# Kick GC after occurring GC.
def Kick_GC():
    threading.Thread(target=os.system, args=("jcmd 0 GC.run",)).start()
    return True

common.initialize("OnResourceExhausted", Kick_GC, "TGCWatcher::entryPoint:RACE_COND_DEBUG_POINT", common.return_true, False, False, True)

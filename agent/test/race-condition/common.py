import gdb

class ContinueInvoker:
    def __init__(self, th):
        self.__thread_num = th

    def __call__(self):
        gdb.execute("thread " + str(self.__thread_num))
        gdb.execute("continue")


class BreakPointHandler(gdb.Breakpoint):
    def __init__(self, bp, condition):
        super(BreakPointHandler, self).__init__(bp)
        self.__cond = condition
        self.thread_num = -1

    def stop(self):
        if(self.__cond()):
            self.thread_num = gdb.selected_thread().num
            self.enabled = False
            gdb.write("Suspend thread #" + str(self.thread_num) + "\n")
            return True
        else:
            return False


class RaceConditionGenerator:
    def __init__(self, primary, secondary, secondary_enabled):
        self.__primary = primary
        self.__secondary = secondary
        self.__secondary_enabled = secondary_enabled

    def coordinate(self):
        if((not self.__secondary_enabled) and (not self.__primary.enabled) and (self.__secondary.thread_num == -1)):
            self.__secondary.enabled = True
        elif((self.__primary.thread_num != -1) and (self.__secondary.thread_num != -1)):
            gdb.write("Test start!\n")
            gdb.post_event(ContinueInvoker(self.__primary.thread_num))
            gdb.post_event(ContinueInvoker(self.__secondary.thread_num))


class StopHandler:
    def __init__(self, rcgen):
        self.__rcgen = rcgen

    def __call__(self, event):
        # Ignore signals
        if(isinstance(event, gdb.SignalEvent)):
            gdb.post_event(ContinueInvoker(event.inferior_thread.num))
        elif(isinstance(event, gdb.BreakpointEvent)):
            self.__rcgen.coordinate()


def return_true():
    return True


def exit_handler(event):
    gdb.execute("quit")


def initialize(primary, primary_cond, secondary, secondary_cond, secondary_enabled):
    gdb.execute("set breakpoint pending on")
    gdb.execute("set target-async on")
    gdb.execute("set pagination off")
    gdb.execute("set non-stop on")

    p = BreakPointHandler(primary, primary_cond)
    s = BreakPointHandler(secondary, secondary_cond)
    s.enabled = secondary_enabled
    rcgen = RaceConditionGenerator(p, s, secondary_enabled)

    gdb.events.stop.connect(StopHandler(rcgen))
    gdb.events.exited.connect(exit_handler)


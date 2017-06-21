'''
Copyright (C) 2017 Nippon Telegraph and Telephone Corporation

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
'''

import gdb, threading, os

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


class SafepointBreaker(gdb.Breakpoint):
    def __init__(self, rcgen):
        super(SafepointBreaker, self).__init__("Interpreter::notice_safepoints")
        self.at_safepoint = False
        self.__rcgen = rcgen

    def stop(self):
        self.at_safepoint = True
        self.enabled = False
        self.__rcgen.coordinate()
        return False


class RaceConditionGenerator:
    def __init__(self, primary, secondary, secondary_enabled, need_safepoint, is_reverse):
        self.__primary = primary
        self.__secondary = secondary
        self.__secondary_enabled = secondary_enabled
        self.__need_safepoint = need_safepoint
        self.__safepoint_breaker = None
        self.__is_reverse = is_reverse

    def coordinate(self):
        if((not self.__secondary_enabled) and (not self.__primary.enabled) and (self.__secondary.thread_num == -1)):
            self.__secondary.enabled = True
        elif((self.__primary.thread_num != -1) and (self.__secondary.thread_num != -1)):
            if self.__need_safepoint:
                if self.__safepoint_breaker is None:
                    self.__safepoint_breaker = SafepointBreaker(self)
                    threading.Thread(target=os.system, args=("jcmd 0 GC.run",)).start()
                    return
                elif not self.__safepoint_breaker.at_safepoint:
                    return

            gdb.write("Test start!\n")
            gdb.execute("p SafepointSynchronize::_state")

            if self.__is_reverse:
                gdb.post_event(ContinueInvoker(self.__secondary.thread_num))
                gdb.post_event(ContinueInvoker(self.__primary.thread_num))
            else:
                gdb.post_event(ContinueInvoker(self.__primary.thread_num))
                gdb.post_event(ContinueInvoker(self.__secondary.thread_num))


def is_stopped():
    result = True
    for thread in gdb.selected_inferior().threads():
        result &= thread.is_stopped()
    return result

class StopHandler:
    def __init__(self, rcgen):
        self.__rcgen = rcgen
        self.__is_abort = False

    def __call__(self, event):
        # Ignore signals
        if isinstance(event, gdb.SignalEvent):
            if(event.stop_signal == "SIGABRT"):
                self.__is_abort = True
                gdb.write("Stop all threads...\n")
                gdb.execute("interrupt -a")
            else:
                gdb.post_event(ContinueInvoker(event.inferior_thread.num))
        elif isinstance(event, gdb.BreakpointEvent):
            self.__rcgen.coordinate()
        elif(self.__is_abort and is_stopped()):
            self.__is_abort = False
            gdb.write("Dumping core...\n")
            gdb.execute("gcore")
            gdb.execute("quit")


def return_true():
    return True


def exit_handler(event):
    gdb.execute("quit")


def initialize(primary, primary_cond, secondary, secondary_cond, secondary_enabled, is_reverse=False, at_safepoint=False):
    gdb.execute("set confirm off")
    gdb.execute("set breakpoint pending on")
    gdb.execute("set target-async on")
    gdb.execute("set pagination off")
    gdb.execute("set non-stop on")

    p = BreakPointHandler(primary, primary_cond)
    s = BreakPointHandler(secondary, secondary_cond)
    s.enabled = secondary_enabled
    rcgen = RaceConditionGenerator(p, s, secondary_enabled, at_safepoint, is_reverse)

    gdb.events.stop.connect(StopHandler(rcgen))
    gdb.events.exited.connect(exit_handler)


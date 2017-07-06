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

import sys, os, time
sys.path.append(os.pardir + "/../")

import common

def Cond_OnClassPrepare():
    gdb.newest_frame().older().select()
    symbol = gdb.execute("p (char *)klass->_name->_body", False, True)
    return (symbol.find("DynLoad") != -1)


common.initialize("OnClassPrepare", Cond_OnClassPrepare, "TGCWatcher::entryPoint:RACE_COND_DEBUG_POINT", common.return_true, True, False, True)

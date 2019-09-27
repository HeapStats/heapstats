/*
 * module-info.java
 *
 * Copyright (C) 2019 Yasumasa Suenaga
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

module heapstats.core {
    requires java.logging;
    requires java.xml.bind;
    
    exports jp.co.ntt.oss.heapstats.parser;
    exports jp.co.ntt.oss.heapstats.parser.handler;
    
    exports jp.co.ntt.oss.heapstats.container.log;
    exports jp.co.ntt.oss.heapstats.container.snapshot;
    exports jp.co.ntt.oss.heapstats.container.threadrecord;
    
    exports jp.co.ntt.oss.heapstats.snapshot;
    exports jp.co.ntt.oss.heapstats.task;
    exports jp.co.ntt.oss.heapstats.xml.binding;
    
    exports jp.co.ntt.oss.heapstats.lambda to heapstats.jmx, heapstats.fx, heapstats.cli;
}

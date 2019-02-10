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

module heapstats.fx {
    requires java.logging;
    requires java.rmi; // for JMX
    requires java.xml.bind;

    requires heapstats.jmx;
    requires heapstats.core;
    requires heapstats.plugin.api;
    
    requires javafx.fxml;
    requires javafx.controls;
    requires javafx.swing;

    // JGraphX has automatic module name:
    //   https://github.com/jgraph/jgraphx/pull/93
    requires com.mxgraph.jgraphx;
    
    exports jp.co.ntt.oss.heapstats.fx to javafx.graphics, javafx.fxml;
    opens jp.co.ntt.oss.heapstats.fx to javafx.fxml;
}

/*
 * ReferenceGraph.java
 * Created on 2012/11/18
 *
 * Copyright (C) 2011-2015 Nippon Telegraph and Telephone Corporation
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
package jp.co.ntt.oss.heapstats.plugin.builtin.snapshot.tabs;

import java.util.HashMap;
import java.util.Map;

import com.mxgraph.util.mxConstants;
import com.mxgraph.view.mxGraph;
import com.mxgraph.view.mxStylesheet;

/**
 * extends {@link mxGraph}<br>
 * <br>
 * Implements a graph object that allows to create diagrams from a graph model
 * and stylesheet.
 */
public class ReferenceGraph extends mxGraph {

    @Override
    public final String getToolTipForCell(Object cell) {

        if (cell instanceof ReferenceCell) {
            return ((ReferenceCell) cell).toString();
        }

        return super.getToolTipForCell(cell);
    }

    /**
     * Sets the style for edges.
     */
    public final void setEdgeStyle() {
        Map<String, Object> edgeStyle = new HashMap<>();
        edgeStyle.put(mxConstants.STYLE_SHAPE, mxConstants.SHAPE_CONNECTOR);
        edgeStyle.put(mxConstants.STYLE_ENDARROW, mxConstants.ARROW_DIAMOND);
        edgeStyle.put(mxConstants.STYLE_STROKECOLOR, "#000000");
        edgeStyle.put(mxConstants.STYLE_FONTCOLOR, "#000000");
        edgeStyle.put(mxConstants.STYLE_ALIGN, mxConstants.ALIGN_RIGHT);

        mxStylesheet style = new mxStylesheet();
        style.setDefaultEdgeStyle(edgeStyle);
        setStylesheet(style);
    }

}

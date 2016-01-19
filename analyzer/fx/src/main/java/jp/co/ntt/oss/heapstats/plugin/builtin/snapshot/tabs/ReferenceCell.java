/*
 * ReferenceCell.java
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

import java.text.NumberFormat;
import com.mxgraph.model.mxCell;
import com.mxgraph.model.mxGeometry;
import jp.co.ntt.oss.heapstats.container.snapshot.ObjectData;

/**
 * extends {@link mxCell}<br>
 * <br>
 * Cells are the elements of the graph model.
 */
public class ReferenceCell extends mxCell {

    /**
     * serialVersionUID.
     */
    private static final long serialVersionUID = -4403355352725440764L;

    /**
     * Defines the height of the cell.
     */
    private static final int CELL_HEIGHT = 30;

    /**
     * Define the width of the character in the cell.
     */
    private static final int CHAR_WIDTH = 6;

    /**
     * Information about the objects to display Map.
     */
    private final ObjectData objectData;

    /**
     * Holds information whether the cell as a starting point.
     */
    private boolean rootCell;

    /**
     * Constructs a new cell.
     *
     * @param data Information about the objects to display Map.
     * @param root This cell is root or not.
     * @param edge This cell is edge or not.
     */
    public ReferenceCell(ObjectData data, boolean root, boolean edge) {
        super();

        objectData = data;

        if (edge) {
            setEdge(true);
        } else {
            setValue(data.getName());
            setConnectable(false);
            setVertex(true);

            if (root) {
                setStyle("shape=ellipse;fillColor=red;fontColor=black");
            }

            rootCell = root;
        }

        setConnectable(false);
        setGeometry(new mxGeometry(0, 0, CHAR_WIDTH * data.getName().length(), CELL_HEIGHT));
    }

    /**
     * Return the tag.
     *
     * @return Return the tag
     */
    public final long getTag() {
        return objectData.getTag();
    }

    /**
     * It returns whether or not the cells to be the origin.
     *
     * @return It returns whether or not the cells to be the origin.
     */
    public final boolean isRoot() {
        return rootCell;
    }

    @Override
    public final String toString() {
        NumberFormat format = NumberFormat.getInstance();
        StringBuilder buf = new StringBuilder();

        if (isVertex()) {
            buf.append(objectData.getLoaderName());
            buf.append(" (instances = ");
            buf.append(format.format(objectData.getCount()));
            buf.append("; heapsize = ");
            buf.append(format.format(objectData.getTotalSize()));
            buf.append(" bytes)");
        } else {
            buf.append("instances = ");
            buf.append(format.format(objectData.getCount()));
            buf.append("; heapsize = ");
            buf.append(format.format(objectData.getTotalSize()));
            buf.append(" bytes");
        }

        return buf.toString();
    }

}

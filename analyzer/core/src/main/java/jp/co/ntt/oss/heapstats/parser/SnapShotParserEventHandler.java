/*
 * ParserEventHandler.java
 * Created on 2011/10/13
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

package jp.co.ntt.oss.heapstats.parser;

import jp.co.ntt.oss.heapstats.container.snapshot.ChildObjectData;
import jp.co.ntt.oss.heapstats.container.snapshot.ObjectData;
import jp.co.ntt.oss.heapstats.container.snapshot.SnapShotHeader;

/**
 * a ParserEventHandler is an event handler to write to the file.
 */
public interface SnapShotParserEventHandler {
    /** this enumeration is the Parse Result. */
    public enum ParseResult {
        /** Continue. */
        HEAPSTATS_PARSE_CONTINUE,
        /** Abort. */
        HEAPSTATS_PARSE_ABORT,
        /** Skip. */
        HEAPSTATS_PARSE_SKIP
    }

    /**
     * Call at the beginning of the snapshot in the file reading.
     *
     * @param off the snapshot file position
     * @return the ParseResult
     */
    ParseResult onStart(long off);

    /**
     * Set the header of the snapshot.
     *
     * @param header the SnapShot Header Information
     * @param parent The absolute path of the snapshot file
     * @return Return the ParseResult
     */
    ParseResult onNewSnapShot(SnapShotHeader header, String parent);

    /**
     * Entry the information of the parent class.
     *
     * @param data the Object information.
     * @return Return the ParseResult
     */
    ParseResult onEntry(ObjectData data);

    /**
     * Entry the information of the child class.
     *
     * @param parentClassTag Tag of the parent class
     * @param child child data of this entry
     * @return Return the ParseResult
     */
    ParseResult onChildEntry(long parentClassTag, ChildObjectData child);

    /**
     * Call at the end of the reading of the snapshot in the file.
     *
     * @param off the snapshot file position
     * @return Return the ParseResult
     */
    ParseResult onFinish(long off);
}

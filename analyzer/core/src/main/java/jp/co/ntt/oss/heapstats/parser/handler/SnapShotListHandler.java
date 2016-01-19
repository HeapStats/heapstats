/*
 * Copyright (C) 2014-2015 Yasumasa Suenaga
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
 */

package jp.co.ntt.oss.heapstats.parser.handler;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.function.Consumer;
import jp.co.ntt.oss.heapstats.container.snapshot.ChildObjectData;
import jp.co.ntt.oss.heapstats.container.snapshot.ObjectData;
import jp.co.ntt.oss.heapstats.container.snapshot.SnapShotHeader;
import jp.co.ntt.oss.heapstats.parser.SnapShotParserEventHandler;

/**
 * HeapStats SnapShot parser handler for parsing SnapShot headers.
 * @author Yasumasa Suenaga
 */
public class SnapShotListHandler implements SnapShotParserEventHandler{
    
    private final List<SnapShotHeader> headers;
    
    private long instances;
    
    private SnapShotHeader currentHeader;
    
    private final Optional<Consumer<? super Long>> progressUpdater;

    /**
     * Constructor of SnapShotListHandler
     * 
     * @param progressUpdater Consumer for ProgressIndicator.
     */
    public SnapShotListHandler(Consumer<? super Long> progressUpdater) {
        this.headers = new ArrayList<>();
        this.instances = 0;
        this.progressUpdater = Optional.ofNullable(progressUpdater);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ParseResult onStart(long off) {
        this.instances = 0;
        return ParseResult.HEAPSTATS_PARSE_CONTINUE;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ParseResult onNewSnapShot(SnapShotHeader header, String parent) {
        currentHeader = header;
        headers.add(header);
        return ParseResult.HEAPSTATS_PARSE_CONTINUE;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ParseResult onEntry(ObjectData data) {
        instances += data.getCount();
        return ParseResult.HEAPSTATS_PARSE_CONTINUE;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ParseResult onChildEntry(long parentClassTag, ChildObjectData child) {
        /* Nothing to do */
        return ParseResult.HEAPSTATS_PARSE_CONTINUE;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ParseResult onFinish(long off) {
        long snapshotSize = off - currentHeader.getFileOffset();
        currentHeader.setSnapShotSize(snapshotSize);
        currentHeader.setNumInstances(instances);
        progressUpdater.ifPresent(c -> c.accept(snapshotSize));
        return ParseResult.HEAPSTATS_PARSE_CONTINUE;
    }

    /**
     * Get SnapShot headers which are result of this task.
     * 
     * @return List of SnapShot header.
     */
    public List<SnapShotHeader> getHeaders() {
        return headers;
    }
    
}

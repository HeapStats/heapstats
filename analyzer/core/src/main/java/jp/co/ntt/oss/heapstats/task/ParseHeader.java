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

package jp.co.ntt.oss.heapstats.task;

import java.io.File;
import java.util.Comparator;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.atomic.AtomicLong;
import java.util.stream.Collectors;
import jp.co.ntt.oss.heapstats.container.snapshot.SnapShotHeader;
import jp.co.ntt.oss.heapstats.lambda.ConsumerWrapper;
import jp.co.ntt.oss.heapstats.parser.SnapShotParser;
import jp.co.ntt.oss.heapstats.parser.handler.SnapShotListHandler;

/**
 * Task thread implementation for parsing HeapStats SnapShot header.
 * @author Yasumasa Suenaga
 */
public class ParseHeader extends ProgressRunnable{
    
    private final List<String> files;
    
    private final boolean needJavaStyle;
    
    private final boolean parseAsPossible;
    
    private List<SnapShotHeader> snapShotList;
    
    /**
     * Constructor of ParseHeader.
     * 
     * @param files List of HeapStats SnapShot files.
     * @param needJavaStyle true if class name should be converted to Java-Style
     * @param parseAsPossible Parse SnapShot before occuring error.
     */
    public ParseHeader(List<String> files, boolean needJavaStyle, boolean parseAsPossible) {
        this.files = files;
        setTotal(this.files.stream()
                           .map(f -> new File(f))
                           .mapToLong(f -> f.length())
                           .sum());
        this.needJavaStyle = needJavaStyle;
        this.parseAsPossible = parseAsPossible;
        snapShotList = null;
    }

    /**
     * Get list of HeapStats SnapShot headers.
     * @return List of HeapStats SnapShot headers.
     */
    public List<SnapShotHeader> getSnapShotList() {
        return snapShotList;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run() {
        AtomicLong progress = new AtomicLong();
        SnapShotListHandler handler = new SnapShotListHandler(p -> updateProgress.ifPresent(c -> c.accept(progress.addAndGet(p))));
        SnapShotParser parser = new SnapShotParser(needJavaStyle);
        
        ConsumerWrapper<String> parseMethod = new ConsumerWrapper<>(f -> parser.parse(f, handler));
        Optional<Exception> parseError = Optional.empty();
        
        try{
            files.forEach(parseMethod);
        }
        catch(Exception e){
            parseError = Optional.of(e);
            
            if(!parseAsPossible){
                throw e;
            }
            
        }
        
        snapShotList = handler.getHeaders().stream()
                                           .sorted(Comparator.naturalOrder())
                                           .collect(Collectors.toList());
        
        if(parseError.isPresent()){
            throw new RuntimeException(parseError.get());
        }
        
    }
    
}

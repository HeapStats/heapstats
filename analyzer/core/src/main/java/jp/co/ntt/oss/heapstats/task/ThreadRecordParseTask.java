/*
 * Copyright (C) 2015 Yasumasa Suenaga
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
import java.io.IOException;
import java.io.UncheckedIOException;
import java.util.List;
import java.util.Map;
import jp.co.ntt.oss.heapstats.container.threadrecord.ThreadStat;
import jp.co.ntt.oss.heapstats.parser.ThreadRecordParser;

/**
 * Task class for parsing HeapStats Thread Recorder file.
 * 
 * @author Yasumasa Suenaga
 */
public class ThreadRecordParseTask extends ProgressRunnable{
    
    private final File file;
    
    private Map<Long, String> idMap;
    
    private List<ThreadStat> threadStatList;
    
    /**
     * Constructor of ThreadRecordParserTask.
     * 
     * @param files File to parse.
     */
    public ThreadRecordParseTask(File files){
        file = files;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run() {
        ThreadRecordParser parser = new ThreadRecordParser();
        
        try{
            parser.parse(file.toPath());
            
            idMap = parser.getIdMap();
            threadStatList = parser.getThreadStatList();
        }
        catch(IOException e){
            throw new UncheckedIOException(e);
        }
        
    }

    /**
     * Get Thread ID - Thread Name map.
     * 
     * @return Thread ID - Thread Name map.
     */
    public Map<Long, String> getIdMap() {
        return idMap;
    }

    /**
     * Get Thread stat list.
     * 
     * @return  List of ThreadStat.
     */
    public List<ThreadStat> getThreadStatList() {
        return threadStatList;
    }
    
}

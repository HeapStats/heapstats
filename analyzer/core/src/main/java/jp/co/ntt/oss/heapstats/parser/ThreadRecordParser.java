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
package jp.co.ntt.oss.heapstats.parser;

import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.SeekableByteChannel;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;
import java.util.stream.IntStream;
import jp.co.ntt.oss.heapstats.container.threadrecord.ThreadStat;
import jp.co.ntt.oss.heapstats.container.threadrecord.ThreadStat.ThreadEvent;

/**
 * Parser class for HeapStats Thread Recorder.
 * 
 * @author Yasumasa Suenaga
 */
public class ThreadRecordParser {
    
    /**
     * This map holds Thread ID - Thread Name map.
     */
    private Map<Long, String> idMap;
    
    /**
     * This list holds thread status.
     */
    private List<ThreadStat> threadStatList;
    
    /**
     * Get byte order from channel.
     * 
     * @param ch Channel to read.
     * @param buffer ByteBuffer to use.
     * @return ByteOrder in this channel.
     * @throws IOException 
     */
    private ByteOrder getByteOrder(SeekableByteChannel ch, ByteBuffer buffer) throws IOException{
        buffer.position(buffer.capacity() - 1);
        buffer.mark();
        ch.read(buffer);
        buffer.reset();
        
        return (buffer.get() == (byte)'L') ? ByteOrder.LITTLE_ENDIAN : ByteOrder.BIG_ENDIAN;
    }
    
    /**
     * Get Thread ID - Thread Name map entry from channel.
     * This method may throw UncheckedIOException if IOException occurrs.
     * 
     * @param ch Channel to read.
     * @param buffer ByteBuffer to use.
     * @param map Thread ID - Thread Name map to set.
     */
    private void setThreadStatToMap(SeekableByteChannel ch, ByteBuffer buffer, Map<Long, String> map){
        int capacity = buffer.capacity();
        
        try{
            buffer.position(capacity- 8);
            buffer.mark();
            ch.read(buffer);
            buffer.reset();
            long id = buffer.getLong();

            buffer.position(capacity - 4);
            buffer.mark();
            ch.read(buffer);
            buffer.reset();
            int len = buffer.getInt();

            ByteBuffer utf8Buffer = ByteBuffer.allocate(len);
            ch.read(utf8Buffer);
            utf8Buffer.flip();
            String className = new String(utf8Buffer.array());
        
            map.put(id, className);
        }
        catch(IOException e){
            throw new UncheckedIOException(e);
        }

    }

    /**
     * Get Thread ID - Thread Name map from channel.
     * 
     * @param ch Channel to read.
     * @param buffer ByteBuffer to use.
     * @return Thread ID - Thread Name map.
     * @throws IOException
     */
    private Map<Long, String> getThreadIdMap(SeekableByteChannel ch, ByteBuffer buffer) throws IOException{
        int capacity = buffer.capacity();

        buffer.position(capacity - 4);
        buffer.mark();
        ch.read(buffer);
        buffer.reset();

        int mapSize = buffer.getInt();
        Map<Long, String> result = new HashMap<>();
        
        IntStream.range(0, mapSize)
                 .forEach(i -> setThreadStatToMap(ch, buffer, result));
        
        return result;
    }

    /**
     * Get Thread Stat list from channel.
     * 
     * @param ch Channel to read.
     * @param buffer ByteBuffer to use.
     * @return Thread Stat list.
     * @throws IOException 
     */
    private List<ThreadStat> getThreadStatList(SeekableByteChannel ch, ByteBuffer buffer) throws IOException{
        List<ThreadStat> result = new ArrayList<>();
        buffer.flip();

        while(ch.read(buffer) != -1){
            buffer.flip();
            
            while(buffer.hasRemaining()){
                ThreadStat stat = new ThreadStat(buffer.getLong(), buffer.getLong(), buffer.getLong(), buffer.getLong());

                /* Reaches End Of Ring Buffer */
                if(stat.getEvent() == ThreadEvent.Unused){
                    break;
                }

                result.add(stat);
            }
            
            buffer.flip();
        }

        return result;
    }

    /**
     * Parse HeapStats Thread Recorder file.
     * 
     * @param path Path to recorder file.
     * @throws IOException 
     */
    public void parse(Path path) throws IOException{
        ByteBuffer buffer = ByteBuffer.allocateDirect(1024 * 1024); // 1MB

        try(SeekableByteChannel ch = Files.newByteChannel(path, StandardOpenOption.READ)){
            buffer.order(getByteOrder(ch, buffer));
            idMap = getThreadIdMap(ch, buffer);
            threadStatList = getThreadStatList(ch, buffer);
        }
        
        threadStatList = threadStatList.parallelStream()
                                       .sorted()
                                       .collect(Collectors.toList());

    }

    /**
     * Get Thread ID - Thread Name map.
     * @return Thread ID - Thread Name map.
     */
    public Map<Long, String> getIdMap() {
        return idMap;
    }

    /**
     * Get list of ThreadStat.
     * 
     * @return ThreadStat list.
     */
    public List<ThreadStat> getThreadStatList() {
        return threadStatList;
    }

}

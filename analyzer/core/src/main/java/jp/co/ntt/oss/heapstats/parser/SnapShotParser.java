/*
 * HeapStatsParser.java
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

import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.nio.channels.SeekableByteChannel;
import java.nio.file.FileSystems;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import jp.co.ntt.oss.heapstats.container.snapshot.ChildObjectData;
import jp.co.ntt.oss.heapstats.container.snapshot.ObjectData;
import jp.co.ntt.oss.heapstats.container.snapshot.SnapShotHeader;
import jp.co.ntt.oss.heapstats.parser.SnapShotParserEventHandler.ParseResult;

/**
 * This class loads a file java heap information.
 */
public class SnapShotParser {

    private final ByteBuffer longBuffer;
    
    private final ByteBuffer intBuffer;
    
    /**
     * Whether JNI style classname should be replaced.
     */
    private final boolean replace;

    /**
     * Constructor of SnapShotParser.
     * 
     * @param replace true if class name should be converted to Java-Style.
     */
    public SnapShotParser(boolean replace) {
        longBuffer = ByteBuffer.allocate(48);
        intBuffer = ByteBuffer.allocate(4);
        this.replace = replace;
    }
    
    /**
     * Setter of byte order.
     * This value is used in parsing of SnapShot file.
     * 
     * @param order byte order of this SnapShot file.
     */
    private void setByteOrder(ByteOrder order){
        longBuffer.order(order);
        intBuffer.order(order);
    }
    
    /**
     * Reader of long (8 bytes) value from channel.
     * 
     * @param ch Channel to be read.
     * @param size Read size.
     * @throws IOException 
     */
    private void readLong(SeekableByteChannel ch, int size) throws IOException{
        longBuffer.position(0);
        longBuffer.limit(size);
        ch.read(longBuffer);
        longBuffer.flip();
    }

    /**
     * Reader of int (4 bytes) value from channel.
     * 
     * @param ch Channel to be read.
     * @return read value.
     * @throws IOException 
     */
    private int readInt(SeekableByteChannel ch) throws IOException{
        ch.read(intBuffer);
        intBuffer.flip();
        int ret = intBuffer.getInt();
        intBuffer.flip();
        
        return ret;
    }
    
    /**
     * Parse single SnapShot.
     * @param header SnapShotHeader to parse.
     * @param handler SnapShot handler.
     * @return true if parsing is succeeded.
     * @throws IOException 
     */
    public boolean parseSingle(SnapShotHeader header, SnapShotParserEventHandler handler) throws IOException {

        try(FileInputStream stream = new FileInputStream(header.getSnapshotFile().toFile())) {
            FileChannel ch = stream.getChannel();
            ch.position(header.getFileOffset() + header.getSnapShotHeaderSize());
            setByteOrder(header.getByteOrderMark());

            handler.onStart(header.getFileOffset());

            if(handler.onNewSnapShot(header, header.getSnapshotFile().toString()) != ParseResult.HEAPSTATS_PARSE_CONTINUE){
                return false;
            }
                    
            if(parseElement(stream, header, handler) == ParseResult.HEAPSTATS_PARSE_ABORT){
                return false;
            }

            if(handler.onFinish(ch.position()) == ParseResult.HEAPSTATS_PARSE_ABORT){
                return false;
            }
                
        }

        return true;
    }

    /**
     * Parse HeapStats SnapShot file.
     *
     * @param fname the file java heap information.
     * @param handler the ParserEventHandler.
     * @return Return the Reading results file
     * @throws IOException If you load the file java heap information is not set
     *         ByteOrder agent if that occurs
     */
    public boolean parse(String fname, SnapShotParserEventHandler handler) throws IOException {

        try(FileInputStream stream = new FileInputStream(fname)) {
            SnapShotHeader header;
            FileChannel ch = stream.getChannel();
            
            while ((header = parseHeader(stream, fname)) != null) {
                handler.onStart(header.getFileOffset());

                if(handler.onNewSnapShot(header, fname) != ParseResult.HEAPSTATS_PARSE_CONTINUE){
                    return false;
                }
                    
                if(parseElement(stream, header, handler) == ParseResult.HEAPSTATS_PARSE_ABORT){
                    return false;
                }

                if(handler.onFinish(ch.position()) == ParseResult.HEAPSTATS_PARSE_ABORT){
                    return false;
                }
                
            }
            
        }

        return true;
    }

    /**
     * Extracting the header information of the snapshot.
     *
     * @param stream the file Java Heap Information.
     * @param fname Snapshot file name.
     * @return Return the SnapShotHeader
     * @throws IOException If other I / O error occurs.
     */
    protected SnapShotHeader parseHeader(FileInputStream stream, String fname) throws IOException {
        SnapShotHeader header = new SnapShotHeader();
        FileChannel ch = stream.getChannel();
        long startPos = ch.position();
        
        int ret = stream.read();

        if (ret == -1) {
          // EOF
          return null;
        }
        else if(((byte)ret == SnapShotHeader.FILE_FORMAT_1_0) ||
                ((byte)ret == SnapShotHeader.FILE_FORMAT_1_1) ||
                (((byte)ret & SnapShotHeader.EXTENDED_FORMAT) != 0)){
          // Heap
          header.setSnapShotType((byte)ret);
          header.setFileOffset(startPos);
          header.setSnapshotFile(FileSystems.getDefault().getPath(fname));
        }
        else{
          StringBuilder errString = new StringBuilder();
          errString.append("Unknown SnapShot Type! (");
          errString.append(ret);
          errString.append(")");
          throw new IOException(errString.toString());
        }

        ret = stream.read();

        switch (ret) {
            
            case 'L':
                header.setByteOrderMark(ByteOrder.LITTLE_ENDIAN);
                break;
                
            case 'B':
                header.setByteOrderMark(ByteOrder.BIG_ENDIAN);
                break;
                
            default:
                StringBuilder errString = new StringBuilder();
                errString.append("Unknown ByteOrderMark! (");
                errString.append(ret);
                errString.append(")");
                throw new IOException(errString.toString());
                
        }

        setByteOrder(header.getByteOrderMark());

        // SnapShot Date
        readLong(ch, 16);
        header.setSnapShotDateAsLong(longBuffer.getLong());
        // Entries
        header.setNumEntries(longBuffer.getLong());

        // SnapShot Cause
        header.setCause(readInt(ch));

        // GC Cause
        readLong(ch, 8);
        int len = (int)longBuffer.getLong();
        byte[] gcCause = new byte[len];
        if (stream.read(gcCause) != gcCause.length) {
            throw new IOException("Could not get the GC Cause.");
        }
        header.setGcCause(gcCause[0] == '\0' ? "-" : new String(gcCause));

        readLong(ch, 48);
        // Full GC Count
        header.setFullCount(longBuffer.getLong());

        // Young GC Count
        header.setYngCount(longBuffer.getLong());

        // GC Time
        header.setGcTime(longBuffer.getLong());

        // New Heap Size
        header.setNewHeap(longBuffer.getLong());

        // Old Heap Size
        header.setOldHeap(longBuffer.getLong());

        // Total Heap Size
        header.setTotalCapacity(longBuffer.getLong());

        if(header.hasMetaspaceData()){
          readLong(ch, 16);
          
          // Metaspace usage
          header.setMetaspaceUsage(longBuffer.getLong());

          // Metaspace capacity
          header.setMetaspaceCapacity(longBuffer.getLong());
        }
        
        header.setSnapShotHeaderSize(ch.position() - startPos);

        return header;
    }

    /**
     * Stored in a temporary file to extract the object information.
     *
     * @param stream the file Java Heap Information.
     * @param header the SnapShot header
     * @param handler ParserEventHandler
     * @return Return the Parse result.
     * @throws IOException If some other I/O error occurs
     */
    protected ParseResult parseElement(FileInputStream stream, SnapShotHeader header, SnapShotParserEventHandler handler) throws IOException {

        FileChannel ch = stream.getChannel();
        
        for (long i = 0; i < header.getNumEntries(); i++) {
            byte[] classNameInBytes;
            SnapShotParserEventHandler.ParseResult eventResult;
            ObjectData obj;
            obj = new ObjectData();
            
            readLong(ch, 16);

            // tag
            obj.setTag(longBuffer.getLong());

            // class Name
            classNameInBytes = new byte[(int)longBuffer.getLong()];
            if (stream.read(classNameInBytes) != classNameInBytes.length) {
                throw new IOException("Could not get the Class name.");
            }

            if (replace) {
                String tmp = new String(classNameInBytes)
                                  .replaceAll("^L|(^\\[*)L|;$", "$1")
                                  .replaceAll("(^\\[*)B$", "$1byte")
                                  .replaceAll("(^\\[*)C$", "$1char")
                                  .replaceAll("(^\\[*)I$", "$1int")
                                  .replaceAll("(^\\[*)S$", "$1short")
                                  .replaceAll("(^\\[*)J$", "$1long")
                                  .replaceAll("(^\\[*)D$", "$1double")
                                  .replaceAll("(^\\[*)F$", "$1float")
                                  .replaceAll("(^\\[*)V$", "$1void")
                                  .replaceAll("(^\\[*)Z$", "$1boolean");

                Pattern pattern = Pattern.compile("^\\[(.*)");
                Matcher matcher = pattern.matcher(tmp);
                while (matcher.find()) {
                    tmp = matcher.replaceAll("$1 []");
                    matcher = pattern.matcher(tmp);
                }

                obj.setName(tmp.replaceAll("\\/", "."));
            }
            else {
                obj.setName(new String(classNameInBytes));
            }

            if (header.getSnapShotType() != SnapShotHeader.FILE_FORMAT_1_0) {
                readLong(ch, 16);
                obj.setClassLoader(longBuffer.getLong());
                obj.setClassLoaderTag(longBuffer.getLong());
            }

            readLong(ch, 16);
            
            // instance
            obj.setCount(longBuffer.getLong());
            // heap usage
            obj.setTotalSize(longBuffer.getLong());

            eventResult = handler.onEntry(obj);

            if ((eventResult == ParseResult.HEAPSTATS_PARSE_CONTINUE) && header.hasReferenceData()) {
                eventResult = parseChildClass(obj.getTag(), ch,
                                         header.getByteOrderMark(), handler);
            }

            if (eventResult != ParseResult.HEAPSTATS_PARSE_CONTINUE) {
                return eventResult;
            }
            
        }

        return ParseResult.HEAPSTATS_PARSE_CONTINUE;
    }

    /**
     * Child class to extract information from a stream of snapshots.
     *
     * @param parentClassTag Tags parent class for uniquely identifying the
     *        parent class
     * @param ch FileChannel of Snapshot file.
     * @param byteOrder the byte order
     * @param handler ParserEventHandler
     * @return Return the Parse result.
     * @throws IOException If some other I/O error occurs
     */
    protected ParseResult parseChildClass(final long parentClassTag,
            final FileChannel ch, final ByteOrder byteOrder,
            final SnapShotParserEventHandler handler) throws IOException {

        while (true) {
            readLong(ch, 24);
            
            long childClassTag = longBuffer.getLong();
            long instances = longBuffer.getLong();
            long totalSize = longBuffer.getLong();

            if (childClassTag == -1) {
                return ParseResult.HEAPSTATS_PARSE_CONTINUE;
            }

            ParseResult result = handler.onChildEntry(parentClassTag,
                    new ChildObjectData(childClassTag, instances, totalSize));

            if (result != ParseResult.HEAPSTATS_PARSE_CONTINUE) {
                return result;
            }
            
        }
        
    }
    
}

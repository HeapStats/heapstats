/*
 * SnapShotHeader.java
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
package jp.co.ntt.oss.heapstats.container.snapshot;

import java.io.IOException;
import java.io.Serializable;
import java.io.UncheckedIOException;
import java.lang.ref.SoftReference;
import java.nio.ByteOrder;
import java.nio.file.Path;
import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import jp.co.ntt.oss.heapstats.parser.SnapShotParser;
import jp.co.ntt.oss.heapstats.parser.handler.SnapShotHandler;

/**
 * implements {@link Serializable} <br>
 * <br>
 * Contains the header information of the snapshot.Also, write to the temporary
 * file, do the reading.
 */
public class SnapShotHeader implements Comparable<SnapShotHeader>, Serializable {
    
    /**
     * HeapStats 1.0 SnapShot format.
     */
    public static final byte FILE_FORMAT_1_0 = 49;

    /**
     * HeapStats 1.1 SnapShot format.
     */
    public static final byte FILE_FORMAT_1_1 = 61;
    
    /**
     * Extended SnapShot format from 2.0 .
     */
    public static final byte EXTENDED_FORMAT = (byte)0b10000000;
    
    /**
     * Flag for reference data of extended SnapShot format.
     */
    public static final byte EXTENDED_FORMAT_FLAG_REFTREE = 0b00000001;
    
    /**
     * Flag for safepoint time of extended SnapShot format.
     */
    public static final byte EXTENDED_FORMAT_FLAG_SAFEPOINT_TIME = 0b00000010;

    /**
     * serialVersionUID.
     */
    static final long serialVersionUID = -539015033687122109L;

    /**
     * SnapShot Case GC.
     */
    public static final int SNAPSHOT_CAUSE_GC = 1;

    /**
     * SnapShot Case DUMP REQUEST.
     */
    public static final int SNAPSHOT_CAUSE_DATA_DUMP_REQUEST = 2;

    /**
     * SnapShot Case INTERVAL.
     */
    public static final int SNAPSHOT_CAUSE_INTERVAL = 3;
    
    /**
     * Byte order of the agent Dump file.
     */
    private ByteOrder byteOrderMark;

    /**
     * Time the snapshot was taken.
     */
    private LocalDateTime snapShotDate;

    /**
     * Number of live classes.
     */
    private long numEntries;

    /**
     * Number of Instances.
     */
    private long numInstances;

    /**
     * SnapShot Cause.
     */
    private int cause;

    /**
     * GC Cause.
     */
    private String gcCause;

    /**
     * Full GC Count.
     */
    private long fullCount;

    /**
     * Young GC Count.
     */
    private long yngCount;

    /**
     * GC Time.
     */
    private long gcTime;

    /**
     * New Heap Size.
     */
    private long newHeap;

    /**
     * old Heap Size.
     */
    private long oldHeap;

    /**
     * Total Heap Capacity.
     */
    private long totalCapacity;

    /**
     * Metaspace usage.
     */
    private long metaspaceUsage;

    /**
     * Metaspace capacity.
     */
    private long metaspaceCapacity;

    /**
     * Safepoint time.
     */
    private long safepointTime;

    private Path snapshotFile;

    private byte snapShotType;

    private long fileOffset;

    private long snapShotHeaderSize;

    private long snapShotSize;

    private SoftReference<Map<Long, ObjectData>> snapShotCache;

    /**
     * Creates a SnapShotHeader.
     */
    public SnapShotHeader() {
        byteOrderMark = ByteOrder.nativeOrder();
        snapShotDate = null;
        numEntries = 0;
        numInstances = 0;
        cause = 0;
        fullCount = 0;
        yngCount = 0;
        gcTime = 0;
        newHeap = 0;
        oldHeap = 0;
        totalCapacity = 0;
        metaspaceUsage = 0;
        metaspaceCapacity = 0;
        safepointTime = 0;
        snapShotCache = new SoftReference<>(null);
    }

    /**
     * Set the byte order.
     *
     * @param value ByteOrder
     */
    public final void setByteOrderMark(final ByteOrder value) {
        byteOrderMark = value;
    }

    /**
     * Return the byte order.
     *
     * @return Return the byte order
     */
    public final ByteOrder getByteOrderMark() {
        return byteOrderMark;
    }

    /**
     * Set the date and time the snapshot was taken.
     *
     * @param value Time the snapshot was taken
     */
    public final void setSnapShotDate(final LocalDateTime value) {
        snapShotDate = value;
    }

    /**
     * Set the date and time the snapshot was taken.
     *
     * @param value Time the snapshot was taken
     */
    public final void setSnapShotDateAsLong(final long value) {
        Instant inst = Instant.ofEpochMilli(value);
        snapShotDate = LocalDateTime.ofInstant(inst, ZoneId.systemDefault());
    }

    /**
     * To get the SnapShot Date.
     *
     * @return Return the SnapShot Date
     */
    public final LocalDateTime getSnapShotDate() {
        return snapShotDate;
    }

    /**
     * To set the Number of Object entries.
     *
     * @param value the Number of Object entries
     */
    public final void setNumEntries(final long value) {
        numEntries = value;
    }

    /**
     * To get the Number of Object entries.
     *
     * @return Return the Number of Object entries
     */
    public final long getNumEntries() {
        return numEntries;
    }

    /**
     * To get the Number of Instances.
     *
     * @return Return the Number of Instances.
     */
    public long getNumInstances() {
        return numInstances;
    }

    /**
     * To set the Number of Instances.
     *
     * @param numInstances the Number of Instances.
     */
    public void setNumInstances(long numInstances) {
        this.numInstances = numInstances;
    }

    /**
     * Set the source snapshot was taken.
     *
     * @param value Cause the snapshot was taken
     */
    public final void setCause(final int value) {
        cause = value;
    }

    /**
     * Return the Cause the snapshot was taken.
     *
     * @return Return the Cause the snapshot was taken
     */
    public final int getCause() {
        return cause;
    }

    /**
     * Get GC cause by string.
     * @return Return the GC Cause
     */
    public final String getCauseString() {
        switch (cause) {
            case SNAPSHOT_CAUSE_GC:
                return "GC";
            case SNAPSHOT_CAUSE_DATA_DUMP_REQUEST:
                return "DataDumpRequest";
            case SNAPSHOT_CAUSE_INTERVAL:
                return "Interval";
            default:
                return "Unknown";
        }
    }

    /**
     * Return Cause the GC.
     *
     * @return Return Cause the GC
     */
    public final String getGcCause() {
        return gcCause;
    }

    /**
     * Sets the cause of the GC.
     *
     * @param value Cause the GC
     */
    public final void setGcCause(final String value) {
        gcCause = value;
    }

    /**
     * Return the number of occurrences of FullGC.
     *
     * @return Return the number of occurrences of FullGC
     */
    public final long getFullCount() {
        return fullCount;
    }

    /**
     * Set the number of occurrences of FullGC.
     *
     * @param value number of occurrences of FullGC
     */
    public final void setFullCount(final long value) {
        fullCount = value;
    }

    /**
     * Return the number of occurrences of YoungGC.
     *
     * @return Return the number of occurrences of YoungGC
     */
    public final long getYngCount() {
        return yngCount;
    }

    /**
     * Set the number of occurrences of YoungGC.
     *
     * @param value number of occurrences of YoungGC
     */
    public final void setYngCount(final long value) {
        yngCount = value;
    }

    /**
     * Return the processing time of the GC.
     *
     * @return Return the processing time of the GC
     */
    public final long getGcTime() {
        return gcTime;
    }

    /**
     * Set the processing time of the GC.
     *
     * @param value the processing time of the GC
     */
    public final void setGcTime(final long value) {
        gcTime = value;
    }

    /**
     * Return the New heap space usage.
     *
     * @return Return the New heap space usage
     */
    public final long getNewHeap() {
        return newHeap;
    }

    /**
     * Set the heap space usage New.
     *
     * @param value the heap space usage New
     */
    public final void setNewHeap(final long value) {
        newHeap = value;
    }

    /**
     * Return the Old heap space usage.
     *
     * @return Return the Old heap space usage
     */
    public final long getOldHeap() {
        return oldHeap;
    }

    /**
     * Set the heap space usage Old.
     *
     * @param value the heap space usage Old
     */
    public final void setOldHeap(final long value) {
        oldHeap = value;
    }

    /**
     * Return the total heap usage.
     *
     * @return Return the total heap usage
     */
    public final long getTotalCapacity() {
        return totalCapacity;
    }

    /**
     * Sets the total heap usage.
     *
     * @param value the total heap usage
     */
    public final void setTotalCapacity(final long value) {
        totalCapacity = value;
    }

    /**
     * Return the Metaspace or PermGen usage.
     *
     * @return Return the Metaspace or PermGen usage
     */
    public final long getMetaspaceUsage() {
        return metaspaceUsage;
    }

    /**
     * Sets the metaspace or PermGen usage.
     *
     * @param value the Metaspace or PermGen usage
     */
    public final void setMetaspaceUsage(final long value) {
        metaspaceUsage = value;
    }

    /**
     * Return the Metaspace or PermGen capacity.
     *
     * @return Return the Metaspace or PermGen capacity
     */
    public final long getMetaspaceCapacity() {
        return metaspaceCapacity;
    }

    /**
     * Sets the metaspace or PermGen capacity.
     *
     * @param value the Metaspace or PermGen capacity
     */
    public final void setMetaspaceCapacity(final long value) {
        metaspaceCapacity = value;
    }

    /**
     * Safepoint time.
     *
     * @return Return accumulated safepoint time
     */
    public final long getSafepointTime() {
        return safepointTime;
    }

    /**
     * Set safepoint time.
     *
     * @param value safepoint time
     */
    public final void setSafepointTime(final long value) {
        safepointTime = value;
    }

    /**
     * Getter of SnapShot File.
     *
     * @return Path of this SnapShot.
     */
    public Path getSnapshotFile() {
        return snapshotFile;
    }

    /**
     * Setter of this SnapSHot.
     *
     * @param snapshotFile Path of this SnapShot.
     */
    public void setSnapshotFile(Path snapshotFile) {
        this.snapshotFile = snapshotFile;
    }

    /**
     * Get SnapShot type. This type is defined in HeapStatsParser:
     * FILE_FORMAT_NO_CHILD, FILE_FORMAT_HAVE_CHILD,
     * FILE_FORMAT_HAVE_CHILD_AND_METASPACE
     *
     * @return SnapShot Type
     */
    public int getSnapShotType() {
        return snapShotType;
    }

    /**
     * Set SnapShot type. This type is defined in HeapStatsParser:
     * FILE_FORMAT_NO_CHILD, FILE_FORMAT_HAVE_CHILD,
     * FILE_FORMAT_HAVE_CHILD_AND_METASPACE
     *
     * @param snapShotType SnapShot type.
     */
    public void setSnapShotType(byte snapShotType) {
        this.snapShotType = snapShotType;
    }

    /**
     * Getter of offset of this snapshot in file.
     *
     * @return Offset of this snapshot in file.
     */
    public long getFileOffset() {
        return fileOffset;
    }

    /**
     * Setter of offset of this snapshot in file.
     *
     * @param fileOffset Offset of this snapshot in file.
     */
    public void setFileOffset(long fileOffset) {
        this.fileOffset = fileOffset;
    }

    /**
     * Get Size of SnapShot header.
     *
     * @return header size.
     */
    public long getSnapShotHeaderSize() {
        return snapShotHeaderSize;
    }

    /**
     * Set size of SnapShot header.
     *
     * @param snapShotHeaderSize Size of SnapShot header.
     */
    public void setSnapShotHeaderSize(long snapShotHeaderSize) {
        this.snapShotHeaderSize = snapShotHeaderSize;
    }

    /**
     * Getter of ofset of this snapshot in file.
     *
     * @return Offset of this snapshot in file.
     */
    public long getSnapShotSize() {
        return snapShotSize;
    }

    /**
     * Setter of size of this snapshot.
     *
     * @param snapShotSize Size of this snapshot.
     */
    public void setSnapShotSize(long snapShotSize) {
        this.snapShotSize = snapShotSize;
    }

    /**
     * Get SnapShot data from file.
     *
     * @param needJavaStyle true if class name should be Java style, false means
     * JNI style.
     * @return SnapShot which is related to this header.
     */
    private Map<Long, ObjectData> getSnapShotDirectly(boolean needJavaStyle) {
        SnapShotHandler handler = new SnapShotHandler();
        SnapShotParser parser = new SnapShotParser(needJavaStyle);

        try {
            parser.parseSingle(this, handler);
        } catch (IOException ex) {
            throw new UncheckedIOException(ex);
        }

        Map<Long, ObjectData> result = handler.getSnapShot();
        setSnapShot(result);

        return result;
    }

    /**
     * Get SnapShot in this header.
     *
     * @param needJavaStyle true if class name should be Java style, false means
     * JNI style.
     * @return SnapShot in this header.
     */
    public Map<Long, ObjectData> getSnapShot(boolean needJavaStyle) {
        return Optional.ofNullable(snapShotCache.get())
                .orElseGet(() -> getSnapShotDirectly(needJavaStyle));
    }

    /**
     * Set SnapShot in this header. This SnapShot is managed as SoftReference.
     *
     * @param snapShot SnapShot to be managed.
     */
    public void setSnapShot(Map<Long, ObjectData> snapShot) {
        this.snapShotCache = new SoftReference<>(snapShot);
    }

    /**
     * Get true if this snapshot data has a reference data.
     * @return true if has a reference data.
     */
    public boolean hasReferenceData(){
        final byte extended_reftree = EXTENDED_FORMAT | EXTENDED_FORMAT_FLAG_REFTREE;
        
        return (snapShotType == FILE_FORMAT_1_1) ||
               ((snapShotType & extended_reftree) == extended_reftree);
    }
    /**
     * Get true if this snapshot data has a safepoint time.
     * @return true if has a safepoint time.
     */
    public boolean hasSafepointTime(){
        return (snapShotType & EXTENDED_FORMAT_FLAG_SAFEPOINT_TIME) == EXTENDED_FORMAT_FLAG_SAFEPOINT_TIME;
    }

    /**
     * Get true if this snapshot data has a metaspace data.
     * @return true if has a metaspace data.
     */
    public boolean hasMetaspaceData(){
        return (snapShotType != FILE_FORMAT_1_0);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public final String toString() {
        StringBuilder buf = new StringBuilder();

        buf.append("SnapShot at ");
        buf.append(snapShotDate);
        buf.append(", ");
        buf.append(numEntries);
        buf.append(" entries, ");
        buf.append(byteOrderMark);
        buf.append(" Caused by ");
        buf.append(getCauseString());
        buf.append(", GC Caused by ");
        buf.append(gcCause);
        buf.append(", Full GC ");
        buf.append(fullCount);
        buf.append(", Young GC ");
        buf.append(yngCount);
        buf.append(", GC Time at ");
        buf.append(gcTime);
        buf.append(", New Heap Usage ");
        buf.append(newHeap);
        buf.append(" byte ");
        buf.append(", Old Heap Usage ");
        buf.append(oldHeap);
        buf.append(" byte ");
        buf.append(", Total Heap Capacity ");
        buf.append(totalCapacity);
        buf.append(" byte ");
        buf.append(", Metaspace Usage ");
        buf.append(metaspaceUsage);
        buf.append(" byte ");
        buf.append(", Metaspace Capacity ");
        buf.append(metaspaceCapacity);
        buf.append(" byte ");
        buf.append(", Accumulated safepoint time ");
        buf.append(safepointTime);
        buf.append(" ms");

        return buf.toString();
    }

    /**
     * Compare function for SnapShotHeader. This method is based on
     * snapShotDate.
     *
     * @param o SnapHotHeader.
     * @return Compared result.
     */
    @Override
    public int compareTo(SnapShotHeader o) {
        return snapShotDate.compareTo(o.snapShotDate);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int hashCode() {
        return snapShotDate.hashCode();
    }

    /**
     * equals method of this SnapShotHeader. This method is based on
     * snapShotDate.
     *
     * @param obj Object
     * @return Compared result.
     */
    @Override
    public boolean equals(Object obj) {

        if (obj == null) {
            return false;
        }

        if (getClass() != obj.getClass()) {
            return false;
        }

        return Objects.equals(this.snapShotDate, ((SnapShotHeader) obj).snapShotDate);
    }

}

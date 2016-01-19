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

package jp.co.ntt.oss.heapstats.container.log;

import java.nio.file.Paths;
import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneId;

/**
 * Container class for log data.
 * @author Yasumasa Suenaga
 */
public class LogData implements Comparable<LogData>{

    public enum LogCause{
        EXHAUSTED,
        SIGNAL,
        INTERVAL,
        DEADLOCK,
        ILLEGAL
    }
    
    private LocalDateTime dateTime;
    
    private LogCause logCause;
    
    private long javaUserTime;
    
    private long javaSysTime;
    
    private long javaVSSize;
    
    private long javaRSSize;
    
    private long systemUserTime;
    
    private long systemNiceTime;
    
    private long systemSysTime;
    
    private long systemIdleTime;
    
    private long systemIOWaitTime;
    
    private long systemIRQTime;
    
    private long systemSoftIRQTime;
    
    private long systemStealTime;
    
    private long systemGuestTime;
    
    private long jvmSyncPark;
    
    private long jvmSafepointTime;
    
    private long jvmSafepoints;
    
    private long jvmLiveThreads;
    
    private String archivePath;
    
    /**
     * This method creates LogData from CSV.
     * 
     * @param csv CSV data to be parsed.
     * @param logdir Directory to deflate if parser finds log archive.
     * @throws IllegalArgumentException 
     */
    public void parseFromCSV(String csv, String logdir) throws IllegalArgumentException{
        String[] csvArray = csv.split(",");
        if((csvArray.length != 19) && (csvArray.length != 20)){
            throw new IllegalArgumentException("CSV data is not valid: " + csv);
        }
        
        Instant instant = Instant.ofEpochMilli(Long.parseLong(csvArray[0]));
        dateTime = LocalDateTime.ofInstant(instant, ZoneId.systemDefault());
        
        switch(Integer.parseInt(csvArray[1])){
            
            case 1:
                logCause = LogCause.EXHAUSTED;
                break;
                
            case 2:
                logCause = LogCause.SIGNAL;
                break;
                
            case 3:
                logCause = LogCause.INTERVAL;
                break;
                
            case 4:
                logCause = LogCause.DEADLOCK;
                break;
                
            default:
                logCause = LogCause.ILLEGAL;
                break;
        }
        
        javaUserTime = Long.parseUnsignedLong(csvArray[2]);
        javaSysTime  = Long.parseUnsignedLong(csvArray[3]);
        javaVSSize = Long.parseUnsignedLong(csvArray[4]);
        javaRSSize = Long.parseUnsignedLong(csvArray[5]);
        
        systemUserTime     = Long.parseUnsignedLong(csvArray[6]);
        systemNiceTime     = Long.parseUnsignedLong(csvArray[7]);
        systemSysTime      = Long.parseUnsignedLong(csvArray[8]);
        systemIdleTime     = Long.parseUnsignedLong(csvArray[9]);
        systemIOWaitTime   = Long.parseUnsignedLong(csvArray[10]);
        systemIRQTime      = Long.parseUnsignedLong(csvArray[11]);
        systemSoftIRQTime  = Long.parseUnsignedLong(csvArray[12]);
        systemStealTime    = Long.parseUnsignedLong(csvArray[13]);
        systemGuestTime    = Long.parseUnsignedLong(csvArray[14]);
        
        jvmSyncPark = Long.parseLong(csvArray[15]);
        jvmSafepointTime = Long.parseLong(csvArray[16]);
        jvmSafepoints = Long.parseLong(csvArray[17]);
        jvmLiveThreads = Long.parseLong(csvArray[18]);
        archivePath = (csvArray.length == 20) ? Paths.get(logdir, csvArray[19]).toString() : null;
    }

    /**
     * Get date time of this log data.
     * @return Date time of this log data
     */
    public LocalDateTime getDateTime() {
        return dateTime;
    }

    /**
     * Get cause of collecting this log data.
     * @return Cause of collecting this log data
     */
    public LogCause getLogCause() {
        return logCause;
    }

    /**
     * Get user time of java process.
     * @return User time of java process
     */
    public long getJavaUserTime() {
        return javaUserTime;
    }

    /**
     * Get system time of java process.
     * @return System time of java process.
     */
    public long getJavaSysTime() {
        return javaSysTime;
    }

    /**
     * Get virtual memory size (VSZ) of java process.
     * @return VSZ of java process.
     */
    public long getJavaVSSize() {
        return javaVSSize;
    }

    /**
     * Get resident set size (RSS) of java process.
     * @return RSS of java process.
     */
    public long getJavaRSSize() {
        return javaRSSize;
    }

    /**
     * Get user time of system.
     * @return User time of system.
     */
    public long getSystemUserTime() {
        return systemUserTime;
    }

    /**
     * Get nice time of system.
     * @return Nice time of system.
     */
    public long getSystemNiceTime() {
        return systemNiceTime;
    }

    /**
     * Get system time of system.
     * @return System time of system.
     */
    public long getSystemSysTime() {
        return systemSysTime;
    }

    /**
     * Get idle time of system.
     * @return Idle time of system.
     */
    public long getSystemIdleTime() {
        return systemIdleTime;
    }

    /**
     * Get IOWait time of system.
     * @return IOWait time of system.
     */
    public long getSystemIOWaitTime() {
        return systemIOWaitTime;
    }

    /**
     * Get IRQ time of system.
     * @return IRQ time of system.
     */
    public long getSystemIRQTime() {
        return systemIRQTime;
    }

    /**
     * Get soft IRQ time of system.
     * @return soft IRQ time of system.
     */
    public long getSystemSoftIRQTime() {
        return systemSoftIRQTime;
    }

    /**
     * Get steal time of system.
     * @return steal time of system.
     */
    public long getSystemStealTime() {
        return systemStealTime;
    }

    /**
     * Get guest time of system.
     * @return guest time of system.
     */
    public long getSystemGuestTime() {
        return systemGuestTime;
    }

    /**
     * Get park count of JVM.
     * @return Park count of JVM
     */
    public long getJvmSyncPark() {
        return jvmSyncPark;
    }

    /**
     * Get safepoint time of JVM.
     * @return Safepoint time of JVM.
     */
    public long getJvmSafepointTime() {
        return jvmSafepointTime;
    }

    /**
     * Get safepoint count of JVM.
     * @return Safepoint count of JVM.
     */
    public long getJvmSafepoints() {
        return jvmSafepoints;
    }

    /**
     * Get live Java threads in JVM.
     * @return Live Java threads in JVM.
     */
    public long getJvmLiveThreads() {
        return jvmLiveThreads;
    }

    /**
     * Get archive path when trouble occurrs.
     * @return Archive path.
     */
    public String getArchivePath() {
        return archivePath;
    }

    /**
     * This method compares with another LogData.
     * This method is based on dateTime field.
     * 
     * @param o
     * @return 
     */
    @Override
    public int compareTo(LogData o) {
        return dateTime.compareTo(o.dateTime);
    }

}

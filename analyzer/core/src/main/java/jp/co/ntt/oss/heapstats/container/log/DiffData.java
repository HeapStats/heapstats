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

import java.time.LocalDateTime;

/**
 * Container class for difference data (e.g. CPU usage, Safepoint time).
 * @author Yasumasa Suenaga
 */
public class DiffData {
    
    private final LocalDateTime dateTime;
    
    private double javaUserUsage;
    
    private double javaSysUsage;
    
    private double cpuUserUsage;
    
    private double cpuNiceUsage;
    
    private double cpuSysUsage;
    
    private double cpuIdleUsage;
    
    private double cpuIOWaitUsage;
    
    private double cpuIRQUsage;
    
    private double cpuSoftIRQUsage;
    
    private double cpuStealUsage;
    
    private double cpuGuestUsage;
    
    private long jvmSyncPark;
    
    private long jvmSafepointTime;
    
    private long jvmSafepoints;
    
    private final boolean minusData;
    
    /**
     * Constructor of DiffData.
     * Each fields is based on "current - prev" .
     * 
     * @param prev
     * @param current 
     */
    public DiffData(LogData prev, LogData current){
        dateTime = current.getDateTime();
        
        /* Java CPU usage */
        double javaUserTime = current.getJavaUserTime() - prev.getJavaUserTime();
        double javaSysTime = current.getJavaSysTime() - prev.getJavaSysTime();
        double javaCPUTotal = javaUserTime + javaSysTime;
        javaUserUsage = javaUserTime / javaCPUTotal * 100.0d;
        javaSysUsage = javaSysTime / javaCPUTotal * 100.0d;
        
        /* System CPU usage */
        double systemUserTime = current.getSystemUserTime() - prev.getSystemUserTime();
        double systemNiceTime = current.getSystemNiceTime() - prev.getSystemNiceTime();
        double systemSysTime = current.getSystemSysTime() - prev.getSystemSysTime();
        double systemIdleTime = current.getSystemIdleTime() - prev.getSystemIdleTime();
        double systemIOWaitTime = current.getSystemIOWaitTime() - prev.getSystemIOWaitTime();
        double systemIRQTime = current.getSystemIRQTime() - prev.getSystemIRQTime();
        double systemSoftIRQTime = current.getSystemSoftIRQTime() - prev.getSystemSoftIRQTime();
        double systemStealTime = current.getSystemStealTime() - prev.getSystemStealTime();
        double systemGuestTime = current.getSystemGuestTime() - prev.getSystemGuestTime();
        double systemCPUTotal = systemUserTime + systemNiceTime + systemSysTime +
                                systemIdleTime + systemIOWaitTime + systemIRQTime +
                                systemSoftIRQTime + systemStealTime + systemGuestTime;
        cpuUserUsage    = systemUserTime / systemCPUTotal * 100.0d;
        cpuNiceUsage    = systemNiceTime / systemCPUTotal * 100.0d;
        cpuSysUsage     = systemSysTime / systemCPUTotal * 100.0d;
        cpuIdleUsage    = systemIdleTime / systemCPUTotal * 100.0d;
        cpuIOWaitUsage  = systemIOWaitTime / systemCPUTotal * 100.0d;
        cpuIRQUsage     = systemIRQTime / systemCPUTotal * 100.0d;
        cpuSoftIRQUsage = systemSoftIRQTime / systemCPUTotal * 100.0d;
        cpuStealUsage   = systemStealTime / systemCPUTotal * 100.0d;
        cpuGuestUsage   = systemGuestTime / systemCPUTotal * 100.0d;
        
        /* JVM statistics */
        jvmSyncPark      = current.getJvmSyncPark() - prev.getJvmSyncPark();
        jvmSafepointTime = current.getJvmSafepointTime() - prev.getJvmSafepointTime();
        jvmSafepoints    = current.getJvmSafepoints() - prev.getJvmSafepoints();
        
        minusData = (systemUserTime < 0.0d) || (systemNiceTime < 0.0d) ||
                    (systemSysTime < 0.0d) || (systemIdleTime < 0.0d) ||
                    (systemIOWaitTime < 0.0d) || (systemIRQTime < 0.0d) ||
                    (systemSoftIRQTime < 0.0d) || (systemStealTime < 0.0d) ||
                    (systemGuestTime < 0.0d) ||
                    (jvmSyncPark < 0) || (jvmSafepointTime < 0) || (jvmSafepoints < 0);

        /*
         * If this data have minus entry, it is suspected reboot.
         * So I clear to all fields.
         */
        if(minusData){
            javaUserUsage   = 0.0d;
            javaSysUsage    = 0.0d;
            cpuUserUsage    = 0.0d;
            cpuNiceUsage    = 0.0d;
            cpuSysUsage     = 0.0d;
            cpuIdleUsage    = 0.0d;
            cpuIOWaitUsage  = 0.0d;
            cpuIRQUsage     = 0.0d;
            cpuSoftIRQUsage = 0.0d;
            cpuStealUsage   = 0.0d;
            cpuGuestUsage   = 0.0d;
            jvmSyncPark      = 0;
            jvmSafepointTime = 0;
            jvmSafepoints    = 0;
        }
        
    }

    /**
     * Get *current* log date time.
     * @return Date time of current log data.
     */
    public LocalDateTime getDateTime() {
        return dateTime;
    }

    /**
     * Get %user of java process.
     * 
     * @return %user
     */
    public double getJavaUserUsage() {
        return javaUserUsage;
    }

    /**
     * Get %sys of java process.
     * @return %sys
     */
    public double getJavaSysUsage() {
        return javaSysUsage;
    }

    /**
     * Get %user of system.
     * @return %user
     */
    public double getCpuUserUsage() {
        return cpuUserUsage;
    }

    /**
     * Get %nice of system.
     * @return %nice
     */
    public double getCpuNiceUsage() {
        return cpuNiceUsage;
    }

    /**
     * Get %sys of system.
     * @return %sys
     */
    public double getCpuSysUsage() {
        return cpuSysUsage;
    }

    /**
     * Get %idle of system.
     * @return %idle
     */
    public double getCpuIdleUsage() {
        return cpuIdleUsage;
    }

    /**
     * Get %iowait of system.
     * @return %iowait
     */
    public double getCpuIOWaitUsage() {
        return cpuIOWaitUsage;
    }

    /**
     * Get %irq of system.
     * @return %irq
     */
    public double getCpuIRQUsage() {
        return cpuIRQUsage;
    }

    /**
     * Get %soft of system.
     * @return %soft
     */
    public double getCpuSoftIRQUsage() {
        return cpuSoftIRQUsage;
    }

    /**
     * Get %steal of system.
     * @return %steal
     */
    public double getCpuStealUsage() {
        return cpuStealUsage;
    }

    /**
     * Get %guest of system.
     * @return %guest
     */
    public double getCpuGuestUsage() {
        return cpuGuestUsage;
    }

    /**
     * Get park count of java process.
     * @return Park count
     */
    public long getJvmSyncPark() {
        return jvmSyncPark;
    }

    /**
     * Get safepoint time of java process.
     * @return Safepoint time
     */
    public long getJvmSafepointTime() {
        return jvmSafepointTime;
    }

    /**
     * Get safepoint count of java process.
     * @return Safepoint count
     */
    public long getJvmSafepoints() {
        return jvmSafepoints;
    }
    
    /**
     * Get CPU usage of system.
     * @return CPU usage
     */
    public double getCpuTotalUsage(){
        return cpuUserUsage + cpuNiceUsage + cpuSysUsage + cpuIOWaitUsage +
               cpuIRQUsage + cpuSoftIRQUsage + cpuStealUsage + cpuGuestUsage;
    }

    /**
     * Get whether this difference has minus data.
     * If this method return true, JVM may reboot.
     * @return true if this difference has minus data.
     */
    public boolean hasMinusData(){
        return minusData;
    }
    
}

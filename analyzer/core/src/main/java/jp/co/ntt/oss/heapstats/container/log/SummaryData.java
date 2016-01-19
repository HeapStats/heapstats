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

import java.util.DoubleSummaryStatistics;
import java.util.List;

/**
 * Summary data class.<br/>
 * This class holds process summary information.
 * It shows at process summary table.
 * @author Yasumasa Suenaga
 */
public class SummaryData {
    
    public static class SummaryDataEntry{
        
        private String category;
        
        private String value;
        
        public SummaryDataEntry(String category, String value){
            this.category = category;
            this.value = value;
        }

        public String getCategory() {
            return category;
        }

        public void setCategory(String category) {
            this.category = category;
        }

        public String getValue() {
            return value;
        }

        public void setValue(String value) {
            this.value = value;
        }
        
    }
    
    private final double averageCPUUsage;
    
    private final double maxCPUUsage;
    
    private final double averageVSZ;
    
    private final double maxVSZ;
    
    private final double averageRSS;
    
    private final double maxRSS;
    
    private final double averageLiveThreads;
    
    private final long maxLiveThreads;
    
    /**
     * Constructor of SummaryData.
     * @param logData Log data list to be summarized.
     * @param diffData Difference data list to be summarized.
     */
    public SummaryData(List<LogData> logData, List<DiffData> diffData){
        DoubleSummaryStatistics cpuUsage = diffData.parallelStream()
                                                   .mapToDouble(d -> d.getCpuTotalUsage())
                                                   .summaryStatistics();
        averageCPUUsage = cpuUsage.getAverage();
        maxCPUUsage = cpuUsage.getMax();
        
        DiffSummaryStatistics diffSummary = logData.parallelStream()
                                                   .collect(DiffSummaryStatistics::new,
                                                            DiffSummaryStatistics::accept,
                                                            DiffSummaryStatistics::combine);
        
        averageVSZ = diffSummary.getAverageVSZ() / 1024.0d / 1024.0d; // in MB
        maxVSZ = diffSummary.getMaxVSZ() / 1024.0d / 1024.0d; // in MB
        
        averageRSS = diffSummary.getAverageRSS() / 1024.0d / 1024.0d; // in MB
        maxRSS = diffSummary.getMaxRSS() / 1024.0d / 1024.0d; // in MB

        averageLiveThreads = diffSummary.getAverageLiveThreads();
        maxLiveThreads = diffSummary.getMaxLiveThreads();
    }

    /**
     * Get average CPU usage of system.
     * @return Average of CPU usage
     */
    public double getAverageCPUUsage() {
        return averageCPUUsage;
    }

    /**
     * Get maximum CPU usage of system.
     * @return Maximum value of CPU usage
     */
    public double getMaxCPUUsage() {
        return maxCPUUsage;
    }

    /**
     * Get average virtual memoly size (VSZ) of java process.
     * @return Average of VSZ
     */
    public double getAverageVSZ() {
        return averageVSZ;
    }

    /**
     * Get maximum virtual memory size (VSZ) of java process.
     * @return Maximum value of VSZ
     */
    public double getMaxVSZ() {
        return maxVSZ;
    }

    /**
     * Get average resident set size (RSS) of java process.
     * @return Average of RSS
     */
    public double getAverageRSS() {
        return averageRSS;
    }

    /**
     * Get maximum resident set size (RSS) of java process.
     * @return Maximum value of RSS
     */
    public double getMaxRSS() {
        return maxRSS;
    }

    /**
     * Get average live Java thread count.
     * @return Average of Java live thread
     */
    public double getAverageLiveThreads() {
        return averageLiveThreads;
    }

    /**
     * Get maximum live Java thread count.
     * @return Maximum value of Java live thread
     */
    public long getMaxLiveThreads() {
        return maxLiveThreads;
    }
    
    /**
     * Statistics class for SnapSHot diff calculation.
     */
    private class DiffSummaryStatistics{
        
        private long count;
        
        private long vsz;
        
        private long maxVSZ;
        
        private long rss;
        
        private long maxRSS;
        
        private long liveThreads;
        
        private long maxLiveThreads;
        
        public DiffSummaryStatistics(){
            count = 0;
            vsz = 0;
            maxVSZ = 0;
            rss = 0;
            maxRSS = 0;
            liveThreads = 0;
            maxLiveThreads = 0;
        }
        
        public void accept(LogData logData){
            count++;
            
            vsz += logData.getJavaVSSize();
            maxVSZ = Math.max(maxVSZ, logData.getJavaVSSize());
            
            rss += logData.getJavaRSSize();
            maxRSS = Math.max(maxRSS, logData.getJavaRSSize());
            
            liveThreads += logData.getJvmLiveThreads();
            maxLiveThreads = Math.max(maxLiveThreads, logData.getJvmLiveThreads());
        }
        
        public void combine(DiffSummaryStatistics other){
            count += other.count;
            
            vsz += other.vsz;
            maxVSZ = Math.max(maxVSZ, other.maxVSZ);
            
            rss += other.rss;
            maxRSS = Math.max(maxRSS, other.maxRSS);
            
            liveThreads += other.liveThreads;
            maxLiveThreads = Math.max(maxLiveThreads, other.maxLiveThreads);
        }
        
        public double getAverageVSZ(){
            return (double)vsz / (double)count;
        }
        
        public long getMaxVSZ(){
            return maxVSZ;
        }
        
        public double getAverageRSS(){
            return (double)rss / (double)count;
        }
        
        public long getMaxRSS(){
            return maxRSS;
        }
        
        public double getAverageLiveThreads(){
            return (double)liveThreads / (double)count;
        }
        
        public long getMaxLiveThreads(){
            return maxLiveThreads;
        }
        
    }
    
}

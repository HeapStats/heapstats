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

package jp.co.ntt.oss.heapstats.container.snapshot;

import java.time.LocalDateTime;
import java.util.Optional;

/**
 * Difference data between two snapshots.
 * 
 * @author Yasumasa Suenaga
 */
public class DiffData {
    
    private final long tag;
    
    private final LocalDateTime diffDate;
    
    private final String className;
    
    private final String classLoaderName;
    
    private final long instances;
    
    private final long totalSize;
    
    private final boolean ranked;

    /**
     * Constructor of DiffData.
     * 
     * @param diffDate Date time of this difference data.
     * @param prev Previous data.
     * @param current Current data.
     * @param isRanked true if this object data is ranked in.
     */
    public DiffData(LocalDateTime diffDate, ObjectData prev, ObjectData current, boolean isRanked) {
        Optional<ObjectData> optPrev = Optional.ofNullable(prev);
        
        this.tag = current.getTag();
        this.diffDate = diffDate;
        this.className = current.getName();
        this.classLoaderName = current.getLoaderName();
        this.instances = current.getCount() - optPrev.map(o -> o.getCount()).orElse(0L);
        this.totalSize = current.getTotalSize() - optPrev.map(o -> o.getTotalSize()).orElse(0L);
        this.ranked = isRanked;
    }

    public DiffData(long tag, LocalDateTime diffDate, String className, String classLoaderName, Long instances, long totalSize, boolean ranked) {
        this.tag = tag;
        this.diffDate = diffDate;
        this.className = className;
        this.classLoaderName = classLoaderName;
        this.instances = instances;
        this.totalSize = totalSize;
        this.ranked = ranked;
    }

    /**
     * Get date time of this difference data.
     * @return Date time of this difference data.
     */
    public LocalDateTime getDiffDate() {
        return diffDate;
    }

    /**
     * Get class name of this difference data.
     * @return Class name
     */
    public String getClassName() {
        return className;
    }

    /**
     * Get class loader name of this difference data.
     * @return ClassLoader name
     */
    public String getClassLoaderName() {
        return classLoaderName;
    }

    /**
     * Get instance count of this difference data.
     * @return Instance count
     */
    public long getInstances() {
        return instances;
    }

    /**
     * Get total size of this difference data.
     * @return Total size
     */
    public long getTotalSize() {
        return totalSize;
    }

    /**
     * Get object tag of this difference data.
     * @return Object tag
     */
    public long getTag() {
        return tag;
    }

    /**
     * Get whether this object data is ranked in.
     * @return true if this object data is ranked in.
     */
    public boolean isRanked() {
        return ranked;
    }

}

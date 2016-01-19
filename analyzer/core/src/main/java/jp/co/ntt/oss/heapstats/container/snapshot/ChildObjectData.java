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

/**
 * This class represents child object data.
 * 
 * @author Yasumasa Suenaga
 */
public class ChildObjectData {
    
    private final long tag;
    
    private final long instances;
    
    private final long totalSize;

    /**
     * Constructor of Child data.
     * 
     * @param tag Child class tag.
     * @param instances Number of instances of this child class.
     * @param totalSize Total size of this child class.
     */
    public ChildObjectData(long tag, long instances, long totalSize) {
        this.tag = tag;
        this.instances = instances;
        this.totalSize = totalSize;
    }

    /**
     * Get tag of this child class.
     * 
     * @return Tag of this child class.
     */
    public long getTag() {
        return tag;
    }

    /**
     * Get number of instances of this child class.
     * @return Number of instances.
     */
    public long getInstances() {
        return instances;
    }

    /**
     * Get total size of this child class.
     * 
     * @return Total size of this child class.
     */
    public long getTotalSize() {
        return totalSize;
    }
    
}

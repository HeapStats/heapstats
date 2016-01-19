/*
 * ObjectData.java
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

import java.io.Serializable;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.StringJoiner;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * implements {@link Serializable}, {@link Cloneable} <br>
 * <br>
 * Contains information about the classes loaded in the java virtual machine.
 */
public class ObjectData implements Serializable, Cloneable {
    /** serialVersionUID. */
    private static final long serialVersionUID = -1685786847473848152L;

    /** Holds the tag to uniquely identify the class that is loaded. */
    private long tag;

    /** Class Name. */
    private String name;

    /** Holds the ID for uniquely identifying the ClassLoader. */
    private long classLoader;

    /** Holds the class tag that indicates the ClassLoader. */
    private long classLoaderTag;

    /** Instances of classes. */
    private long count;

    /** Total heap usage class. */
    private long totalSize;

    /** The name of the ClassLoader. */
    private String loaderName;
    
    /** List of Child Object */
    private List<ChildObjectData> referenceList;

    /**
     * Create a ObjectData.
     */
    public ObjectData() {
        tag = 0;
        name = null;
        classLoader = -1;
        classLoaderTag = -1;
        count = 0;
        totalSize = 0;
        this.loaderName = null;
        referenceList = null;
    }

    public ObjectData(long tag, String name, long classLoader, long classLoaderTag, long count, long totalSize, String loaderName, List<ChildObjectData> referenceList) {
        this.tag = tag;
        this.name = name;
        this.classLoader = classLoader;
        this.classLoaderTag = classLoaderTag;
        this.count = count;
        this.totalSize = totalSize;
        this.loaderName = loaderName;
        this.referenceList = referenceList;
    }

    /**
     * Sets the tag.
     *
     * @param value tag
     */
    public final void setTag(final long value) {
        tag = value;
    }

    /**
     * Return the tag.
     *
     * @return Return the tag
     */
    public final long getTag() {
        return tag;
    }

    /**
     * Sets the Class path.
     *
     * @param value Class path
     */
    public final void setName(final String value) {
        name = value;
    }

    /**
     * Return the Class Name.
     *
     * @return Return the Class Name
     */
    public final String getName() {
        return name;
    }

    /**
     * Return the class loader ID.
     *
     * @return Return the class loader ID
     */
    public final long getClassLoader() {
        return classLoader;
    }

    /**
     * Set the class loader ID.
     *
     * @param value class loader ID
     */
    public final void setClassLoader(final long value) {
        this.classLoader = value;
    }

    /**
     * Returns the unique tag of the class loader.
     *
     * @return Returns the unique tag of the class loader.
     */
    public final long getClassLoaderTag() {
        return classLoaderTag;
    }

    /**
     * Set the unique tag of the class loader.
     *
     * @param value unique tag
     */
    public final void setClassLoaderTag(final long value) {
        this.classLoaderTag = value;
    }

    /**
     * Sets the Object count.
     *
     * @param value Number of Classes
     */
    public final void setCount(final long value) {
        count = value;
    }

    /**
     * Return the Object count.
     *
     * @return Return the Object count
     */
    public final long getCount() {
        return count;
    }

    /**
     * Sets the total heap size.
     *
     * @param value total heap size
     */
    public final void setTotalSize(final long value) {
        totalSize = value;
    }

    /**
     * Return the total heap size.
     *
     * @return Return the total heap size
     */
    public final long getTotalSize() {
        return totalSize;
    }

    /**
     * Returns the name of the class loader.
     *
     * @return Returns the name of the class loader
     */
    public final String getLoaderName() {
        return loaderName;
    }

    /**
     * Set the name of the class loader.
     *
     * @param value the name of the class loader.
     */
    public final void setLoaderName(final String value) {
        loaderName = value;
    }
    
    /**
     * Setter method of class loader name.
     * 
     * @param snapShotData SnapShot data map. Key is object tag. Value is ObjectData.
     */
    public void setLoaderName(Map<Long, ObjectData> snapShotData){
        
        if(classLoaderTag < 0){
            loaderName = "-";
        }
        else if(classLoaderTag == 0){
            loaderName = "<SystemClassLoader>";
        }
        else{
            String loaderClass =  Optional.ofNullable(snapShotData.get(classLoaderTag))
                                          .map(o -> o.name)
                                          .orElse("<Unknown>");
            loaderName = String.format("%s (0x%x)", loaderClass, classLoader);
        }
        
    }

    /**
     * Get reference list.
     * 
     * @return Reference list. This list represents referrer of this ObjectData.
     */
    public List<ChildObjectData> getReferenceList() {
        return referenceList;
    }

    /**
     * Setter of reference list.
     * 
     * @param referenceList New reference list.
     */
    public void setReferenceList(List<ChildObjectData> referenceList) {
        this.referenceList = referenceList;
    }

    @Override
    public final String toString() {
        return (new StringJoiner(",")).add(Long.toHexString(tag))
                                      .add(name)
                                      .add(Long.toHexString(classLoader))
                                      .add(Long.toHexString(classLoaderTag))
                                      .add(Long.toString(count))
                                      .add(Long.toString(totalSize))
                                      .toString();
    }

    @Override
    public final ObjectData clone() {
        
        try {
            super.clone();
        } catch (CloneNotSupportedException ex) {
            Logger.getLogger(ObjectData.class.getName()).log(Level.SEVERE, null, ex);
        }

        ObjectData cloneObj = new ObjectData();

        cloneObj.setTag(tag);
        cloneObj.setName(name);
        cloneObj.setClassLoader(classLoader);
        cloneObj.setClassLoaderTag(classLoaderTag);
        cloneObj.setCount(count);
        cloneObj.setTotalSize(totalSize);
        cloneObj.setLoaderName(loaderName);
        cloneObj.setReferenceList(referenceList);

        return cloneObj;
    }

    /**
     * Returns hash code of this ObjectData.
     * This value is calculated from tag.
     * 
     * @return hash code of tag.
     */
    @Override
    public int hashCode() {
        return Long.valueOf(tag).hashCode();
    }

    /**
     * Equals method of this ObjectData.
     * This method is based on Tag.
     * 
     * @param obj
     * @return true if object equals.
     */
    @Override
    public boolean equals(Object obj) {
        
        if (obj == null) {
            return false;
        }
        
        if (getClass() != obj.getClass()) {
            return false;
        }
        
        return this.tag == ((ObjectData)obj).tag;
    }

}

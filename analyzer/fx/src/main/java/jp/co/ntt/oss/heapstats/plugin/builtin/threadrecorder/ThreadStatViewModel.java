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
package jp.co.ntt.oss.heapstats.plugin.builtin.threadrecorder;

import javafx.beans.property.BooleanProperty;
import javafx.beans.property.ReadOnlyObjectProperty;
import javafx.beans.property.ReadOnlyObjectWrapper;
import javafx.beans.property.SimpleBooleanProperty;
import jp.co.ntt.oss.heapstats.container.threadrecord.ThreadStat;

import java.time.LocalDateTime;
import java.util.List;

/**
 * Thread Status Data Model for presentation.
 * 
 * @author Yasumasa Suenaga
 */
public class ThreadStatViewModel {
    
    private final BooleanProperty show;
    
    private long id;
    
    private String name;

    private LocalDateTime startTime;

    private LocalDateTime endTime;

    private final ReadOnlyObjectWrapper<List<ThreadStat>> threadStats;

    /**
     * Constructor of ThreadStatViewModel
     * 
     * @param id Thread ID
     * @param name Thread Name
     * @param startTime Start time
     * @param endTime End time
     * @param threadStats List of ThreadStat to draw.
     */
    public ThreadStatViewModel(long id, String name, LocalDateTime startTime, LocalDateTime endTime,
            List<ThreadStat> threadStats) {
        this.id = id;
        this.name = name;
        this.startTime = startTime;
        this.endTime = endTime;
        this.threadStats = new ReadOnlyObjectWrapper<>(threadStats);
        
        if((threadStats == null) || threadStats.isEmpty()){
            this.show = new SimpleBooleanProperty(false);
        }
        else{
            this.show = new SimpleBooleanProperty(true);
        }
        
    }
    
    /**
     * Get show property.
     * 
     * @return show property.
     */
    public BooleanProperty showProperty(){
        return show;
    }

    /**
     * Get Thread ID.
     * 
     * @return Thread ID.
     */
    public long getId() {
        return id;
    }

    /**
     * Set Thread ID.
     * @param id New Thread ID.
     */
    public void setId(long id) {
        this.id = id;
    }

    /**
     * Get Thread name.
     * @return Thread name.
     */
    public String getName() {
        return name;
    }

    /**
     * Set Thread name.
     * @param name New Thread name.
     */
    public void setName(String name) {
        this.name = name;
    }

    /**
     * Get Start time of Thread Recorder events.
     * 
     * @return Start time.
     */
    public LocalDateTime getStartTime() {
        return startTime;
    }

    /**
     * Set Start time of Thread Recorder events.
     * 
     * @param startTime New start time.
     */
    public void setStartTime(LocalDateTime startTime) {
        this.startTime = startTime;
    }

    /**
     * Get End time of Thread Recorder events.
     * 
     * @return End time.
     */
    public LocalDateTime getEndTime() {
        return endTime;
    }

    /**
     * Set End time of Thread Recorder events.
     * 
     * @param endTime New end time.
     */
    public void setEndTime(LocalDateTime endTime) {
        this.endTime = endTime;
    }

    /**
     * Get list of ThreadStat.
     * 
     * @return ThreadStat list.
     */
    public List<ThreadStat> getThreadStats() {
        return threadStats.get();
    }

    /**
     * Get threadStats property.
     * @return threadStats property.
     */
    public ReadOnlyObjectProperty<List<ThreadStat>> threadStatsProperty() {
        return threadStats.getReadOnlyProperty();
    }

}

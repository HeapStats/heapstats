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
package jp.co.ntt.oss.heapstats.container.threadrecord;

import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;

/**
 * Thread status holder.
 * 
 * @author Yasumasa Suenaga
 */
public class ThreadStat implements Comparable<ThreadStat>{

    public static enum ThreadEvent{
        Unused,
        ThreadStart,
        ThreadEnd,
        MonitorWait,
        MonitorWaited,
        MonitorContendedEnter,
        MonitorContendedEntered,
        ThreadSleepStart,
        ThreadSleepEnd,
        Park,
        Unpark,
        FileWriteStart,
        FileWriteEnd,
        FileReadStart,
        FileReadEnd,
        SocketWriteStart,
        SocketWriteEnd,
        SocketReadStart,
        SocketReadEnd
    }
  
    private final LocalDateTime time;

    private final long id;

    private final ThreadEvent event;

    private final long additionalData;

    /**
     * Constructor of ThreadStat.
     * 
     * @param longTime Date time from UNIX epoch in MS.
     * @param id Thread ID.
     * @param longEvent Event value of ThreadEvent.
     * @param additionalData Additional information like size of reading, etc.
     */
    public ThreadStat(long longTime, long id, long longEvent, long additionalData){
      time = LocalDateTime.ofInstant(Instant.ofEpochMilli(longTime), ZoneId.systemDefault());
      this.id = id;
      event = ThreadEvent.values()[(int)longEvent];
      this.additionalData = additionalData;
    }

    /**
     * Get date time of this event.
     * @return Date time of this event.
     */
    public LocalDateTime getTime(){
        return time;
    }

    /**
     * Get Thread ID of this event.
     * @return Thread ID of this event.
     */
    public long getId(){
        return id;
    }

    /**
     * Get this event.
     * @return This event.
     */
    public ThreadEvent getEvent(){
        return event;
    }

    /**
     * Get additional information.
     * @return Additional information.
     */
    public long getAdditionalData() {
        return additionalData;
    }

    @Override
    public int compareTo(ThreadStat o) {
        return time.compareTo(o.time);
    }

    @Override
    public String toString() {
        return String.format("%d: %s: %s (%d)", id, time.format(DateTimeFormatter.ISO_LOCAL_DATE_TIME), event.toString(), additionalData);
    }
    
}

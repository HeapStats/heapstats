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
package jp.co.ntt.oss.heapstats.task;

import java.util.Optional;
import java.util.function.Consumer;

/**
 * Adapter class for adding progress notification to Runnable interface.
 * @author Yasumasa Suenaga
 */
public abstract class ProgressRunnable implements Runnable{
    
    private long total;
    
    protected Optional<Consumer<Long>> updateProgress;
    
    /**
     * Constructor of ProgressRunnable.
     */
    public ProgressRunnable(){
        this.total = 0;
        this.updateProgress = Optional.empty();
    }

    /**
     * Get total steps in this task.
     * @return Total steps
     */
    public long getTotal() {
        return total;
    }

    /**
     * Set total steps in this task.
     * @param total Total steps
     */
    public void setTotal(long total) {
        this.total = total;
    }
    
    /**
     * Set progress notificator.
     * @param updateProgress Progress notificator
     */
    public void setUpdateProgress(Consumer<Long> updateProgress){
        this.updateProgress = Optional.ofNullable(updateProgress);
    }
    
}

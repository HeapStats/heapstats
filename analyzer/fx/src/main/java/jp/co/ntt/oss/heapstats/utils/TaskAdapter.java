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
package jp.co.ntt.oss.heapstats.utils;

import javafx.application.Platform;
import javafx.concurrent.Task;
import jp.co.ntt.oss.heapstats.task.ProgressRunnable;

/**
 * Adapter class for Task in Java FX.
 * This class adapt ProgressRunnable to Java FX Void Task.
 * 
 * @author Yasumasa Suenaga
 * @param <T> Implementation of ProgressRunnable
 */
public class TaskAdapter<T extends ProgressRunnable> extends Task<Void>{
    
    private final T task;
    
    /**
     * Constructor of TaskAdapter.
     * 
     * @param task Instance of ProgressRunnable
     */
    public TaskAdapter(T task){
        this.task = task;
        this.task.setUpdateProgress(p -> updateProgress(p, task.getTotal()));
    }

    /**
     * Get ProgressRunnable instance which is related to this task.
     * @return Instance of ProgressRunnable
     */
    public T getTask() {
        return task;
    }
    
    @Override
    protected Void call() throws Exception {
        
        try{
            task.run();
        }
        catch(Exception e){
            Platform.runLater(() -> HeapStatsUtils.showExceptionDialog(e));
        }
        
        return null;
    }
    
}

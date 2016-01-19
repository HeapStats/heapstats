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

package jp.co.ntt.oss.heapstats.plugin;

import java.net.URL;
import java.util.Map;
import java.util.ResourceBundle;
import javafx.beans.value.ChangeListener;
import javafx.concurrent.Task;
import javafx.event.Event;
import javafx.event.EventHandler;
import javafx.fxml.Initializable;
import javafx.scene.control.ProgressIndicator;
import javafx.scene.layout.Region;
import jp.co.ntt.oss.heapstats.WindowController;

/**
 * Base class for HeapStats FX Analyzer plugin.
 * 
 * @author Yasumasa Suenaga
 */
public abstract class PluginController implements Initializable{
    
    /** License indication for GPLv2 */
    public static final String LICENSE_GPL_V2 = "GNU General Public License version 2";
    
    /** License indication for BSD License */
    public static final String LICENSE_BSD = "Berkeley Software Distribution License";

    private Region veil;
    
    private ProgressIndicator progress;
    
    public abstract String getPluginName();
    
    /**
     * Getter of license of this plugin.
     * 
     * @return License of this plugin.
     */
    public abstract String getLicense();
    
    /**
     * Getter of license map which is used by this plugin.
     * Key is library name, value is license of library.
     * 
     * @return License of libraryes.
     */
    public abstract Map<String, String> getLibraryLicense();
    
    /**
     * Event handler when tab of this plugin is selected.
     * 
     * @return Event handler of this plugin.
     */
    public abstract EventHandler<Event> getOnPluginTabSelected();
   
    /**
     * Event andler when main window is closed.
     * 
     * @return Event handler of this plugin.
     */
    public abstract Runnable getOnCloseRequest();

    /**
     * Setter of veil region.
     * This region is used for veiling (e.g. showing progress)
     * 
     * @param veil 
     */
    public void setVeil(Region veil){
        this.veil = veil;
    }
    
    /**
     * Setter of progress indicator.
     * This region is used for veiling (e.g. showing progress)
     * 
     * @param progress
     */
    public void setProgress(ProgressIndicator progress){
        this.progress = progress;
    }
    
    @Override
    public void initialize(URL location, ResourceBundle resources) {
    }
    
    /**
     * Task binder.
     * This method binds veil and progress indicator to task.
     * 
     * @param task Task to be binded.
     */
    public void bindTask(Task task){
        veil.visibleProperty().bind(task.runningProperty());
        progress.visibleProperty().bind(task.runningProperty());
        progress.progressProperty().bind(task.progressProperty());
    }
 
    public void setOnWindowResize(ChangeListener<? super Number> event){
        WindowController.getInstance().getOwner().widthProperty().addListener(event);
        WindowController.getInstance().getOwner().heightProperty().addListener(event);
    }
    
    /** 
     * Set data to another plugin. 
     * This method will be overrided by each plugins. 
     *  
     * @param data Data to set. 
     * @param select Plugin tab will be actived if this value is true. 
     */ 
    public void setData(Object data, boolean select){ 
        if(select){ 
            WindowController.getInstance().selectTab(getPluginName()); 
        } 
    } 

    /**
     * This class represents license of libraries which are used by each plugin.
     */
    public static class LibraryLicense{
        
        private final String pluginName;
        
        private final String libraryName;
        
        private final String license;

        public LibraryLicense(String pluginName, String libraryName, String license) {
            this.pluginName = pluginName;
            this.libraryName = libraryName;
            this.license = license;
        }

        public String getPluginName() {
            return pluginName;
        }

        public String getLibraryName() {
            return libraryName;
        }

        public String getLicense() {
            return license;
        }
        
    }

}

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
package jp.co.ntt.oss.heapstats.plugin.builtin.jvmlive.mbean;

import javafx.beans.property.BooleanProperty;
import javafx.beans.property.Property;
import javafx.beans.property.SimpleBooleanProperty;
import javafx.beans.property.SimpleLongProperty;
import javafx.beans.property.SimpleObjectProperty;
import javafx.beans.property.SimpleStringProperty;
import javafx.beans.property.StringProperty;
import javafx.scene.control.Control;

/**
 * Container class of HeapStats agent configuration.
 * 
 * @author Yasumasa Suenaga
 */
public class HeapStatsConfig {
    
    private final StringProperty key;
    
    private final Property value;
    
    private final Object currentValue;
    
    private Control cellContent;
    
    private final BooleanProperty changed;
    
    /**
     * Constructor of HeapStatsConfig.
     * 
     * @param key Configuration key.
     * @param value Configuration value.
     */
    @SuppressWarnings("unchecked")
    public HeapStatsConfig(String key, Object value){
        this.key = new SimpleStringProperty(key);
        changed = new SimpleBooleanProperty();
        
        if(value == null){
            this.value = new SimpleStringProperty(null);
            this.currentValue = null;
        }
        else if(value instanceof Boolean){
            this.value = new SimpleBooleanProperty((Boolean)value);
            this.currentValue = (Boolean)value;
        }
        else if(value instanceof Long){
            this.value = new SimpleLongProperty((Long)value);
            this.currentValue = (Long)value;
        }
        else if(value instanceof String){
            this.value = new SimpleStringProperty((String)value);
            this.currentValue = (String)value;
        }
        else{
            this.value = new SimpleObjectProperty<>(value);
            this.currentValue = value;
        }
        
        this.value.addListener((v, o, n) -> changed.set(!n.equals(currentValue)));
        this.cellContent = null;
    }
    
    /**
     * Get configuration key.
     * 
     * @return Configuration key.
     */
    public StringProperty keyProperty(){
        return key;
    }
    
    /**
     * Get configuration value.
     * 
     * @return Configuration value.
     */
    public Property valueProperty(){
        return value;
    }

    /**
     * Get change property.
     * true if this configuration is changed.
     * 
     * @return Change property.
     */
    public BooleanProperty changedProperty(){
        return changed;
    }

    /**
     * Get table cell control of this configuration.
     * 
     * @return Control in TableCell.
     */
    public Control getCellContent() {
        return cellContent;
    }

    /**
     * Set table cell control of this configuration.
     * 
     * @param cellContent New control in TableCell.
     */
    public void setCellContent(Control cellContent) {
        this.cellContent = cellContent;
    }

}

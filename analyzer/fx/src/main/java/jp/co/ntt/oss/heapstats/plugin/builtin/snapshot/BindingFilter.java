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
package jp.co.ntt.oss.heapstats.plugin.builtin.snapshot;

import javafx.beans.property.BooleanProperty;
import javafx.beans.property.SimpleBooleanProperty;
import jp.co.ntt.oss.heapstats.xml.binding.Filter;

/**
 * JAXB binding class for exclude filter.
 * This class will be used in JavaFX application.
 * 
 * @author Yasumasa Suenaga
 */
public class BindingFilter extends Filter{
    
    private final BooleanProperty hideProp;
    
    private final BooleanProperty applied;
    
    public BindingFilter(Filter f){
        setName(f.getName());
        setClasses(f.getClasses());
        hideProp = new SimpleBooleanProperty(f.isHide());
        applied = new SimpleBooleanProperty(false);
    }
    
    public BooleanProperty hideProperty(){
        return hideProp;
    }

    public BooleanProperty appliedProperty(){
        return applied;
    }

    @Override
    public void setHide(boolean visible) {
        super.setHide(visible);
        hideProp.set(visible);
    }

    @Override
    public boolean isHide() {
        return hideProp.get();
    }
    
}

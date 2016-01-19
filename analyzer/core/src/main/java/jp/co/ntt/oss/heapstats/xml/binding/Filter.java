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

package jp.co.ntt.oss.heapstats.xml.binding;

import javax.xml.bind.annotation.XmlAttribute;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlTransient;

/**
 * Filter class in XML filter.
 * This class will use in JAXB.
 * 
 * @author Yasumasa Suenaga
 */
public class Filter {
    
    @XmlAttribute(name="name")
    private String name;
    
    @XmlElement(name="visible")
    private boolean hide;
    
    @XmlElement(name="classes")
    private Classes classes;

    public Filter() {
        hide = false;
    }
    
    @XmlTransient
    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    @XmlTransient
    public Classes getClasses() {
        return classes;
    }
    
    public void setClasses(Classes classes) {
        this.classes = classes;
    }

    @XmlTransient
    public boolean isHide() {
        return hide;
    }
    
    public void setHide(boolean visible) {
        this.hide = visible;
    }

}

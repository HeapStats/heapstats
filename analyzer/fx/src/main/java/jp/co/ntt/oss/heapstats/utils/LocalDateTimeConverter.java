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

package jp.co.ntt.oss.heapstats.utils;

import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import javafx.util.StringConverter;

/**
 * StringConverter for LocalDateTime. <br/>
 * This class is used at JavaFX controls.
 * 
 * @author Yasumasa Suenaga
 */
public class LocalDateTimeConverter extends StringConverter<LocalDateTime>{
    
    private static final DateTimeFormatter formatter;
    
    static{
        formatter = DateTimeFormatter.ofPattern("yyyy/MM/dd HH:mm:ss");
    }
    
    @Override
    public String toString(LocalDateTime object) {
        return formatter.format(object);
    }

    @Override
    public LocalDateTime fromString(String string) {
        return LocalDateTime.parse(string, formatter);
    }
    
}

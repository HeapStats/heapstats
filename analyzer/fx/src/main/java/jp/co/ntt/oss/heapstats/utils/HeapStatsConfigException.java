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

/**
 * Exception class when config loader encounters error.
 * This exception shows that user sets invalid value into configuration file (heapstats.properties) .
 * @author Yasumasa Suenaga
 */
public class HeapStatsConfigException extends Exception{
    
    public HeapStatsConfigException(String message){
        super(message);
    }
    
    public HeapStatsConfigException(String message, Throwable cause){
        super(message, cause);
    }
    
}

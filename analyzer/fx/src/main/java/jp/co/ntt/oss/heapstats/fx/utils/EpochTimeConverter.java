/*
 * Copyright (C) 2016 Yasumasa Suenaga
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
package jp.co.ntt.oss.heapstats.fx.utils;

import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneId;
import javafx.util.StringConverter;
import jp.co.ntt.oss.heapstats.api.utils.HeapStatsUtils;

/**
 * String converter for epoch time.
 * This class supports SECOND time unit ONLY.
 * 
 * @author Yasumasa Suenaga
 */
public class EpochTimeConverter extends StringConverter<Number>{

    @Override
    public String toString(Number object) {
        return LocalDateTime.ofInstant(Instant.ofEpochSecond(object.longValue()), ZoneId.systemDefault()).format(HeapStatsUtils.getDateTimeFormatter());
    }

    @Override
    public Number fromString(String string) {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }
    
}

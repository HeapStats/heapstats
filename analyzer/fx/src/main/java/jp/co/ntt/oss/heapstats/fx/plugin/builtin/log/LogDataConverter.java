/*
 * Copyright (C) 2014-2019 Yasumasa Suenaga
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
package jp.co.ntt.oss.heapstats.fx.plugin.builtin.log;

import javafx.util.StringConverter;
import jp.co.ntt.oss.heapstats.container.log.LogData;
import jp.co.ntt.oss.heapstats.api.utils.HeapStatsUtils;

/**
 * StringConverter for LogData. This class provides method to convert
 * LocalDateTime in LogData to String.
 *
 * @author Yasumasa Suenaga.
 */

public class LogDataConverter extends StringConverter<LogData> {

    @Override
    public String toString(LogData object) {
        return object.getDateTime().format(HeapStatsUtils.getDateTimeFormatter());
    }

    /**
     * This class DO NOT support this method.
     *
     * @param string String
     * @return UnsupportedOperationException This class cannot convert from string.
     */
    @Override
    public LogData fromString(String string) {
        throw new UnsupportedOperationException("LogData DO NOT convert from String.");
    }

}

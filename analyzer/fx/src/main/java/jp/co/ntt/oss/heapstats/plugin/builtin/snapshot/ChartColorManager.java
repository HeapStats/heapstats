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

import java.util.HashMap;
import java.util.Map;

/**
 * Color management class for chart and table cell.
 *
 * @author Yasumasa Suenaga
 */
public class ChartColorManager {

    private static final String[] chartColorArray;

    private static int colorIndex;

    private static final Map<String, String> colorCache;

    static {
        String colors[] = {"aquamarine", "bisque", "blueviolet", "brown", "blue", "chartreuse", "coral",
            "cornflowerblue", "crimson", "darkcyan", "darkgoldenrod", "darkgreen", "darkkhaki",
            "darkmagenta", "darkorange", "darksalmon", "darkseagreen", "deeppink", "dodgerblue",
            "gold", "green", "orangered", "orchid", "plum", "red", "sandybrown", "slateblue",
            "lime", "tomato", "turquoise", "violet", "orange"};

        chartColorArray = colors;
        colorIndex = 0;
        colorCache = new HashMap<>();
    }

    /**
     * Get next color from sequence. If color which relates className has been
     * returned, this method returns same color.
     *
     * @param className Class name to get.
     * @return Color string which relates className.
     */
    public static String getNextColor(String className) {
        String result = colorCache.computeIfAbsent(className, k -> chartColorArray[colorIndex++]);

        if (colorIndex == chartColorArray.length) {
            colorIndex = 0;
        }

        return result;
    }

    /**
     * Get class color from color cache.
     *
     * @param className Class name to get.
     * @return Color string which relates className. transparent if className
     * does not exist in cache.
     */
    public static String getCurrentColor(String className) {
        return colorCache.getOrDefault(className, "transparent");
    }

}

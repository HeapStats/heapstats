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
package jp.co.ntt.oss.heapstats.container.snapshot;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;
import java.util.OptionalInt;

/**
 * Summary data class.<br/>
 * This class holds process summary information. It shows at process summary
 * table.
 *
 * @author Yasu
 */
public class SummaryData {

    public static class SummaryDataEntry {

        private String category;

        private String value;

        public SummaryDataEntry(String category, String value) {
            this.category = category;
            this.value = value;
        }

        public String getCategory() {
            return category;
        }

        public void setCategory(String category) {
            this.category = category;
        }

        public String getValue() {
            return value;
        }

        public void setValue(String value) {
            this.value = value;
        }

    }

    private final int count;

    private final long fullCount;

    private final long yngCount;

    private final long latestHeapUsage;

    private final long latestMetaspaceUsage;

    private final long maxGCTime;

    private final long maxSnapshotSize;

    private final long maxEntryCount;

    private final List<LocalDateTime> rebootSuspectList;

    /**
     * Constructor of SummaryData.
     *
     * @param headers List of SnapShot headers which should be summarized.
     */
    public SummaryData(List<SnapShotHeader> headers) {
        rebootSuspectList = new ArrayList<>();
        OptionalInt lastRebootIndex = OptionalInt.empty();

        for (int idx = 1; idx < headers.size(); idx++) {
            SnapShotHeader before = headers.get(idx - 1);
            SnapShotHeader after = headers.get(idx);

            if ((before.getCause() == SnapShotHeader.SNAPSHOT_CAUSE_GC) && (after.getCause() == SnapShotHeader.SNAPSHOT_CAUSE_GC)
                    && ((after.getYngCount() - before.getYngCount() <= 0) && (after.getFullCount() - before.getFullCount() <= 0)
                    || (after.getFullCount() - before.getFullCount() <= 0))) {
                /*
                 * SnapShot Data graphs show the diff of each snapshots. So, HeapStats need
                 * to set the xAxis of "before" to draw the reboot time correctly.
                 */
                rebootSuspectList.add(before.getSnapShotDate());
                lastRebootIndex = OptionalInt.of(idx);
            }
        }

        count = headers.size();

        SnapShotHeader lastRebootStart = headers.get(lastRebootIndex.orElse(0));
        SnapShotHeader end = headers.get(count - 1);

        fullCount = end.getFullCount() - lastRebootStart.getFullCount() + 1;
        yngCount = end.getYngCount() - lastRebootStart.getYngCount() + 1;

        latestHeapUsage = end.getNewHeap() + end.getOldHeap();
        latestMetaspaceUsage = end.getMetaspaceUsage();

        MaxSummaryStatistics statistics = headers.parallelStream()
                .collect(MaxSummaryStatistics::new,
                        MaxSummaryStatistics::accept,
                        MaxSummaryStatistics::combine);
        maxGCTime = statistics.getMaxGCTime();
        maxSnapshotSize = statistics.getMaxSnapshotSize();
        maxEntryCount = statistics.getMaxEntryCount();
    }

    /**
     * Get count of SnapShot headers.
     *
     * @return SnapShot count
     */
    public int getCount() {
        return count;
    }

    /**
     * Get count of FullGC.
     *
     * @return FullGC count
     */
    public long getFullCount() {
        return fullCount;
    }

    /**
     * Get count of YoungGC.
     *
     * @return YoungGC count
     */
    public long getYngCount() {
        return yngCount;
    }

    /**
     * Get Java heap usage in last SnapShot.
     *
     * @return Java heap usage
     */
    public long getLatestHeapUsage() {
        return latestHeapUsage;
    }

    /**
     * Get metaspace usage in last SnapShot.
     *
     * @return Metaspace usage
     */
    public long getLatestMetaspaceUsage() {
        return latestMetaspaceUsage;
    }

    /**
     * Get maximum value of GC time.
     *
     * @return Maximum value of GC time
     */
    public long getMaxGCTime() {
        return maxGCTime;
    }

    /**
     * Get maximum value of SnapShot size.
     *
     * @return Maximum value of SnapShot size
     */
    public long getMaxSnapshotSize() {
        return maxSnapshotSize;
    }

    /**
     * Get maximum value of entries in SnapShot.
     *
     * @return Maximum value of SnapShot entries
     */
    public long getMaxEntryCount() {
        return maxEntryCount;
    }

    /**
     * Get list of reboot suspection timing.
     *
     * @return list of reboot suspection timing.
     */
    public List<LocalDateTime> getRebootSuspectList() {
        return rebootSuspectList;
    }

    private class MaxSummaryStatistics {

        private long maxGCTime;

        private long maxSnapshotSize;

        private long maxEntryCount;

        public MaxSummaryStatistics() {
            maxGCTime = 0;
            maxSnapshotSize = 0;
            maxEntryCount = 0;
        }

        public void accept(SnapShotHeader header) {
            maxGCTime = Math.max(maxGCTime, header.getGcTime());
            maxSnapshotSize = Math.max(maxSnapshotSize, header.getSnapShotSize());
            maxEntryCount = Math.max(maxEntryCount, header.getNumEntries());
        }

        public void combine(MaxSummaryStatistics other) {
            maxGCTime = Math.max(maxGCTime, other.maxGCTime);
            maxSnapshotSize = Math.max(maxSnapshotSize, other.maxSnapshotSize);
            maxEntryCount = Math.max(maxEntryCount, other.maxEntryCount);
        }

        public long getMaxGCTime() {
            return maxGCTime;
        }

        public long getMaxSnapshotSize() {
            return maxSnapshotSize;
        }

        public long getMaxEntryCount() {
            return maxEntryCount;
        }

    }

}

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
package jp.co.ntt.oss.heapstats.task;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.function.Predicate;
import java.util.stream.Collectors;
import jp.co.ntt.oss.heapstats.container.snapshot.ObjectData;
import jp.co.ntt.oss.heapstats.container.snapshot.SnapShotHeader;
import jp.co.ntt.oss.heapstats.container.snapshot.DiffData;

/**
 * Task thread implementation for calculating difference data.
 *
 * @author Yasumasa Suenaga
 */
public class DiffCalculator extends ProgressRunnable {

    private final List<SnapShotHeader> snapShots;

    private final Map<LocalDateTime, List<ObjectData>> topNList;

    private final List<DiffData> lastDiffList;

    private final int rankLevel;

    private final boolean includeOthers;

    private final Optional<Predicate<? super ObjectData>> filter;

    private final boolean needJavaStyle;

    private final boolean isInstanceGraph;

    private long progressCounter;
    
    /**
     * Constructor of DiffCalcurator.
     * 
     * @param snapShots SnapShot list to calculate.
     * @param rankLevel Rank (TopN) level to calculate.
     * @param includeOthers true if result should be included TopN and "Others" data.
     * @param filter Class filter.
     * @param needJavaStyle true if class name should be Java-style FQCN. false means JNI-style.
     */
    public DiffCalculator(List<SnapShotHeader> snapShots, int rankLevel, boolean includeOthers,
            Predicate<? super ObjectData> filter, boolean needJavaStyle) {
        this(snapShots, rankLevel, includeOthers, filter, needJavaStyle, false);
    }

    /**
     * Constructor of DiffCalcurator.
     * 
     * @param snapShots SnapShot list to calculate.
     * @param rankLevel Rank (TopN) level to calculate.
     * @param includeOthers true if result should be included TopN and "Others" data.
     * @param filter Class filter.
     * @param needJavaStyle true if class name should be Java-style FQCN. false means JNI-style.
     * @param isInstanceGraph true if analyzer shows instance based graphs.
     */
    public DiffCalculator(List<SnapShotHeader> snapShots, int rankLevel, boolean includeOthers,
            Predicate<? super ObjectData> filter, boolean needJavaStyle, boolean isInstanceGraph) {
        this.snapShots = snapShots;
        this.topNList = new HashMap<>();
        this.lastDiffList = new ArrayList<>();
        this.rankLevel = rankLevel;
        this.includeOthers = includeOthers;
        this.filter = Optional.ofNullable(filter);
        this.needJavaStyle = needJavaStyle;
        this.isInstanceGraph = isInstanceGraph;

        setTotal(this.snapShots.size());
    }

    /**
     * Build TopN data from givien snapshot header.
     *
     * @param header SnapShot header to build.
     */
    private void buildTopNData(SnapShotHeader header) {
        List<ObjectData> topNBuffer = header.getSnapShot(needJavaStyle)
                .values()
                .parallelStream()
                .filter(filter.orElse(o -> true))
                .sorted(Comparator.comparingLong(isInstanceGraph ?
                        ObjectData::getCount : ObjectData::getTotalSize).reversed())
                .limit(rankLevel)
                .collect(Collectors.toList());

        if (includeOthers) {
            ObjectData other = new ObjectData();
            other.setName("Others");

            if (isInstanceGraph) {
                other.setCount(
                        header.getNumInstances() -
                        topNBuffer.stream().mapToLong(d -> d.getCount()).sum()
                );
            } else {
                other.setTotalSize(header.getNewHeap() + header.getOldHeap() - topNBuffer.stream()
                    .mapToLong(d -> d.getTotalSize())
                    .sum());
            }
            topNBuffer.add(other);
        }
        topNList.put(header.getSnapShotDate(), topNBuffer);
        progressCounter++;
        updateProgress.ifPresent(c -> c.accept(progressCounter));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run() {
        progressCounter = 0;

        /* Calculate top N data */
        snapShots.stream()
                .forEachOrdered(h -> buildTopNData(h));

        List<Long> rankedTagList = topNList.values().stream()
                .flatMap(c -> c.stream())
                .mapToLong(o -> o.getTag())
                .filter(t -> t != 0L)
                .distinct()
                .boxed()
                .collect(Collectors.toList());

        /* Calculate summarize diff */
        SnapShotHeader startHeader = snapShots.get(0);
        SnapShotHeader endHeader = snapShots.get(snapShots.size() - 1);

        Map<Long, ObjectData> start = startHeader.getSnapShot(needJavaStyle);
        Map<Long, ObjectData> end = endHeader.getSnapShot(needJavaStyle);
        start.forEach((k, v) -> end.putIfAbsent(k, new ObjectData(k, v.getName(), v.getClassLoader(), v.getClassLoaderTag(), 0, 0, v.getLoaderName(), null)));

        if (filter.isPresent()) {
            end.entrySet().stream()
                    .filter(e -> filter.get().test(e.getValue()))
                    .map(e -> new DiffData(endHeader.getSnapShotDate(), start.get(e.getKey()), e.getValue(), rankedTagList.contains(e.getValue().getTag())))
                    .forEach(d -> lastDiffList.add(d));
        } else {
            end.forEach((k, v) -> lastDiffList.add(new DiffData(endHeader.getSnapShotDate(), start.get(k), v, rankedTagList.contains(v.getTag()))));
        }

    }

    /**
     * Get TopN list which is a result of this task.
     * @return TopN list.
     */
    public Map<LocalDateTime, List<ObjectData>> getTopNList() {
        return topNList;
    }

    /**
     * Get diff data - Top of list and tail of list.
     * @return Diff Data.
     */
    public List<DiffData> getLastDiffList() {
        return lastDiffList;
    }

}

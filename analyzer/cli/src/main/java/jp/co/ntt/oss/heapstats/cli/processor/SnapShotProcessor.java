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
package jp.co.ntt.oss.heapstats.cli.processor;

import java.time.format.DateTimeFormatter;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.OptionalInt;
import java.util.StringJoiner;
import java.util.stream.Collectors;
import java.util.stream.IntStream;
import java.util.stream.Stream;
import jp.co.ntt.oss.heapstats.cli.Options;
import jp.co.ntt.oss.heapstats.container.snapshot.DiffData;
import jp.co.ntt.oss.heapstats.container.snapshot.ObjectData;
import jp.co.ntt.oss.heapstats.container.snapshot.SnapShotHeader;
import jp.co.ntt.oss.heapstats.snapshot.ReferenceTracker;
import jp.co.ntt.oss.heapstats.task.CSVDumpGC;
import jp.co.ntt.oss.heapstats.task.CSVDumpHeap;
import jp.co.ntt.oss.heapstats.task.DiffCalculator;
import jp.co.ntt.oss.heapstats.task.ParseHeader;

/**
 * CLI task for SnapShot.
 * @author Yasumasa Suenaga
 */
public class SnapShotProcessor implements CliProcessor{
    
    private final Options options;
    
    /**
     * Constructor of SnapShotProcessor.
     * 
     * @param options Options to use.
     */
    public SnapShotProcessor(Options options){
        this.options = options;
    }
    
    /**
     * Show SnapShot summary.
     * @param header SnapShot header.
     */
    private void showSnapShotSummary(SnapShotHeader header){
        System.out.println(header.getSnapShotDate().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME) + ": Cause: " + header.getCauseString());
        System.out.println("GC Cause: " + header.getGcCause() + ", Full: " + header.getFullCount() + ", Young: " + header.getYngCount() + ", GC Time: " + header.getGcTime() + "ms");
        System.out.printf("Java heap: capacity: %dMB, new: %dMB, old: %dMB\n", header.getTotalCapacity() / 1024 / 1024, header.getNewHeap() / 1024 / 1024, header.getOldHeap() / 1024 / 1024);
        System.out.printf("Metaspace: capacity: %dMB, usage: %dMB\n", header.getMetaspaceCapacity() / 1024 / 1024, header.getMetaspaceUsage() / 1024 / 1024);
        System.out.println("Total instances: " + header.getNumInstances() + ", Total entries: " + header.getNumEntries());
        System.out.println("---------------------------------------------");
        System.out.println("Tag\tClass\tClassLoader\tInstances\tSize(KB)");
        
        Stream<ObjectData> stream = header.getSnapShot(true).values().stream();
        if(options.getFilterPredicate() != null){
            stream = stream.filter(options.getFilterPredicate());
        }
        
        stream.sorted((o1, o2) -> Long.compare(o2.getTotalSize(), o1.getTotalSize()))
              .limit(5)
              .map(o -> (new StringJoiner("\t")).add("0x" + Long.toHexString(o.getTag()))
                                                .add(o.getName())
                                                .add(o.getLoaderName())
                                                .add(Long.toString(o.getCount()))
                                                .add(Long.toString(o.getTotalSize() / 1024))
                                                .toString())
              .forEachOrdered(System.out::println);
        System.out.println();
    }
    
    /**
     * Show class histogram.
     * @param header SnapShot header.
     */
    private void showClassHistogram(SnapShotHeader header){
        System.out.println(header.getSnapShotDate().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME));
        System.out.println("Tag\tClass\tClassLoader\tInstances\tSize(KB)");
        
        Stream<ObjectData> stream = header.getSnapShot(true).values().stream();
        if(options.getFilterPredicate() != null){
            stream = stream.filter(options.getFilterPredicate());
        }
        
        stream.sorted((o1, o2) -> Long.compare(o2.getTotalSize(), o1.getTotalSize()))
              .map(o -> (new StringJoiner("\t")).add("0x" + Long.toHexString(o.getTag()))
                                                .add(o.getName())
                                                .add(o.getLoaderName())
                                                .add(Long.toString(o.getCount()))
                                                .add(Long.toString(o.getTotalSize() / 1024))
                                                .toString())
              .forEachOrdered(System.out::println);
    }

    /**
     * Show different data.
     * @param from Before header.
     * @param to After header.
     */
    private void showDiffHistogram(SnapShotHeader from, SnapShotHeader to){
        List<SnapShotHeader> diffTarget = Arrays.asList(from, to);
        DiffCalculator diffCalc = new DiffCalculator(diffTarget, 0, false, options.getFilterPredicate(), true);
        diffCalc.run();
        
        System.out.println("DiffData of " + from.getSnapShotDate().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME) + " - " + to.getSnapShotDate().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME));
        System.out.println("Tag\tClass\tClassLoader\tInstances\tSize(KB)");
        
        diffCalc.getLastDiffList().stream()
                                  .sorted(Comparator.comparingLong(DiffData::getTotalSize).reversed())
                                  .map(d -> (new StringJoiner("\t")).add("0x" + Long.toHexString(d.getTag()))
                                                                    .add(d.getClassName())
                                                                    .add(d.getClassLoaderName())
                                                                    .add(Long.toString(d.getInstances()))
                                                                    .add(Long.toString(d.getTotalSize() / 1024))
                                                                    .toString())
                                  .forEachOrdered(System.out::println);
    }

    /**
     * Show class reference.
     * @param header SnapShot header.
     */
    private void showClassReference(SnapShotHeader header){
        long refStart = options.getRefStartTag();
        Map<Long, ObjectData> snapShot = header.getSnapShot(true);
        
        System.out.println(header.getSnapShotDate().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME));
        System.out.println("Start: " + snapShot.get(refStart).getName());
        System.out.println("Direction: " + (options.isRefToParent() ? "Parent" : "Child"));
        System.out.println("\tTag\tClass\tClassLoader\tInstances\tSize(KB)");
        
        ReferenceTracker refTracker = new ReferenceTracker(snapShot, OptionalInt.empty(), Optional.ofNullable(options.getFilterPredicate()));
        
        List<ObjectData> objectList = options.isRefToParent() ? refTracker.getParents(refStart, true)
                                                              : refTracker.getChildren(refStart, true);
        objectList.stream()
                  .map(o -> (new StringJoiner("\t")).add("\t")
                                                    .add("0x" + Long.toHexString(o.getTag()))
                                                    .add(o.getName())
                                                    .add(o.getLoaderName())
                                                    .add(Long.toString(o.getCount()))
                                                    .add(Long.toString(o.getTotalSize() / 1024))
                                                    .toString())
                  .forEachOrdered(System.out::println);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void process() {
        ParseHeader headerParser = new ParseHeader(options.getFile().stream()
                                                                    .map(p -> p.toString())
                                                                    .collect(Collectors.toList()), true, false);
        headerParser.run();
        List<SnapShotHeader> snapShots = headerParser.getSnapShotList();
        int start = options.getStart().orElse(0);
        
        if(options.isShowId()){
            IntStream.range(start, options.getEnd().orElse(snapShots.size()))
                     .mapToObj(i -> String.format("%d: %s", i, snapShots.get(i).getSnapShotDate().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME)))
                     .forEachOrdered(System.out::println);
        }
        else{
            Stream<SnapShotHeader> snapshotStream = IntStream.range(start, options.getEnd().orElse(snapShots.size()))
                                                             .mapToObj(i -> snapShots.get(i));
            
            switch(options.getMode()){
                case SNAPSHOT_SUMMARY:
                    snapshotStream.forEachOrdered(this::showSnapShotSummary);
                    break;
                case CLASS_HISTO:
                    snapshotStream.forEachOrdered(this::showClassHistogram);
                    break;
                case DIFF_HISTO:
                    System.out.println(options.getEnd().orElse(snapShots.size() - 1));
                    showDiffHistogram(snapShots.get(start), snapShots.get(options.getEnd().orElse(snapShots.size()) - 1));
                    break;
                case CLASS_REFERENCES:
                    snapshotStream.forEachOrdered(this::showClassReference);
                    break;
                case HEAP_CSV:
                    CSVDumpHeap heapDumper = new CSVDumpHeap(options.getCsvFile(), snapshotStream.collect(Collectors.toList()), options.getFilterPredicate(), true);
                    heapDumper.run();
                    break;
                case GC_CSV:
                    CSVDumpGC csvDumper = new CSVDumpGC(options.getCsvFile(), snapshotStream.collect(Collectors.toList()));
                    csvDumper.run();
                    break;
            }
            
        }
        
    }
    
}

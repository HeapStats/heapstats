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

import java.io.File;
import java.time.format.DateTimeFormatter;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.IntStream;
import jp.co.ntt.oss.heapstats.cli.Options;
import jp.co.ntt.oss.heapstats.container.log.ArchiveData;
import jp.co.ntt.oss.heapstats.container.log.DiffData;
import jp.co.ntt.oss.heapstats.container.log.LogData;
import jp.co.ntt.oss.heapstats.lambda.ConsumerWrapper;
import jp.co.ntt.oss.heapstats.task.ParseLogFile;

/**
 * CLI task for CSV resource log file.
 * @author Yasumasa Suenaga
 */
public class LogProcessor implements CliProcessor{
    
    private final Options options;
    
    private final ParseLogFile parser;
    
    /**
     * Constructor of LogProcessor.
     * 
     * @param options Options to use.
     */
    public LogProcessor(Options options){
        this.options = options;
        this.parser = new ParseLogFile(options.getFile().stream()
                                                        .map(p -> p.toFile())
                                                        .collect(Collectors.toList()), false);
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public void process(){
        parser.run();
        List<LogData> logEntries = parser.getLogEntries();
        List<DiffData> diffEntries = parser.getDiffEntries();
        int start = options.getStart().orElse(0);
        
        if(options.isShowId()){
            IntStream.range(start, options.getEnd().orElse(logEntries.size()))
                     .mapToObj(i ->String.format("%d: %s", i, logEntries.get(i).getDateTime().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME)))
                     .forEachOrdered(System.out::println);
        }
        else{
            
            switch(options.getMode()){
                case JAVA_CPU:
                    System.out.println("Java CPU:");
                    System.out.println("date time,  %user,  %sys");
                    IntStream.range(start, options.getEnd().orElse(diffEntries.size()))
                             .mapToObj(i -> diffEntries.get(i))
                             .forEach(d -> System.out.println(String.format("%s: %.2f %.2f", d.getDateTime().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME), d.getJavaUserUsage(), d.getJavaSysUsage())));
                    break;
                case SYSTEM_CPU:
                    System.out.println("System CPU:");
                    System.out.println("date time,  %user,  %nice,  %sys,  %iowait,  %irq,  %softirq,  %steal,  %guest,  %idle");
                    IntStream.range(start, options.getEnd().orElse(diffEntries.size()))
                             .mapToObj(i -> diffEntries.get(i))
                             .forEach(d -> System.out.println(String.format("%s: %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f", d.getDateTime().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME),
                                                                                                                                d.getCpuUserUsage(), d.getCpuNiceUsage(), d.getCpuSysUsage(),
                                                                                                                                d.getCpuIOWaitUsage(), d.getCpuIRQUsage(), d.getCpuSoftIRQUsage(),
                                                                                                                                d.getCpuStealUsage(), d.getCpuGuestUsage(), d.getCpuIdleUsage())));
                    break;
                case MEMORIES:
                    System.out.println("Java Memory:");
                    System.out.println("date time,  VSZ (MB),  RSS (MB)");
                    IntStream.range(start, options.getEnd().orElse(logEntries.size()))
                             .mapToObj(i -> logEntries.get(i))
                             .forEach(d -> System.out.println(String.format("%s: %d %d", d.getDateTime().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME), d.getJavaVSSize() / 1024 / 1024, d.getJavaRSSize() / 1024 / 1024)));
                    break;
                case SAFEPOINTS:
                    System.out.println("Safepoints:");
                    System.out.println("date time,  count,  time (ms)");
                    IntStream.range(start, options.getEnd().orElse(diffEntries.size()))
                             .mapToObj(i -> diffEntries.get(i))
                             .forEach(d -> System.out.println(String.format("%s: %d %d", d.getDateTime().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME), d.getJvmSafepoints(), d.getJvmSafepointTime())));
                    break;
                case MONITORS:
                    System.out.println("Monitor Contention:");
                    System.out.println("date time,  count");
                    IntStream.range(start, options.getEnd().orElse(diffEntries.size()))
                             .mapToObj(i -> diffEntries.get(i))
                             .forEach(d -> System.out.println(String.format("%s: %d", d.getDateTime().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME), d.getJvmSyncPark())));
                    break;
                case THREADS:
                    System.out.println("Live threads:");
                    System.out.println("date time,  count");
                    IntStream.range(start, options.getEnd().orElse(logEntries.size()))
                             .mapToObj(i -> logEntries.get(i))
                             .forEach(d -> System.out.println(String.format("%s: %d", d.getDateTime().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME), d.getJvmLiveThreads())));
                    break;
                case PARSE_ARCHIVES:
                    System.out.println("Archive list:");
                    System.out.println("date time,  path");
                    IntStream.range(start, options.getEnd().orElse(logEntries.size()))
                             .mapToObj(i -> logEntries.get(i))
                             .filter(d -> d.getArchivePath() != null)
                             .map(d -> new ArchiveData(d, new File(d.getArchivePath().replaceAll("\\..*$", ""))))
                             .peek(a -> a.getExtractPath().mkdir())
                             .peek(new ConsumerWrapper<>(a -> a.parseArchive()))
                             .forEach(a -> System.out.println(String.format("%s: %s", a.getDate().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME), a.getExtractPath().getAbsolutePath())));
                    break;
            }
            
        }
        
    }
    
}

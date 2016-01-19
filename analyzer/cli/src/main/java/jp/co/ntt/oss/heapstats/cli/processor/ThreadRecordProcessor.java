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
import java.util.List;
import java.util.stream.IntStream;
import java.util.stream.Stream;
import jp.co.ntt.oss.heapstats.cli.Options;
import jp.co.ntt.oss.heapstats.container.threadrecord.ThreadStat;
import jp.co.ntt.oss.heapstats.container.threadrecord.ThreadStat.ThreadEvent;
import jp.co.ntt.oss.heapstats.task.ThreadRecordParseTask;

/**
 * CLI task for Thread Recorder.
 * @author Yasumasa Suenaga
 */
public class ThreadRecordProcessor implements CliProcessor{
    
    private final Options options;

    /**
     * Constructor of ThreadRecordProcessor.
     * 
     * @param options Options to use.
     */
    public ThreadRecordProcessor(Options options) {
        this.options = options;
    }
    
    

    /**
     * {@inheritDoc}
     */
    @Override
    public void process() {
        ThreadRecordParseTask threadRecordParser = new ThreadRecordParseTask(options.getFile().get(0).toFile());
        threadRecordParser.run();
        List<ThreadStat> threadStatList = threadRecordParser.getThreadStatList();
        int start = options.getStart().orElse(0);
        
        if(options.isShowId()){
            IntStream.range(start, options.getEnd().orElse(threadStatList.size()))
                     .filter(i -> options.getIdPredicate().test(threadStatList.get(i)))
                     .mapToObj(i -> String.format("%d: %s", i, threadStatList.get(i).getTime().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME)))
                     .forEachOrdered(System.out::println);
        }
        else if(options.getMode() == Options.Mode.SHOW_THREAD_RECORD_ID){
            threadRecordParser.getIdMap().forEach((k, v) -> System.out.println(k.toString() + ": " + v));
        }
        else{
            Stream<ThreadStat> threadStatStream = IntStream.range(start, options.getEnd().orElse(threadStatList.size()))
                                                                                .mapToObj(threadStatList::get)
                                                                                .filter(options.getIdPredicate())
                                                                                .sorted();
            
            switch(options.getMode()){
                case DUMP_SUSPEND_EVENTS:
                    List<ThreadEvent> suspendEventList = Arrays.asList(ThreadEvent.MonitorWait, ThreadEvent.MonitorWaited,
                                                                       ThreadEvent.MonitorContendedEnter, ThreadEvent.MonitorContendedEntered,
                                                                       ThreadEvent.ThreadSleepStart, ThreadEvent.ThreadSleepEnd,
                                                                       ThreadEvent.Park, ThreadEvent.Unpark);
                    threadStatStream = threadStatStream.filter(s -> suspendEventList.contains(s.getEvent()));
                    break;
                case DUMP_LOCK_EVENTS:
                    List<ThreadEvent> lockEventList = Arrays.asList(ThreadEvent.MonitorContendedEnter, ThreadEvent.MonitorContendedEntered,
                                                                    ThreadEvent.Park, ThreadEvent.Unpark);
                    threadStatStream = threadStatStream.filter(s -> lockEventList.contains(s.getEvent()));
                    break;
                case DUMP_IO_EVENTS:
                    List<ThreadEvent> ioEventList = Arrays.asList(ThreadEvent.FileWriteStart, ThreadEvent.FileWriteEnd,
                                                                  ThreadEvent.FileReadStart, ThreadEvent.FileReadEnd,
                                                                  ThreadEvent.SocketWriteStart, ThreadEvent.SocketWriteEnd,
                                                                  ThreadEvent.SocketReadStart, ThreadEvent.SocketReadEnd);
                    threadStatStream = threadStatStream.filter(s -> ioEventList.contains(s.getEvent()));
                    break;
            }
            
            threadStatStream.forEachOrdered(System.out::println);
        }
        
    }
    
}

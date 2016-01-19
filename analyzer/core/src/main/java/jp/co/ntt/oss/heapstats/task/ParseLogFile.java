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

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.atomic.AtomicLong;
import java.util.stream.Stream;
import jp.co.ntt.oss.heapstats.container.log.DiffData;
import jp.co.ntt.oss.heapstats.container.log.LogData;
import jp.co.ntt.oss.heapstats.lambda.ConsumerWrapper;

/**
 * HeapStats log file (CSV) parser.
 * @author Yasumasa Suenaga
 */
public class ParseLogFile extends ProgressRunnable{
    
    private final List<LogData> logEntries;
    
    private final List<DiffData> diffEntries;
    
    private final List<File> fileList;
    
    private final boolean parseAsPossible;
    
    /**
     * Constructor of LogFileParser.
     * 
     * @param fileList List of log to be parsed.
     * @param parseAsPossible Parse log before occuring error.
     */
    public ParseLogFile(List<File> fileList, boolean parseAsPossible){
        logEntries = new ArrayList<>();
        diffEntries = new ArrayList<>();
        this.fileList = fileList;
        this.parseAsPossible = parseAsPossible;
    }
    
    /**
     * This method addes log value from CSV.
     * 
     * @param csvLine CSV data to be added. This value must be 1-raw (1-record).
     * @param logdir Log directory. This value is used to store log value.
     */
    private void addEntry(String csvLine, String logdir){
        LogData element = new LogData();
        
        element.parseFromCSV(csvLine, logdir);        
        logEntries.add(element);
    }
    
    /**
     * Parse log file.
     * 
     * @param logfile Log to be parsedd.
     * @param progress
     * @throws java.io.IOException
     */
    protected void parse(String logfile, AtomicLong progress) throws IOException{
        Path logPath = Paths.get(logfile);
        String logdir =logPath.getParent().toString();
        
        try(Stream<String> paths = Files.lines(logPath)){
            paths.peek(s -> addEntry(s, logdir))
                 .forEach(s -> updateProgress.ifPresent(p -> p.accept(progress.addAndGet(s.length()))));
        }
        
    }
    
    /**
     * Returns log entries of resulting on this task.
     * @return results of this task.
     */
    public List<LogData> getLogEntries() {
        return logEntries;
    }

    /**
     * Returns diff entries of resulting on this task.
     * 
     * @return results of this task.
     */
    public List<DiffData> getDiffEntries() {
        return diffEntries;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run() {
        setTotal(fileList.stream()
                         .mapToLong(f -> f.length())
                         .sum());
        AtomicLong progress = new AtomicLong();
        
        /* Parse log files */
        ConsumerWrapper<String> parseMethod = new ConsumerWrapper<>(f -> parse(f, progress));
        Optional<Exception> parseError = Optional.empty();
        
        try{
            fileList.stream()
                    .map(File::getAbsolutePath)
                    .forEach(parseMethod);
        }
        catch(Exception e){
            parseError = Optional.of(e);
            
            if(!parseAsPossible){
                throw e;
            }
            
        }
        
        /* Sort log files order by date&time */
        logEntries.sort(Comparator.naturalOrder());

        /*
         * Calculate diff data
         * Difference data needs 2 elements at least.
         *
         *  1. Skip top element in logEntries.
         *  2. Pass top element in logEntries to reduce() as identity value.
         *  3. Calculate difference data in reduce().
         *  4. Return current element in logEntries. That value passes next
         *     calculation in reduce().
         */
        logEntries.stream()
                  .skip(1)
                  .reduce(logEntries.get(0), (x, y) -> {
                                                          diffEntries.add(new DiffData(x, y));
                                                          return y;
                                                       });
        
        if(parseError.isPresent()){
            throw new RuntimeException(parseError.get());
        }
        
    }
    
}

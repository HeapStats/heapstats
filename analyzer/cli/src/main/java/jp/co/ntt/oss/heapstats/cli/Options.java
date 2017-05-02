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
package jp.co.ntt.oss.heapstats.cli;

import java.io.File;
import java.net.MalformedURLException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;
import java.util.Optional;
import java.util.OptionalInt;
import java.util.OptionalLong;
import java.util.function.Predicate;
import java.util.regex.Pattern;
import java.util.stream.Collectors;
import javax.management.remote.JMXServiceURL;
import javax.xml.bind.JAXB;
import jp.co.ntt.oss.heapstats.container.snapshot.ObjectData;
import jp.co.ntt.oss.heapstats.container.threadrecord.ThreadStat;
import jp.co.ntt.oss.heapstats.xml.binding.Filters;

/**
 * HeapStats CLI commandline options.
 * 
 * @author Yasumasa Suenaga
 */
public class Options {
    
    /**
     * File type to parse.
     */
    public enum FileType{
        LOG,
        SNAPSHOT,
        THREADRECORD,
        JMX
    }
    
    /**
     * Parse mode.
     */
    public static enum Mode{
        /* Log (-log) */
        JAVA_CPU,       /* -j */
        SYSTEM_CPU,     /* -c */
        MEMORIES,       /* -m */
        SAFEPOINTS,     /* -s */
        MONITORS,       /* -l */
        THREADS,        /* -t */
        PARSE_ARCHIVES, /* -a */
        
        /* Heap SnapShot (-snapshot) */
        SNAPSHOT_SUMMARY, /* -s */
        CLASS_HISTO,      /* -c */
        DIFF_HISTO,       /* -d */
        CLASS_REFERENCES, /* -r */
        HEAP_CSV,         /* -e */
        GC_CSV,           /* -g */
        
        /* Thread Recorder (-record) */
        SHOW_THREAD_RECORD_ID, /* -threads */
        DUMP_THREAD_RECORD,    /* -a */
        DUMP_SUSPEND_EVENTS,   /* -s */
        DUMP_LOCK_EVENTS,      /* -l */
        DUMP_IO_EVENTS,        /* -i */
        
        /* JMX access (-jmx) */
        JMX_GET_VERSION,     /* -v */
        JMX_GET_SNAPSHOT,    /* -s */
        JMX_GET_LOG,         /* -r */
        JMX_GET_CONFIG,      /* -c */
        JMX_CHANGE_CONFIG,   /* -n */
        JMX_INVOKE_SNAPSHOT, /* -is */
        JMX_INVOKE_LOG,      /* -ir */
        JMX_INVOKE_ALL_LOG   /* -ia */
    }
    
    /**
     * File list to parse.
     */
    private List<Path> file;
    
    /**
     * File type.
     */
    private FileType type;
    
    /**
     * Parse mode.
     */
    private Mode mode;
    
    /* -filter */
    private Optional<Pattern> filter = Optional.empty();
    
    /* -exclude */
    private Optional<Path> excludeFilterFile = Optional.empty();
    
    /* -start */
    private OptionalInt start = OptionalInt.empty();
    
    /* -end */
    private OptionalInt end = OptionalInt.empty();
    
    /* -showids */
    private boolean showId = false;
    
    /**
     * Start of reference.
     * This option effects -snapshot -r only.
     */
    private long refStartTag;
    
    /**
     * Direction of reference traverse.
     * This option effects -snapshot -r only.
     */
    private boolean refToParent;
    
    /**
     * CSV file name to dump.
     * This option effects -snapshot -e or -g only.
     */
    private File csvFile;

    /**
     * Thread ID to dump.
     * This option effects -event only.
     */
    private OptionalLong tid = OptionalLong.empty();
    
    /**
     * JMX URL to connect.
     * This value affects -jmx only.
     */
    private JMXServiceURL jmxURL;
    
    /**
     * Configuration key of HeapStats agent.
     * This value affects -jmx only.
     */
    private String configKey;
    
    /**
     * New configuration value of HeapStats agent.
     * This value affects -jmx only.
     */
    private String newConfigValue;
    
    /**
     * Print help message of HeapStats CLI.
     */
    public void printHelp(){
        System.out.println("Usage:");
        System.out.println("  java -jar heapstats-cli.jar <common options> <mode> <options> <file...>");
        System.out.println();
        System.out.println("common options:");
        System.out.println("  -start: Start ID");
        System.out.println("  -end  : End ID");
        System.out.println();
        System.out.println("mode:");
        System.out.println("  -log     : Process HeapStats resource log file.");
        System.out.println("  -snapshot: Process HeapStats snapshot file.");
        System.out.println("  -event   : Process HeapStats thread recorder file.");
        System.out.println("  -jmx     : Control remote HeapStats agent through JMX.");
        System.out.println();
        System.out.println("options:");
        System.out.println("  -log:");
        System.out.println("    -showids: List all IDs in files.");
        System.out.println("    -i      : Show CPU percentage in java process.");
        System.out.println("    -c      : Show CPU usage all over the system.");
        System.out.println("    -m      : Show VSZ/RSS usage at java process.");
        System.out.println("    -s      : Show Safepoint count/time at java process.");
        System.out.println("    -l      : Show monitor contention count at java process.");
        System.out.println("    -t      : Show count of live java threads at java process.");
        System.out.println("    -a      : Report and extract error archive in resource file.");
        System.out.println("  -snapshot:");
        System.out.println("    -showids      : List all IDs in files.");
        System.out.println("    -filter       : Set filter to out. You can use regex.");
        System.out.println("    -exclude      : Set exclude filter to out. You have to pass exclude XML file.");
        System.out.println("    -s            : Show snapshot summary.");
        System.out.println("    -c            : Show class histogram.");
        System.out.println("    -d            : Show histogram from diff of snapshots.");
        System.out.println("    -r <class tag>: Show class references. You have to pass class id as start point.");
        System.out.println("      -d <p|c>: Select the direction to traverse references. p means parent, c means child.");
        System.out.println("    -e <CSV file> : Dump class histogram(s) as CSV.");
        System.out.println("    -g <CSV file> : Dump GC information as CSV.");
        System.out.println("  -event: Process HeapStats thread recorder file.");
        System.out.println("    -showids      : List all IDs in files.");
        System.out.println("    -threads: List all thread IDs in files.");
        System.out.println("    -id <ID>: Choose specified thread ID.");
        System.out.println("    -a      : List all events.");
        System.out.println("    -s      : List suspend events.");
        System.out.println("    -l      : List lock events.");
        System.out.println("    -i      : List I/O events.");
        System.out.println("  -jmx <URL> : Access remote HeapStats agent through JMX.");
        System.out.println("    -v               : Get version of HeapStats agent.");
        System.out.println("    -s               : Save remote HeapStats snapshot to file.");
        System.out.println("    -r               : Save remote HeapStats resource log to file.");
        System.out.println("    -c <name>        : Get configuration value of <name> in remote HeapStats agent.");
        System.out.println("    -n <name> <value>: Change configuration value of <name> to <value> in remote HeapStats agent.");
        System.out.println("    -is              : Invoke SnapShot collection.");
        System.out.println("    -ir              : Invoke resource log collection.");
        System.out.println("    -ia              : Invoke all log collection.");
    }
    
    /**
     * Get next value from iterator.
     * @param itr Iterator of commandline options.
     * @param errorMessage Exception message if iterator has no more option.
     * @return Option value.
     */
    private String getNextValue(Iterator<String> itr, String errorMessage){
        
        if(!itr.hasNext()){
            throw new IllegalArgumentException(errorMessage);
        }
        
        return itr.next();
    }
    
    /**
     * Parse commandline options for -log option.
     * @param itr List iterator of commandline options.
     */
    private void parseLogOptions(Iterator<String> itr){
        String option = getNextValue(itr, "-log option need more argument.");
        type = FileType.LOG;
        
        switch(option){
            case "-showids":
                showId = true;
                break;
            case "-i":
                mode = Mode.JAVA_CPU;
                break;
            case "-c":
                mode = Mode.SYSTEM_CPU;
                break;
            case "-m":
                mode = Mode.MEMORIES;
                break;
            case "-s":
                mode = Mode.SAFEPOINTS;
                break;
            case "-l":
                mode = Mode.MONITORS;
                break;
            case "-t":
                mode = Mode.THREADS;
                break;
            case "-a":
                mode = Mode.PARSE_ARCHIVES;
                break;
            default:
                throw new IllegalArgumentException("Unknown option: " + option);
        }
        
    }
    
    /**
     * Parse commandline options for -snapshot option.
     * @param itr List iterator of commandline options.
     */
    private void parseSnapShotOptions(ListIterator<String> itr){
        type = FileType.SNAPSHOT;
        
        while(true){
            String option = getNextValue(itr, "-snapshot option need more argument.");
            if(option.charAt(0) != '-'){
                itr.previous();
                break;
            }
            
            switch(option){
                case "-showids":
                    showId = true;
                    break;
                case "-filter":
                    filter = Optional.of(Pattern.compile(getNextValue(itr, "-filter option needs regex pattern.")));
                    break;
                case "-exclude":
                    excludeFilterFile = Optional.of(Paths.get(getNextValue(itr, "-exclude option needs exclude XML file.")));
                    break;
                case "-s":
                    mode = Mode.SNAPSHOT_SUMMARY;
                    break;
                case "-c":
                    mode = Mode.CLASS_HISTO;
                    break;
                case "-d":
                    mode = Mode.DIFF_HISTO;
                    break;
                case "-r":
                    mode = Mode.CLASS_REFERENCES;
                    refStartTag = Long.decode(getNextValue(itr, "Class Reference option (-r) needs class tag value."));

                    if(!getNextValue(itr, "Class Reference option (-r) needs direction option (-d <p|c>)").equals("-d")){
                        throw new IllegalArgumentException("Class Reference option (-r) needs direction option (-d <p|c>)");
                    }

                    switch(getNextValue(itr, "Class Reference option (-r) needs direction option (-d <p|c>)")){
                        case "p":
                            refToParent = true;
                            break;
                        case "c":
                            refToParent = false;
                            break;
                        default:
                            throw new IllegalArgumentException("Class Reference option (-r) needs direction option (-d <p|c>)");
                    }

                    break;
                case "-e":
                    mode = Mode.HEAP_CSV;
                    csvFile = new File(getNextValue(itr, "Heap CSV option (-e) needs file name of CSV."));
                    break;
                case "-g":
                    mode = Mode.GC_CSV;
                    csvFile = new File(getNextValue(itr, "GC CSV option (-g) needs file name of CSV."));
                    break;
                default:
                    throw new IllegalArgumentException("Unknown option: " + option);
            }
            
        }
        
    }
    
    /**
     * Parse commandline options for -event option.
     * @param itr List iterator of commandline options.
     */
    private void parseThreadRecorderOptions(ListIterator<String> itr){
        type = FileType.THREADRECORD;
        
        while(true){
            String option = getNextValue(itr, "-event option need more argument.");
            if(option.charAt(0) != '-'){
                itr.previous();
                break;
            }
            
            switch(option){
                case "-showids":
                    showId = true;
                    break;
                case "-id":
                    tid = OptionalLong.of(Long.parseLong(getNextValue(itr, "-id option needs Thrad ID.")));
                    break;
                case "-threads":
                    mode = Mode.SHOW_THREAD_RECORD_ID;
                    break;
                case "-a":
                    mode = Mode.DUMP_THREAD_RECORD;
                    break;
                case "-s":
                    mode = Mode.DUMP_SUSPEND_EVENTS;
                    break;
                case "-l":
                    mode = Mode.DUMP_LOCK_EVENTS;
                    break;
                case "-i":
                    mode = Mode.DUMP_IO_EVENTS;
                    break;
                default:
                    throw new IllegalArgumentException("Unknown option: " + option);
            }
            
        }
        
    }
    
    /**
     * Parse commandline options for -jmx option.
     * @param itr List iterator of commandline options.
     */
    private void parseJMXOptions(ListIterator<String> itr){
        type = FileType.JMX;
        
        try {
            jmxURL = new JMXServiceURL(getNextValue(itr, "-jmx option need a URL to connect."));
        } catch (MalformedURLException ex) {
            throw new IllegalArgumentException("Invalid JMX URL", ex);
        }
        
        String option = getNextValue(itr, "-jmx option need more argument.");
        switch(option){
            case "-v":
                mode = Mode.JMX_GET_VERSION;
                break;
            case "-s":
                mode = Mode.JMX_GET_SNAPSHOT;
                break;
            case "-r":
                mode = Mode.JMX_GET_LOG;
                break;
            case "-c":
                mode = Mode.JMX_GET_CONFIG;
                configKey = getNextValue(itr, "-jmx option need more argument.");
                break;
            case "-n":
                mode = Mode.JMX_CHANGE_CONFIG;
                configKey = getNextValue(itr, "-jmx option need more argument.");
                newConfigValue = getNextValue(itr, "-jmx option need more argument.");
                break;
            case "-is":
                mode = Mode.JMX_INVOKE_SNAPSHOT;
                break;
            case "-ir":
                mode = Mode.JMX_INVOKE_LOG;
                break;
            case "-ia":
                mode = Mode.JMX_INVOKE_ALL_LOG;
                break;
            default:
                throw new IllegalArgumentException("Unknown option: " + option);
        }
        
    }

    /**
     * Parse commandline options.
     * @param options Array of commandline options.
     */
    public void parse(String[] options){
        file = new ArrayList<>();
        ListIterator<String> itr = Arrays.asList(options).listIterator();
        
        while(itr.hasNext()){
            String option = itr.next();
            
            switch(option){
                case "-start":
                    start = OptionalInt.of(Integer.parseInt(getNextValue(itr, "-start option needs valid ID.")));
                    break;
                case "-end":
                    end = OptionalInt.of(Integer.parseInt(getNextValue(itr, "-end option needs valid ID.")) + 1);
                    break;
                case "-log":
                    parseLogOptions(itr);
                    break;
                case "-snapshot":
                    parseSnapShotOptions(itr);
                    break;
                case "-event":
                    parseThreadRecorderOptions(itr);
                    break;
                case "-jmx":
                    parseJMXOptions(itr);
                    break;
                default:
                    file.add(Paths.get(option));
            }
            
        }
        
    }

    /**
     * Get file list to parse.
     * @return File list to parse.
     */
    public List<Path> getFile() {
        return file;
    }

    /**
     * Get parse mode.
     * @return Parse mode.
     */
    public Mode getMode() {
        return mode;
    }

    /**
     * Get class regex filter.
     * @return Class regex filter.
     */
    public Optional<Pattern> getFilter() {
        return filter;
    }

    /**
     * Get class exclude filter.
     * @return Class exclude filter.
     */
    public Optional<Path> getExcludeFilterFile() {
        return excludeFilterFile;
    }

    /**
     * Get id to parse starting.
     * @return Start ID
     */
    public OptionalInt getStart() {
        return start;
    }

    /**
     * Get id to parse ending.
     * @return End ID
     */
    public OptionalInt getEnd() {
        return end;
    }

    /**
     * Get start of reference.
     * This option effects -snapshot -r only.
     * @return Start tag of reference.
     */
    public long getRefStartTag() {
        return refStartTag;
    }

    /**
     * Direction of reference traverse.
     * This option effects -snapshot -r only.
     * @return true if parent.
     */
    public boolean isRefToParent() {
        return refToParent;
    }

    /**
     * Show ID option.
     * @return true if show id.
     */
    public boolean isShowId() {
        return showId;
    }
    
    /**
     * Get file type to parse.
     * @return File type.
     */
    public FileType getType() {
        return type;
    }
    
    /**
     * Get clsss filter predicate.
     * @return Class filter predicate.
     */
    public Predicate<? super ObjectData> getFilterPredicate(){
        Optional<Predicate<ObjectData>> includePredicate = filter.map(p -> (o -> p.asPredicate().test(o.getName())));
        List<String> excludeFilterList = excludeFilterFile.map(f -> ((Filters)JAXB.unmarshal(f.toFile(), Filters.class)).getFilter().stream()
                                                                                                                                    .filter(l -> l != null)
                                                                                                                                    .flatMap(l -> l.getClasses().getName().stream())
                                                                                                                                    .map(s -> ".*" + s + ".*")
                                                                                                                                    .collect(Collectors.toList()))
                                                          .orElse(new ArrayList<>());
        Predicate<ObjectData> excludePredicate = o -> excludeFilterList.stream().noneMatch(s -> o.getName().matches(s));
        
        Predicate<? super ObjectData> pred = null;
        
        if(includePredicate.isPresent()){
            pred = excludeFilterList.isEmpty() ? includePredicate.get() : includePredicate.get().and(excludePredicate);
        }
        else if(!excludeFilterList.isEmpty()){
            pred = excludePredicate;
        }
        
        return pred;
    }

    /**
     * Get CSV file name.
     * @return CSV file name.
     */
    public File getCsvFile() {
        return csvFile;
    }
    
    /**
     * Get predicate of Thread ID filter.
     * @return Thread ID filter.
     */
    public Predicate<? super ThreadStat> getIdPredicate(){
        return tid.isPresent() ? (s -> tid.getAsLong() == s.getId()) : (s -> true);
    }

    /**
     * Get JMX URL to connect.
     * @return JMX URL.
     */
    public JMXServiceURL getJmxURL() {
        return jmxURL;
    }

    /**
     * Get configuration key of HeapStats agent.
     * @return Configuration key.
     */
    public String getConfigKey() {
        return configKey;
    }

    /**
     * Get new configuration value of HeapStats agent.
     * @return New configuration value.
     */
    public String getNewConfigValue() {
        return newConfigValue;
    }
    
}

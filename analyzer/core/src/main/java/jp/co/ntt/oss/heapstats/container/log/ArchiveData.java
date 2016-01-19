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

package jp.co.ntt.oss.heapstats.container.log;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Properties;
import java.util.StringJoiner;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import jp.co.ntt.oss.heapstats.lambda.ConsumerWrapper;

/**
 * This class stores archive data.
 * 
 * @author Yasumasa Suenaga
 */
public class ArchiveData {
    
    private static final int INDEX_LOCAL_ADDR = 1;

    private static final int INDEX_FOREIGN_ADDR = 2;

    private static final int INDEX_STATE = 3;

    private static final int INDEX_QUEUE = 4;

    /** Represents the index value of the inode of the file socket endpoint. */
    private static final int INDEX_INODE = 9;

    private final LocalDateTime date;
    
    private final String archivePath;
    
    private File extractPath;
    
    private Map<String, String> envInfo;
    
    private List<String> tcp;
    
    private List<String> tcp6;
    
    private List<String> udp;
    
    private List<String> udp6;
    
    private List<String> sockOwner;
    
    private boolean parsed;
    
    /**
     * Constructor of ArchiveData.
     * 
     * @param log LogData. This value must be included archive data.
     * @throws java.io.IOException
     */
    public ArchiveData(LogData log) throws IOException{
        this(log, null);
        extractPath = Files.createTempDirectory("heapstats_archive").toFile();
        extractPath.deleteOnExit();
    }
    
    /**
     * Constructor of ArchiveData.
     * 
     * @param log LogData. This value must be included archive data.
     * @param location Location to extract archive data.
     */
    public ArchiveData(LogData log, File location){
        date = log.getDateTime();
        archivePath = log.getArchivePath();
        extractPath = location;
        parsed = false;
    }
    
    /**
     * Build environment info from envInfo.txt .
     * 
     * @param archive HeapStats ZIP archive.
     * @param entry  ZipEntry of methodInfo.
     */
    private void buildEnvInfo(ZipFile archive, ZipEntry entry) throws IOException{
        DateTimeFormatter formatter = DateTimeFormatter.ofPattern("yyyy/MM/dd HH:mm:ss");
        
        try(InputStream in = archive.getInputStream(entry)){
            Properties prop = new Properties();
            prop.load(in);
            prop.computeIfPresent("CollectionDate", (k, v) -> formatter.format(
                                                                   LocalDateTime.ofInstant(Instant.ofEpochMilli(
                                                                         Long.parseLong((String)v)), ZoneId.systemDefault())));
            String[] triggers = {"Resource Exhausted", "Signal", "Interval", "Deadlock"};
            prop.computeIfPresent("LogTrigger", (k, v) -> triggers[Integer.parseInt((String)v) - 1]);
                
            envInfo = new HashMap<>();
            envInfo.put("archive", archivePath);
            prop.forEach((k, v) -> envInfo.put((String)k, (String)v));
        }

    }
    
    /**
     * Build String data from ZIP entry
     * 
     * @param archive HeapStats ZIP archive.
     * @param entry ZipEntry to be parsed.
     * @return String value from ZipEntry.
     * @throws java.io.IOException
     */
    private List<String> buildStringData(ZipFile archive, ZipEntry entry) throws IOException{

        try(BufferedReader reader = new BufferedReader(new InputStreamReader(archive.getInputStream(entry)))){
            return reader.lines()
                         .skip(1)
                         .map(s -> s.trim())
                         .collect(Collectors.toList());
        }

    }
    
    /**
     * Deflating file in ZIP.
     * 
     * @param archive HeapStats ZIP archive.
     * @param entry ZipEntry to be deflated.
     * @throws java.io.IOException
     */
    private void deflateFileData(ZipFile archive, ZipEntry entry) throws IOException{
        Path destPath = Paths.get(extractPath.getAbsolutePath(), entry.getName());
        
        try(BufferedReader reader = new BufferedReader(new InputStreamReader(archive.getInputStream(entry)));
            PrintWriter writer = new PrintWriter(Files.newBufferedWriter(destPath, StandardOpenOption.CREATE));){
            
            reader.lines()
                  .map(s -> s.replace('\0', ' '))
                  .forEach(writer::println);
        }

    }
    
    /**
     * Get IPv4 data.
     * 
     * @param data String data in procfs.
     * @return IPv4 address.
     */
    private String getIPv4(String data){
        StringJoiner joiner = (new StringJoiner("."))
                                 .add(Integer.valueOf(data.substring(6, 8), 16).toString())
                                 .add(Integer.valueOf(data.substring(4, 6), 16).toString())
                                 .add(Integer.valueOf(data.substring(2, 4), 16).toString())
                                 .add(Integer.valueOf(data.substring(0, 2), 16).toString());
        
        return joiner.toString();
    }
    
    /**
     * Get IPv6 data.
     * 
     * @param data String data in procfs.
     * @return IPv4 address.
     */
    private String getIPv6(String data){
        StringJoiner joiner = (new StringJoiner(":"))
                                 .add(data.substring(6, 8) + data.substring(4, 6))
                                 .add(data.substring(2, 4) + data.substring(0, 2))
                                 .add(data.substring(14, 16) + data.substring(12, 14))
                                 .add(data.substring(10, 12) + data.substring(8, 10))
                                 .add(data.substring(22, 24) + data.substring(20, 22))
                                 .add(data.substring(18, 20) + data.substring(16, 18))
                                 .add(data.substring(26, 28) + data.substring(24, 26));

        return joiner.toString();
    }
    
    /**
     * Write socket data.
     * 
     * @param proto Protocol. tcp or udp.
     * @param data Socket owner data.
     * @param writer PrintWriter to write.
     * @param isIPv4  true if this arguments represent IPv4.
     */
    private void writeSockDataInternal(String proto, String[] data, PrintWriter writer, boolean isIPv4){
        writer.print(String.format("%-7s", sockOwner.contains(data[INDEX_INODE]) ? "jvm" : "")); // owner
        writer.print(String.format("%-7s", proto));
        
        String[] queueData = data[INDEX_QUEUE].split(":");
        writer.print(String.format("%-8d", Integer.parseInt(queueData[1], 16))); // Recv-Q
        writer.print(String.format("%-8d", Integer.parseInt(queueData[0], 16))); // Send-Q
        
        String[] localAddr = data[INDEX_LOCAL_ADDR].split(":"); // local address
        String localAddrStr = isIPv4 ? getIPv4(localAddr[0]) : getIPv6(localAddr[0]);
        localAddrStr += String.format(":%d", Integer.parseInt(localAddr[1], 16));
        writer.print(String.format("%-42s", localAddrStr));
        
        String[] foreignAddr = data[INDEX_FOREIGN_ADDR].split(":"); // foreign address
        String foreignAddrStr = isIPv4 ? getIPv4(foreignAddr[0]) : getIPv6(foreignAddr[0]);
        foreignAddrStr += String.format(":%d", Integer.parseInt(foreignAddr[1], 16));
        writer.print(String.format("%-42s", foreignAddrStr));
        
        try{
            String[] states = {"ESTABLISHED", "SYN_SENT", "SYN_RECV", "FIN_WAIT1", "FIN_WAIT2",
                                  "TIME_WAIT", "CLOSE", "CLOSE_WAIT", "LAST_ACK", "LISTEN", "CLOSING"};
            writer.println(states[Integer.parseInt(data[INDEX_STATE], 16) - 1]); // connection state
        }
        catch(ArrayIndexOutOfBoundsException e){
            writer.println("-");
        }
        
    }
    
    /**
     * Build socket data from archive data.
     * 
     * @throws IOException 
     */
    private void buildSockData() throws IOException{
        Path sockfile = Paths.get(extractPath.getAbsolutePath(), "socket");
        
        try(PrintWriter writer = new PrintWriter(Files.newOutputStream(sockfile, StandardOpenOption.CREATE))){
            writer.print(String.format("%-7s", "Owner"));
            writer.print(String.format("%-7s", "Proto"));
            writer.print(String.format("%-8s", "Recv-Q"));
            writer.print(String.format("%-8s", "Send-Q"));
            writer.print(String.format("%-42s", "Local Address"));
            writer.print(String.format("%-42s", "Foreign Address"));
            writer.println("State");
            
            Optional.ofNullable(tcp)
                    .ifPresent(l -> l.stream()
                                     .map(s -> s.split("\\s+"))
                                     .forEach(d -> writeSockDataInternal("tcp", d, writer, true)));
            Optional.ofNullable(tcp6)
                    .ifPresent(l -> l.stream()
                                     .map(s -> s.split("\\s+"))
                                     .forEach(d -> writeSockDataInternal("tcp6", d, writer, false)));
            Optional.ofNullable(udp)
                    .ifPresent(l -> l.stream()
                                     .map(s -> s.split("\\s+"))
                                     .forEach(d -> writeSockDataInternal("udp", d, writer, true)));
            Optional.ofNullable(udp6)
                    .ifPresent(l -> l.stream()
                                     .map(s -> s.split("\\s+"))
                                     .forEach(d -> writeSockDataInternal("udp6", d, writer, false)));
        }
        
    }
    
    private void processZipEntry(ZipFile archive, ZipEntry entry) throws IOException{
        switch(entry.getName()){
            case "envInfo.txt":
                buildEnvInfo(archive, entry);
                break;

            case "tcp":
                tcp = buildStringData(archive, entry);
                break;

            case "tcp6":
                tcp6 = buildStringData(archive, entry);
                break;

            case "udp":
                udp = buildStringData(archive, entry);
                break;

            case "udp6":
                udp6 = buildStringData(archive, entry);
                break;

            case "sockowner":
                sockOwner = buildStringData(archive, entry);
                break;

            default:
                deflateFileData(archive, entry);
                break;
        }
    }
    
    /**
     * Parsing Archive data.
     * @throws java.io.IOException
     */
    public void parseArchive() throws IOException{
        
        if(parsed){
            return;
        }
        
        try(ZipFile archive = new ZipFile(archivePath)){
            archive.stream().forEach(new ConsumerWrapper<ZipEntry>(d -> processZipEntry(archive, d)));
            buildSockData();
        }
        
        parsed = true;
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            extractPath.delete();
        } finally {
            super.finalize();
        }
    }

    /**
     * Getter of this date this archive is created.
     * @return LocalDateTime this archive is created.
     */
    public LocalDateTime getDate() {
        return date;
    }

    /**
     * Getter of envInfo.
     * 
     * @return envInfo in this archive.
     */
    public Map<String, String> getEnvInfo() {
        return envInfo;
    }
    
    /**
     * Getter of file list in this archive.
     * This list includes all deflated files in archive.
     * 
     * @return file list in this archive.
     */
    public List<String> getFileList(){
        return Arrays.asList(extractPath.list());
    }

    /**
     * Getter of location to archive data.
     * 
     * @return Path of archive data.
     */
    public File getExtractPath() {
        return extractPath;
    }
    
    /**
     * Getter of file contents.
     * Contents is represented as String.
     * 
     * @param file File to be got.
     * @return Contents of file.
     * @throws IOException 
     */
    public String getFileContents(String file) throws IOException{
        Path filePath = Paths.get(extractPath.getAbsolutePath(), file);
        
        try(Stream<String> paths = Files.lines(filePath)){
            return paths.collect(Collectors.joining("\n"));
        }
        
    }
    
}

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

package jp.co.ntt.oss.heapstats.plugin.builtin.jvmlive.jdp;

import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.time.LocalDateTime;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import javafx.application.Platform;
import javafx.beans.property.BooleanProperty;
import javafx.beans.property.ObjectProperty;
import javafx.beans.property.SimpleBooleanProperty;
import javafx.beans.property.SimpleObjectProperty;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.concurrent.Task;
import javafx.scene.control.Hyperlink;
import javafx.scene.control.Label;
import javafx.scene.control.Labeled;
import javafx.scene.control.ListView;
import jp.co.ntt.oss.heapstats.fx.lambda.EventHandlerWrapper;
import jp.co.ntt.oss.heapstats.jmx.JMXHelper;
import jp.co.ntt.oss.heapstats.lambda.RunnableWrapper;
import jp.co.ntt.oss.heapstats.utils.LocalDateTimeConverter;


/**
 * JDP packet decoder.
 * This class containes JDP packet data and method for parse it.
 * 
 * @author Yasumasa Suenaga
 */
public class JdpDecoder extends Task<Void>{
    
    /** Key of JDP for main class. */
    public static final String MAIN_CLASS_KEY = "MAIN_CLASS";
    
    /** Key of JDP for instance name. */
    public static final String INSTANCE_NAME_KEY = "INSTANCE_NAME";
    
    /** Key of JDP for JMX URL. */
    public static final String JMX_SERVICE_URL_KEY = "JMX_SERVICE_URL";
    
    /** Key of JDP for UUID. */
    public static final String DISCOVERABLE_SESSION_UUID_KEY = "DISCOVERABLE_SESSION_UUID";
    
    /** Key of JDP for broadcast interval. */
    public static final String PACKET_BROADCAST_INTERVAL_KEY = "BROADCAST_INTERVAL";
    
    /** Key of JDP for PID. */
    public static final String PROCESS_ID_KEY = "PROCESS_ID";
    
    /** Raw JDP packet data. */
    private final ByteBuffer jdpRawData;
    
    /**
     * Received time of this instance.
     */
    private LocalDateTime receivedTime;
    
    /** Source address of this JDP packet. */
    private final InetSocketAddress sourceAddr;
    
    /** Main class of this JDP packet. */
    private String mainClass;
    
    /** Instance name of this JDP packet. */
    private String instanceName;
    
    /** JMX URL of this JDP packet. */
    private String jmxServiceURL;
    
    /** PID of this JDP packet. */
    private int pid;
    
    /** UUID of this JDP packet. */
    private UUID uuid;
    
    /** Broadcast interval of this JDP packet. */
    private int broadcastInterval;
    
    /** If this packet is expired, this flag is set to true by JdpValidatorService. */
    private final BooleanProperty invalidate;
    
    private final ObjectProperty<ObservableList<JdpTableKeyValue>> jdpTableKeyValue;
    
    private final ListView<JdpDecoder> jdpList;
    
    private final Optional<String> jconsolePath;
    
    private final ExecutorService jmxProcPool;
    
    private JdpTableKeyValue heapstatsValue;
    
    /**
     * Constructor of JdpDecorder.
     * 
     * @param sourceAddr Source address of JDP packet.
     * @param rawData JDP raw data.
     * @param jdpList ListView which includes JDP.
     * @param jconsolePath Path to JConsole.
     * @param jmxProcPool ThreadPool which processes JMX access.
     */
    public JdpDecoder(InetSocketAddress sourceAddr, ByteBuffer rawData, ListView<JdpDecoder> jdpList, Optional<String> jconsolePath, ExecutorService jmxProcPool){
        this.receivedTime = LocalDateTime.now();
        this.sourceAddr = sourceAddr;
        this.jdpRawData = rawData;
        this.jdpList = jdpList;
        this.invalidate = new SimpleBooleanProperty(false);
        this.jdpTableKeyValue = new SimpleObjectProperty<>();
        this.jconsolePath = jconsolePath;
        this.jmxProcPool = jmxProcPool;
    }
    
    /**
     * Create String instance from ByteBuffer.
     * 
     * @param buffer ByteBuffer to parse. Position must be set to top of String data.
     * @return String from ByteBuffer.
     */
    private String getStringFromByteBuffer(ByteBuffer buffer){
        int length = Short.toUnsignedInt(buffer.getShort());
        byte[] rawValue = new byte[length];
        buffer.get(rawValue);
        
        return new String(rawValue, StandardCharsets.UTF_8);
    }
    
    private Map<String, String> parseJdpPacket(){

        if(jdpRawData.getInt() != 0xC0FFEE42){
            throw new RuntimeException("Invalid magic number.");
        }
        
        if(jdpRawData.getShort() != 1){
            throw new RuntimeException("Invalid JDP version.");
        }
        
        Map<String, String> jdpContents = new HashMap<>();
        
        while(jdpRawData.hasRemaining()){
            String key = getStringFromByteBuffer(jdpRawData);
            String value = getStringFromByteBuffer(jdpRawData);
            
            jdpContents.put(key, value);
        }
        
        return jdpContents;
    }
    
    private void setJdpTableKeyValue(JdpTableKeyValue val, Labeled jmxURL){
        switch(val.keyProperty().get()){
            case "Received Time":
                val.valueProperty().set((new LocalDateTimeConverter()).toString(receivedTime));
                break;

            case "Address":
                val.valueProperty().set(sourceAddr.getAddress().getHostAddress());
                break;

            case "JDP Instance Name":
                val.valueProperty().set(instanceName);
                break;

            case "Main Class":
                val.valueProperty().set(mainClass);
                break;

            case "UUID":
                val.valueProperty().set(uuid.toString());
                break;

            case "PID":
                val.valueProperty().set(Integer.toString(pid));
                break;

            case "JMX URL":
                val.valueProperty().set(jmxURL);
        }
    }
    
    /**
     * Update JDP packet if same UUID is registered in JDP list.
     */
    private void updateJDPData(){
        Labeled jmxURL;

        if(jconsolePath.isPresent()){
            String[] execParam = {jconsolePath.get(), jmxServiceURL};
            jmxURL = new Hyperlink(jmxServiceURL);
            ((Hyperlink)jmxURL).setOnAction(new EventHandlerWrapper<>(e -> Runtime.getRuntime().exec(execParam)));
        }
        else{
            jmxURL = new Label(jmxServiceURL);
        }
        
        int idx = jdpList.getItems().indexOf(this);
        
        if(idx == -1){
            heapstatsValue = new JdpTableKeyValue("HeapStats", "Checking...");
            jmxProcPool.submit(new RunnableWrapper(() -> heapstatsValue.valueProperty().set(new JMXHelper(jmxServiceURL))));
            
            jdpTableKeyValue.set(FXCollections.observableArrayList(
                    new JdpTableKeyValue("Received Time", (new LocalDateTimeConverter()).toString(receivedTime)),
                    new JdpTableKeyValue("Address", sourceAddr.getAddress().getHostAddress()),
                    new JdpTableKeyValue("JDP Instance Name", instanceName),
                    new JdpTableKeyValue("Main Class", mainClass),
                    new JdpTableKeyValue("UUID", uuid.toString()),
                    new JdpTableKeyValue("PID", Integer.toString(pid)),
                    new JdpTableKeyValue("JMX URL", jmxURL),
                    heapstatsValue
            ));
            jdpList.getItems().add(this);
        }
        else{
            JdpDecoder existData = jdpList.getItems().get(idx);
            existData.receivedTime = receivedTime;
            existData.jdpTableKeyValue.get().forEach(p -> setJdpTableKeyValue(p, jmxURL));
        }
        
    }

    @Override
    protected Void call() throws Exception {
        Map<String, String> jdpData = parseJdpPacket();
        
        this.mainClass = jdpData.get(MAIN_CLASS_KEY);
        this.instanceName = jdpData.get(INSTANCE_NAME_KEY);
        this.jmxServiceURL = jdpData.get(JMX_SERVICE_URL_KEY);
        this.uuid = UUID.fromString(jdpData.get(DISCOVERABLE_SESSION_UUID_KEY));
        this.pid = Integer.parseInt(jdpData.get(PROCESS_ID_KEY));
        this.broadcastInterval = Integer.parseInt(jdpData.get(PACKET_BROADCAST_INTERVAL_KEY));
        
        Platform.runLater(this::updateJDPData);

        return null;
    }

    /**
     * Get time of this JDP packet was received.
     * 
     * @return Received time.
     */
    public LocalDateTime getReceivedTime() {
        return receivedTime;
    }

    /**
     * Get source address of JDP packet.
     * 
     * @return Source address of JDP.
     */
    public InetSocketAddress getSourceAddr() {
        return sourceAddr;
    }

    /**
     * Get main class in JDP packet.
     * 
     * @return Main class.
     */
    public String getMainClass() {
        return mainClass;
    }

    /**
     * Get JMX URL in JDP packet.
     * 
     * @return JDP URL.
     */
    public String getJmxServiceURL() {
        return jmxServiceURL;
    }

    /**
     * Get PID in JDP packet.
     * @return PID
     */
    public int getPid() {
        return pid;
    }

    /**
     * Get UUID in JDP packet.
     * @return UUID
     */
    public UUID getUuid() {
        return uuid;
    }

    /**
     * Get instance name in JDP packet.
     * @return Instance name.
     */
    public String getInstanceName() {
        return instanceName;
    }

    /**
     * Get broadcast interval in JDP packet.
     * 
     * @return JDP broadcast interval.
     */
    public int getBroadcastInterval() {
        return broadcastInterval;
    }
    
    /**
     * Set invalidate to this JDP packet.
     */
    public void setInvalidate(){
        invalidate.set(true);
    }
    
    /**
     * Get invalidate property.
     * 
     * @return Invalidate property.
     */
    public BooleanProperty invalidateProperty(){
        return this.invalidate;
    }
    
    /**
     * Get JDP packet data list.
     * This method returns key-value list.
     * 
     * @return JDP packet data list.
     */
    public ObjectProperty<ObservableList<JdpTableKeyValue>> jdpTableKeyValueProperty(){
        return this.jdpTableKeyValue;
    }
    
    /**
     * Get JDP Key-Value.
     * 
     * @return JDP packet data.
     */
    public JdpTableKeyValue getHeapStatsTableKeyValue(){
        return this.heapstatsValue;
    }
    
    @Override
    public int hashCode() {
        return this.uuid.hashCode();
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        return this.uuid.equals(((JdpDecoder)obj).uuid);
    }

    @Override
    public String toString() {
        return Optional.ofNullable(instanceName).orElse(mainClass);
    }

}

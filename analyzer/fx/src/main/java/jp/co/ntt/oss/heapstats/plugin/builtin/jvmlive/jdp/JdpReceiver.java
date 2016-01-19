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

import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.net.SocketAddress;
import java.net.StandardProtocolFamily;
import java.net.StandardSocketOptions;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.channels.ClosedByInterruptException;
import java.nio.channels.DatagramChannel;
import java.util.Collections;
import java.util.Optional;
import java.util.concurrent.ExecutorService;
import javafx.concurrent.Task;
import javafx.scene.control.ListView;
import jp.co.ntt.oss.heapstats.lambda.ConsumerWrapper;
import jp.co.ntt.oss.heapstats.lambda.PredicateWrapper;

/**
 * JDP Packet receiver thread.
 * 
 * @author Yasumasa Suenaga
 */
public class JdpReceiver extends Task<Void>{
    
    /** System property key of JDP multicast address. */
    public static final String JDP_ADDRESS_PROP_NAME = "com.sun.management.jdp.address";
    
    /** System property key of JDP port, */
    public static final String JDP_PORT_PROP_NAME = "com.sun.management.jdp.port";
    
    /** Default JDP multicast address. */
    public static final String JDP_DEFAULT_ADDRESS = "224.0.23.178";
    
    /** Default JDP port. */
    public static final int JDP_DEFAULT_PORT = 7095;
    
    /** UDP packet length. */
    public static final int UDP_PACKET_LENGTH = 65535; // 64KB
    
    private InetAddress jdpAddr;
    
    private int jdpPort;
    
    /** Thread pool for JdpDecoder. */
    private ExecutorService jdpProcPool;
    
    private ExecutorService jmxProcPool;
    
    private ListView<JdpDecoder> jdpList;
    
    private Optional<String> jconsolePath;
    
    /**
     * Constructor of JdpReceiver.
     * 
     * @param jdpAddr JDP multicast address.
     * @param jdpPort JDP port.
     * @param jdpList ListView which includes JDP packet data.
     * @param threadPool ThreadPool which processes JDP packet decording.
     * @param jconsolePath Path to JConsole.
     * @param jmxPool ThreadPool which processes JMX access.
     */
    public JdpReceiver(InetAddress jdpAddr, int jdpPort, ListView<JdpDecoder> jdpList, ExecutorService threadPool, Optional<String> jconsolePath, ExecutorService jmxPool){
        this.jdpAddr = jdpAddr;
        this.jdpPort = jdpPort;
        this.jdpProcPool = threadPool;
        this.jdpList = jdpList;
        this.jconsolePath = jconsolePath;
        this.jmxProcPool = jmxPool;
    }
    
    /**
     * Constructor of JdpReceiver.
     * 
     * @param jdpList ListView which includes JDP packet data.
     * @param threadPool ThreadPool which processes JDP packet decording.
     * @param jconsolePath Path to JConsole.
     * @param jmxPool ThreadPool which processes JMX access.
     * 
     * @throws UnknownHostException 
     */
    public JdpReceiver(ListView<JdpDecoder> jdpList, ExecutorService threadPool, Optional<String> jconsolePath, ExecutorService jmxPool) throws UnknownHostException{
        this(InetAddress.getByName(Optional.ofNullable(System.getProperty(JDP_ADDRESS_PROP_NAME)).orElse(JDP_DEFAULT_ADDRESS)),
                   Optional.ofNullable(System.getProperty(JDP_PORT_PROP_NAME)).map(p -> Integer.parseInt(p)).orElse(JDP_DEFAULT_PORT),
                   jdpList, threadPool, jconsolePath, jmxPool);
    }
    
    @Override
    protected Void call() throws Exception {
        
        try(DatagramChannel jdpChannel = DatagramChannel.open(StandardProtocolFamily.INET)){
            jdpChannel.bind(new InetSocketAddress(jdpPort));
            jdpChannel.setOption(StandardSocketOptions.SO_REUSEADDR, true);

            Collections.list(NetworkInterface.getNetworkInterfaces()).stream()
                                                                     .filter(i -> i.getInterfaceAddresses().stream()
                                                                                                           .anyMatch(n -> n.getAddress() instanceof Inet4Address))
                                                                     .filter(new PredicateWrapper<>(i -> i.supportsMulticast()))
                                                                     .peek(new ConsumerWrapper<>(i ->jdpChannel.setOption(StandardSocketOptions.IP_MULTICAST_IF, i)))
                                                                     .forEach(new ConsumerWrapper<>(i -> jdpChannel.join(jdpAddr, i)));

            while(!isCancelled()){
                ByteBuffer buf = ByteBuffer.allocateDirect(UDP_PACKET_LENGTH);
                SocketAddress src = jdpChannel.receive(buf);
                
                buf.flip();
                jdpProcPool.submit(new JdpDecoder((InetSocketAddress)src, buf, this.jdpList, jconsolePath, jmxProcPool));
            }
            
        }
        catch(ClosedByInterruptException ex){
            // Do nothing.
            // This exception may be occurred by thread interruption.
        }

        return null;
    }
    
}

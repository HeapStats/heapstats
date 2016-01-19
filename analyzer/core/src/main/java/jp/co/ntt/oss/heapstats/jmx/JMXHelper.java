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
package jp.co.ntt.oss.heapstats.jmx;

import java.io.IOException;
import java.io.UncheckedIOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.MalformedURLException;
import java.nio.ByteBuffer;
import java.nio.channels.AsynchronousServerSocketChannel;
import java.nio.channels.AsynchronousSocketChannel;
import java.nio.channels.SeekableByteChannel;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import javax.management.MBeanServerConnection;
import javax.management.MBeanServerInvocationHandler;
import javax.management.MalformedObjectNameException;
import javax.management.ObjectName;
import javax.management.remote.JMXConnector;
import javax.management.remote.JMXConnectorFactory;
import javax.management.remote.JMXServiceURL;
import jp.co.ntt.oss.heapstats.lambda.SupplierWrapper;
import jp.co.ntt.oss.heapstats.mbean.HeapStatsMBean;

/**
 * Helper class for JMX access to HeapStats agent.
 * @author Yasumasa Suenaga
 */
public class JMXHelper implements AutoCloseable{
    
    public static final String DEFAULT_OBJECT_NAME = "heapstats:type=HeapStats";
    
    private JMXServiceURL url;
    
    private JMXConnector connector;
    
    private final HeapStatsMBean mbean;
    
    /**
     * Constructor of JMXHelper.
     * 
     * @param url JMX URL to connect.
     * 
     * @throws MalformedURLException
     * @throws IOException
     * @throws MalformedObjectNameException 
     */
    public JMXHelper(String url) throws MalformedURLException, IOException, MalformedObjectNameException{
        this(new JMXServiceURL(url));
    }
    
    /**
     * Constructor of JMXHelper.
     * @param url JMX URL to connect.
     * 
     * @throws IOException
     * @throws MalformedObjectNameException 
     */
    public JMXHelper(JMXServiceURL url) throws IOException, MalformedObjectNameException{
        this.url = url;
        connector = JMXConnectorFactory.connect(url);
        MBeanServerConnection connection = connector.getMBeanServerConnection();
        mbean = MBeanServerInvocationHandler.newProxyInstance(connection, new ObjectName(DEFAULT_OBJECT_NAME), HeapStatsMBean.class, false);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void close() throws IOException {
        connector.close();
    }

    /**
     * Get HeapStats MBean instance.
     * 
     * @return Instance of HeapStatsMBean.
     */
    public HeapStatsMBean getMbean() {
        return mbean;
    }
    
    private void receiveFile(Path path, AsynchronousSocketChannel socket){
        try(SeekableByteChannel ch = Files.newByteChannel(path, StandardOpenOption.CREATE, StandardOpenOption.WRITE)){
            ByteBuffer buffer = ByteBuffer.allocate(1024);

            while(socket.read(buffer).get() != -1){
                buffer.flip();
                ch.write(buffer);
                buffer.rewind();
            }

        } catch (IOException ex) {
            throw new UncheckedIOException(ex);
        } catch (InterruptedException | ExecutionException ex) {
            throw new RuntimeException(ex);
        }
    }

    private void getFileInternal(Path path, boolean isSnapShot) throws IOException, InterruptedException, ExecutionException{
        try(AsynchronousServerSocketChannel server = AsynchronousServerSocketChannel.open()){
            server.bind(new InetSocketAddress(InetAddress.getLocalHost(), 0));
            CompletableFuture<Void> receiveFuture = CompletableFuture.supplyAsync(new SupplierWrapper<>(() -> server.accept().get()))
                                                                     .thenAccept(s -> receiveFile(path, s));
            InetSocketAddress addr = (InetSocketAddress)server.getLocalAddress();

            if(isSnapShot){
                mbean.getSnapShot(addr.getAddress(), addr.getPort());
            }
            else{
                mbean.getResourceLog(addr.getAddress(), addr.getPort());
            }

            receiveFuture.get();
        }
    }
    
    /**
     * Get SnapShot from remote.
     * 
     * @param path Path to save file.
     * 
     * @throws IOException
     * @throws InterruptedException
     * @throws ExecutionException 
     */
    public void getSnapShot(Path path) throws IOException, InterruptedException, ExecutionException{
        getFileInternal(path, true);
    }
    
    /**
     * Get Resource log from remote.
     * 
     * @param path Path to save file.
     * 
     * @throws IOException
     * @throws InterruptedException
     * @throws ExecutionException 
     */
    public void getResourceLog(Path path) throws IOException, InterruptedException, ExecutionException{
        getFileInternal(path, false);
    }
    
    /**
     * Change configuration of remote HeapStats.
     * 
     * @param key Key name.
     * @param value Value string.
     */
    public void changeConfigurationThroughString(String key, String value){
        Object currentObj = mbean.getConfiguration(key);
        Object newObj = value;
        
        if(currentObj instanceof Integer){
            newObj = Integer.valueOf(value);
        }
        else if(currentObj instanceof Long){
            newObj = Long.valueOf(value);
        }
        else if(currentObj instanceof Boolean){
            newObj = Boolean.valueOf(value);
        }
        else if(currentObj instanceof HeapStatsMBean.LogLevel){
            newObj = HeapStatsMBean.LogLevel.valueOf(value);
        }
        else if(currentObj instanceof HeapStatsMBean.RankOrder){
            newObj = HeapStatsMBean.RankOrder.valueOf(value);
        }
        
        mbean.changeConfiguration(key, newObj);
    }

    /**
     * Get JMX URL.
     * 
     * @return JMX URL
     */
    public JMXServiceURL getUrl() {
        return url;
    }
    
}

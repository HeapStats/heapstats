/*
 * HeapStats.java
 *
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
 *
 */

package jp.co.ntt.oss.heapstats.mbean;

import java.io.File;
import java.io.IOException;
import java.io.Closeable;
import java.io.FileInputStream;
import java.nio.channels.FileChannel;
import java.nio.channels.SocketChannel;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.util.Map;


/**
 * Implementation of HeapStatsMBean
 *
 * @author Yasumasa Suenaga
 */
public class HeapStats implements HeapStatsMBean{

  static{
    System.loadLibrary("heapstats-mbean");
    registerNatives();
  }

  /**
   * Register JNI functions in libheapstats.
   */ 
  public static native void registerNatives();

  /**
   * Get HeapStats version string from libheapstats.
   *
   * @return Version string which is attached.
   */
  private native String getHeapStatsVersion0();

  /**
   * Get HeapStats agent configuration from libheapstats.
   *
   * @param key Name of configuration.
   * @return Current value of configuration key.
   */
  private native Object getConfiguration0(String key);

  /**
   * Get all configurations in libheapstats.
   * Key is configuration name in heapstats.conf, value is current value.
   *
   * @return Current configuration list.
   */
  private native Map<String, Object> getConfigurationList0();

  /**
   * Change HeapStats agent configuration in libheapstats.
   *
   * @param key   Name of configuration.
   * @param value New configuration value.
   * @return Result of this change.
   */
  private native boolean changeConfiguration0(String key, Object value);

  /**
   * Invoke Resource Log collection at libheapstats.
   *
   * @return Result of this call.
   */
  private native boolean invokeLogCollection0();

  /**
   * Invoke All Log collection at libheapstats.
   *
   * @return Result of this call.
   */
  private native boolean invokeAllLogCollection0();

  /**
   * {@inheritDoc}
   */
  @Override
  public String getHeapStatsVersion(){
    return getHeapStatsVersion0();
  }

  /**
   * Close Closeable resource silently.
   *
   * @param resource Resource to be closed.
   */
  private void closeSilently(Closeable resource){
    try{
      resource.close();
    }
    catch(NullPointerException e){
      // Do nothing
    }
    catch(Exception e){
      e.printStackTrace();
    }
  }

  /**
   * Send file through socket.
   *
   * @param fname   Filename to be sent.
   * @param address InetAddress of receiver.
   * @param port    Port number of receiver.
   */
  private void sendFile(String fname, InetAddress address, int port){
    File file = new File(fname);
    if(!file.isFile()){
      throw new RuntimeException(new IOException(fname + " does not exist."));
    }

    FileInputStream stream = null;
    SocketChannel sock = null;
    InetSocketAddress remote = new InetSocketAddress(address, port);
    try{
      stream = new FileInputStream(file);
      FileChannel ch = stream.getChannel();
      sock = SocketChannel.open(remote);

      ch.transferTo(0, file.length(), sock);
    }
    catch(IOException e){
      throw new RuntimeException(e);
    }
    finally{
      closeSilently(stream);
      closeSilently(sock);
    }

  }

  /**
   * {@inheritDoc}
   */
  @Override
  public void getSnapShot(InetAddress address, int port){
    sendFile((String)getConfiguration("file"), address, port);
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public void getResourceLog(InetAddress address, int port){
    sendFile((String)getConfiguration("heaplogfile"), address, port);
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public Object getConfiguration(String key){
    return getConfiguration0(key);
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public Map<String, Object> getConfigurationList(){
    return getConfigurationList0();
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public boolean changeConfiguration(String key, Object value){
    return changeConfiguration0(key, value);
  }

  /**
   * Call System.gc() to get Heap SnapShot.
   */
  @Override
  public void invokeSnapShotCollection(){
    // Invoke GC to collect SnapShot.
    System.gc();
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public boolean invokeLogCollection(){
    return invokeLogCollection0();
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public boolean invokeAllLogCollection(){
    return invokeAllLogCollection0();
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public void create() throws Exception{
    // Do nothing;
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public void start() throws Exception{
    // Do nothing;
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public void stop() throws Exception{
    // Do nothing;
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public void destroy() throws Exception{
    // Do nothing;
  }

}


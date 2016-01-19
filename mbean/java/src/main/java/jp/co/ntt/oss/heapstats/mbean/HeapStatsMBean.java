/*
 * HeapStatsMBean.java
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

import java.net.InetAddress;
import java.util.Map;


/**
 * MBean interface for HeapStats.
 *
 * @author Yasumasa Suenaga
 */
public interface HeapStatsMBean{

  /**
   * Enumeration of log level.
   * These values links loglevel directive in heapstats.conf .
   */
  public static enum LogLevel{
    UNKNOWN,
    CRIT,
    WARN,
    INFO,
    DEBUG
  }

  /**
   * Enumeration of rank order.
   * These values links rank_order directive in heapstats.conf .
   */
  public static enum RankOrder{
    DELTA,
    USAGE
  }

  /**
   * Get HeapStats version string.
   *
   * @return Version string which is attached.
   */
  public String getHeapStatsVersion();

  /**
   * Get SnapShot data through socket.
   *
   * @param address InetAddress of receiver.
   * @param port    Port number of receiver.
   */
  public void getSnapShot(InetAddress address, int port);

  /**
   * Get Resource log data (CSV) through socket.
   *
   * @param address InetAddress of receiver.
   * @param port    Port number of receiver.
   */
  public void getResourceLog(InetAddress address, int port);

  /**
   * Get HeapStats agent configuration.
   *
   * @param key Name of configuration.
   * @return Current value of configuration key.
   */
  public Object getConfiguration(String key);

  /**
   * Get all configurations in HeapStats agent.
   * Key is configuration name in heapstats.conf, value is current value.
   *
   * @return Current configuration list.
   */
  public Map<String, Object> getConfigurationList();

  /**
   * Change HeapStats agent configuration.
   *
   * @param key   Name of configuration.
   * @param value New configuration value.
   * @return Result of this change.
   */
  public boolean changeConfiguration(String key, Object value);

  /**
   * Invoke Heap SnapShot collection.
   * This function might just call System.gc() .
   */
  public void invokeSnapShotCollection();

  /**
   * Invoke Resource Log collection.
   *
   * @return Result of this call.
   */
  public boolean invokeLogCollection();

  /**
   * Invoke All Log collection.
   *
   * @return Result of this call.
   */
  public boolean invokeAllLogCollection();

  /**
   * This function is for WildFly/JBoss.
   * @throws java.lang.Exception
   */
  public void create() throws Exception;

  /**
   * This function is for WildFly/JBoss.
   * @throws java.lang.Exception
   */
  public void start() throws Exception;

  /**
   * This function is for WildFly/JBoss.
   * @throws java.lang.Exception
   */
  public void stop() throws Exception;

  /**
   * This function is for WildFly/JBoss.
   * @throws java.lang.Exception
   */
  public void destroy() throws Exception;

}


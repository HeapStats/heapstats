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

package jp.co.ntt.oss.heapstats.plugin.builtin.jvmlive;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.NoSuchFileException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.util.Properties;
import jp.co.ntt.oss.heapstats.plugin.builtin.jvmlive.errorreporter.ErrorReportServer;
import jp.co.ntt.oss.heapstats.utils.HeapStatsConfigException;
import jp.co.ntt.oss.heapstats.utils.HeapStatsUtils;

/**
 * Configuration of JVMLive.
 * 
 * @author Yasumasa Suenaga
 */
public class JVMLiveConfig {
    
    public static final String JDP_WAIT_DURATION_KEY = "jdp.waitDuration";

    public static final String THREADPOOL_SHOTDOWN_AWAIT_TIME_KEY = "threadpool.shotdownAwaitTime";
    
    public static final String ERRORREPORT_SERVER_PORT_KEY = "errorreport.server.port";

    private static final Properties prop = new Properties();
    
    /**
     * Check and set value to JVMListConfig.
     * 
     * @param key configuration key.
     * @param defaultValue default value of configuration key.
     * @throws HeapStatsConfigException 
     */
    private static void setupNumericProperty(String key, int defaultValue) throws HeapStatsConfigException{
        String valStr = prop.getProperty(key);
        
        if(valStr == null){
            prop.setProperty(key, Integer.toString(defaultValue));
        }
        else{
            try{
                Integer.decode(valStr);
            }
            catch(NumberFormatException e){
                throw new HeapStatsConfigException(String.format("Invalid option: %s=%s", key, valStr), e);
            }
        }
        
    }
    
    /**
     * Load configuration from &lt;HeapStats home directory&gt;/jvmlive.properties .
     * 
     * @throws IOException
     * @throws HeapStatsConfigException 
     */
    public static void load() throws IOException, HeapStatsConfigException{
        Path properties = Paths.get(HeapStatsUtils.getHeapStatsHomeDirectory().toString(), "jvmlive.properties");
        
        try(InputStream in = Files.newInputStream(properties, StandardOpenOption.READ)){
            prop.load(in);
        }
        catch(NoSuchFileException e){
            // use default values.
        }
        
        setupNumericProperty(JDP_WAIT_DURATION_KEY, 3);
        setupNumericProperty(THREADPOOL_SHOTDOWN_AWAIT_TIME_KEY, 5);
        setupNumericProperty(ERRORREPORT_SERVER_PORT_KEY, ErrorReportServer.DEFAULT_ERROR_REPORT_SERVER_PORT);
    }
    
    /**
     * Wait duration for JDP packet.
     * This value uses as duration for JdpValidatorService.
     * 
     * @return Wait duration for JDP packet.
     */
    public static int getJdpWaitDuration(){
        return Integer.parseInt(prop.getProperty(JDP_WAIT_DURATION_KEY));
    }
    
    /**
     * @return Await time for threadpool shutdown.
     */
    public static int getThreadpoolShutdownAwaitTime(){
        return Integer.parseInt(prop.getProperty(THREADPOOL_SHOTDOWN_AWAIT_TIME_KEY));
    }
    
    /**
     * @return Port for error report receiver.
     */
    public static int getErrorReportServerPort(){
        return Integer.parseInt(prop.getProperty(ERRORREPORT_SERVER_PORT_KEY));
    }
    
}

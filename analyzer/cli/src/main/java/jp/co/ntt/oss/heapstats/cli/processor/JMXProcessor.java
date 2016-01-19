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

import java.io.IOException;
import java.io.UncheckedIOException;
import java.util.concurrent.ExecutionException;
import javax.management.MalformedObjectNameException;
import jp.co.ntt.oss.heapstats.cli.Options;
import jp.co.ntt.oss.heapstats.jmx.JMXHelper;

/**
 * CLI task for JMX access.
 * 
 * @author Yasumasa Suenaga
 */
public class JMXProcessor implements CliProcessor{
    
    private final Options options;

    /**
     * Constructor of JMXProcessor.
     * 
     * @param options Options to use.
     */
    public JMXProcessor(Options options) {
        this.options = options;
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public void process() {
        try(JMXHelper jmx = new JMXHelper(options.getJmxURL())){
            switch(options.getMode()){
                case JMX_GET_VERSION:
                    System.out.println(jmx.getMbean().getHeapStatsVersion());
                    break;
                case JMX_GET_SNAPSHOT:
                    jmx.getSnapShot(options.getFile().get(0));
                    System.out.println("SnapShot saved to " + options.getFile().get(0));
                    break;
                case JMX_GET_LOG:
                    jmx.getResourceLog(options.getFile().get(0));
                    System.out.println("Resource log saved to " + options.getFile().get(0));
                    break;
                case JMX_GET_CONFIG:
                    System.out.println(jmx.getMbean().getConfiguration(options.getConfigKey()));
                    break;
                case JMX_CHANGE_CONFIG:
                    System.out.print(options.getConfigKey() + ": " + jmx.getMbean().getConfiguration(options.getConfigKey()) + " -> ");
                    jmx.changeConfigurationThroughString(options.getConfigKey(), options.getNewConfigValue());
                    System.out.println(jmx.getMbean().getConfiguration(options.getConfigKey()));
                    break;
                case JMX_INVOKE_SNAPSHOT:
                    jmx.getMbean().invokeSnapShotCollection();
                    break;
                case JMX_INVOKE_LOG:
                    jmx.getMbean().invokeLogCollection();
                    break;
                case JMX_INVOKE_ALL_LOG:
                    jmx.getMbean().invokeAllLogCollection();
                    break;
            }
            
        }
        catch(IOException ex){
            throw new UncheckedIOException(ex);
        } catch (MalformedObjectNameException | InterruptedException | ExecutionException ex) {
            throw new RuntimeException(ex);
        }
    }
    
}

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

import jp.co.ntt.oss.heapstats.cli.processor.CliProcessor;
import jp.co.ntt.oss.heapstats.cli.processor.JMXProcessor;
import jp.co.ntt.oss.heapstats.cli.processor.LogProcessor;
import jp.co.ntt.oss.heapstats.cli.processor.SnapShotProcessor;
import jp.co.ntt.oss.heapstats.cli.processor.ThreadRecordProcessor;

/**
 * Main class of HeapStats CLI
 * 
 * @author Yasumasa Suenaga
 */
public class CliMain {

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        Options options = new Options();
        
        try{
            options.parse(args);
        }
        catch(Exception e){
            options.printHelp();
            System.exit(1);
        }
        
        CliProcessor processor;
        Options.FileType fileType = options.getType();

        if(fileType == null){
            options.printHelp();
            return;
        }
        
        switch(fileType){
            case LOG:
                processor = new LogProcessor(options);
                break;
            case SNAPSHOT:
                processor = new SnapShotProcessor(options);
                break;
            case THREADRECORD:
                processor = new ThreadRecordProcessor(options);
                break;
            case JMX:
                processor = new JMXProcessor(options);
                break;
            default:
                throw new IllegalStateException("Unexpected file mode: " + fileType.toString());
        }
        
        processor.process();        
    }
    
}

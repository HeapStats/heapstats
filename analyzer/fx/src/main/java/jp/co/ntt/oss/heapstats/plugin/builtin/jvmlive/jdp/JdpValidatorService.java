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

import java.time.LocalDateTime;
import javafx.application.Platform;
import javafx.concurrent.ScheduledService;
import javafx.concurrent.Task;
import javafx.scene.control.ListView;
import jp.co.ntt.oss.heapstats.plugin.builtin.jvmlive.JVMLiveConfig;

/**
 * Validation service class for JdpDecoder.
 * This class should be run on ScheduledService.
 * 
 * The task which is provided from this class checkes all JdpDecoder in ListView.
 * If JDP Packet is not received until JDP packet interval, the Task change its
 * color to orange.
 * 
 * @author Yasumasa Suenaga
 */
public class JdpValidatorService extends ScheduledService<Void>{
    
    private final ListView<JdpDecoder> jdpList;

    public JdpValidatorService(ListView<JdpDecoder> jdpList) {
        this.jdpList = jdpList;
    }

    @Override
    protected Task<Void> createTask() {
        return new JdpValidationTask();
    }
    
    class JdpValidationTask extends Task<Void>{
        
        private void swapJdpDecoder(int index){
            JdpDecoder target = jdpList.getItems().get(index);
            target.setInvalidate();
            
            jdpList.getItems().remove(index);
            jdpList.getItems().add(index, target);
        }

        @Override
        protected Void call() throws Exception {
            Platform.runLater(() -> {
                                       LocalDateTime now = LocalDateTime.now();
                                       jdpList.getItems().stream()
                                                         .filter(d -> !d.invalidateProperty().get())
                                                         .filter(d -> d.getReceivedTime().plusSeconds(d.getBroadcastInterval() / 1000 + JVMLiveConfig.getJdpWaitDuration()).isBefore(now))
                                                         .mapToInt(d -> jdpList.getItems().indexOf(d))
                                                         .forEach(i -> swapJdpDecoder(i));
                                    });

            return null;
        }
        
    }
    
}

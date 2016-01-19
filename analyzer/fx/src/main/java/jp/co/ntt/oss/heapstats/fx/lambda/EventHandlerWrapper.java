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
package jp.co.ntt.oss.heapstats.fx.lambda;

import javafx.event.Event;
import javafx.event.EventHandler;

/**
 * Wrapper for EventHandler interface which has check exception.
 * @author Yasumasa Suenaga
 */
public class EventHandlerWrapper<T extends Event> implements EventHandler<T>{
    
    private final ExceptionalEventHandler<T> eventHandler;

    public EventHandlerWrapper(ExceptionalEventHandler<T> eventHandler) {
        this.eventHandler = eventHandler;
    }

    @Override
    public void handle(T event) {
        try{
            eventHandler.handle(event);
        }
        catch(Exception e){
            throw new RuntimeException(e);
        }
    }
    
}

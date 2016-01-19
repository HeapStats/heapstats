/*
 * Copyright (C) 2015 Takashi Aoe
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
package jp.co.ntt.oss.heapstats.plugin.builtin.threadrecorder;

import javafx.geometry.Pos;
import javafx.scene.control.ContentDisplay;
import javafx.scene.control.TableCell;
import javafx.scene.layout.HBox;
import jp.co.ntt.oss.heapstats.container.threadrecord.ThreadStat;

import java.time.LocalDateTime;
import java.util.List;
import javafx.beans.property.ObjectProperty;

/**
 * Table cell for thread timeline.
 */
public class TimelineCell extends TableCell<ThreadStatViewModel, List<ThreadStat>> {
    
    private final ObjectProperty<LocalDateTime> rangeStart;
    
    private final ObjectProperty<LocalDateTime> rangeEnd;

    /**
     * Constructor of TielineCell.
     * 
     * @param rangeStart Start time.
     * @param rangeEnd End time.
     */
    public TimelineCell(ObjectProperty<LocalDateTime> rangeStart, ObjectProperty<LocalDateTime> rangeEnd) {
        setContentDisplay(ContentDisplay.GRAPHIC_ONLY);
        setAlignment(Pos.CENTER_LEFT);

        this.rangeStart = rangeStart;
        this.rangeEnd = rangeEnd;
    }

    @Override
    protected void updateItem(List<ThreadStat> item, boolean empty) {
        super.updateItem(item, empty);
        ThreadStatViewModel model = (ThreadStatViewModel)getTableRow().getItem();
        
        if (empty || (item == null) || item.isEmpty() || (model == null)) {
            updateToEmptyCell();
        } else {
            drawTimeline(model);
        }
        
    }

    private void drawTimeline(ThreadStatViewModel viewModel) {
        TimelineGenerator generator = new TimelineGenerator(viewModel, getTableColumn().prefWidthProperty());
        HBox container = generator.createTimeline(rangeStart.get(), rangeEnd.get());
        
        container.visibleProperty().bind(viewModel.showProperty());
        setGraphic(container);
    }

    private void updateToEmptyCell() {
        setText(null);
        setGraphic(null);
    }

}

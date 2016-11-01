/*
 * Copyright (C) 2016 Yasumasa Suenaga
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
package jp.co.ntt.oss.heapstats.plugin.builtin.log.tabs;

import java.net.URL;
import java.util.List;
import java.util.ResourceBundle;
import java.util.function.Function;
import java.util.stream.Collectors;
import javafx.concurrent.Task;
import javafx.scene.chart.CategoryAxis;
import javafx.scene.chart.LineChart;
import javafx.scene.chart.NumberAxis;
import javafx.scene.chart.StackedAreaChart;
import javafx.scene.chart.XYChart;
import javafx.scene.input.MouseEvent;
import jp.co.ntt.oss.heapstats.container.log.DiffData;
import jp.co.ntt.oss.heapstats.container.log.LogData;

/**
 * FXML Controller class for "Resource Data" tab in LogData plugin.
 */
public class LogResourcesCategoryController extends LogResourcesControllerBase {
    
    /**
     * {@inheritDoc}
     */
    @Override
    public void initialize(URL url, ResourceBundle rb) {
        javaCPUChart = new StackedAreaChart<>(new CategoryAxis(), new NumberAxis("%", 0.0d, 100.d, 10.0d));
        systemCPUChart = new StackedAreaChart<>(new CategoryAxis(), new NumberAxis("%", 0.0d, 100.d, 10.0d));
        javaMemoryChart = new LineChart<>(new CategoryAxis(), new NumberAxis());
        safepointChart = new LineChart<>(new CategoryAxis(), new NumberAxis());
        safepointTimeChart = new LineChart<>(new CategoryAxis(), new NumberAxis());
        monitorChart = new LineChart<>(new CategoryAxis(), new NumberAxis());
        threadChart = new LineChart<>(new CategoryAxis(), new NumberAxis());

        super.initialize(url, rb);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    @SuppressWarnings("unchecked")
    protected void showChartPopup(XYChart chart, Object xValue, MouseEvent event, Function<? super Number, String> labelFunc) {
        XYChart<String, Number> pChart = (XYChart<String, Number>)chart;
        String label = pChart.getData()
                             .stream()
                             .map(s -> s.getName() + " = " + s.getData()
                                                              .stream()
                                                              .filter(d -> d.getXValue().equals(xValue))
                                                              .map(d -> labelFunc.apply(d.getYValue()))
                                                              .findAny()
                                                              .orElse("<none>"))
                             .collect(Collectors.joining("\n"));
        
        popupText.setText(xValue.toString() + "\n" + label);
        chartPopup.show(chart, event.getScreenX() + 15.0d, event.getScreenY() + 3.0d);
    }

    /**
     * {@inheritDoc}
     */
    private class DrawLogCategoryChartTask extends LogResourcesControllerBase.DrawLogChartTask {
        
        public DrawLogCategoryChartTask(List<LogData> targetLogData, List<DiffData> targetDiffData) {
            super(targetLogData, targetDiffData);
        }

        @Override
        protected void setChartRange() {
            // Do nothing
        }

    }

    /**
     * @{inheritDoc}
     */
    @Override
    public Task<Void> createDrawResourceCharts(List<LogData> targetLogData, List<DiffData> targetDiffData) {
        return new DrawLogCategoryChartTask(targetLogData, targetDiffData);
    }

}

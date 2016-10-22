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
import java.time.ZoneId;
import java.util.List;
import java.util.ResourceBundle;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import javafx.concurrent.Task;
import javafx.scene.chart.LineChart;
import javafx.scene.chart.NumberAxis;
import javafx.scene.chart.StackedAreaChart;
import javafx.scene.chart.XYChart;
import javafx.scene.input.MouseEvent;
import jp.co.ntt.oss.heapstats.container.log.DiffData;
import jp.co.ntt.oss.heapstats.container.log.LogData;
import jp.co.ntt.oss.heapstats.utils.EpochTimeConverter;
import jp.co.ntt.oss.heapstats.utils.HeapStatsUtils;

/**
 * FXML Controller class for "Resource Data" tab in LogData plugin.
 */
public class LogResourcesNumberController extends LogResourcesControllerBase {
    
    private EpochTimeConverter epochTimeConverter;

    /**
     * {@inheritDoc}
     */
    @Override
    public void initialize(URL url, ResourceBundle rb) {
        javaCPUChart = new StackedAreaChart<>(new NumberAxis(), new NumberAxis("%", 0.0d, 100.d, 10.0d));
        systemCPUChart = new StackedAreaChart<>(new NumberAxis(), new NumberAxis("%", 0.0d, 100.d, 10.0d));
        javaMemoryChart = new LineChart<>(new NumberAxis(), new NumberAxis());
        safepointChart = new LineChart<>(new NumberAxis(), new NumberAxis());
        safepointTimeChart = new LineChart<>(new NumberAxis(), new NumberAxis());
        monitorChart = new LineChart<>(new NumberAxis(), new NumberAxis());
        threadChart = new LineChart<>(new NumberAxis(), new NumberAxis());
        
        Stream.of(javaCPUChart, systemCPUChart, javaMemoryChart, safepointChart, safepointTimeChart, monitorChart, threadChart)
              .peek(c -> ((NumberAxis)c.getXAxis()).setMinorTickVisible(false))
              .forEach(c -> c.getXAxis().setAutoRanging(false));
        
        epochTimeConverter = new EpochTimeConverter();

        super.initialize(url, rb);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    @SuppressWarnings("unchecked")
    protected void showChartPopup(XYChart chart, Object xValue, MouseEvent event, Function<? super Number, String> labelFunc) {
        boolean isContained = ((XYChart<Number, Number>)chart).getData()
                                                              .stream()
                                                              .flatMap(s -> s.getData().stream())
                                                              .anyMatch(d -> d.getXValue().longValue() == ((Number)xValue).longValue());
        if(isContained){
            String label = ((XYChart<Number, Number>)chart).getData()
                                                           .stream()
                                                           .map(s -> s.getName() + " = " + s.getData()
                                                                                            .stream()
                                                                                            .filter(d -> d.getXValue().longValue() == ((Number)xValue).longValue())
                                                                                            .map(d -> labelFunc.apply(d.getYValue()))
                                                                                            .findAny()
                                                                                            .get())
                                                           .collect(Collectors.joining("\n"));
            popupText.setText(epochTimeConverter.toString((Number)xValue) + "\n" + label);
            chartPopup.show(chart, event.getScreenX() + 15.0d, event.getScreenY() + 3.0d);
        }
    }

    /**
     * {@inheritDoc}
     */
    private class DrawLogNumberChartTask extends LogResourcesControllerBase.DrawLogChartTask {
        
        public DrawLogNumberChartTask(List<LogData> targetLogData, List<DiffData> targetDiffData) {
            super(targetLogData, targetDiffData);
        }

        @Override
        protected void setChartRange() {
            long startLogEpoch = targetLogData.get(0).getDateTime().atZone(ZoneId.systemDefault()).toEpochSecond();
            long endLogEpoch = targetLogData.get(targetLogData.size() - 1).getDateTime().atZone(ZoneId.systemDefault()).toEpochSecond();
            long startDiffEpoch = targetDiffData.get(0).getDateTime().atZone(ZoneId.systemDefault()).toEpochSecond();
            long endDiffEpoch = targetDiffData.get(targetDiffData.size() - 1).getDateTime().atZone(ZoneId.systemDefault()).toEpochSecond();
            Stream.of(javaMemoryChart, threadChart)
                  .map(c -> (NumberAxis)c.getXAxis())
                  .peek(a -> a.setTickUnit((endLogEpoch - startLogEpoch) / HeapStatsUtils.getXTickUnit()))
                  .peek(a -> a.setLowerBound(startLogEpoch))
                  .forEach(a -> a.setUpperBound(endLogEpoch));
            Stream.of(javaCPUChart, systemCPUChart, safepointChart, safepointTimeChart, monitorChart)
                  .map(c -> (NumberAxis)c.getXAxis())
                  .peek(a -> a.setTickUnit((endDiffEpoch - startDiffEpoch) / HeapStatsUtils.getXTickUnit()))
                  .peek(a -> a.setLowerBound(startDiffEpoch))
                  .forEach(a -> a.setUpperBound(endDiffEpoch));
        }

    }

    /**
     * @{inheritDoc}
     */
    @Override
    public Task<Void> createDrawResourceCharts(List<LogData> targetLogData, List<DiffData> targetDiffData) {
        return new DrawLogNumberChartTask(targetLogData, targetDiffData);
    }

}

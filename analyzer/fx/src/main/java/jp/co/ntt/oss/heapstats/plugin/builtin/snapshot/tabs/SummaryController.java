/*
 * Copyright (C) 2015-2016 Nippon Telegraph and Telephone Corporation
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
package jp.co.ntt.oss.heapstats.plugin.builtin.snapshot.tabs;

import java.net.URL;
import java.time.ZoneId;
import java.util.Locale;
import java.util.ResourceBundle;
import java.util.function.Consumer;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import javafx.application.Platform;
import javafx.beans.property.ObjectProperty;
import javafx.beans.property.SimpleObjectProperty;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.collections.ObservableSet;
import javafx.concurrent.Task;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.chart.AreaChart;
import javafx.scene.chart.LineChart;
import javafx.scene.chart.NumberAxis;
import javafx.scene.chart.StackedAreaChart;
import javafx.scene.chart.XYChart;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableView;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.scene.layout.AnchorPane;
import javafx.util.StringConverter;
import jp.co.ntt.oss.heapstats.container.snapshot.SnapShotHeader;
import jp.co.ntt.oss.heapstats.container.snapshot.SummaryData;
import jp.co.ntt.oss.heapstats.utils.EpochTimeConverter;
import jp.co.ntt.oss.heapstats.utils.HeapStatsUtils;

/**
 * FXML Controller class for "Summary Data" tab in SnapShot plugin.
 */
public class SummaryController implements Initializable {

    @FXML
    private TableView<SummaryData.SummaryDataEntry> summaryTable;

    @FXML
    private TableColumn<SummaryData.SummaryDataEntry, String> keyColumn;

    @FXML
    private TableColumn<SummaryData.SummaryDataEntry, String> valueColumn;

    @FXML
    private StackedAreaChart<Number, Long> heapChart;

    private XYChart.Series<Number, Long> youngUsage;

    private XYChart.Series<Number, Long> oldUsage;

    private XYChart.Series<Number, Long> free;

    @FXML
    private LineChart<Number, Long> instanceChart;

    private XYChart.Series<Number, Long> instances;

    @FXML
    private LineChart<Number, Long> gcTimeChart;

    private XYChart.Series<Number, Long> gcTime;

    @FXML
    private AreaChart<Number, Long> metaspaceChart;

    private XYChart.Series<Number, Long> metaspaceUsage;

    private XYChart.Series<Number, Long> metaspaceCapacity;

    private ObjectProperty<SummaryData> summaryData;

    private ObjectProperty<ObservableList<SnapShotHeader>> currentTarget;

    private ObjectProperty<ObservableSet<String>> currentClassNameSet;

    /**
     * Initializes the controller class.
     */
    @Override
    public void initialize(URL url, ResourceBundle rb) {
        summaryData = new SimpleObjectProperty<>();
        summaryData.addListener((v, o, n) -> setSummaryTable(n));
        currentTarget = new SimpleObjectProperty<>(FXCollections.emptyObservableList());
        currentClassNameSet = new SimpleObjectProperty<>(FXCollections.emptyObservableSet());

        keyColumn.setCellValueFactory(new PropertyValueFactory<>("category"));
        valueColumn.setCellValueFactory(new PropertyValueFactory<>("value"));

        String bgcolor = "-fx-background-color: " + HeapStatsUtils.getChartBgColor() + ";";
        Stream.of(heapChart, instanceChart, gcTimeChart, metaspaceChart)
              .peek(c -> c.lookup(".chart").setStyle(bgcolor))
              .forEach(c -> c.getXAxis().setTickMarkVisible(HeapStatsUtils.getTickMarkerSwitch()));

        StringConverter<Number> converter = new EpochTimeConverter();
        Stream.of(heapChart, instanceChart, gcTimeChart, metaspaceChart)
              .map(c -> (NumberAxis)c.getXAxis())
              .forEach(a -> a.setTickLabelFormatter(converter));
        
        initializeChartSeries();
    }

    private void setSummaryTable(SummaryData data) {
        if (data == null) {
            summaryTable.getItems().clear();
        } else {
            ResourceBundle resource = ResourceBundle.getBundle("snapshotResources", new Locale(HeapStatsUtils.getLanguage()));
            summaryTable.setItems(FXCollections.observableArrayList(new SummaryData.SummaryDataEntry(resource.getString("summary.snapshot.count"), Integer.toString(data.getCount())),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.gc.count"), String.format("%d (Full: %d, Young: %d)", data.getFullCount() + data.getYngCount(), data.getFullCount(), data.getYngCount())),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.heap.usage"), String.format("%.1f MB", data.getLatestHeapUsage() / 1024.0d / 1024.0d)),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.metaspace.usage"), String.format("%.1f MB", data.getLatestMetaspaceUsage() / 1024.0d / 1024.0d)),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.gc.time"), String.format("%d ms", data.getMaxGCTime())),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.snapshot.size"), String.format("%.1f KB", data.getMaxSnapshotSize() / 1024.0d)),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.snapshot.entrycount"), Long.toString(data.getMaxEntryCount()))
            ));
        }
    }

    /**
     * Initialize Series in Chart. This method uses to avoid RuntimeException
     * which is related to: RT-37994: [FXML] ProxyBuilder does not support
     * read-only collections https://javafx-jira.kenai.com/browse/RT-37994
     */
    @SuppressWarnings("unchecked")
    private void initializeChartSeries() {
        youngUsage = new XYChart.Series<>();
        youngUsage.setName("Young");
        oldUsage = new XYChart.Series<>();
        oldUsage.setName("Old");
        free = new XYChart.Series<>();
        free.setName("Free");
        String[] colors = {"blue", "limegreen", "red"};
        if (HeapStatsUtils.getHeapOrder()) {
            heapChart.getData().addAll(youngUsage, oldUsage, free);
        } else {
            heapChart.getData().addAll(oldUsage, youngUsage, free);
            /* swap color order */
            colors[0] = "limegreen";
            colors[1] = "blue";
        }
        /* Set heapChart colors */
        Platform.runLater(() -> {
            for (int i = 0; i < colors.length; i++) {
                heapChart.lookup(".default-color" + i + ".chart-series-area-fill").setStyle(String.format("-fx-fill: %s;", colors[i]));
                heapChart.lookup(".default-color" + i + ".chart-series-area-line").setStyle(String.format("-fx-stroke: %s;", colors[i]));
                heapChart.lookup(".default-color" + i + ".area-legend-symbol").setStyle(String.format("-fx-background-color: %s, white;", colors[i]));
                heapChart.lookup(".default-color" + i + ".chart-area-symbol").setStyle(String.format("-fx-background-color: %s, white;", colors[i]));
            }
        });

        instances = new XYChart.Series<>();
        instances.setName("Instances");
        instanceChart.getData().add(instances);

        gcTime = new XYChart.Series<>();
        gcTime.setName("GC Time");
        gcTimeChart.getData().add(gcTime);

        metaspaceCapacity = new XYChart.Series<>();
        metaspaceCapacity.setName("Capacity");
        metaspaceUsage = new XYChart.Series<>();
        metaspaceUsage.setName("Usage");
        metaspaceChart.getData().addAll(metaspaceCapacity, metaspaceUsage);
    }

    /**
     * JavaFX task class for calculating GC summary.
     */
    private class CalculateGCSummaryTask extends Task<Void> {

        private int processedIndex;

        private final Consumer<XYChart<Number, ? extends Number>> drawRebootSuspectLine;

        /* Java Heap Usage Chart */
        private final ObservableList<XYChart.Data<Number, Long>> youngUsageBuf;
        private final ObservableList<XYChart.Data<Number, Long>> oldUsageBuf;
        private final ObservableList<XYChart.Data<Number, Long>> freeBuf;

        /* Instances */
        private final ObservableList<XYChart.Data<Number, Long>> instanceBuf;

        /* GC time Chart */
        private final ObservableList<XYChart.Data<Number, Long>> gcTimeBuf;

        /* Metaspace Chart */
        private final ObservableList<XYChart.Data<Number, Long>> metaspaceUsageBuf;
        private final ObservableList<XYChart.Data<Number, Long>> metaspaceCapacityBuf;

        /**
         * Constructor of CalculateGCSummaryTask.
         *
         * @param drawRebootSuspectLine Consumer for drawing reboot line. This
         * consumer is called in Platform#runLater() at succeeded().
         */
        public CalculateGCSummaryTask(Consumer<XYChart<Number, ? extends Number>> drawRebootSuspectLine) {
            this.drawRebootSuspectLine = drawRebootSuspectLine;

            youngUsageBuf = FXCollections.observableArrayList();
            oldUsageBuf = FXCollections.observableArrayList();
            freeBuf = FXCollections.observableArrayList();
            instanceBuf = FXCollections.observableArrayList();
            gcTimeBuf = FXCollections.observableArrayList();
            metaspaceUsageBuf = FXCollections.observableArrayList();
            metaspaceCapacityBuf = FXCollections.observableArrayList();
        }

        private void processSnapShotHeader(SnapShotHeader header) {
            long time = header.getSnapShotDate().atZone(ZoneId.systemDefault()).toEpochSecond();

            youngUsageBuf.add(new XYChart.Data<>(time, header.getNewHeap() / 1024 / 1024));
            oldUsageBuf.add(new XYChart.Data<>(time, header.getOldHeap() / 1024 / 1024));
            freeBuf.add(new XYChart.Data<>(time, (header.getTotalCapacity() - header.getNewHeap() - header.getOldHeap()) / 1024 / 1024));

            instanceBuf.add(new XYChart.Data<>(time, header.getNumInstances()));

            gcTimeBuf.add(new XYChart.Data<>(time, header.getGcTime()));

            metaspaceUsageBuf.add(new XYChart.Data<>(time, header.getMetaspaceUsage() / 1024 / 1024));
            metaspaceCapacityBuf.add(new XYChart.Data<>(time, header.getMetaspaceCapacity() / 1024 / 1024));

            currentClassNameSet.get().addAll(header.getSnapShot(HeapStatsUtils.getReplaceClassName())
                    .values()
                    .stream()
                    .map(s -> s.getName())
                    .collect(Collectors.toSet()));

            updateProgress(++processedIndex, currentTarget.get().size());
        }

        @Override
        protected Void call() throws Exception {
            updateMessage("Calcurating GC summary...");
            processedIndex = 0;
            currentTarget.get().stream()
                    .forEachOrdered(d -> processSnapShotHeader(d));
            return null;
        }

        @Override
        protected void succeeded() {
            long startEpoch = currentTarget.get().get(0).getSnapShotDate().atZone(ZoneId.systemDefault()).toEpochSecond();
            long endEpoch = currentTarget.get().get(currentTarget.get().size() - 1).getSnapShotDate().atZone(ZoneId.systemDefault()).toEpochSecond();
            Stream.of(heapChart, instanceChart, gcTimeChart, metaspaceChart)
                  .peek(c -> ((NumberAxis)c.getXAxis()).setLowerBound(startEpoch))
                  .peek(c -> ((NumberAxis)c.getXAxis()).setUpperBound(endEpoch))
                  .forEach(c -> Platform.runLater(() -> drawRebootSuspectLine.accept(c)));
            
            /* Replace new chart data */
            youngUsage.setData(youngUsageBuf);
            oldUsage.setData(oldUsageBuf);
            free.setData(freeBuf);

            instances.setData(instanceBuf);

            gcTime.setData(gcTimeBuf);

            metaspaceUsage.setData(metaspaceUsageBuf);
            metaspaceCapacity.setData(metaspaceCapacityBuf);
        }

    }

    /**
     * Get property of SummaryData.
     *
     * @return Property of SummaryData.
     */
    public ObjectProperty<SummaryData> summaryDataProperty() {
        return summaryData;
    }

    /**
     * Get property of list of SnapShotHeader.
     *
     * @return Property of list of SnapShotHeader.
     */
    public ObjectProperty<ObservableList<SnapShotHeader>> currentTargetProperty() {
        return currentTarget;
    }

    /**
     * Get property of class name set.
     *
     * @return Property of class name set.
     */
    public ObjectProperty<ObservableSet<String>> currentClassNameSetProperty() {
        return currentClassNameSet;
    }

    /**
     * Get new task for calculating GC summary.
     *
     * @param drawRebootSuspectLine Consumer for drawing reboot line.
     * @return Task for calculating GC summary.
     */
    public Task<Void> getCalculateGCSummaryTask(Consumer<XYChart<Number, ? extends Number>> drawRebootSuspectLine) {
        return new CalculateGCSummaryTask(drawRebootSuspectLine);
    }

    /**
     * Get Java heap chart.
     *
     * @return Java heap chart.
     */
    public StackedAreaChart<Number, Long> getHeapChart() {
        return heapChart;
    }

    /**
     * Get instance chart.
     *
     * @return Instance chart.
     */
    public LineChart<Number, Long> getInstanceChart() {
        return instanceChart;
    }

    /**
     * Get GC time chart.
     *
     * @return GC time chart.
     */
    public LineChart<Number, Long> getGcTimeChart() {
        return gcTimeChart;
    }

    /**
     * Get Metaspace chart.
     *
     * @return Metaspace chart.
     */
    public AreaChart<Number, Long> getMetaspaceChart() {
        return metaspaceChart;
    }

    /**
     * Clear all items in SnapShot Summary tab.
     */
    public void clearAllItems(){
        summaryTable.getItems().clear();
        
        Stream.of(heapChart, instanceChart, gcTimeChart, metaspaceChart)
              .peek(c -> c.getData().stream().forEach(s -> s.getData().clear())) // Clear chart series data.
              .flatMap(c -> c.getParent().getChildrenUnmodifiable().stream()) // Get parent node of chart.
              .filter(n -> n instanceof AnchorPane)
              .forEach(p -> ((AnchorPane)p).getChildren().clear()); // Chart reboot suspect lines.
}

}

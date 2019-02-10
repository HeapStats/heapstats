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
package jp.co.ntt.oss.heapstats.fx.plugin.builtin.snapshot.tabs;

import java.net.URL;
import java.time.ZoneId;
import java.util.Locale;
import java.util.ResourceBundle;
import java.util.function.Consumer;
import java.util.stream.Collectors;
import java.util.stream.IntStream;
import java.util.stream.Stream;
import javafx.application.Platform;
import javafx.beans.property.ObjectProperty;
import javafx.beans.property.SimpleObjectProperty;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.collections.ObservableSet;
import javafx.concurrent.Task;
import javafx.event.EventHandler;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.chart.AreaChart;
import javafx.scene.chart.LineChart;
import javafx.scene.chart.NumberAxis;
import javafx.scene.chart.StackedAreaChart;
import javafx.scene.chart.XYChart;
import javafx.scene.control.ContentDisplay;
import javafx.scene.control.Label;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableView;
import javafx.scene.control.Tooltip;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.scene.input.MouseEvent;
import javafx.scene.layout.AnchorPane;
import javafx.scene.layout.GridPane;
import javafx.scene.shape.Path;
import javafx.scene.shape.Rectangle;
import jp.co.ntt.oss.heapstats.container.snapshot.SnapShotHeader;
import jp.co.ntt.oss.heapstats.container.snapshot.SummaryData;
import jp.co.ntt.oss.heapstats.fx.utils.EpochTimeConverter;
import jp.co.ntt.oss.heapstats.api.utils.HeapStatsUtils;

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

    private EpochTimeConverter epochTimeConverter;

    private Tooltip heapTooltip;

    private GridPane heapTooltipGrid;

    private Label youngLabel;

    private Label oldLabel;

    private Label freeLabel;

    private Tooltip metaspaceTooltip;

    private GridPane metaspaceTooltipGrid;

    private Label metaspaceUsageLabel;

    private Label metaspaceCapacityLabel;

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

        epochTimeConverter = new EpochTimeConverter();
        Stream.of(heapChart, instanceChart, gcTimeChart, metaspaceChart)
              .map(c -> (NumberAxis)c.getXAxis())
              .forEach(a -> a.setTickLabelFormatter(epochTimeConverter));

        initializeChartSeries();
    }

    private void setSummaryTable(SummaryData data) {
        if (data == null) {
            summaryTable.getItems().clear();
        } else {
            ResourceBundle resource = ResourceBundle.getBundle("snapshotResources", new Locale(HeapStatsUtils.getLanguage()));
            String safepointTimeStr = data.hasSafepointTime() ? String.format("%d ms (%.02f %%)", data.getSafepointTime(), data.getSafepointPercentage())
                                                              : "N/A";

            summaryTable.setItems(FXCollections.observableArrayList(new SummaryData.SummaryDataEntry(resource.getString("summary.snapshot.count"), Integer.toString(data.getCount())),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.gc.count"), String.format("%d (Full: %d, Young: %d)", data.getFullCount() + data.getYngCount(), data.getFullCount(), data.getYngCount())),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.heap.usage"), String.format("%.1f MB", data.getLatestHeapUsage() / 1024.0d / 1024.0d)),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.metaspace.usage"), String.format("%.1f MB", data.getLatestMetaspaceUsage() / 1024.0d / 1024.0d)),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.gc.time"), String.format("%d ms", data.getMaxGCTime())),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.gc.totaltime"), String.format("%d ms", data.getTotalGCTime())),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.safepoint.time"), safepointTimeStr),
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

        String cssName = "/jp/co/ntt/oss/heapstats/plugin/builtin/snapshot/tabs/";
        if (HeapStatsUtils.getHeapOrder()) {
            heapChart.getData().addAll(youngUsage, oldUsage, free);
            cssName += "heapsummary-bottom-young.css";
        } else {
            heapChart.getData().addAll(oldUsage, youngUsage, free);
            cssName += "heapsummary-bottom-old.css";
        }
        heapChart.getStylesheets().add(cssName);

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

        /* Tooltip setup */
        /* Java heap */
        heapTooltipGrid = new GridPane();
        heapTooltipGrid.setHgap(HeapStatsUtils.TOOLTIP_GRIDPANE_GAP);
        youngLabel = new Label();
        oldLabel = new Label();
        freeLabel = new Label();
        Rectangle youngRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);
        Rectangle oldRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);
        Rectangle freeRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);

        Platform.runLater(() -> {
            youngRect.setStyle("-fx-fill: " + ((Path)heapChart.lookup(".series0")).getFill().toString().replace("0x", "#"));
            oldRect.setStyle("-fx-fill: " + ((Path)heapChart.lookup(".series1")).getFill().toString().replace("0x", "#"));
            freeRect.setStyle("-fx-fill: " + ((Path)heapChart.lookup(".series2")).getFill().toString().replace("0x", "#"));
        });

        heapTooltipGrid.add(youngRect, 0, 0);
        heapTooltipGrid.add(new Label("Young"), 1, 0);
        heapTooltipGrid.add(youngLabel, 2, 0);
        heapTooltipGrid.add(oldRect, 0, 1);
        heapTooltipGrid.add(new Label("Old"), 1, 1);
        heapTooltipGrid.add(oldLabel, 2, 1);
        heapTooltipGrid.add(freeRect, 0, 2);
        heapTooltipGrid.add(new Label("Free"), 1, 2);
        heapTooltipGrid.add(freeLabel, 2, 2);
        heapTooltip = new Tooltip();
        heapTooltip.setGraphic(heapTooltipGrid);
        heapTooltip.setContentDisplay(ContentDisplay.BOTTOM);

        /* Metaspace */
        metaspaceTooltipGrid = new GridPane();
        metaspaceTooltipGrid.setHgap(HeapStatsUtils.TOOLTIP_GRIDPANE_GAP);
        metaspaceUsageLabel = new Label();
        metaspaceCapacityLabel = new Label();
        Rectangle metaUsageRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);
        Rectangle metaCapacityRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);

        Platform.runLater(() -> {
            metaUsageRect.setStyle("-fx-fill: " + ((Path)metaspaceChart.lookup(".default-color0.chart-series-area-fill")).getFill().toString().replace("0x", "#"));
            metaCapacityRect.setStyle("-fx-fill: " + ((Path)metaspaceChart.lookup(".default-color1.chart-series-area-fill")).getFill().toString().replace("0x", "#"));
        });

        metaspaceTooltipGrid.add(metaUsageRect, 0, 0);
        metaspaceTooltipGrid.add(new Label("Usage"), 1, 0);
        metaspaceTooltipGrid.add(metaspaceUsageLabel, 2, 0);
        metaspaceTooltipGrid.add(metaCapacityRect, 0, 1);
        metaspaceTooltipGrid.add(new Label("Capacity"), 1, 1);
        metaspaceTooltipGrid.add(metaspaceCapacityLabel, 2, 1);
        metaspaceTooltip = new Tooltip();
        metaspaceTooltip.setGraphic(metaspaceTooltipGrid);
        metaspaceTooltip.setContentDisplay(ContentDisplay.BOTTOM);
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

        private void setupJavaHeapChartTooltip(int idx){
            XYChart.Data<Number, Long> youngNode = youngUsage.getData().get(idx);
            XYChart.Data<Number, Long> oldNode = oldUsage.getData().get(idx);
            XYChart.Data<Number, Long> freeNode = free.getData().get(idx);
            long youngInMB = youngNode.getYValue();
            long oldInMB = oldNode.getYValue();
            long freeInMB = freeNode.getYValue();

            EventHandler<MouseEvent> handler = e -> {
                heapTooltip.setText(epochTimeConverter.toString(youngNode.getXValue()));
                youngLabel.setText(youngInMB + " MB");
                oldLabel.setText(oldInMB + " MB");
                freeLabel.setText(freeInMB + " MB");
            };

            Tooltip.install(youngNode.getNode(), heapTooltip);
            youngNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
            Tooltip.install(oldNode.getNode(), heapTooltip);
            oldNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
            Tooltip.install(freeNode.getNode(), heapTooltip);
            freeNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
        }

        private void setupMetaspaceChartTooltip(int idx){
            XYChart.Data<Number, Long> usageNode = metaspaceUsage.getData().get(idx);
            XYChart.Data<Number, Long> capacityNode = metaspaceCapacity.getData().get(idx);
            long usage = usageNode.getYValue();
            long capacity = capacityNode.getYValue();

            EventHandler<MouseEvent> handler = e -> {
                metaspaceTooltip.setText(epochTimeConverter.toString(usageNode.getXValue()));
                metaspaceUsageLabel.setText(usage + " MB");
                metaspaceCapacityLabel.setText(capacity + " MB");
            };

            Tooltip.install(usageNode.getNode(), metaspaceTooltip);
            usageNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
            Tooltip.install(capacityNode.getNode(), metaspaceTooltip);
            capacityNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
        }

        @Override
        protected void succeeded() {
            long startEpoch = currentTarget.get().get(0).getSnapShotDate().atZone(ZoneId.systemDefault()).toEpochSecond();
            long endEpoch = currentTarget.get().get(currentTarget.get().size() - 1).getSnapShotDate().atZone(ZoneId.systemDefault()).toEpochSecond();
            Stream.of(heapChart, instanceChart, gcTimeChart, metaspaceChart)
                  .peek(c -> ((NumberAxis)c.getXAxis()).setTickUnit((endEpoch - startEpoch) / HeapStatsUtils.getXTickUnit()))
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

            /* Set tooltip */
            /* Java Heap & Metaspace */
            IntStream.range(0, currentTarget.get().size())
                     .peek(this::setupJavaHeapChartTooltip)
                     .forEach(this::setupMetaspaceChartTooltip);

            Tooltip tooltip = new Tooltip();

            /* Insatances */
            instances.getData()
                     .stream()
                     .peek(d -> Tooltip.install(d.getNode(), tooltip))
                     .forEach(d -> d.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, e -> tooltip.setText((epochTimeConverter.toString(d.getXValue()) + ": " + d.getYValue()))));

            /* GC time */
            gcTime.getData()
                  .stream()
                  .peek(d -> Tooltip.install(d.getNode(), tooltip))
                  .forEach(d -> d.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, e -> tooltip.setText(String.format("%s: %d ms", epochTimeConverter.toString(d.getXValue()), d.getYValue()))));
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

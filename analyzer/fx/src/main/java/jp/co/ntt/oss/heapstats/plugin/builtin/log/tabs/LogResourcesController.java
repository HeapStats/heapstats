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
package jp.co.ntt.oss.heapstats.plugin.builtin.log.tabs;

import java.net.URL;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.util.List;
import java.util.Optional;
import java.util.ResourceBundle;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import javafx.application.Platform;
import javafx.beans.property.ObjectProperty;
import javafx.beans.property.SimpleObjectProperty;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.concurrent.Task;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.Node;
import javafx.scene.chart.Axis;
import javafx.scene.chart.LineChart;
import javafx.scene.chart.NumberAxis;
import javafx.scene.chart.StackedAreaChart;
import javafx.scene.chart.XYChart;
import javafx.scene.control.Label;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableView;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.scene.input.MouseEvent;
import javafx.scene.layout.AnchorPane;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.StackPane;
import javafx.scene.shape.Rectangle;
import javafx.stage.Popup;
import jp.co.ntt.oss.heapstats.container.log.ArchiveData;
import jp.co.ntt.oss.heapstats.container.log.DiffData;
import jp.co.ntt.oss.heapstats.container.log.LogData;
import jp.co.ntt.oss.heapstats.container.log.SummaryData;
import jp.co.ntt.oss.heapstats.utils.EpochTimeConverter;
import jp.co.ntt.oss.heapstats.utils.HeapStatsUtils;

/**
 * FXML Controller class for "Resource Data" tab in LogData plugin.
 */
public class LogResourcesController implements Initializable {

    @FXML
    private GridPane chartGrid;

    @FXML
    private StackedAreaChart<Number, Double> javaCPUChart;

    private XYChart.Series<Number, Double> javaUserUsage;

    private XYChart.Series<Number, Double> javaSysUsage;

    @FXML
    private StackedAreaChart<Number, Double> systemCPUChart;

    private XYChart.Series<Number, Double> systemUserUsage;

    private XYChart.Series<Number, Double> systemNiceUsage;

    private XYChart.Series<Number, Double> systemSysUsage;

    private XYChart.Series<Number, Double> systemIdleUsage;

    private XYChart.Series<Number, Double> systemIOWaitUsage;

    private XYChart.Series<Number, Double> systemIRQUsage;

    private XYChart.Series<Number, Double> systemSoftIRQUsage;

    private XYChart.Series<Number, Double> systemStealUsage;

    private XYChart.Series<Number, Double> systemGuestUsage;

    @FXML
    private LineChart<Number, Long> javaMemoryChart;

    private XYChart.Series<Number, Long> javaVSZUsage;

    private XYChart.Series<Number, Long> javaRSSUsage;

    @FXML
    private LineChart<Number, Long> safepointChart;

    private XYChart.Series<Number, Long> safepoints;

    @FXML
    private LineChart<Number, Long> safepointTimeChart;

    private XYChart.Series<Number, Long> safepointTime;

    @FXML
    private LineChart<Number, Long> threadChart;

    private XYChart.Series<Number, Long> threads;

    @FXML
    private LineChart<Number, Long> monitorChart;

    private XYChart.Series<Number, Long> monitors;

    @FXML
    private TableView<SummaryData.SummaryDataEntry> procSummary;

    @FXML
    private TableColumn<SummaryData.SummaryDataEntry, String> categoryColumn;

    @FXML
    private TableColumn<SummaryData.SummaryDataEntry, String> valueColumn;

    private Popup chartPopup;

    private Label popupText;

    private ObjectProperty<ObservableList<ArchiveData>> archiveList;

    private List<LocalDateTime> suspectList;
    
    private ResourceBundle resource;
    
    private EpochTimeConverter epochTimeConverter;

    /**
     * Initializes the controller class.
     */
    @Override
    public void initialize(URL url, ResourceBundle rb) {
        resource = rb;
        
        categoryColumn.setCellValueFactory(new PropertyValueFactory<>("category"));
        valueColumn.setCellValueFactory(new PropertyValueFactory<>("value"));

        String bgcolor = "-fx-background-color: " + HeapStatsUtils.getChartBgColor() + ";";
        javaCPUChart.lookup(".chart").setStyle(bgcolor);
        systemCPUChart.lookup(".chart").setStyle(bgcolor);
        javaMemoryChart.lookup(".chart").setStyle(bgcolor);
        safepointChart.lookup(".chart").setStyle(bgcolor);
        safepointTimeChart.lookup(".chart").setStyle(bgcolor);
        threadChart.lookup(".chart").setStyle(bgcolor);
        monitorChart.lookup(".chart").setStyle(bgcolor);

        initializeChartSeries();

        chartPopup = new Popup();
        popupText = new Label();
        popupText.setStyle("-fx-font-family: monospace; -fx-text-fill: white; -fx-background-color: black;");
        chartPopup.getContent().add(popupText);
        archiveList = new SimpleObjectProperty<>();
        
        epochTimeConverter = new EpochTimeConverter();
    }

    /**
     * Initialize Series in Chart. This method uses to avoid RuntimeException
     * which is related to: RT-37994: [FXML] ProxyBuilder does not support
     * read-only collections https://javafx-jira.kenai.com/browse/RT-37994
     */
    @SuppressWarnings("unchecked")
    private void initializeChartSeries() {
        threads = new XYChart.Series<>();
        threads.setName("Threads");
        threadChart.getData().add(threads);

        javaUserUsage = new XYChart.Series<>();
        javaUserUsage.setName("user");
        javaSysUsage = new XYChart.Series<>();
        javaSysUsage.setName("sys");
        javaCPUChart.getData().addAll(javaUserUsage, javaSysUsage);

        systemUserUsage = new XYChart.Series<>();
        systemUserUsage.setName("user");
        systemNiceUsage = new XYChart.Series<>();
        systemNiceUsage.setName("nice");
        systemSysUsage = new XYChart.Series<>();
        systemSysUsage.setName("sys");
        systemIdleUsage = new XYChart.Series<>();
        systemIdleUsage.setName("idle");
        systemIOWaitUsage = new XYChart.Series<>();
        systemIOWaitUsage.setName("I/O wait");
        systemIRQUsage = new XYChart.Series<>();
        systemIRQUsage.setName("IRQ");
        systemSoftIRQUsage = new XYChart.Series<>();
        systemSoftIRQUsage.setName("soft IRQ");
        systemStealUsage = new XYChart.Series<>();
        systemStealUsage.setName("steal");
        systemGuestUsage = new XYChart.Series<>();
        systemGuestUsage.setName("guest");
        systemCPUChart.getData().addAll(systemUserUsage, systemNiceUsage, systemSysUsage,
                systemIdleUsage, systemIOWaitUsage, systemIRQUsage,
                systemSoftIRQUsage, systemStealUsage, systemGuestUsage);

        javaVSZUsage = new XYChart.Series<>();
        javaVSZUsage.setName("VSZ");
        javaRSSUsage = new XYChart.Series<>();
        javaRSSUsage.setName("RSS");
        javaMemoryChart.getData().addAll(javaVSZUsage, javaRSSUsage);

        safepoints = new XYChart.Series<>();
        safepoints.setName("Safepoints");
        safepointChart.getData().add(safepoints);

        safepointTime = new XYChart.Series<>();
        safepointTime.setName("Safepoint Time");
        safepointTimeChart.getData().add(safepointTime);

        monitors = new XYChart.Series<>();
        monitors.setName("Monitors");
        monitorChart.getData().add(monitors);
    }

    /**
     * Show popup window with pointing data in chart.
     *
     * @param chart Target chart.
     * @param xValue value in X Axis.
     * @param event Mouse event.
     * @param labelFunc Function to format label string.
     */
    private void showChartPopup(XYChart<Number, ? extends Number> chart, Number xValue, MouseEvent event, Function<? super Number, String> labelFunc) {
        boolean isContained = chart.getData()
                                   .stream()
                                   .flatMap(s -> s.getData().stream())
                                   .anyMatch(d -> d.getXValue().longValue() == xValue.longValue());
        if(isContained){
            String label = chart.getData()
                                .stream()
                                .map(s -> s.getName() + " = " + s.getData()
                                                                 .stream()
                                                                 .filter(d -> d.getXValue().longValue() == xValue.longValue())
                                                                 .map(d -> labelFunc.apply(d.getYValue()))
                                                                 .findAny()
                                                                 .get())
                                .collect(Collectors.joining("\n"));
            popupText.setText(epochTimeConverter.toString(xValue) + "\n" + label);
            chartPopup.show(chart, event.getScreenX() + 15.0d, event.getScreenY() + 3.0d);
        }
    }

    @FXML
    @SuppressWarnings("unchecked")
    private void onChartMouseMoved(MouseEvent event) {
        XYChart<Number, ? extends Number> chart = (XYChart<Number, ? extends Number>) event.getSource();
        NumberAxis xAxis = (NumberAxis)chart.getXAxis();
        Function<? super Number, String> labelFunc;

        if ((chart == javaCPUChart) || (chart == systemCPUChart)) {
            labelFunc = d -> String.format("%.02f %%", d);
        } else if (chart == javaMemoryChart) {
            labelFunc = d -> String.format("%d MB", d);
        } else if (chart == safepointTimeChart) {
            labelFunc = d -> String.format("%d ms", d);
        } else {
            labelFunc = d -> d.toString();
        }

        Optional.ofNullable(chart.getXAxis().getValueForDisplay(event.getX() - xAxis.getLayoutX()))
                .ifPresent(v -> showChartPopup(chart, v, event, labelFunc));
    }

    @FXML
    private void onChartMouseExited(MouseEvent event) {
        chartPopup.hide();
    }

    private void drawLineInternal(StackPane target, List<Number> drawList, String style) {
        AnchorPane anchor = null;
        XYChart chart = null;

        for (Node node : ((StackPane) target).getChildren()) {

            if (node instanceof AnchorPane) {
                anchor = (AnchorPane) node;
            } else if (node instanceof XYChart) {
                chart = (XYChart) node;
            }

            if ((anchor != null) && (chart != null)) {
                break;
            }

        }

        if ((anchor == null) || (chart == null)) {
            throw new IllegalStateException(resource.getString("message.drawline"));
        }

        ObservableList<Node> anchorChildren = anchor.getChildren();
        anchorChildren.removeAll(anchorChildren.stream()
                .filter(n -> n instanceof Rectangle)
                .map(n -> (Rectangle) n)
                .filter(r -> r.getStyle().equals(style))
                .collect(Collectors.toList()));

        NumberAxis xAxis = (NumberAxis) chart.getXAxis();
        Axis yAxis = chart.getYAxis();
        Label chartTitle = (Label) chart.getChildrenUnmodifiable().stream()
                .filter(n -> n.getStyleClass().contains("chart-title"))
                .findFirst()
                .get();

        double startX = xAxis.getLayoutX() + 4.0d;
        double yPos = yAxis.getLayoutY() + chartTitle.getLayoutY() + chartTitle.getHeight();
        List<Rectangle> rectList = drawList.stream()
                .map(t -> new Rectangle(xAxis.getDisplayPosition(t) + startX, yPos, 2.0d, yAxis.getHeight()))
                .peek(r -> ((Rectangle) r).setStyle(style))
                .collect(Collectors.toList());
        anchorChildren.addAll(rectList);
    }

    private void drawArchiveLine() {

        if (archiveList.get().isEmpty()) {
            return;
        }

        List<Number> archiveDateList = archiveList.get()
                                                  .stream()
                                                  .map(a -> a.getDate().atZone(ZoneId.systemDefault()).toEpochSecond())
                                                  .collect(Collectors.toList());
        chartGrid.getChildren().stream()
                .filter(n -> n instanceof StackPane)
                .forEach(p -> drawLineInternal((StackPane) p, archiveDateList, "-fx-fill: black;"));
    }

    /**
     * Draw line which represents to suspect to reboot. This method does not
     * clear AnchorPane to draw lines. So this method must be called after
     * drawArchiveLine().
     */
    private void drawRebootSuspectLine() {

        if ((suspectList == null) || suspectList.isEmpty()) {
            return;
        }

        List<Number> suspectRebootDateList = suspectList.stream()
                                                        .map(d -> d.atZone(ZoneId.systemDefault()).toEpochSecond())
                                                        .collect(Collectors.toList());
        chartGrid.getChildren().stream()
                .filter(n -> n instanceof StackPane)
                .forEach(p -> drawLineInternal((StackPane) p, suspectRebootDateList, "-fx-fill: yellow;"));
    }

    /**
     * Draw lines which represents reboot and ZIP archive timing.
     */
    public void drawEventLineToChart() {
        drawArchiveLine();
        drawRebootSuspectLine();
    }

    /**
     * Task class for drawing log chart data.
     */
    private class DrawLogChartTask extends Task<Void> {

        /* Java CPU */
        private final ObservableList<XYChart.Data<Number, Double>> javaUserUsageBuf;
        private final ObservableList<XYChart.Data<Number, Double>> javaSysUsageBuf;

        /* System CPU */
        private final ObservableList<XYChart.Data<Number, Double>> systemUserUsageBuf;
        private final ObservableList<XYChart.Data<Number, Double>> systemNiceUsageBuf;
        private final ObservableList<XYChart.Data<Number, Double>> systemSysUsageBuf;
        private final ObservableList<XYChart.Data<Number, Double>> systemIdleUsageBuf;
        private final ObservableList<XYChart.Data<Number, Double>> systemIOWaitUsageBuf;
        private final ObservableList<XYChart.Data<Number, Double>> systemIRQUsageBuf;
        private final ObservableList<XYChart.Data<Number, Double>> systemSoftIRQUsageBuf;
        private final ObservableList<XYChart.Data<Number, Double>> systemStealUsageBuf;
        private final ObservableList<XYChart.Data<Number, Double>> systemGuestUsageBuf;

        /* Java Memory */
        private final ObservableList<XYChart.Data<Number, Long>> javaVSZUsageBuf;
        private final ObservableList<XYChart.Data<Number, Long>> javaRSSUsageBuf;

        /* Safepoints */
        private final ObservableList<XYChart.Data<Number, Long>> safepointsBuf;
        private final ObservableList<XYChart.Data<Number, Long>> safepointTimeBuf;

        /* Threads */
        private final ObservableList<XYChart.Data<Number, Long>> threadsBuf;

        /* Monitor contantion */
        private final ObservableList<XYChart.Data<Number, Long>> monitorsBuf;

        private final List<LogData> targetLogData;

        private final List<DiffData> targetDiffData;

        private long loopCount;

        private final long totalLoopCount;

        public DrawLogChartTask(List<LogData> targetLogData, List<DiffData> targetDiffData) {
            javaUserUsageBuf = FXCollections.observableArrayList();
            javaSysUsageBuf = FXCollections.observableArrayList();
            systemUserUsageBuf = FXCollections.observableArrayList();
            systemNiceUsageBuf = FXCollections.observableArrayList();
            systemSysUsageBuf = FXCollections.observableArrayList();
            systemIdleUsageBuf = FXCollections.observableArrayList();
            systemIOWaitUsageBuf = FXCollections.observableArrayList();
            systemIRQUsageBuf = FXCollections.observableArrayList();
            systemSoftIRQUsageBuf = FXCollections.observableArrayList();
            systemStealUsageBuf = FXCollections.observableArrayList();
            systemGuestUsageBuf = FXCollections.observableArrayList();
            javaVSZUsageBuf = FXCollections.observableArrayList();
            javaRSSUsageBuf = FXCollections.observableArrayList();
            safepointsBuf = FXCollections.observableArrayList();
            safepointTimeBuf = FXCollections.observableArrayList();
            threadsBuf = FXCollections.observableArrayList();
            monitorsBuf = FXCollections.observableArrayList();

            this.targetLogData = targetLogData;
            this.targetDiffData = targetDiffData;
            totalLoopCount = targetDiffData.size() + targetLogData.size();
        }

        private void addDiffData(DiffData data) {
            long time = data.getDateTime().atZone(ZoneId.systemDefault()).toEpochSecond();

            javaUserUsageBuf.add(new XYChart.Data<>(time, data.getJavaUserUsage()));
            javaSysUsageBuf.add(new XYChart.Data<>(time, data.getJavaSysUsage()));
            systemUserUsageBuf.add(new XYChart.Data<>(time, data.getCpuUserUsage()));
            systemNiceUsageBuf.add(new XYChart.Data<>(time, data.getCpuNiceUsage()));
            systemSysUsageBuf.add(new XYChart.Data<>(time, data.getCpuSysUsage()));
            systemIdleUsageBuf.add(new XYChart.Data<>(time, data.getCpuIdleUsage()));
            systemIOWaitUsageBuf.add(new XYChart.Data<>(time, data.getCpuIOWaitUsage()));
            systemIRQUsageBuf.add(new XYChart.Data<>(time, data.getCpuIRQUsage()));
            systemSoftIRQUsageBuf.add(new XYChart.Data<>(time, data.getCpuSoftIRQUsage()));
            systemStealUsageBuf.add(new XYChart.Data<>(time, data.getCpuStealUsage()));
            systemGuestUsageBuf.add(new XYChart.Data<>(time, data.getCpuGuestUsage()));
            monitorsBuf.add(new XYChart.Data<>(time, data.getJvmSyncPark()));
            safepointsBuf.add(new XYChart.Data<>(time, data.getJvmSafepoints()));
            safepointTimeBuf.add(new XYChart.Data<>(time, data.getJvmSafepointTime()));

            updateProgress();
        }

        private void addLogData(LogData data) {
            long time = data.getDateTime().atZone(ZoneId.systemDefault()).toEpochSecond();

            javaVSZUsageBuf.add(new XYChart.Data<>(time, data.getJavaVSSize() / 1024 / 1024));
            javaRSSUsageBuf.add(new XYChart.Data<>(time, data.getJavaRSSize() / 1024 / 1024));
            threadsBuf.add(new XYChart.Data<>(time, data.getJvmLiveThreads()));

            updateProgress();
        }

        private void setChartData() {
            /* Set chart range */
            long startLogEpoch = targetLogData.get(0).getDateTime().atZone(ZoneId.systemDefault()).toEpochSecond();
            long endLogEpoch = targetLogData.get(targetLogData.size() - 1).getDateTime().atZone(ZoneId.systemDefault()).toEpochSecond();
            long startDiffEpoch = targetDiffData.get(0).getDateTime().atZone(ZoneId.systemDefault()).toEpochSecond();
            long endDiffEpoch = targetDiffData.get(targetDiffData.size() - 1).getDateTime().atZone(ZoneId.systemDefault()).toEpochSecond();
            Stream.of(javaMemoryChart, threadChart)
                  .map(c -> (NumberAxis)c.getXAxis())
                  .peek(a -> a.setLowerBound(startLogEpoch))
                  .forEach(a -> a.setUpperBound(endLogEpoch));
            Stream.of(javaCPUChart, systemCPUChart, safepointChart, safepointTimeChart, monitorChart)
                  .map(c -> (NumberAxis)c.getXAxis())
                  .peek(a -> a.setLowerBound(startDiffEpoch))
                  .forEach(a -> a.setUpperBound(endDiffEpoch));
            
            /* Replace new chart data */
            javaUserUsage.setData(javaUserUsageBuf);
            javaSysUsage.setData(javaSysUsageBuf);

            systemUserUsage.setData(systemUserUsageBuf);
            systemNiceUsage.setData(systemNiceUsageBuf);
            systemSysUsage.setData(systemSysUsageBuf);
            systemIdleUsage.setData(systemIdleUsageBuf);
            systemIOWaitUsage.setData(systemIOWaitUsageBuf);
            systemIRQUsage.setData(systemIRQUsageBuf);
            systemSoftIRQUsage.setData(systemSoftIRQUsageBuf);
            systemStealUsage.setData(systemStealUsageBuf);
            systemGuestUsage.setData(systemGuestUsageBuf);

            monitors.setData(monitorsBuf);

            safepoints.setData(safepointsBuf);
            safepointTime.setData(safepointTimeBuf);

            javaVSZUsage.setData(javaVSZUsageBuf);
            javaRSSUsage.setData(javaRSSUsageBuf);

            threads.setData(threadsBuf);

            /* Put summary data to table */
            SummaryData summary = new SummaryData(targetLogData, targetDiffData);
            procSummary.setItems(FXCollections.observableArrayList(new SummaryData.SummaryDataEntry(resource.getString("summary.cpu.average"), String.format("%.1f %%", summary.getAverageCPUUsage())),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.cpu.peak"), String.format("%.1f %%", summary.getMaxCPUUsage())),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.vsz.average"), String.format("%.1f MB", summary.getAverageVSZ())),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.vsz.peak"), String.format("%.1f MB", summary.getMaxVSZ())),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.rss.average"), String.format("%.1f MB", summary.getAverageRSS())),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.rss.peak"), String.format("%.1f MB", summary.getMaxRSS())),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.threads.average"), String.format("%.1f", summary.getAverageLiveThreads())),
                    new SummaryData.SummaryDataEntry(resource.getString("summary.threads.peak"), Long.toString(summary.getMaxLiveThreads()))
            ));

            /*
             * drawArchiveLine() needs positions in each chart.
             * So I call it next event.
             */
            suspectList = targetDiffData.stream()
                    .filter(d -> d.hasMinusData())
                    .map(d -> d.getDateTime())
                    .collect(Collectors.toList());

            Platform.runLater(() -> drawEventLineToChart());
        }

        private void updateProgress() {
            updateProgress(++loopCount, totalLoopCount);
        }

        @Override
        protected Void call() throws Exception {
            loopCount = 0;

            /* Generate graph data */
            targetDiffData.forEach(this::addDiffData);
            targetLogData.forEach(this::addLogData);

            Platform.runLater(this::setChartData);

            return null;
        }

    }

    /**
     * Get Task instance which draws each chart.
     *
     * @param targetLogData List of LogData to draw.
     * @param targetDiffData List of DiffData to draw.
     *
     * @return Task instance to draw charts.
     */
    public Task<Void> createDrawResourceCharts(List<LogData> targetLogData, List<DiffData> targetDiffData) {
        return new DrawLogChartTask(targetLogData, targetDiffData);
    }

    /**
     * Get HeapStats ZIP archive list as Property.
     *
     * @return List of HeapStats ZIP archive.
     */
    public ObjectProperty<ObservableList<ArchiveData>> archiveListProperty() {
        return archiveList;
    }

    /**
     * Clear all items in Resource Data tab.
     */
    @SuppressWarnings({"raw", "unchecked"})
    public void clearAllItems(){
        
        for(Node n : chartGrid.getChildren()){
            if(n instanceof StackPane){
                for(Node cn : ((StackPane)n).getChildren()){
                    if(cn instanceof AnchorPane){ // Clear archive and reboot suspect lines.
                        ((AnchorPane)cn).getChildren().clear();
}
                    else if(cn instanceof XYChart){ // Clear chart data.
                        ((XYChart)cn).getData().stream()
                                .forEach(s -> ((XYChart.Series)s).getData().clear());
                    }
                }
            }
        }
        
        /* Claer summary table. */
        procSummary.getItems().clear();
    }

}

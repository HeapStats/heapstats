/*
 * Copyright (C) 2015-2017 Nippon Telegraph and Telephone Corporation
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
import java.util.ResourceBundle;
import java.util.stream.Collectors;
import java.util.stream.IntStream;
import java.util.stream.Stream;
import javafx.application.Platform;
import javafx.beans.property.ObjectProperty;
import javafx.beans.property.SimpleObjectProperty;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.concurrent.Task;
import javafx.event.EventHandler;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.Node;
import javafx.scene.chart.Axis;
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
import javafx.scene.layout.StackPane;
import javafx.scene.shape.Path;
import javafx.scene.shape.Rectangle;
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

    private ObjectProperty<ObservableList<ArchiveData>> archiveList;

    private List<LocalDateTime> suspectList;
    
    private ResourceBundle resource;
    
    private EpochTimeConverter epochTimeConverter;

    /* Tooltip for Java CPU chart */
    private Tooltip javaCPUTooltip;

    private GridPane javaCPUTooltipGrid;

    private Label javaUserLabel;

    private Label javaSysLabel;

    /* Tooltip for System CPU chart */
    private Tooltip systemCPUTooltip;

    private GridPane systemCPUTooltipGrid;

    private Label systemUserLabel;

    private Label systemNiceLabel;

    private Label systemSysLabel;

    private Label systemIdleLabel;

    private Label systemIOWaitLabel;

    private Label systemIRQLabel;

    private Label systemSoftIRQLabel;

    private Label systemStealLabel;

    private Label systemGuestLabel;

    /* Tooltip for Java Memory chart */
    private Tooltip javaMemoryTooltip;

    private GridPane javaMemoryTooltipGrid;

    private Label javaMemoryVSZLabel;

    private Label javaMemoryRSSLabel;

    /* Generic Tooltip */
    private Tooltip tooltip;

    private void initializeJavaCPUTooltip(){
        javaUserLabel = new Label();
        javaSysLabel = new Label();

        Rectangle javaUserRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);
        Rectangle javaSysRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);

        Platform.runLater(() -> {
            javaUserRect.setStyle("-fx-fill: " + ((Path)javaCPUChart.lookup(".series0")).getFill().toString().replace("0x", "#"));
            javaSysRect.setStyle("-fx-fill: " + ((Path)javaCPUChart.lookup(".series1")).getFill().toString().replace("0x", "#"));
        });

        javaCPUTooltipGrid = new GridPane();
        javaCPUTooltipGrid.setHgap(HeapStatsUtils.TOOLTIP_GRIDPANE_GAP);
        javaCPUTooltipGrid.add(javaUserRect, 0, 0);
        javaCPUTooltipGrid.add(new Label("user"), 1, 0);
        javaCPUTooltipGrid.add(javaUserLabel, 2, 0);
        javaCPUTooltipGrid.add(javaSysRect, 0, 1);
        javaCPUTooltipGrid.add(new Label("sys"), 1, 1);
        javaCPUTooltipGrid.add(javaSysLabel, 2, 1);

        javaCPUTooltip = new Tooltip();
        javaCPUTooltip.setGraphic(javaCPUTooltipGrid);
        javaCPUTooltip.setContentDisplay(ContentDisplay.BOTTOM);
    }

    private void initializeJavaMemoryTooltip(){
        javaMemoryVSZLabel = new Label();
        javaMemoryRSSLabel = new Label();

        Rectangle javaMemoryVSZRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);
        Rectangle javaMemoryRSSRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);

        Platform.runLater(() -> {
            javaMemoryRSSRect.setStyle("-fx-fill: " + ((Path)javaCPUChart.lookup(".series0")).getFill().toString().replace("0x", "#"));
            javaMemoryVSZRect.setStyle("-fx-fill: " + ((Path)javaCPUChart.lookup(".series1")).getFill().toString().replace("0x", "#"));
        });

        javaMemoryTooltipGrid = new GridPane();
        javaMemoryTooltipGrid.setHgap(HeapStatsUtils.TOOLTIP_GRIDPANE_GAP);
        javaMemoryTooltipGrid.add(javaMemoryVSZRect, 0, 0);
        javaMemoryTooltipGrid.add(new Label("VSZ"), 1, 0);
        javaMemoryTooltipGrid.add(javaMemoryVSZLabel, 2, 0);
        javaMemoryTooltipGrid.add(javaMemoryRSSRect, 0, 1);
        javaMemoryTooltipGrid.add(new Label("RSS"), 1, 1);
        javaMemoryTooltipGrid.add(javaMemoryRSSLabel, 2, 1);

        javaMemoryTooltip = new Tooltip();
        javaMemoryTooltip.setGraphic(javaMemoryTooltipGrid);
        javaMemoryTooltip.setContentDisplay(ContentDisplay.BOTTOM);
    }

    private void initializeSystemCPUTooltip(){
        systemUserLabel = new Label();
        systemNiceLabel = new Label();
        systemSysLabel = new Label();
        systemIdleLabel = new Label();
        systemIOWaitLabel = new Label();
        systemIRQLabel = new Label();
        systemSoftIRQLabel = new Label();
        systemStealLabel = new Label();
        systemGuestLabel = new Label();

        Rectangle systemUserRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);
        Rectangle systemNiceRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);
        Rectangle systemSysRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);
        Rectangle systemIdleRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);
        Rectangle systemIOWaitRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);
        Rectangle systemIRQRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);
        Rectangle systemSoftIRQRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);
        Rectangle systemStealRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);
        Rectangle systemGuestRect = new Rectangle(HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE, HeapStatsUtils.TOOLTIP_LEGEND_RECT_SIZE);

        Platform.runLater(() -> {
            systemUserRect.setStyle("-fx-fill: " + ((Path)systemCPUChart.lookup(".series0")).getFill().toString().replace("0x", "#"));
            systemNiceRect.setStyle("-fx-fill: " + ((Path)systemCPUChart.lookup(".series1")).getFill().toString().replace("0x", "#"));
            systemSysRect.setStyle("-fx-fill: " + ((Path)systemCPUChart.lookup(".series2")).getFill().toString().replace("0x", "#"));
            systemIdleRect.setStyle("-fx-fill: " + ((Path)systemCPUChart.lookup(".series3")).getFill().toString().replace("0x", "#"));
            systemIOWaitRect.setStyle("-fx-fill: " + ((Path)systemCPUChart.lookup(".series4")).getFill().toString().replace("0x", "#"));
            systemIRQRect.setStyle("-fx-fill: " + ((Path)systemCPUChart.lookup(".series5")).getFill().toString().replace("0x", "#"));
            systemSoftIRQRect.setStyle("-fx-fill: " + ((Path)systemCPUChart.lookup(".series6")).getFill().toString().replace("0x", "#"));
            systemStealRect.setStyle("-fx-fill: " + ((Path)systemCPUChart.lookup(".series7")).getFill().toString().replace("0x", "#"));
            systemGuestRect.setStyle("-fx-fill: " + ((Path)systemCPUChart.lookup(".series8")).getFill().toString().replace("0x", "#"));
        });

        systemCPUTooltipGrid = new GridPane();
        systemCPUTooltipGrid.setHgap(HeapStatsUtils.TOOLTIP_GRIDPANE_GAP);
        systemCPUTooltipGrid.add(systemUserRect, 0, 0);
        systemCPUTooltipGrid.add(new Label("user"), 1, 0);
        systemCPUTooltipGrid.add(systemUserLabel, 2, 0);
        systemCPUTooltipGrid.add(systemNiceRect, 0, 1);
        systemCPUTooltipGrid.add(new Label("nice"), 1, 1);
        systemCPUTooltipGrid.add(systemNiceLabel, 2, 1);
        systemCPUTooltipGrid.add(systemSysRect, 0, 2);
        systemCPUTooltipGrid.add(new Label("sys"), 1, 2);
        systemCPUTooltipGrid.add(systemSysLabel, 2, 2);
        systemCPUTooltipGrid.add(systemIdleRect, 0, 3);
        systemCPUTooltipGrid.add(new Label("idle"), 1, 3);
        systemCPUTooltipGrid.add(systemIdleLabel, 2, 3);
        systemCPUTooltipGrid.add(systemIOWaitRect, 0, 4);
        systemCPUTooltipGrid.add(new Label("iowait"), 1, 4);
        systemCPUTooltipGrid.add(systemIOWaitLabel, 2, 4);
        systemCPUTooltipGrid.add(systemIRQRect, 0, 5);
        systemCPUTooltipGrid.add(new Label("IRQ"), 1, 5);
        systemCPUTooltipGrid.add(systemIRQLabel, 2, 5);
        systemCPUTooltipGrid.add(systemSoftIRQRect, 0, 6);
        systemCPUTooltipGrid.add(new Label("Soft IRQ"), 1, 6);
        systemCPUTooltipGrid.add(systemSoftIRQLabel, 2, 6);
        systemCPUTooltipGrid.add(systemStealRect, 0, 7);
        systemCPUTooltipGrid.add(new Label("steal"), 1, 7);
        systemCPUTooltipGrid.add(systemStealLabel, 2, 7);
        systemCPUTooltipGrid.add(systemGuestRect, 0, 8);
        systemCPUTooltipGrid.add(new Label("guest"), 1, 8);
        systemCPUTooltipGrid.add(systemGuestLabel, 2, 8);

        systemCPUTooltip = new Tooltip();
        systemCPUTooltip.setGraphic(systemCPUTooltipGrid);
        systemCPUTooltip.setContentDisplay(ContentDisplay.BOTTOM);
    }

    /**
     * Initializes the controller class.
     */
    @Override
    public void initialize(URL url, ResourceBundle rb) {
        resource = rb;
        
        categoryColumn.setCellValueFactory(new PropertyValueFactory<>("category"));
        valueColumn.setCellValueFactory(new PropertyValueFactory<>("value"));

        String bgcolor = "-fx-background-color: " + HeapStatsUtils.getChartBgColor() + ";";
        Stream.of(javaCPUChart, systemCPUChart, javaMemoryChart, safepointChart, safepointTimeChart, threadChart, monitorChart)
              .peek(c -> c.lookup(".chart").setStyle(bgcolor))
              .forEach(c -> c.getXAxis().setTickMarkVisible(HeapStatsUtils.getTickMarkerSwitch()));

        initializeChartSeries();

        archiveList = new SimpleObjectProperty<>();
        epochTimeConverter = new EpochTimeConverter();

        initializeJavaCPUTooltip();
        initializeSystemCPUTooltip();
        initializeJavaMemoryTooltip();
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

        private void setJavaCPUChartTooltip(int idx){
            XYChart.Data<Number, Double> userNode = javaUserUsage.getData().get(idx);
            XYChart.Data<Number, Double> sysNode = javaSysUsage.getData().get(idx);

            EventHandler<MouseEvent> handler = e -> {
                javaCPUTooltip.setText(epochTimeConverter.toString(userNode.getXValue()));
                javaUserLabel.setText(String.format("%.02f", userNode.getYValue()) + " %");
                javaSysLabel.setText(String.format("%.02f", sysNode.getYValue()) + " %");
            };

            Tooltip.install(userNode.getNode(), javaCPUTooltip);
            userNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
            Tooltip.install(sysNode.getNode(), javaCPUTooltip);
            sysNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
        }

        private void setJavaMemoryChartTooltip(int idx){
            XYChart.Data<Number, Long> vszNode = javaVSZUsage.getData().get(idx);
            XYChart.Data<Number, Long> rssNode = javaRSSUsage.getData().get(idx);

            EventHandler<MouseEvent> handler = e -> {
                javaMemoryTooltip.setText(epochTimeConverter.toString(vszNode.getXValue()));
                javaMemoryVSZLabel.setText(vszNode.getYValue() + " MB");
                javaMemoryRSSLabel.setText(rssNode.getYValue() + " MB");
            };

            Tooltip.install(vszNode.getNode(), javaMemoryTooltip);
            vszNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
            Tooltip.install(rssNode.getNode(), javaMemoryTooltip);
            rssNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
        }

        private void setSystemCPUChartTooltip(int idx){
            XYChart.Data<Number, Double> userNode = systemUserUsage.getData().get(idx);
            XYChart.Data<Number, Double> niceNode = systemNiceUsage.getData().get(idx);
            XYChart.Data<Number, Double> sysNode = systemSysUsage.getData().get(idx);
            XYChart.Data<Number, Double> idleNode = systemIdleUsage.getData().get(idx);
            XYChart.Data<Number, Double> iowaitNode = systemIOWaitUsage.getData().get(idx);
            XYChart.Data<Number, Double> irqNode = systemIRQUsage.getData().get(idx);
            XYChart.Data<Number, Double> softIrqNode = systemSoftIRQUsage.getData().get(idx);
            XYChart.Data<Number, Double> stealNode = systemStealUsage.getData().get(idx);
            XYChart.Data<Number, Double> guestNode = systemGuestUsage.getData().get(idx);

            EventHandler<MouseEvent> handler = e -> {
                systemCPUTooltip.setText(epochTimeConverter.toString(userNode.getXValue()));
                systemUserLabel.setText(String.format("%.02f", userNode.getYValue()) + " %");
                systemNiceLabel.setText(String.format("%.02f", niceNode.getYValue()) + " %");
                systemSysLabel.setText(String.format("%.02f", sysNode.getYValue()) + " %");
                systemIdleLabel.setText(String.format("%.02f", idleNode.getYValue()) + " %");
                systemIOWaitLabel.setText(String.format("%.02f", iowaitNode.getYValue()) + " %");
                systemIRQLabel.setText(String.format("%.02f", irqNode.getYValue()) + " %");
                systemSoftIRQLabel.setText(String.format("%.02f", softIrqNode.getYValue()) + " %");
                systemStealLabel.setText(String.format("%.02f", stealNode.getYValue()) + " %");
                systemGuestLabel.setText(String.format("%.02f", guestNode.getYValue()) + " %");
            };

            Tooltip.install(userNode.getNode(), systemCPUTooltip);
            userNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
            Tooltip.install(niceNode.getNode(), systemCPUTooltip);
            niceNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
            Tooltip.install(sysNode.getNode(), systemCPUTooltip);
            sysNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
            Tooltip.install(idleNode.getNode(), systemCPUTooltip);
            idleNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
            Tooltip.install(iowaitNode.getNode(), systemCPUTooltip);
            iowaitNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
            Tooltip.install(irqNode.getNode(), systemCPUTooltip);
            irqNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
            Tooltip.install(softIrqNode.getNode(), systemCPUTooltip);
            softIrqNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
            Tooltip.install(stealNode.getNode(), systemCPUTooltip);
            stealNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
            Tooltip.install(guestNode.getNode(), systemCPUTooltip);
            guestNode.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, handler);
        }

        private void setChartData() {
            
            if(targetLogData.isEmpty() || targetDiffData.isEmpty()){
                Stream.of(javaMemoryChart, threadChart)
                      .flatMap(c -> c.getData().stream())
                      .forEach(s -> s.getData().clear());
                Stream.of(javaCPUChart, systemCPUChart, safepointChart, safepointTimeChart, monitorChart)
                      .flatMap(c -> c.getData().stream())
                      .forEach(s -> s.getData().clear());
                procSummary.getItems().clear();
                suspectList = null;
                return;
            }
            
            /* Set chart range */
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

            /* Tooltip setting */
            IntStream.range(0, targetDiffData.size())
                     .peek(this::setJavaCPUChartTooltip)
                     .forEach(this::setSystemCPUChartTooltip);
            IntStream.range(0, targetLogData.size())
                     .forEach(this::setJavaMemoryChartTooltip);
            Tooltip tooltip = new Tooltip();
            Stream.of(threads, safepoints, monitors)
                  .flatMap(c -> c.getData().stream())
                  .peek(d -> Tooltip.install(d.getNode(), tooltip))
                  .forEach(d -> d.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, e -> tooltip.setText((epochTimeConverter.toString(d.getXValue()) + ": " + d.getYValue()))));
            safepointTime.getData()
                         .stream()
                         .peek(d -> Tooltip.install(d.getNode(), tooltip))
                         .forEach(d -> d.getNode().addEventHandler(MouseEvent.MOUSE_ENTERED_TARGET, e -> tooltip.setText(String.format("%s: %d ms", epochTimeConverter.toString(d.getXValue()), d.getYValue()))));

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

/*
 * Copyright (C) 2014-2016 Yasumasa Suenaga
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
package jp.co.ntt.oss.heapstats.plugin.builtin.snapshot;

import java.io.File;
import java.net.URL;
import java.time.LocalDateTime;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Optional;
import java.util.ResourceBundle;
import java.util.function.Predicate;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import javafx.application.Platform;
import javafx.beans.property.LongProperty;
import javafx.beans.property.ObjectProperty;
import javafx.beans.property.SimpleLongProperty;
import javafx.beans.property.SimpleObjectProperty;
import javafx.beans.value.ObservableValue;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.collections.ObservableMap;
import javafx.collections.ObservableSet;
import javafx.concurrent.Task;
import javafx.event.ActionEvent;
import javafx.event.Event;
import javafx.event.EventHandler;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.Node;
import javafx.scene.chart.Axis;
import javafx.scene.chart.CategoryAxis;
import javafx.scene.chart.XYChart;
import javafx.scene.control.Button;
import javafx.scene.control.ComboBox;
import javafx.scene.control.Label;
import javafx.scene.control.RadioButton;
import javafx.scene.control.SelectionModel;
import javafx.scene.control.Tab;
import javafx.scene.control.TabPane;
import javafx.scene.control.TextField;
import javafx.scene.layout.AnchorPane;
import javafx.scene.shape.Rectangle;
import javafx.stage.FileChooser;
import javafx.stage.FileChooser.ExtensionFilter;
import jp.co.ntt.oss.heapstats.WindowController;
import jp.co.ntt.oss.heapstats.container.snapshot.ObjectData;
import jp.co.ntt.oss.heapstats.container.snapshot.SnapShotHeader;
import jp.co.ntt.oss.heapstats.container.snapshot.SummaryData;
import jp.co.ntt.oss.heapstats.plugin.PluginController;
import jp.co.ntt.oss.heapstats.plugin.builtin.snapshot.tabs.HistogramController;
import jp.co.ntt.oss.heapstats.plugin.builtin.snapshot.tabs.RefTreeController;
import jp.co.ntt.oss.heapstats.plugin.builtin.snapshot.tabs.SnapshotController;
import jp.co.ntt.oss.heapstats.plugin.builtin.snapshot.tabs.SummaryController;
import jp.co.ntt.oss.heapstats.task.CSVDumpGC;
import jp.co.ntt.oss.heapstats.task.CSVDumpHeap;
import jp.co.ntt.oss.heapstats.task.ParseHeader;
import jp.co.ntt.oss.heapstats.utils.HeapStatsUtils;
import jp.co.ntt.oss.heapstats.utils.TaskAdapter;

/**
 * FXML Controller of SnapShot builtin plugin.
 *
 * @author Yasumasa Suenaga
 */
public class SnapShotController extends PluginController implements Initializable {

    @FXML
    private SummaryController summaryController;

    @FXML
    private HistogramController histogramController;

    @FXML
    private SnapshotController snapshotController;

    @FXML
    private RefTreeController reftreeController;

    @FXML
    private ComboBox<SnapShotHeader> startCombo;

    @FXML
    private ComboBox<SnapShotHeader> endCombo;

    @FXML
    private TextField snapshotList;

    @FXML
    private RadioButton radioInstance;

    @FXML
    private Button okBtn;

    @FXML
    private TabPane snapshotMain;

    @FXML
    private Tab histogramTab;

    @FXML
    private Tab snapshotTab;

    @FXML
    private Tab reftreeTab;

    private ObjectProperty<ObservableList<SnapShotHeader>> currentTarget;

    private ObjectProperty<SummaryData> summaryData;

    private ObjectProperty<ObservableSet<String>> currentClassNameSet;

    private ObjectProperty<SelectionModel<SnapShotHeader>> snapshotSelectionModel;

    private ObjectProperty<ObservableMap<LocalDateTime, List<ObjectData>>> topNList;

    private ObjectProperty<SnapShotHeader> currentSnapShotHeader;

    private LongProperty currentObjectTag;

    /**
     * Initializes the controller class.
     */
    @Override
    public void initialize(URL url, ResourceBundle rb) {
        super.initialize(url, rb);

        summaryData = new SimpleObjectProperty<>();
        summaryController.summaryDataProperty().bind(summaryData);
        currentTarget = new SimpleObjectProperty<>(FXCollections.emptyObservableList());
        summaryController.currentTargetProperty().bind(currentTarget);
        histogramController.currentTargetProperty().bind(currentTarget);
        snapshotController.currentTargetProperty().bind(currentTarget);
        currentClassNameSet = new SimpleObjectProperty<>();
        summaryController.currentClassNameSetProperty().bind(currentClassNameSet);
        histogramController.currentClassNameSetProperty().bind(currentClassNameSet);
        histogramController.instanceGraphProperty().bind(radioInstance.selectedProperty());
        snapshotController.instanceGraphProperty().bind(radioInstance.selectedProperty());
        snapshotSelectionModel = new SimpleObjectProperty<>();
        snapshotSelectionModel.bind(snapshotController.snapshotSelectionModelProperty());
        histogramController.snapshotSelectionModelProperty().bind(snapshotSelectionModel);
        histogramController.setDrawRebootSuspectLine(this::drawRebootSuspectLine);
        topNList = new SimpleObjectProperty<>();
        topNList.bind(histogramController.topNListProperty());
        snapshotController.topNListProperty().bind(topNList);
        currentSnapShotHeader = new SimpleObjectProperty<>();
        currentSnapShotHeader.bind(snapshotController.snapshotSelectionModelProperty().get().selectedItemProperty());
        reftreeController.currentSnapShotHeaderProperty().bind(currentSnapShotHeader);

        currentObjectTag = new SimpleLongProperty();
        // TODO:
        //   Why can I NOT use binding?
        //   First binding is enabled. But second binding is disabled.
        //   So I use ChangeListener to avoid this issue.
        //currentObjectTag.bind(histogramController.currentObjectTagProperty());
        //currentObjectTag.bind(snapshotController.currentObjectTagProperty());
        histogramController.currentObjectTagProperty().addListener((v, o, n) -> Optional.ofNullable(n).ifPresent(m -> currentObjectTag.set((Long) m)));
        snapshotController.currentObjectTagProperty().addListener((v, o, n) -> Optional.ofNullable(n).ifPresent(m -> currentObjectTag.set((Long) m)));
        reftreeController.currentObjectTagProperty().bind(currentObjectTag);

        snapshotMain.getSelectionModel().selectedItemProperty().addListener(this::onTabChanged);

        startCombo.setConverter(new SnapShotHeaderConverter());
        endCombo.setConverter(new SnapShotHeaderConverter());

        okBtn.disableProperty().bind(startCombo.getSelectionModel().selectedIndexProperty().greaterThanOrEqualTo(endCombo.getSelectionModel().selectedIndexProperty()));

        setOnWindowResize((v, o, n) -> Platform.runLater(() -> Stream.of(summaryController.getHeapChart(),
                summaryController.getInstanceChart(),
                summaryController.getGcTimeChart(),
                summaryController.getMetaspaceChart(),
                histogramController.getTopNChart())
                .forEach(c -> Platform.runLater(() -> drawRebootSuspectLine(c)))));

        histogramController.setTaskExecutor(t -> {
            bindTask(t);
            (new Thread(t)).start();
        });
    }

    private void onTabChanged(ObservableValue<? extends Tab> observable, Tab oldValue, Tab newValue) {
        if (newValue == reftreeTab) {
            if (oldValue == histogramTab) {
                currentObjectTag.set(histogramController.currentObjectTagProperty().get());
            } else if (oldValue == snapshotTab) {
                currentObjectTag.set(snapshotController.currentObjectTagProperty().get());
            }
        }
    }

    /**
     * Event handler of SnapShot file button.
     *
     * @param event ActionEvent of this event.
     */
    @FXML
    public void onSnapshotFileClick(ActionEvent event) {
        FileChooser dialog = new FileChooser();
        ResourceBundle resource = ResourceBundle.getBundle("snapshotResources", new Locale(HeapStatsUtils.getLanguage()));

        dialog.setTitle(resource.getString("dialog.filechooser.title"));
        dialog.setInitialDirectory(new File(HeapStatsUtils.getDefaultDirectory()));
        dialog.getExtensionFilters().addAll(new ExtensionFilter("SnapShot file (*.dat)", "*.dat"),
                new ExtensionFilter("All files", "*.*"));

        List<File> snapshotFileList = dialog.showOpenMultipleDialog(WindowController.getInstance().getOwner());

        if (snapshotFileList != null) {
            clearAllItems();
            
            HeapStatsUtils.setDefaultDirectory(snapshotFileList.get(0).getParent());
            List<String> files = snapshotFileList.stream()
                    .map(File::getAbsolutePath)
                    .collect(Collectors.toList());
            snapshotList.setText(files.stream().collect(Collectors.joining("; ")));

            TaskAdapter<ParseHeader> task = new TaskAdapter<>(new ParseHeader(files, HeapStatsUtils.getReplaceClassName(), true));
            task.setOnSucceeded(evt -> {
                ObservableList<SnapShotHeader> list = FXCollections.observableArrayList(task.getTask().getSnapShotList());
                startCombo.setItems(list);
                endCombo.setItems(list);
                startCombo.getSelectionModel().selectFirst();
                endCombo.getSelectionModel().selectLast();
            });
            super.bindTask(task);

            Thread parseThread = new Thread(task);
            parseThread.start();
        }

    }

    /**
     * Event handler of OK button.
     *
     * @param event ActionEvent of this event.
     */
    @FXML
    private void onOkClick(ActionEvent event) {
        int startIdx = startCombo.getSelectionModel().getSelectedIndex();
        int endIdx = endCombo.getSelectionModel().getSelectedIndex();
        currentTarget.set(FXCollections.observableArrayList(startCombo.getItems().subList(startIdx, endIdx + 1)));
        currentClassNameSet.set(FXCollections.observableSet());
        summaryData.set(new SummaryData(currentTarget.get()));

        Task<Void> topNTask = histogramController.getDrawTopNDataTask(currentTarget.get(), true, null);
        super.bindTask(topNTask);
        Thread topNThread = new Thread(topNTask);
        topNThread.start();

        Task<Void> summarizeTask = summaryController.getCalculateGCSummaryTask(this::drawRebootSuspectLine);
        super.bindTask(summarizeTask);
        Thread summarizeThread = new Thread(summarizeTask);
        summarizeThread.start();
    }

    /**
     * Returns plugin name. This value is used to show in main window tab.
     *
     * @return Plugin name.
     */
    @Override
    public String getPluginName() {
        return "SnapShot Data";
    }

    @Override
    public EventHandler<Event> getOnPluginTabSelected() {
        return null;
    }

    @Override
    public String getLicense() {
        return PluginController.LICENSE_GPL_V2;
    }

    @Override
    public Map<String, String> getLibraryLicense() {
        Map<String, String> licenseMap = new HashMap<>();
        licenseMap.put("JGraphX", PluginController.LICENSE_BSD);

        return licenseMap;
    }

    /**
     * Dump GC Statistics to CSV.
     *
     * @param isSelected If this value is true, this method dumps data which is
     * selected time range, otherwise this method dumps all snapshot data.
     */
    public void dumpGCStatisticsToCSV(boolean isSelected) {
        FileChooser dialog = new FileChooser();
        dialog.setTitle("Select CSV files");
        dialog.setInitialDirectory(new File(HeapStatsUtils.getDefaultDirectory()));
        dialog.getExtensionFilters().addAll(new ExtensionFilter("CSV file (*.csv)", "*.csv"),
                new ExtensionFilter("All files", "*.*"));
        File csvFile = dialog.showSaveDialog(WindowController.getInstance().getOwner());

        if (csvFile != null) {
            TaskAdapter<CSVDumpGC> task = new TaskAdapter<>(new CSVDumpGC(csvFile, isSelected ? currentTarget.get() : startCombo.getItems()));
            super.bindTask(task);

            Thread parseThread = new Thread(task);
            parseThread.start();
        }

    }

    /**
     * Dump Java Class Histogram to CSV.
     *
     * @param isSelected If this value is true, this method dumps data which is
     * selected in class filter, otherwise this method dumps all snapshot data.
     */
    public void dumpClassHistogramToCSV(boolean isSelected) {
        FileChooser dialog = new FileChooser();
        ResourceBundle resource = ResourceBundle.getBundle("snapshotResources", new Locale(HeapStatsUtils.getLanguage()));

        dialog.setTitle(resource.getString("dialog.csvchooser.title"));
        dialog.setInitialDirectory(new File(HeapStatsUtils.getDefaultDirectory()));
        dialog.getExtensionFilters().addAll(new ExtensionFilter("CSV file (*.csv)", "*.csv"),
                new ExtensionFilter("All files", "*.*"));
        File csvFile = dialog.showSaveDialog(WindowController.getInstance().getOwner());

        if (csvFile != null) {
            Predicate<? super ObjectData> filter = histogramController.getFilter();
            TaskAdapter<CSVDumpHeap> task = new TaskAdapter<>(new CSVDumpHeap(csvFile, isSelected ? currentTarget.get() : startCombo.getItems(), isSelected ? filter : null, HeapStatsUtils.getReplaceClassName()));
            super.bindTask(task);

            Thread parseThread = new Thread(task);
            parseThread.start();
        }

    }

    @Override
    public Runnable getOnCloseRequest() {
        return null;
    }

    @Override
    public void setData(Object data, boolean select) {
        super.setData(data, select);
        snapshotList.setText((String) data);

        TaskAdapter<ParseHeader> task = new TaskAdapter<>(new ParseHeader(Arrays.asList((String) data), HeapStatsUtils.getReplaceClassName(), true));
        task.setOnSucceeded(evt -> {
            startCombo.setItems(FXCollections.observableArrayList(task.getTask().getSnapShotList()));
            endCombo.setItems(FXCollections.observableArrayList(task.getTask().getSnapShotList()));
            startCombo.getSelectionModel().selectFirst();
            endCombo.getSelectionModel().selectLast();
        });
        super.bindTask(task);

        Thread parseThread = new Thread(task);
        parseThread.start();
    }

    private void drawRebootSuspectLine(XYChart<String, ? extends Number> target) {

        if (target.getData().isEmpty() || target.getData().get(0).getData().isEmpty()) {
            return;
        }

        AnchorPane anchor = (AnchorPane) target.getParent().getChildrenUnmodifiable()
                .stream()
                .filter(n -> n instanceof AnchorPane)
                .findAny()
                .get();
        ObservableList<Node> anchorChildren = anchor.getChildren();
        anchorChildren.clear();

        CategoryAxis xAxis = (CategoryAxis) target.getXAxis();
        Axis yAxis = target.getYAxis();
        Label chartTitle = (Label) target.getChildrenUnmodifiable().stream()
                .filter(n -> n.getStyleClass().contains("chart-title"))
                .findFirst()
                .get();

        double startX = xAxis.getLayoutX() + xAxis.getStartMargin() - 1.0d;
        double yPos = yAxis.getLayoutY() + chartTitle.getLayoutY() + chartTitle.getHeight();
        List<Rectangle> rectList = summaryData.get().getRebootSuspectList()
                .stream()
                .map(d -> d.format(HeapStatsUtils.getDateTimeFormatter()))
                .map(s -> new Rectangle(xAxis.getDisplayPosition(s) + startX, yPos, 4d, yAxis.getHeight()))
                .peek(r -> ((Rectangle) r).setStyle("-fx-fill: yellow;"))
                .collect(Collectors.toList());
        anchorChildren.addAll(rectList);
    }

    /**
     * Clear all items in SnapShot plugin.
     */
    public void clearAllItems(){
        currentTarget.set(FXCollections.emptyObservableList());
        summaryData.set(null);
        currentClassNameSet.set(FXCollections.emptyObservableSet());
        currentObjectTag.set(-1);
        
        summaryController.clearAllItems();
        histogramController.clearAllItems();
        snapshotController.clearAllItems();
}
    
}

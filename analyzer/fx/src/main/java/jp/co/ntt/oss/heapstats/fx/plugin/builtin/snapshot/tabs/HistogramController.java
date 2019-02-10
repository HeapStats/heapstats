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

import java.io.File;
import java.net.URL;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Optional;
import java.util.ResourceBundle;
import java.util.function.Consumer;
import java.util.function.Predicate;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import javafx.application.Platform;
import javafx.beans.binding.Bindings;
import javafx.beans.property.BooleanProperty;
import javafx.beans.property.LongProperty;
import javafx.beans.property.ObjectProperty;
import javafx.beans.property.SimpleBooleanProperty;
import javafx.beans.property.SimpleLongProperty;
import javafx.beans.property.SimpleObjectProperty;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.collections.ObservableMap;
import javafx.collections.ObservableSet;
import javafx.concurrent.Task;
import javafx.event.ActionEvent;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.Node;
import javafx.scene.chart.NumberAxis;
import javafx.scene.chart.StackedAreaChart;
import javafx.scene.chart.XYChart;
import javafx.scene.control.Alert;
import javafx.scene.control.Button;
import javafx.scene.control.ButtonType;
import javafx.scene.control.ListView;
import javafx.scene.control.SelectionMode;
import javafx.scene.control.SelectionModel;
import javafx.scene.control.TableCell;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableView;
import javafx.scene.control.TextField;
import javafx.scene.control.Tooltip;
import javafx.scene.control.cell.CheckBoxTableCell;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.scene.input.KeyEvent;
import javafx.scene.layout.AnchorPane;
import javafx.stage.FileChooser;
import javax.xml.bind.JAXB;
import jp.co.ntt.oss.heapstats.container.snapshot.DiffData;
import jp.co.ntt.oss.heapstats.container.snapshot.ObjectData;
import jp.co.ntt.oss.heapstats.container.snapshot.SnapShotHeader;
import jp.co.ntt.oss.heapstats.fx.plugin.builtin.snapshot.BindingFilter;
import jp.co.ntt.oss.heapstats.fx.plugin.builtin.snapshot.ChartColorManager;
import jp.co.ntt.oss.heapstats.task.DiffCalculator;
import jp.co.ntt.oss.heapstats.fx.utils.EpochTimeConverter;
import jp.co.ntt.oss.heapstats.api.utils.HeapStatsUtils;
import jp.co.ntt.oss.heapstats.fx.utils.TaskAdapter;
import jp.co.ntt.oss.heapstats.xml.binding.Filter;
import jp.co.ntt.oss.heapstats.xml.binding.Filters;

/**
 * FXML Controller class for "Histogram" tab in SnapShot plugin.
 */
public class HistogramController implements Initializable {

    @FXML
    private TableView<BindingFilter> excludeTable;

    @FXML
    private TableColumn<BindingFilter, Boolean> hideColumn;

    @FXML
    private TableColumn<BindingFilter, String> excludeNameColumn;

    @FXML
    private TextField searchText;

    @FXML
    private ListView<String> searchList;

    @FXML
    private Button selectFilterApplyBtn;

    @FXML
    private StackedAreaChart<Number, Long> topNChart;

    @FXML
    private AnchorPane topNChartAnchor;

    @FXML
    private NumberAxis topNYAxis;

    @FXML
    private TableView<DiffData> lastDiffTable;

    @FXML
    private TableColumn<DiffData, String> colorColumn;

    @FXML
    private TableColumn<DiffData, String> classNameColumn;

    @FXML
    private TableColumn<DiffData, String> classLoaderColumn;

    @FXML
    private TableColumn<DiffData, Long> instanceColumn;

    @FXML
    private TableColumn<DiffData, Long> totalSizeColumn;

    private ObjectProperty<ObservableList<SnapShotHeader>> currentTarget;

    private ObjectProperty<ObservableSet<String>> currentClassNameSet;

    private BooleanProperty instanceGraph;

    private ObjectProperty<SelectionModel<SnapShotHeader>> snapshotSelectionModel;

    private boolean searchFilterEnable;

    private boolean excludeFilterEnable;

    private ObjectProperty<ObservableMap<LocalDateTime, List<ObjectData>>> topNList;

    private LongProperty currentObjectTag;

    private Consumer<XYChart<Number, ? extends Number>> drawRebootSuspectLine;

    private Consumer<Task<Void>> taskExecutor;

    private List<String> hideRegexList;
    
    private EpochTimeConverter epochTimeConverter;

    /**
     * Initializes the controller class.
     */
    @Override
    public void initialize(URL url, ResourceBundle rb) {
        instanceGraph = new SimpleBooleanProperty();
        currentTarget = new SimpleObjectProperty<>(FXCollections.emptyObservableList());
        currentClassNameSet = new SimpleObjectProperty<>(FXCollections.emptyObservableSet());
        snapshotSelectionModel = new SimpleObjectProperty<>();
        topNList = new SimpleObjectProperty<>();
        currentObjectTag = new SimpleLongProperty();
        searchFilterEnable = false;
        excludeFilterEnable = false;

        hideColumn.setCellValueFactory(new PropertyValueFactory<>("hide"));
        hideColumn.setCellFactory(CheckBoxTableCell.forTableColumn(hideColumn));

        excludeNameColumn.setCellFactory(p -> new TableCell<BindingFilter, String>(){
            @Override
            protected void updateItem(String item, boolean empty) {
                super.updateItem(item, empty);
                BindingFilter filter = (BindingFilter)getTableRow().getItem();
                
                if(!empty && (filter != null)){
                    styleProperty().bind(Bindings.createStringBinding(() -> filter.appliedProperty().get() ? "-fx-background-color: skyblue;" : "-fx-background-color: white;", filter.appliedProperty()));
                    setText(filter.getName());
                }

            }
        });

        colorColumn.setCellFactory(p -> new TableCell<DiffData, String>() {
            @Override
            protected void updateItem(String item, boolean empty) {
                super.updateItem(item, empty);
                String style = Optional.ofNullable((DiffData) getTableRow().getItem())
                        .filter(d -> d.isRanked())
                        .map(d -> "-fx-background-color: " + ChartColorManager.getNextColor(d.getClassName()))
                        .orElse("-fx-background-color: transparent;");
                setStyle(style);
            }
        });

        classNameColumn.setCellValueFactory(new PropertyValueFactory<>("className"));
        classLoaderColumn.setCellValueFactory(new PropertyValueFactory<>("classLoaderName"));
        instanceColumn.setCellValueFactory(new PropertyValueFactory<>("instances"));
        instanceColumn.setSortType(TableColumn.SortType.DESCENDING);
        totalSizeColumn.setCellValueFactory(new PropertyValueFactory<>("totalSize"));
        totalSizeColumn.setSortType(TableColumn.SortType.DESCENDING);

        searchList.getSelectionModel().setSelectionMode(SelectionMode.MULTIPLE);

        topNChart.lookup(".chart").setStyle("-fx-background-color: " + HeapStatsUtils.getChartBgColor() + ";");
        topNChart.getXAxis().setTickMarkVisible(HeapStatsUtils.getTickMarkerSwitch());

        searchFilterEnable = false;
        excludeFilterEnable = false;

        selectFilterApplyBtn.disableProperty().bind(searchList.selectionModelProperty().getValue().selectedItemProperty().isNull());
        currentObjectTag.bind(Bindings.createLongBinding(() -> Optional.ofNullable(lastDiffTable.getSelectionModel().getSelectedItem())
                .map(d -> d.getTag())
                .orElse(0xffffffffffffffffl),
                lastDiffTable.getSelectionModel().selectedItemProperty()));
        
        epochTimeConverter = new EpochTimeConverter();
    }

    /**
     * Build TopN Data for Chart with givien data.
     *
     * @param header SnapShot header which you want to build.
     * @param seriesMap Chart series map which is contains class name as key,
     * chart series as value.
     * @param objData ObjectData which is you want to build.
     */
    private void buildTopNChartData(SnapShotHeader header, Map<String, XYChart.Series<Number, Long>> seriesMap, ObjectData objData) {
        XYChart.Series<Number, Long> series = seriesMap.get(objData.getName());

        if (series == null) {
            series = new XYChart.Series<>();
            series.setName(objData.getName());
            topNChart.getData().add(series);
            seriesMap.put(objData.getName(), series);
        }

        long time = header.getSnapShotDate().atZone(ZoneId.systemDefault()).toEpochSecond();
        long yValue = instanceGraph.get() ? objData.getCount()
                : objData.getTotalSize() / 1024 / 1024;
        XYChart.Data<Number, Long> data = new XYChart.Data<>(time, yValue);

        series.getData().add(data);
        String unit = instanceGraph.get() ? "instances" : "MB";
        String tip = String.format("%s: %s, %d " + unit, series.getName(), epochTimeConverter.toString(time), yValue);
        Tooltip.install(data.getNode(), new Tooltip(tip));
    }

    private void setTopNChartColor(XYChart.Series<Number, Long> series) {
        String color = ChartColorManager.getNextColor(series.getName());

        series.getNode().lookup(".chart-series-area-line").setStyle(String.format("-fx-stroke: %s;", color));
        series.getNode().lookup(".chart-series-area-fill").setStyle(String.format("-fx-fill: %s;", color));

        series.getData().stream()
                .map(d -> d.getNode().lookup(".chart-area-symbol"))
                .forEach(n -> n.setStyle(String.format("-fx-background-color: %s, white;", color)));
    }

    /**
     * onSucceeded event handler for DiffTask.
     *
     * @param diff Target task.
     * @param seriesMap Chart series map which is contains class name as key,
     * chart series as value.
     */
    private void onDiffTaskSucceeded(DiffCalculator diff, Map<String, XYChart.Series<Number, Long>> seriesMap) {
        /* Set chart range */
        long startEpoch = currentTarget.get().get(0).getSnapShotDate().atZone(ZoneId.systemDefault()).toEpochSecond();
        long endEpoch = currentTarget.get().get(currentTarget.get().size() - 1).getSnapShotDate().atZone(ZoneId.systemDefault()).toEpochSecond();
        NumberAxis xAxis = (NumberAxis)topNChart.getXAxis();
        xAxis.setTickUnit((endEpoch - startEpoch) / HeapStatsUtils.getXTickUnit());
        xAxis.setLowerBound(startEpoch);
        xAxis.setUpperBound(endEpoch);
        
        topNList.set(FXCollections.observableMap(diff.getTopNList()));

        currentTarget.get().stream()
                .forEachOrdered(h -> topNList.get().get(h.getSnapShotDate()).stream()
                        .forEachOrdered(o -> buildTopNChartData(h, seriesMap, o)));

        lastDiffTable.getItems().addAll(diff.getLastDiffList());
        lastDiffTable.getSortOrder().add(instanceGraph.get() ? instanceColumn : totalSizeColumn);
        topNChart.getData().forEach(this::setTopNChartColor);
        Platform.runLater(() -> drawRebootSuspectLine.accept(topNChart));

        long maxVal = topNChart.getData().stream()
                .flatMap(s -> s.dataProperty().get().stream())
                .collect(Collectors.groupingBy(d -> d.getXValue(), Collectors.summingLong(d -> d.getYValue())))
                .values()
                .stream()
                .mapToLong(Long::longValue)
                .max()
                .getAsLong();

        topNYAxis.setUpperBound(maxVal * 1.05d);
        topNYAxis.setTickUnit(maxVal / 20);
        topNYAxis.setLabel(instanceGraph.get() ? "instances" : "MB");

        snapshotSelectionModel.get().selectLast();
        
        if(excludeFilterEnable){
            excludeTable.getItems().stream()
                    .forEach(f -> f.appliedProperty().set(f.isHide()));
    }
        else{
            excludeTable.getItems().stream()
                    .forEach(f -> f.appliedProperty().set(false));
        }

    }

    private void putIfAbsentToExcludeFilter(Filter filter){
        Optional<BindingFilter> exists = excludeTable.getItems().stream()
                                                         .filter(f -> f.getName().equals(filter.getName()))
                                                         .findAny();
        
        if(exists.isPresent()){
            Alert dialog = new Alert(Alert.AlertType.ERROR, filter.getName() + " already exists!", ButtonType.OK);
            dialog.showAndWait();
        }
        else{
            excludeTable.getItems().add(new BindingFilter(filter));
        }
        
    }

    /**
     * Event handler for adding exclude filter.
     *
     * @param event ActionEvent of this event.
     */
    @FXML
    private void onAddClick(ActionEvent event) {
        FileChooser dialog = new FileChooser();
        ResourceBundle resource = ResourceBundle.getBundle("snapshotResources", new Locale(HeapStatsUtils.getLanguage()));

        dialog.setTitle(resource.getString("dialog.filterchooser.title"));
        dialog.setInitialDirectory(new File(HeapStatsUtils.getDefaultDirectory()));
        dialog.getExtensionFilters().addAll(new FileChooser.ExtensionFilter("Filter file (*.xml)", "*.xml"),
                new FileChooser.ExtensionFilter("All files", "*.*"));

        List<File> excludeFilterList = dialog.showOpenMultipleDialog(((Node) event.getSource()).getScene().getWindow());
        if (excludeFilterList != null) {
            excludeFilterList.stream()
                    .map(f -> (Filters) JAXB.unmarshal(f, Filters.class))
                    .filter(f -> f != null)
                    .flatMap(f -> f.getFilter().stream())
                    .forEach(this::putIfAbsentToExcludeFilter);
        }

    }

    /**
     * Drawing and Showing table with beging selected value.
     *
     * @return Class name filter. null if any filter is disabled.
     */
    public Predicate<? super ObjectData> getFilter() {
        HashSet<String> targetSet = new HashSet<>(searchList.getSelectionModel().getSelectedItems());
        Predicate<ObjectData> searchFilter = o -> targetSet.contains(o.getName());
        Predicate<ObjectData> hideFilter = o -> hideRegexList.stream().noneMatch(s -> o.getName().matches(s));

        Predicate<ObjectData> filter;

        if (searchFilterEnable && excludeFilterEnable) {
            filter = searchFilter.and(hideFilter);
        } else if (searchFilterEnable) {
            filter = searchFilter;
        } else if (excludeFilterEnable) {
            filter = hideFilter;
        } else {
            filter = null;
        }

        return filter;
    }

    /**
     * Event handler of apply button in exclude filter.
     *
     * @param event ActionEvent of this event.
     */
    @FXML
    private void onHiddenFilterApply(ActionEvent event) {
        excludeFilterEnable = true;
        hideRegexList = excludeTable.getItems().stream()
                .filter(f -> f.isHide())
                .flatMap(f -> f.getClasses().getName().stream())
                .map(s -> ".*" + s + ".*")
                .collect(Collectors.toList());
        Predicate<? super ObjectData> filter = getFilter();

        taskExecutor.accept(getDrawTopNDataTask(currentTarget.get(), false, filter));
    }

    /**
     * Event handler of changing search TextField.
     *
     * @param event KeyEvent of this event.
     */
    @FXML
    private void onSearchTextChanged(KeyEvent event) {
        Stream<String> searchStrings = currentClassNameSet.get().stream();
        
        if(excludeFilterEnable){
            searchStrings = searchStrings.filter(n -> hideRegexList.stream().noneMatch(s -> n.matches(s)));
        }
        
        searchList.getItems().clear();
        searchList.getItems().addAll(searchStrings.filter(n -> n.contains(searchText.getText()))
                .collect(Collectors.toList()));
    }

    /**
     * Selection method for incremental search.
     *
     * @param event ActionEvent of this event.
     */
    @FXML
    private void onSelectFilterApply(ActionEvent event) {
        searchFilterEnable = true;
        Predicate<? super ObjectData> filter = getFilter();

        taskExecutor.accept(getDrawTopNDataTask(currentTarget.get(), false, filter));
    }

    /**
     * Event handler of clear button.
     *
     * @param event ActionEvent of this event.
     */
    @FXML
    private void onSelectFilterClear(ActionEvent event) {
        searchFilterEnable = false;
        excludeFilterEnable = false;

        searchText.setText("");
        searchList.getItems().clear();

        taskExecutor.accept(getDrawTopNDataTask(currentTarget.get(), true, null));
    }

    /**
     * Get task for drawing Top N data to Chart and Table.
     *
     * @param target SnapShot to be drawed.
     * @param includeOthers *Others* data should be included in this Top N data.
     * @param predicate Filter function.
     * @return Task for drawing Top N data.
     */
    public Task<Void> getDrawTopNDataTask(List<SnapShotHeader> target, boolean includeOthers, Predicate<? super ObjectData> predicate) {
        topNChart.getData().clear();
        lastDiffTable.getItems().clear();
        Map<String, XYChart.Series<Number, Long>> seriesMap = new HashMap<>();

        TaskAdapter<DiffCalculator> diff = new TaskAdapter<>(new DiffCalculator(target, HeapStatsUtils.getRankLevel(),
                includeOthers, predicate, HeapStatsUtils.getReplaceClassName(), instanceGraph.get()));
        diff.setOnSucceeded(evt -> onDiffTaskSucceeded(diff.getTask(), seriesMap));

        return diff;
    }

    /**
     * Set Consumer for drawing reboot line.
     *
     * @param drawRebootSuspectLine Consumer for drawing reboot line.
     */
    public void setDrawRebootSuspectLine(Consumer<XYChart<Number, ? extends Number>> drawRebootSuspectLine) {
        this.drawRebootSuspectLine = drawRebootSuspectLine;
    }

    /**
     * Set Consumer for executing JavaFX task. This Consumer is used for tasks
     * which should be shown ProgressIndicator.
     *
     * @param taskExecutor Consumer for executing JavaFX task.
     */
    public void setTaskExecutor(Consumer<Task<Void>> taskExecutor) {
        this.taskExecutor = taskExecutor;
    }

    /**
     * Get property which contains chart category (instance count or object
     * size).
     *
     * @return Property which contains chart category.
     */
    public BooleanProperty instanceGraphProperty() {
        return instanceGraph;
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
     * Get property for current SnapShot selection.
     *
     * @return Property for current SnapShot selection.
     */
    public ObjectProperty<SelectionModel<SnapShotHeader>> snapshotSelectionModelProperty() {
        return snapshotSelectionModel;
    }

    /**
     * Get TopN chart.
     *
     * @return TopN chart.
     */
    public StackedAreaChart<Number, Long> getTopNChart() {
        return topNChart;
    }

    /**
     * Get property of Map of Top N list.
     *
     * @return Property of map of Top N list.
     */
    public ObjectProperty<ObservableMap<LocalDateTime, List<ObjectData>>> topNListProperty() {
        return topNList;
    }

    /**
     * Get property of current Object tag.
     *
     * @return Property of currentObjectTag.
     */
    public LongProperty currentObjectTagProperty() {
        return currentObjectTag;
    }

    /**
     * Get selected diff data.
     *
     * @return Selected diff data.
     */
    public DiffData getSelectedData() {
        return lastDiffTable.getSelectionModel().getSelectedItem();
    }

    /**
     * Clear all items in Histogram tab.
     */
    public void clearAllItems(){
        excludeTable.getItems().stream()
                .forEach((f -> f.setHide(false)));
        excludeFilterEnable = false;
        
        searchText.setText("");
        searchList.getItems().clear();
        searchFilterEnable = false;
        
        topNChart.getData().clear();
        topNChartAnchor.getChildren().clear();
        
        lastDiffTable.getItems().clear();
}
    
}

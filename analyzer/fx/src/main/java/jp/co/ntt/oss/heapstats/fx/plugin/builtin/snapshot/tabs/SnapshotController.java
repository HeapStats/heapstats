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
import java.time.LocalDateTime;
import java.util.AbstractMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Optional;
import java.util.ResourceBundle;
import java.util.stream.Collectors;
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
import javafx.event.ActionEvent;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.chart.PieChart;
import javafx.scene.control.ComboBox;
import javafx.scene.control.SingleSelectionModel;
import javafx.scene.control.TableCell;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableView;
import javafx.scene.control.cell.PropertyValueFactory;
import jp.co.ntt.oss.heapstats.container.snapshot.ObjectData;
import jp.co.ntt.oss.heapstats.container.snapshot.SnapShotHeader;
import jp.co.ntt.oss.heapstats.fx.plugin.builtin.snapshot.ChartColorManager;
import jp.co.ntt.oss.heapstats.fx.plugin.builtin.snapshot.SnapShotHeaderConverter;
import jp.co.ntt.oss.heapstats.api.utils.HeapStatsUtils;

/**
 * FXML Controller class for "SnapShot Data" tab in SnapShot plugin.
 */
public class SnapshotController implements Initializable {

    @FXML
    private ComboBox<SnapShotHeader> snapShotTimeCombo;

    @FXML
    private TableView<Map.Entry<String, String>> snapShotSummaryTable;

    @FXML
    private TableColumn<Map.Entry<String, String>, String> snapShotSummaryKey;

    @FXML
    private TableColumn<Map.Entry<String, String>, String> snapShotSummaryValue;

    @FXML
    private PieChart usagePieChart;

    @FXML
    private TableColumn<ObjectData, String> objColorColumn;

    @FXML
    private TableView<ObjectData> objDataTable;

    @FXML
    private TableColumn<ObjectData, String> objClassNameColumn;

    @FXML
    private TableColumn<ObjectData, String> objClassLoaderColumn;

    @FXML
    private TableColumn<ObjectData, Long> objInstancesColumn;

    @FXML
    private TableColumn<ObjectData, Long> objSizeColumn;

    private ObjectProperty<ObservableList<SnapShotHeader>> currentTarget;

    private BooleanProperty instanceGraph;

    private ObjectProperty<ObservableMap<LocalDateTime, List<ObjectData>>> topNList;

    private LongProperty currentObjectTag;

    /**
     * Initializes the controller class.
     */
    @Override
    public void initialize(URL url, ResourceBundle rb) {
        currentTarget = new SimpleObjectProperty<>(FXCollections.emptyObservableList());
        instanceGraph = new SimpleBooleanProperty();
        topNList = new SimpleObjectProperty<>();
        currentObjectTag = new SimpleLongProperty();

        snapShotTimeCombo.itemsProperty().bind(currentTarget);

        snapShotTimeCombo.setConverter(new SnapShotHeaderConverter());
        snapShotSummaryKey.setCellValueFactory(new PropertyValueFactory<>("key"));
        snapShotSummaryValue.setCellValueFactory(new PropertyValueFactory<>("value"));

        objColorColumn.setCellFactory(p -> new TableCell<ObjectData, String>() {
            @Override
            protected void updateItem(String item, boolean empty) {
                super.updateItem(item, empty);
                String style = Optional.ofNullable((ObjectData) getTableRow().getItem())
                        .map(o -> "-fx-background-color: " + ChartColorManager.getCurrentColor(o.getName()) + ";")
                        .orElse("-fx-background-color: transparent;");
                setStyle(style);
            }
        });

        objClassNameColumn.setCellValueFactory(new PropertyValueFactory<>("name"));
        objClassLoaderColumn.setCellValueFactory(new PropertyValueFactory<>("loaderName"));
        objInstancesColumn.setCellValueFactory(new PropertyValueFactory<>("count"));
        objInstancesColumn.setSortType(TableColumn.SortType.DESCENDING);
        objSizeColumn.setCellValueFactory(new PropertyValueFactory<>("totalSize"));
        objSizeColumn.setSortType(TableColumn.SortType.DESCENDING);

        currentObjectTag.bind(Bindings.createLongBinding(() -> Optional.ofNullable(objDataTable.getSelectionModel().getSelectedItem())
                .map(o -> o.getTag())
                .orElse(0xffffffffffffffffl),
                objDataTable.getSelectionModel().selectedItemProperty()));
    }

    /**
     * Event handler of SnapShot TIme.
     *
     * @param event ActionEvent of this event.
     */
    @FXML
    @SuppressWarnings("unchecked")
    private void onSnapShotTimeSelected(ActionEvent event) {
        SnapShotHeader header = snapShotTimeCombo.getSelectionModel().getSelectedItem();
        if (header == null) {
            return;
        }

        ObservableList<Map.Entry<String, String>> summaryList = snapShotSummaryTable.getItems();
        summaryList.clear();
        usagePieChart.getData().clear();
        objDataTable.getItems().clear();
        ResourceBundle resource = ResourceBundle.getBundle("snapshotResources", new Locale(HeapStatsUtils.getLanguage()));

        summaryList.addAll(
                new AbstractMap.SimpleEntry<>(resource.getString("snapshot.date"), header.getSnapShotDate().format(HeapStatsUtils.getDateTimeFormatter())),
                new AbstractMap.SimpleEntry<>(resource.getString("snapshot.hasreftree"), header.hasReferenceData() ? "Yes" : "N/A"),
                new AbstractMap.SimpleEntry<>(resource.getString("snapshot.entries"), Long.toString(header.getNumEntries())),
                new AbstractMap.SimpleEntry<>(resource.getString("snapshot.instances"), Long.toString(header.getNumInstances())),
                new AbstractMap.SimpleEntry<>(resource.getString("snapshot.heap"), String.format("%.02f MB", (double) (header.getNewHeap() + header.getOldHeap()) / 1024.0d / 1024.0d)),
                new AbstractMap.SimpleEntry<>(resource.getString("snapshot.metaspace"), String.format("%.02f MB", (double) (header.getMetaspaceUsage()) / 1024.0d / 1024.0d)),
                new AbstractMap.SimpleEntry<>(resource.getString("snapshot.cause"), header.getCauseString()),
                new AbstractMap.SimpleEntry<>(resource.getString("snapshot.gccause"), header.getGcCause()),
                new AbstractMap.SimpleEntry<>(resource.getString("snapshot.gctime"), String.format("%d ms", header.getGcTime())));

        usagePieChart.getData().addAll(topNList.get().get(header.getSnapShotDate()).stream()
                .map(o -> new PieChart.Data(o.getName(), instanceGraph.get() ? o.getCount() : o.getTotalSize()))
                .collect(Collectors.toList()));
        usagePieChart.getData().stream()
                .forEach(d -> d.getNode().setStyle("-fx-pie-color: " + ChartColorManager.getNextColor(d.getName())));

        objDataTable.setItems(FXCollections.observableArrayList(
                header.getSnapShot(HeapStatsUtils.getReplaceClassName()).values().stream().collect(Collectors.toList())));
        objDataTable.getSortOrder().add(instanceGraph.get() ? objInstancesColumn : objSizeColumn);
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
     * Get property for current SnapShot selection. This property is same as
     * SnapShot time ComboBox.
     *
     * @return Property for current SnapShot selection.
     */
    public ObjectProperty<SingleSelectionModel<SnapShotHeader>> snapshotSelectionModelProperty() {
        return snapShotTimeCombo.selectionModelProperty();
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
     * Get Object Data table.
     *
     * @return Object Data table.
     */
    public TableView<ObjectData> getObjDataTable() {
        return objDataTable;
    }

    /**
     * Clear all items in SnapShot tab.
     */
    public void clearAllItems(){
        snapShotSummaryTable.getItems().clear();
        usagePieChart.getData().clear();
        objDataTable.getItems().clear();
}
    
}

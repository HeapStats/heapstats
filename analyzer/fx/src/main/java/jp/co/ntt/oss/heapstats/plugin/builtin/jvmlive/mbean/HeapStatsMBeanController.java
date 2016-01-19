/*
 * Copyright (C) 2014-2015 Yasumasa Suenaga
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
package jp.co.ntt.oss.heapstats.plugin.builtin.jvmlive.mbean;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.Locale;
import java.util.ResourceBundle;
import java.util.concurrent.ExecutionException;
import javafx.beans.binding.Bindings;
import javafx.beans.property.Property;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.event.ActionEvent;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.control.Alert;
import javafx.scene.control.Alert.AlertType;
import javafx.scene.control.ButtonType;
import javafx.scene.control.CheckBox;
import javafx.scene.control.ChoiceBox;
import javafx.scene.control.Control;
import javafx.scene.control.Label;
import javafx.scene.control.TableCell;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableView;
import javafx.scene.control.TextField;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.stage.FileChooser;
import javafx.util.converter.IntegerStringConverter;
import javafx.util.converter.LongStringConverter;
import jp.co.ntt.oss.heapstats.WindowController;
import jp.co.ntt.oss.heapstats.jmx.JMXHelper;
import jp.co.ntt.oss.heapstats.mbean.HeapStatsMBean;
import jp.co.ntt.oss.heapstats.utils.HeapStatsUtils;

/**
 * FXML Controller class of HeapStatsMBean.
 *
 * @author Yasumasa Suenaga
 */
public class HeapStatsMBeanController implements Initializable {
    
    @FXML
    private Label headerLabel;
    
    @FXML
    TableView<HeapStatsConfig> configTable;
    
    @FXML
    TableColumn<HeapStatsConfig, String> keyColumn;
    
    @FXML
    TableColumn<HeapStatsConfig, Object> valueColumn;
    
    private JMXHelper jmxHelper;
    
    
    private static class ConfigKeyTableCell extends TableCell<HeapStatsConfig, String>{
        
        @Override
        protected void updateItem(String item, boolean empty) {
            super.updateItem(item, empty);
            
            if(!empty){
                HeapStatsConfig config = (HeapStatsConfig)getTableRow().getItem();
                styleProperty().bind(Bindings.createStringBinding(() -> config.changedProperty().get() ? "-fx-text-fill: orange;" : "-fx-text-fill: black;", config.changedProperty()));
                setText(item);
            }
            
       }
        
    }
    
    private static class VariousTableCell extends TableCell<HeapStatsConfig, Object>{
        
        @Override
        @SuppressWarnings("unchecked")
        protected void updateItem(Object item, boolean empty) {
            super.updateItem(item, empty);
            
            if(empty){
               return;
            }

            HeapStatsConfig config = (HeapStatsConfig)getTableRow().getItem();
            if(config == null){
                return;
            }
            
            Control cellContent = config.getCellContent();
            if(item instanceof Boolean){
                if(!(cellContent instanceof CheckBox)){
                    cellContent = new CheckBox();
                    ((CheckBox)cellContent).selectedProperty().bindBidirectional(config.valueProperty());
                }
            }
            else if(item instanceof HeapStatsMBean.LogLevel){
                if(!(cellContent instanceof ChoiceBox)){
                    cellContent = new ChoiceBox(FXCollections.observableArrayList(HeapStatsMBean.LogLevel.values()));
                    ((ChoiceBox<HeapStatsMBean.LogLevel>)cellContent).valueProperty().bindBidirectional(config.valueProperty());
                }
            }
            else if(item instanceof HeapStatsMBean.RankOrder){
                if(!(cellContent instanceof ChoiceBox)){
                    cellContent = new ChoiceBox(FXCollections.observableArrayList(HeapStatsMBean.RankOrder.values()));
                    ((ChoiceBox<HeapStatsMBean.RankOrder>)cellContent).valueProperty().bindBidirectional(config.valueProperty());
                }
            }
            else if(item instanceof Integer){
                if(!(cellContent instanceof TextField)){
                    cellContent = new TextField();
                    ((TextField)cellContent).textProperty().bindBidirectional((Property<Integer>)config.valueProperty(), new IntegerStringConverter());
                }
            }
            else if(item instanceof Long){
                if(!(cellContent instanceof TextField)){
                    cellContent = new TextField();
                    ((TextField)cellContent).textProperty().bindBidirectional((Property<Long>)config.valueProperty(), new LongStringConverter());
                }
            }
            else{
                if(!(cellContent instanceof TextField)){
                    cellContent = new TextField();
                    ((TextField)cellContent).textProperty().bindBidirectional(config.valueProperty());
                }
            }
            
            config.setCellContent(cellContent);
            setGraphic(cellContent);
        }
        
    }
    

    /**
     * Initializes the controller class.
     */
    @Override
    public void initialize(URL url, ResourceBundle rb) {
        keyColumn.setCellValueFactory(new PropertyValueFactory<>("key"));
        keyColumn.setCellFactory(p -> new ConfigKeyTableCell());
        valueColumn.setCellValueFactory(new PropertyValueFactory<>("value"));
        valueColumn.setCellFactory(p -> new VariousTableCell());
    }    

    /**
     * Get JMXHelper instance.
     * 
     * @return Instance of JMXHelper.
     */
    public JMXHelper getJmxHelper() {
        return jmxHelper;
    }

    /**
     * Set JMXHelper instance.
     * 
     * @param jmxHelper New JMXHelper instance.
     */
    public void setJmxHelper(JMXHelper jmxHelper) {
        this.jmxHelper = jmxHelper;
    }
    
    /**
     * Load all configs from remote HeapStats agent through JMX.
     */
    public void loadAllConfigs(){
        headerLabel.setText(jmxHelper.getUrl().toString());
        configTable.setItems(jmxHelper.getMbean().getConfigurationList()
                                                 .entrySet()
                                                 .stream()
                                                 .map(e -> new HeapStatsConfig(e.getKey(), e.getValue()))
                                                 .collect(FXCollections::observableArrayList, ObservableList::add, ObservableList::addAll));
    }
    
    @FXML
    private void onCommitBtnClick(ActionEvent event){
        
        try{
            configTable.getItems().stream()
                                  .filter(c -> c.changedProperty().get())
                                  .forEach(c -> jmxHelper.getMbean().changeConfiguration(c.keyProperty().get(), c.valueProperty().getValue()));
        }
        catch(IllegalArgumentException e){
            HeapStatsUtils.showExceptionDialog(e);
            return;
        }
        
        ResourceBundle resource = ResourceBundle.getBundle("jvmliveResources", new Locale(HeapStatsUtils.getLanguage()));
        Alert dialog = new Alert(AlertType.INFORMATION, resource.getString("dialog.config.message.applyConfig"), ButtonType.OK);
        dialog.show();
        
        loadAllConfigs();
    }

    @FXML
    private void onInvokeResourceBtnClick(ActionEvent event){
        ResourceBundle resource = ResourceBundle.getBundle("jvmliveResources", new Locale(HeapStatsUtils.getLanguage()));
        
        if(jmxHelper.getMbean().invokeLogCollection()){
            Alert dialog = new Alert(AlertType.INFORMATION, resource.getString("dialog.config.message.invoke.resource.success"), ButtonType.OK);
            dialog.show();
        }
        else{
            Alert dialog = new Alert(AlertType.ERROR, resource.getString("dialog.config.message.invoke.resource.fail"), ButtonType.OK);
            dialog.show();
        }
    }

    @FXML
    private void onInvokeAllBtnClick(ActionEvent event){
        ResourceBundle resource = ResourceBundle.getBundle("jvmliveResources", new Locale(HeapStatsUtils.getLanguage()));
        
        if(jmxHelper.getMbean().invokeAllLogCollection()){
            Alert dialog = new Alert(AlertType.INFORMATION, resource.getString("dialog.config.message.invoke.archive.success"), ButtonType.OK);
            dialog.show();
        }
        else{
            Alert dialog = new Alert(AlertType.ERROR, resource.getString("dialog.config.message.invoke.archive.fail"), ButtonType.OK);
            dialog.show();
        }
    }

    @FXML
    private void onInvokeSnapShotBtnClick(ActionEvent event){
        jmxHelper.getMbean().invokeSnapShotCollection();
        ResourceBundle resource = ResourceBundle.getBundle("jvmliveResources", new Locale(HeapStatsUtils.getLanguage()));
        Alert dialog = new Alert(AlertType.INFORMATION, resource.getString("dialog.config.message.invoke.snapshot"), ButtonType.OK);
        dialog.show();
    }
    
    @FXML
    private void onGetResourceBtnClick(ActionEvent event){
        FileChooser dialog = new FileChooser();
        ResourceBundle resource = ResourceBundle.getBundle("jvmliveResources", new Locale(HeapStatsUtils.getLanguage()));
        dialog.setTitle(resource.getString("dialog.config.resource.save"));
        dialog.setInitialDirectory(new File(HeapStatsUtils.getDefaultDirectory()));
        dialog.getExtensionFilters().addAll(new FileChooser.ExtensionFilter("CSV file (*.csv)", "*.csv"),
                                            new FileChooser.ExtensionFilter("All files", "*.*"));
        File logFile = dialog.showSaveDialog(WindowController.getInstance().getOwner());
        
        if(logFile != null){
            try {
                jmxHelper.getResourceLog(logFile.toPath());
            } catch (IOException | InterruptedException | ExecutionException ex) {
                HeapStatsUtils.showExceptionDialog(ex);
            }
        }
        
    }

    @FXML
    private void onGetSnapShotBtnClick(ActionEvent event){
        FileChooser dialog = new FileChooser();
        ResourceBundle resource = ResourceBundle.getBundle("jvmliveResources", new Locale(HeapStatsUtils.getLanguage()));
        dialog.setTitle(resource.getString("dialog.config.snapshot.save"));
        dialog.setInitialDirectory(new File(HeapStatsUtils.getDefaultDirectory()));
        dialog.getExtensionFilters().addAll(new FileChooser.ExtensionFilter("SnapShot file (*.dat)", "*.dat"),
                                            new FileChooser.ExtensionFilter("All files", "*.*"));
        File snapshotFile = dialog.showSaveDialog(WindowController.getInstance().getOwner());
        
        if(snapshotFile != null){
            try {
                jmxHelper.getSnapShot(snapshotFile.toPath());
            } catch (IOException | InterruptedException | ExecutionException ex) {
                HeapStatsUtils.showExceptionDialog(ex);
            }
        }
        
    }

}

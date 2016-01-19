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

package jp.co.ntt.oss.heapstats;

import java.net.URL;
import java.util.AbstractMap;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.ResourceBundle;
import javafx.event.ActionEvent;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.control.Accordion;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableView;
import javafx.scene.control.TitledPane;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.stage.Stage;
import jp.co.ntt.oss.heapstats.plugin.PluginController;

/**
 * FXML Controller of About dialog.
 *
 * @author Yasumasa Suenaga
 */
public class AboutDialogController implements Initializable {
    
    @FXML
    private Accordion accordion;
    
    @FXML
    private TitledPane pluginPane;
    
    @FXML
    private TableView<Map.Entry<String, String>> pluginTable;
    
    @FXML
    private TableColumn<Map.Entry<String, String>, String> pluinTableNameColumn;

    @FXML
    private TableColumn<Map.Entry<String, String>, String> pluginTableLicenseColumn;
    
    @FXML
    private TableView<PluginController.LibraryLicense> libraryTable;
    
    @FXML
    private TableColumn<PluginController.LibraryLicense, String> libraryTablePluginColumn;

    @FXML
    private TableColumn<PluginController.LibraryLicense, String> libraryTableLibraryColumn;

    @FXML
    private TableColumn<PluginController.LibraryLicense, String> libraryTableLicenseColumn;
    
    private Stage stage;
    
    /**
     * Setter method for Stage.
     * 
     * @param stage Instance of main Stage.
     */
    public void setStage(Stage stage) {
        this.stage = stage;
    }

    /**
     * Getter method for Stage.
     * 
     * @return stage of this dialog.
     */
    public Stage getStage() {
        return stage;
    }
    
    public void setPluginInfo(){
        /*
         * Set plugin info to pluginTable
         * Map.Entry which is implemented in HashMap, Hashtable does not work in TableView.
         * Thus I create array of AbstractMap.SimpleEntry .
         */
        List<AbstractMap.SimpleEntry<String, String>> plugins = new ArrayList<>();
        WindowController.getInstance().getPluginList().forEach((k, v) -> plugins.add(new AbstractMap.SimpleEntry<>(k, v.getLicense())));
        pluginTable.getItems().addAll(plugins);
        
        /* Set library license to libraryTable */
        List<PluginController.LibraryLicense> libraryList = new ArrayList<>();
        WindowController.getInstance().getPluginList().forEach((n, c) -> Optional.ofNullable(c.getLibraryLicense())
                                                                                 .ifPresent(l -> l.forEach((k, v) -> libraryList.add(new PluginController.LibraryLicense(n, k, v)))));
        libraryTable.getItems().addAll(libraryList);
    }

    /**
     * Initializes the controller class.
     */
    @Override
    public void initialize(URL url, ResourceBundle rb) {
        pluinTableNameColumn.setCellValueFactory(new PropertyValueFactory<>("key"));
        pluginTableLicenseColumn.setCellValueFactory(new PropertyValueFactory<>("value"));
        
        libraryTablePluginColumn.setCellValueFactory(new PropertyValueFactory<>("pluginName"));
        libraryTableLibraryColumn.setCellValueFactory(new PropertyValueFactory<>("libraryName"));
        libraryTableLicenseColumn.setCellValueFactory(new PropertyValueFactory<>("license"));
        
        accordion.setExpandedPane(pluginPane);
    }    
  
    /**
     * Event handler when user clickes "OK" button.
     * 
     * @param event ActionEvent of this event.
     */
    @FXML
    private void onOKClick(ActionEvent event){
        stage.close();
    }

}

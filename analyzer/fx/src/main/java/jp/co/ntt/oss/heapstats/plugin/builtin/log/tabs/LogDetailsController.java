/*
 * Copyright (C) 2015 Nippon Telegraph and Telephone Corporation
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

import java.io.IOException;
import java.net.URL;
import java.util.AbstractMap;
import java.util.Map;
import java.util.ResourceBundle;
import javafx.beans.property.ObjectProperty;
import javafx.collections.ObservableList;
import javafx.event.ActionEvent;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.control.ComboBox;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableView;
import javafx.scene.control.TextArea;
import javafx.scene.control.cell.PropertyValueFactory;
import jp.co.ntt.oss.heapstats.container.log.ArchiveData;
import jp.co.ntt.oss.heapstats.plugin.builtin.log.ArchiveDataConverter;
import jp.co.ntt.oss.heapstats.utils.HeapStatsUtils;

/**
 * FXML Controller class for "Log Detail Data" tab in LogData plugin.
 */
public class LogDetailsController implements Initializable {

    @FXML
    private TableView<Map.Entry<String, String>> archiveEnvInfoTable;

    @FXML
    private TableColumn<Map.Entry<String, String>, String> archiveKeyColumn;

    @FXML
    private TableColumn<Map.Entry<String, String>, String> archiveVauleColumn;

    @FXML
    private ComboBox<String> fileCombo;

    @FXML
    private ComboBox<ArchiveData> archiveCombo;

    @FXML
    private TextArea logArea;

    /**
     * Initializes the controller class.
     */
    @Override
    public void initialize(URL url, ResourceBundle rb) {
        archiveCombo.setConverter(new ArchiveDataConverter());
        archiveKeyColumn.setCellValueFactory(new PropertyValueFactory<>("key"));
        archiveVauleColumn.setCellValueFactory(new PropertyValueFactory<>("value"));
    }

    /**
     * Event handler of archive combobox. This handler is fired that user select
     * archive.
     *
     * @param event ActionEvent of this event.
     */
    @FXML
    private void onArchiveComboAction(ActionEvent event) {
        archiveEnvInfoTable.getItems().clear();
        fileCombo.getItems().clear();
        ArchiveData target = archiveCombo.getValue();

        if (target == null) {
            return;
        }

        /*
         * Convert Map to List.
         * Map.Entry of HashMap (HashMap$Node) is package private class. So JavaFX
         * cannot access them through reflection API.
         * Thus I convert Map.Entry to AbstractMap.SimpleEntry.
         */
        target.getEnvInfo().entrySet().forEach(e -> archiveEnvInfoTable.getItems().add(new AbstractMap.SimpleEntry<>(e)));

        fileCombo.getItems().addAll(target.getFileList());
    }

    /**
     * Event handler of selecting log in this archive. This handler is fired
     * that user select log.
     *
     * @param event ActionEvent of this event.
     */
    @FXML
    private void onFileComboAction(ActionEvent event) {
        ArchiveData target = archiveCombo.getValue();
        String file = fileCombo.getValue();

        if((target == null) || (file == null)){
            return;
        }
        
        try {
            logArea.setText(target.getFileContents(file));
        } catch (IOException ex) {
            HeapStatsUtils.showExceptionDialog(ex);
            logArea.setText("");
        }

    }

    /**
     * Get HeapStats ZIP archive list as Property.
     *
     * @return List of HeapStats ZIP archive.
     */
    public ObjectProperty<ObservableList<ArchiveData>> archiveListProperty() {
        return archiveCombo.itemsProperty();
    }

    /**
     * Clear all items in Log Details tab.
     */
    public void clearAllItems(){
        fileCombo.getItems().clear();
        logArea.setText("");
        archiveEnvInfoTable.getItems().clear();
}

}

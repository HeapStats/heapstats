/*
 * Copyright (C) 2014-2017 Yasumasa Suenaga
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
package jp.co.ntt.oss.heapstats.plugin.builtin.log;

import javafx.application.Platform;
import javafx.beans.property.ObjectProperty;
import javafx.beans.property.SimpleObjectProperty;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.concurrent.Task;
import javafx.event.ActionEvent;
import javafx.event.Event;
import javafx.event.EventHandler;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.SplitPane;
import javafx.scene.control.TextField;
import javafx.stage.FileChooser;
import javafx.stage.FileChooser.ExtensionFilter;
import jp.co.ntt.oss.heapstats.MainWindowController;
import jp.co.ntt.oss.heapstats.container.log.ArchiveData;
import jp.co.ntt.oss.heapstats.container.log.DiffData;
import jp.co.ntt.oss.heapstats.container.log.LogData;
import jp.co.ntt.oss.heapstats.lambda.ConsumerWrapper;
import jp.co.ntt.oss.heapstats.lambda.FunctionWrapper;
import jp.co.ntt.oss.heapstats.plugin.PluginController;
import jp.co.ntt.oss.heapstats.plugin.builtin.log.tabs.LogDetailsController;
import jp.co.ntt.oss.heapstats.plugin.builtin.log.tabs.LogResourcesController;
import jp.co.ntt.oss.heapstats.task.ParseLogFile;
import jp.co.ntt.oss.heapstats.utils.HeapStatsUtils;
import jp.co.ntt.oss.heapstats.utils.TaskAdapter;

import java.io.File;
import java.net.URL;
import java.time.LocalDateTime;
import java.time.temporal.ChronoUnit;
import java.util.*;
import java.util.stream.Collectors;

/**
 * FXML Controller of LOG builtin plugin.
 *
 * @author Yasumasa Suenaga
 */
public class LogController extends PluginController implements Initializable {

    @FXML
    private LogResourcesController logResourcesController;

    @FXML
    private LogDetailsController logDetailsController;

    @FXML
    private Label startTimeLabel;

    @FXML
    private Label endTimeLabel;

    @FXML
    private TextField logFileList;

    @FXML
    private Button okBtn;

    @FXML
    private SplitPane rangePane;

    private ObjectProperty<LocalDateTime> rangeStart;

    private ObjectProperty<LocalDateTime> rangeEnd;

    private List<LogData> logEntries;

    private List<DiffData> diffEntries;

    private ObjectProperty<ObservableList<ArchiveData>> archiveList;

    /**
     * Update caption of label which represents time of selection.
     *
     * @param target Label compornent to draw.
     * @param newValue Percentage of timeline. This value is between 0.0 and 1.0 .
     */
    private void updateRangeLabel(Label target, double newValue){
        if(!Optional.ofNullable(logEntries).map(List::isEmpty).orElse(true)){
            LocalDateTime start = logEntries.get(0).getDateTime();
            LocalDateTime end = logEntries.get(logEntries.size() - 1).getDateTime();
            long diff = start.until(end, ChronoUnit.MILLIS);
            LocalDateTime newTime = start.plus((long)(diff * (Math.round(newValue * 100.0d) / 100.0d)), ChronoUnit.MILLIS);

            if(target == startTimeLabel){
                rangeStart.set(newTime.truncatedTo(ChronoUnit.SECONDS));
            }
            else{
                rangeEnd.set(newTime.plusSeconds(1).truncatedTo(ChronoUnit.SECONDS));
            }

            target.setText(newTime.format(HeapStatsUtils.getDateTimeFormatter()));
        }
    }

    /**
     * Initializes the controller class.
     */
    @Override
    public void initialize(URL url, ResourceBundle rb) {
        super.initialize(url, rb);

        logEntries = null;
        diffEntries = null;
        rangeStart = new SimpleObjectProperty<>();
        rangeEnd = new SimpleObjectProperty<>();

        rangePane.getDividers().get(0).positionProperty().addListener((v, o, n) -> updateRangeLabel(startTimeLabel, n.doubleValue()));
        rangePane.getDividers().get(1).positionProperty().addListener((v, o, n) -> updateRangeLabel(endTimeLabel, n.doubleValue()));

        archiveList = new SimpleObjectProperty<>(FXCollections.emptyObservableList());
        logResourcesController.archiveListProperty().bind(archiveList);
        logDetailsController.archiveListProperty().bind(archiveList);
        setOnWindowResize((v, o, n) -> Platform.runLater(logResourcesController::drawEventLineToChart));
    }

    /**
     * onSucceeded event handler for LogFileParser.
     *
     * @param parser Targeted LogFileParser.
     */
    private void onLogFileParserSucceeded(ParseLogFile parser) {
        logEntries = parser.getLogEntries();
        diffEntries = parser.getDiffEntries();

        rangePane.getDividers().get(0).setPosition(0.0d);
        rangePane.getDividers().get(1).setPosition(1.0d);

        rangePane.setDisable(false);
        okBtn.setDisable(false);
    }

    /**
     * Event handler of LogFile button.
     *
     * @param event ActionEvent of this event.
     */
    @FXML
    public void onLogFileClick(ActionEvent event) {
        FileChooser dialog = new FileChooser();
        ResourceBundle resource = ResourceBundle.getBundle("logResources", new Locale(HeapStatsUtils.getLanguage()));
        dialog.setTitle(resource.getString("dialog.filechooser.title"));
        dialog.setInitialDirectory(new File(HeapStatsUtils.getDefaultDirectory()));
        dialog.getExtensionFilters().addAll(new ExtensionFilter("Log file (*.csv)", "*.csv"),
                new ExtensionFilter("All files", "*.*"));

        List<File> logList = dialog.showOpenMultipleDialog(MainWindowController.getInstance().getOwner());

        if (logList != null) {
            logResourcesController.clearAllItems();
            logDetailsController.clearAllItems();
            
            HeapStatsUtils.setDefaultDirectory(logList.get(0).getParent());
            String logListStr = logList.stream()
                    .map(File::getAbsolutePath)
                    .collect(Collectors.joining("; "));

            logFileList.setText(logListStr);

            TaskAdapter<ParseLogFile> task = new TaskAdapter<>(new ParseLogFile(logList, true));
            task.setOnSucceeded(evt -> onLogFileParserSucceeded(task.getTask()));
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
        /* Get range */
        LocalDateTime start = rangeStart.getValue();
        LocalDateTime end = rangeEnd.getValue();

        List<LogData> targetLogData = logEntries.parallelStream()
                .filter(d -> ((d.getDateTime().compareTo(start) >= 0) && (d.getDateTime().compareTo(end) <= 0)))
                .collect(Collectors.toList());
        List<DiffData> targetDiffData = diffEntries.parallelStream()
                .filter(d -> ((d.getDateTime().compareTo(start) >= 0) && (d.getDateTime().compareTo(end) <= 0)))
                .collect(Collectors.toList());

        Task<Void> task = logResourcesController.createDrawResourceCharts(targetLogData, targetDiffData);
        super.bindTask(task);
        Thread drawChartThread = new Thread(task);
        drawChartThread.start();
        
        archiveList.set(FXCollections.observableArrayList(targetLogData.stream()
                                                                       .filter(d -> d.getArchivePath() != null)
                                                                       .map(new FunctionWrapper<>(ArchiveData::new))
                                                                       .peek(new ConsumerWrapper<>(a -> a.parseArchive()))
                                                                       .collect(Collectors.toList())));
    }

    /**
     * Returns plugin name. This value is used to show in main window tab.
     *
     * @return Plugin name.
     */
    @Override
    public String getPluginName() {
        return "Log Data";
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
        return null;
    }

    @Override
    public Runnable getOnCloseRequest() {
        return null;
    }

    @Override
    public void setData(Object data, boolean select) {
        super.setData(data, select);
        logFileList.setText((String) data);

        TaskAdapter<ParseLogFile> task = new TaskAdapter<>(new ParseLogFile(Arrays.asList(new File((String) data)), true));
        task.setOnSucceeded(evt -> onLogFileParserSucceeded(task.getTask()));
        super.bindTask(task);

        Thread parseThread = new Thread(task);
        parseThread.start();
    }

}

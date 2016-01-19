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

package jp.co.ntt.oss.heapstats.plugin.builtin.jvmlive;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.net.UnknownHostException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;
import java.rmi.ConnectException;
import java.util.Locale;
import java.util.Map;
import java.util.Optional;
import java.util.ResourceBundle;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.stream.Collectors;
import javafx.beans.binding.Bindings;
import javafx.concurrent.ScheduledService;
import javafx.event.ActionEvent;
import javafx.event.Event;
import javafx.event.EventHandler;
import javafx.fxml.FXML;
import javafx.fxml.FXMLLoader;
import javafx.fxml.Initializable;
import javafx.scene.Scene;
import javafx.scene.control.Alert;
import javafx.scene.control.Alert.AlertType;
import javafx.scene.control.Hyperlink;
import javafx.scene.control.Labeled;
import javafx.scene.control.ListCell;
import javafx.scene.control.ListView;
import javafx.scene.control.MenuItem;
import javafx.scene.control.TableCell;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableView;
import javafx.scene.control.TextArea;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.scene.image.Image;
import javafx.scene.input.MouseButton;
import javafx.scene.input.MouseEvent;
import javafx.stage.FileChooser;
import javafx.stage.Modality;
import javafx.stage.Stage;
import javafx.stage.StageStyle;
import javafx.util.Duration;
import jp.co.ntt.oss.heapstats.WindowController;
import jp.co.ntt.oss.heapstats.jmx.JMXHelper;
import jp.co.ntt.oss.heapstats.lambda.ConsumerWrapper;
import jp.co.ntt.oss.heapstats.plugin.PluginController;
import jp.co.ntt.oss.heapstats.plugin.builtin.jvmlive.errorreporter.ErrorReportDecoder;
import jp.co.ntt.oss.heapstats.plugin.builtin.jvmlive.errorreporter.ErrorReportServer;
import jp.co.ntt.oss.heapstats.plugin.builtin.jvmlive.jdp.JdpDecoder;
import jp.co.ntt.oss.heapstats.plugin.builtin.jvmlive.jdp.JdpReceiver;
import jp.co.ntt.oss.heapstats.plugin.builtin.jvmlive.jdp.JdpTableKeyValue;
import jp.co.ntt.oss.heapstats.plugin.builtin.jvmlive.jdp.JdpValidatorService;
import jp.co.ntt.oss.heapstats.plugin.builtin.jvmlive.mbean.HeapStatsMBeanController;
import jp.co.ntt.oss.heapstats.utils.HeapStatsConfigException;
import jp.co.ntt.oss.heapstats.utils.HeapStatsUtils;

/**
 * FXML Controller class for JVMLive
 *
 * @author Yasumasa Suenaga
 */
public class JVMLiveController extends PluginController implements Initializable {
    
    @FXML
    private ListView<JdpDecoder> jvmList;
    
    @FXML
    private TableView<JdpTableKeyValue> detailTable;
    
    @FXML
    private TableColumn<JdpTableKeyValue, String> jdpKey;

    @FXML
    private TableColumn<JdpTableKeyValue, Object> jdpValue;
    
    @FXML
    private ListView<ErrorReportDecoder> crashList;
    
    @FXML
    private MenuItem detailsMenu;
    
    @FXML
    private MenuItem saveMenu;

    private JdpReceiver jdpReceiver;
    
    private Thread receiverThread;
    
    private ScheduledService<Void> jdpValidator;
    
    private ExecutorService taskThreadPool;
    
    private ExecutorService jmxThreadPool;
    
    private ErrorReportServer errorReportServer;
    
    private Thread errorReportThread;
    
    private Optional<String> jconsolePath;

    /**
     * Initializes the controller class.
     */
    @Override
    public void initialize(URL url, ResourceBundle rb) {
        
        try {
            JVMLiveConfig.load();
        } catch (IOException | HeapStatsConfigException ex) {
            HeapStatsUtils.showExceptionDialog(ex);
        }
        
        jdpKey.setCellValueFactory(new PropertyValueFactory<>("key"));
        jdpValue.setCellValueFactory(new PropertyValueFactory<>("value"));
        jdpValue.setCellFactory(c -> new TableCell<JdpTableKeyValue, Object>(){
                                       @Override
                                       protected void updateItem(Object item, boolean empty) {
                                         super.updateItem(item, empty);
                                         Optional.ofNullable(item).ifPresent(i -> {
                                                                                     try{
                                                                                        if(i instanceof Labeled){
                                                                                            setGraphic((Labeled)i);
                                                                                        }
                                                                                        else if(i instanceof JMXHelper){
                                                                                            setGraphic(createHyperlink((JMXHelper)i));
                                                                                        }
                                                                                        else{
                                                                                            setText((String)i);
                                                                                        }
                                                                                     }
                                                                                     catch(Throwable t){
                                                                                         setText("<N/A>");
                                                                                     }
                                                                                  });
                                       }
                                     }
                               );
        
        jvmList.getSelectionModel().selectedItemProperty()
                                   .addListener((v, o, n) -> Optional.ofNullable(n).ifPresent(d -> showJdpDecoderDetail((JdpDecoder)d)));
        
        jvmList.setCellFactory(l -> new ListCell<JdpDecoder>(){
                                      @Override
                                      protected void updateItem(JdpDecoder jdp, boolean b) {
                                        super.updateItem(jdp, b);
                                        Optional.ofNullable(jdp)
                                                .ifPresent(d -> {
                                                                   setText(d.toString());
                                                                   styleProperty().bind(Bindings.createStringBinding(() -> d.invalidateProperty().get() ? "-fx-text-fill: orange;" : "-fx-text-fill: black;", d.invalidateProperty()));
                                                                });
                                      }
                                    }
                              );

        Path jdkBase = Paths.get(Paths.get(System.getProperties().getProperty("java.home")).getParent().toString(), "bin");
        File f = new File(jdkBase.toString(), "jconsole");
        if(f.canExecute()){
            jconsolePath = Optional.of(f.getAbsolutePath());
        }
        else{
            f = new File(jdkBase.toString(), "jconsole.exe");
            if(f.canExecute()){
                jconsolePath = Optional.of(f.getAbsolutePath());
            }
            else{
                jconsolePath = Optional.empty();
            }
        }

                        
        taskThreadPool = Executors.newCachedThreadPool();
        jmxThreadPool = Executors.newCachedThreadPool();
        
        try {
            jdpReceiver = new JdpReceiver(jvmList, taskThreadPool, jconsolePath, jmxThreadPool);
            receiverThread = new Thread(jdpReceiver, "JDP receiver thread");
            receiverThread.start();

            jdpValidator = new JdpValidatorService(jvmList);
            jdpValidator.setPeriod(Duration.seconds(2));
            jdpValidator.start();
        } catch (UnknownHostException ex) {
            HeapStatsUtils.showExceptionDialog(ex);
        }
            
        errorReportServer = new ErrorReportServer(JVMLiveConfig.getErrorReportServerPort(), crashList.getItems(), taskThreadPool);
        errorReportThread = new Thread(errorReportServer, "ErrorReport receiver thread");
        errorReportThread.start();
        
        detailsMenu.disableProperty().bind(crashList.selectionModelProperty().getValue().selectedItemProperty().isNull());
        saveMenu.disableProperty().bind(crashList.selectionModelProperty().getValue().selectedItemProperty().isNull());
    }
    
    private void onHeapStatsLinkClicked(JMXHelper jmxHelper){
        ResourceBundle pluginResource = ResourceBundle.getBundle("jvmliveResources", new Locale(HeapStatsUtils.getLanguage()));
        FXMLLoader loader = new FXMLLoader(JVMLiveController.class.getResource("/jp/co/ntt/oss/heapstats/plugin/builtin/jvmlive/mbean/heapstatsMBean.fxml"), pluginResource);
        HeapStatsMBeanController mbeanController;
        Scene mbeanDialogScene;

        try {
            loader.load();
            mbeanController = (HeapStatsMBeanController)loader.getController();
            mbeanDialogScene = new Scene(loader.getRoot());        
        }
        catch (IOException ex) {
            HeapStatsUtils.showExceptionDialog(ex);
            return;
        }

        mbeanController.setJmxHelper(jmxHelper);
        mbeanController.loadAllConfigs();
        Stage dialog = new Stage(StageStyle.UTILITY);

        try(InputStream icon = WindowController.class.getResourceAsStream("heapstats-icon.png")){
            dialog.getIcons().add(new Image(icon));
        }
        catch(IOException e){
            HeapStatsUtils.showExceptionDialog(e);
        }
        
        dialog.setScene(mbeanDialogScene);
        dialog.initModality(Modality.APPLICATION_MODAL);
        dialog.setResizable(false);
        dialog.setTitle("HeapStats configuration @ " + jmxHelper.getUrl().toString());
        dialog.showAndWait();
    }
    
    private Hyperlink createHyperlink(JMXHelper jmxHelper){
        Hyperlink heapstatsLink = new Hyperlink(jmxHelper.getMbean().getHeapStatsVersion());
        heapstatsLink.setOnAction(e -> onHeapStatsLinkClicked(jmxHelper));
        
        return heapstatsLink;
    }
    
    private void showHsErrDialog(ErrorReportDecoder data){
        String host = data.toString();
        String hsErr;
            
        try(BufferedReader reader = Files.newBufferedReader(data.getHsErrFile().toPath())){
            hsErr = reader.lines().collect(Collectors.joining("\n"));
        } catch (IOException ex) {
            HeapStatsUtils.showExceptionDialog(ex);
            return;
        }

        Alert dialog = new Alert(AlertType.WARNING);
        ResourceBundle resource = ResourceBundle.getBundle("jvmliveResources", new Locale(HeapStatsUtils.getLanguage()));
        
        dialog.setTitle(resource.getString("dialog.crashreport.title"));
        dialog.setHeaderText(resource.getString("dialog.crashreport.header") + host);
        TextArea hsErrArea = new TextArea(hsErr);
        hsErrArea.setEditable(false);
        dialog.getDialogPane().setExpandableContent(hsErrArea);
        dialog.showAndWait();
    }
    
    @FXML
    private void onCrashHistoryClicked(MouseEvent event){
        if(event.getButton().equals(MouseButton.PRIMARY) && (event.getClickCount() == 2)){
            showHsErrDialog(crashList.getSelectionModel().getSelectedItem());
        }
    }
    
    @FXML
    private void onDetailsMenuClicked(ActionEvent event){
        showHsErrDialog(crashList.getSelectionModel().getSelectedItem());
    }
    
    @FXML
    private void onSaveMenuClicked(ActionEvent event){
        FileChooser dialog = new FileChooser();
        ResourceBundle resource = ResourceBundle.getBundle("jvmliveResources", new Locale(HeapStatsUtils.getLanguage()));
        
        dialog.setTitle(resource.getString("dialog.crashreport.save.title"));
        dialog.setInitialDirectory(new File(HeapStatsUtils.getDefaultDirectory()));
        dialog.getExtensionFilters().addAll(new FileChooser.ExtensionFilter("Log file (*.log)", "*.log"),
                                            new FileChooser.ExtensionFilter("All files", "*.*"));
        
        Path hs_err_path = crashList.getSelectionModel().getSelectedItem().getHsErrFile().toPath();
        Optional.ofNullable(dialog.showSaveDialog(WindowController.getInstance().getOwner()))
                .ifPresent(new ConsumerWrapper<>(t -> Files.copy(hs_err_path, t.toPath(), StandardCopyOption.REPLACE_EXISTING)));
    }
    
    private void showJdpDecoderDetail(JdpDecoder data){
        detailTable.setItems(data.jdpTableKeyValueProperty().get());
    }
        

    @Override
    public String getPluginName() {
        return "JVMLive";
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
    public EventHandler<Event> getOnPluginTabSelected() {
        // do nothing
        return null;
    }
    
    private void closeJMXConnection(Object obj){
        if(obj instanceof JMXHelper){
            try{
                ((JMXHelper)obj).close();
            } catch(ConnectException ex){
                // Do nothing
                // Runtime will connect remote host when JMX connection will be closed.
                // So we can skip this operation.
                Logger.getLogger(JVMLiveController.class.getName()).log(Level.SEVERE, null, ex);
            } catch (IOException ex) {
                HeapStatsUtils.showExceptionDialog(ex);
            }
        }
    }

    @Override
    public Runnable getOnCloseRequest() {
        return () -> {
                        Optional.ofNullable(jdpValidator).ifPresent(p -> p.cancel());
                        Optional.ofNullable(jdpReceiver).ifPresent(r -> r.cancel(true));
                        Optional.ofNullable(receiverThread).ifPresent(new ConsumerWrapper<>(t -> t.join()));
                        errorReportServer.cancel(true);
                        try{
                            errorReportThread.join();
                        }
                        catch (InterruptedException ex){
                            Logger.getLogger(JVMLiveController.class.getName()).log(Level.SEVERE, null, ex);
                        }

                        taskThreadPool.shutdownNow();
                        try {
                            taskThreadPool.awaitTermination(JVMLiveConfig.getThreadpoolShutdownAwaitTime(), TimeUnit.SECONDS);
                        } catch (InterruptedException ex) {
                            Logger.getLogger(JVMLiveController.class.getName()).log(Level.SEVERE, null, ex);
                        }

                        jmxThreadPool.shutdownNow();
                        try {
                            jmxThreadPool.awaitTermination(JVMLiveConfig.getThreadpoolShutdownAwaitTime(), TimeUnit.SECONDS);
                        } catch (InterruptedException ex) {
                            Logger.getLogger(JVMLiveController.class.getName()).log(Level.SEVERE, null, ex);
                        }
                        
                        jvmList.getItems().forEach(p -> closeJMXConnection(p.getHeapStatsTableKeyValue().valueProperty().get()));
                     };
    }
    
}

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

import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.InvocationTargetException;
import java.net.URL;
import java.net.URLClassLoader;
import java.nio.file.DirectoryStream;
import java.nio.file.Files;
import java.nio.file.InvalidPathException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.MissingResourceException;
import java.util.ResourceBundle;
import java.util.concurrent.ConcurrentHashMap;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.stream.Collectors;
import java.util.stream.StreamSupport;
import javafx.application.HostServices;
import javafx.application.Platform;
import javafx.event.ActionEvent;
import javafx.fxml.FXML;
import javafx.fxml.FXMLLoader;
import javafx.fxml.Initializable;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.scene.control.ProgressIndicator;
import javafx.scene.control.Tab;
import javafx.scene.control.TabPane;
import javafx.scene.control.TextInputDialog;
import javafx.scene.image.Image;
import javafx.scene.layout.Region;
import javafx.scene.layout.StackPane;
import javafx.stage.Modality;
import javafx.stage.Stage;
import javafx.stage.StageStyle;
import javafx.stage.Window;
import javafx.util.Callback;
import jp.co.ntt.oss.heapstats.lambda.FunctionWrapper;
import jp.co.ntt.oss.heapstats.plugin.PluginController;
import jp.co.ntt.oss.heapstats.plugin.builtin.log.LogController;
import jp.co.ntt.oss.heapstats.plugin.builtin.log.tabs.LogResourcesControllerBase;
import jp.co.ntt.oss.heapstats.plugin.builtin.log.tabs.LogResourcesNumberController;
import jp.co.ntt.oss.heapstats.plugin.builtin.snapshot.SnapShotController;
import jp.co.ntt.oss.heapstats.plugin.builtin.threadrecorder.ThreadRecorderController;
import jp.co.ntt.oss.heapstats.utils.HeapStatsUtils;

/**
 * Main window controller.
 *
 * @author Yasumasa Suenaga
 */
public class WindowController implements Initializable {

    private Map<String, PluginController> pluginList;

    private Region veil;

    private ProgressIndicator progress;

    private ClassLoader pluginClassLoader;

    private Window owner;

    @FXML
    private StackPane stackPane;

    @FXML
    private TabPane tabPane;

    private AboutDialogController aboutDialogController;

    private Scene aboutDialogScene;

    private static WindowController thisController;
    
    private HostServices hostServices;

    @FXML
    private void onExitClick(ActionEvent event) {
        Platform.exit();
    }

    @FXML
    private void onRankLevelClick(ActionEvent event){
        TextInputDialog dialog = new TextInputDialog(Integer.toString(HeapStatsUtils.getRankLevel()));
        
        try(InputStream icon = getClass().getResourceAsStream("heapstats-icon.png")){
            Stage dialogStage = (Stage)dialog.getDialogPane().getScene().getWindow();
            dialogStage.getIcons().add(new Image(icon));
        }
        catch(IOException e){
            HeapStatsUtils.showExceptionDialog(e);
        }
        
        dialog.setTitle("Rank Level setting");
        dialog.setHeaderText("Rank Level setting");
        ResourceBundle resource = ResourceBundle.getBundle("HeapStatsResources", new Locale(HeapStatsUtils.getLanguage()));
        dialog.setContentText(resource.getString("rank.label"));
        dialog.showAndWait()
              .ifPresent(v -> HeapStatsUtils.setRankLevel(Integer.parseInt(v)));
    }
    
    @FXML
    private void onHowToClick(ActionEvent event) {
        hostServices.showDocument("http://icedtea.classpath.org/wiki/HeapStats/Analyzer-version2");
    }

    @FXML
    private void onAboutMenuClick(ActionEvent event){
        Stage dialog = new Stage(StageStyle.UTILITY);
        aboutDialogController.setStage(dialog);

        dialog.setScene(aboutDialogScene);
        dialog.initModality(Modality.APPLICATION_MODAL);
        dialog.setResizable(false);
        dialog.setTitle("About HeapStats Analyzer");
        dialog.showAndWait();
    }
    
    private void loadAndRegisterFXML(FXMLLoader loader){
        Parent root;

        try {
            root = loader.load();
        }
        catch (IOException ex) {
            HeapStatsUtils.showExceptionDialog(ex);
            return;
        }

        PluginController controller = (PluginController)loader.getController();
        controller.setVeil(veil);
        controller.setProgress(progress);

        Tab tab = new Tab();
        tab.setText(controller.getPluginName());
        tab.setContent(root);
        tab.setOnSelectionChanged(controller.getOnPluginTabSelected());

        tabPane.getTabs().add(tab);

        pluginList.put(controller.getPluginName(), controller);
    }
    
    private void loadLogPlugin(){
        String fxmlName = LogController.class.getPackage().getName().replace('.', '/') + "/log.fxml";
        ResourceBundle pluginResource = ResourceBundle.getBundle("logResources", new Locale(HeapStatsUtils.getLanguage()), pluginClassLoader);
        FXMLLoader loader = new FXMLLoader(pluginClassLoader.getResource(fxmlName), pluginResource);
        
        Callback<Class<?>, Object> origFactory = loader.getControllerFactory();
        loader.setControllerFactory(c -> {
                                            System.out.println(c);
                                            if(c.equals(LogResourcesControllerBase.class)){
                                                return HeapStatsUtils.isNumberAxis() ? new LogResourcesNumberController() : null; // TODO: implement CategoryAxis
                                            }
                                            else{
                                                System.out.println("unmatch");
                                                try{
                                                    return c.getConstructor().newInstance();
                                                }
                                                catch(NoSuchMethodException | InstantiationException | IllegalAccessException | InvocationTargetException e){
                                                    throw new RuntimeException(e);
                                                }
                                            }
                                         });
        
        loadAndRegisterFXML(loader);
    }

    private void addPlugin(String packageName){
        String lastPackageName = packageName.substring(packageName.lastIndexOf('.') + 1);
        packageName = packageName.replace('.', '/');
        String fxmlName = packageName + "/" + lastPackageName + ".fxml";
        FXMLLoader loader;

        try{
            ResourceBundle pluginResource = ResourceBundle.getBundle(lastPackageName + "Resources", new Locale(HeapStatsUtils.getLanguage()), pluginClassLoader);
            loader = new FXMLLoader(pluginClassLoader.getResource(fxmlName), pluginResource);
        }
        catch(MissingResourceException e){
            loader = new FXMLLoader(pluginClassLoader.getResource(fxmlName));
        }

        loadAndRegisterFXML(loader);
    }

    public static WindowController getInstance(){
        return thisController;
    }

    @FXML
    private void onGCAllClick(ActionEvent event) {
        SnapShotController snapShotController = (SnapShotController)getPluginController("SnapShot Data");
        snapShotController.dumpGCStatisticsToCSV(false);
    }

    @FXML
    private void onGCSelectedClick(ActionEvent event) {
        SnapShotController snapShotController = (SnapShotController)getPluginController("SnapShot Data");
        snapShotController.dumpGCStatisticsToCSV(true);
    }

    @FXML
    private void onHeapAllClick(ActionEvent event) {
        SnapShotController snapShotController = (SnapShotController)getPluginController("SnapShot Data");
        snapShotController.dumpClassHistogramToCSV(false);
    }

    @FXML
    private void onHeapSelectedClick(ActionEvent event) {
        SnapShotController snapShotController = (SnapShotController)getPluginController("SnapShot Data");
        snapShotController.dumpClassHistogramToCSV(true);
    }

    @FXML
    private void onSnapShotOpenClick(ActionEvent event) {
        Tab snapShotTab = tabPane.getTabs().stream()
                                 .filter(t -> t.getText().equals("SnapShot Data"))
                                 .findAny()
                                 .orElseThrow(() -> new IllegalStateException("SnapShot plugin must be loaded."));
        tabPane.getSelectionModel().select(snapShotTab);
        SnapShotController snapShotController = (SnapShotController)getPluginController("SnapShot Data");

        snapShotController.onSnapshotFileClick(event);
    }

    @FXML
    private void onLogOpenClick(ActionEvent event) {
        Tab snapShotTab = tabPane.getTabs().stream()
                                 .filter(t -> t.getText().equals("Log Data"))
                                 .findAny()
                                 .orElseThrow(() -> new IllegalStateException("Log plugin must be loaded."));
        tabPane.getSelectionModel().select(snapShotTab);
        LogController logController = (LogController)getPluginController("Log Data");

        logController.onLogFileClick(event);
    }

    @FXML
    private void onThreadRecorderOpenClick(ActionEvent event) {
        Tab threadRecorderTab = tabPane.getTabs().stream()
                                       .filter(t -> t.getText().equals("Thread Recorder"))
                                       .findAny()
                                       .orElseThrow(() -> new IllegalStateException("Thread Recorder plugin must be loaded."));
        tabPane.getSelectionModel().select(threadRecorderTab);
        ThreadRecorderController threadRecorderController = (ThreadRecorderController)getPluginController("Thread Recorder");

        threadRecorderController.onOpenBtnClick(event);
    }

    private void initializeAboutDialog(){
        FXMLLoader loader = new FXMLLoader(getClass().getResource("/jp/co/ntt/oss/heapstats/aboutDialog.fxml"), HeapStatsUtils.getResourceBundle());

        try {
            loader.load();
            aboutDialogController = (AboutDialogController)loader.getController();
            aboutDialogScene = new Scene(loader.getRoot());
        }
        catch (IOException ex) {
            HeapStatsUtils.showExceptionDialog(ex);
        }

    }

    @Override
    public void initialize(URL url, ResourceBundle rb) {
        thisController = this;
        pluginList = new ConcurrentHashMap<>();
        veil = new Region();
        veil.setStyle("-fx-background-color: rgba(0, 0, 0, 0.2)");
        veil.setVisible(false);

        progress = new ProgressIndicator();
        progress.setMaxSize(200.0d, 200.0d);
        progress.setVisible(false);

        stackPane.getChildren().add(veil);
        stackPane.getChildren().add(progress);

        initializeAboutDialog();
    }
    
    /**
     * Set HostServices to open HowTo page.
     * @param hostServices HostServices
     */
    protected void setHostServices(HostServices hostServices) {
        this.hostServices = hostServices;
    }

    /**
     * Load plugins which is defined in heapstats.properties.
     */
    public void loadPlugin(){
        String resourceName = "/" + this.getClass().getName().replace('.', '/') + ".class";
        String appJarString = this.getClass().getResource(resourceName).getPath();
        appJarString = appJarString.substring(0, appJarString.indexOf('!')).replaceFirst("file:", "");

        Path appJarPath;

        try{
            appJarPath = Paths.get(appJarString);
        }
        catch(InvalidPathException e){
            if((appJarString.charAt(0) == '/') && (appJarString.length() > 2)){ // for Windows
                appJarPath = Paths.get(appJarString.substring(1));
            }
            else{
                throw e;
            }
        }

        Path libPath = appJarPath.getParent().resolve("lib");
        URL[] jarURLList = null;

        try(DirectoryStream<Path> jarPaths = Files.newDirectoryStream(libPath, "*.jar")){
            jarURLList = StreamSupport.stream(jarPaths.spliterator(), false)
                                      .map(new FunctionWrapper<>(p -> p.toUri().toURL()))
                                      .filter(u -> !u.getFile().endsWith("heapstats-core.jar"))
                                      .filter(u -> !u.getFile().endsWith("heapstats-mbean.jar"))
                                      .filter(u -> !u.getFile().endsWith("jgraphx.jar"))
                                      .collect(Collectors.toList())
                                      .toArray(new URL[0]);
        }
        catch(IOException ex) {
            Logger.getLogger(WindowController.class.getName()).log(Level.SEVERE, null, ex);
        }

        pluginClassLoader = (jarURLList == null) ? WindowController.class.getClassLoader() : new URLClassLoader(jarURLList);
        FXMLLoader.setDefaultClassLoader(pluginClassLoader);
        
        /* Load base plugins */
        loadLogPlugin();

        List<String> plugins = HeapStatsUtils.getPlugins();
        plugins.stream().forEach(s -> addPlugin(s));

        aboutDialogController.setPluginInfo();
    }

    /**
     * Get controller instance of plugin.
     *
     * @param pluginName Plugin name which you want.
     * @return Controller of Plugin. If it does not exist, return null.
     */
    public PluginController getPluginController(String pluginName){
        return pluginList.get(pluginName);
    }

    /**
     * Get loaded plugin list.
     *
     * @return Loaded plugin list.
     */
    public Map<String, PluginController> getPluginList() {
        return pluginList;
    }

    /**
     * Select plugin tab
     *
     * @param pluginName Name of plugin to active.
     */
    public void selectTab(String pluginName) throws IllegalArgumentException{
        Tab target = tabPane.getTabs().stream()
                            .filter(t -> t.getText().equals(pluginName))
                            .findAny()
                            .orElseThrow(() -> new IllegalArgumentException(pluginName + " is not loaded."));
        tabPane.getSelectionModel().select(target);
    }

    public Window getOwner() {
        return owner;
    }

    public void setOwner(Window owner) {
        this.owner = owner;
    }

}

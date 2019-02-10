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
package jp.co.ntt.oss.heapstats.fx;

import java.io.InputStream;
import java.util.Optional;
import javafx.application.Application;
import javafx.application.Platform;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.scene.image.Image;
import javafx.stage.Stage;
import jp.co.ntt.oss.heapstats.api.utils.HeapStatsConfigException;
import jp.co.ntt.oss.heapstats.api.utils.HeapStatsUtils;

/**
 * Main class of HeapStats FX Analyzer.
 * This class provides entry point of HeapStats FX Analyzer.
 *
 * @author Yasumasa Suenaga
 */
public class HeapStatsFXAnalyzer extends Application {
    
    private MainWindowController mainWindowController;

    @Override
    public void start(Stage stage) throws Exception {
        stage.setTitle("HeapStats Analyzer");

        try (InputStream icon = getClass().getResourceAsStream("heapstats-icon.png")) {
            stage.getIcons().add(new Image(icon));
        }

        Thread.setDefaultUncaughtExceptionHandler((t, e) -> Platform.runLater(() -> HeapStatsUtils.showExceptionDialog(e)));
        try {
            HeapStatsUtils.load();
        } catch (HeapStatsConfigException e) {
            HeapStatsUtils.showExceptionDialog(e);
            Runtime.getRuntime().exit(-1);
        }
        FXMLLoader mainWindowLoader = new FXMLLoader(getClass().getResource("window.fxml"), HeapStatsUtils.getResourceBundle());

        Parent root = mainWindowLoader.load();
        Scene scene = new Scene(root);
        HeapStatsUtils.setWindowController(mainWindowLoader.getController());
        mainWindowController = (MainWindowController) HeapStatsUtils.getWindowController();
        mainWindowController.setOwner(stage);
        mainWindowController.setHostServices(getHostServices());
        mainWindowController.loadPlugin();

        stage.setScene(scene);
        stage.show();
    }

    @Override
    public void stop() throws Exception {
        mainWindowController.getPluginList().values().forEach(c -> Optional.ofNullable(c.getOnCloseRequest()).ifPresent(r -> r.run()));
    }

    /**
     * Main method of HeapStats analyzer.
     *
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        launch(args);
    }

}

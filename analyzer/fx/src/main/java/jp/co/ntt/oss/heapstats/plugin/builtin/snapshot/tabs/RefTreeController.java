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
package jp.co.ntt.oss.heapstats.plugin.builtin.snapshot.tabs;

import javafx.scene.layout.VBox;
import jp.co.ntt.oss.heapstats.snapshot.ReferenceTracker;
import com.mxgraph.layout.hierarchical.mxHierarchicalLayout;
import com.mxgraph.model.mxCell;
import com.mxgraph.model.mxGeometry;
import com.mxgraph.model.mxICell;
import com.mxgraph.swing.mxGraphComponent;
import com.mxgraph.swing.util.mxGraphTransferable;
import java.awt.Point;
import java.awt.datatransfer.DataFlavor;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.OptionalInt;
import java.util.ResourceBundle;
import javafx.application.Platform;
import javafx.beans.binding.Bindings;
import javafx.beans.property.LongProperty;
import javafx.beans.property.ObjectProperty;
import javafx.beans.property.SimpleLongProperty;
import javafx.beans.property.SimpleObjectProperty;
import javafx.embed.swing.SwingNode;
import javafx.event.ActionEvent;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.control.Alert;
import javafx.scene.control.Alert.AlertType;
import javafx.scene.control.ButtonType;
import javafx.scene.control.CheckBox;
import javafx.scene.control.Label;
import javafx.scene.control.RadioButton;
import javafx.scene.layout.AnchorPane;
import javax.swing.SwingConstants;
import javax.swing.SwingUtilities;
import jp.co.ntt.oss.heapstats.container.snapshot.ObjectData;
import jp.co.ntt.oss.heapstats.container.snapshot.SnapShotHeader;
import jp.co.ntt.oss.heapstats.plugin.builtin.snapshot.SnapShotHeaderConverter;
import jp.co.ntt.oss.heapstats.utils.HeapStatsUtils;

/**
 * FXML Controller class of Reference Data tab.
 *
 * @author Yasumasa Suenaga
 */
public class RefTreeController implements Initializable, MouseListener {

    /**
     * Reference to the region for displaying the object.
     */
    private ReferenceGraph graph;

    /**
     * Reference to the enclosing graph component.
     */
    private mxGraphComponent graphComponent;

    @FXML
    private VBox topVBox;

    @FXML
    private Label snapshotLabel;

    @FXML
    private RadioButton radioParent;

    @FXML
    private RadioButton radioSize;

    @FXML
    private CheckBox rankCheckBox;

    @FXML
    private SwingNode graphNode;

    private ObjectProperty<SnapShotHeader> currentSnapShotHeader;

    private LongProperty currentObjectTag;

    private ResourceBundle resource;

    private void buildTab() {
        if ((currentSnapShotHeader.get() != null)
                && currentSnapShotHeader.get().hasReferenceData()
                && (currentObjectTag.get() != -1)){
            SwingUtilities.invokeLater(() -> initializeSwingNode());
        }
        else{
            /*
             * Initialize SwingNode with dummy mxGraphComponent.
             * SwingNode seems to hook focus ungrab event.
             * If SwingNode is empty, IllegalArgumentException with "null source" is
             * thrown when another stage (e.g. about dialog) is shown.
             * To avoid this, dummy mxGraphComponent is set to SwingNode at first.
             */
            SwingUtilities.invokeLater(() -> graphNode.setContent(new mxGraphComponent(new ReferenceGraph())));
        }
    }

    /**
     * Initialize method for SwingNode. This method is called by Swing Event
     * Dispatcher Thread.
     */
    private void initializeSwingNode() {
        graph = new ReferenceGraph();
        graph.setCellsEditable(false);

        graph.getModel().beginUpdate();
        {
            ObjectData data = currentSnapShotHeader.get().getSnapShot(HeapStatsUtils.getReplaceClassName()).get(currentObjectTag.get());
            if (data == null) {
                throw new IllegalStateException(resource.getString("reftree.message.notselect"));
            }
            ReferenceCell cell = new ReferenceCell(data, true, false);
            graph.addCell(cell);
        }
        graph.getModel().endUpdate();

        graph.setEdgeStyle();

        graphComponent = new mxGraphComponent(graph);
        graphComponent.getGraphControl().addMouseListener(this);
        graphComponent.setToolTips(true);

        graphNode.setContent(graphComponent);

        /*
         * FIXME!
         * SwingNode is not shown immediately.
         * So I call repaint() method at last and request layout() call.
         */
        graphComponent.repaint();
        Platform.runLater(() -> topVBox.layout());
    }

    /**
     * Initializes the controller class.
     */
    @Override
    public void initialize(URL url, ResourceBundle rb) {
        resource = rb;
        ReferenceCell.initialize();
        currentSnapShotHeader = new SimpleObjectProperty<>();
        currentObjectTag = new SimpleLongProperty();
        currentObjectTag.addListener((v, o, n) -> Optional.ofNullable(n).ifPresent(t -> buildTab()));
        snapshotLabel.textProperty().bind(Bindings.createStringBinding(() -> (new SnapShotHeaderConverter()).toString(currentSnapShotHeader.get()), currentSnapShotHeader));

        /*
         * Initialize SwingNode with dummy mxGraphComponent.
         * SwingNode seems to hook focus ungrab event.
         * If SwingNode is empty, IllegalArgumentException with "null source" is
         * thrown when another stage (e.g. about dialog) is shown.
         * To avoid this, dummy mxGraphComponent is set to SwingNode at first.
         */
        SwingUtilities.invokeLater(() -> {
            mxGraphComponent dummyGraphComponent = new mxGraphComponent(new ReferenceGraph());
            graphNode.setContent(dummyGraphComponent);
        });

    }

    /**
     * Add reference cells to graph. This method creates cell which represents
     * child, and connects to it from parent.
     *
     * @param parentCell Parent cell
     * @param child Child data.
     */
    private void addReferenceCell(ReferenceCell parentCell, ObjectData objData) {
        ReferenceCell cell = null;
        mxCell tmp = (mxCell) parentCell.getParent();

        for (int i = 0; i < tmp.getChildCount(); i++) {
            mxICell target = tmp.getChildAt(i);

            if (target.isVertex() && (((ReferenceCell) target).getTag() == objData.getTag())) {
                cell = (ReferenceCell) target;
            }

        }

        ReferenceCell edge = new ReferenceCell(objData, false, true);

        if (cell == null) {
            cell = new ReferenceCell(objData, false, false);
            graph.addCell(cell);
        }

        long size = objData.getTotalSize() / 1024; // KB

        edge.setValue((size == 0) ? objData.getCount() + "\n< 1"
                : objData.getCount() + "\n" + objData.getTotalSize() / 1024); // KB

        graph.addEdge(edge, graph.getDefaultParent(), parentCell, cell, 1);
    }

    /**
     * Displayed on the graph to get a child or of a cell object you clicked,
     * the information of the parent.
     *
     * @param parentCell Cell object you clicked
     */
    private void drawMind(ReferenceCell parentCell) {

        if ((parentCell.getEdgeCount() > 1)
                || (parentCell.isRoot() && (parentCell.getEdgeCount() > 0))
                || (parentCell.isEdge())) {
            return;
        }

        OptionalInt rankLevel = rankCheckBox.isSelected() ? OptionalInt.of(HeapStatsUtils.getRankLevel())
                : OptionalInt.empty();
        ReferenceTracker refTracker = new ReferenceTracker(currentSnapShotHeader.get().getSnapShot(HeapStatsUtils.getReplaceClassName()), rankLevel, Optional.empty());
        List<ObjectData> objectList = radioParent.isSelected() ? refTracker.getParents(parentCell.getTag(), radioSize.isSelected())
                : refTracker.getChildren(parentCell.getTag(), radioSize.isSelected());

        if (objectList.isEmpty()) {
            return;
        }

        graph.getModel().beginUpdate();
        {
            objectList.forEach(o -> addReferenceCell(parentCell, o));

            if (mxGraphTransferable.dataFlavor == null) {
                try {
                    mxGraphTransferable.dataFlavor = new DataFlavor(
                            DataFlavor.javaJVMLocalObjectMimeType + "; class=com.mxgraph.swing.util.mxGraphTransferable",
                            null, new mxGraphTransferable(null, null).getClass().getClassLoader());
                } catch (ClassNotFoundException e) {
                    // do nothing
                }
            }

            mxHierarchicalLayout layout = new mxHierarchicalLayout(graph, SwingConstants.WEST);
            layout.execute(graph.getDefaultParent());
        }
        graph.getModel().endUpdate();
    }

    @Override
    public void mouseClicked(MouseEvent e) {
        Object cell = graphComponent.getCellAt(e.getX(), e.getY());

        if ((e.getButton() == MouseEvent.BUTTON1) && (cell != null) && (cell instanceof ReferenceCell)) {
            drawMind((ReferenceCell) cell);
        }

        graphComponent.repaint();
        Platform.runLater(() -> topVBox.layout());
    }

    @Override
    public void mousePressed(MouseEvent e) {
        /* Nothing to do */
    }

    @Override
    public void mouseReleased(MouseEvent e) {
        /* Nothing to do */
    }

    @Override
    public void mouseEntered(MouseEvent e) {
        /* Nothing to do */
    }

    @Override
    public void mouseExited(MouseEvent e) {
        /* Nothing to do */
    }

    /**
     * Event handler of OK button.
     *
     * @param event ActionEvent of this event.
     */
    @FXML
    private void onOkClick(ActionEvent event) {

        if ((currentSnapShotHeader.get() == null)
                || (currentObjectTag.get() == -1)){
            return;
        } else if (!currentSnapShotHeader.get().hasReferenceData()) {
            // Version 1.0.x or "collect_reftree=false" do not have reftree data
            Alert dialog = new Alert(AlertType.ERROR, resource.getString("reftree.nodata"), ButtonType.OK);
            dialog.show();
            return;
        }

        mxCell cell = (mxCell) graph.getDefaultParent();
        List<Object> removeCells = new ArrayList<>();

        for (int i = 0; i < cell.getChildCount(); i++) {
            mxCell removeCell = (mxCell) cell.getChildAt(i);

            if (removeCell.isEdge() || !((ReferenceCell) removeCell).isRoot()) {
                removeCells.add(removeCell);
            } else {
                graph.removeSelectionCell(removeCell);
                mxGeometry geometry = ((ReferenceCell) removeCell).getGeometry();
                geometry.setX(0);
                geometry.setY(0);
                ((ReferenceCell) removeCell).setGeometry(geometry);
            }

        }

        graph.removeCells(removeCells.toArray());
        graph.refresh();
        graphComponent.refresh();
        graphComponent.getViewport().setViewPosition(new Point(0, 0));
    }

    /**
     * Get property of current SnapShotHeader.
     *
     * @return Property of currentSnapShotHeader.
     */
    public ObjectProperty<SnapShotHeader> currentSnapShotHeaderProperty() {
        return currentSnapShotHeader;
    }

    /**
     * Get property of current Object tag.
     *
     * @return Property of currentObjectTag.
     */
    public LongProperty currentObjectTagProperty() {
        return currentObjectTag;
    }

}

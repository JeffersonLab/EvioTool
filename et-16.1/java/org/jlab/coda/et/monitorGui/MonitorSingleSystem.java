/*----------------------------------------------------------------------------*
 *  Copyright (c) 2002        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, MS-12H        *
 *             Phone: (757) 269-5130             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*/

package org.jlab.coda.et.monitorGui;

import java.lang.*;
import java.io.*;
import java.awt.*;
import java.awt.geom.*;
import java.util.*;
import javax.swing.*;
import javax.swing.tree.*;

import org.jlab.coda.et.*;
import org.jlab.coda.et.data.*;
import org.jlab.coda.et.exception.*;
import com.loox.jloox.*;
import com.loox.jloox.layout.*;

/**
 * This class implements a view of a single ET system.
 *
 * @author Carl Timmer
 */

public class MonitorSingleSystem {
    // general stuff
    private int        updatePeriod; // in seconds
    private int        dividerPosition, graphHeight, orientation;
    private double     attWidth, stationWidth, stationGap;
    private String     key;
    private EtSystem   sys;
    private AllData    data;
    private boolean    initialized, updated;
    private boolean    isSolaris, isJava, isLinux;
    private boolean    debug = false;

    // rate stuff
    private long  rate, prevGcOut, time1, time2, updateTime;
    private final JSplitPane splitPane;
    private JScrollPane graphPane;

    // tree stuff
    private final JTree tree;
    private final DefaultTreeModel treeModel;

    // JLoox widgets
    private final LxView  view;
    private final LxGraph graph;
    private final LxAlignLayout layout;
    private final LxLink mainLink1, mainLink2;

    // nodes
    private final DefaultMutableTreeNode topNode;
    private final DefaultMutableTreeNode systemNode;
    private final DefaultMutableTreeNode systemStaticNode;
    private final DefaultMutableTreeNode systemDynamicNode;
    private final DefaultMutableTreeNode stationNode;
    private final DefaultMutableTreeNode attachNode;
    private final DefaultMutableTreeNode processNode;

    // object storage
    private final HashMap    stations;
    private final HashMap    attachments;
    // private final HashMap    stationIcons;
    private final HashMap    attachIcons;
    private final LinkedList stationManagers;
    private final LinkedList parallelManagers;

    // Default colors
    public final Color stationColorDefault        = Color.cyan;
    public final Color stationIdleColorDefault    = Color.pink;
    public final Color attachColorDefault         = new Color(255, 200 , 255);
    public final Color eventColorDefault          = Color.red;
    public final Color lineColorDefault           = Color.black;
    public final Color textColorDefault           = Color.black;

    // Colors used
    private Color stationColor        = stationColorDefault;
    private Color stationIdleColor    = stationIdleColorDefault;
    private Color attachColor         = attachColorDefault;
    private Color eventColor          = eventColorDefault;
    private Color lineColor           = lineColorDefault;
    private Color textColor           = textColorDefault;


    // Define constants
    public static final boolean HORIZONTAL = true;
    public static final boolean VERTICAL   = false;

    // class to hold flow data needed for graphical station layout
    class StationFlowData {
        int flowMode;
        boolean isHead;
        LxAlignLayout parentLayout;
    }

    // class to hold link data needed for graphical station drawing
    class StationLinkData {
        LxLink link1;
        LxLink link2;
        public StationLinkData(LxLink l1) {
            link1 = l1;
        }
        public StationLinkData(LxLink l1, LxLink l2) {
            link1 = l1;
            link2 = l2;
        }
    }

    // Constructor
    public MonitorSingleSystem(EtSystem use, String key, JTabbedPane tabbedPane,
                               int period, int divider,
                               int orient, Color[] colors) {
        sys  = use;
        data = new AllData();
        updatePeriod = period;
        dividerPosition = divider;

        if (colors != null) {
            if (colors[0] != null) stationColor        = colors[0];
            if (colors[1] != null) stationIdleColor    = colors[1];
            if (colors[2] != null) attachColor         = colors[2];
            if (colors[3] != null) eventColor          = colors[3];
            if (colors[4] != null) lineColor           = colors[4];
            if (colors[5] != null) textColor           = colors[5];

        }

        // Create static tree nodes.
        topNode           = new DefaultMutableTreeNode(key);
        systemNode        = new DefaultMutableTreeNode("System");
        systemStaticNode  = new DefaultMutableTreeNode("Static Info");
        systemDynamicNode = new DefaultMutableTreeNode("Dynamic Info");
        stationNode       = new DefaultMutableTreeNode("Stations");
        attachNode        = new DefaultMutableTreeNode("Attachments");
        processNode       = new DefaultMutableTreeNode("Proceses");
        topNode.add(systemNode);
        systemNode.add(systemStaticNode);
        systemNode.add(systemDynamicNode);
        topNode.add(stationNode);
        topNode.add(attachNode);

        // Create a tree that allows one selection at a time.
        treeModel = new DefaultTreeModel(topNode);
        tree      = new JTree(treeModel);

        tree.setDoubleBuffered(true);
        tree.setLargeModel(true);
        tree.getSelectionModel().setSelectionMode
                (TreeSelectionModel.SINGLE_TREE_SELECTION);
        tree.putClientProperty("JTree.lineStyle", "Angled");
        // Get rid of tree's leaf icon.
        DefaultTreeCellRenderer renderer = new DefaultTreeCellRenderer();
        renderer.setLeafIcon(null);
        tree.setCellRenderer(renderer);

        // Create the scroll pane and add the tree to it.
        JScrollPane treePane = new JScrollPane(tree);

        // Graphics panel
        graph = new LxGraph();
        // Make graph visible
        view = (LxView)graph.addView();
        // Allow user to interact with the objects in EDITOR_MODE
        view.setEditMode(LxView.STANDARD_MODE);
        if (debug) {
            view.setEditMode(LxView.EDITOR_MODE);
        }
        // Allow smoother graphics
        view.setAntialiasingActivated(true);
        view.setDoubleBuffered(true);
        // Scrollbars appear when smaller than preferred size.
        // The preferred width must be reset according to the
        // number & size of station graphics.
        graphHeight = 304;
        view.setPreferredSize(new Dimension(500, graphHeight));

        // Create scroll pane and add graph to it.
        graphPane = new JScrollPane();
        graphPane.setViewportView(view);

        // Add the split pane to this frame.
        orientation = orient;
        splitPane = new JSplitPane(orient);
        splitPane.setLeftComponent(treePane);
        splitPane.setRightComponent(graphPane);
        // Ignored in some releases of Swing. bug 4101306
        splitPane.setDividerLocation(dividerPosition);
        // Workaround for bug 4101306:
        // treePane.setPreferredSize(new Dimension(100, 100));
        splitPane.setPreferredSize(new Dimension(1100, 700));

        // Add to main window's tabbed pane
        tabbedPane.addTab(key, null, splitPane, "monitored ET system");

        // JLoox layout of graphics
        layout = new LxAlignLayout(new Rectangle2D.Double(0., 0., 600., (double)graphHeight));
        layout.setOrder(LxAlignLayout.LIST_ORDER);
        layout.setOrientation(LxAlignLayout.HORIZONTAL);
        layout.setAlignAxisMethod(LxAlignLayout.EVEN_USE_SIZE_METHOD);
        layout.setNormalAxisMethod(LxAlignLayout.LEFT_TOP_METHOD);
        layout.setVisible(true);
        if (debug) {
            graph.add(layout);
        }

        // Store objects for future use (more efficient
        // than always recreating objects)
        int size1 = EtConstants.defaultStationsMax,
            size2 = EtConstants.defaultAttsMax;
        try {
            size1 = sys.getStationsMax();
            size2 = sys.getAttachmentsMax();
        }
        catch (IOException ex) {
        }
        catch (EtClosedException ex) {
        }

        float loadfactor = 0.75F;
        stations         = new HashMap(size1, loadfactor);
        attachments      = new HashMap(size2, loadfactor);
        //stationIcons     = new HashMap(size1, loadfactor);
        attachIcons      = new HashMap(size2, loadfactor);
        stationManagers  = new LinkedList();
        parallelManagers = new LinkedList();
        mainLink1        = new LxLink();
        mainLink2        = new LxLink();
        mainLink1.setLineArrow(LxArrowElement.ARROW_END);
        mainLink1.setLineThickness(2.f);
        mainLink1.setLineColor(lineColor);
        mainLink2.setLineArrow(LxArrowElement.ARROW_MIDDLE);
        mainLink2.setLineThickness(2.f);
        mainLink2.setLineColor(lineColor);
        mainLink1.setName("link1");
        mainLink2.setName("link2");
    }


    // Setters, getters, etc.
    public void setOrientation(int orient) {
        if (orient == orientation) {
            return;
        }
        splitPane.setOrientation(orient);

        if (orient == JSplitPane.HORIZONTAL_SPLIT) {
            splitPane.setDividerLocation(dividerPosition);
            orientation = JSplitPane.HORIZONTAL_SPLIT;
        }
        else {
            // Give bottom window just enough room to view graph
            // unless the window is too small. In that case split it.
            int height = splitPane.getHeight();
            height = (height < 2*(graphHeight+50)) ? height/2 : height-(graphHeight + 50);
            splitPane.setDividerLocation(height);
            orientation = JSplitPane.VERTICAL_SPLIT;
        }
        splitPane.updateUI();
    }

    public int getOrientation() {return orientation;}

    public boolean timeToUpdate() {
        long currentTime = System.currentTimeMillis();
        if ((currentTime - updateTime) > 1000*updatePeriod) {
            return true;
        }
        return false;
    }

    public void setUpdatePeriod(int period) {
        updatePeriod = (period < 1) ? 1 : period;
    }

    public int  getUpdatePeriod()     {return updatePeriod;}
    public int  getDividerPosition()  {return splitPane.getDividerLocation();}
    public Component getDisplayPane() {return splitPane;}
    public void treeDidChange()       {tree.treeDidChange();}
    public void updateUI()            {tree.updateUI();}
    public boolean isInitialized()    {return initialized;}
    public boolean isUpdated()        {return updated;}
    public DefaultMutableTreeNode getNode() {return topNode;}
    public void getData() throws EtException, IOException, EtClosedException {
        data = sys.getData();
    }
    public void   close()  {sys.close();}
    public String getKey() {return key;}

    public Color getEventColor()          {return new Color(eventColor.getRGB());}
    public Color getStationColor()        {return new Color(stationColor.getRGB());}
    public Color getStationIdleColor()    {return new Color(stationIdleColor.getRGB());}
    public Color getAttachmentColor()     {return new Color(attachColor.getRGB());}
    public Color getLineColor()           {return new Color(lineColor.getRGB());}
    public Color getTextColor()           {return new Color(textColor.getRGB());}


    public void staticDisplay()
    {
        int           end = 499;
        StringBuffer  str = new StringBuffer(500);

        // Display static system data
        str.append("Host = ");
        str.append(sys.getHost());
        int lang = sys.getLanguage();
        if (lang != EtConstants.langJava) {
            str.append(",  language = C,  pid = ");
            str.append(data.sysData.getMainPid());
            // processes exist on Solaris only
            if (data.sysData.getShare() == EtConstants.mutexShare) {
                isSolaris = true;
                topNode.add(processNode);
            }
            else {
                isLinux = true;
            }
        }
        else {
            isJava = true;
            str.append(",  language = Java");
        }
        if (data.sysData.isBit64()) {
            str.append(", bits = 64");
        }
        else {
            str.append(", bits = 32");

        }
        systemStaticNode.add(new DefaultMutableTreeNode(str.toString()));

        str.delete(0, end);
        str.append("Ports: tcp = ");
        str.append(data.sysData.getTcpPort());
        str.append(", udp = ");
        str.append(data.sysData.getUdpPort());
        systemStaticNode.add(new DefaultMutableTreeNode(str.toString()));

        str.delete(0, end);
        str.append("Events: total = ");
        str.append(data.sysData.getEvents());
        str.append(", size = ");
        str.append(data.sysData.getEventSize());
        str.append(" bytes, temps = ");
        str.append(data.sysData.getTempsMax());
        systemStaticNode.add(new DefaultMutableTreeNode(str.toString()));

        str.delete(0, end);
        str.append("Max #: stations = ");
        str.append(data.sysData.getStationsMax());
        str.append(", attachments = ");
        str.append(data.sysData.getAttachmentsMax());
        if (isSolaris) {
            str.append(", processes = ");
            str.append(data.sysData.getProcessesMax());
        }
        systemStaticNode.add(new DefaultMutableTreeNode(str.toString()));

        str.delete(0, end);
        if (data.sysData.getInterfaces() > 0) {
            str.append("Network interfaces: ");
            int limit = data.sysData.getInterfaces();
            for (int i=0; i < limit; i++) {
                str.append(data.sysData.getInterfaceAddresses()[i]);
                if (i == limit-1) break;
                str.append(", ");
            }
        }
        else {
            str.append("network interfaces(0): none");
        }
        systemStaticNode.add(new DefaultMutableTreeNode(str.toString()));

        if (data.sysData.getMulticasts() > 0) {
            str.delete(0, end);
            str.append("Multicast addresses: ");
            int limit = data.sysData.getMulticasts();
            for (int i=0; i < limit; i++) {
                str.append(data.sysData.getMulticastAddresses()[i]);
                if (i == limit-1) break;
                str.append(", ");
            }
            systemStaticNode.add(new DefaultMutableTreeNode(str.toString()));
        }

        initialized = true;
        return;
    }


    public void updateDisplay()
    {
        int           end = 499;
        int[]         kids3 = {0, 1, 2},  kids4 = {1, 2, 3, 4};
        int[]         kids6 = {1, 2, 3, 4, 5, 6};
        Integer       attId;
        boolean       blocking=false, isNewNode=false;
        String        statName;
        StringBuffer  str = new StringBuffer(end+1);
        DefaultMutableTreeNode node = null, leaf = null;
        DefaultMutableTreeNode statsNode = null, configNode = null;
        // Update system info

        // Event rate
        if (!updated) {
            time1 = System.currentTimeMillis();
            str.append("Event rate = ");
            str.append(rate);
            str.append(" Hz");
            systemDynamicNode.add(new DefaultMutableTreeNode(str.toString()));
        }


        str.delete(0, end);
        str.append("Events owned by: ");
        str.append(" sys (");
        str.append(data.sysData.getEventsOwned());
        str.append("), atts ");
        for (int i=0; i < data.attData.length; i++) {
            str.append(data.attData[i].getId());
            str.append("(");
            str.append(data.attData[i].getEventsOwned());
            str.append(")");
            if (i == data.attData.length - 1) break;
            str.append(", ");
        }
        if (updated) {
            leaf = systemDynamicNode.getFirstLeaf().getNextLeaf();
            leaf.setUserObject(str.toString());
        }
        else {
            systemDynamicNode.add(new DefaultMutableTreeNode(str.toString()));
        }

        // idle stations
        str.delete(0, end);
        str.append("Idle stations: ");
        boolean gotNone = true;
        for (int i=0; i < data.statData.length; i++) {
            if (data.statData[i].getStatus() == EtConstants.stationIdle) {
                str.append(data.statData[i].getName());
                str.append(", ");
                gotNone = false;
            }
        }
        if (gotNone) {
            str.append("none");
        }
        if (updated) {
            leaf = leaf.getNextLeaf();
            leaf.setUserObject(str.toString());
        }
        else {
            systemDynamicNode.add(new DefaultMutableTreeNode(str.toString()));
        }


        // stations linked list
        str.delete(0, end);
        str.append("All stations: ");
        for (int i=0; i < data.statData.length; i++) {
            str.append(data.statData[i].getName());
            if (i == data.statData.length - 1) break;
            str.append(", ");
        }
        if (updated) {
            leaf = leaf.getNextLeaf();
            leaf.setUserObject(str.toString());
        }
        else {
            systemDynamicNode.add(new DefaultMutableTreeNode(str.toString()));
        }

        str.delete(0, end);
        str.append("Stations =");
        str.append(data.sysData.getStations());
        str.append(", attachments = ");
        str.append(data.sysData.getAttachments());
        str.append(", temp events = ");
        str.append(data.sysData.getTemps());
        if (updated) {
            leaf = leaf.getNextLeaf();
            leaf.setUserObject(str.toString());
        }
        else {
            systemDynamicNode.add(new DefaultMutableTreeNode(str.toString()));
        }

        if (!isJava) {
            str.delete(0, end);
            str.append("Processes = ");
            str.append(data.sysData.getProcesses());
            str.append(", hearbeat = ");
            str.append(data.sysData.getHeartbeat());
            if (updated) {
                leaf = leaf.getNextLeaf();
                leaf.setUserObject(str.toString());
            }
            else {
                systemDynamicNode.add(new DefaultMutableTreeNode(str.toString()));
            }

            // mutexes
            str.delete(0, end);
            str.append("Locked mutexes: ");
            if (data.sysData.getMutex() == EtConstants.mutexLocked)
                str.append("sys, ");
            if (data.sysData.getStatMutex() == EtConstants.mutexLocked)
                str.append("stat, ");
            if (data.sysData.getStatAddMutex() == EtConstants.mutexLocked)
                str.append("add_stat, ");

            for (int i=0; i < data.statData.length; i++) {
                if (data.statData[i].getMutex() == EtConstants.mutexLocked) {
                    str.append(data.statData[i].getName());
                    str.append(", ");
                }
                if (data.statData[i].getInListMutex() == EtConstants.mutexLocked) {
                    str.append(data.statData[i].getName());
                    str.append("-in, ");
                }
                if (data.statData[i].getOutListMutex() == EtConstants.mutexLocked) {
                    str.append(data.statData[i].getName());
                    str.append("-out, ");
                }
            }
            if (updated) {
                leaf = systemDynamicNode.getLastLeaf();
                leaf.setUserObject(str.toString());
                treeModel.nodesChanged(systemDynamicNode, kids6);
            }
            else {
                systemDynamicNode.add(new DefaultMutableTreeNode(str.toString()));
            }
        }
        else if (updated) {
            treeModel.nodesChanged(systemDynamicNode, kids4);
        }

        // Update station info

        // first remove stations not in current list
        int numStations = data.statData.length;
        oldList: for (Iterator i=stations.keySet().iterator(); i.hasNext();) {
            statName = (String) i.next();
            newList: for (int j=0; j < numStations; j++) {
                if (statName.equals(data.statData[j].getName())) {
                    continue oldList;
                }
            }
//System.out.println("Removing node for station " + statName);
            node = (DefaultMutableTreeNode) stations.get(statName);
            treeModel.removeNodeFromParent(node);
            i.remove();
        }

        // create new stations and update existing stations
        for (int i=0; i < numStations; i++) {

            // Get station name & find its node object if there is one,
            // else make a new node and store it in the hash table
            statName = data.statData[i].getName();
            if (stations.containsKey(statName)) {
                isNewNode = false;
                node = (DefaultMutableTreeNode) stations.get(statName);
//System.out.println("Node " + node + " has got " + node.getChildCount() + " children");
                configNode = (DefaultMutableTreeNode)node.getFirstChild();
                statsNode  = (DefaultMutableTreeNode)node.getLastChild();
//System.out.println("Child 1 = " + configNode);
//System.out.println("Child 2 = " + statsNode);
            }
            else {
//System.out.println("Adding node for station " + statName);
                isNewNode  = true;
                node       = new DefaultMutableTreeNode(statName);
                configNode = new DefaultMutableTreeNode("Configuration");
                statsNode  = new DefaultMutableTreeNode("Status");
                node.add(configNode);
                node.add(statsNode);
                stations.put(statName, node);
                // wait for all "node" changes before inserting into tree
            }

            // station config (skip GC as it never changes)
            if ((i != 0) || isNewNode ) {
                str.delete(0, end);
                if (data.statData[i].getFlowMode() == EtConstants.stationSerial) {
                    str.append("Serial, ");
                }
                else {
                    str.append("Parallel, ");
                }

                if (data.statData[i].getBlockMode() == EtConstants.stationBlocking) {
                    str.append("blocking, ");
                    blocking = true;
                    str.append("prescale = ");
                    str.append(data.statData[i].getPrescale());
                    str.append(", (cue = ");
                    str.append(data.statData[i].getCue());
                    str.append(")");
                }
                else {
                    str.append("nonblocking, ");
                    blocking = false;
                    str.append("cue = ");
                    str.append(data.statData[i].getCue());
                    str.append(", (prescale = ");
                    str.append(data.statData[i].getPrescale());
                    str.append(")");
                }

                if (isNewNode) {
//System.out.println("Add first leaf to config node");
                    configNode.add(new DefaultMutableTreeNode(str.toString()));
                }
                else {
//System.out.println("Config has got " + configNode.getLeafCount() + " # of leaves");
                    leaf = configNode.getFirstLeaf();
//System.out.println("First config leaf = " + leaf);
                    leaf.setUserObject(str.toString());
                }


                str.delete(0, end);
                if (data.statData[i].getUserMode() == EtConstants.stationUserMulti) {
                    str.append("Users = multi");
                }
                else {
                    str.append("Users = ");
                    str.append(data.statData[i].getUserMode());
                }

                if (data.statData[i].getRestoreMode() == EtConstants.stationRestoreOut)
                    str.append(", restore = out, ");
                else if (data.statData[i].getRestoreMode() == EtConstants.stationRestoreIn)
                    str.append(", restore = in, ");
                else
                    str.append(", restore = GC, ");

                if (data.statData[i].getSelectMode() == EtConstants.stationSelectAll)
                    str.append("select = all");
                else if (data.statData[i].getSelectMode() == EtConstants.stationSelectMatch)
                    str.append("select = match");
                else if (data.statData[i].getSelectMode() == EtConstants.stationSelectUser)
                    str.append("select = user");
                else if (data.statData[i].getSelectMode() == EtConstants.stationSelectRRobin)
                    str.append("select = rrobin");
                else
                    str.append("select = equalcue");

                if (isNewNode) {
//System.out.println("Add second leaf to config node");
                    configNode.add(new DefaultMutableTreeNode(str.toString()));
                }
                else {
                    leaf = leaf.getNextLeaf();
//System.out.println("Next config leaf = " + leaf);
                    leaf.setUserObject(str.toString());
                }


                str.delete(0, end);
                str.append("Select words: ");
                for (int j=0; j < EtConstants.stationSelectInts; j++) {
                    str.append(data.statData[i].getSelect()[j]);
                    if (j == EtConstants.stationSelectInts - 1) break;
                    str.append(", ");
                }
                if (isNewNode) {
//System.out.println("Add third leaf to config node");
                    configNode.add(new DefaultMutableTreeNode(str.toString()));
                }
                else {
                    leaf = leaf.getNextLeaf();
//System.out.println("Next config leaf = " + leaf);
                    leaf.setUserObject(str.toString());
                }

                // Select mode never changes after station creation,
                // but station can be removed and then recreated with
                // a new configuration (in less time than it takes to
                // update).
                if (data.statData[i].getSelectMode() == EtConstants.stationSelectUser) {
                    str.delete(0, end);
                    if (isJava) {
                        str.append("Class = ");
                        str.append(data.statData[i].getSelectClass());
                    }
                    else {
                        str.append("Function = ");
                        str.append(data.statData[i].getSelectFunction());
                        str.append(", library = ");
                        str.append(data.statData[i].getSelectLibrary());
                    }
                    if (isNewNode) {
//System.out.println("Add fourth leaf to config node");
                        configNode.add(new DefaultMutableTreeNode(str.toString()));
                    }
                    else {
                        leaf = leaf.getNextLeaf();
//System.out.println("Next config leaf = " + leaf);
                        leaf.setUserObject(str.toString());
                    }
                }

                if (!isNewNode) {
                    treeModel.nodesChanged(configNode, kids3);
                }


            } // if not GC



            // statistical station info

            str.delete(0, end);
            if (data.statData[i].getStatus() == EtConstants.stationIdle) {
                str.append("Idle, ");
            }
            else {
                str.append("Active, ");
            }
            str.append("attachments: total = ");
            str.append(data.statData[i].getAttachments());
            str.append(",  ids = ");
            int limit = data.statData[i].getAttachments();
            for (int j=0; j < limit; j++) {
                str.append(data.statData[i].getAttachmentIds()[j]);
                if (j == limit - 1) break;
                str.append(", ");
            }

            if (isNewNode) {
//System.out.println("Add first leaf to statistics");
                statsNode.add(new DefaultMutableTreeNode(str.toString()));
            }
            else {
//System.out.println("Statistics has got " + statsNode.getLeafCount() + " # of leaves");
                leaf = statsNode.getFirstLeaf();
//System.out.println("First statistics leaf = " + leaf);
                leaf.setUserObject(str.toString());
            }

            str.delete(0, end);
            str.append("Input events:   ");
            str.append(data.statData[i].getInListCount());
            str.append(", total = ");
            str.append(data.statData[i].getInListIn());
            // if blocking station and not grandcentral ...
            if (blocking && (data.statData[i].getId() != 0)) {
                str.append(", try = ");
                str.append(data.statData[i].getInListTry());
            }
            // helps reduce widget flashing
            str.append("            ");

            if (isNewNode) {
//System.out.println("Add second leaf to statistics");
                statsNode.add(new DefaultMutableTreeNode(str.toString()));
            }
            else {
                leaf = leaf.getNextLeaf();
//System.out.println("Next statistics leaf = " + leaf);
                leaf.setUserObject(str.toString());
            }

            str.delete(0, end);
            str.append("Output events: ");
            str.append(data.statData[i].getOutListCount());
            str.append(", total = ");
            str.append(data.statData[i].getOutListOut());
            str.append("            ");

            if (isNewNode) {
//System.out.println("Add third leaf to statisics");
                statsNode.add(new DefaultMutableTreeNode(str.toString()));
                // add new station to main tree
                treeModel.insertNodeInto(node, stationNode, i);
            }
            else {
                leaf = leaf.getNextLeaf();
//System.out.println("Next statistics leaf = " + leaf);
                //treeModel.valueForPathChanged(new TreePath(leaf), str.toString());
                leaf.setUserObject(str.toString());
                treeModel.nodesChanged(statsNode, kids3);
            }

            // keep track of grandcentral data rate
            if ((i==0) && (updated)) {
                long gcOut = data.statData[i].getOutListOut();
                time2 = System.currentTimeMillis();
                rate = ((1000 * (gcOut - prevGcOut))/(time2-time1));
                prevGcOut = gcOut;
                time1 = time2;
                str.delete(0, end);
                str.append("Event rate = ");
                str.append(rate);
                str.append(" Hz");
                leaf = systemDynamicNode.getFirstLeaf();
                leaf.setUserObject(str.toString());
                treeModel.nodeChanged(leaf);
            }
        } // for (int i=0; i < numStations; i++) {

        // User processes exist on Solaris only
        if (isSolaris) {
            // Processes are only leaves on the tree, so reuse
            // any that are there, delete or add to suit.

            int numProcs    = data.procData.length;
            int numLeaves   = processNode.getChildCount();
            int leafCounter = numLeaves;

            if (numProcs > 0) {
                for (int i=0; i < numProcs; i++) {
                    str.delete(0, end);
                    if (data.procData[i].getAttachments() < 1) {
                        str.append("Id = ");
                        str.append(data.procData[i].getId());
                        str.append(", no attachments, ");
                    }
                    else {
                        str.append("Id = ");
                        str.append(data.procData[i].getId());
                        str.append(", ");
                        str.append(data.procData[i].getAttachments());
                        str.append(" attachments, ids = ");
                        for (int j=0; j < data.procData[i].getAttachments(); j++) {
                            str.append(data.procData[i].getAttachmentIds()[j]);
                            str.append(", ");
                        }
                    }
                    str.append("pid = ");
                    str.append(data.procData[i].getPid());
                    str.append(", hbeat = ");
                    str.append(data.procData[i].getHeartbeat());

                    if (leafCounter < 1) {
                        node = new DefaultMutableTreeNode(str.toString());
                        treeModel.insertNodeInto(node, processNode, i);
                    }
                    else {
                        node = (DefaultMutableTreeNode) processNode.getChildAt(i);
                        node.setUserObject(str.toString());
                        leafCounter--;
                    }
                }

                if (numLeaves > 1) {
//System.out.println("More than one leaf");
                    // update through leaves that were reused
                    leafCounter = numProcs > numLeaves ? numLeaves : numProcs;
                    int[] leaves = new int[leafCounter];
                    for (int i=0; i < leafCounter; i++) {
                        leaves[i] = i;
                    }
                    treeModel.nodesChanged(processNode, leaves);

                    // remove leaves that aren't needed
                    if (numLeaves > numProcs) {
//System.out.println("More leaves than processes");
                        for (int i=0; i < numLeaves-numProcs; i++) {
                            node = (DefaultMutableTreeNode) processNode.getChildAt(i + numProcs);
//System.out.println("Remove node " + (i+numProcs) + " called " + node);
                            treeModel.removeNodeFromParent(node);
                        }
                    }
                }
            }
        }

        // user attachments

        // first remove attachments not in current list
        int numAtts = data.attData.length;

        oldList: for (Iterator i=attachments.keySet().iterator(); i.hasNext();) {
            attId = (Integer) i.next();
            newList: for (int j=0; j < numAtts; j++) {
                if (attId.intValue() == data.attData[j].getId()) {
                    continue oldList;
                }
            }
//System.out.println("Removing node for att " + attId);
            node = (DefaultMutableTreeNode) attachments.get(attId);
            treeModel.removeNodeFromParent(node);
            i.remove();
        }

        if (data.attData.length > 0) {
            // create new attachments and update existing attachments
            for (int i=0; i < numAtts; i++) {
                // Get Attachment Id & find its node object if there is one,
                // else make a new node and store it in the hash table
                attId = new Integer(data.attData[i].getId());
                if (attachments.containsKey(attId)) {
                    isNewNode = false;
//System.out.println("Using old node for attachment " + attId);
                    node = (DefaultMutableTreeNode) attachments.get(attId);
                }
                else {
//System.out.println("Adding node for attachment " + attId);
                    isNewNode  = true;
                    node       = new DefaultMutableTreeNode(attId);
                    attachments.put(attId, node);
                    // wait for all "node" changes before inserting into tree
                }

                // graph attIcons & links

                str.delete(0, end);
                str.append("Station = ");
                str.append(data.attData[i].getStationName());
                str.append(", host = ");
                str.append(data.attData[i].getHost());

                if (isNewNode) {
                    node.add(new DefaultMutableTreeNode(str.toString()));
                }
                else {
                    leaf = node.getFirstLeaf();
                    leaf.setUserObject(str.toString());
                }

                if (data.attData[i].getIpAddress() != null) {
                    str.delete(0, end);
                    str.append("Sending ip address = ");
                    str.append(data.attData[i].getIpAddress());

                    if (isNewNode) {
                        node.add(new DefaultMutableTreeNode(str.toString()));
                    }
                    else {
                        leaf = leaf.getNextLeaf();
                        leaf.setUserObject(str.toString());
                    }
                }

                str.delete(0, end);
                if (data.attData[i].blocked() == true) {
                    str.append("Blocked");
                }
                else {
                    str.append("Not blocked");
                }
                if (data.attData[i].quitting() == true) {
                    str.append(", quitting");
                }

                if (!isJava) {
                    str.append(", pid = ");
                    str.append(data.attData[i].getPid());
                    if (isSolaris) {
                        str.append(", proc = ");
                        str.append(data.attData[i].getProc());
                    }
                }

                if (isNewNode) {
                    node.add(new DefaultMutableTreeNode(str.toString()));
                }
                else {
                    leaf = leaf.getNextLeaf();
                    leaf.setUserObject(str.toString());
                }

                str.delete(0, end);
                str.append("Own(");
                str.append(data.attData[i].getEventsOwned());
                str.append("), make(");
                str.append(data.attData[i].getEventsMake());
                str.append("), get(");
                str.append(data.attData[i].getEventsGet());
                str.append("), put(");
                str.append(data.attData[i].getEventsPut());
                str.append("), dump(");
                str.append(data.attData[i].getEventsDump());
                str.append(")            ");

                if (isNewNode) {
                    node.add(new DefaultMutableTreeNode(str.toString()));
//System.out.println("Inserting new attach node at position " + i);
                    treeModel.insertNodeInto(node, attachNode, i);
                }
                else {
                    leaf = leaf.getNextLeaf();
                    leaf.setUserObject(str.toString());
                    treeModel.nodesChanged(node, kids3);
                }
            } // for (int i=0; i < numAtts; i++) {
        } // if (data.attData.length > 0) {

        // update graphics as well
        updateGraph();
        updated = true;
        updateTime = System.currentTimeMillis();
        return;
    }


    private void makeStationIdle(LxGroup stat) {
        LxComponent[] comps = stat.getComponents();
        ((LxElement) comps[0]).setPaint(Lx.getGradientPaint(0, 0, stationIdleColor, 200, 0, Color.black, true));
        ((LxRectangle)((LxAbstractGroup)comps[7]).getComponent(0)).setLineColor(stationIdleColor);
        ((LxRectangle)((LxAbstractGroup)comps[8]).getComponent(0)).setLineColor(stationIdleColor);
        return;
    }


    private void makeStationActive(LxGroup stat) {
        LxComponent[] comps = stat.getComponents();
        ((LxElement) comps[0]).setPaint(Lx.getGradientPaint(0, 0, stationColor, 200, 0, Color.black, true));
        ((LxRectangle)((LxAbstractGroup)comps[7]).getComponent(0)).setLineColor(stationColor);
        ((LxRectangle)((LxAbstractGroup)comps[8]).getComponent(0)).setLineColor(stationColor);
        return;
    }


    private LxGroup makeStationIcon(String name, int status) {
        LxGroup station = new LxGroup();

        stationWidth = 100.;
        double width = stationWidth, height = 100., border = 2.;
        double textWidth = 90., textHeight = 10., textBoxHeight = 20.;
        double x = 100., y = 100.;

        // main background

        LxRectangle backgrd = new LxRectangle();
        backgrd.setSize(width, height);
        backgrd.setLocation(x, y);
        // set the rect to have a linear color gradient fill pattern
        if (status == EtConstants.stationActive) {
            backgrd.setPaint(Lx.getGradientPaint(0, 0, stationColor, 200, 0, Color.black, true));
        }
        else {
            backgrd.setPaint(Lx.getGradientPaint(0, 0, stationIdleColor, 200, 0, Color.black, true));
        }
        station.add(backgrd, 0);

        // Station name display

        LxRectangle textBackgrd = new LxRectangle();
        textBackgrd.setSize(width - 2.*border, textBoxHeight);
        textBackgrd.setCenter(x + width / 2., y + border + textBoxHeight / 2.);
        textBackgrd.setPaint(Color.white);
        station.add(textBackgrd, 1);

        LxText text = new LxText(name);
        text.setSize(textWidth, textHeight);
        text.setCenter(x + width/2., y + border + textBoxHeight/2.);
        //text.setLineThickness(.0f);
        text.setPaint(textColor);  // set text background
        text.setLineColor(textColor); // set text outline
        text.setRegularTextAntialiased(true);
        station.add(text, 2);

        // "IN" & "OUT" labels

        double recWidth, recHeight, recTextHeight, yCenter, space;
        recHeight = 14;
        recTextHeight = 7;
        recWidth = 35;
        space = (width - 2*recWidth)/3.;
        yCenter = y + border + textBoxHeight + space + recHeight/2.;

        LxRectangle recIn = new LxRectangle();
        recIn.setSize(recWidth, recHeight);
        recIn.setCenter(x + space + recWidth / 2., yCenter);
        recIn.setPaint(Color.white);
        station.add(recIn, 3);

        LxRectangle recOut = new LxRectangle();
        recOut.setSize(recWidth, recHeight);
        recOut.setCenter(x + 2. * space + 1.5 * recWidth, yCenter);
        recOut.setPaint(Color.white);
        station.add(recOut, 4);

        LxText in = new LxText("In");
        in.setSize(recWidth - 25, recTextHeight);
        in.setCenter(x + space + recWidth/2., yCenter);
        in.setRegularTextAntialiased(true);
        in.setPaint(textColor);
        in.setLineColor(textColor);
        station.add(in, 5);

        LxText out = new LxText("Out");
        out.setSize(recWidth - 15, recTextHeight);
        out.setCenter(x + 2.*space + 1.5*recWidth, yCenter);
        out.setRegularTextAntialiased(true);
        out.setPaint(textColor);
        out.setLineColor(textColor);
        station.add(out, 6);

        // event count bars

        double slideHeight = height - (5*border + space + textBoxHeight + recHeight);
        double yTop = y + border + textBoxHeight + space + recHeight + 2*border;
        double sliderWidth = 20;
        space = (width - 2*sliderWidth)/3.;

        // Create a BOX for the in slider.
        LxRectangle inBox = new LxRectangle();
        inBox.setName("BOX");
        inBox.setLocation(x + space, yTop);
        inBox.setSize(sliderWidth, slideHeight);
        if (status == EtConstants.stationActive) {
            inBox.setLineColor(stationColor);
        }
        else {
            inBox.setLineColor(stationIdleColor);
        }
        inBox.setLineThickness(2);

        // Create INDICATOR of in slider.
        LxRectangle inIndic = new LxRectangle();
        inIndic.setName("INDICATOR");
        inIndic.setLocation(x + space, yTop);
        inIndic.setSize(sliderWidth, slideHeight);
        inIndic.setPaint(Lx.getGradientPaint(0, 0, eventColor, 45, 0, Color.black, true));
        inIndic.setLineColor(eventColor);

        // in slider
        LxSlider inSlider = new LxSlider(false, false);
        inSlider.setName("inCount");
        inSlider.setMaximum((double) sys.getNumEvents());
        inSlider.setMinimum(0.);
        inSlider.setValue(0.);
        inSlider.add(inBox);
        inSlider.add(inIndic);

        // Create a BOX for the out slider.
        LxRectangle outBox = new LxRectangle();
        outBox.setName("BOX");
        outBox.setLocation(x + 2.*space + sliderWidth, yTop);
        outBox.setSize(sliderWidth, slideHeight);
        if (status == EtConstants.stationActive) {
            outBox.setLineColor(stationColor);
        }
        else {
            outBox.setLineColor(stationIdleColor);
        }
        outBox.setLineThickness(2);

        // Create INDICATOR of out slider.
        LxRectangle outIndic = new LxRectangle();
        outIndic.setName("INDICATOR");
        outIndic.setLocation(x + 2.*space + sliderWidth, yTop);
        outIndic.setSize(sliderWidth, slideHeight);
        outIndic.setPaint(Lx.getGradientPaint(0, 0, eventColor, 45, 0, Color.black, true));
        outIndic.setLineColor(eventColor);

        // out slider
        LxSlider outSlider = new LxSlider(false, false);
        outSlider.setName("outCount");
        outSlider.setMaximum((double) sys.getNumEvents());
        outSlider.setMinimum(0.);
        outSlider.setValue(0.);
        outSlider.add(outBox);
        outSlider.add(outIndic);

        station.add(inSlider,  7);
        station.add(outSlider, 8);

        return station;
    }


    private LxGroup makeAttachmentIcon(String name) {
        LxGroup attachment = new LxGroup();

        attWidth = 40.;
        double width = attWidth, height = 80., border = 2.;
        double textWidth = 10., textHeight = 10., textBoxHeight = 20.;
        double x = 150., y = 150.;

        LxRectangle rec = new LxRectangle();
        rec.setSize(width, height);
        rec.setLocation(x, y);
        rec.setPaint(Lx.getGradientPaint(0, 0, attachColor, 45, 0, Color.black, true));
        attachment.add(rec, 0);

        LxRectangle rec2 = new LxRectangle();
        rec2.setSize(width - 2.*border, textBoxHeight);
        rec2.setCenter(x + width/2., y + border + textBoxHeight/2.);
        rec2.setPaint(Color.white);
        attachment.add(rec2, 1);

        LxText text = new LxText(name);
        text.setSize(textWidth, textHeight);
        text.setCenter(x + width/2., y + border + textBoxHeight/2.);
        //text.setLineThickness(.0f);
        text.setPaint(textColor);     // set text background color
        text.setLineColor(textColor); // set text outline color
        text.setRegularTextAntialiased(true);
        attachment.add(text, 2);

        // event count bar
        double sliderWidth = 20;
        double space = (width - sliderWidth)/2.;
        double sliderHeight = height - (3*border + space + textBoxHeight);
        double yTop = y + border + textBoxHeight + space;

        // Create a BOX for the slider.
        LxRectangle box = new LxRectangle();
        box.setName("BOX");
        box.setLocation(x + space, yTop);
        box.setSize(sliderWidth, sliderHeight);
        box.setLineColor(attachColor);
        box.setLineThickness(2);

        // Create INDICATOR of slider.
        LxRectangle indic = new LxRectangle();
        indic.setName("INDICATOR");
        indic.setLocation(x + space, yTop);
        indic.setSize(sliderWidth, sliderHeight);
        indic.setPaint(Lx.getGradientPaint(0, 0, eventColor, 45, 0, Color.black, true));
        indic.setLineColor(eventColor);

        // slider
        LxSlider slider = new LxSlider(false, false);
        slider.setName("count");
        slider.setMaximum((double) sys.getNumEvents());
        slider.setMinimum(0.);
        slider.setValue(0.);
        slider.add(box);
        slider.add(indic);
        attachment.add(slider, 3);

        return attachment;
    }


    private void updateGraph()
    {
        double        newWidth, width=0., height=0., standardHeight;
        LxLink        link, link2;
        LxGroup       icon=null, statIcon=null, firstIcon=null;
        LxHandle      handleA=null, handleB=null, handleLeftGC=null, handleRightGC=null;
        LxTreeLayout  statManager=null, prevStatManager=null;
        LxAlignLayout parallelManager=null, prevParallelManager=null, parallelLayout=null;
        LxComponent[] comps;
        StationFlowData userData;
        StationLinkData linkData;
        String        stationName, statName=null;
        int           att, limit, position;
        int           numStations = data.statData.length;
        boolean       gotStation = false, redoLinks = false, didParallelLinks = false;
        boolean       newParallelManager = false;

        // distance between stations
        stationGap = 30.;
        // height of station & its attachments
        standardHeight = 270.;

        // if starting new graph ...
        if (stationManagers.size() < 1) {
            redoLinks = true;
        }
        else {
            // See if stations have changed. If so, redo links.
            if (numStations != stationManagers.size()) {
                redoLinks = true;
            }
            else {
                for (int i=0; i < numStations; i++) {
                    // look thru linked list for this station
                    statManager = (LxTreeLayout) stationManagers.get(i);
                    if (statManager == null) {
                        redoLinks = true;
                        break;
                    }
                    else {
                        stationName = (String) statManager.getName();
                        statName = data.statData[i].getName();
                        if (statName.equals(stationName) == false) {
                            redoLinks = true;
                            break;
                        }
                    }
                }
            }

            // If redoing links, first delete all existing links between stations.
            // Links to attachments remain untouched.
            if (redoLinks) {
                comps = graph.getComponents();
                for (int j=0; j<comps.length; j++) {
                    if (comps[j].getName().equals("station")) {
                        statIcon = (LxGroup) comps[j];
                        linkData = (StationLinkData) statIcon.getUserData();
                        if (linkData != null) {
                            graph.remove(linkData.link1);
                            graph.remove(linkData.link2);
                            linkData = null;
                            statIcon.setUserData(null);
                        }
                    }
                }
            }
        }


        // Delete stations that no longer exist (& their attachments).
        // Links between stations & layout managers are already gone.
        if (numStations < stationManagers.size()) {
            // Go thru linked list of previously displayed stations
            for (ListIterator it=stationManagers.listIterator(); it.hasNext();) {
                statManager = (LxTreeLayout) it.next();
                stationName = (String) statManager.getName();
                gotStation = false;

                // Look thru current list of stations. If not there, delete it.
                for (int i=0; i < numStations; i++) {
                    if (stationName.equals(data.statData[i].getName())) {
                        gotStation = true;
                        break;
                    }
                }

                if (!gotStation) {
                    // If the station we're deleting has the 2 main links attached to it,
                    // we need take care of these by temporary linking them elsewhere until
                    // the real links are made later. This way we don't have to delete and
                    // recreate link objects (which I assume is more time consuming than
                    // temporarily changing the handles).
                    // Remove them from the graph to take care of the case that only
                    // GRAND_CENTRAL is left and we need no links. (Setting them to be
                    // invisible seems to cause the layout some problems in this case).
                    statIcon = (LxGroup) statManager.getComponent(0);
                    if (mainLink1.getHandle2() == statIcon.getHandle(2)) {
                        mainLink1.setHandles(mainLink1.getHandle1(), mainLink2.getHandle2());
                        mainLink2.setHandles(mainLink1.getHandle1(), mainLink2.getHandle2());
                        graph.remove(mainLink1);
                        graph.remove(mainLink2);
                    }

                    // Remove from graph
                    comps = statManager.getComponents();
                    for (int i=0; i < comps.length; i++) {
                        graph.remove(comps[i]);
                        // If attachment icon, remove from hash table.
                        if ((comps[i] instanceof LxGroup) && (i > 0)) {
                            attachIcons.remove(new Integer(Integer.parseInt(comps[i].getName())));
                        }
                    }

                    userData = (StationFlowData) statManager.getUserData();
                    parallelLayout = userData.parentLayout;
                    parallelLayout.remove(statManager);
                    int statCount = parallelLayout.getComponentCount();
                    if (parallelLayout.getComponentCount() > 0) {
                        parallelLayout.setHeight((double) (statCount*(standardHeight + 5.)));
                    }
                    if (debug) {
                        graph.remove(statManager);
                    }
                    // take it out of the linked list
                    it.remove();
                }
            }
        }

        // find first existing parallelManager
        ListIterator pList = parallelManagers.listIterator();
        int parallelPlace=0;

        // create new stations and update existing stations
        for (int i=0; i < numStations; i++) {
            // Get station name
            statName = data.statData[i].getName();
            gotStation = false;
            position = 0;

            // look thru linked list for this station
            for (ListIterator it=stationManagers.listIterator(); it.hasNext(); position++) {
                statManager = (LxTreeLayout) it.next();
                stationName = (String) statManager.getName();
                if (statName.equals(stationName)) {
                    gotStation = true;
                    break;
                }
            }

            // If this is a parallel station, but not the head,
            // it belongs with previous parallelManager.
            if (data.statData[i].getFlowMode() == EtConstants.stationParallel) {
                parallelManager = prevParallelManager;
                newParallelManager = false;
            }
            // First use an existing manager
            else if (pList.hasNext()) {
                parallelManager = (LxAlignLayout) pList.next();
                newParallelManager = false;
            }
            // Make a new one if none available
            else {
                parallelManager = new LxAlignLayout(new Rectangle2D.Double(0., 0., 100., standardHeight + 5.));
                parallelManager.setOrder(LxAlignLayout.LIST_ORDER);
                parallelManager.setOrientation(LxAlignLayout.VERTICAL);
                parallelManager.setAlignAxisMethod(LxAlignLayout.EVEN_USE_SIZE_METHOD);
                parallelManager.setNormalAxisMethod(LxAlignLayout.CENTER_METHOD);
                parallelManager.setLineColor(Color.green);
                parallelManager.setLineThickness(1.f);
                parallelManager.setVisible(true);
                pList.add(parallelManager);
                newParallelManager = true;
            }

            // Keep track of which place in the parallel manager a station is,
            // and the previous parallelManager
            if (data.statData[i].getFlowMode() != EtConstants.stationParallel) {
                parallelPlace       = 0;
                prevParallelManager = parallelManager;
            }
            else {
                parallelPlace++;
            }

            // make sure parallel managers are the right height
            parallelManager.setHeight((double) ((parallelPlace + 1)*(standardHeight + 5.)));

            if (gotStation) {
                // get ref to station icon in graph
                statIcon = (LxGroup) statManager.getComponent(0);
                // change color if idle
                if (data.statData[i].getStatus() == EtConstants.stationIdle) {
                    makeStationIdle(statIcon);
                }
                else {
                    makeStationActive(statIcon);
                }

                // make sure station is right place in linked list
                if (position != i) {
                    stationManagers.remove(statManager);
                    stationManagers.add(i, statManager);
                }

                // this station's parallel layout manager
                userData = (StationFlowData) statManager.getUserData();
                parallelLayout = userData.parentLayout;
                // make sure userData is up-to-date
                userData.flowMode = data.statData[i].getFlowMode();
                userData.isHead = false;
                if (userData.flowMode == EtConstants.stationParallelHead) {
                    userData.isHead = true;
                }

                // If this station is in this parallelManager, see that it's in the
                // right place, otherwise remove station from the old manager and
                // add it to this one.
                if (parallelLayout == parallelManager) {
                    if (parallelManager.getComponentIndex(statManager) != parallelPlace) {
                        parallelManager.remove(statManager);
                        parallelManager.add(statManager, parallelPlace);
                    }
                }
                else {
                    parallelLayout.remove(statManager);
                    parallelManager.add(statManager, parallelPlace);
                    userData.parentLayout = parallelManager;
                }
            }
            else {
                // create and store ref to station layout in graph
                statManager = new LxTreeLayout(new Rectangle2D.Double(0.0, 0.0, 110., standardHeight));
                statManager.setOrientation(LxTreeLayout.ROOT_TOP);
                statManager.setMethod(LxTreeLayout.SEPARATE_METHOD);
                statManager.setFixedSpacing(true);
                statManager.setFixedXSpacing(5.);
                statManager.setFixedYSpacing(60.);
                statManager.setName(statName);
                statManager.setVisible(true);
                statManager.setLineColor(Color.red);
                statManager.setLineThickness(1.f);
                statManager.setOrder(LxTreeLayout.LIST_ORDER);
                stationManagers.add(i, statManager);

                // keep track of a few things associated with each station
                userData = new StationFlowData();
                userData.flowMode = data.statData[i].getFlowMode();
                userData.isHead = false;
                if (userData.flowMode == EtConstants.stationParallelHead) {
                    userData.isHead = true;
                }
                userData.parentLayout = parallelManager;
                statManager.setUserData(userData);

                statIcon = makeStationIcon(statName, data.statData[i].getStatus());
                statIcon.setName("station");

                graph.add(statIcon);
                statManager.add(statIcon, 0);
                statManager.setRoot(statIcon);

                // add statManager to parallel layout manager
                parallelManager.add(statManager, parallelPlace);
                if (debug) {
                    graph.add(statManager);
                }
            }

            if (i == 0) {
                firstIcon     = statIcon;
                handleLeftGC  = statIcon.getHandle(2);
                handleRightGC = statIcon.getHandle(6);
            }

            // add to horizotal layout manager
            if (newParallelManager) {
                layout.add(parallelManager);
                if (debug) {
                    graph.add(parallelManager);
                }
            }

            // Make sure layout is wide enough. Pick the widest single station
            // as the width for a group of parallel stations.
            limit = data.statData[i].getAttachments();
            double totalAttWidth = limit*(attWidth + 5.);
            newWidth = totalAttWidth > stationWidth + 6. ? totalAttWidth : stationWidth + 6.;
            statManager.setWidth(newWidth);
            width = parallelManager.getWidth();
            // if parallel station ...
            if (userData.flowMode != EtConstants.stationSerial) {
                // if first station but using old parallelManager (that might be too wide), reset width
                if (!newParallelManager && userData.flowMode == EtConstants.stationParallelHead) {
                    parallelManager.setWidth(newWidth + 50.);
                }
                // need to increase width
                else if (width < newWidth + 49.) {
                    parallelManager.setWidth(newWidth + 50.);
                }
            }
            // if serial station ...
            else {
                parallelManager.setWidth(newWidth + 50.);
            }

            // graph links (skip GRAND_CENTRAL)
            if (redoLinks && i!=0) {
                // if last head/serial station in list, adjust top 2 links
                if (userData.flowMode != EtConstants.stationParallel) {
                    mainLink1.setHandles(handleRightGC, statIcon.getHandle(2));
                    mainLink2.setHandles(statIcon.getHandle(6), handleLeftGC);
                }
                // else if a parallel station, link with previous station
                else {
                    link = new LxLink(handleA, statIcon.getHandle(2));
                    link.setLinkType(LxAbstractLink.LINK_AUTOMATIC_ORTHOGONAL);
                    link.setLineArrow(LxArrowElement.ARROW_END);
                    link.setLineThickness(1.5f);
                    link.setLineColor(lineColor);
                    int linkWidth = (int)(parallelManager.getWidth()/2.) - 60;
                    link.setMinimumSegmentLength(linkWidth);
                    graph.add(link, 0);

                    link2 = new LxLink(handleB, statIcon.getHandle(6));
                    link2.setLinkType(LxAbstractLink.LINK_AUTOMATIC_ORTHOGONAL);
                    link2.setLineArrow(LxArrowElement.ARROW_NONE);
                    link2.setLineThickness(1.5f);
                    link2.setLineColor(lineColor);
                    link2.setMinimumSegmentLength(linkWidth);
                    graph.add(link2, 0);
                    statIcon.setUserData(new StationLinkData(link, link2));
                    // The following line corrects a bug in the Jloox package which
                    // improperly sets the minimum segment length back to the default
                    // (too small) under certain circumstances. Moving the icon seems
                    // to force Jloox to layout the links properly.
                    statIcon.setCenterY(statIcon.getCenterY() + 100.);
                    didParallelLinks = true;
                }

                // if this is the last station, make sure links are in the graph
                if (i == numStations-1) {
                    if (graph.getComponent("link1") == null) {
                        graph.add(mainLink1, 0);
                    }
                    if (graph.getComponent("link2") == null) {
                        graph.add(mainLink2, 1);
                    }
                }

            }
            // Always need to redo links for parallel stations because
            // changing width (number of attachments) affects the links.
            else if (userData.flowMode == EtConstants.stationParallel) {
                int linkWidth;
                comps = parallelManager.getComponents();
                for (int j=1; j < comps.length; j++) {
                    icon = (LxGroup) ((LxTreeLayout) comps[j]).getComponent(0);
                    linkData = (StationLinkData) icon.getUserData();
                    if (linkData != null) {
                        // The following line corrects a bug in the Jloox package which
                        // improperly sets the minimum segment length back to the default
                        // (too small) under certain circumstances. Moving the icon seems
                        // to force Jloox to layout the links properly.
                        statIcon.setCenterY(statIcon.getCenterY() + 100.);
                        linkWidth = (int)(parallelManager.getWidth()/2.) - 60;
                        linkData.link1.setMinimumSegmentLength(linkWidth);
                        if (linkData.link2 != null) {
                            linkData.link2.setMinimumSegmentLength(linkWidth);
                        }
                    }
                }
            }


            // update event count for input & output list
            LxSlider slider = (LxSlider) statIcon.getComponent(7);
            slider.setValue((double) data.statData[i].getInListCount());
            slider = (LxSlider) statIcon.getComponent(8);
            slider.setValue((double) data.statData[i].getOutListCount());

            // store objects for use by next station
            prevStatManager = statManager;
            if (userData.flowMode != EtConstants.stationParallel) {
                handleB = statIcon.getHandle(6);
                handleA = statIcon.getHandle(2);
            }

            // Check to see if any of the old attachments are gone.
            // If so, remove them and their links.
            comps = statManager.getComponents();

            // There are only attachments if length > 1
            if (comps.length > 1) {
                for (int j=0; j < comps.length ; j++) {
                    try {
                        // skip links & stations
                        if ((comps[j] instanceof LxLink) ||
                                (comps[j].getName().equals("station"))) {
                            continue;
                        }
                        boolean match = false;
                        int previousAtt = Integer.parseInt(((LxGroup) comps[j]).getName());
                        // see if current attachments include it
                        for (int k=0; k < limit; k++) {
                            att = data.statData[i].getAttachmentIds()[k];
                            if (att == previousAtt) {
                                match = true;
                                break;
                            }
                        }

                        if (!match) {
                            // remove this attachment from graph & layout manager
                            statManager.remove(comps[j]);
                            graph.remove(comps[j]);
                            // remove link too
                            graph.remove((LxLink) comps[j].getUserData());
                            // remove from hash table
                            attachIcons.remove(new Integer(previousAtt));
                        }
                    }
                    catch (NumberFormatException ex) {
                        //System.out.println("NumberFormatException");
                    }
                }
            }

            // There is still the possibility that
            // additional attachments were created.
            Integer attId;
            boolean addLink;
            LxGroup attIcon=null;

            for (int j=0; j < limit; j++) {
                att = data.statData[i].getAttachmentIds()[j];
                addLink = false;

                // Get Attachment Id & find its icon object if there is one,
                // else make a new icon and store it in the hash table
                attId = new Integer(att);
                if (attachIcons.containsKey(attId)) {
                    // Get ref to attachment icon in graph
                    attIcon = (LxGroup) attachIcons.get(attId);
                    // If it's not already in station's layout manager, add it.
                    if (statManager.getComponent(""+att) == null) {
                        statManager.add(attIcon);
                        addLink = true;
                    }
                }
                else {
                    // create and store ref to attachment icon in graph
                    attIcon = makeAttachmentIcon(attId.toString());
                    attIcon.setName(""+att);
                    attachIcons.put(attId, attIcon);
                    graph.add(attIcon);
                    statManager.add(attIcon);
                    addLink = true;
                }

                // link with station
                if (addLink) {
                    link = new LxLink(statIcon.getHandle(4), attIcon.getHandle(8));
                    link.setLineColor(lineColor);
                    link.setStartOnBorder(true);
                    // link.setLineArrow(LxArrowElement.ARROW_END);
                    graph.add(link);
                    attIcon.setUserData(link);
                }

                // update event count
                slider = (LxSlider) attIcon.getComponent(3);
                for (int k=0; k < data.attData.length; k++) {
                    if (data.attData[k].getId() == att) {
                        slider.setValue((double) data.attData[k].getEventsOwned());
                        break;
                    }
                }
            }
        } // for (int i=0; i < numStations; i++) {

        // remove unused parallelManagers
        while (pList.hasNext()) {
            parallelLayout = (LxAlignLayout) pList.next();
            pList.remove();
            layout.remove(parallelLayout);
            if (debug) {
                graph.remove(parallelLayout);
            }
        }

        // Add space between stations & for buffer on either side.
        width  = 0.;
        height = 0.;
        double tempHeight = 0;
        for (ListIterator li = parallelManagers.listIterator(); li.hasNext();) {
            parallelLayout = (LxAlignLayout) li.next();
            width  += parallelLayout.getWidth();
            tempHeight =  parallelLayout.getHeight();
            if (height < tempHeight) {
                height = tempHeight;
            }
        }

        // Set width of visible graphics.
        width += (parallelManagers.size()-1)*stationGap + 60.;
        layout.setWidth(width);
        // Set height of visible graphics.
        layout.setHeight(height + 5.);
        // Set boundaries of scrollbars appearing.
        view.setPreferredSize(new Dimension((int)width, (int)height));
        graphPane.updateUI();
        layout.setPropagate(true);
        layout.run();

        // change last link to look a little nicer
        if (numStations > 1) {
            // make the link go around from last to first station
            double deltaX = 20.;
            double deltaY = 20. + 100/2;
            double[]  tempPath = new double[12];
            Point2D[] tempPoints = mainLink2.getPathPoints();

            tempPath[0]  = tempPoints[0].getX();
            tempPath[1]  = tempPoints[0].getY();
            tempPath[10] = tempPoints[tempPoints.length -1].getX();
            tempPath[11] = tempPoints[tempPoints.length -1].getY();
            tempPath[2]  = tempPath[0]  + parallelManager.getWidth()/2. - 60.;
            tempPath[3]  = tempPath[1];
            tempPath[4]  = tempPath[2];
            tempPath[5]  = tempPath[1]  - deltaY;
            tempPath[6]  = tempPath[10] - deltaX;
            tempPath[7]  = tempPath[1]  - deltaY;
            tempPath[8]  = tempPath[10] - deltaX;
            tempPath[9]  = tempPath[11];

            mainLink2.setPath(tempPath);
        }
    }

}

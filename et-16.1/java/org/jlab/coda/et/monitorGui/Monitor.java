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
import java.net.*;
import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.util.List;
import javax.swing.*;
import javax.swing.tree.*;
import javax.swing.border.*;
import javax.swing.event.*;

import org.xml.sax.SAXException;
import org.jlab.coda.et.*;
import org.jlab.coda.et.exception.*;

/**
 * This class implements a monitor of ET systems.
 *
 * @author Carl Timmer
 */

public class Monitor extends JFrame {
    // static variables
    private static MonitorConfiguration config;
    private static File configurationFile;

    // important widgets' names
    private final JTabbedPane tabbedPane;
    private final JFrame openFrame;
    private final JMenu disconnectMenu, loadConnectionParametersMenu;
    private AddressJList bAddress;
    private AddressJList mAddress;
    private JComboBox<String> etName;
    private JComboBox<String> hostname;
    private JComboBox<String> cast;
    private WholeNumberField ttl, udpPort, tcpPort, period;
    private JButton connect;
    private JCheckBox allBroadcastAddrsBox;

    // other variables
    private String currentMonitorKey;
    private int defaultPeriod;
    private Color entryColor = Color.blue;
    private List<String> broadcastAddresses;
    private boolean multicasting = false;
    private boolean broadcasting = true;

    // keep track of connections to & monitors of ET systems
    public final Map<String, EtSystem> connections =
            Collections.synchronizedMap(new HashMap<String, EtSystem>(20));
    public final Map<String,MonitorSingleSystem> monitors =
            Collections.synchronizedMap(new HashMap<String,MonitorSingleSystem>(20));


    public Monitor() {
        this(null, null);
    }


    public Monitor(Dimension frameSize, Point frameLocation) {
        super("ET System Monitor");

        // Set window location.
        if (frameLocation != null) {
            setLocation(frameLocation);
        }
        // Default data update period in seconds.
        defaultPeriod = 5;

        // tabbedPane stuff
        tabbedPane = new JTabbedPane();

        if (frameSize == null) {
            frameSize = new Dimension(1100, 700);
        }
        tabbedPane.setPreferredSize(frameSize);
        // Keep track of which ET system we're currently looking at.
        tabbedPane.addChangeListener(
                new ChangeListener() {
                    public void stateChanged(ChangeEvent e) {
                        JTabbedPane source = (JTabbedPane) e.getSource();
                        int tabIndex = source.getSelectedIndex();
                        // if help pane is showing, reset period & return
                        if (tabIndex < 1) {
                            period.setValue(defaultPeriod);
                            currentMonitorKey = "Help";
                            return;
                        }
                        currentMonitorKey = source.getTitleAt(tabIndex);
                        int updatePeriod = (monitors.get(currentMonitorKey)).getUpdatePeriod();
                        period.setValue(updatePeriod);
                    }
                }
        );
        getContentPane().add(tabbedPane, BorderLayout.CENTER);

        // Final members need to be initialized in all constructors.
        openFrame = new JFrame("Open ET System");
        disconnectMenu = new JMenu("Disconnect");
        loadConnectionParametersMenu = new JMenu("Load Connection Parameters");

        // Make window used to input data needed to connect to an ET system.
        makeEtOpenWindow();
        // Define this window's menubar.
        makeMenubar();
        // Add to help screen to main window's tabbed pane
        tabbedPane.addTab("Help", null, makeHelpPane(), "help");
        currentMonitorKey = "Help";
    }

    // Change the update period of current single system monitor.
    private void setUpdatePeriod() {
        if (currentMonitorKey.equals("Help")) return;
        int updatePeriod = period.getValue();
        MonitorSingleSystem mon = (MonitorSingleSystem) monitors.get(currentMonitorKey);
        mon.setUpdatePeriod(updatePeriod);
        return;
    }

    //===================
    // Getters & Setters
    //===================

    // add ET file names to combo box
    public void addFileName(String name) {
        boolean nameIsThere = false;
        int count = etName.getItemCount();

        for (int i = 0; i < count; i++) {
            if (name.equals(etName.getItemAt(i))) {
                return;
            }
        }
        if (!nameIsThere) {
            etName.addItem(name);
        }
        return;
    }

    // add host names to combo box
    public boolean addHostname(String name) {
        if (name.equals(EtConstants.hostLocal) ||
                name.equals(EtConstants.hostRemote) ||
                name.equals(EtConstants.hostAnywhere)) {
            return false;
        }
        boolean nameIsThere = false;
        int count = hostname.getItemCount();
        for (int i = 0; i < count; i++) {
            if (name.equals(hostname.getItemAt(i))) {
                return true;
            }
        }
        if (!nameIsThere) {
            hostname.addItem(name);
        }
        return true;
    }

    // add addresses to combo boxes
    public void addBroadcastAddress(String addr) {
//        bAddress.addItem(addr);
        bAddress.addAddress(addr);
    }

    public void addMulticastAddress(String addr) {
//        mAddress.addItem(addr);
        mAddress.addAddress(addr);
    }

    //get ET names from combo box
    public String[] getFileNames() {
        int count = etName.getItemCount();
        if (count == 0) return null;
        String[] names = new String[count];
        for (int i = 0; i < count; i++) {
            names[i] = etName.getItemAt(i);
        }
        return names;
    }

    //get host names from combo box
    public String[] getHostnames() {
        // Skip the first 3 items as they never change.
        int count = hostname.getItemCount() - 3;
        if (count < 1) return null;
        String[] names = new String[count];
        for (int i = 0; i < count; i++) {
            names[i] = hostname.getItemAt(i + 3);
        }
        return names;
    }

    public int getMonitorWidth() {
        return tabbedPane.getWidth();
    }

    public int getMonitorHeight() {
        return tabbedPane.getHeight();
    }

    private boolean isValidIpAddress(String addr) {
        StringTokenizer tok = new StringTokenizer(addr, ".");
        if (tok.countTokens() != 4) {
            return false;
        }

        int number;
        String num;
        try {
            while (tok.hasMoreTokens()) {
                num = tok.nextToken();
                number = Integer.parseInt(num);
                if (number < 0 || number > 255) {
                    return false;
                }
                if (num.charAt(0) == '0' && (number != 0 || num.length() > 1)) {
                    return false;
                }
            }
        }
        catch (NumberFormatException ex) {
            return false;
        }
        return true;
    }

    private boolean isValidMulticastAddress(String addr) {
        InetAddress address = null;
        try {
            address = InetAddress.getByName(addr);
        }
        catch (UnknownHostException e) {
            return false;
        }

        return (address.isMulticastAddress());
    }

    public static void main(String[] args) {
        try {
            Monitor frame = null;

//            System.setProperty("javax.xml.parsers.SAXParserFactory",
//                               "com.sun.org.apache.xerces.internal.jaxp.SAXParserFactoryImpl");
//
            // allow for a configuration file argument
            if (args.length > 0) {
                if (args.length != 2) {
                    System.out.println("Usage: java Monitor [-f,-file <configFile>]");
                    return;
                }
                if (!(args[0].equalsIgnoreCase("-f") || args[0].equalsIgnoreCase("-file"))) {
                    System.out.println("Usage: java Monitor [-f,-file <configFile>]");
                    return;
                }

                configurationFile = new File(args[1]);

                // Read config file once to get window data only.
                // This is done because the frame needs to have the
                // size and position BEFORE it displays anything.
                config = new MonitorConfiguration(null);
                config.loadWindowParameters(configurationFile);
                Dimension size = config.getWindowSize();
                Point location = config.getWindowLocation();
                frame = new Monitor(size, location);
                // Read config file again to get the rest of the data.
                // This needs the application to have already started
                // (as in the previous line) - the reason being that
                // connections to ET systems need to be made etc.
                config.setMonitor(frame);
                config.load(configurationFile);
            }
            else {
                frame = new Monitor();
                config = new MonitorConfiguration(frame);
            }
            frame.addWindowListener(new WindowAdapter() {
                public void windowClosing(WindowEvent e) {
                    System.exit(0);
                }
            });

            frame.pack();
            frame.setVisible(true);

            MonitorSingleSystem mon = null;
            DefaultMutableTreeNode monNode = null;

            // Class designed to run graphics commands in the Swing thread.
            class Updater extends Thread {
                MonitorSingleSystem mon;  // single system monitor of interest

                public void setMonitor(MonitorSingleSystem m) {
                    mon = m;
                }

                public Updater(MonitorSingleSystem m) {
                    mon = m;
                }

                public void run() {
                    if (mon.isInitialized()) {
                        mon.updateDisplay();
                        mon.treeDidChange();
                    }
                    else {
                        mon.staticDisplay();
                        mon.updateDisplay();
                        mon.updateUI();
                    }
                }
            }

            Updater updater = new Updater(mon);

            while (true) {
                // While we're in the iterator, we CANNOT have monitors added
                // (and thereby change the structure of the HashMap).
                synchronized (frame.monitors) {
                    for (Iterator<Map.Entry<String, MonitorSingleSystem>> i = frame.monitors.entrySet().iterator(); i.hasNext();) {
                        // get monitor object
                        mon = (i.next()).getValue();

                        try {
                            // only update if enough time has elapsed
                            if (mon.timeToUpdate()) {
                                // get data
                                mon.getData();
                                updater.setMonitor(mon);
                                // display new data
                                SwingUtilities.invokeLater(updater);
                            }
                        }
                        catch (EtException ex) {
                            //System.out.print("\n*****************************************\n");
                            //System.out.print("*   Error getting data from ET system   *");
                            //System.out.print("\n*****************************************\n");
                            //ex.printStackTrace();
                        }
                        catch (Exception ex) {
                            //System.out.print("\n*****************************************\n");
                            //System.out.print("* I/O error getting data from ET system *");
                            //System.out.print("\n*****************************************\n");
                            String key = mon.getKey();
                            // Remove connection with the IO problem.
                            frame.removeConnection(frame, mon, key, false);
                            // Remove single system monitor from hash table.
                            i.remove();
                            // Remove EtSystem object from hash table.
                            frame.connections.remove(key);

                            //ex.printStackTrace();
                        }
                    }
                }
                Thread.sleep(500);
            }
        }

        catch (Exception ex) {
            System.out.println("Unrecoverable error in ET monitor:");
            ex.printStackTrace();
        }

    }


    private void makeMenubar() {

        JMenuBar menuBar = new JMenuBar();
        setJMenuBar(menuBar);

        // file menu
        JMenu fileMenu = new JMenu("File");
        menuBar.add(fileMenu);

        // Create a file chooser
        final JFileChooser fc = new JFileChooser(System.getProperty("user.dir"));

        // file menu item to save configuration
        JMenuItem menuItem = new JMenuItem("Save Configuration");
        fileMenu.add(menuItem);
        menuItem.addActionListener(
                new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        File file;
                        if (configurationFile == null) {
                            if (fc.showSaveDialog(Monitor.this) == JFileChooser.CANCEL_OPTION) {
                                return;
                            }
                            file = fc.getSelectedFile();
                        }
                        else {
                            file = configurationFile;
                        }

                        try {
                            config.save(file);
                        }
                        catch (IOException ex) {
                            JOptionPane.showMessageDialog(new JFrame(),
                                                          "Cannot write to file \"" + file.getName() + "\"",
                                                          "Error",
                                                          JOptionPane.ERROR_MESSAGE);
                            return;
                        }
                        configurationFile = file;

                    }
                }
        );

        // file menu item to save configuration
        menuItem = new JMenuItem("Save Configuration As");
        fileMenu.add(menuItem);
        menuItem.addActionListener(
                new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        int returnVal = fc.showSaveDialog(Monitor.this);

                        if (returnVal == JFileChooser.APPROVE_OPTION) {
                            File file = fc.getSelectedFile();
                            try {
                                if (file.exists()) {
                                    int n = JOptionPane.showConfirmDialog(
                                            new JFrame(),
                                            "Overwrite existing file?",
                                            "WARNING",
                                            JOptionPane.YES_NO_OPTION);
                                    if (n == JOptionPane.NO_OPTION) return;
                                }
                                config.save(file);
                            }
                            catch (IOException ex) {
                                JOptionPane.showMessageDialog(new JFrame(),
                                                              "Cannot write to file \"" + file.getName() + "\"",
                                                              "Error",
                                                              JOptionPane.ERROR_MESSAGE);
                                return;
                            }
                            configurationFile = file;
                        }
                    }
                }
        );

        // file menu item to load configuration
        menuItem = new JMenuItem("Load Configuration");
        fileMenu.add(menuItem);
        menuItem.addActionListener(
                new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        int returnVal = fc.showDialog(Monitor.this, "Load");

                        if (returnVal == JFileChooser.APPROVE_OPTION) {
                            File file = fc.getSelectedFile();
                            try {
                                config.load(file);
                            }
                            catch (SAXException ex) {
                                JOptionPane.showMessageDialog(new JFrame(),
                                                              "Cannot load file \"" + file.getName() + "\"",
                                                              "Error",
                                                              JOptionPane.ERROR_MESSAGE);
                                return;
                            }
                            catch (IOException ex) {
                                JOptionPane.showMessageDialog(new JFrame(),
                                                              "Cannot load file \"" + file.getName() + "\"",
                                                              "Error",
                                                              JOptionPane.ERROR_MESSAGE);
                                return;
                            }
                        }
                    }
                }
        );

        // File menu item to quit.
        menuItem = new JMenuItem("Quit");
        fileMenu.add(menuItem);
        menuItem.addActionListener(
                new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        System.exit(0);
                    }
                }
        );

        // View menu to change update period of monitored ET system.
        JMenu viewMenu = new JMenu("View");
        menuBar.add(viewMenu);

        period = new WholeNumberField(defaultPeriod, 5, 1, Integer.MAX_VALUE);
        period.setAlignmentX(Component.LEFT_ALIGNMENT);
        period.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                WholeNumberField source = (WholeNumberField) e.getSource();
                source.correctValue();
                setUpdatePeriod();
            }
        }
        );
        period.addMouseListener(new MouseAdapter() {
            public void mouseExited(MouseEvent e) {
                WholeNumberField source = (WholeNumberField) e.getSource();
                source.correctValue();
                setUpdatePeriod();
            }
        }
        );

        JMenu updatePeriod = new JMenu("Update Period (sec)");
        updatePeriod.add(period);
        viewMenu.add(updatePeriod);

        // menu to load connection parameters from a specific, existing connection
        viewMenu.add(loadConnectionParametersMenu);

        // menuitem to switch JSplitPane orientation
        menuItem = new JMenuItem("Change Orientation");
        viewMenu.add(menuItem);
        menuItem.addActionListener(
                new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        if (currentMonitorKey.equals("Help")) return;
                        MonitorSingleSystem mon = (MonitorSingleSystem) monitors.get(currentMonitorKey);
                        int orient = mon.getOrientation();
                        if (orient == JSplitPane.HORIZONTAL_SPLIT) {
                            mon.setOrientation(JSplitPane.VERTICAL_SPLIT);
                        }
                        else {
                            mon.setOrientation(JSplitPane.HORIZONTAL_SPLIT);
                        }
                    }
                }
        );

        // connect menu
        JMenu connectMenu = new JMenu("Connections");
        menuBar.add(connectMenu);

        menuItem = new JMenuItem("Connect to ET System");
        connectMenu.add(menuItem);
        menuItem.addActionListener(
                new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        openFrame.setVisible(true);
                        openFrame.setState(Frame.NORMAL);
                    }
                }
        );

        // menu to disconnect existing connections
        connectMenu.add(disconnectMenu);
    }


    private JScrollPane makeHelpPane() {
        // Put this into the tabbedPane.
        JTextArea text = new JTextArea(10, 200);
        text.setLineWrap(true);
        text.setWrapStyleWord(true);
        text.setTabSize(3);
        text.setEditable(false);
        text.setBorder(new EmptyBorder(20, 20, 20, 20));
        JScrollPane pane = new JScrollPane(text);

        // Put stuff into the text area.
        text.append(
                "CONNECTING TO AN ET SYSTEM\n" +

                        "Select the \"Connect to ET System\" option of the \"Connections\" menu. " +
                        "There are a number of options on the appearing window which must be set.\n\n" +

                        "1) ET Name\nThis is the name of the ET system (actually, its file) that you " +
                        "want to connect to. The names of several ET systems can be stored in its " +
                        "list. The \"X\" button is for removing unwanted entries.\n\n" +

                        "2) ET Location\nTo look for the named ET system on the local computer or host, " +
                        "select \"local\". To look only on another computer, select \"remote\", " +
                        "or \"anywhere\" if you don't care where the ET system is. If you know the " +
                        "name of the computer, this is the place to type it in.\n\n" +

                        "3) Find ET by\n" +
                        "There are several ways to connect to an ET system. The following list showing " +
                        "the available choices:\n\n" +

                        "\ta) broadcasting\nThis selection is generally chosen when the name of the host " +
                        "that the ET system is residing on is unknown or if the user wants to write a very " +
                        "general application with no hostnames \"hardcoded\" or input in some fashion. " +
                        "(If a specific hostname is used, a UDP packet is sent directly to that host in " +
                        "addition to a broadcast being made.)\n" +
                        "A UDP broadcast is made on all the subnet broadcast addresses listed in the " +
                        "\"Subnet Addresses\" entry. Items can be removed with the \"X\" button. " +
                        "This broadcast is sent to the port found in the \"UDP Port\" entry. " +
                        "Once an ET system receives the broadcast, it responds by sending its host name " +
                        "and the tcp port on which it is listening. This information is used " +
                        "to establish a permanent tcp connection.\n\n" +

                        "\tb) multicasting\nAs in broadcasting, this selection is generally chosen when " +
                        "the name of the host that the ET system is residing on is unknown or if the user " +
                        "wants to write a very general application with no hostnames \"hardcoded\" or input " +
                        "in some fashion. (If a specific hostname is used, a UDP packet is sent directly to " +
                        "that host on the port in the \"UDP Port\" entry in addition to a multicast " +
                        "being made.)\n" +
                        "A UDP multicast is made on all the multicast addresses " +
                        "listed in the \"Multicast Addresses\" entry. Items can be removed with the \"X\" " +
                        "button. This multicast is sent to the port found in the \"Multicast Port\" " +
                        "entry, and its \"ttl\" value can be set as well. (It defaults to \"1\" which " +
                        "should limit its scope to the local subnets.) " +
                        "Once an ET system receives the multicast, it responds by sending its host name " +
                        "and the tcp port on which it is listening. This information is used " +
                        "to establish a permanent tcp connection.\n\n" +

                        "\tc) broad & multicasting\nThis selection can simultaneously UDP broadcast " +
                        "and UDP multicast.\n\n" +

                        "\td) direct connection\nA direct, permanent tcp connection is made between the ET " +
                        "system and the user. In this case, a specific hostname must be used (not \"local\", " +
                        "\"remote\", or \"anywhere\"). The \"TCP Port\" entry is used for the port number.\n\n" +

                        "RESETTING CONNECTION PARAMETERS\n" +
                        "Reseting all connection parameters to those previously used to make an actual " +
                        "connection can be done by selecting the \"Load connection parameters\" item from " +
                        "the \"View\" menu. Simply select from the list of existing connections.\n\n\n\n" +

                        "VIEWING AN ET SYSTEM\n" +

                        "After connecting to an ET system, a tab appears with the ET system's name on it. " +
                        "By selecting this tab, the user can see all the system parameters in text form on " +
                        "the left side of the window and a visual representation on the right side. Not all " +
                        "text information is relevant for all systems. For example, the ET systems written in " +
                        "Java do not have process or mutex information available. Text information " +
                        "is divided into sections with a short explanation of each following:\n\n" +

                        "1) System - general ET system parameters\n" +
                        "\ta) Static Info - information that does NOT change\n" +
                        "\t\tHost - host system is running on, language code was written in, and unix pid\n" +
                        "\t\tPorts - the tcp, udp, and multicast port numbers\n" +
                        "\t\tEvents - total # of events, size of each, # of temporary (extra large) events\n" +
                        "\t\tMax - maximum number of stations, attachments, and processes allowed\n" +
                        "\t\tNetwork interfaces - list of host's network interfaces\n" +
                        "\t\tMulticast addreses - list of multicast addresses the system is listening on\n\n" +

                        "\tb) Dynamic Info - information that can or will change in time\n" +
                        "\t\tEvents rate - rate of events leaving GRAND_CENTRAL station\n" +
                        "\t\tEvents owned by - number of events owned by each attachment & system.\n" +
                        "\t\tIdle stations - list of stations with no attachments (receive no events)\n" +
                        "\t\tAll stations - list of all stations in proper order\n" +
                        "\t\tStations - current number of stations, attachments, and temporary events\n" +
                        "\t\tProcesses - # of non-system, unix processes with access to shared memory (Solaris)\n" +
                        "\t\tHeartbeat - value of non-Java system's counter in shared memory (changes if alive)\n" +
                        "\t\tLocked Mutexes - on non-Java systems, locked pthread mutexes.\n\n" +

                        "2) Stations - stations are listed by name under this heading\n" +
                        "\ta) Configuration - parameters which define station behavior\n" +
                        "\t\t- active or idle,  blocking or nonblocking,  prescale & cue values\n" +
                        "\t\t- single user, multiple users, or the exact number of allowed users\n" +
                        "\t\t- events restored to station's input, output, or to GRAND_CENTRAL station\n" +
                        "\t\t- select all events, those matching default condition, or matching user condition\n" +
                        "\t\t- values of integers in selection array\n" +
                        "\t\t- class or library & function of user's matching condition\n\n" +

                        "\tb) Statistics - \n" +
                        "\t\t- total number of attachments and their id numbers\n" +
                        "\t\t- current # of events in input list, total # put in input, # tried to put in input\n" +
                        "\t\t- current # of events in output list, total # put in output list\n\n" +

                        "3) Processes - on Solaris, local unix processes with attachments are listed by id #\n" +
                        "\t- total # of attachments, list of attachments' ids, unix pid, current heartbeat value\n\n" +

                        "4) Attachments - attachments are listed by their id numbers\n" +
                        "\t- name of station attached to, host attachment is running on\n" +
                        "\t- is attachment blocked waiting to read events?\n" +
                        "\t- has attachment been told to quit reading events and return?\n" +
                        "\t- unix pid and process id (non-Java)\n" +
                        "\t- # events currently owned, total #: newly made, gotten, put, and dumped\n\n\n\n" +

                        "SETTING AN UPDATE PERIOD\n" +
                        "Each ET system has its information updated at a regular period which can be set " +
                        "by selecting the \"View\" menu item and typing in the period value.\n\n\n\n" +

                        "DISCONNECTING AN ET SYSTEM\n" +
                        "Each ET system can be removed from the monitor by selecting the \"Connections\" " +
                        "menu item followed by selecting the \"Disconnect\" item, and then selecting the " +
                        "system to be removed.\n\n\n\n" +

                        "CONFIGURATION FILES\n" +
                        "Configuration files can be created, saved, and loaded through the \"File\" menu item. " +
                        "The configuration files are in XML format and store all current connections and the" +
                        "current state of the application." +
                        "The main window's size and placement, is recorded in the configuration file. These particular " +
                        "parameters, however, will only be set in the application if the configuration file is " +
                        "given on the command line (-f or -file). Once the monitor is up and running, loading a " +
                        "configuration file simply adds any additional ET system connections listed there as well " +
                        "as adding items to the \"ET Name\" or \"ET Location\" lists."


        );
        return pane;
    }


    private void makeEtOpenWindow() {
        // widget sizes & spacings
        int edge1 = 10, edge2 = 5, horSpace = 10;

        // Several combo boxes use this to filter input.
        ActionListener al = new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                JComboBox jcb = (JComboBox) e.getSource();
                String listItem;
                String selectedItem = (String) jcb.getSelectedItem();
                int numItems = jcb.getItemCount();
                boolean addNewItem = true;

                if (selectedItem == null || selectedItem.equals("")) {
                    addNewItem = false;
                }
                else if (numItems == 0) {
                    addNewItem = true;
                }
                else {
                    for (int i = 0; i < numItems; i++) {
                        listItem = (String) jcb.getItemAt(i);
                        if (listItem.equals(selectedItem)) {
                            addNewItem = false;
                            break;
                        }
                    }
                }

                if (addNewItem) {
                    jcb.addItem(selectedItem);
                }
            }
        };

        //-------------------------------
        // setting ET name
        //-------------------------------
        TitledBorder border1 = new TitledBorder(new EmptyBorder(0, 0, 0, 0),
                                                "ET Name",
                                                TitledBorder.LEFT,
                                                TitledBorder.TOP);
        JPanel p1 = new JPanel();
        p1.setLayout(new BorderLayout());
        p1.setBorder(border1);

        etName = new JComboBox<String>();
        etName.setEditable(true);
        etName.addActionListener(al);
        // Set editable comboBox colors
        etName.getEditor().getEditorComponent().setForeground(entryColor);

        // button for ET name removal
        final JButton removeName = new JButton("X");
        removeName.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                int index = etName.getSelectedIndex();
                if (index > -1) {
                    etName.removeItemAt(index);
                }
            }
        });

        p1.add(etName, BorderLayout.CENTER);
        p1.add(removeName, BorderLayout.EAST);

        //-------------------------------
        // setting ET location
        //-------------------------------
        TitledBorder border2 = new TitledBorder(new EmptyBorder(0, 0, 0, 0),
                                                "ET Location",
                                                TitledBorder.LEFT,
                                                TitledBorder.TOP);
        JPanel p2 = new JPanel();
        p2.setLayout(new BorderLayout());
        p2.setBorder(border2);

        hostname = new JComboBox<String>(new String[]{"anywhere", "local", "remote"});
        hostname.setEditable(true);
        hostname.addActionListener(al);
        hostname.getEditor().getEditorComponent().setForeground(entryColor);

        // button for ET name removal
        final JButton removeHost = new JButton("X");
        removeHost.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                int index = hostname.getSelectedIndex();
                if (index > 2) {
                    hostname.removeItemAt(index);
                }
            }
        });

        p2.add(hostname, BorderLayout.CENTER);
        p2.add(removeHost, BorderLayout.EAST);

        // panel to hold panels 1 & 2
        JPanel p1and2 = new JPanel();
        p1and2.setLayout(new GridLayout(2, 1, 0, 10));
        p1and2.add(p1);
        p1and2.add(p2);

        //-------------------------------
        // broadcast addresses
        //-------------------------------
        // AddressJList is also a JPanel
        bAddress = new AddressJList();
        bAddress.setTextColor(entryColor);

        // Border for broadcast list box
        TitledBorder border3 = new TitledBorder(new EtchedBorder(),
                                                "Subnet Addresses",
                                                TitledBorder.LEFT,
                                                TitledBorder.ABOVE_TOP);
        bAddress.setBorder(border3);

        //-------------------------------
        // multicast addresses
        //-------------------------------
        mAddress = new AddressJList(true);
        mAddress.setTextColor(entryColor);

        // panel for multicast combo box & button
        TitledBorder border4 = new TitledBorder(new EtchedBorder(),
                                                "Multicast Addresses",
                                                TitledBorder.LEFT,
                                                TitledBorder.ABOVE_TOP);
        mAddress.setBorder(border4);
        mAddress.addAddress(EtConstants.multicastAddr);

        //-------------------------------
        // port entries & ttl labels
        //-------------------------------
        JLabel l0 = new JLabel("UDP Port: ", JLabel.RIGHT);
        JLabel l1 = new JLabel("TCP Port: ", JLabel.RIGHT);
        JLabel l2 = new JLabel("Multicast Port: ", JLabel.RIGHT);
        JLabel l3 = new JLabel("TTL Value: ", JLabel.RIGHT);

        // text input for udp/broadcast port number
        udpPort = new WholeNumberField(EtConstants.udpPort, 8, 1024, 65535);
        udpPort.setForeground(entryColor);
        // make sure there's a valid value entered
        udpPort.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                WholeNumberField source = (WholeNumberField) e.getSource();
                source.correctValue();
            }
        }
        );
        udpPort.addFocusListener(new FocusAdapter() {
            public void focusLost(FocusEvent e) {
                WholeNumberField source = (WholeNumberField) e.getSource();
                source.correctValue();
            }
        }
        );
        udpPort.addMouseListener(new MouseAdapter() {
            public void mouseExited(MouseEvent e) {
                WholeNumberField source = (WholeNumberField) e.getSource();
                source.correctValue();
            }
        }
        );

        // text input for tcp server port number
        tcpPort = new WholeNumberField(EtConstants.serverPort, 8, 1024, 65535);
        tcpPort.setForeground(entryColor);
        tcpPort.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                WholeNumberField source = (WholeNumberField) e.getSource();
                source.correctValue();
            }
        }
        );
        tcpPort.addFocusListener(new FocusAdapter() {
            public void focusLost(FocusEvent e) {
                WholeNumberField source = (WholeNumberField) e.getSource();
                source.correctValue();
            }
        }
        );
        tcpPort.addMouseListener(new MouseAdapter() {
            public void mouseExited(MouseEvent e) {
                WholeNumberField source = (WholeNumberField) e.getSource();
                source.correctValue();
            }
        }
        );

        // text input for TTL value
        ttl = new WholeNumberField(EtConstants.multicastTTL, 6, 0, 255);
        ttl.setForeground(entryColor);
        ttl.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                WholeNumberField source = (WholeNumberField) e.getSource();
                source.correctValue();
            }
        }
        );
        ttl.addFocusListener(new FocusAdapter() {
            public void focusLost(FocusEvent e) {
                WholeNumberField source = (WholeNumberField) e.getSource();
                source.correctValue();
            }
        }
        );
        ttl.addMouseListener(new MouseAdapter() {
            public void mouseExited(MouseEvent e) {
                WholeNumberField source = (WholeNumberField) e.getSource();
                source.correctValue();
            }
        }
        );

        //-------------------------------
        // panels for ports & ttl
        //-------------------------------
        JPanel portsLeft = new JPanel();
        portsLeft.setLayout(new GridLayout(2, 2, horSpace, 0));
        portsLeft.setBorder(new EmptyBorder(edge2, edge2, 0, edge2));

        JPanel portsRight = new JPanel();
        portsRight.setLayout(new GridLayout(2, 2, horSpace, 0));
        portsRight.setBorder(new EmptyBorder(edge2, edge2, 0, edge2));

        // add widgets to parent panels
        portsRight.add(new JLabel(""));
        portsRight.add(new JLabel(""));
        portsRight.add(l3);
        portsRight.add(ttl);

        portsLeft.add(l0);
        portsLeft.add(udpPort);
        portsLeft.add(l1);
        portsLeft.add(tcpPort);


        //-------------------------------
        // comboBox for connection method (direct, multicast, broadcast)
        //-------------------------------

        // Only way to set foreground color for non-editable ComboBox
        UIManager.put("ComboBox.foreground", entryColor);

        cast = new JComboBox<String>(new String[]{"broadcasting",
                "multicasting",
                "broad & multicasting",
                "direct connection"
        });
        cast.addActionListener(
                new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        JComboBox jcb = (JComboBox) e.getSource();
                        String selecteditem = (String) jcb.getSelectedItem();

                        if (selecteditem.equals("broadcasting")) {
                            broadcasting = true;
                            multicasting = false;
                        }
                        else if (selecteditem.equals("multicasting")) {
                            broadcasting = false;
                            multicasting = true;
                        }
                        else if (selecteditem.equals("broad & multicasting")) {
                            broadcasting = true;
                            multicasting = true;
                        }
                        // direct connection
                        else {
                            broadcasting = false;
                            multicasting = false;
                        }
                    }
                }
        );

        // default is broadcasting
        cast.setSelectedIndex(0);
        int castEdge = edge1 + edge2;
        cast.setBorder(new EmptyBorder(castEdge, castEdge, 0, castEdge));
        cast.setEditable(true);
        // Can only set color if editable
        cast.getEditor().getEditorComponent().setForeground(entryColor);
        cast.setEditable(false);

        //-------------------------------
        // button for using all local subnets
        //-------------------------------
        allBroadcastAddrsBox = new JCheckBox("Broadcast on all local subnets");
        allBroadcastAddrsBox.setBorder(new EmptyBorder(castEdge, castEdge, 0, castEdge));
        allBroadcastAddrsBox.addActionListener(
                new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        JCheckBox jcb = (JCheckBox) e.getSource();

                        if (jcb.isSelected()) {
                            broadcastAddresses = EtUtils.getAllBroadcastAddresses();
                            bAddress.addAddresses(broadcastAddresses);
                        }
                    }
                }
        );
        // Fill comboBox with subnet addresses to start with
        allBroadcastAddrsBox.doClick();

        //-------------------------------
        // buttons to connect to ET system or not
        //-------------------------------
        JPanel buttonPanel = new JPanel();
        buttonPanel.setLayout(new GridLayout(1, 2, 10, 0));

        // button for connecting to ET system
        connect = new JButton("Connect");
        connect.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                EtSystemOpenConfig config = getEtSystemConfig();
                if (config == null) return;
                addEtSystem(config);
            }
        });

        // button for dismissing window
        JButton dismiss = new JButton("Dismiss");
        dismiss.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                openFrame.setVisible(false);
            }
        });

        buttonPanel.add(connect);
        buttonPanel.add(dismiss);


        //-------------------------------
        // panels for ports, ttl, & addresses
        //-------------------------------
        JPanel allLeft = new JPanel();
        allLeft.setLayout(new BorderLayout());
        allLeft.setBorder(new EmptyBorder(edge2, edge2, edge2, edge2));
        allLeft.add(bAddress, BorderLayout.NORTH);
        allLeft.add(portsLeft, BorderLayout.SOUTH);

        JPanel allRight = new JPanel();
        allRight.setLayout(new BorderLayout());
        allRight.setBorder(new EmptyBorder(edge2, edge2, edge2, edge2));
        allRight.add(mAddress, BorderLayout.NORTH);
        allRight.add(portsRight, BorderLayout.SOUTH);

        JPanel portsAddrs = new JPanel();
        portsAddrs.setLayout(new GridLayout(1,2));
        portsAddrs.setBorder(new EmptyBorder(edge2, edge2, edge2, edge2));
        portsAddrs.add(allLeft);
        portsAddrs.add(allRight);


        //-------------------------------
        // panel to hold to "Find ET By" stuff
        //-------------------------------
        TitledBorder border5 = new TitledBorder(new EtchedBorder(),
                                                "Find ET by",
                                                TitledBorder.LEFT,
                                                TitledBorder.ABOVE_TOP);
        JPanel findByPanel = new JPanel();
        findByPanel.setLayout(new BorderLayout());
        findByPanel.setBorder(border5);
        findByPanel.add(cast, BorderLayout.NORTH);
        findByPanel.add(allBroadcastAddrsBox, BorderLayout.CENTER);
        findByPanel.add(portsAddrs, BorderLayout.SOUTH);


        //-------------------------------
        // pack everything into one main panel
        //-------------------------------
        JPanel openPanel = new JPanel();
        openPanel.setLayout(new BorderLayout(0, 10));
        openPanel.setBorder(new EmptyBorder(edge1, edge1, edge1, edge1));
        openPanel.add(p1and2, BorderLayout.NORTH);
        openPanel.add(findByPanel, BorderLayout.CENTER);
        openPanel.add(buttonPanel, BorderLayout.SOUTH);

        openFrame.getContentPane().add(openPanel);
        openFrame.pack();
    }


    /**
     * Do everything to add another monitored ET system to this application.
     */
    private void addEtSystem(final EtSystemOpenConfig config) {
        ConnectionThread t = new ConnectionThread(Monitor.this, config);
        t.start();
    }


    /**
     * Do everything to add another monitored ET system to this application.
     */
    public void addEtSystem(final EtSystemOpenConfig config, int updatePeriod,
                            int dividerLocation, int orientation, Color[] colors) {
        ConnectionThread t = new ConnectionThread(Monitor.this, config,
                                                  updatePeriod,
                                                  dividerLocation,
                                                  orientation,
                                                  colors);
        t.start();
    }


    /**
     * Gather data about which ET system and how to connect to it.
     */
    private EtSystemOpenConfig getEtSystemConfig() {

        try {
            boolean specifingHostname = false;
            EtSystemOpenConfig config = null;

            // Get ET system name.
            String etSystem = (String) etName.getSelectedItem();
            // Get host name.
            String host = (String) hostname.getSelectedItem();
            // Find out how we're connecting with the ET system.
            String howToConnect = (String) cast.getSelectedItem();

            if (host.equals("local")) {
                host = EtConstants.hostLocal;
                specifingHostname = true;
            }
            else if (host.equals("remote")) {
                host = EtConstants.hostRemote;
            }
            else if (host.equals("anywhere")) {
                host = EtConstants.hostAnywhere;
            }
            else {
                specifingHostname = true;
            }

            // Find broadcast addresses in JList
            List<String> bAddresses = null;
            if (broadcasting) {
                bAddresses = bAddress.getAddresses();
            }

            // Find multicast addresses in JList
            List<String> mAddresses = null;
            if (multicasting) {
                mAddresses = mAddress.getAddresses();
            }

            // Create the config object for opening ET system
            if (howToConnect.equals("broadcasting")) {
                // If all local subnets are used ...
                if (allBroadcastAddrsBox.isSelected()) {
                    config = new EtSystemOpenConfig(etSystem, udpPort.getValue(), host);
                }
                else {
                    config = new EtSystemOpenConfig(etSystem, host,
                                   bAddresses, null, false, EtConstants.broadcast,
                                   EtConstants.serverPort, udpPort.getValue(),
                                   EtConstants.multicastTTL,
                                   EtConstants.policyFirst);
                }
            }
            else if (howToConnect.equals("multicasting")) {
                // Use the multicast specific constructor allowing one to set the udp
                // port. This is significant since going local or to a specific host,
                // a direct udp packet is sent to that host on the udp port
                // (as well as the specified multicast).
                config = new EtSystemOpenConfig(etSystem, host,
                                                mAddresses,
                                                udpPort.getValue(),
                                                ttl.getValue());
            }
            else if (howToConnect.equals("broad & multicasting")) {
                config = new EtSystemOpenConfig(etSystem, host,
                                                bAddresses,  mAddresses, false,
                                                EtConstants.broadAndMulticast,
                                                tcpPort.getValue(), udpPort.getValue(),
                                                ttl.getValue(),
                                                EtConstants.policyError);
            }
            else if (howToConnect.equals("direct connection")) {
                // Since we've making a direct connection, a host name
                // (not remote, or anywhere) must be specified. The selection
                // of "local" can be easily resolved into an actual host name.
                if (!specifingHostname) {
                    throw new EtException("Specify a host's name (not remote, or anywhere) to make a direct connection.");
                }
                config = new EtSystemOpenConfig(etSystem, host, tcpPort.getValue());
            }

            return config;
        }
        catch (EtException ex) {
            JOptionPane.showMessageDialog(new JFrame(),
                                          ex.getMessage(),
                                          "Error",
                                          JOptionPane.ERROR_MESSAGE);
        }

        return null;
    }


    /**
     * Make a connection to an ET system & record it.
     */
    public EtSystem makeConnection(final EtSystemOpenConfig config) {
        if (config == null) {
            return null;
        }

        // Make a connection. Use EtSystemOpen object directly here
        // instead of EtSystem object so we can see exactly who
        // responded to a broad/multicast if there were multiple
        // responders.
        config.setConnectRemotely(true);  // forget loading JNI lib
        EtSystemOpen open = new EtSystemOpen(config);

        // Change cursor & disable button for waiting.
        openFrame.setCursor(Cursor.getPredefinedCursor(Cursor.WAIT_CURSOR));
        connect.setEnabled(false);

        try {
            open.connect();
        }
        catch (UnknownHostException ex) {
            JOptionPane.showMessageDialog(new JFrame(),
                                          config.getHost() + " is an unknown host",
                                          "Error",
                                          JOptionPane.ERROR_MESSAGE);
            openFrame.setCursor(Cursor.getDefaultCursor());
            connect.setEnabled(true);
            return null;
        }
        catch (IOException ex) {
            JOptionPane.showMessageDialog(new JFrame(),
                                          "Communication problems with " +
                                                  config.getEtName() + " on " +
                                                  config.getHost() + ":\n" + ex.getMessage(),
                                          "Error",
                                          JOptionPane.ERROR_MESSAGE);
            openFrame.setCursor(Cursor.getDefaultCursor());
            connect.setEnabled(true);
            return null;
        }
        catch (EtTooManyException ex) {
            // This can only happen if specifying "anywhere"
            // or "remote" for the host name.

            String host = null;
            String[] hosts = open.getAllHosts();
            int port = 0;
            int[]    ports = open.getAllPorts();

            if (hosts.length > 1) {
                host = (String) JOptionPane.showInputDialog(
                        new JFrame(),
                        "Choose the ET system responding from host:",
                        "ET System Choice",
                        JOptionPane.PLAIN_MESSAGE,
                        null,
                        hosts,
                        hosts[0]
                );

                if (host == null) {
                    return null;
                }

                for (int i = 0; i < hosts.length; i++) {
                    if (host.equals(hosts[i])) {
                        port = ports[i];
                    }
                }

                // now connect to specified host & port
                try {
                    config.setHost(host);
                    config.setTcpPort(port);
                    config.setNetworkContactMethod(EtConstants.direct);
                    open.connect();
                }
                catch (Exception except) {
                    JOptionPane.showMessageDialog(new JFrame(),
                                                  "Communication problems with " +
                                                          config.getEtName() + " on " +
                                                          config.getHost() + ":\n" + ex.getMessage(),
                                                  "Error",
                                                  JOptionPane.ERROR_MESSAGE);
                    openFrame.setCursor(Cursor.getDefaultCursor());
                    connect.setEnabled(true);
                    return null;
                }
            }
        }
        catch (EtException ex) {
            JOptionPane.showMessageDialog(new JFrame(),
                                          "Cannot find or connect to " + config.getEtName(),
                                          "Error",
                                          JOptionPane.ERROR_MESSAGE);
            openFrame.setCursor(Cursor.getDefaultCursor());
            connect.setEnabled(true);
            return null;
        }

        // Change cursor back to the default & enable button.
        openFrame.setCursor(Cursor.getDefaultCursor());
        connect.setEnabled(true);

        // Keep track of connections since you only want to connect
        // once to each system.
        // Create unique name & enter into Map (hash table)
        String key = new String(config.getEtName() + " (" + open.getHostAddress() + ")");

        if (connections.containsKey(key)) {
            open.disconnect();
            // pop up Dialog box
            JOptionPane.showMessageDialog(new JFrame(),
                                          "You are already connected to " +
                                                  config.getEtName() + " on " +
                                                  config.getHost(),
                                          "ERROR",
                                          JOptionPane.ERROR_MESSAGE);
            return null;
        }

        // Return a EtSystem object - create from EtSystemOpen object
        EtSystem use = null;
        try {
            use = new EtSystem(open, EtConstants.debugNone);
        }
        catch (Exception ex) {
            open.disconnect();
            JOptionPane.showMessageDialog(new JFrame(),
                                          "Communication problems with " +
                                                  config.getEtName() + " on " +
                                                  config.getHost() + ":\n" + ex.getMessage(),
                                          "Error",
                                          JOptionPane.ERROR_MESSAGE);
        }

        // Put connection into hash table
        connections.put(key, use);

        // Finally, put an item into the "Load Connection Parameters" menu
        final EtSystem useObject = use;
        final JMenuItem menuItem = new JMenuItem(key);
        loadConnectionParametersMenu.add(menuItem);

        menuItem.addActionListener(
                new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        EtSystemOpenConfig config = useObject.getConfig();

                        // Add ET system name (if not listed already).
                        addFileName(config.getEtName());
                        // Select ET system name.
                        etName.setSelectedItem(config.getEtName());
                        // Add hostname (if not listed already).
                        String host = config.getHost();
                        if (host.equals(EtConstants.hostLocal)) {
                            host = "local";
                        }
                        else if (host.equals(EtConstants.hostRemote)) {
                            host = "remote";
                        }
                        else if (host.equals(EtConstants.hostAnywhere)) {
                            host = "anywhere";
                        }
                        if (addHostname(host)) {
                            // and select it (only if it's a proper name).
                            hostname.setSelectedItem(host);
                        }

                        // select contact method
                        int method = config.getNetworkContactMethod();
                        if (method == EtConstants.broadcast) {
                            cast.setSelectedItem("broadcasting");
                            // set broadcast addresses
                            bAddress.clearAddresses();
                            bAddress.addAddresses(config.getBroadcastAddrs());
                            // broadcast port
                            udpPort.setValue(config.getUdpPort());
                        }
                        else if (method == EtConstants.multicast) {
                            cast.setSelectedItem("multicasting");
                            // set multicast addresses
                            mAddress.clearAddresses();
                            mAddress.addAddresses(config.getMulticastAddrs());
//                            // multicast port
//                            mcastPort.setValue(config.getUdpPort());
                            // broadcast port
                            udpPort.setValue(config.getUdpPort());
                            // ttl value
                            ttl.setValue(config.getTTL());
                        }
                        else if (method == EtConstants.broadAndMulticast) {
                            cast.setSelectedItem("broad & multicasting");
                            // set broadcast addresses
                            bAddress.clearAddresses();
                            bAddress.addAddresses(config.getBroadcastAddrs());
                            // set multicast addresses
                            mAddress.clearAddresses();
                            mAddress.addAddresses(config.getMulticastAddrs());
//                            // multicast port
//                            mcastPort.setValue(config.getUdpPort());
                            // broadcast port
                            udpPort.setValue(config.getUdpPort());
                            // ttl value
                            ttl.setValue(config.getTTL());
                        }
                        else if (method == EtConstants.direct) {
                            cast.setSelectedItem("direct connection");
                            // tcp port
                            tcpPort.setValue(config.getTcpPort());
                        }
                    }
                }
        );

        return use;
    }


    /**
     * Make a connection to an ET system & record it.
     */
    private void removeConnection(Monitor monitor, MonitorSingleSystem mon,
                                  String key, boolean notInIterator) {
        // Remove display for this connection.
        monitor.tabbedPane.remove(mon.getDisplayPane());
        // Remove menu item used to disconnect it.
        JMenuItem mi;
        int count = monitor.disconnectMenu.getItemCount();
        for (int j = 0; j < count; j++) {
            mi = monitor.disconnectMenu.getItem(j);
            // If we got the right menu item, delete it.
            if (mi.getText().equals(key)) {
                monitor.disconnectMenu.remove(mi);
                break;
            }
        }
        // Remove menu item used to load connection parameters.
        count = monitor.loadConnectionParametersMenu.getItemCount();
        for (int j = 0; j < count; j++) {
            mi = monitor.loadConnectionParametersMenu.getItem(j);
            // If we got the right menu item, delete it.
            if (mi.getText().equals(key)) {
                monitor.loadConnectionParametersMenu.remove(mi);
                break;
            }
        }

        // Close network connection to ET.
        mon.close();

        // Only change HashMap structures if in safe programming environment.
        if (notInIterator) {
            // Remove single system monitor from hash table.
            monitor.monitors.remove(key);
            // Remove EtSystem object from hash table.
            monitor.connections.remove(key);
        }

        // Find currently selected tab (and therefore the key).
        // Set the currentMonitorKey and update period.
        int index = monitor.tabbedPane.getSelectedIndex();
        key = monitor.tabbedPane.getTitleAt(index);
        monitor.currentMonitorKey = key;
        if (key.equals("Help")) {
            monitor.period.setValue(monitor.defaultPeriod);
        }
        else {
            mon = monitor.monitors.get(key);
            monitor.period.setValue(mon.getUpdatePeriod());
        }
        return;
    }


    /**
     * Display a new ET system connection.
     */
    public void displayEtSystem(final EtSystemOpenConfig config, final EtSystem use) {
        displayEtSystem(config, use, defaultPeriod, tabbedPane.getWidth() / 2,
                        JSplitPane.HORIZONTAL_SPLIT, null);
    }


    /**
     * Display a new ET system connection.
     */
    public void displayEtSystem(final EtSystemOpenConfig config, final EtSystem use,
                                int updatePeriod, int dividerLocation,
                                int orientation, Color[] colors) {

        // Create name used for menuitem & hashtable key
        final String key = new String(config.getEtName() + " (" + use.getHost() + ")");

        // Create monitor for single ET system
        final MonitorSingleSystem et = new MonitorSingleSystem(use, key,
                                                               tabbedPane,
                                                               updatePeriod,
                                                               dividerLocation,
                                                               orientation,
                                                               colors);

        // Add monitor to hash table for access by other methods - but
        // only if the HashMap is not being iterated through. This can
        // be a problem since this method is called in a separate thread.
        // We solve the problem by synchronizing on "monitors".
        synchronized (monitors) {
            monitors.put(key, et);
        }

        // Create menuitem to remove ET system from monitor.
        final JMenuItem menuItem = new JMenuItem(key);
        disconnectMenu.add(menuItem);

        menuItem.addActionListener(
                new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        removeConnection(Monitor.this, et, key, true);
                    }
                }
        );

        return;
    }

}

/**
 * This class is used to start an independent
 * thread that connects to an ET system. The
 * point of doing this is to avoid doing it in
 * the Swing graphics thread (thereby blocking
 * the GUI for long periods of time).
 */

class ConnectionThread extends Thread {
    private EtSystem use;
    private final Monitor monitor;
    private final Runnable runnable;
    private final EtSystemOpenConfig config;

    public EtSystem getSystemUse() {
        return use;
    }

    public ConnectionThread(final Monitor mon, EtSystemOpenConfig con) {
        monitor = mon;
        config = con;
        runnable = new Runnable() {
            public void run() {
                monitor.displayEtSystem(config, use);
            }
        };
    }

    public ConnectionThread(final Monitor mon, EtSystemOpenConfig con,
                            final int updatePeriod, final int dividerLocation,
                            final int orientation, final Color[] colors) {
        monitor = mon;
        config = con;
        runnable = new Runnable() {
            public void run() {
                monitor.displayEtSystem(config, use, updatePeriod,
                                        dividerLocation, orientation, colors);
            }
        };
    }

    public void run() {
        // Make a connection to the ET system.
        use = monitor.makeConnection(config);
        if (use == null) return;
        // Run displayEtSystem (graphics stuff) in swing thread.
        SwingUtilities.invokeLater(runnable);
    }
}





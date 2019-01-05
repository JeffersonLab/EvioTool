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

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.JSplitPane;
import javax.xml.parsers.SAXParserFactory;
import javax.xml.parsers.SAXParser;

import org.jlab.coda.et.*;
import org.jlab.coda.et.exception.*;

import org.xml.sax.*;
import org.xml.sax.helpers.DefaultHandler;

/**
 * A SAX2 ContentHandler.
 */
public class MonitorConfiguration extends DefaultHandler {
    // Defaults
    private SAXParser parser;

    private StringBuffer buffer = new StringBuffer(100);
    private HashMap<String,String>  dataStorage = new HashMap<String,String>(100);
    
    private Monitor   monitor;
    private boolean   readWindowParametersOnly;
    private boolean   finishedReadingWindowParameters;
    private String    currentElement;
    private String    findMethod;
    private Point     windowLocation;
    private Dimension windowSize;
    

    /** Constructor. */
    public MonitorConfiguration(Monitor mon) {
        monitor = mon;

        try {
            SAXParserFactory factory = SAXParserFactory.newInstance();
            factory.setNamespaceAware(true);
            parser = factory.newSAXParser();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    

    public void setMonitor(Monitor mon) {monitor = mon;}
    
    // Methods for getting application window & color data from
    // window parameter reading of config file.
    public Point getWindowLocation() {return new Point(windowLocation);}
    public Dimension getWindowSize() {return new Dimension(windowSize);}

    // Methods for parsing window & color data from config file.
    public void loadWindowParameters(File file) throws IOException, SAXException {
      loadWindowParameters(file.getPath());
    }
    public void loadWindowParameters(String fileName) throws IOException, SAXException {
        File f = new File(fileName);

        readWindowParametersOnly = true;
        parser.parse(f,this);
        readWindowParametersOnly = false;
        finishedReadingWindowParameters = false;
        return;
    }
    
    // Methods for reading the rest of the config information.
    public void load(String fileName) throws IOException, SAXException {
        File f = new File(fileName);
        parser.parse(f, this);
    }
    public void load(File file) throws IOException, SAXException {
      load(file.getPath());
    }


    //=============================
    // SAX DocumentHandler methods
    //=============================
    // Start element.
    public void startElement(String uri, String local, String qname,
                             Attributes attrs) {
        // keep track of the current element
        currentElement = local;

        // Keep track of method used to find ET system.
        if (local.equals("broadcasting") ||
            local.equals("multicasting") ||
            local.equals("broadAndMulticasting") ||
            local.equals("direct")) {

            findMethod = currentElement;
        }
    }

    // Characters. This may be called more than once for each item.
    public void characters(char ch[], int start, int length) {
      if (finishedReadingWindowParameters || ch == null || length == 0) {
        return;
      }
      // put data into a buffer
      buffer.append(ch, start, length);
    }
    
    
    // End element. Note that white space is ignored when validating an element's
    // value. So, white space gets passed on to the user and must be trimmed off.
    public void endElement(String uri, String local, String qname) {
        if (finishedReadingWindowParameters) {
            return;
        }

        // If it's only a closing bracket, don't save it's data
        String elementData = buffer.toString().trim();
        if (elementData.length() > 0) {
            // put data (as string) into hash table
            dataStorage.put(currentElement, elementData);
//System.out.println("PUT: currentElement = " + currentElement + ", buffer = " + elementData);
            // erase buffer for next element
            buffer.setLength(0);
        }

        // adjust GUI parameters
        if (local.equals("graphics")) {
            // First, handle case of only reading main application's window's parameters.
            if (readWindowParametersOnly) {
                int w = Integer.parseInt(dataStorage.get("width"));
                int h = Integer.parseInt(dataStorage.get("height"));
                int x = Integer.parseInt(dataStorage.get("xPosition"));
                int y = Integer.parseInt(dataStorage.get("yPosition"));
                windowLocation = new Point(x, y);
                windowSize = new Dimension(w, h);
                finishedReadingWindowParameters = true;

                dataStorage.clear();
                return;
            }

            // optional elements
            if (dataStorage.containsKey("fileNameList")) {
                // Divide list - items separated by white space - into component parts.
                StringTokenizer tok = new StringTokenizer(dataStorage.get("fileNameList"));
                while (tok.hasMoreTokens()) {
                    monitor.addFileName(tok.nextToken());
                }
            }
            if (dataStorage.containsKey("hostList")) {
                StringTokenizer tok = new StringTokenizer(dataStorage.get("hostList"));
                while (tok.hasMoreTokens()) {
                    monitor.addHostname(tok.nextToken());
                }
            }

            dataStorage.clear();
        }

        // try to make connection to ET system
        else if (local.equals("etConnection")) {
            String etSystem = dataStorage.get("fileName");
            int period = Integer.parseInt(dataStorage.get("period"));
            int divider = Integer.parseInt(dataStorage.get("dividerPosition"));
            int orientation = JSplitPane.HORIZONTAL_SPLIT;
            if (dataStorage.get("orientation").equals("vertical")) {
                orientation = JSplitPane.VERTICAL_SPLIT;
            }

            String host;
            EtSystemOpenConfig config = null;
            int broadcastPort, multicastPort, port, ttl;

            try {
                if (dataStorage.containsKey("location")) {
                    host = dataStorage.get("location");
                    if (host.equals("local")) {
                        host = EtConstants.hostLocal;
                    }
                    else if (host.equals("remote")) {
                        host = EtConstants.hostRemote;
                    }
                    else {
                        host = EtConstants.hostAnywhere;
                    }
                }
                else {
                    host = dataStorage.get("host");
                    monitor.addHostname(host);
                }

                if (findMethod.equals("broadcasting")) {
                    broadcastPort = Integer.parseInt(dataStorage.get("broadcastPort"));
                    StringTokenizer tok = new StringTokenizer(dataStorage.get("broadcastAddressList"));
                    ArrayList<String> addrs = new ArrayList<String>();
                    while (tok.hasMoreTokens()) {
                        addrs.add(tok.nextToken());
                    }

                    config = new EtSystemOpenConfig(etSystem, broadcastPort, host, addrs);
                }
                else if (findMethod.equals("multicasting")) {
                    ttl = Integer.parseInt((dataStorage.get("ttl")).trim());
                    multicastPort = Integer.parseInt(dataStorage.get("multicastPort"));
                    StringTokenizer tok = new StringTokenizer(dataStorage.get("multicastAddressList"));
                    ArrayList<String> addrs = new ArrayList<String>();
                    while (tok.hasMoreTokens()) {
                        addrs.add(tok.nextToken());
                    }

                    config = new EtSystemOpenConfig(etSystem, host, addrs,
                                                    multicastPort, ttl);
                }
                else if (findMethod.equals("broadAndMulticasting")) {
                    ttl = Integer.parseInt(dataStorage.get("ttl"));
                    broadcastPort = Integer.parseInt(dataStorage.get("broadcastPort"));
                    multicastPort = Integer.parseInt(dataStorage.get("multicastPort"));
                    StringTokenizer tok = new StringTokenizer(dataStorage.get("broadcastAddressList"));
                    ArrayList<String> bAddrs = new ArrayList<String>();
                    while (tok.hasMoreTokens()) {
                        bAddrs.add(tok.nextToken());
                    }
                    tok = new StringTokenizer(dataStorage.get("multicastAddressList"));
                    ArrayList<String> mAddrs = new ArrayList<String>();
                    while (tok.hasMoreTokens()) {
                        mAddrs.add(tok.nextToken());
                    }
                    config = new EtSystemOpenConfig(etSystem, host, bAddrs, mAddrs, true,
                                                    EtConstants.broadAndMulticast, 0,
                                                    broadcastPort, ttl,
                                                    EtConstants.policyError);
                }
                else if (findMethod.equals("direct")) {
                    port = Integer.parseInt(dataStorage.get("tcpPort"));
                    config = new EtSystemOpenConfig(etSystem, host, port);
                }
            }
            catch (EtException ex) {
                ex.printStackTrace();
            }

            monitor.addFileName(etSystem);
            monitor.addEtSystem(config, period, divider, orientation, null);
            dataStorage.clear();
        }
    }


    // Warning.
    public void warning(SAXParseException ex) {
        System.err.println("[Warning] "+
                           getLocationString(ex)+": "+
                           ex.getMessage());
    }


    // Error.
    public void error(SAXParseException ex) throws SAXException {
        System.err.println("[Error] "+
                           getLocationString(ex)+": "+
                           ex.getMessage());
        throw ex;
    }


    // Fatal error.
    public void fatalError(SAXParseException ex) throws SAXException {
        System.err.println("[Fatal Error] "+
                           getLocationString(ex)+": "+
                           ex.getMessage());
        throw ex;
    }

    //===================================
    // End of SAX DocumentHandler methods
    //===================================
    
    

    // Returns a string of the location.
    private String getLocationString(SAXParseException ex) {
        StringBuffer str = new StringBuffer();
        String systemId  = ex.getSystemId();
        if (systemId != null) {
            int index = systemId.lastIndexOf('/');
            if (index != -1)
                systemId = systemId.substring(index + 1);
            str.append(systemId);
        }
        str.append(": line ");
        str.append(ex.getLineNumber());
        str.append(" :col ");
        str.append(ex.getColumnNumber());

        return str.toString();
    }


    // Saves data nto a proper xml format configuration file.
    public void save(File file) throws FileNotFoundException {
        String fileName = file.getPath();
        try {
            FileOutputStream fos   = new FileOutputStream(fileName);
            OutputStreamWriter osw = new OutputStreamWriter(fos, "ASCII");

            StringBuilder text = new StringBuilder(1000);

            // Configuration file is in XML format.
            text.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");
            text.append("<configuration>\n\n");
            text.append("  <graphics>\n    <width>");
            text.append(monitor.getMonitorWidth());
            text.append("</width>\n    <height>");
            text.append(monitor.getMonitorHeight());
            text.append("</height>\n    <xPosition>");
            text.append(monitor.getX());
            text.append("</xPosition>\n    <yPosition>");
            text.append(monitor.getY());
            text.append("</yPosition>\n");

            osw.write(text.toString());
            text.setLength(0);

            // Lists of host and ET names
            String[] names = monitor.getHostnames();
            if (names != null) {
                text.append("    <hostList>");
                for (String name : names) {
                    text.append("\n      ");
                    text.append(name);
                }
                text.append("\n    </hostList>\n");
            }

            names = monitor.getFileNames();
            if (names != null) {
                text.append("    <fileNameList>");
                for (String name : names) {
                    text.append("\n      ");
                    text.append(name);
                }
                text.append("\n    </fileNameList>\n");
            }
            text.append("  </graphics>\n\n");

            osw.write(text.toString());
            text.setLength(0);

            // Connections to ET systems
            String key;
            EtSystem use;
            EtSystemOpenConfig config;
            for (Map.Entry<String, EtSystem> entry : monitor.connections.entrySet()) {
                // Get object with connection info in it.
                use = entry.getValue();
                key = entry.getKey();
                config = use.getConfig();

                text.append("  <etConnection>\n    <fileName>");
                // ET name.
                text.append(config.getEtName());
                text.append("</fileName>\n");

                // Method of finding ET system.
                int method = config.getNetworkContactMethod();
                if (method == EtConstants.broadcast) {
                    text.append("    <broadcasting>\n");
                    // Location or host?
                    String host = config.getHost();
                    if (host.equals(EtConstants.hostLocal)) {
                        text.append("      <location>local</location>\n");
                    }
                    else if (host.equals(EtConstants.hostRemote)) {
                        text.append("      <location>remote</location>\n");
                    }
                    else if (host.equals(EtConstants.hostAnywhere)) {
                        text.append("      <location>anywhere</location>\n");
                    }
                    else {
                        text.append("      <host>");
                        text.append(host);
                        text.append("</host>\n");
                    }
                    // List of subnet addresses to broadcast on.
                    text.append("      <broadcastAddressList>");
                    for (String baddr : config.getBroadcastAddrs()) {
                        text.append("\n        ");
                        text.append(baddr);
                    }
                    text.append("\n      </broadcastAddressList>\n      <broadcastPort>");
                    text.append(config.getUdpPort());
                    text.append("</broadcastPort>\n    </broadcasting>\n");
                }
                else if (method == EtConstants.multicast) {
                    text.append("    <multicasting>\n");
                    String host = config.getHost();
                    if (host.equals(EtConstants.hostLocal)) {
                        text.append("      <location>local</location>\n");
                    }
                    else if (host.equals(EtConstants.hostRemote)) {
                        text.append("      <location>remote</location>\n");
                    }
                    else if (host.equals(EtConstants.hostAnywhere)) {
                        text.append("      <location>anywhere</location>\n");
                    }
                    else {
                        text.append("      <host>");
                        text.append(host);
                        text.append("</host>\n");
                    }
                    // List of multicast addresses to multicast on.
                    text.append("      <multicastAddressList>");
                    for (String maddr : config.getMulticastAddrs()) {
                        text.append("\n        ");
                        text.append(maddr);
                    }
                    text.append("\n      </multicastAddressList>\n      <multicastPort>");
                    text.append(config.getUdpPort());
                    text.append("</multicastPort>\n      <ttl>");
                    text.append(config.getTTL());
                    text.append("</ttl>\n    </multicasting>\n");
                }
                else if (method == EtConstants.broadAndMulticast) {
                    text.append("    <broadAndMulticasting>\n");
                    String host = config.getHost();
                    if (host.equals(EtConstants.hostLocal)) {
                        text.append("      <location>local</location>\n");
                    }
                    else if (host.equals(EtConstants.hostRemote)) {
                        text.append("      <location>remote</location>\n");
                    }
                    else if (host.equals(EtConstants.hostAnywhere)) {
                        text.append("      <location>anywhere</location>\n");
                    }
                    else {
                        text.append("      <host>");
                        text.append(host);
                        text.append("</host>\n");
                    }
                    // List of subnet addresses to broadcast on.
                    text.append("      <broadcastAddressList>");
                    for (String baddr : config.getBroadcastAddrs()) {
                        text.append("\n        ");
                        text.append(baddr);
                    }
                    text.append("\n      </broadcastAddressList>\n      <broadcastPort>");
                    text.append(config.getUdpPort());
                    text.append("</broadcastPort>\n");
                    // List of multicast addresses to multicast on.
                    text.append("      <multicastAddressList>");
                    for (String maddr : config.getMulticastAddrs()) {
                        text.append("\n        ");
                        text.append(maddr);
                    }
                    text.append("\n      </multicastAddressList>\n      <multicastPort>");
                    text.append(config.getUdpPort());
                    text.append("</multicastPort>\n      <ttl>");
                    text.append(config.getTTL());
                    text.append("</ttl>\n    </broadAndMulticasting>\n");
                }
                // if (method == EtConstants.direct)
                else {
                    text.append("    <direct>\n");
                    String host = config.getHost();
                    if (host.equals(EtConstants.hostLocal)) {
                        text.append("      <location>local</location>\n");
                    }
                    else {
                        text.append("      <host>");
                        text.append(host);
                        text.append("</host>\n");
                    }
                    text.append("      <tcpPort>");
                    text.append(config.getTcpPort());
                    text.append("</tcpPort>\n");
                    text.append("    </direct>\n");
                }

                // Update period & splitPane divider position & orientation
                MonitorSingleSystem singleMonitor = monitor.monitors.get(key);
                text.append("    <period>");
                text.append(singleMonitor.getUpdatePeriod());
                text.append("</period>\n    <dividerPosition>");
                text.append(singleMonitor.getDividerPosition());
                text.append("</dividerPosition>\n    <orientation>");
                if (singleMonitor.getOrientation() == JSplitPane.HORIZONTAL_SPLIT) {
                    text.append("horizontal");
                }
                else {
                    text.append("vertical");
                }
                text.append("</orientation>\n");


                text.append("  </etConnection>\n\n");
                osw.write(text.toString());
                text.setLength(0);
            }

            text.append("</configuration>\n");

            osw.write(text.toString());
            osw.close();
            fos.close();
        }
        catch (UnsupportedEncodingException ex) {}
        catch (IOException ex) {}
    }


}

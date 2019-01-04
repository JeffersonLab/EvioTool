package org.jlab.coda.jevio.graphics;

import java.awt.event.*;
import java.awt.*;
import java.io.File;
import java.io.IOException;
import java.util.Arrays;
import java.util.EventListener;

import javax.swing.*;
import javax.swing.event.EventListenerList;
import javax.swing.filechooser.FileNameExtensionFilter;
import javax.swing.border.EmptyBorder;

import org.jlab.coda.jevio.*;

/**
 * This class creates the menus used in the GUI.
 * @author Heddle
 * @author Timmer
 */
public class EventTreeMenu {

    //----------------------
    // gui stuff
    //----------------------

    /** A button for selecting "next" event. */
    JButton nextButton;

    /** A button for selecting "previous" event. */
    JButton prevButton;

	/** Menu item for exporting file to XML. */
	private JMenuItem xmlExportItem;

    /** Menu item for opening event file. */
    private JMenuItem openEventFile;

    /** Menu item setting the number of the event (from a file) to be displayed. */
    private JTextField eventNumberInput;

    /** The panel that holds the tree and all associated widgets. */
	private EventTreePanel eventTreePanel;

    /** Number of event currently being displayed. */
    private int eventIndex;

    //----------------------
    // file stuff
    //----------------------

     /** Last selected data file. */
    private String dataFilePath;

    /** Last selected dictionary file. */
    private String dictionaryFilePath;

    /** Last selected xml file to export event file into. */
    private String xmlFilePath;

    /** Filter so only files with specified extensions are seen in file viewer. */
    private FileNameExtensionFilter evioFileFilter;

    /** The reader object for the currently viewed evio file. */
    private EvioReader evioFileReader;

    //----------------------
    // dictionary stuff
    //----------------------

    /** Is the user-selected or file-embedded dictionary currently used? */
    private boolean isUserDictionary;

    /** User-selected dictionary file. */
    private INameProvider userDictionary;

    /** Dictionary embedded with opened evio file. */
    private INameProvider fileDictionary;

    /** Dictionary currently in use. */
    private INameProvider currentDictionary;


    //----------------------------
    // General function
    //----------------------------
	/**
	 * Listener list for structures (banks, segments, tagsegments) encountered while processing an event.
	 */
	private EventListenerList evioListenerList;



    /**
	 * Constructor. Holds the menus for a frame or internal frame that wants to manage a tree panel.
	 * @param eventTreePanel holds the tree and all associated the widgets.
	 */
	public EventTreeMenu(final EventTreePanel eventTreePanel) {
		this.eventTreePanel = eventTreePanel;
	}

    /**
     * Get the main event display panel.
     * @return main event display panel.
     */
    public EventTreePanel getEventTreePanel() {
        return eventTreePanel;
    }


    /**
     * Create a panel to change events in viewer.
     */
    void addEventControlPanel() {

        nextButton = new JButton("next >");

        // next event menu item
        ActionListener al_next = new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                // If we're looking at a file, there are multiple events contained in it
                if (evioFileReader != null) {
                    try {
                        EvioEvent event = evioFileReader.parseEvent(++eventIndex);
                        if (event != null) {
                            eventTreePanel.setEvent(event);
                        }
                        if (eventIndex > 1) prevButton.setEnabled(true);
                    }
                    catch (IOException e1) {
                        eventIndex--;
                        e1.printStackTrace();
                    }
                    catch (EvioException e1) {
                        eventIndex--;
                        e1.printStackTrace();
                    }
                    catch (Exception e1) {
                        e1.printStackTrace();
                    }
                }

            }
        };
        nextButton.addActionListener(al_next);

        prevButton = new JButton("< prev");
        prevButton.setEnabled(false);
        // previous event menu item
        ActionListener al_prev = new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                // If we're looking at a file, there are multiple events contained in it
                if (evioFileReader != null) {
                    try {
                        if (eventIndex > 1) {
                            EvioEvent event = evioFileReader.parseEvent(--eventIndex);
                            if (event != null) {
                                eventTreePanel.setEvent(event);
                            }
                            if (eventIndex < 2) {
                                prevButton.setEnabled(false);
                            }
                        }
                    }
                    catch (IOException e1) {
                        eventIndex++;
                        e1.printStackTrace();
                    }
                    catch (EvioException e1) {
                        eventIndex++;
                        e1.printStackTrace();
                    }
                }

            }
        };
        prevButton.addActionListener(al_prev);


        // goto event menu item
        ActionListener al_eventNumIn = new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                try {
                    String num = eventNumberInput.getText();
                    if (num != null) {
                        int eventNum = Integer.parseInt(num);

                        // Go to the specified event number
                        if ((eventNum > 0) && (eventNum <= evioFileReader.getEventCount())) {
                            eventIndex = eventNum;
                            EvioEvent event = evioFileReader.gotoEventNumber(eventIndex);
                            if (event != null) {
                                eventTreePanel.setEvent(event);
                            }
                        }
                        else {
                            eventNumberInput.setText("");
                        }
                    }
                }
                catch (NumberFormatException e1) {
                    // bad entry in widget
                    eventNumberInput.setText("");
                }
                catch (Exception e1) {
                    e1.printStackTrace();
                }
            }
        };

        JLabel label = new JLabel("Go to event # ");
        eventNumberInput = new JTextField();
        eventNumberInput.addActionListener(al_eventNumIn);

        JPanel cPanel = eventTreePanel.getEventInfoPanel().controlPanel;
        cPanel.setBorder(new EmptyBorder(5,8,5,8));
        cPanel.setLayout(new GridLayout(2,2,5,5));   // rows, cols, hgap, vgap
        cPanel.add(prevButton);
        cPanel.add(nextButton);
        cPanel.add(label);
        cPanel.add(eventNumberInput);
    }


    /**
     * Create the view menu.
     *
     * @return the view menu.
     */
    public JMenu createViewMenu() {
        final JMenu menu = new JMenu(" View ");

        // ints-viewed-as-hex menu item
        ActionListener al_hex = new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                JMenuItem item = (JMenuItem) e.getSource();
                String txt = item.getText();
                if (txt.equals("Hexidecimal")) {
                    eventTreePanel.setIntsInHex(true);
                    item.setText("Decimal");
                }
                else {
                    eventTreePanel.setIntsInHex(false);
                    item.setText("Hexidecimal");
                }
                eventTreePanel.refreshDisplay();
            }
        };

        JMenuItem hexItem = new JMenuItem("Hexidecimal");
        hexItem.addActionListener(al_hex);
        hexItem.setEnabled(true);
        menu.add(hexItem);


        // switch dictionary menu item
        ActionListener al_switchDict = new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                if (switchDictionary()) {
                    eventTreePanel.refreshDisplay();
                }
            }
        };

        JMenuItem switchDictItem = new JMenuItem("Switch dictionary");
        switchDictItem.addActionListener(al_switchDict);
        switchDictItem.setEnabled(true);
        menu.add(switchDictItem);

        return menu;
    }


	/**
	 * Create the file menu.
	 *
	 * @return the file menu.
	 */
	public JMenu createFileMenu() {
		JMenu menu = new JMenu(" File ");

		// open event file menu item
		ActionListener al_oef = new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				doOpenEventFile();
			}
		};
		openEventFile = new JMenuItem("Open Event File...");
		openEventFile.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_O, ActionEvent.CTRL_MASK));
		openEventFile.addActionListener(al_oef);
		menu.add(openEventFile);

		// open dictionary menu item
		ActionListener al_odf = new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				doOpenDictionary();
			}
		};
		JMenuItem df_item = new JMenuItem("Open Dictionary...");
		df_item.addActionListener(al_odf);
		menu.add(df_item);

        // separator
		menu.addSeparator();

		// open dictionary menu item
		ActionListener al_xml = new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				exportToXML();
			}
		};
		xmlExportItem = new JMenuItem("Export File to XML...");
		xmlExportItem.addActionListener(al_xml);
		xmlExportItem.setEnabled(false);
		menu.add(xmlExportItem);
		menu.addSeparator();

		// Quit menu item
		ActionListener al_exit = new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				System.exit(0);
			}
		};
		JMenuItem exit_item = new JMenuItem("Quit");
		exit_item.addActionListener(al_exit);
		menu.add(exit_item);

		return menu;
	}


    /**
     * Select and open an event file.
     */
    private void doOpenEventFile() {
        EvioReader eFile    = evioFileReader;
        EvioReader evioFile = openEventFile();
        // handle cancel button properly
        if (eFile == evioFile) {
            return;
        }
        nextButton.setEnabled(evioFile != null);
//        prevButton.setEnabled(evioFile != null);
        prevButton.setEnabled(false);
        xmlExportItem.setEnabled(evioFile != null);
        eventTreePanel.setEvent(null);
        // automatically go to the first event
        nextButton.doClick();
    }

    /**
     * Convenience method to open a file programmatically.
     * @param file the file to open
     */
    public void manualOpenEventFile(File file) {
        EvioReader eFile    = evioFileReader;
        EvioReader evioFile = openEventFile(file);
        // handle cancel button properly
        if (eFile == evioFile) {
            return;
        }
        nextButton.setEnabled(evioFile != null);
//        prevButton.setEnabled(evioFile != null);
        prevButton.setEnabled(false);
        xmlExportItem.setEnabled(evioFile != null);
        eventTreePanel.setEvent(null);
    }

	/**
	 * Select and open a dictionary.
	 */
	private void doOpenDictionary() {
        openDictionary();
	}


    /**
     * Attempt to switch dictionaries. If using dictionary from file,
     * switch to user-selected dictionary if it exists, or vice versa.
     *
     * @return <code>true</code> if dictionary was switched, else <code>false</code>
     */
    public boolean switchDictionary() {
        // if switching from user-selected dictionary file to embedded file ..
        if (isUserDictionary) {
            if (fileDictionary != null) {
                currentDictionary = fileDictionary;
                NameProvider.setProvider(currentDictionary);
                eventTreePanel.refreshDescription();
                eventTreePanel.getEventInfoPanel().setDictionary("from file");
                isUserDictionary = false;
                return true;
            }
        }
        // if switching from embedded file to user-selected dictionary file ..
        else {
            if (userDictionary != null) {
                currentDictionary = userDictionary;
                NameProvider.setProvider(currentDictionary);
                eventTreePanel.refreshDescription();
                eventTreePanel.getEventInfoPanel().setDictionary(dictionaryFilePath);
                isUserDictionary = true;
                return true;
            }
        }
        return false;
    }


    /**
     * Select a file and then write into it the current event file in xml format.
     */
    public void exportToXML() {
        eventTreePanel.getProgressBar().setValue(0);
        JFileChooser chooser = new JFileChooser(xmlFilePath);
        chooser.setSelectedFile(null);
        FileNameExtensionFilter filter = new FileNameExtensionFilter("XML Evio Files", "xml");
        chooser.setFileFilter(filter);

        int returnVal = chooser.showSaveDialog(eventTreePanel);
        if (returnVal == JFileChooser.APPROVE_OPTION) {

            // remember which file was chosen
            File selectedFile = chooser.getSelectedFile();
            xmlFilePath = selectedFile.getAbsolutePath();

            if (selectedFile.exists()) {
                int answer = JOptionPane.showConfirmDialog(null, selectedFile.getPath()
                        + "  already exists. Do you want to overwrite it?", "Overwite Existing File?",
                        JOptionPane.YES_NO_OPTION);

                if (answer != JFileChooser.APPROVE_OPTION) {
                    return;
                }
            }

            // keep track of progress writing file
            final IEvioProgressListener progressListener = new IEvioProgressListener() {
                public void completed(int num, int total) {
                    int percentDone = (int) ((100.0 * num) / total);
                    eventTreePanel.getProgressBar().setValue(percentDone);
                    eventTreePanel.getProgressBar().repaint();
                }
            };


            // do the xml processing in a separate thread.
            Runnable runner = new Runnable() {
                public void run() {
                    try {
                        evioFileReader.toXMLFile(xmlFilePath, progressListener);
                    }
                    catch (Exception e) {e.printStackTrace();}
                    eventTreePanel.getProgressBar().setValue(0);
                    JOptionPane.showMessageDialog(eventTreePanel, "XML Writing has completed.", "Done",
                            JOptionPane.INFORMATION_MESSAGE);
                }
            };
            (new Thread(runner)).start();

        }
    }


    /**
     * Add a file extension for viewing evio format files in file chooser.
     * @param extension file extension to add
     */
    public void addEventFileExtension(String extension) {
        if (evioFileFilter == null) {
            evioFileFilter = new FileNameExtensionFilter("EVIO Event Files", "ev", "evt", "evio", extension);
        }
        else {
            String[] exts = evioFileFilter.getExtensions();
            String[] newExts = Arrays.copyOf(exts, exts.length + 1);
            newExts[exts.length] = extension;
            evioFileFilter = new FileNameExtensionFilter("EVIO Event Files", newExts);
        }
    }


    /**
     * Set all file extensions for viewing evio format files in file chooser.
     * @param extensions all file extensions
     */
    public void setEventFileExtensions(String[] extensions) {
        evioFileFilter = new FileNameExtensionFilter("EVIO Event Files", extensions);
    }


    /**
     * Select and open an event file.
     *
     * @return the opened file reader, or <code>null</code>
     */
    public EvioReader openEventFile() {
        eventTreePanel.getEventInfoPanel().setEventNumber(0);

        JFileChooser chooser = new JFileChooser(dataFilePath);
        chooser.setSelectedFile(null);
        if (evioFileFilter == null) {
            evioFileFilter = new FileNameExtensionFilter("EVIO Event Files", "ev", "evt", "evio");
        }
        chooser.setFileFilter(evioFileFilter);
        int returnVal = chooser.showOpenDialog(eventTreePanel);
        if (returnVal == JFileChooser.APPROVE_OPTION) {
            eventTreePanel.getHeaderPanel().setHeader(null);

            // remember which file was chosen
            File selectedFile = chooser.getSelectedFile();
            dataFilePath = selectedFile.getAbsolutePath();

            // set the text field
            eventTreePanel.getEventInfoPanel().setSource(dataFilePath);

            try {
                if (evioFileReader != null) {
                    evioFileReader.close();
                    eventTreePanel.getEventInfoPanel().setNumberOfEvents(0);
                }

                evioFileReader = new EvioReader(selectedFile);
                eventTreePanel.getEventInfoPanel().setNumberOfEvents(evioFileReader.getEventCount());
                if (evioFileReader.hasDictionaryXML()) {
                    fileDictionary = NameProviderFactory.createNameProvider(evioFileReader.getDictionaryXML());
                    currentDictionary = fileDictionary;
                    NameProvider.setProvider(currentDictionary);
                    isUserDictionary = false;
                    eventTreePanel.getEventInfoPanel().setDictionary("from file");
                }
                eventIndex = 0;
            }
            catch (EvioException e) {
                evioFileReader = null;
                e.printStackTrace();
            }
            catch (IOException e) {
                evioFileReader = null;
                e.printStackTrace();
            }
        }
        connectEvioListeners();     // Connect Listeners to the parser.
        
        return evioFileReader;
    }


    /**
     * Open an event file using a given file.
     *
     * @param file the file to use, i.e., an event file
     * @return the opened file reader, or <code>null</code>
     */
    public EvioReader openEventFile(File file) {
        eventTreePanel.getEventInfoPanel().setEventNumber(0);

        eventTreePanel.getHeaderPanel().setHeader(null);

        // remember which file was chosen
        dataFilePath = file.getAbsolutePath();

        // set the text field
        eventTreePanel.getEventInfoPanel().setSource(dataFilePath);

        try {
            if (evioFileReader != null) {
                evioFileReader.close();
                eventTreePanel.getEventInfoPanel().setNumberOfEvents(0);
            }

            evioFileReader = new EvioReader(file);
            eventTreePanel.getEventInfoPanel().setNumberOfEvents(evioFileReader.getEventCount());
            if (evioFileReader.hasDictionaryXML()) {
                fileDictionary = NameProviderFactory.createNameProvider(evioFileReader.getDictionaryXML());
                currentDictionary = fileDictionary;
                NameProvider.setProvider(currentDictionary);
                isUserDictionary = false;
                eventTreePanel.getEventInfoPanel().setDictionary("from file");
            }
            eventIndex = 0;
        }
        catch (EvioException e) {
            evioFileReader = null;
            e.printStackTrace();
        }
        catch (IOException e) {
            evioFileReader = null;
            e.printStackTrace();
        }
        connectEvioListeners();     // Connect Listeners to the parser.
        return evioFileReader;
    }


    /**
     * Get the EvioReader object so the file/buffer can be read.
     * @return  EvioReader object
     */
    public EvioReader getEvioFileReader() {
        return evioFileReader;
    }


    /**
     * Set the default directory in which to look for event files.
     * @param defaultDataDir default directory in which to look for event files
     */
    public void setDefaultDataDir(String defaultDataDir) {
        dataFilePath = defaultDataDir;
    }


    /**
     * Select and open a dictionary.
     * @return <code>true</code> if file was opened and dictionary loaded.
     */
    public boolean openDictionary() {
        JFileChooser chooser = new JFileChooser(dictionaryFilePath);
         chooser.setSelectedFile(null);
        FileNameExtensionFilter filter = new FileNameExtensionFilter("Dictionary Files", "xml", "dict", "txt");
        chooser.setFileFilter(filter);
        int returnVal = chooser.showOpenDialog(eventTreePanel);
        if (returnVal == JFileChooser.APPROVE_OPTION) {
            File selectedFile = chooser.getSelectedFile();
            // create a dictionary, set the global name provider
            userDictionary = NameProviderFactory.createNameProvider(selectedFile);
            currentDictionary = userDictionary;
            NameProvider.setProvider(currentDictionary);
            isUserDictionary = true;
            // remember which file was chosen
            dictionaryFilePath = selectedFile.getAbsolutePath();
            // set the text field
            eventTreePanel.getEventInfoPanel().setDictionary(dictionaryFilePath);
            eventTreePanel.refreshDescription();
            return true;
        }
        return false;
    }


    /**
     * Select and open a dictionary.
     * @param file file to open
     */
    public void openDictionaryFile(File file) {
        if (file != null) {
            userDictionary = NameProviderFactory.createNameProvider(file);
            currentDictionary = userDictionary;
            NameProvider.setProvider(currentDictionary);
            isUserDictionary = true;
            // remember which file was chosen
            dictionaryFilePath = file.getAbsolutePath();
            // set the text field
            eventTreePanel.getEventInfoPanel().setDictionary(dictionaryFilePath);
            eventTreePanel.refreshDescription();
        }
    }

	/**
	 * Add an Evio listener. Evio listeners listen for structures encountered when an event is being parsed.
	 * The listeners are passed to the EventParser once a file is opened.
	 * @param listener The Evio listener to add.
	 */
	public void addEvioListener(IEvioListener listener) {

		if (listener == null) {
			return;
		}

		if (evioListenerList == null) {
			evioListenerList = new EventListenerList();
		}

		evioListenerList.add(IEvioListener.class, listener);
	}
	/**
	 * Connect the listeners in the evioListenerList to the EventParser
	 */
	private void connectEvioListeners(){
		
		if (evioListenerList == null) {
			return;
		}

		EventParser parser = getEvioFileReader().getParser();
		
		EventListener listeners[] = evioListenerList.getListeners(IEvioListener.class);

		for (int i = 0; i < listeners.length; i++) {
			parser.addEvioListener((IEvioListener)listeners[i]);
		}		
	}

}

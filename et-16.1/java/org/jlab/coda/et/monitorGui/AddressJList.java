/*----------------------------------------------------------------------------*
 *  Copyright (c) 2015        Jefferson Science Associates,                   *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, MS-12B3       *
 *             Phone: (757) 269-5130             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-6248             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*/

package org.jlab.coda.et.monitorGui;

import org.jlab.coda.et.EtUtils;

import javax.swing.*;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Enumeration;
import java.util.List;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

/**
 * This class creates a panel for entering and
 * viewing dot-decimal formatted IP addresses.
 *
 * @author timmer (5/14/15).
 */
public class AddressJList extends JPanel implements ListSelectionListener {
    /** Widget for displaying address list. */
    private JList<String> list;
    /** Data model for list. */
    private DefaultListModel<String> listModel;

    /** Label of "Add" button. */
    private static final String addString = "Add";
    /** Label of "Delete" button. */
    private static final String deleteString = "Delete";
    /** Delete button. */
    private JButton deleteButton;
    /** Widget for inputting new address. */
    private JTextField netAddress;
    /** Does this object contain multicast addresses? */
    private boolean forMulticasting;



    /** Constructor for regular IP addresses. */
    public AddressJList() {
        this(false);
    }


    /**
     * Constructor.
     * @param forMulticasting {@code true} if for multicast addresses
     */
    public AddressJList(boolean forMulticasting) {
        super(new BorderLayout(0,10));

        this.forMulticasting = forMulticasting;

        // Create the list and put it in a scroll pane.
        listModel = new DefaultListModel<String>();
        list = new JList<String>(listModel);
        list.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        list.setSelectedIndex(0);
        list.addListSelectionListener(this);
        list.setVisibleRowCount(4);
        JScrollPane listScrollPane = new JScrollPane(list);

        JButton addButton = new JButton(addString);
        AddListener addListener = new AddListener(addButton);
        addButton.setActionCommand(addString);
        addButton.addActionListener(addListener);
        addButton.setEnabled(false);

        deleteButton = new JButton(deleteString);
        deleteButton.setActionCommand(deleteString);
        deleteButton.addActionListener(new DeleteListener());
        deleteButton.setEnabled(false);

        netAddress = new JTextField(0);
        netAddress.addActionListener(addListener);
        netAddress.getDocument().addDocumentListener(addListener);

        // Create organizing panels
        JPanel buttonPane2 = new JPanel();
        buttonPane2.setLayout(new GridLayout(1, 2, 5, 0));
        buttonPane2.add(deleteButton);
        buttonPane2.add(addButton);

        JPanel buttonPane1 = new JPanel();
        buttonPane1.setLayout(new BorderLayout(0,2));
        buttonPane1.add(netAddress, BorderLayout.NORTH);
        buttonPane1.add(buttonPane2, BorderLayout.SOUTH);

        add(listScrollPane, BorderLayout.CENTER);
        add(buttonPane1, BorderLayout.SOUTH);
    }


    /**
     * Set the color of displayed text of addresses.
     * @param color desired color
     */
    synchronized public void setTextColor(Color color) {
        list.setForeground(color);
    }


    /** Clear all displayed addresses. */
    synchronized public void clearAddresses() {
        listModel.removeAllElements();
    }


    /**
     * Add a single IP address to be displayed.
     * @param address address to be displayed in dot-decimal format.
     */
    synchronized public void addAddress(String address) {
        if (address == null) return;

        addAddressToJList(address, false);
    }


    /**
     * Add a collection of IP addresses to be displayed.
     * @param addresses collection of IP addresses to be displayed in dot-decimal format.
     */
    synchronized public void addAddresses(Collection<String> addresses) {
        if (addresses == null) return;

        for (String addr : addresses) {
            addAddressToJList(addr, false);
        }
    }


    /**
     * Get all the addresses currently being displayed.
     * @return all the addresses currently being displayed.
     */
    synchronized public List<String> getAddresses() {
        ArrayList<String> list = new ArrayList<String>(10);
        Enumeration<String> en = listModel.elements();
        while (en.hasMoreElements()) {
            list.add(en.nextElement());
        }
        return list;
    }


    /**
     * Is this address already being displayed?
     * @param address IP address in dot-decimal format.
     * @return {@code true} if address is currently displayed, else {@code false}.
     */
    public boolean addressAlreadyListed(String address) {
        return listModel.contains(address);
    }


    /**
     * Filter out addresses which are not unique, dot-decimal format,
     * or possibly not multicast-valid.
     * @param addr address to filter
     * @param hitAddButton  did this method get called by hitting
     *                      "Add" button or by calling addAddress(es)?
     */
    private void addAddressToJList(String addr, boolean hitAddButton) {
        // User didn't type in a unique name.or not in dot-decimal format
        if (addr.equals("") || addressAlreadyListed(addr)) {
            Toolkit.getDefaultToolkit().beep();
            netAddress.requestFocusInWindow();
            netAddress.selectAll();
            return;
        }
        else {
            // If not in proper, dot-decimal, format ...
            if (EtUtils.isDottedDecimal(addr) == null) {
                Toolkit.getDefaultToolkit().beep();
                netAddress.requestFocusInWindow();
                netAddress.selectAll();
                if (hitAddButton) {
                    JOptionPane.showMessageDialog(new JFrame(),
                                                  "Address not in dotted-decimal format",
                                                  "Error",
                                                  JOptionPane.ERROR_MESSAGE);
                }
                return;
            }
            else if (forMulticasting) {
                try {
                    InetAddress a = InetAddress.getByName(addr);
                    if (!a.isMulticastAddress()) {
                        if (hitAddButton) {
                            JOptionPane.showMessageDialog(new JFrame(),
                                                          "Address not a multicast address",
                                                          "Error",
                                                          JOptionPane.ERROR_MESSAGE);
                        }
                        return;
                    }
                }
                catch (UnknownHostException e1) {
                    if (hitAddButton) {
                        JOptionPane.showMessageDialog(new JFrame(),
                                                      "Address not a multicast address",
                                                      "Error",
                                                      JOptionPane.ERROR_MESSAGE);
                    }
                    return;
                }
            }
        }

         // Get selected index
        int index = list.getSelectedIndex();
        if (index == -1) {
             // No selection, so insert at beginning
            index = 0;
        } else {
            // Add after the selected item
            index++;
        }

        listModel.insertElementAt(addr, index);
        // If we just wanted to add to the end, we'd do this:
        // listModel.addElement(adr);

        // Reset the text field
        if (hitAddButton) {
            netAddress.requestFocusInWindow();
            netAddress.setText("");
        }

        // Select the new item and make it visible.
        list.setSelectedIndex(index);
        list.ensureIndexIsVisible(index);
    }


    /**
     * Listener for "Delete" button.
     */
    class DeleteListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            // This method can be called only if there's a valid
            // selection so go ahead and remove whatever is selected.
            int index = list.getSelectedIndex();
            listModel.remove(index);

            int size = listModel.getSize();

            if (size == 0) {
                // Nothing left, disable deletions
                deleteButton.setEnabled(false);

            } else {
                // Select an index
                if (index == listModel.getSize()) {
                    // Removed item in last position
                    index--;
                }

                list.setSelectedIndex(index);
                list.ensureIndexIsVisible(index);
            }
        }
    }


    /**
     * This listener is shared by the text field and the "Add" button.
     */
    class AddListener implements ActionListener, DocumentListener {
        private boolean alreadyEnabled = false;
        private JButton button;

        public AddListener(JButton button) {
            this.button = button;
        }

        // Required by ActionListener for button
        public void actionPerformed(ActionEvent e) {
            addAddressToJList(netAddress.getText(), true);
        }


        // Required by DocumentListener for text field
        public void insertUpdate(DocumentEvent e) {
            enableButton();
        }

        // Required by DocumentListener for text field
        public void removeUpdate(DocumentEvent e) {
            handleEmptyTextField(e);
        }

        // Required by DocumentListener for text field
        public void changedUpdate(DocumentEvent e) {
            if (!handleEmptyTextField(e)) {
                enableButton();
            }
        }

        private void enableButton() {
            if (!alreadyEnabled) {
                button.setEnabled(true);
            }
        }

        private boolean handleEmptyTextField(DocumentEvent e) {
            if (e.getDocument().getLength() <= 0) {
                button.setEnabled(false);
                alreadyEnabled = false;
                return true;
            }
            return false;
        }
    }

    // Required by ListSelectionListener.
    public void valueChanged(ListSelectionEvent e) {
        if (!e.getValueIsAdjusting()) {
            if (list.getSelectedIndex() == -1) {
                // No selection, disable delete button.
                deleteButton.setEnabled(false);

            } else {
                // Selection, enable the delete button.
                deleteButton.setEnabled(true);
            }
        }
    }

}
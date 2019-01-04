package org.jlab.coda.jevio.graphics;


import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.EventQueue;
import java.awt.Toolkit;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.UIManager.LookAndFeelInfo;

/**
 * This is a simple GUI that displays an evio event in a tree. It allows the user to open event files and
 * dictionaries, and go event-by-event showing the event in a JTree.
 * @author heddle
 */
@SuppressWarnings("serial")
public class EventTreeFrame extends JFrame  {

    /**
	 * Constructor for a simple tree viewer for evio files.
	 */
	public EventTreeFrame() {
		super("Jevio Event Tree");
		initializeLookAndFeel();

		// set the close to call System.exit
		WindowAdapter wa = new WindowAdapter() {
			@Override
			public void windowClosing(WindowEvent event) {
				System.out.println("Exiting.");
				System.exit(0);
			}
		};
		addWindowListener(wa);

		// add all the components
		addComponents();

		// size to the screen
		sizeToScreen(this, 0.85);
	}

	/**
	 * Add the components to the frame.
	 */
	protected void addComponents() {
        // Panel which holds the tree display and related widgets
        EventTreePanel eventTreePanel = new EventTreePanel();
		addMenus(eventTreePanel);
		add(eventTreePanel, BorderLayout.CENTER);
	}

    /**
     * Add the menus to the frame.
     * @param eventTreePanel panel which holds the tree display and related widgets.
     */
	protected void addMenus(EventTreePanel eventTreePanel) {
		JMenuBar menuBar = new JMenuBar();
		EventTreeMenu eventTreeMenu = new EventTreeMenu(eventTreePanel);
        menuBar.add(eventTreeMenu.createFileMenu());
        menuBar.add(eventTreeMenu.createViewMenu());
		setJMenuBar(menuBar);
        eventTreeMenu.addEventControlPanel();
	}


	/**
	 * Size and center a JFrame relative to the screen.
	 *
	 * @param frame the frame to size.
	 * @param fractionalSize the fraction desired of the screen--e.g., 0.85 for 85%.
	 */
	private void sizeToScreen(JFrame frame, double fractionalSize) {
		Dimension d = Toolkit.getDefaultToolkit().getScreenSize();
		d.width = (int) (fractionalSize * .65 * d.width);
		d.height = (int) (fractionalSize * d.height);
		frame.setSize(d);
		centerComponent(frame);
	}

	/**
	 * Center a component.
	 *
	 * @param component the Component to center.
	 */
	private void centerComponent(Component component) {

		if (component == null)
			return;

		try {
			Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
			Dimension componentSize = component.getSize();
			if (componentSize.height > screenSize.height) {
				componentSize.height = screenSize.height;
			}
			if (componentSize.width > screenSize.width) {
				componentSize.width = screenSize.width;
			}

			int x = ((screenSize.width - componentSize.width) / 2);
			int y = ((screenSize.height - componentSize.height) / 2);

			component.setLocation(x, y);

		}
		catch (Exception e) {
			component.setLocation(200, 200);
			e.printStackTrace();
		}
	}

	/**
	 * Initialize the look and feel.
	 */
	private void initializeLookAndFeel() {

		LookAndFeelInfo[] lnfinfo = UIManager.getInstalledLookAndFeels();

		if ((lnfinfo == null) || (lnfinfo.length < 1)) {
			return;
		}

		String desiredLookAndFeel = "Windows";

		for (int i = 0; i < lnfinfo.length; i++) {
			if (lnfinfo[i].getName().equals(desiredLookAndFeel)) {
				try {
					UIManager.setLookAndFeel(lnfinfo[i].getClassName());
				}
				catch (Exception e) {
					e.printStackTrace();
				}
				return;
			}
		}
	}


	/**
	 * Main program for launching the frame.
	 *
	 * @param args command line arguments--ignored.
	 */
	public static void main(String args[]) {

		final EventTreeFrame frame = new EventTreeFrame();

		// now make the frame visible, in the AWT thread
		EventQueue.invokeLater(new Runnable() {
			public void run() {
				frame.setVisible(true);
			}

		});

	}

}
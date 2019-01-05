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

import java.awt.Toolkit;
import javax.swing.*; 
import javax.swing.text.*; 
//import java.text.NumberFormat;
//import java.text.ParseException;
//import java.util.Locale;

public class WholeNumberField extends JTextField {
    private Toolkit toolkit;
    // private NumberFormat integerFormatter;
    private boolean enforceLimits = false;
    private int upperLimit=0;
    private int lowerLimit=0;

    public WholeNumberField(int value, int columns) {
        super(columns);
        toolkit = Toolkit.getDefaultToolkit();
        // integerFormatter = NumberFormat.getNumberInstance(Locale.US);
        // integerFormatter.setParseIntegerOnly(true);
        setValue(value);
    }

    public WholeNumberField(int value, int columns, int limit1, int limit2) {
        super(columns);
        toolkit = Toolkit.getDefaultToolkit();
        // integerFormatter = NumberFormat.getNumberInstance(Locale.US);
        // integerFormatter.setParseIntegerOnly(true);
	enforceLimits = true;
	setLimits(limit1, limit2);
        setValue(value);
    }

    private int adjustValue(int value) {
        if (enforceLimits == true) {
	    if (value > upperLimit) {
	        value = upperLimit;
	    }
	    else if (value < lowerLimit) {
	        value = lowerLimit;
	    }
	}
	return value;
     }
    
    public int getValue() {
        int value = 0;
        try {
            // value = integerFormatter.parse(getText()).intValue();
            value = Integer.parseInt(getText());
        } catch (NumberFormatException e) {
            // This should never happen because insertString allows
            // only properly formatted data to get in the field.
            toolkit.beep();
        }
	value = adjustValue(value);
        return value;
    }

    public void correctValue() {
        int value = 0;
        try {
            // value = integerFormatter.parse(getText()).intValue();
            value = Integer.parseInt(getText());
        } catch (NumberFormatException e) {
            // This should never happen because insertString allows
            // only properly formatted data to get in the field.
            toolkit.beep();
        }
	setValue(value);
     }
    
    public void setValue(int value) {
	value = adjustValue(value);
        setText(""+value);
	// setText(integerFormatter.format(value));
    }

    public void setLimits(int limit1, int limit2) {
        enforceLimits = true;
	if (limit1 >= limit2) {
            upperLimit = limit1;
	    lowerLimit = limit2;
	}
	else {
            upperLimit = limit2;
	    lowerLimit = limit1;
	}
    }
    
    public void removeLimits() {
        enforceLimits = false;
    }
    
    public void setUpperLimit(int limit) {
        enforceLimits = true;
        upperLimit = limit;
    }
    
    public void setLowerLimit(int limit) {
        enforceLimits = true;
        lowerLimit = limit;
    }
    
    public int getUpperLimit() {
      return upperLimit;
    }
    
    public int getLowerLimit() {
      return lowerLimit;
    }
    
    protected Document createDefaultModel() {
        return new WholeNumberDocument();
    }

    protected class WholeNumberDocument extends PlainDocument {
        public void insertString(int offs, String str, AttributeSet a) 
                     throws BadLocationException {
            char[] source = str.toCharArray();
            char[] result = new char[source.length];
            int value, j=0;

            for (int i=0; i < result.length; i++) {
                // allow for typing in minus sign
		if ((offs == 0) && (i == 0) && (source[0] == '-')) {
                    result[j++] = source[0];
		    if (result.length == 1) {
                      super.insertString(0, new String(source,0,1), a);
		      return;
		    }
		}
		else if (Character.isDigit(source[i]))
                    result[j++] = source[i];
                else {
                    toolkit.beep();
                }
            }
	    String number = new String(getText(0,offs) + new String(result, 0, j));
	    // value = integerFormatter.parse(number).intValue();
 	    // value = adjustValue(value);
            super.remove(0, offs);
            // super.insertString(0, integerFormatter.format(value), a);
            super.insertString(0, number, a);
        }
    }
}

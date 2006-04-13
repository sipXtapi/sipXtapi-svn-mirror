/*
 * 
 * 
 * Copyright (C) 2004 SIPfoundry Inc.
 * Licensed by SIPfoundry under the LGPL license.
 * 
 * Copyright (C) 2004 Pingtel Corp.
 * Licensed to SIPfoundry under a Contributor Agreement.
 * 
 * $
 */
package org.sipfoundry.sipxconfig.components;

import java.util.Arrays;
import java.util.Collection;
import java.util.Iterator;
import java.util.Locale;

import junit.framework.TestCase;

import org.apache.hivemind.Messages;
import org.apache.hivemind.impl.AbstractMessages;
import org.sipfoundry.sipxconfig.common.NamedObject;

/**
 * Comments
 */
public class TapestryUtilsTest extends TestCase {
    
    Collection m_autoComplete;
    
    protected void setUp() {
        AutoCompleteItems[] itemsData = new AutoCompleteItems[] {
                new AutoCompleteItems("robin"),
                new AutoCompleteItems("bluejay"),
                new AutoCompleteItems("hawk"),
                new AutoCompleteItems("harrier"),
                new AutoCompleteItems("grayjay")                
        };
        m_autoComplete = Arrays.asList(itemsData);        
    }

    public void testAssertParameterArrayBounds() {
        try {
            TapestryUtils.assertParameter(String.class, null, 0);
            fail();
        } catch (IllegalArgumentException expected) {
            assertTrue(true);
        }
    }

    public void testAssertParameterWrongClass() {
        Object[] strings = new Object[] {
            "i'm a string"
        };
        try {
            TapestryUtils.assertParameter(Integer.class, strings, 0);
            fail();
        } catch (IllegalArgumentException expected) {
            assertTrue(true);
        }
    }

    public void testAssertParameterIsAssignableFrom() {
        Object[] strings = new Object[] {
            "i'm a string"
        };
        TapestryUtils.assertParameter(Object.class, strings, 0);
        assertTrue(true);
    }

    public void testAssertParameterNotAssignableFrom() {
        Object[] objects = new Object[] {
            new Object()
        };
        try {
            TapestryUtils.assertParameter(String.class, objects, 0);
            fail();
        } catch (IllegalArgumentException expected) {
            assertTrue(true);
        }
    }

    public void testGetMessages() throws Exception {
        Messages messages = new DummyMessages();

        assertEquals("cde", TapestryUtils.getMessage(messages, "abc", "cde"));
        assertNull("cde", TapestryUtils.getMessage(messages, "xyz", null));
        assertEquals("dummy label", TapestryUtils.getMessage(messages, "dummy", null));
    }

    private class DummyMessages extends AbstractMessages {
        protected Locale getLocale() {
            return Locale.US;
        }

        protected String findMessage(String key) {
            if (key.equals("dummy")) {
                return "dummy label";
            }
            return null;
        }
    }
    
    public void testGetAutoCompleteCandidates() {
        Collection actual;
        Iterator i;
        
        actual = TapestryUtils.getAutoCompleteCandidates(m_autoComplete, null);
        assertEquals(5, actual.size());
        
        actual =  TapestryUtils.getAutoCompleteCandidates(m_autoComplete, "");
        assertEquals(5, actual.size());

        actual =  TapestryUtils.getAutoCompleteCandidates(m_autoComplete, " ");
        assertEquals(5, actual.size());

        actual =  TapestryUtils.getAutoCompleteCandidates(m_autoComplete, "n");
        assertEquals(0, actual.size());

        actual =  TapestryUtils.getAutoCompleteCandidates(m_autoComplete, "r");
        assertEquals(1, actual.size());
        assertEquals("robin", actual.iterator().next());

        actual =  TapestryUtils.getAutoCompleteCandidates(m_autoComplete, "robi");
        assertEquals(1, actual.size());
        assertEquals("robin", actual.iterator().next());
        
        actual =  TapestryUtils.getAutoCompleteCandidates(m_autoComplete, "robin");
        assertEquals(1, actual.size());
        assertEquals("robin", actual.iterator().next());

        actual =  TapestryUtils.getAutoCompleteCandidates(m_autoComplete, "h");
        assertEquals(2, actual.size());
        i = actual.iterator();
        assertEquals("hawk", i.next());
        assertEquals("harrier", i.next());
        
        // multi-value
        actual =  TapestryUtils.getAutoCompleteCandidates(m_autoComplete, "peacock");
        assertEquals(0, actual.size());

        actual =  TapestryUtils.getAutoCompleteCandidates(m_autoComplete, "peacock ");
        assertEquals(5, actual.size());
        
        actual =  TapestryUtils.getAutoCompleteCandidates(m_autoComplete, "peacock x");
        assertEquals(0, actual.size());

        actual =  TapestryUtils.getAutoCompleteCandidates(m_autoComplete, "peacock h");
        assertEquals(2, actual.size());
        i = actual.iterator();
        assertEquals("peacock hawk", i.next());
        assertEquals("peacock harrier", i.next());

        actual =  TapestryUtils.getAutoCompleteCandidates(m_autoComplete, "bluejay r");
        assertEquals(1, actual.size());
        i = actual.iterator();
        assertEquals("bluejay robin", i.next());
    }
    
    public void testAutoCompleteCaseInsensitive() {
        Collection actual;
        Iterator i;

        actual =  TapestryUtils.getAutoCompleteCandidates(m_autoComplete, "Bluejay");
        assertEquals(1, actual.size());
        i = actual.iterator();
        assertEquals("bluejay", i.next());
    }        
    
    public void testAutoCompleteRemoveDuplicates() {
        Collection actual;
        actual =  TapestryUtils.getAutoCompleteCandidates(m_autoComplete, "bluejay hawk blue");
        assertEquals(0, actual.size());
    }        

    class AutoCompleteItems implements NamedObject {
        String m_name;
        AutoCompleteItems(String name) {
            m_name = name;
        }
        public String getName() {
            return m_name;
        }
        public void setName(String name_) {
        }
    }
}

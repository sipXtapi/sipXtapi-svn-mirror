/*
 * 
 * 
 * Copyright (C) 2005 SIPfoundry Inc.
 * Licensed by SIPfoundry under the LGPL license.
 * 
 * Copyright (C) 2005 Pingtel Corp.
 * Licensed to SIPfoundry under a Contributor Agreement.
 * 
 * $
 */
package org.sipfoundry.sipxconfig.admin.dialplan.config;

import junit.framework.TestCase;

import org.dom4j.DocumentFactory;
import org.dom4j.Element;

public class FullTransformTest extends TestCase {

    public void testAddChildren() {
        FullTransform ft = new FullTransform();
        ft.setUser("testuser");
        ft.setHost("testhost");
        ft.setFieldParams(new String[] {
            "fp1", "fp2"
        });
        ft.setHeaderParams(new String[] {
            "hp1", "hp2"
        });

        Element element = DocumentFactory.getInstance().createElement("test");
        ft.addChildren(element);

        assertEquals("testhost", element.valueOf("host"));
        assertEquals("testuser", element.valueOf("user"));
        assertEquals("fp1", element.valueOf("fieldparams[1]"));
        assertEquals("fp2", element.valueOf("fieldparams[2]"));
        assertEquals("hp1", element.valueOf("headerparams[1]"));
        assertEquals("hp2", element.valueOf("headerparams[2]"));
    }

    public void testAddChildrenEmpty() {
        FullTransform ft = new FullTransform();

        Element element = DocumentFactory.getInstance().createElement("test");
        ft.addChildren(element);

        // no nodes are added if empty
        assertEquals(0, element.nodeCount());
    }
}

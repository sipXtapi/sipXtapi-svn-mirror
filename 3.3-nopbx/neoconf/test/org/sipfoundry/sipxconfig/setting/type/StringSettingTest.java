/*
 * 
 * 
 * Copyright (C) 2006 SIPfoundry Inc.
 * Licensed by SIPfoundry under the LGPL license.
 * 
 * Copyright (C) 2006 Pingtel Corp.
 * Licensed to SIPfoundry under a Contributor Agreement.
 * 
 * $
 */
package org.sipfoundry.sipxconfig.setting.type;

import junit.framework.TestCase;

public class StringSettingTest extends TestCase {

    public void testConvertToTypedValue() {
        SettingType type = new StringSetting();
        assertEquals("", type.convertToTypedValue(""));
        assertEquals("bongo", type.convertToTypedValue("bongo"));
        assertNull(type.convertToTypedValue(null));
    }

    public void testConvertToStringValue() {
        SettingType type = new StringSetting();
        assertEquals("bongo", type.convertToStringValue("bongo"));
        assertNull("Empty string needs to be converted into null", type.convertToStringValue(""));
        assertNull("Blank string needs to be converted into null", type.convertToStringValue("\t "));
        assertNull(type.convertToStringValue(null));
    }
}

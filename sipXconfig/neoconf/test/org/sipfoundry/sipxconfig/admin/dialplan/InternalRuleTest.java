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
package org.sipfoundry.sipxconfig.admin.dialplan;

import java.util.ArrayList;
import java.util.List;

import junit.framework.TestCase;

import org.apache.commons.lang.ArrayUtils;
import org.apache.commons.lang.StringUtils;
import org.sipfoundry.sipxconfig.admin.dialplan.MappingRule.Operator;
import org.sipfoundry.sipxconfig.admin.dialplan.config.UrlTransform;
import org.sipfoundry.sipxconfig.permission.PermissionName;

/**
 * InternalRuleTest
 */
public class InternalRuleTest extends TestCase {

    private static final String URL_PARAMS = ";voicexml={voicemail}%2Fcgi-bin%2Fvoicemail%2Fmediaserver.cgi%3Faction%3D";
    private static final String VOICEMAIL_URL = "<sip:{digits}@{mediaserver}" + URL_PARAMS
            + "retrieve>";
    private static final String VOICEMAIL_FALLBACK_URL = "<sip:{digits}@{mediaserver}"
            + URL_PARAMS + "deposit%26mailbox%3D{digits-escaped}>;q=0.1";
    private static final String VOICEMAIL_TRANSFER_URL = "<sip:{vdigits}@{mediaserver}"
            + URL_PARAMS + "deposit%26mailbox%3D{vdigits-escaped}>";

    private static final String TEST_DESCRIPTION = "kuku description";
    private static final String TEST_NAME = "kuku name";

    public void testAppendToGenerationRules() throws Exception {
        InternalRule ir = new InternalRule();
        ir.setName("kuku");
        ir.setDescription(TEST_DESCRIPTION);
        ir.setLocalExtensionLen(5);
        ir.setVoiceMail("20004");
        ir.setVoiceMailPrefix("7");
        ir.setEnabled(true);

        List rules = new ArrayList();
        ir.appendToGenerationRules(rules);

        assertEquals(3, rules.size());

        MappingRule v = (MappingRule) rules.get(0);
        MappingRule vt = (MappingRule) rules.get(1);
        MappingRule vf = (MappingRule) rules.get(2);

        assertEquals(TEST_DESCRIPTION, v.getDescription());
        assertEquals("20004", v.getPatterns()[0]);
        assertEquals(0, v.getPermissions().size());
        UrlTransform tv = (UrlTransform) v.getTransforms()[0];
        assertEquals(VOICEMAIL_URL, tv.getUrl());

        assertEquals(TEST_DESCRIPTION, vt.getDescription());
        assertEquals("7xxxxx", vt.getPatterns()[0]);
        assertEquals(0, vt.getPermissions().size());
        UrlTransform tvt = (UrlTransform) vt.getTransforms()[0];
        assertEquals(VOICEMAIL_TRANSFER_URL, tvt.getUrl());

        assertEquals(TEST_DESCRIPTION, vf.getDescription());
        assertEquals(".", vf.getPatterns()[0]);
        assertEquals(PermissionName.VOICEMAIL.getName(), vf.getPermissionNames().get(0));
        UrlTransform tvf = (UrlTransform) vf.getTransforms()[0];
        assertEquals(VOICEMAIL_FALLBACK_URL, tvf.getUrl());
    }

    public void testOperator() {
        AutoAttendant aa = new AutoAttendant();
        aa.setName(TEST_NAME);
        aa.setDescription(TEST_DESCRIPTION);

        Operator o = new MappingRule.Operator(aa, "100", ArrayUtils.EMPTY_STRING_ARRAY);
        assertEquals("100", StringUtils.join(o.getPatterns(), "|"));
        assertEquals(TEST_NAME, o.getName());
        assertEquals(TEST_DESCRIPTION, o.getDescription());
    }

    public void testOperatorWithAliases() {
        AutoAttendant aa = new AutoAttendant();
        aa.setName(TEST_NAME);
        aa.setDescription(TEST_DESCRIPTION);

        Operator o = new MappingRule.Operator(aa, "100", new String[] {
            "0", "operator"
        });
        assertEquals("100|0|operator", StringUtils.join(o.getPatterns(), "|"));
        assertEquals(TEST_NAME, o.getName());
        assertEquals(TEST_DESCRIPTION, o.getDescription());
    }

    public void testOperatorNoExtension() {
        AutoAttendant aa = new AutoAttendant();
        aa.setName(TEST_NAME);
        aa.setDescription(TEST_DESCRIPTION);

        Operator o = new MappingRule.Operator(aa, null, new String[] {
            "0", "operator"
        });
        assertEquals("0|operator", StringUtils.join(o.getPatterns(), "|"));
        assertEquals(TEST_NAME, o.getName());
        assertEquals(TEST_DESCRIPTION, o.getDescription());
    }
}
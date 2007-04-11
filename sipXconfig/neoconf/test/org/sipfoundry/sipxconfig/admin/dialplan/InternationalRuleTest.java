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

import org.sipfoundry.sipxconfig.admin.dialplan.config.FullTransform;
import org.sipfoundry.sipxconfig.admin.dialplan.config.Transform;
import org.sipfoundry.sipxconfig.gateway.Gateway;
import org.sipfoundry.sipxconfig.permission.PermissionName;

/**
 * InternationalRuleTest
 */
public class InternationalRuleTest extends TestCase {
    private InternationalRule m_rule;

    protected void setUp() throws Exception {
        m_rule = new InternationalRule();
        m_rule.setInternationalPrefix("011");

        List list = new ArrayList();
        Gateway g1 = new Gateway();
        g1.setAddress("i1.gateway.com");
        list.add(g1);
        Gateway g2 = new Gateway();
        g2.setAddress("i2.gateway.com");
        g2.setPrefix("4321");
        list.add(g2);
        m_rule.setGateways(list);
    }

    public void testGetPatterns() {
        String[] patterns = m_rule.getPatterns();
        assertEquals(1, patterns.length);
        assertEquals("011x.", patterns[0]);
    }

    public void testGetTransforms() {
        Transform[] transforms = m_rule.getTransforms();
        assertEquals(2, transforms.length);
        FullTransform transform = (FullTransform) transforms[0];
        assertEquals("011{vdigits}", transform.getUser());
        assertEquals("i1.gateway.com", transform.getHost());
        transform = (FullTransform) transforms[1];
        assertEquals("4321011{vdigits}", transform.getUser());
        assertEquals("i2.gateway.com", transform.getHost());
    }

    public void testGetPermissionNames() {
        List permissions = m_rule.getPermissionNames();
        assertEquals(1, permissions.size());
        assertEquals(PermissionName.INTERNATIONAL_DIALING.getName(), permissions.get(0));
    }
}
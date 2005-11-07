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
package org.sipfoundry.sipxconfig.conference;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

import org.dbunit.database.IDatabaseConnection;
import org.sipfoundry.sipxconfig.SipxDatabaseTestCase;
import org.sipfoundry.sipxconfig.TestHelper;
import org.sipfoundry.sipxconfig.common.CoreContext;
import org.sipfoundry.sipxconfig.common.User;
import org.sipfoundry.sipxconfig.common.UserException;

public class ConferenceBridgeContextImplTestDb extends SipxDatabaseTestCase {

    private ConferenceBridgeContext m_context;
    private CoreContext m_coreContext;

    protected void setUp() throws Exception {
        m_context = (ConferenceBridgeContext) TestHelper.getApplicationContext().getBean(
                ConferenceBridgeContext.CONTEXT_BEAN_NAME);
        m_coreContext = (CoreContext) TestHelper.getApplicationContext().getBean(
                CoreContext.CONTEXT_BEAN_NAME);

        TestHelper.cleanInsert("ClearDb.xml");
        TestHelper.insertFlat("conference/users.db.xml");
    }

    public void testGetBridges() throws Exception {
        TestHelper.insertFlat("conference/participants.db.xml");
        assertEquals(2, m_context.getBridges().size());
    }

    public void testStore() throws Exception {
        IDatabaseConnection db = TestHelper.getConnection();
        User u1 = m_coreContext.loadUser(new Integer(1003));

        Bridge bridge = new Bridge();
        bridge.setName("b1");
        Conference conference = new Conference();
        conference.setName("c1");
        Participant p1 = new Participant();
        p1.setUser(u1);
        conference.insertParticipant(p1);
        bridge.insertConference(conference);

        m_context.store(bridge);

        assertEquals(1, db.getRowCount("meetme_bridge"));
        assertEquals(1, db.getRowCount("meetme_conference"));
        assertEquals(1, db.getRowCount("meetme_participant"));
    }

    public void testRemoveBridges() throws Exception {
        IDatabaseConnection db = TestHelper.getConnection();
        TestHelper.insertFlat("conference/participants.db.xml");

        assertEquals(2, db.getRowCount("meetme_bridge"));
        assertEquals(3, db.getRowCount("meetme_conference"));
        assertEquals(6, db.getRowCount("meetme_participant"));

        m_context.removeBridges(Collections.singleton(new Integer(2005)));

        assertEquals(1, db.getRowCount("meetme_bridge"));
        assertEquals(1, db.getRowCount("meetme_conference"));
        assertEquals(2, db.getRowCount("meetme_participant"));
    }

    public void testRemoveConferences() throws Exception {
        IDatabaseConnection db = TestHelper.getConnection();
        TestHelper.insertFlat("conference/participants.db.xml");

        assertEquals(2, db.getRowCount("meetme_bridge"));
        assertEquals(3, db.getRowCount("meetme_conference"));
        assertEquals(6, db.getRowCount("meetme_participant"));

        m_context.removeConferences(Collections.singleton(new Integer(3002)));

        assertEquals(2, db.getRowCount("meetme_bridge"));
        assertEquals(2, db.getRowCount("meetme_conference"));
        assertEquals(3, db.getRowCount("meetme_participant"));
    }

    public void testRemoveParticipants() throws Exception {
        IDatabaseConnection db = TestHelper.getConnection();
        TestHelper.insertFlat("conference/participants.db.xml");

        Conference conference = m_context.loadConference(new Integer(3002));

        Collection ps = conference.getParticipants();
        assertEquals(3, ps.size());

        assertEquals(2, db.getRowCount("meetme_bridge"));
        assertEquals(3, db.getRowCount("meetme_conference"));
        assertEquals(6, db.getRowCount("meetme_participant"));

        List remove = new ArrayList();
        Iterator iter = ps.iterator();
        for (int i = 0; i < 2; i++) {
            Participant p = (Participant) iter.next();
            remove.add(p.getId());
        }

        m_context.removeParticipants(remove);

        assertEquals(2, db.getRowCount("meetme_bridge"));
        assertEquals(3, db.getRowCount("meetme_conference"));
        assertEquals(4, db.getRowCount("meetme_participant"));

        conference = m_context.loadConference(new Integer(3002));
        ps = conference.getParticipants();
        assertEquals(1, ps.size());
    }

    public void testAddParticipantsToConference() throws Exception {
        TestHelper.insertFlat("conference/participants.db.xml");
        Collection usersIds = new ArrayList();
        for (int i = 0; i < 4; i++) {
            usersIds.add(new Integer(1002 + i));
        }
        Integer conferenceId = new Integer(3001);
        Conference conference = m_context.loadConference(conferenceId);
        assertEquals(1, conference.getParticipants().size());
        Participant p = (Participant) conference.getParticipants().iterator().next();
        assertEquals(new Integer(1002), p.getUser().getId());

        m_context.addParticipantsToConference(conferenceId, usersIds);

        conference = m_context.loadConference(conferenceId);
        // it's still 4 and not 5 - 1002 was already added
        assertEquals(4, conference.getParticipants().size());
    }

    public void testLoadBridge() throws Exception {
        TestHelper.insertFlat("conference/participants.db.xml");
        Bridge bridge = m_context.loadBridge(new Integer(2006));

        assertEquals(1, bridge.getConferences().size());
    }

    public void testLoadConference() throws Exception {
        TestHelper.insertFlat("conference/participants.db.xml");
        Conference conference = m_context.loadConference(new Integer(3001));

        assertEquals(1, conference.getParticipants().size());
    }

    public void testLoadParticipant() throws Exception {
        TestHelper.insertFlat("conference/participants.db.xml");
        Participant participant = m_context.loadParticipant(new Integer(4006));

        assertEquals(new Integer(1005), participant.getUser().getId());
        assertEquals(new Integer(3003), participant.getConference().getId());
    }

    public void testClear() throws Exception {
        IDatabaseConnection db = TestHelper.getConnection();
        TestHelper.insertFlat("conference/participants.db.xml");

        assertTrue(0 < db.getRowCount("meetme_bridge"));
        assertTrue(0 < db.getRowCount("meetme_conference"));
        assertTrue(0 < db.getRowCount("meetme_participant"));

        m_context.clear();

        assertEquals(0, db.getRowCount("meetme_bridge"));
        assertEquals(0, db.getRowCount("meetme_conference"));
        assertEquals(0, db.getRowCount("meetme_participant"));
    }    

    public void testIsAliasInUse() throws Exception {
        TestHelper.getConnection();
        TestHelper.insertFlat("conference/participants.db.xml");
        
        // conference names are aliases
        assertTrue(m_context.isAliasInUse("conf_3001"));
        assertTrue(m_context.isAliasInUse("conf_3002"));
        assertTrue(m_context.isAliasInUse("conf_3003"));

        // conference extensions are aliases
        assertTrue(m_context.isAliasInUse("1699"));
        assertTrue(m_context.isAliasInUse("1700"));
        assertTrue(m_context.isAliasInUse("1701"));
        
        // we're not using this extension
        assertFalse(m_context.isAliasInUse("1702"));
    }

    public void testGetBeanIdsOfObjectsWithAlias() throws Exception {
        TestHelper.getConnection();
        TestHelper.insertFlat("conference/participants.db.xml");
        
        // conference names are aliases
        assertTrue(m_context.getBeanIdsOfObjectsWithAlias("conf_3001").size() == 1);
        assertTrue(m_context.getBeanIdsOfObjectsWithAlias("conf_3002").size() == 1);
        assertTrue(m_context.getBeanIdsOfObjectsWithAlias("conf_3003").size() == 1);

        // conference extensions are aliases
        assertTrue(m_context.getBeanIdsOfObjectsWithAlias("1699").size() == 1);
        assertTrue(m_context.getBeanIdsOfObjectsWithAlias("1700").size() == 1);
        assertTrue(m_context.getBeanIdsOfObjectsWithAlias("1701").size() == 1);
        
        // we're not using this extension
        assertTrue(m_context.getBeanIdsOfObjectsWithAlias("1702").size() == 0);        
    }

    public void testValidate() throws Exception {
        TestHelper.getConnection();
        TestHelper.insertFlat("conference/participants.db.xml");
        
        // create a conference with a duplicate extension, should fail to validate
        Conference conf = new Conference();
        conf.setName("Appalachian");
        conf.setExtension("1699");
        try {
            m_context.validate(conf);
            fail("conference has duplicate extension but was validated anyway");
        } catch (UserException e) {
            // expected
        }
        
        // pick an unused extension, should be OK
        conf.setExtension("1800");
        m_context.validate(conf);
    }
}

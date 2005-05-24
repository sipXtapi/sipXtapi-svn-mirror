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
package org.sipfoundry.sipxconfig.admin.callgroup;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;

import org.sipfoundry.sipxconfig.admin.dialplan.config.Orbits;
import org.sipfoundry.sipxconfig.admin.forwarding.GenerateMessage;
import org.sipfoundry.sipxconfig.common.CoreContext;
import org.sipfoundry.sipxconfig.common.Organization;
import org.sipfoundry.sipxconfig.phone.PhoneContext;
import org.springframework.jms.core.JmsOperations;
import org.springframework.orm.hibernate.HibernateTemplate;
import org.springframework.orm.hibernate.support.HibernateDaoSupport;

/**
 * Hibernate implementation of the call group context
 */
public class CallGroupContextImpl extends HibernateDaoSupport implements CallGroupContext {
    private PhoneContext m_phoneContext;

    private CoreContext m_coreContext;

    private Orbits m_orbitsGenerator;

    private JmsOperations m_jms;

    public CallGroup loadCallGroup(Integer id) {
        return (CallGroup) getHibernateTemplate().load(CallGroup.class, id);
    }

    public void storeCallGroup(CallGroup callGroup) {
        getHibernateTemplate().saveOrUpdate(callGroup);
    }

    public void removeCallGroups(Collection ids) {
        removeAll(CallGroup.class, ids);
    }

    public List getCallGroups() {
        return getHibernateTemplate().loadAll(CallGroup.class);
    }

    public void duplicateCallGroups(Collection ids_) {
        // TODO Auto-generated method stub
    }

    /**
     * Remove all call groups - mostly used for testing
     */
    public void clear() {
        HibernateTemplate template = getHibernateTemplate();
        Collection callGroups = template.loadAll(CallGroup.class);
        template.deleteAll(callGroups);
        Collection orbits = template.loadAll(ParkOrbit.class);
        template.deleteAll(orbits);
    }

    public void activateCallGroups() {
        List callGroups = getCallGroups();

        for (Iterator i = callGroups.iterator(); i.hasNext();) {
            CallGroup cg = (CallGroup) i.next();
            cg.activate(m_phoneContext);
        }

        triggerAliasGeneration();
    }

    /**
     * Sends notification to progile generator to trigger alias generation
     */
    private void triggerAliasGeneration() {
        // make profilegenerator to propagate new aliases
        if (null != m_jms) {
            m_jms.send(new GenerateMessage(GenerateMessage.TYPE_ALIAS));
        }
    }

    /**
     * Generate aliases for all call groups
     */
    public List getAliases() {
        Organization org = m_coreContext.loadRootOrganization();
        final String dnsDomain = org.getDnsDomain();
        List callGroups = getCallGroups();
        List allAliases = new ArrayList();
        for (Iterator i = callGroups.iterator(); i.hasNext();) {
            CallGroup cg = (CallGroup) i.next();
            allAliases.addAll(cg.generateAliases(dnsDomain));
        }
        return allAliases;
    }

    public void storeParkOrbit(ParkOrbit parkOrbit) {
        getHibernateTemplate().saveOrUpdate(parkOrbit);
    }

    public void removeParkOrbits(Collection ids) {
        removeAll(ParkOrbit.class, ids);
    }

    void removeAll(Class klass, Collection ids) {
        HibernateTemplate template = getHibernateTemplate();
        Collection entities = new ArrayList(ids.size());
        for (Iterator i = ids.iterator(); i.hasNext();) {
            Integer id = (Integer) i.next();
            Object entity = template.load(klass, id);
            entities.add(entity);
        }
        template.deleteAll(entities);
    }

    public ParkOrbit loadParkOrbit(Integer id) {
        return (ParkOrbit) getHibernateTemplate().load(ParkOrbit.class, id);
    }

    public Collection getParkOrbits() {
        return getHibernateTemplate().loadAll(ParkOrbit.class);
    }

    public void activateParkOrbits() {
        try {
            Collection orbits = getParkOrbits();
            BackgroundMusic defaultMusic = getBackgroundMusic();
            m_orbitsGenerator.generate(defaultMusic, orbits);
            m_orbitsGenerator.writeToFile();
        } catch (IOException e) {
            new RuntimeException("Activating call parking configuration failed.", e);
        }
    }

    public void setJms(JmsOperations jms) {
        m_jms = jms;
    }

    public void setCoreContext(CoreContext coreContext) {
        m_coreContext = coreContext;
    }

    public void setPhoneContext(PhoneContext phoneContext) {
        m_phoneContext = phoneContext;
    }

    public void setOrbitsGenerator(Orbits orbitsGenerator) {
        m_orbitsGenerator = orbitsGenerator;
    }

    public String getDefaultMusicOnHold() {
        return getBackgroundMusic().getMusic();
    }

    public void setDefaultMusicOnHold(String music) {
        BackgroundMusic backgroundMusic = getBackgroundMusic();
        backgroundMusic.setMusic(music);
        getHibernateTemplate().saveOrUpdate(backgroundMusic);
    }

    private BackgroundMusic getBackgroundMusic() {
        List musicList = getHibernateTemplate().loadAll(BackgroundMusic.class);
        if (!musicList.isEmpty()) {
            return (BackgroundMusic) musicList.get(0);
        }
        return new BackgroundMusic();
    }
}

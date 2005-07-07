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
package org.sipfoundry.sipxconfig.admin.forwarding;

import org.apache.commons.lang.StringUtils;
import org.sipfoundry.sipxconfig.admin.callgroup.AbstractRing;

/**
 * Ring - represents one stage in a call forwaring sequence
 */
public class Ring extends AbstractRing {
    private String m_number = StringUtils.EMPTY;
    private CallSequence m_callSequence;

    /**
     * Default "bean" constructor
     */
    public Ring() {
        // empty
    }

    /**
     * @param number phone number or SIP url to which call is to be transfered
     * @param expiration number of seconds that call will ring
     * @param type if the call should wait for the previous call failure or start ringing at the
     *        same time
     */
    Ring(String number, int expiration, Type type) {
        m_number = number;
        setExpiration(expiration);
        setType(type);
    }

    /**
     * Retrieves the user part of the contact used to calculate contact
     * 
     * @return String or object implementing toString method
     */
    protected Object getUserPart() {
        return m_number;
    }

    public synchronized String getNumber() {
        return m_number;
    }

    public synchronized void setNumber(String number) {
        m_number = number;
    }

    public synchronized CallSequence getCallSequence() {
        return m_callSequence;
    }

    public synchronized void setCallSequence(CallSequence callSequence) {
        m_callSequence = callSequence;
    }
}

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

import java.io.Serializable;
import java.util.Collection;
import java.util.List;

import org.sipfoundry.sipxconfig.alias.AliasOwner;

public interface ConferenceBridgeContext extends AliasOwner {

    public static final String CONTEXT_BEAN_NAME = "conferenceBridgeContext";

    List getBridges();

    void store(Bridge bridge);

    void store(Conference conference);
    
    /**
     * Check whether the conference is valid and can be stored.  If the conference
     * is OK, then return.  If it's not OK, then throw UserException.
     */
    void validate(Conference conference);

    Bridge newBridge();

    Conference newConference();

    void removeBridges(Collection bridgesIds);

    void removeConferences(Collection conferencesIds);

    void removeParticipants(Collection participantsIds);

    void addParticipantsToConference(Serializable conferenceId, Collection usersIds);

    Bridge loadBridge(Serializable serverId);

    Conference loadConference(Serializable id);

    Participant loadParticipant(Serializable id);

    List getAliases();

    void clear();
}

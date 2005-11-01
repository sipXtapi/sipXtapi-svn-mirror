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
package org.sipfoundry.sipxconfig.admin.parkorbit;

import java.util.Collection;

import org.sipfoundry.sipxconfig.admin.commserver.AliasProvider;
import org.sipfoundry.sipxconfig.alias.AliasOwner;

public interface ParkOrbitContext extends AliasOwner, AliasProvider {
    public static final String CONTEXT_BEAN_NAME = "parkOrbitContext";

    ParkOrbit loadParkOrbit(Integer id);

    void storeParkOrbit(ParkOrbit parkOrbit);

    void removeParkOrbits(Collection ids);

    Collection getParkOrbits();

    void activateParkOrbits();

    String getDefaultMusicOnHold();

    void setDefaultMusicOnHold(String newMusic);

    void clear();
}

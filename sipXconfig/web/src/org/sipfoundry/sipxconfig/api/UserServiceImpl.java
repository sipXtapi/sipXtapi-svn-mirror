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
package org.sipfoundry.sipxconfig.api;


public class UserServiceImpl implements UserService {
    
    public void hello(String message, String message2) {
        System.out.print("got message " + message);
        System.out.print("got message2 " + message2);
    }
}

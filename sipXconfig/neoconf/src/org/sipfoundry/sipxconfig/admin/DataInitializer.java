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
package org.sipfoundry.sipxconfig.admin;

import org.sipfoundry.sipxconfig.common.InitializationTask;
import org.sipfoundry.sipxconfig.common.SystemTaskEntryPoint;
import org.springframework.context.ApplicationContext;
import org.springframework.context.ApplicationContextAware;

/**
 * Iterates through all beans in Spring context and calls the beans that implement Patch interface
 * with opportunity to apply a db patch.
 */
public class DataInitializer implements SystemTaskEntryPoint, ApplicationContextAware {
    private AdminContext m_adminContext;
    private ApplicationContext m_app;
    
    public void runSystemTask(String[] args) {
        String[] tasks = m_adminContext.getInitializationTasks();
        for (int i = 0; i < tasks.length; i++) {
            initializeData(tasks[i]);
        }
        
        // unclear exactly why we'd need to ever call exit.  If you find out why,
        // replace this comment w/reason
        if (args.length >= 2 || !"noexit".equals(args[1])) {
            System.exit(0);
        }
    }
    
    void initializeData(String task) {
        InitializationTask event = new InitializationTask(task);
        m_app.publishEvent(event);
        m_adminContext.deleteInitializationTask(task);        
    }

    public void setApplicationContext(ApplicationContext applicationContext) {
        m_app = applicationContext;
    }

    public void setAdminContext(AdminContext adminContext) {
        m_adminContext = adminContext;
    }
}
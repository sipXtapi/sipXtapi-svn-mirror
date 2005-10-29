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
package org.sipfoundry.sipxconfig.site.dialplan;

import org.apache.tapestry.IRequestCycle;
import org.apache.tapestry.html.BasePage;
import org.sipfoundry.sipxconfig.admin.commserver.SipxProcessManager.Process;
import org.sipfoundry.sipxconfig.admin.dialplan.DialPlanManager;
import org.sipfoundry.sipxconfig.admin.dialplan.config.ConfigFileType;
import org.sipfoundry.sipxconfig.admin.dialplan.config.ConfigGenerator;
import org.sipfoundry.sipxconfig.site.admin.commserver.RestartReminderPanel;

/**
 * ActivateDialPlan
 */
public abstract class ActivateDialPlan extends BasePage {
    public static final String PAGE = "ActivateDialPlan";

    public abstract ConfigFileType getSelected();

    public abstract DialPlanManager getDialPlanContext();

    public String getXml() {
        ConfigGenerator generator = getDialPlanContext().getGenerator();
        ConfigFileType type = getSelected();
        return generator.getFileContent(type);
    }

    public void setXml(String xml_) {
        // ignore xml - read only field
    }

    public Process[] getAffectedProcesses() {
        return new Process[] {
            Process.REGISTRAR, Process.AUTH_PROXY
        };
    }

    public void activate(IRequestCycle cycle) {
        DialPlanManager manager = getDialPlanContext();
        manager.activateDialPlan();
        RestartReminderPanel reminder = (RestartReminderPanel) getComponent("reminder");
        reminder.restart();
        cycle.activate(EditFlexibleDialPlan.PAGE);
    }
}

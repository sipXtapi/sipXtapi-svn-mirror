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
package org.sipfoundry.sipxconfig.site.gateway;

import java.util.Collection;
import java.util.Iterator;
import java.util.List;

import org.apache.tapestry.IRequestCycle;
import org.apache.tapestry.event.PageEvent;
import org.apache.tapestry.event.PageRenderListener;
import org.apache.tapestry.html.BasePage;
import org.sipfoundry.sipxconfig.admin.dialplan.DialPlanManager;
import org.sipfoundry.sipxconfig.admin.dialplan.DialingRule;
import org.sipfoundry.sipxconfig.gateway.Gateway;
import org.sipfoundry.sipxconfig.gateway.GatewayManager;
import org.sipfoundry.sipxconfig.site.dialplan.EditDialRule;

/**
 * List all the gateways, allow adding and deleting gateways
 */
public abstract class SelectGateways extends BasePage implements PageRenderListener {
    public static final String PAGE = "SelectGateways";

    // virtual properties
    public abstract DialPlanManager getDialPlanContext();

    public abstract void setDialPlanContext(DialPlanManager manager);

    public abstract GatewayManager getGatewayContext();

    public abstract void setGatewayContext(GatewayManager context);

    public abstract Integer getRuleId();

    public abstract void setRuleId(Integer id);

    public abstract Collection getSelectedRows();

    public abstract void setGateways(Collection gateways);

    public abstract Collection getGateways();

    public abstract String getNextPage();

    public abstract void setNextPage(String nextPage);

    public void pageBeginRender(PageEvent event_) {
        Collection gateways = getGateways();
        if (null == gateways) {
            gateways = getGatewayContext().getAvailableGateways(getRuleId());
            setGateways(gateways);
        }
    }

    public void formSubmit(IRequestCycle cycle) {
        Collection selectedRows = getSelectedRows();
        if (selectedRows != null) {
            selectGateways(selectedRows);
        }
        EditDialRule editPage = (EditDialRule) cycle.getPage(getNextPage());
        editPage.setRuleId(getRuleId());
        cycle.activate(editPage);
    }

    /**
     * Adds/removes gateways from dial plan
     * 
     * @param gatewayIds list of gateway ids to be added to the dial plan
     */
    void selectGateways(Collection gatewayIds) {
        DialPlanManager manager = getDialPlanContext();
        Integer ruleId = getRuleId();
        DialingRule rule = manager.getRule(ruleId);
        if (null == rule) {
            return;
        }
        List gateways = getGatewayContext().getGatewayByIds(gatewayIds);
        for (Iterator i = gateways.iterator(); i.hasNext();) {
            Gateway gateway = (Gateway) i.next();
            rule.addGateway(gateway);
        }
        manager.storeRule(rule);
    }
}

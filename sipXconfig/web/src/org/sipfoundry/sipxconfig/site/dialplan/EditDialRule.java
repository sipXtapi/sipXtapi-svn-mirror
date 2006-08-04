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

import org.apache.tapestry.IPage;
import org.apache.tapestry.IRequestCycle;
import org.apache.tapestry.callback.ICallback;
import org.apache.tapestry.callback.PageCallback;
import org.apache.tapestry.event.PageBeginRenderListener;
import org.apache.tapestry.event.PageEvent;
import org.apache.tapestry.html.BasePage;
import org.sipfoundry.sipxconfig.admin.dialplan.DialPlanContext;
import org.sipfoundry.sipxconfig.admin.dialplan.DialingRule;
import org.sipfoundry.sipxconfig.admin.dialplan.DialingRuleFactory;
import org.sipfoundry.sipxconfig.admin.dialplan.DialingRuleType;
import org.sipfoundry.sipxconfig.common.Permission;

/**
 * EditDialRule
 */
public abstract class EditDialRule extends BasePage implements PageBeginRenderListener {
    /**
     * list of permission types allowed for long distance permission, used in permssions modle on
     * a long distance page
     */
    public static final Permission[] LONG_DISTANCE_PERMISSIONS = {
        Permission.LONG_DISTANCE_DIALING, Permission.RESTRICTED_DIALING,
        Permission.TOLL_FREE_DIALING
    };

    private DialingRuleType m_ruleType;

    public abstract DialPlanContext getDialPlanContext();

    public abstract Integer getRuleId();

    public abstract void setRuleId(Integer ruleId);

    public abstract DialingRule getRule();

    public abstract void setRule(DialingRule rule);

    public abstract ICallback getCallback();

    public abstract void setCallback(ICallback callback);

    public Permission[] getCallHandlingPermissions() {
        return Permission.CALL_HANDLING.getChildren();
    }

    public DialingRuleType getRuleType() {
        return m_ruleType;
    }

    public void setRuleType(DialingRuleType dialingType) {
        m_ruleType = dialingType;
    }

    public void pageBeginRender(PageEvent event_) {
        DialingRule rule = getRule();
        if (null != rule) {
            return;
        }
        Integer id = getRuleId();
        if (null != id) {
            DialPlanContext manager = getDialPlanContext();
            rule = manager.getRule(id);
        } else {
            rule = createNewRule();
        }
        setRule(rule);

        // Ignore the callback passed to us for now because we're navigating
        // to unexpected places. Always go to the EditFlexibleDialPlan plan.
        setCallback(new PageCallback(EditFlexibleDialPlan.PAGE));
    }

    protected DialingRule createNewRule() {
        DialingRuleFactory ruleFactory = getDialPlanContext().getRuleFactory();
        DialingRuleType ruleType = getRuleType();
        return ruleFactory.create(ruleType);
    }

    /**
     * Go to emergency routing page. Set callback to return to this page. Used only in
     * EditEmergencyRouting rule
     */
    public IPage emergencyRouting(IRequestCycle cycle) {
        EditEmergencyRouting page = (EditEmergencyRouting) cycle
                .getPage(EditEmergencyRouting.PAGE);
        PageCallback callback = new PageCallback(getPage());
        page.setCallback(callback);
        return page;
    }
}

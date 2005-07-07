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
package com.pingtel.pds.pgs.phone;

import java.rmi.RemoteException;

import com.pingtel.pds.common.MD5Encoder;
import com.pingtel.pds.pgs.organization.OrganizationBusiness;
import com.pingtel.pds.pgs.profile.RefPropertyBusiness;

/**
 * DeviceHelper
 */
public class DeviceHelper {
    private DeviceBusiness m_device;

    DeviceHelper(DeviceBusiness device) {
        m_device = device;
    }

    public String calculateDeviceLineUrl(OrganizationBusiness org) throws RemoteException {
        return m_device.getShortName() + "<sip:" + m_device.getSerialNumber() + "@"
                + org.getDNSDomain() + ">";
    }

    public String createInitialDeviceLine(OrganizationBusiness org, RefPropertyBusiness rp)
            throws RemoteException {
        String deviceLineURL = calculateDeviceLineUrl(org);
        String dnsDomain = org.getDNSDomain();
        String realm = org.getAuthenticationRealm();
        String serialNumber = m_device.getSerialNumber();
        String deviceID = serialNumber + "@" + dnsDomain;

        String digestedPassword = getDigest(org);

        StringBuffer xmlContent = new StringBuffer();
        xmlContent.append("<PROFILE>");
        xmlContent.append("<PHONESET_LINE ref_property_id=\"" + rp.getID() + "\">");
        xmlContent.append("<PHONESET_LINE>");
        xmlContent.append("<ALLOW_FORWARDING>" + CDATAIt("DISABLE") + "</ALLOW_FORWARDING>");
        xmlContent.append("<REGISTRATION>" + CDATAIt("REGISTER") + "</REGISTRATION>");
        xmlContent.append("<URL>" + CDATAIt(deviceLineURL) + "</URL>");
        xmlContent.append("<CREDENTIAL autogenerated=\"true\">");
        xmlContent.append("<REALM>" + CDATAIt(realm) + "</REALM>");
        xmlContent.append("<USERID>" + CDATAIt(deviceID) + "</USERID>");
        xmlContent.append("<PASSTOKEN>" + CDATAIt(digestedPassword) + "</PASSTOKEN>");
        xmlContent.append("</CREDENTIAL>");
        xmlContent.append("</PHONESET_LINE>");
        xmlContent.append("</PHONESET_LINE>");
        xmlContent.append("</PROFILE>");
        return xmlContent.toString();
    }

    public String getDigest(OrganizationBusiness org) throws RemoteException {
        String serialNumber = m_device.getSerialNumber();
        String realm = org.getAuthenticationRealm();
        String timeStamp = Long.toString(System.currentTimeMillis());
        return MD5Encoder.digestPassword(serialNumber, realm, timeStamp);
    }

    /**
     * wraps the given input string with CDATA markup.
     * 
     * @param source
     *            String to be wrapped
     * @return wrapped CDATA String
     */
    public static String CDATAIt(String source) {
        return "<![CDATA[" + source + "]]>";
    }

}

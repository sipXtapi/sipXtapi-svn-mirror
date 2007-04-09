/*
 * 
 * 
 * Copyright (C) 2007 SIPfoundry Inc.
 * Licensed by SIPfoundry under the LGPL license.
 * 
 * Copyright (C) 2007 Pingtel Corp.
 * Licensed to SIPfoundry under a Contributor Agreement.
 * 
 * $
 */
package org.sipfoundry.sipxconfig.phone.cisco;

import org.sipfoundry.sipxconfig.device.DeviceDefaults;
import org.sipfoundry.sipxconfig.setting.SettingEntry;

public class CiscoAtaDefaults {
    private static final String VOICEMAIL_PATH = "caller/VoiceMailNumber";
    private static final String TIMEZONE_SETTING = "service/TimeZone";
    private static final String TFTP_PATH = "network/TftpURL";

    private DeviceDefaults m_defaults;

    public CiscoAtaDefaults(DeviceDefaults defaults) {
        m_defaults = defaults;
    }

    @SettingEntry(path = TIMEZONE_SETTING)
    public int getTimeZoneOffset() {
        int tzmin = m_defaults.getTimeZone().getOffsetWithDst() / 60;
        int atatz;

        if (tzmin % 60 == 0) {
            atatz = tzmin / 60;
            if (atatz < 0) {
                atatz += 25;
            }
        } else {
            atatz = tzmin;
        }

        return atatz;
    }

    @SettingEntry(path = TFTP_PATH)
    public String getTftpServer() {
        return m_defaults.getTftpServer();
    }

    @SettingEntry(path = VOICEMAIL_PATH)
    public String getVoiceMailNumber() {
        return m_defaults.getVoiceMail();
    }
    
    @SettingEntry(path = "network/NTPIP")
    public String getNtpServer() {
        return m_defaults.getNtpServer();
    }

    @SettingEntry(path = "network/AltNTPIP")
    public String getAlternateNtpServer() {
        return m_defaults.getAlternateNtpServer();
    }
}
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
package org.sipfoundry.sipxconfig.admin.commserver.imdb;

import java.util.Iterator;
import java.util.List;
import java.util.Map;

import org.apache.commons.lang.enums.Enum;

/**
 * Each enum corresponds to a single IMDB table
 */
public class DataSet extends Enum {
    public static final DataSet EXTENSION = new DataSet("extension");
    public static final DataSet ALIAS = new DataSet("alias");
    public static final DataSet CREDENTIAL = new DataSet("credential");
    public static final DataSet PERMISSION = new DataSet("permission");
    public static final DataSet AUTH_EXCEPTION = new DataSet("authexception");
    public static final DataSet CALLER_ALIAS = new DataSet("caller-alias");

    public DataSet(String dataSet) {
        super(dataSet);
    }

    /**
     * @return name that is used in spring to register the bean of this type
     */
    public String getBeanName() {
        return getName() + "DataSet";
    }

    public static DataSet getEnum(String name) {
        return (DataSet) getEnum(DataSet.class, name);
    }

    public static Map getEnumMap() {
        return getEnumMap(DataSet.class);
    }

    public static List getEnumList() {
        return getEnumList(DataSet.class);
    }

    public static Iterator iterator() {
        return iterator(DataSet.class);
    }
}
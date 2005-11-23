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
package org.sipfoundry.sipxconfig.search;

import java.util.List;

import org.apache.commons.collections.Transformer;

public interface SearchManager {
    String CONTEXT_BEAN_NAME = "searchManager";

    List search(String query, Transformer transformer);

    List search(Class entityClass, String query, Transformer transformer);

    /**
     * Search for an indexed item using class and user entered query.
     * 
     * @param entityClass class of the entity we are searching for
     * @param query lucene query that is ANDed with entityClass query
     * @param firstResult index of the first found result
     * @param pageSize
     * @param sort field that the result is sorted
     * @param orderAscending if true use ascending order when sorting, if false use descending
     *        order
     * @param transformer commons collections compatible transformed - if provided it'll be
     *        applied to each found item before the results are returned
     * @return collection of BeanAdapter.Identity objects - if no transformer provided,
     *         transformed collection if there was a transformer
     */
    List search(Class entityClass, String query, int firstResult, int pageSize, String sort,
            boolean orderAscending, Transformer transformer);
}

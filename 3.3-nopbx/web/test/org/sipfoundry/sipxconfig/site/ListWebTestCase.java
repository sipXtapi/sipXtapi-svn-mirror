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
package org.sipfoundry.sipxconfig.site;

import net.sourceforge.jwebunit.ExpectedRow;
import net.sourceforge.jwebunit.ExpectedTable;
import net.sourceforge.jwebunit.WebTestCase;

import org.apache.commons.lang.ArrayUtils;
import org.apache.commons.lang.StringUtils;

import com.meterware.httpunit.WebForm;

/**
 * Support for testing screens (or parts of the screen) that display table with add, delete, move
 * etc. capability
 * 
 */
public abstract class ListWebTestCase extends WebTestCase {

    /**
     * id of the link on the home page that needs to be pressed before every test to reset the
     * environent
     */
    private final String m_resetLink;

    /** prefix for page components - callgroup:list is a table etc. */
    private final String m_idPrefix;

    private final String m_pageLink;

    private boolean m_hasDuplicate = true;
    private boolean m_hasMove = false;
    private boolean m_exactCheck = true;

    public ListWebTestCase(String pageLink, String resetLink, String idPrefix) {
        m_pageLink = pageLink;
        m_idPrefix = idPrefix;
        m_resetLink = resetLink;
    }

    public void setUp() {
        getTestContext().setBaseUrl(SiteTestHelper.getBaseUrl());
        SiteTestHelper.home(getTester());
        clickLink(m_resetLink);
        clickLink(m_pageLink);
    }

    /**
     * Overrite to create an array of the form parameter names.
     * 
     * These names will be set on the add form for the item. Corresponding values have to be
     * provided by getParamValues function.
     * 
     */
    protected abstract String[] getParamNames();

    /**
     * Returns a set of values corresponding to set of names returned by the first function.
     * 
     * @param i - to diversify the values - make it part of the value in some way to make tests
     *        more robust.
     */
    protected abstract String[] getParamValues(int i);

    /**
     * Returns the table row view of the tested item.
     * 
     * By default table displays exactly the same values as entered on add form - override it to
     * change it.
     * 
     * @param paramValues generated by getParamValues function
     * @return array of values that correspond to what we need to see in the table
     */
    protected Object[] getExpectedTableRow(String[] paramValues) {
        return paramValues;
    }

    protected String buildId(String id) {
        return m_idPrefix + ":" + id;
    }

    protected String buildEditId(String id) {
        return StringUtils.chop(m_idPrefix) + ":" + id;
    }

    // common tests
    public void testDisplay() {
        SiteTestHelper.assertNoException(tester);
        assertFormPresent();
        assertLinkPresent(buildId("add"));
        assertEquals(1, SiteTestHelper.getRowCount(tester, getTableId()));
        assertButtonPresent(buildId("delete"));

        if (m_hasDuplicate) {
            assertButtonPresent(buildId("duplicate"));
        }
        if (m_hasMove) {
            assertButtonPresent(buildId("moveUp"));
            assertButtonPresent(buildId("moveDown"));
        }
    }

    public void testAdd() throws Exception {
        final int count = 5;
        ExpectedTable expected = new ExpectedTable();
        for (int i = 0; i < count; i++) {
            String[] values = getParamValues(i);
            addItem(getParamNames(), values);
            expected.appendRow(new ExpectedRow(getExpectedTableRow(values)));
        }
        assertEquals(count + 1, SiteTestHelper.getRowCount(tester, getTableId()));
        if (m_exactCheck) {
            assertTableRowsEqual(getTableId(), 1, expected);
        } else {
            assertTableRowsExist(getTableId(), expected);
        }
    }

    public void testEdit() throws Exception {
        String[] values = getParamValues(7);
        addItem(getParamNames(), values);

        // click on name - it should take us to the edit page
        clickLinkWithText(values[0]);

        String[] names = getParamNames();
        for (int i = 0; i < names.length; i++) {
            assertFormElementEquals(names[i], values[i]);
        }
    }

    public void testDelete() throws Exception {
        final int[] toBeRemoved = {
            2, 4
        };
        final int count = 5;
        ExpectedTable expected = new ExpectedTable();

        for (int i = 0; i < count; i++) {
            String[] values = getParamValues(i);
            addItem(getParamNames(), values);
            if (!ArrayUtils.contains(toBeRemoved, i)) {
                expected.appendRow(new ExpectedRow(getExpectedTableRow(values)));
            }
        }
        // remove 2nd and 4th
        for (int i = 0; i < toBeRemoved.length; i++) {
            SiteTestHelper.selectRow(tester, toBeRemoved[i], true);
        }

        clickDeleteButton();

        SiteTestHelper.assertNoUserError(tester);
        SiteTestHelper.assertNoException(tester);

        assertEquals(count + 1 - toBeRemoved.length, SiteTestHelper.getRowCount(tester,
                getTableId()));
        if (m_exactCheck) {
            assertTableRowsEqual(getTableId(), 1, expected);
        }
    }

    protected void clickDeleteButton() {
        clickButton(buildId("delete"));
    }

    protected final void addItem(String[] names, String[] values) throws Exception {
        SiteTestHelper.assertNoException(tester);
        assertEquals(names.length, values.length);
        clickAddLink();
        SiteTestHelper.assertNoException(tester);
        SiteTestHelper.assertNoUserError(tester);
        setAddParams(names, values);
        clickButton("form:ok");
        SiteTestHelper.assertNoException(tester);
        SiteTestHelper.assertNoUserError(tester);
    }

    protected void clickAddLink() throws Exception {
        clickLink(buildId("add"));
    }

    private void assertTableRowsExist(String tableId, ExpectedTable expected) {
        for (int i = 0; i < expected.getExpectedStrings().length; i++) {
            assertTextInTable(tableId, expected.getExpectedStrings()[i][0]);
        }
    }

    /**
     * Overwrite this to set any aditional params
     * 
     * @param names
     * @param values
     */
    protected void setAddParams(String[] names, String[] values) {
        WebForm form = getDialog().getForm();
        for (int i = 0; i < names.length; i++) {
            form.setParameter(names[i], values[i]);
        }
    }

    public void setHasDuplicate(boolean hasDuplicate) {
        m_hasDuplicate = hasDuplicate;
    }

    public void setHasMove(boolean hasMove) {
        m_hasMove = hasMove;
    }

    public void setExactCheck(boolean exactCheck) {
        m_exactCheck = exactCheck;
    }

    protected String getFormId() {
        return buildId("form");
    }

    protected String getTableId() {
        return buildId("list");
    }
}

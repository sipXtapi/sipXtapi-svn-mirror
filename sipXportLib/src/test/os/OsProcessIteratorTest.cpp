//
// Copyright (C) 2007-2010 SIPez LLC  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#include <os/OsProcess.h>
#include <os/OsStatus.h>
#include <sipxunittests.h>

class OsProcessIteratorTest : public SIPX_UNIT_BASE_CLASS
{
    CPPUNIT_TEST_SUITE(OsProcessIteratorTest);
    CPPUNIT_TEST(testIterator);
    CPPUNIT_TEST_SUITE_END();


public:

    /**
     * Just excersizes AIP. Unclear how to create pass/fail tests
     */
    void testIterator()
    {
        OsStatus stat;
        OsProcess process;
        OsProcessIterator pi;

        stat = pi.findFirst(process);
        CPPUNIT_ASSERT_MESSAGE("First process", stat == OS_SUCCESS);

        while (stat == OS_SUCCESS)
        {
            UtlString name;
            process.getProcessName(name);
            #ifdef WIN32
            /*on Windows, the system process is pid 0 */
            CPPUNIT_ASSERT_MESSAGE("Valid PID",process.getPID() >= 0);
            #else
            CPPUNIT_ASSERT_MESSAGE("Valid PID", process.getPID() != 0);
            #endif
            CPPUNIT_ASSERT_MESSAGE("Valid Parent PID", process.getParentPID() >= 0);
            CPPUNIT_ASSERT_MESSAGE("Valid process name", name.data() != NULL);
            
            stat = pi.findNext(process);
        }
    }
};

#ifdef WINCE
#pragma message( "OsProcessIteratorTest disabled undef Win CE" )
#else
CPPUNIT_TEST_SUITE_REGISTRATION(OsProcessIteratorTest);
#endif

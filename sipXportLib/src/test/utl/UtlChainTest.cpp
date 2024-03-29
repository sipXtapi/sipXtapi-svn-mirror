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

#include <os/OsIntTypes.h>
#include <sipxunittests.h>
#include "utl/UtlLink.h"

/// Unit test of the UtlChain
class UtlChainTest :
   public SIPX_UNIT_BASE_CLASS,
   public UtlChain
{
   CPPUNIT_TEST_SUITE(UtlChainTest);
   CPPUNIT_TEST(testBefore);
   CPPUNIT_TEST(testAfter);   
   CPPUNIT_TEST(testUnchainUnlinked);
   CPPUNIT_TEST(testUnchainFirst);
   CPPUNIT_TEST(testUnchainLast);
   CPPUNIT_TEST(testUnchainMiddle);
   CPPUNIT_TEST(testChainRing);
   CPPUNIT_TEST(testListBefore);
   CPPUNIT_TEST(testListAfter);
   CPPUNIT_TEST_SUITE_END();

private:

public:

   void testBefore()
      {
         UtlChain chain1;
         UtlChain chain2;
         UtlChain chain3;

         // new chains should be properly terminated.
         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == NULL);
         CPPUNIT_ASSERT(chain2.prev == NULL);
         CPPUNIT_ASSERT(chain2.next == NULL);
         CPPUNIT_ASSERT(chain3.prev == NULL);
         CPPUNIT_ASSERT(chain3.next == NULL);

         chain2.chainBefore(&chain3);

         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == NULL);
         CPPUNIT_ASSERT(chain2.prev == NULL);
         CPPUNIT_ASSERT(chain2.next == &chain3);
         CPPUNIT_ASSERT(chain3.prev == &chain2);
         CPPUNIT_ASSERT(chain3.next == NULL);
         
         chain1.chainBefore(&chain2);

         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == &chain2);
         CPPUNIT_ASSERT(chain2.prev == &chain1);
         CPPUNIT_ASSERT(chain2.next == &chain3);
         CPPUNIT_ASSERT(chain3.prev == &chain2);
         CPPUNIT_ASSERT(chain3.next == NULL);
      }
   
   
   void testAfter()
      {
         UtlChain chain1;
         UtlChain chain2;
         UtlChain chain3;

         // new chains should be properly terminated.
         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == NULL);
         CPPUNIT_ASSERT(chain2.prev == NULL);
         CPPUNIT_ASSERT(chain2.next == NULL);
         CPPUNIT_ASSERT(chain3.prev == NULL);
         CPPUNIT_ASSERT(chain3.next == NULL);

         chain2.chainAfter(&chain1);

         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == &chain2);
         CPPUNIT_ASSERT(chain2.prev == &chain1);
         CPPUNIT_ASSERT(chain2.next == NULL);
         CPPUNIT_ASSERT(chain3.prev == NULL);
         CPPUNIT_ASSERT(chain3.next == NULL);
         
         chain3.chainAfter(&chain2);

         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == &chain2);
         CPPUNIT_ASSERT(chain2.prev == &chain1);
         CPPUNIT_ASSERT(chain2.next == &chain3);
         CPPUNIT_ASSERT(chain3.prev == &chain2);
         CPPUNIT_ASSERT(chain3.next == NULL);
      }


   void testUnchainUnlinked()
      {
         UtlChain unlinked;

         CPPUNIT_ASSERT(unlinked.prev == NULL);
         CPPUNIT_ASSERT(unlinked.next == NULL);

         unlinked.unchain();

         CPPUNIT_ASSERT(unlinked.prev == NULL);
         CPPUNIT_ASSERT(unlinked.next == NULL);
      }
   

   void testUnchainFirst()
      {
         UtlChain chain1;
         UtlChain chain2;
         UtlChain chain3;

         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == NULL);
         CPPUNIT_ASSERT(chain2.prev == NULL);
         CPPUNIT_ASSERT(chain2.next == NULL);
         CPPUNIT_ASSERT(chain3.prev == NULL);
         CPPUNIT_ASSERT(chain3.next == NULL);

         chain2.chainBefore(&chain3);
         chain1.chainBefore(&chain2);

         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == &chain2);
         CPPUNIT_ASSERT(chain2.prev == &chain1);
         CPPUNIT_ASSERT(chain2.next == &chain3);
         CPPUNIT_ASSERT(chain3.prev == &chain2);
         CPPUNIT_ASSERT(chain3.next == NULL);

         chain1.unchain();
         
         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == NULL);
         CPPUNIT_ASSERT(chain2.prev == NULL);
         CPPUNIT_ASSERT(chain2.next == &chain3);
         CPPUNIT_ASSERT(chain3.prev == &chain2);
         CPPUNIT_ASSERT(chain3.next == NULL);
      }


   void testUnchainLast()
      {
         UtlChain chain1;
         UtlChain chain2;
         UtlChain chain3;

         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == NULL);
         CPPUNIT_ASSERT(chain2.prev == NULL);
         CPPUNIT_ASSERT(chain2.next == NULL);
         CPPUNIT_ASSERT(chain3.prev == NULL);
         CPPUNIT_ASSERT(chain3.next == NULL);

         chain2.chainBefore(&chain3);
         chain1.chainBefore(&chain2);

         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == &chain2);
         CPPUNIT_ASSERT(chain2.prev == &chain1);
         CPPUNIT_ASSERT(chain2.next == &chain3);
         CPPUNIT_ASSERT(chain3.prev == &chain2);
         CPPUNIT_ASSERT(chain3.next == NULL);

         chain3.unchain();
         
         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == &chain2);
         CPPUNIT_ASSERT(chain2.prev == &chain1);
         CPPUNIT_ASSERT(chain2.next == NULL);
         CPPUNIT_ASSERT(chain3.prev == NULL);
         CPPUNIT_ASSERT(chain3.next == NULL);
      }

   
   void testUnchainMiddle()
      {
         UtlChain chain1;
         UtlChain chain2;
         UtlChain chain3;

         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == NULL);
         CPPUNIT_ASSERT(chain2.prev == NULL);
         CPPUNIT_ASSERT(chain2.next == NULL);
         CPPUNIT_ASSERT(chain3.prev == NULL);
         CPPUNIT_ASSERT(chain3.next == NULL);

         chain2.chainBefore(&chain3);
         chain1.chainBefore(&chain2);

         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == &chain2);
         CPPUNIT_ASSERT(chain2.prev == &chain1);
         CPPUNIT_ASSERT(chain2.next == &chain3);
         CPPUNIT_ASSERT(chain3.prev == &chain2);
         CPPUNIT_ASSERT(chain3.next == NULL);

         chain2.unchain();
         
         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == &chain3);
         CPPUNIT_ASSERT(chain2.prev == NULL);
         CPPUNIT_ASSERT(chain2.next == NULL);
         CPPUNIT_ASSERT(chain3.prev == &chain1);
         CPPUNIT_ASSERT(chain3.next == NULL);
      }

   void testChainRing()
      {
         UtlChain chain1;
         UtlChain chain2;
         UtlChain chain3;

         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == NULL);
         CPPUNIT_ASSERT(chain2.prev == NULL);
         CPPUNIT_ASSERT(chain2.next == NULL);
         CPPUNIT_ASSERT(chain3.prev == NULL);
         CPPUNIT_ASSERT(chain3.next == NULL);

         chain1.chainBefore(&chain1);
         
         CPPUNIT_ASSERT(chain1.prev == &chain1);
         CPPUNIT_ASSERT(chain1.next == &chain1);
         CPPUNIT_ASSERT(chain2.prev == NULL);
         CPPUNIT_ASSERT(chain2.next == NULL);
         CPPUNIT_ASSERT(chain3.prev == NULL);
         CPPUNIT_ASSERT(chain3.next == NULL);

         chain2.chainAfter(&chain1);

         CPPUNIT_ASSERT(chain1.prev == &chain2);
         CPPUNIT_ASSERT(chain1.next == &chain2);
         CPPUNIT_ASSERT(chain2.prev == &chain1);
         CPPUNIT_ASSERT(chain2.next == &chain1);
         CPPUNIT_ASSERT(chain3.prev == NULL);
         CPPUNIT_ASSERT(chain3.next == NULL);

         chain3.chainBefore(&chain1);

         CPPUNIT_ASSERT(chain1.prev == &chain3);
         CPPUNIT_ASSERT(chain1.next == &chain2);
         CPPUNIT_ASSERT(chain2.prev == &chain1);
         CPPUNIT_ASSERT(chain2.next == &chain3);
         CPPUNIT_ASSERT(chain3.prev == &chain2);
         CPPUNIT_ASSERT(chain3.next == &chain1);

         chain3.unchain();

         CPPUNIT_ASSERT(chain1.prev == &chain2);
         CPPUNIT_ASSERT(chain1.next == &chain2);
         CPPUNIT_ASSERT(chain2.prev == &chain1);
         CPPUNIT_ASSERT(chain2.next == &chain1);
         CPPUNIT_ASSERT(chain3.prev == NULL);
         CPPUNIT_ASSERT(chain3.next == NULL);

         chain1.unchain();

         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == NULL);
         CPPUNIT_ASSERT(chain2.prev == &chain2);
         CPPUNIT_ASSERT(chain2.next == &chain2);
         CPPUNIT_ASSERT(chain3.prev == NULL);
         CPPUNIT_ASSERT(chain3.next == NULL);

         chain2.unchain();

         CPPUNIT_ASSERT(chain1.prev == NULL);
         CPPUNIT_ASSERT(chain1.next == NULL);
         CPPUNIT_ASSERT(chain2.prev == NULL);
         CPPUNIT_ASSERT(chain2.next == NULL);
         CPPUNIT_ASSERT(chain3.prev == NULL);
         CPPUNIT_ASSERT(chain3.next == NULL);
      }

   void testListBefore()
      {
         UtlChain list;
         
         UtlChain link1;
         
         link1.listBefore(&list, NULL);

         CPPUNIT_ASSERT(link1.prev == NULL);
         CPPUNIT_ASSERT(link1.next == NULL);
         CPPUNIT_ASSERT(list.prev == &link1);
         CPPUNIT_ASSERT(list.next == &link1);

         UtlChain link3;

         link3.listBefore(&list, NULL);

         CPPUNIT_ASSERT(list.next == &link1);
         CPPUNIT_ASSERT(link1.prev == NULL);
         CPPUNIT_ASSERT(link1.next == &link3);
         CPPUNIT_ASSERT(link3.prev == &link1);
         CPPUNIT_ASSERT(link3.next == NULL);
         CPPUNIT_ASSERT(list.prev == &link3);

         UtlChain link2;
         link2.listBefore(&list, &link3);

         CPPUNIT_ASSERT(list.next == &link1);
         CPPUNIT_ASSERT(link1.prev == NULL);
         CPPUNIT_ASSERT(link1.next == &link2);
         CPPUNIT_ASSERT(link2.prev == &link1);
         CPPUNIT_ASSERT(link2.next == &link3);
         CPPUNIT_ASSERT(link3.prev == &link2);
         CPPUNIT_ASSERT(link3.next == NULL);
         CPPUNIT_ASSERT(list.prev == &link3);

         void* returnedLink;

         returnedLink = link2.detachFromList(&list);
         CPPUNIT_ASSERT(returnedLink == &link2);
         
         CPPUNIT_ASSERT(list.next == &link1);
         CPPUNIT_ASSERT(link1.prev == NULL);
         CPPUNIT_ASSERT(link1.next == &link3);
         CPPUNIT_ASSERT(link3.prev == &link1);
         CPPUNIT_ASSERT(link3.next == NULL);
         CPPUNIT_ASSERT(list.prev == &link3);

         returnedLink = link1.detachFromList(&list);
         CPPUNIT_ASSERT(returnedLink == &link1);

         CPPUNIT_ASSERT(list.next == &link3);
         CPPUNIT_ASSERT(link3.prev == NULL);
         CPPUNIT_ASSERT(link3.next == NULL);
         CPPUNIT_ASSERT(list.prev == &link3);

         returnedLink = link3.detachFromList(&list);
         CPPUNIT_ASSERT(returnedLink == &link3);

         CPPUNIT_ASSERT(list.prev == NULL);
         CPPUNIT_ASSERT(list.next == NULL);
      }

   void testListAfter()
      {
         UtlChain list;

         UtlChain link3;

         link3.listAfter(&list, NULL);

         CPPUNIT_ASSERT(list.next == &link3);
         CPPUNIT_ASSERT(link3.prev == NULL);
         CPPUNIT_ASSERT(link3.next == NULL);
         CPPUNIT_ASSERT(list.prev == &link3);

         UtlChain link1;

         link1.listAfter(&list, NULL);

         CPPUNIT_ASSERT(list.next == &link1);
         CPPUNIT_ASSERT(link1.prev == NULL);
         CPPUNIT_ASSERT(link1.next == &link3);
         CPPUNIT_ASSERT(link3.prev == &link1);
         CPPUNIT_ASSERT(link3.next == NULL);
         CPPUNIT_ASSERT(list.prev == &link3);

         UtlChain link2;

         link2.listAfter(&list, &link1);

         CPPUNIT_ASSERT(list.next == &link1);
         CPPUNIT_ASSERT(link1.prev == NULL);
         CPPUNIT_ASSERT(link1.next == &link2);
         CPPUNIT_ASSERT(link2.prev == &link1);
         CPPUNIT_ASSERT(link2.next == &link3);
         CPPUNIT_ASSERT(link3.prev == &link2);
         CPPUNIT_ASSERT(link3.next == NULL);
         CPPUNIT_ASSERT(list.prev == &link3);

         void* returnedLink;

         returnedLink = link2.detachFromList(&list);

         CPPUNIT_ASSERT(returnedLink == &link2);

         CPPUNIT_ASSERT(list.next == &link1);
         CPPUNIT_ASSERT(link1.prev == NULL);
         CPPUNIT_ASSERT(link1.next == &link3);
         CPPUNIT_ASSERT(link3.prev == &link1);
         CPPUNIT_ASSERT(link3.next == NULL);
         CPPUNIT_ASSERT(list.prev == &link3);

         returnedLink = link1.detachFromList(&list);

         CPPUNIT_ASSERT(returnedLink == &link1);

         CPPUNIT_ASSERT(list.next == &link3);
         CPPUNIT_ASSERT(link3.prev == NULL);
         CPPUNIT_ASSERT(link3.next == NULL);
         CPPUNIT_ASSERT(list.prev == &link3);

         returnedLink = link3.detachFromList(&list);

         CPPUNIT_ASSERT(returnedLink == &link3);

         CPPUNIT_ASSERT(list.prev == NULL);
         CPPUNIT_ASSERT(list.next == NULL);
      }


};

CPPUNIT_TEST_SUITE_REGISTRATION(UtlChainTest);

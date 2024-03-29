//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef _SipLSendCommand_h_
#define _SipLSendCommand_h_

// SYSTEM INCLUDES
//#include <...>

// APPLICATION INCLUDES
#include <siptest/Command.h>
#include <net/SipUserAgent.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

//:Class short description which may consist of multiple lines (note the ':')
// Class detailed description which may extend to multiple lines
class SipLSendCommand : public Command
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

/* ============================ CREATORS ================================== */

   SipLSendCommand();
     //:Default constructor

   virtual
   ~SipLSendCommand();
     //:Destructor

/* ============================ MANIPULATORS ============================== */

   virtual int execute(int argc, char* argv[]);

/* ============================ ACCESSORS ================================= */

   virtual void getUsage(const char* commandName, UtlString* usage) const;

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:


        SipLSendCommand(const SipLSendCommand& rSipLSendCommand);
        //:Copy constructor
        SipLSendCommand& operator=(const SipLSendCommand& rhs);
        //:Assignment operator

#ifdef TEST
   static bool sIsTested;
     //:Set to true after the tests for this class have been executed once

   void test();
     //:Verify assertions for this class

   // Test helper functions
   void testCreators();
   void testManipulators();
   void testAccessors();
   void testInquiry();

#endif //TEST
};

/* ============================ INLINE METHODS ============================ */

#endif  // _SipLSendCommand_h_

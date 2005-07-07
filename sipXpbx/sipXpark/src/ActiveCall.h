// 
// 
// Copyright (C) 2005 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
// 
// Copyright (C) 2005 Pingtel Corp.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// $$
////////////////////////////////////////////////////////////////////////////

#ifndef _ActiveCall_h_
#define _ActiveCall_h_

// SYSTEM INCLUDES

// APPLICATION INCLUDES
#include <ParkedCallObject.h>
#include <utl/UtlContainable.h>

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
class ActiveCall : public UtlContainable
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

/* ============================ CREATORS ================================== */

   ActiveCall(UtlString& callId);

   ActiveCall(UtlString& callId, ParkedCallObject* call);
   
   ~ActiveCall();

   virtual UtlContainableType getContainableType() const;

   virtual unsigned int hash() const;

   int compareTo(const UtlContainable *b) const;

   ParkedCallObject* getCallObject() { return mpCall; };

/* ============================ MANIPULATORS ============================== */

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   
   UtlString mCallId;
   ParkedCallObject* mpCall;
};

/* ============================ INLINE METHODS ============================ */

#endif  // _ActiveCall_h_

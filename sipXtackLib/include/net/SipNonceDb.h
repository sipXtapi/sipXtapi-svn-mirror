// 
// 
// Copyright (C) 2004 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
// 
// Copyright (C) 2004 Pingtel Corp.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// $$
//////////////////////////////////////////////////////////////////////////////

#ifndef _SipNonceDb_h_
#define _SipNonceDb_h_

// SYSTEM INCLUDES

// APPLICATION INCLUDES
#include <utl/UtlString.h>
#include <utl/UtlHashBag.h>




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
class SipNonceDb
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

/* ============================ CREATORS ================================== */

   SipNonceDb();
     //:Default constructor

   virtual
   ~SipNonceDb();
     //:Destructor

/* ============================ MANIPULATORS ============================== */

   void createNewNonce(const UtlString& callId, //input
                       const UtlString& fromTag, // input
                       const UtlString& uri, // input
                       const UtlString& realm, // input
                       UtlString& nonce); // output

   void removeOldNonces(long oldTime);

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

   UtlBoolean isNonceValid(const UtlString& nonce,
                          const UtlString& callId,
                          const UtlString& fromTag,
                          const UtlString& uri,
                          const UtlString& realm,
                          const long expiredTime);

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   SipNonceDb(const SipNonceDb& rSipNonceDb);
     //:Copy constructor

   SipNonceDb& operator=(const SipNonceDb& rhs);
     //:Assignment operator

   UtlString nonceSignature(const UtlString& callId,
						   const UtlString& uri,
						   const UtlString& fromTag,
                           const UtlString& realm,
						   const UtlString& timestamp
                           );

   UtlHashBag mNonceHash;
   UtlString    mNonceSignatureSecret;

};

/* ============================ INLINE METHODS ============================ */

#endif  // _SipNonceDb_h_

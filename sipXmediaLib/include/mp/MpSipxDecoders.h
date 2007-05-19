//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////


#ifndef _MpSipxDecoders_h_
#define _MpSipxDecoders_h_

// SYSTEM INCLUDES

// APPLICATION INCLUDES
// #include "mp/MpBuf.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS

// FORWARD DECLARATIONS

//:class for managing dejitter/decode of incoming RTP.
class MpSipxDecoder
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

/* ============================ CREATORS ================================== */

   MpSipxDecoder(void);
     //:Constructor
     // Returns a new decoder object.
     ////!param: ARGs - (in? out?) What?

   virtual
   ~MpSipxDecoder(void);
     //:Destructor

/* ============================ MANIPULATORS ============================== */

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   MpSipxDecoder(const MpSipxDecoder& rMpSipxDecoder);
     //:Copy constructor

   MpSipxDecoder& operator=(const MpSipxDecoder& rhs);
     //:Assignment operator

   //int mPayloadType;
};

#ifdef __cplusplus
extern "C" {
#endif

extern int G711A_Decoder(int N, JB_uchar* S, Sample* D);
extern int G711U_Decoder(int N, JB_uchar* S, Sample* D);

#ifdef __cplusplus
}
#endif

#endif  // _MpSipxDecoders_h_

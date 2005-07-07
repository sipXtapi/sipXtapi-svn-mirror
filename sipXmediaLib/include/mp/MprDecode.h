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
#ifndef _MprDecode_h_ /* [ */
#define _MprDecode_h_

#include "mp/MpMisc.h"

// SYSTEM INCLUDES

// APPLICATION INCLUDES
#include "mp/MpResource.h"
#include "mp/MprDejitter.h"
#include "mp/MpFlowGraphMsg.h"
#include "net/SdpCodec.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS

// FORWARD DECLARATIONS
class MpConnection;
class MpDecoderBase;
class MprRecorder;

//:The "Decode" media processing resource
class MprDecode : public MpResource
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   friend class MpConnection;

   /* This was 3 in the past, which is probably far too pessimistic most of
    * the time.  1 is probably fine on a LAN or with only a router or two.
    * Let's leave it at 3 for now...
    */
   /* ... that was then, this is now.  VON is coming, squeeze it down! */
   /* ... VON is coming again, and so is NetEQ.  Make it at least 5, or lose */
   enum { MIN_RTP_PACKETS = 5};


/* ============================ CREATORS ================================== */

   MprDecode(const UtlString& rName, MpConnection* pConn,
      int samplesPerFrame, int samplesPerSec);
     //:Constructor

   virtual
   ~MprDecode();
     //:Destructor

/* ============================ MANIPULATORS ============================== */

OsStatus selectCodecs(SdpCodec* codecs[], int numCodecs);

OsStatus selectCodec(SdpCodec& rCodec);

OsStatus deselectCodec(void);

void setMyDejitter(MprDejitter* newDJ);

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   UtlBoolean handleSetDtmfNotify(OsNotification* n);
     //:Handle the FLOWGRAPH_SET_DTMF_NOTIFY message.
     // Returns TRUE

   UtlBoolean setDtmfTerm(MprRecorder *pRecorder);

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   enum AddlMsgTypes
   {
      SELECT_CODECS = MpFlowGraphMsg::RESOURCE_SPECIFIC_START,
      DESELECT_CODECS,
   };

   enum { MAX_RTP_FRAMES = 25};

   unsigned int mTimeStampOffset;
   int          mPreloading;

   MprDejitter* mpMyDJ;

   MpDecoderBase** mpCurrentCodecs;
   int             mNumCurrentCodecs;

   MpDecoderBase** mpPrevCodecs;
   int             mNumPrevCodecs;

   MpConnection*   mpConnection;

   enum {
      MAX_MARKER_NOTICES = 5, // max messages per MARKER_WAIT_FRAMES interval
      MARKER_WAIT_FRAMES = (1*60*60*100) // 1 hour at 100 frames/second
   };
   int   mNumMarkerNotices;
   int   mFrameLastMarkerNotice;
   int   mFrameCounter;

   enum {
      NUM_TRACKED_PACKETS = 128
   };

   virtual UtlBoolean doProcessFrame(MpBufPtr inBufs[],
                                    MpBufPtr outBufs[],
                                    int inBufsSize,
                                    int outBufsSize,
                                    UtlBoolean isEnabled,
                                    int samplesPerFrame=80,
                                    int samplesPerSecond=8000);

   UtlBoolean isPayloadTypeSupported(int payloadType);

   virtual UtlBoolean handleMessage(MpFlowGraphMsg& rMsg);
     //:Handle messages for this resource.

   UtlBoolean handleSelectCodecs(SdpCodec* pCodecs[], int numCodecs);

   UtlBoolean handleDeselectCodecs(void);
   UtlBoolean handleDeselectCodec(MpDecoderBase* pDecoder);

   MprDecode(const MprDecode& rMprDecode);
     //:Copy constructor (not implemented for this class)

   MprDecode& operator=(const MprDecode& rhs);
     //:Assignment operator (not implemented for this class)

   MprDejitter* getMyDejitter(void);

   void pushIntoJitterBuffer(MpBufPtr rtp, int packetLen);

};

/* ============================ INLINE METHODS ============================ */

#endif  /* _MprDecode_h_ ] */

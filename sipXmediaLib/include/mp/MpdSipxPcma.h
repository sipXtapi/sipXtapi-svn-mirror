//
// Copyright (C) 2006 SIPez LLC.
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


#ifndef _MpdSipxPcma_h_
#define _MpdSipxPcma_h_

// SYSTEM INCLUDES

// APPLICATION INCLUDES
#include "mp/MpDecoderBase.h"
#include "mp/JB/jb_typedefs.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS

/// Derived class for G.711 a-Law decoder.
class MpdSipxPcma: public MpDecoderBase
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

/* ============================ CREATORS ================================== */

     /// Constructor
   MpdSipxPcma(int payloadType);
     /**<
     *  @param payloadType - (in) RTP payload type associated with this decoder
     */

     /// Destructor
   virtual ~MpdSipxPcma();

     /// Initializes a codec data structure for use as a decoder
   virtual OsStatus initDecode(MpAudioConnection* pConnection);
     /**<
     *  @param pConnection - (in) Pointer to the MpAudioConnection container
     *
     *  @returns <b>OS_SUCCESS</b> - Success
     *  @returns <b>OS_NO_MEMORY</b> - Memory allocation failure
     */

     /// Frees all memory allocated to the decoder by <i>initDecode</i>
   virtual OsStatus freeDecode();
     /**<
     *  @returns <b>OS_SUCCESS</b> - Success
     *  @returns <b>OS_DELETED</b> - Object has already been deleted
     */

//@}

/* ============================ MANIPULATORS ============================== */
///@name Manipulators
//@{

     /// Receive a packet of RTP data
   virtual int decodeIn(const MpRtpBufPtr &pPacket ///< (in) Pointer to a media buffer
                       );
     /**<
     *  @note This method can be called more than one time per frame interval.
     *
     *  @returns >0 - length of packet to hand to jitter buffer.
     *  @returns 0  - decoder don't want more packets.
     *  @returns -1 - discard packet (e.g. out of order packet).
     */

     /// Decode incoming RTP packet
   virtual int decode(const MpRtpBufPtr &pPacket, ///< (in) Pointer to a media buffer
                      unsigned decodedBufferLength, ///< (in) Length of the samplesBuffer (in samples)
                      MpAudioSample *samplesBuffer ///< (out) Buffer for decoded samples
                     );
     /**<
     *  @return Number of decoded samples.
     */

     /// @brief This method allows a codec to take action based on the length of
     /// the jitter buffer since last asked.
   virtual int reportBufferLength(int iAvePackets);

     /// DOCME
   virtual void frameIncrement();

//@}

/* ============================ ACCESSORS ================================= */
///@name Accessors
//@{

//@}

/* ============================ INQUIRY =================================== */
///@name Inquiry
//@{

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   static const MpCodecInfo smCodecInfo;  ///< Static information about the codec
   JB_inst* pJBState;
   RtpTimestamp mNextPullTimerCount; ///< Timestamp of frame we expect next.
   unsigned int mWaitTimeInFrames; ///< Size of jitter buffer. Frames will be
                                   ///< delayed for mWaitTimeInFrames*20ms.
   int mUnderflowCount;
   RtpSeq mLastSeqNo;        ///< Keep track of the last sequence number so that
                             ///< we don't take out-of-order packets.
   bool mIsFirstFrame;       ///< True, if no frames decoded.
   bool mClockDrift;         ///< True, if clock drift detected.
   int mLastReportSize;
};

#endif  // _MpdSipxPcma_h_

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


#ifndef HAVE_GIPS /* [ */

// APPLICATION INCLUDES
#include "mp/MpAudioConnection.h"
#include "mp/MpdSipxPcma.h"
#include "mp/JB/JB_API.h"
#include "mp/MprDejitter.h"
#include "mp/MpSipxDecoders.h"
#include "mp/MpMisc.h"

const MpCodecInfo MpdSipxPcma::smCodecInfo(
         SdpCodec::SDP_CODEC_PCMA, JB_API_VERSION, true,
         8000, 8, 1, 160, 64000, 1280, 1280, 1280, 160, 3);

MpdSipxPcma::MpdSipxPcma(int payloadType)
: MpDecoderBase(payloadType, &smCodecInfo),
  mNextPullTimerCount(0),
  mWaitTimeInFrames(6),  // This is the jitter buffer size. 6 = 120ms 
  mUnderflowCount(0),
  mLastSeqNo(0),
  mIsFirstFrame(false),
  mClockDrift(false),
  mLastReportSize(-1)
{
}

MpdSipxPcma::~MpdSipxPcma()
{
   freeDecode();
}

OsStatus MpdSipxPcma::initDecode(MpAudioConnection* pConnection)
{
   if (pConnection == NULL)
      return OS_SUCCESS;

   //Get JB pointer
   pJBState = pConnection->getJBinst();

   // Set the payload number for JB
   JB_initCodepoint(pJBState, "PCMA", 8000, getPayloadType());

   mNextPullTimerCount = 0;
   mUnderflowCount = 0;
   mLastSeqNo = 0;
   mIsFirstFrame = false;
   mClockDrift = false;
   mLastReportSize = -1;

   return OS_SUCCESS;
}

OsStatus MpdSipxPcma::freeDecode()
{
   return OS_SUCCESS;
}

int MpdSipxPcma::decode(const MpRtpBufPtr &pPacket,
                        unsigned decodedBufferLength,
                        MpAudioSample *samplesBuffer) 
{
   // Assert that available buffer size is enough for the packet.
   if (pPacket->getPayloadSize() > decodedBufferLength)
   {
      osPrintf("MpdSipxPcma::decode: Jitter buffer overloaded. Glitch!\n");
   }

   if (decodedBufferLength == 0)
      return 0;

   int samples = min(pPacket->getPayloadSize(), decodedBufferLength);
   G711A_Decoder(samples,
                 (const JB_uchar*)pPacket->getDataPtr(),
                 samplesBuffer);
   return samples;
}

int MpdSipxPcma::reportBufferLength(int iAvePackets)
{
   // Every second, the decoder interface reports to us the average number of packets in the 
   // jitter buffer. We want to keep it near our target buffer size

   if(iAvePackets <= 1)
      return 0;  // Zero or one is the starting condition

   int iMax = mWaitTimeInFrames+2;
   int iMin = mWaitTimeInFrames-1; 

   if (iMin < 1)
      iMin = 1;

   if (mLastReportSize == -1)
      mLastReportSize = iAvePackets;

   if (iAvePackets < iMin)
   {
      // There are too few packets in the buffer.
      if (mLastReportSize-iAvePackets <= 1)
      {
         // Only react when the buffer length changes mildly, not dramatically,
         // since the latter probably isn't clock drift
         mClockDrift = true;
      }
   }
   mLastReportSize = iAvePackets;
   return 0;
}

void MpdSipxPcma::frameIncrement() 
{
    // increment the pull timer count one frame's worth
    mNextPullTimerCount+=80;
}

int MpdSipxPcma::decodeIn(const MpRtpBufPtr &pPacket)
{
   RtpTimestamp rtpTimestamp = pPacket->getRtpTimestamp();
   RtpTimestamp delta = 0; // Contain the difference between the current pull
                           // pointer and the rtpTimestamp. Use the delta because
                           // rtpTimestamp and mNextPullTimerCount are unsigned,
                           // so a straight subtraction will fail.

   // If this is our first packet
   if (mIsFirstFrame)
   {
      mIsFirstFrame = false;
      mNextPullTimerCount = rtpTimestamp + (160*(mWaitTimeInFrames*2));
      mLastSeqNo = pPacket->getRtpSequenceNumber();

      // Always accept the first packet
      return pPacket->getPayloadSize();
   }

   // Do not use compare() here, because it is arithmetic compare.
   if (rtpTimestamp > mNextPullTimerCount)
   {
      delta = rtpTimestamp - mNextPullTimerCount;
   } else {
      delta = mNextPullTimerCount - rtpTimestamp;
   }

   if (delta > (160*(mWaitTimeInFrames*2)))
   {
      // Detected timer count silence, skip or stream startup, resetting
      // nextPullTimerCount if we have underflowed the JB, make the reset to
      // a higher value
      if (mClockDrift)
      {
         // Clock drift detected, too few packets in buffer!
         mNextPullTimerCount=rtpTimestamp+(160*(mWaitTimeInFrames*2));
      } else {
         mNextPullTimerCount=rtpTimestamp+(160*mWaitTimeInFrames);
      }

      // Throw out this packet and stop frame processing for this frame.
      return 0;
   }

   if (compare(rtpTimestamp, mNextPullTimerCount) <= 0) {
      // A packet is available within the allotted time span
      mUnderflowCount=0;
      // Process the frame if enough time has passed
      RtpSeq iSeqNo = pPacket->getRtpSequenceNumber();
      if (compare(iSeqNo, mLastSeqNo) < 0)
      {
         // Out of Order Discard
         return -1;  // Discard the packet, it is out of order
      }
      mLastSeqNo = iSeqNo;
      return pPacket->getPayloadSize();
   } else {
      // Count errors if we are not pulling packets for some reason
      mUnderflowCount++;

      return 0;  // We don't want this packet
   }
}

#endif /* HAVE_GIPS ] */


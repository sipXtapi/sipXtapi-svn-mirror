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
 

// SYSTEM INCLUDES

#ifdef __pingtel_on_posix__
#include <stdlib.h>
#endif

// APPLICATION INCLUDES
#include <os/OsQueuedEvent.h>
#include <os/OsTimer.h>
#include <os/OsUtil.h>
#include <net/SipMessageEvent.h>
#include <net/SipUserAgent.h>
#include <net/NameValueTokenizer.h>
#include <net/SdpCodecFactory.h>
#include <net/Url.h>
#include <net/SipSession.h>
#include <mp/dtmflib.h>
#include <cp/SipConnection.h>
#include <cp/CpMediaInterface.h>
#include <cp/CallManager.h>
#include <cp/CpCallManager.h>
#include <cp/CpPeerCall.h>
#include <cp/CpMultiStringMessage.h>
#include <cp/CpIntMessage.h>
#include "ptapi/PtCall.h"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
#define CALL_STATUS_FIELD "status"

#ifdef _WIN32
#   define CALL_CONTROL_TONES
#endif

// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
SipConnection::SipConnection(const char* outboundLineAddress,
                             UtlBoolean isEarlyMediaFor180Enabled,
							 CpCallManager* callMgr,
                             CpCall* call,
                             CpMediaInterface* mediaInterface, 
                             //UiContext* callUiContext,
                             SipUserAgent* sipUA, 
                             int offeringDelayMilliSeconds,
                             int sessionReinviteTimer,
                             int availableBehavior, 
                             const char* forwardUnconditionalUrl,
                             int busyBehavior, 
                             const char* forwardOnBusyUrl) :
Connection(callMgr, call, mediaInterface, 
           //callUiContext, 
           offeringDelayMilliSeconds, 
           availableBehavior, forwardUnconditionalUrl,
           busyBehavior, forwardOnBusyUrl),
mIsEarlyMediaFor180(TRUE)
{
#ifdef TEST_PRINT
    UtlString callId;
    if (call) {
        call->getCallId(callId);
       OsSysLog::add(FAC_CP, PRI_DEBUG, "Entering SipConnection constructor: %s\n", callId.data());
    } else
       OsSysLog::add(FAC_CP, PRI_DEBUG, "Entering SipConnection constructor: call is Null\n");
#endif

   sipUserAgent = sipUA;
   inviteMsg = NULL;
   mReferMessage = NULL;
   lastLocalSequenceNumber = 0;
   lastRemoteSequenceNumber = -1;
   reinviteState = ACCEPT_INVITE;
   mIsEarlyMediaFor180 = isEarlyMediaFor180Enabled;

   // Build a from tag
   int fromTagInt = rand();
   char fromTagBuffer[60];
   sprintf(fromTagBuffer, "%dc%d", call->getCallIndex(), fromTagInt);
   mFromTag = fromTagBuffer;
 
   if(outboundLineAddress)
   {
       mFromUrl = outboundLineAddress;

       // Before adding the from tag, construct the local contact with the 
       // device's NAT friendly contact information (getContactUri).  The host
       // and port from that replaces the public address or record host and 
       // port.  The UserId and URL parameters should be retained.
       UtlString contactHostPort;
       UtlString address;
       Url tempUrl(mFromUrl);
       sipUserAgent->getContactUri(&contactHostPort);
       Url hostPort(contactHostPort);
       hostPort.getHostAddress(address);       
       tempUrl.setHostAddress(address);
       tempUrl.setHostPort(hostPort.getHostPort());
       tempUrl.toString(mLocalContact);

       // Set the from tag in case this is an outbound call
       // If this is an in bound call, the from URL will get
       // over written by the To field from the SIP request
       mFromUrl.setFieldParameter("tag", mFromTag);
   }
  
   mDefaultSessionReinviteTimer = sessionReinviteTimer;
   mSessionReinviteTimer = 0;

#ifdef TEST_PRINT
   osPrintf("SipConnection::mDefaultSessionReinviteTimer = %d\n",
       mDefaultSessionReinviteTimer);
#endif

   mIsReferSent = FALSE;
   mIsAcceptSent = FALSE;

   mbCancelling = FALSE;	// this is the flag that indicates CANCEL is sent 
							// but no response has been received  if set to TRUE

   // State variable which indicates an action to
   // perform after hold has completed.
   mHoldCompleteAction = CpCallManager::CP_UNSPECIFIED;
#ifdef TEST_PRINT
    if (!callId.isNull())
       OsSysLog::add(FAC_CP, PRI_DEBUG, "Leaving SipConnection constructor: %s\n", callId.data());
    else
       OsSysLog::add(FAC_CP, PRI_DEBUG, "Leaving SipConnection constructor: call is Null\n");
#endif
}

// Copy constructor
SipConnection::SipConnection(const SipConnection& rSipConnection)
{
}

// Destructor
SipConnection::~SipConnection()
{    
    UtlString callId;
#ifdef TEST_PRINT
    if (mpCall) {
        mpCall->getCallId(callId);
       OsSysLog::add(FAC_CP, PRI_DEBUG, "Entering SipConnection destructor: %s\n", callId.data());
    } else
       OsSysLog::add(FAC_CP, PRI_DEBUG, "Entering SipConnection destructor: call is Null\n");
#endif

	if(inviteMsg)
	{
		delete inviteMsg;
		inviteMsg = NULL;
	}
    if(mReferMessage)
    {
        delete mReferMessage;
        mReferMessage = NULL;
    }
#ifdef TEST_PRINT
    if (!callId.isNull())
       OsSysLog::add(FAC_CP, PRI_DEBUG, "Leaving SipConnection destructor: %s\n", callId.data());
    else
       OsSysLog::add(FAC_CP, PRI_DEBUG, "Leaving SipConnection destructor: call is Null\n");
#endif
}

/* ============================ MANIPULATORS ============================== */

// Assignment operator
SipConnection& 
SipConnection::operator=(const SipConnection& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;

   return *this;
}


UtlBoolean SipConnection::dequeue(UtlBoolean callInFocus)
{
    UtlBoolean connectionDequeued = FALSE;
    osPrintf("Connection::dequeue this: %p inviteMsg: %p\n", this, inviteMsg);
    if(getState() == CONNECTION_QUEUED)
    {
        int tagNum = -1;
        proceedToRinging(inviteMsg, sipUserAgent, tagNum, //callInFocus,
            mLineAvailableBehavior, mpMediaInterface);

        setState(CONNECTION_ALERTING, CONNECTION_LOCAL);

        connectionDequeued = TRUE;
    }

    return(connectionDequeued);
}

UtlBoolean SipConnection::requestShouldCreateConnection(const SipMessage* sipMsg,
                                                        SipUserAgent& sipUa,
                                                        SdpCodecFactory* codecFactory)
{
   UtlBoolean createConnection = FALSE;
   UtlString method;
   sipMsg->getRequestMethod(&method);
   UtlString toField;
   UtlString address;
   UtlString protocol;
   int port;
   UtlString user;
   UtlString userLabel;
   UtlString tag;
   sipMsg->getToAddress(&address, &port, &protocol, &user, &userLabel, &tag);

   // Dangling or delated ACK
   if(method.compareTo(SIP_ACK_METHOD) == 0)
   {
      // Ignore it and do not create a connection
      createConnection = FALSE;
   }

   // INVITE to create a connection
   //if to tag is already set then return 481 error
   else if(method.compareTo(SIP_INVITE_METHOD) == 0 && tag.isNull())
   {
      // Assume the best case, as this will be checked
      // again before the call is answered
      UtlBoolean atLeastOneCodecSupported = TRUE;
      if(codecFactory == NULL ||
          codecFactory->getCodecCount() == 0) 
          atLeastOneCodecSupported = TRUE;

      // Verify that we have some RTP codecs in common
      else
      {
          // Get the SDP and findout if there are any
          // codecs in common
          UtlString rtpAddress;
          int rtpPort;
          const SdpBody* bodyPtr = sipMsg->getSdpBody();
          if(bodyPtr)
          {
              int numMatchingCodecs = 0;
              SdpCodec** matchingCodecs = NULL;
              bodyPtr->getBestAudioCodecs(*codecFactory,
                                          numMatchingCodecs,
                                          matchingCodecs,
						                  rtpAddress, rtpPort);
              if(numMatchingCodecs > 0)
              {
                  // Need to cleanup the codecs
                  for(int codecIndex = 0; codecIndex < numMatchingCodecs; codecIndex++)
                  {
                      delete matchingCodecs[codecIndex];
                      matchingCodecs[codecIndex] = NULL;
                  }
                  delete[] matchingCodecs;
                  atLeastOneCodecSupported = TRUE;
              }
              else
              {
                  atLeastOneCodecSupported = FALSE;

                  // Send back a bad media error
                  // There are no codecs in common
                  SipMessage badMediaResponse;
                  badMediaResponse.setInviteBadCodecs(sipMsg);
                  sipUa.send(badMediaResponse);
              }

          }

          // Assume that SDP will be sent in ACK
          else
              atLeastOneCodecSupported = TRUE;
      }
       
      if(atLeastOneCodecSupported)
      {
          // Create a new connection
          createConnection = TRUE;
      }
      else
      {
          createConnection = FALSE;
          osPrintf("SipConnection::requestShouldCreateConnection FALSE INVITE with no supported RTP codecs\n");
      }
   }

   // NOTIFY for REFER
   // a non-existing transaction.
   else if(method.compareTo(SIP_NOTIFY_METHOD) == 0)
   {
      UtlString eventType;
      sipMsg->getEventField(eventType);
      eventType.toLower();
      int typeIndex = eventType.index(SIP_EVENT_REFER);
      if(typeIndex >=0)
      {
          // Send a bad callId/transaction message
          SipMessage badTransactionMessage;
          badTransactionMessage.setBadTransactionData(sipMsg);
          sipUa.send(badTransactionMessage);
      }
      // All other NOTIFY events are ignored
      createConnection = FALSE;
   }

   else if(method.compareTo(SIP_REFER_METHOD) == 0)
   {
      createConnection = TRUE;
   }

   // All other methods: this is part of
   // a non-existing transaction.
   else
   {
      // Send a bad callId/transaction message
      SipMessage badTransactionMessage;
      badTransactionMessage.setBadTransactionData(sipMsg);
      sipUa.send(badTransactionMessage);
      createConnection = FALSE;
   }

   return createConnection;
}

UtlBoolean SipConnection::shouldCreateConnection(SipUserAgent& sipUa,
                                                OsMsg& eventMessage, 
                                                SdpCodecFactory* codecFactory)
{
    UtlBoolean createConnection = FALSE;
	int msgType = eventMessage.getMsgType();
	int msgSubType = eventMessage.getMsgSubType();
	const SipMessage* sipMsg = NULL;
	int messageType;

	if(msgType == OsMsg::PHONE_APP &&
		msgSubType == CallManager::CP_SIP_MESSAGE)
	{
		sipMsg = ((SipMessageEvent&)eventMessage).getMessage();
		messageType = ((SipMessageEvent&)eventMessage).getMessageStatus();
#ifdef TEST_PRINT
		osPrintf("SipConnection::messageType: %d\n", messageType);
#endif

        switch(messageType)
        {
		    // This is a request which failed to get sent 
            case SipMessageEvent::TRANSPORT_ERROR:
            case SipMessageEvent::SESSION_REINVITE_TIMER:
            case SipMessageEvent::AUTHENTICATION_RETRY:
                // Ignore it and do not create a connection
                createConnection = FALSE;
                break;

            default:
                // Its a SIP Response
                if(sipMsg->isResponse())
                {
                    // Ignore it and do not create a connection
                   createConnection = FALSE;
                }
                // Its a SIP Request
                else
                {
                   createConnection = SipConnection::requestShouldCreateConnection(sipMsg, sipUa, codecFactory);
                }
                break;
        }

        if(!createConnection)
        {
            UtlString msgBytes;
            int numBytes;
            sipMsg->getBytes(&msgBytes, &numBytes);
            msgBytes.insert(0, "SipConnection::shouldCreateConnection: FALSE\n");
            osPrintf("%s\n", msgBytes.data());
        }
        else
        {
            osPrintf("Create a SIP connection\n");
        }
    }

    return(createConnection);
}

void SipConnection::buildLocalContact(Url fromUrl, 
                                      UtlString& localContact) const
{
    UtlString contactHostPort;
    UtlString address;
    sipUserAgent->getContactUri(&contactHostPort);
    Url hostPort(contactHostPort);
    hostPort.getHostAddress(address);
    int port = hostPort.getHostPort();

    UtlString displayName;
    UtlString userId;
    fromUrl.getDisplayName(displayName);
    fromUrl.getUserId(userId);

    Url contactUrl(mLocalContact, TRUE);
    contactUrl.setUserId(userId.data());
    contactUrl.setDisplayName(displayName);
    contactUrl.setHostAddress(address);
    contactUrl.setHostPort(port);
    contactUrl.includeAngleBrackets();
    contactUrl.toString(localContact);
}

void SipConnection::buildLocalContact(UtlString& localContact) const
{
    UtlString contactHostPort;
    UtlString address;
    
    sipUserAgent->getContactUri(&contactHostPort);
    Url hostPort(contactHostPort);
    hostPort.getHostAddress(address);
    int port = hostPort.getHostPort();

    Url contactUrl(mLocalContact, TRUE);
    contactUrl.setHostAddress(address);
    contactUrl.setHostPort(port);
    contactUrl.includeAngleBrackets();
    contactUrl.toString(localContact);
}

UtlBoolean SipConnection::dial(const char* dialString,
							  const char* localLineAddress,
                              //const char* callerDisplayName,
							  const char* callId,
                              const char* callController,
                              const char* originalCallConnection,
                              UtlBoolean requestQueuedCall)
{
	UtlBoolean dialOk = FALSE;
	SipMessage sipInvite;
    const char* callerDisplayName = NULL;
	int receiveRtpPort;
	UtlString rtpAddress;
	UtlString dummyFrom;
    UtlString fromAddress;
    UtlString goodToAddress;
    int cause = CONNECTION_CAUSE_NORMAL;

	if(getState() == CONNECTION_IDLE)
	{
        // Create a new connection in the media flowgraph
        mpMediaInterface->createConnection(mConnectionId);
        SdpCodecFactory supportedCodecs;
        mpMediaInterface->getCapabilities(mConnectionId, 
                                    rtpAddress, 
                                    receiveRtpPort,
                                    supportedCodecs);

		mRemoteIsCallee = TRUE;
		setCallId(callId);

		lastLocalSequenceNumber++;

        buildFromToAddresses(dialString, "xxxx", callerDisplayName,
            dummyFrom, goodToAddress);

        // The local address is always set
        mFromUrl.toString(fromAddress);

		osPrintf("Using To address: \"%s\"\n", goodToAddress.data());

        { //memory scope
   		    // Get the codecs
            int numCodecs;
            SdpCodec** rtpCodecsArray = NULL;
            supportedCodecs.getCodecs(numCodecs, rtpCodecsArray);

#ifdef TEST_PRINT
            UtlString codecsString;
            supportedCodecs.toString(codecsString);
            osPrintf("SipConnection::dial codecs:\n%s\n",
                codecsString.data());
#endif

            // Prepare to receive the codecs
            mpMediaInterface->startRtpReceive(mConnectionId,
                                              numCodecs, 
                                              rtpCodecsArray);

            // Create a contact using the host & port from the
            // SipUserAgent and the display name and userId from
            // the from URL.
            UtlString localContact;
            buildLocalContact(mFromUrl, localContact);

            // Create and send an INVITE
            sipInvite.setInviteData(fromAddress.data(),
 					    goodToAddress.data(),NULL, 
                         localContact.data(),
                         callId,
   					    rtpAddress.data(), receiveRtpPort, 
   					    lastLocalSequenceNumber, numCodecs, rtpCodecsArray,
                           mDefaultSessionReinviteTimer);

            // Free up the codecs and the array
            for(int codecIndex = 0; codecIndex < numCodecs; codecIndex++)
            {
                delete rtpCodecsArray[codecIndex];
                rtpCodecsArray[codecIndex] = NULL;
            }
            delete[] rtpCodecsArray;
            rtpCodecsArray = NULL;
        }
        // Set caller preference if caller wants queueing or campon
        if(requestQueuedCall)
        {
            sipInvite.addRequestDisposition(SIP_DISPOSITION_QUEUE);
        }

        // Set the requested by field (BYE Also style transfer)
        if(callController && originalCallConnection == NULL)
        {
            UtlString requestedByField(callController);
            const char* alsoTags = strchr(dialString, '>');
            int uriIndex = requestedByField.index('<');
            if(uriIndex < 0)
            {
                requestedByField.insert(0, '<');
                requestedByField.append('>');
            }
            if(alsoTags)
            {
                alsoTags++;
                requestedByField.append(alsoTags);
            }
            sipInvite.setRequestedByField(requestedByField.data());
            cause = CONNECTION_CAUSE_TRANSFER;
        }

        // Set the header fields for REFER style transfer INVITE
        /*else*/ if(callController && originalCallConnection)
        {
            mOriginalCallConnectionAddress = originalCallConnection;
            sipInvite.setReferredByField(callController);
            cause = CONNECTION_CAUSE_TRANSFER;
        }

        // Save a copy of the invite
		inviteMsg = new SipMessage(sipInvite);
        inviteFromThisSide = TRUE;
        setCallerId();

		setState(Connection::CONNECTION_ESTABLISHED, Connection::CONNECTION_LOCAL);

		if(!goodToAddress.isNull() && sipUserAgent->send(sipInvite))
		{
			setState(CONNECTION_INITIATED, CONNECTION_REMOTE, cause);
			osPrintf("INVITE sent successfully\n");
			setState(CONNECTION_OFFERING, CONNECTION_REMOTE, cause);
			dialOk = TRUE;
		}
		else
		{
			osPrintf("INVITE send failed\n");
			setState(CONNECTION_FAILED, CONNECTION_REMOTE, CONNECTION_CAUSE_DEST_NOT_OBTAINABLE);

            // Failed to send a message for transfer
            if(callController && !goodToAddress.isNull())
            {
                // Send back a status to the original call
                UtlString originalCallId;
                mpCall->getOriginalCallId(originalCallId);
                CpMultiStringMessage transfereeStatus(CallManager::CP_TRANSFEREE_CONNECTION_STATUS,
                    originalCallId.data(), 
                    mOriginalCallConnectionAddress.data(),
                    NULL, NULL, NULL,
                    CONNECTION_FAILED, SIP_REQUEST_TIMEOUT_CODE);
#ifdef TEST_PRINT
                osPrintf("SipConnection::dial posting CP_TRANSFEREE_CONNECTION_STATUS to call: %s\n",
                    originalCallId.data());
#endif
                mpCallManager->postMessage(transfereeStatus);
            }

		}


	}

	return(dialOk);
}

UtlBoolean SipConnection::answer()
{
#ifdef TEST_PRINT
    OsSysLog::add(FAC_SIP, PRI_WARNING,
                    "Entering SipConnection::answer inviteMsg=0x%08x ", (int)inviteMsg);
#endif 

	UtlBoolean answerOk = FALSE;
	UtlString rtpAddress;
    const SdpBody* sdpBody = NULL;
	int receiveRtpPort;
    SdpCodecFactory supportedCodecs;
    mpMediaInterface->getCapabilities(mConnectionId, 
                                    rtpAddress, 
                                    receiveRtpPort,
                                    supportedCodecs);

    int currentState = getState();
	if(inviteMsg && !inviteFromThisSide &&
        (currentState == CONNECTION_ALERTING ||
         currentState == CONNECTION_OFFERING ||
         currentState == CONNECTION_INITIATED ||
         currentState == CONNECTION_IDLE))
	{
        int numMatchingCodecs = 0;
        SdpCodec** matchingCodecs = NULL;
        getInitialSdpCodecs(inviteMsg, supportedCodecs,
                            numMatchingCodecs, matchingCodecs,
                            remoteRtpAddress, remoteRtpPort);

        sdpBody = inviteMsg->getSdpBody();
		if(numMatchingCodecs <= 0 && sdpBody)
		{
			osPrintf("No matching codecs rejecting call\n");

			// No common codecs send INVITE error response
			SipMessage sipResponse;
			sipResponse.setInviteBadCodecs(inviteMsg);
			sipUserAgent->send(sipResponse);

			setState(CONNECTION_FAILED, CONNECTION_LOCAL, CONNECTION_CAUSE_RESOURCES_NOT_AVAILABLE);
		}

		// Compatable codecs send OK response
		else
		{
            // Respond with an OK 
			osPrintf("Sending INVITE OK\n");

            // There was no SDP in the INVITE, so give them all of
            // the codecs we support.
            if(!sdpBody)
            {
                osPrintf("Sending initial SDP in OK\n");

                // There were no codecs specified in the INVITE
                // Give the full set of supported codecs
                supportedCodecs.getCodecs(numMatchingCodecs, matchingCodecs);
            }

            // If there was SDP in the INVITE and it indicated hold:
            if(sdpBody && remoteRtpPort <= 0)
            {
                rtpAddress = "0.0.0.0";  // hold address
            }

			SipMessage sipResponse;
			// Send a INVITE OK response
			sipResponse.setInviteOkData(inviteMsg, rtpAddress.data(), 
				receiveRtpPort, numMatchingCodecs, matchingCodecs,
                mDefaultSessionReinviteTimer);

			if(!sipUserAgent->send(sipResponse))
			{
				//sprintf(displayMsg, "INVITE OK failed: %s", remoteHostName.data());
				//phoneSet->setStatusDisplay(displayMsg);
				setState(CONNECTION_FAILED, CONNECTION_LOCAL, CONNECTION_CAUSE_NORMAL);
			}
			else
			{
				setState(CONNECTION_ESTABLISHED, CONNECTION_LOCAL, CONNECTION_CAUSE_NORMAL);
				answerOk = TRUE;

			    // Setup media channel
                osPrintf("Setting up flowgraph receive\n");

                // if we have a send codec chosen Start sending media
                if(numMatchingCodecs > 0)
                {
                    mpMediaInterface->setConnectionDestination(mConnectionId,
                                          remoteRtpAddress.data(), 
                                          remoteRtpPort);
                    // Set up the remote RTP sockets
			 
			        osPrintf("RTP SENDING address: %s port: %d\n", remoteRtpAddress.data(), remoteRtpPort);

			        if(remoteRtpPort > 0)
                    {
				       //SdpCodec sndCodec((SdpCodec::SdpCodecTypes)
                       //    sendCodec);
				       mpMediaInterface->startRtpSend(mConnectionId,
                          numMatchingCodecs, matchingCodecs);
                    }
               }

               // Start receiving media
               SdpCodec recvCodec((SdpCodec::SdpCodecTypes) receiveCodec);
			   mpMediaInterface->startRtpReceive(mConnectionId,
                  numMatchingCodecs, matchingCodecs);
               osPrintf("RECEIVING RTP\n");

               inviteMsg->getAllowField(mAllowedRemote);
			}
		}

        // Free up the codec copies and array
        for(int codecIndex = 0; codecIndex < numMatchingCodecs; codecIndex++)
        {
            delete matchingCodecs[codecIndex];
            matchingCodecs[codecIndex] = NULL;
        }
        delete[] matchingCodecs;
        matchingCodecs = NULL;
	}

#ifdef TEST_PRINT
        OsSysLog::add(FAC_SIP, PRI_WARNING,
                    "Leaving SipConnection::answer inviteMsg=0x%08x ", (int)inviteMsg);
#endif

	return(answerOk);
}

UtlBoolean SipConnection::accept(int ringingTimeOutSeconds)
{
    UtlBoolean ringingSent = FALSE;
    int cause = 0;
    osPrintf("SipConnection::accept ringingTimeOutSeconds=%d\n", ringingTimeOutSeconds);
    if(inviteMsg && !inviteFromThisSide &&
        getState(cause) == CONNECTION_OFFERING)
    {
        UtlString rtpAddress;
        int receiveRtpPort;
        //UtlBoolean receiveCodecSet;
        //UtlBoolean sendCodecSet;
        int numMatchingCodecs = 0;
        SdpCodec** matchingCodecs = NULL;
        SdpCodecFactory supportedCodecs;
        UtlString replaceCallId;
        UtlString replaceToTag;
        UtlString replaceFromTag;

        // Make sure that this isn't part of a transfer.  If we find a
        // REPLACES header, then we shouldn't accept the call, but rather
        // we should return a 481 response.
        if (inviteMsg->getReplacesData(replaceCallId, replaceToTag, replaceFromTag))
        {
           SipMessage badTransaction;
           badTransaction.setBadTransactionData(inviteMsg);
           sipUserAgent->send(badTransaction);
           osPrintf("SipConnection::accept - CONNECTION_FAILED, cause BUSY : 754\n");
           setState(CONNECTION_FAILED, CONNECTION_REMOTE, CONNECTION_CAUSE_BUSY);
        }
        else
        {       
           mpMediaInterface->getCapabilities(mConnectionId, 
                                             rtpAddress, 
                                             receiveRtpPort,
                                             supportedCodecs);

           // Get the codecs if SDP is provided
           getInitialSdpCodecs(inviteMsg,
                               supportedCodecs,
                               numMatchingCodecs, matchingCodecs, 
                               remoteRtpAddress, remoteRtpPort);

           // Try to setup for early receipt of media.
           if(numMatchingCodecs > 0)
           {
               SdpCodec recvCodec((SdpCodec::SdpCodecTypes) receiveCodec);
               mpMediaInterface->startRtpReceive(mConnectionId, 
                   numMatchingCodecs, matchingCodecs);
           }
           ringingSent = TRUE;
           proceedToRinging(inviteMsg, sipUserAgent, -1, //dummy tagNum, callInFocus,
               mLineAvailableBehavior, mpMediaInterface);

           // Keep track of the fact that this is a transfer
           if(cause != CONNECTION_CAUSE_TRANSFER) cause = CONNECTION_CAUSE_NORMAL;
           setState(CONNECTION_ALERTING, CONNECTION_LOCAL, cause);

           // If forward on no answer is enabled set the timer
           if(ringingTimeOutSeconds > 0 )
           {
               // Set a timer to post a message to this call
               // to timeout the ringing and forward
               setRingingTimer(ringingTimeOutSeconds);
           }

           // Free up the codec copies and array
           for(int codecIndex = 0; codecIndex < numMatchingCodecs; codecIndex++)
           {
               delete matchingCodecs[codecIndex];
               matchingCodecs[codecIndex] = NULL;
           }
           delete[] matchingCodecs;
           matchingCodecs = NULL;
       }
    }

    return(ringingSent);
}

UtlBoolean SipConnection::reject()
{
    UtlBoolean responseSent = FALSE;
    if(inviteMsg && !inviteFromThisSide)
    {
       int state = getState();
        if (state == CONNECTION_OFFERING)
       {
           UtlString replaceCallId;
           UtlString replaceToTag;
           UtlString replaceFromTag;

           // Make sure that this isn't part of a transfer.  If we find a
           // REPLACES header, then we shouldn't accept the call, but rather
           // we should return a 481 response.
           if (inviteMsg->getReplacesData(replaceCallId, replaceToTag, replaceFromTag))
           {
              SipMessage badTransaction;
              badTransaction.setBadTransactionData(inviteMsg);
              responseSent = sipUserAgent->send(badTransaction);
              osPrintf("SipConnection::reject - CONNECTION_FAILED, cause BUSY : 825\n");
              setState(CONNECTION_FAILED, CONNECTION_REMOTE, CONNECTION_CAUSE_BUSY);           
           }
           else
           {
              SipMessage busyMessage;
              busyMessage.setInviteBusyData(inviteMsg);
              responseSent = sipUserAgent->send(busyMessage);
              osPrintf("SipConnection::reject - CONNECTION_FAILED, cause BUSY : 833\n");
              setState(CONNECTION_FAILED, CONNECTION_REMOTE, CONNECTION_CAUSE_BUSY);
           }
       }
       else if (state == CONNECTION_ALERTING)
       {
           SipMessage terminateMessage;
           terminateMessage.setRequestTerminatedResponseData(inviteMsg);
           responseSent = sipUserAgent->send(terminateMessage);
           osPrintf("SipConnection::reject - CONNECTION_DISCONNECTED, cause CONNECTION_CAUSE_CANCELLED : 845\n");
           setState(CONNECTION_DISCONNECTED, CONNECTION_REMOTE, CONNECTION_CAUSE_CANCELLED);
       }
    }
    return(responseSent);
}

UtlBoolean SipConnection::redirect(const char* forwardAddress)
{
     UtlBoolean redirectSent = FALSE;
    if(inviteMsg && !inviteFromThisSide &&
        (getState() == CONNECTION_OFFERING ||
        getState() == CONNECTION_ALERTING))
    {
		UtlString targetUrl;
		UtlString dummyFrom;
		const char* callerDisplayName = NULL;
		const char* targetCallId = NULL;
		buildFromToAddresses(forwardAddress, targetCallId, callerDisplayName,
			dummyFrom, targetUrl);
        // Send a redirect message
        SipMessage redirectResponse;
        redirectResponse.setForwardResponseData(inviteMsg, 
						targetUrl.data());
        redirectSent = sipUserAgent->send(redirectResponse);
        setState(CONNECTION_DISCONNECTED, CONNECTION_REMOTE, CONNECTION_CAUSE_REDIRECTED);
        setState(CONNECTION_DISCONNECTED, CONNECTION_LOCAL, CONNECTION_CAUSE_REDIRECTED);
		targetUrl = OsUtil::NULL_OS_STRING;
		dummyFrom = OsUtil::NULL_OS_STRING;
    }
    return(redirectSent);
}

UtlBoolean SipConnection::hangUp()
{
    return(doHangUp());
}

UtlBoolean SipConnection::hold()
{
    UtlBoolean messageSent = FALSE;
    // If the call is connected and
    // we are not in the middle of a SIP transaction
  	//SDUA - fix resend problem with no sdp  - correction already in main
 	if(inviteMsg && getState() == CONNECTION_ESTABLISHED &&
 		reinviteState == ACCEPT_INVITE &&
 		mTerminalConnState!=PtTerminalConnection::HELD)
       {
        UtlString rtpAddress;
        int receiveRtpPort;
        //int numCodecs = mpMediaInterface->getNumCodecs(mConnectionId);
        //SdpCodec* rtpCodecs = new SdpCodec[numCodecs];
        SdpCodecFactory supportedCodecs;
        mpMediaInterface->getCapabilities(mConnectionId, 
                                          rtpAddress, 
                                          receiveRtpPort,
                                          supportedCodecs);
        int numCodecs = 0;
        SdpCodec** codecsArray = NULL;
        supportedCodecs.getCodecs(numCodecs, codecsArray);

         // Build an INVITE with the RTP address in the SDP of 0.0.0.0
#ifdef TEST_PRINT
           osPrintf("SipConnection::hold\n");
#endif
         SipMessage holdMessage;
         UtlString localContact;
         buildLocalContact(localContact);
         holdMessage.setReinviteData(inviteMsg,
                                    mRemoteContact,
                                    localContact,
                                    inviteFromThisSide,
                                    mRouteField,
                                    "0.0.0.0", receiveRtpPort, 
                                    ++lastLocalSequenceNumber,
                                    numCodecs,
                                    codecsArray,
                                    mDefaultSessionReinviteTimer);
   
        if(inviteMsg) delete inviteMsg;
        inviteMsg = new SipMessage(holdMessage);
        inviteFromThisSide = TRUE;

        if(sipUserAgent->send(holdMessage))
        {
            messageSent = TRUE;
            // Disallow INVITEs while this transaction is taking place
            reinviteState = REINVITING;
 			mFarEndHoldState = TERMCONNECTION_HOLDING;
//            mpMediaInterface->stopRtpSend(mConnectionId); // do not stop the media send until the response is OK
        }

        // Free up the codec copies and array
        for(int codecIndex = 0; codecIndex < numCodecs; codecIndex++)
        {
            delete codecsArray[codecIndex];
            codecsArray[codecIndex] = NULL;
        }
        delete[] codecsArray;
        codecsArray = NULL;
    }
    return(messageSent);
}

UtlBoolean SipConnection::offHold()
{
    return(doOffHold(FALSE));
}

UtlBoolean SipConnection::renegotiateCodecs()
{
    return(doOffHold(TRUE));
}

UtlBoolean SipConnection::doOffHold(UtlBoolean forceReInvite)
{
    UtlBoolean messageSent = FALSE;
    // If the call is connected and
    // we are not in the middle of a SIP transaction

	//SDUA - fix resend problem with no sdp  - correction already in main
 	if(inviteMsg && getState() == CONNECTION_ESTABLISHED &&
       reinviteState == ACCEPT_INVITE && 
       (mTerminalConnState == PtTerminalConnection::HELD ||
         (forceReInvite && 
          mTerminalConnState == PtTerminalConnection::TALKING)))
 
    {
        UtlString rtpAddress;
        int receiveRtpPort;
        SdpCodecFactory supportedCodecs;
        mpMediaInterface->getCapabilities(mConnectionId, 
                                          rtpAddress, 
                                          receiveRtpPort,
                                          supportedCodecs);

        int numCodecs = 0;
        SdpCodec** rtpCodecs = NULL;
        supportedCodecs.getCodecs(numCodecs, rtpCodecs);

           // Build an INVITE with the RTP address in the SDP
           // as the real address
#ifdef TEST_PRINT
           osPrintf("SipConnection::offHold rtpAddress: %s\n",
               rtpAddress.data());
#endif
         SipMessage offHoldMessage;
         UtlString localContact;
         buildLocalContact(localContact);
         offHoldMessage.setReinviteData(inviteMsg,
                                        mRemoteContact,
                                        localContact,
                                        inviteFromThisSide,
                                        mRouteField,
                                        rtpAddress.data(), receiveRtpPort, 
                                        ++lastLocalSequenceNumber,
                                        numCodecs,
                                        rtpCodecs,
                                        mDefaultSessionReinviteTimer);
   
        // Free up the codec copies and array
        for(int codecIndex = 0; codecIndex < numCodecs; codecIndex++)
        {
            delete rtpCodecs[codecIndex];
            rtpCodecs[codecIndex] = NULL;
        }
        delete[] rtpCodecs;
        rtpCodecs = NULL;

        if(inviteMsg) delete inviteMsg;
        inviteMsg = new SipMessage(offHoldMessage);
        inviteFromThisSide = TRUE;

        if(sipUserAgent->send(offHoldMessage))
        {
            messageSent = TRUE;
            // Disallow INVITEs while this transaction is taking place
            reinviteState = REINVITING;

            // If we are doing a forced reINVITE
            // there are no state changes

            // Otherwise signal the offhold state changes
            if(!forceReInvite)
            {
                mFarEndHoldState = TERMCONNECTION_TALKING;
                if (mpCall->getCallType() != CpCall::CP_NORMAL_CALL)
                {
                    mpCall->setCallType(CpCall::CP_NORMAL_CALL);
                }
                setState(CONNECTION_ESTABLISHED, CONNECTION_REMOTE, CONNECTION_CAUSE_UNHOLD);
            }
        }
    }
    return(messageSent);
}

UtlBoolean SipConnection::originalCallTransfer(UtlString& dialString,
						           const char* transferControllerAddress,
                                   const char* targetCallId)
{
	UtlBoolean ret = FALSE;

    mIsReferSent = FALSE;
#ifdef TEST_PRINT
    UtlString remoteAddr;
    getRemoteAddress(&remoteAddr);
    UtlString conState;
    getStateString(getState(), &conState);
    osPrintf("SipConnection::originalCallTransfer on %s %x %x:\"%s\" state: %d\n",
        remoteAddr.data(), inviteMsg, dialString.data(), 
        dialString.length() ? dialString.data() : "", conState.data());
#endif
    if(inviteMsg && dialString && *dialString &&
        getState() == CONNECTION_ESTABLISHED)
    {
        // If the transferee (the party at the other end of this
        // connection) supports the REFER method
		const char* callerDisplayName = NULL;

		// If blind transfer
		{
		    UtlString targetUrl;
			UtlString dummyFrom;
			buildFromToAddresses(dialString, targetCallId, callerDisplayName,
				dummyFrom, targetUrl);
            dialString = targetUrl;
			//alsoUri.append(";token1");
		}

        if(isMethodAllowed(SIP_REFER_METHOD))
        {
            mTargetCallConnectionAddress = dialString;
            mTargetCallId = targetCallId;

            // If the connection is not already on hold, do a hold
            // first and then do the REFER transfer
            if(mFarEndHoldState == TERMCONNECTION_TALKING ||
                mFarEndHoldState == TERMCONNECTION_NONE)
            {
                mHoldCompleteAction = CpCallManager::CP_BLIND_TRANSFER;
                //need to do a remote hold first
                // Then after that is complete do the REFER
                hold();
                ret = TRUE;
            }

            else
            {
                // Send a REFER to tell the transferee to
                // complete a blind transfer
                doBlindRefer();

                ret = mIsReferSent;
            }
        }

        // Use the old BYE Also method of transfer
        else
        {
            doHangUp(dialString.data(),
						  transferControllerAddress);
            mTargetCallConnectionAddress = dialString;
			ret = TRUE;
        }
    }
    return(ret);
}

void SipConnection::doBlindRefer()
{
                    // Send a REFER message
                SipMessage referRequest;
                lastLocalSequenceNumber++;
                UtlString localContact;
                buildLocalContact(localContact);

                referRequest.setReferData(inviteMsg, 
                inviteFromThisSide,
                lastLocalSequenceNumber, 
                mRouteField.data(),
                localContact.data(),
                mRemoteContact.data(),
                mTargetCallConnectionAddress.data(), 
                // The following keeps the target call id on the
                // transfer target the same as the consultative call
                //mTargetCallId);
                // The following does not set the call Id on the xfer target
                "");

                mIsReferSent = sipUserAgent->send(referRequest);
}

UtlBoolean SipConnection::targetCallBlindTransfer(const char* dialString,
						           const char* transferControllerAddress)
{
    // This should never get here
    unimplemented("SipConnection::targetCallBlindTransfer");
    return(FALSE);
}

UtlBoolean SipConnection::transferControllerStatus(int connectionState, int response)
{
    // It should never get here
    unimplemented("SipConnection::transferControllerStatus");
    return(FALSE);
}

UtlBoolean SipConnection::transfereeStatus(int callState, int returnCode)
{
    UtlBoolean referResponseSent = FALSE;

#ifdef TEST_PRINT
    osPrintf("SipConnection::transfereeStatus callType: %d referMessage: %x\n",
        mpCall->getCallType(), mReferMessage);
#endif
    // If this call and connection received a REFER request
    if(mpCall->getCallType() == 
          CpCall::CP_TRANSFEREE_ORIGINAL_CALL &&
        mReferMessage)
    {
        UtlString transferMethod;
        mReferMessage->getRequestMethod(&transferMethod);

        // REFER type transfer
        if(transferMethod.compareTo(SIP_REFER_METHOD) == 0)
        {
			int num;
 			UtlString method;
 			mReferMessage->getCSeqField(&num , &method);
 
 			UtlString event;
 			event.append(SIP_EVENT_REFER);
 			event.append(";cseq=");
 			char buff[50];
 			sprintf(buff,"%d", num);
 			event.append(buff);

            // Generate an appropriate NOTIFY message to indicate the
            // outcome
            SipMessage referNotify;

            HttpBody* body = NULL;
            lastLocalSequenceNumber++;
            referNotify.setNotifyData(mReferMessage, 
                                      lastLocalSequenceNumber, 
                                      mRouteField,
                                      event);
            if(callState == CONNECTION_ESTABLISHED)
            {
                //transferResponse.setReferOkData(mReferMessage);
                body = new HttpBody(SIP_REFER_SUCCESS_STATUS, -1, CONTENT_TYPE_MESSAGE_SIPFRAG);
            }
            else if(callState == CONNECTION_ALERTING)
            {
                SipMessage alertingMessage;
                switch(returnCode)
                {
                case SIP_EARLY_MEDIA_CODE:
                    alertingMessage.setResponseFirstHeaderLine(SIP_PROTOCOL_VERSION, 
                        returnCode, SIP_RINGING_TEXT);
                    break;

                default:
                    alertingMessage.setResponseFirstHeaderLine(SIP_PROTOCOL_VERSION, 
                        returnCode, SIP_RINGING_TEXT);
                    break;
                }
                UtlString messageBody;
                int len;
                alertingMessage.getBytes(&messageBody,&len);

                body = new HttpBody(messageBody.data(), -1, CONTENT_TYPE_MESSAGE_SIPFRAG);
            }
            else
            {
                //transferResponse.setReferFailedData(mReferMessage);
                body = new HttpBody(SIP_REFER_FAILURE_STATUS, -1, CONTENT_TYPE_MESSAGE_SIPFRAG);
            }
            referNotify.setBody(body);

            // Add the content type for the body
		    referNotify.setContentType(CONTENT_TYPE_MESSAGE_SIPFRAG);

		    // Add the content length
            int len;
            UtlString bodyString;
		    body->getBytes(&bodyString, &len);
		    referNotify.setContentLength(len);

            referResponseSent = sipUserAgent->send(referNotify);

            // Only delete if this is a final notify
            if(callState != CONNECTION_ALERTING && mReferMessage)
            {
                delete mReferMessage;
                mReferMessage = NULL;
            }
        }

        // Should be BYE Also type transfer
        else
        {
            SipMessage transferResponse;
            if(callState == CONNECTION_ESTABLISHED)
            {
                transferResponse.setOkResponseData(mReferMessage);
            }
            else
            {
                transferResponse.setReferFailedData(mReferMessage);
            }
            referResponseSent = sipUserAgent->send(transferResponse);
            if(mReferMessage) delete mReferMessage;
            mReferMessage = NULL;
        }
        
    }

    else
    {
        osPrintf("SipConnection::transfereeStatus FAILED callType: %d mReferMessage: %p\n",
            mpCall->getCallType() , mReferMessage);
    }
    return(referResponseSent);
}

UtlBoolean SipConnection::doHangUp(const char* dialString,
						          const char* callerId)
{
	int cause;
	int currentState = getState(0, cause);	// always get remote connection state
	UtlBoolean hangUpOk = FALSE;
    const char* callerDisplayName = NULL;
	SipMessage sipRequest;
    UtlString alsoUri;

    // If blind transfer
    if(dialString && *dialString)
    {
        UtlString dummyFrom;
        buildFromToAddresses(dialString, callerId, callerDisplayName,
            dummyFrom, alsoUri);
    }

	// Tell the other end that we are hanging up
	// Need to send SIP CANCEL if we are the caller
	// and the callee connection state is not finalized
	if(mRemoteIsCallee && 
		currentState != CONNECTION_FAILED &&
		currentState != CONNECTION_ESTABLISHED &&
		currentState != CONNECTION_DISCONNECTED &&
        currentState != CONNECTION_UNKNOWN)
	{
		// We are the caller, cancel the incomplete call
		// Send a CANCEL
		//sipRequest = new SipMessage();

        // We are calling and the call is not setup yet so
        // cancel.  If we get a subsequent OK, we need to send
        // a BYE.
        if(inviteFromThisSide)
        {
            sipRequest.setCancelData(inviteMsg);
            mLastRequestMethod = SIP_CANCEL_METHOD;

            // If this was a canceled transfer INVITE, send back a status
            if(!mOriginalCallConnectionAddress.isNull())
            {
                UtlString originalCallId;
                mpCall->getOriginalCallId(originalCallId);
                CpMultiStringMessage transfereeStatus(CallManager::CP_TRANSFEREE_CONNECTION_STATUS,
                    originalCallId.data(), 
                    mOriginalCallConnectionAddress.data(),
                    NULL, NULL, NULL,
                    CONNECTION_FAILED, SIP_REQUEST_TIMEOUT_CODE);
#ifdef TEST_PRINT
                osPrintf("SipConnection::processResponse posting CP_TRANSFEREE_CONNECTION_STATUS to call: %s\n",
                    originalCallId.data());
#endif
                mpCallManager->postMessage(transfereeStatus);

            }
        }

        // Someone is calling us and we are hanging up before the
        // call is setup.  This is not likely to occur and I am
        // not sure what the right thing to do here CANCEL or BYE.
        else
        {
            lastLocalSequenceNumber++;
            osPrintf("doHangup BYE route: %s\n", mRouteField.data());
 			sipRequest.setByeData(inviteMsg, 
 				mRemoteContact,
 				inviteFromThisSide,
                   lastLocalSequenceNumber,
                   //directoryServerUri.data(), 
                   mRouteField.data(),
                   alsoUri.data());
 
               mLastRequestMethod = SIP_BYE_METHOD;
           }
 		
		if(sipUserAgent->send(sipRequest))
		{
			if(inviteFromThisSide) 
                osPrintf("unsetup call CANCEL message sent\n");
            else 
                osPrintf("unsetup call BYE message sent\n");

			// Lets try not setting this to disconected until
			// we get the response or a timeout
			//setState(CONNECTION_DISCONNECTED, CONNECTION_REMOTE, CONNECTION_CAUSE_CANCELLED);

			// However we need something to indicate that the call 
			// is being cancelled to handle a race condition when the callee responds with 200 OK
			// before it receives the Cancel.
			mbCancelling = TRUE;

			hangUpOk = TRUE;
		}
	}

	// We are the Caller or callee
	else if(currentState == CONNECTION_ESTABLISHED)
	{
		// the call is connected
		// Send a BYE
		//sipRequest = new SipMessage();
        //UtlString directoryServerUri;
        //if(!inviteFromThisSide)
        //{
            //UtlString dirAddress;
            //UtlString dirProtocol;
            //int dirPort;

            //sipUserAgent->getDirectoryServer(0, &dirAddress,
			//	&dirPort, &dirProtocol);
            //SipMessage::buildSipUrl(&directoryServerUri, 
            //    dirAddress.data(), dirPort, dirProtocol.data());
        //}
        lastLocalSequenceNumber++;
        osPrintf("setup call BYE route: %s remote contact: %s\n", 
            mRouteField.data(), mRemoteContact.data());
        sipRequest.setByeData(inviteMsg,
                              mRemoteContact,
                              inviteFromThisSide,
                              lastLocalSequenceNumber,
                              //directoryServerUri.data(),
                              mRouteField.data(),
                              alsoUri.data());
        mLastRequestMethod = SIP_BYE_METHOD;
        sipUserAgent->send(sipRequest);
   
 		  // Lets try not setting this to disconected until we get the response or a timeout


      // BOB 6/21/02: Calling setState on the local connection fires a TAO
      // event indicating that the location connection has just dropped.  This
      // is very bad if in a conference.
   	// setState(CONNECTION_DISCONNECTED, CONNECTION_LOCAL);
      hangUpOk = TRUE;
	}

    mpMediaInterface->stopRtpSend(mConnectionId);

	return(hangUpOk);
}

void SipConnection::buildFromToAddresses(const char* dialString,
                                         const char* callerId,
                                         const char* callerDisplayName,
                                         UtlString& fromAddress, 
                                         UtlString& goodToAddress) const
{
    UtlString sipAddress;
	int sipPort;
	UtlString sipProtocol;

    fromAddress.remove(0);
    goodToAddress.remove(0);

		// Build a From address
		sipUserAgent->getFromAddress(&sipAddress, &sipPort, &sipProtocol);
		SipMessage::buildSipUrl(&fromAddress, sipAddress.data(), 
			sipPort, sipProtocol.data(), callerId, callerDisplayName, 
            mFromTag.data());

		// Check the to Address
		UtlString toAddress;
		UtlString toProtocol;
		UtlString toUser;
		UtlString toUserLabel;
		
		int toPort;

#ifdef TEST_PRINT
		osPrintf("SipConnection::dial got dial string: \"%s\"\n",
			dialString);
#endif

        // Use the Url object to perserve parameters and display name
        Url toUrl(dialString);
        toUrl.getHostAddress(toAddress);

		//SipMessage::parseAddressFromUri(dialString, &toAddress, &toPort,
		//	&toProtocol, &toUser, &toUserLabel);
		if(toAddress.isNull())
		{
			sipUserAgent->getDirectoryServer(0, &toAddress,
				&toPort, &toProtocol);
			osPrintf("Got directory server: \"%s\"\n",
				toAddress.data());
            toUrl.setHostAddress(toAddress.data());
            toUrl.setHostPort(toPort);
            if(!toProtocol.isNull())
            {
                toUrl.setUrlParameter("transport", toProtocol.data());
            }
        }
		//SipMessage::buildSipUrl(&goodToAddress, toAddress.data(),
		//		toPort, toProtocol.data(), toUser.data(),
		//		toUserLabel.data());
        toUrl.toString(goodToAddress);
        
}
       
UtlBoolean SipConnection::processMessage(OsMsg& eventMessage,
                                        UtlBoolean callInFocus,
                                        UtlBoolean onHook)
{
	int msgType = eventMessage.getMsgType();
	int msgSubType = eventMessage.getMsgSubType();
	UtlBoolean processedOk = TRUE;
	const SipMessage* sipMsg = NULL;
	int messageType;

	if(msgType == OsMsg::PHONE_APP &&
		msgSubType == CallManager::CP_SIP_MESSAGE)
	{
		sipMsg = ((SipMessageEvent&)eventMessage).getMessage();
		messageType = ((SipMessageEvent&)eventMessage).getMessageStatus();
#ifdef TEST_PRINT
		osPrintf("SipConnection::messageType: %d\n", messageType);
#endif
        UtlBoolean messageIsResponse = sipMsg->isResponse();
        UtlString method;
        if(!messageIsResponse) sipMsg->getRequestMethod(&method);

		// This is a request which failed to get sent 
		if(messageType == SipMessageEvent::TRANSPORT_ERROR)
		{
			osPrintf("Processing message transport error method: %s\n",
                messageIsResponse ? method.data() : "response");
            if(!inviteMsg)
            {
                osPrintf("SipConnection::processMessage failed response\n");
                // THis call was not setup (i.e. did not try to sent an
                // invite and we did not receive one.  This is a bad call
                setState(CONNECTION_FAILED, CONNECTION_REMOTE, CONNECTION_CAUSE_DEST_NOT_OBTAINABLE);
            }
			// We only care about INVITE.
			// BYE, CANCLE and ACK are someone else's problem.  
			// REGISTER and OPTIONS are handled else where
			else if(sipMsg->isSameMessage(inviteMsg) && 
				getState() == CONNECTION_OFFERING)
			{
				osPrintf("No response to INVITE\n");
				setState(CONNECTION_FAILED, CONNECTION_REMOTE, CONNECTION_CAUSE_DEST_NOT_OBTAINABLE);

#ifdef TEST_PRINT
                osPrintf("SipConnection::processMessage originalConnectionAddress: %s connection state: CONNECTION_FAILED transport failed\n",
                    mOriginalCallConnectionAddress.data());
#endif

                // If this was a failed transfer INVITE, send back a status
                if(!mOriginalCallConnectionAddress.isNull())
                {
                    UtlString originalCallId;
                    mpCall->getOriginalCallId(originalCallId);
                    CpMultiStringMessage transfereeStatus(CallManager::CP_TRANSFEREE_CONNECTION_STATUS,
                        originalCallId.data(), 
                        mOriginalCallConnectionAddress.data(),
                        NULL, NULL, NULL,
                        CONNECTION_FAILED, SIP_REQUEST_TIMEOUT_CODE);
#ifdef TEST_PRINT
                    osPrintf("SipConnection::processResponse posting CP_TRANSFEREE_CONNECTION_STATUS to call: %s\n",
                        originalCallId.data());
#endif
                    mpCallManager->postMessage(transfereeStatus);

                }
            
            }

            // We did not get a response to the session timer
            // re-invite, so terminate the connection
            else if(sipMsg->isSameMessage(inviteMsg) && 
				getState() == CONNECTION_ESTABLISHED &&
                reinviteState == REINVITING &&
                mSessionReinviteTimer > 0)
            {
                osPrintf("SipConnection::processMessage failed session timer request\n");
                hangUp();
            }

            // A BYE or CANCEL failed to get sent
            else if(!messageIsResponse && 
                (method.compareTo(SIP_BYE_METHOD) == 0 ||
                 method.compareTo(SIP_CANCEL_METHOD) == 0))
            {
                osPrintf("SipConnection::processMessage failed BYE\n");
                setState(CONNECTION_DISCONNECTED, CONNECTION_REMOTE, CONNECTION_CAUSE_DEST_NOT_OBTAINABLE);
            }
            else
            {
				if (reinviteState == REINVITING)
					reinviteState = ACCEPT_INVITE;
                osPrintf("SipConnection::processMessage unhandled failed message\n");
            }

			processedOk = TRUE;
		}

        // Session timer is about to expire send a Re-INVITE to
        // keep the session going
        else if(messageType == SipMessageEvent::SESSION_REINVITE_TIMER)
        {
            extendSessionReinvite();
        }

        // The response was blocked by the user agent authentication 
        // This message is only to keep in sync. with the sequence number
        else if(messageType == SipMessageEvent::AUTHENTICATION_RETRY)
        {
            lastLocalSequenceNumber++;

                if(sipMsg->isResponse())
            {
                // If this was the INVITE we need to update the
                // cached invite so that its cseq is up to date
                if(inviteMsg && sipMsg->isResponseTo(inviteMsg))
                {
                    inviteMsg->setCSeqField(lastLocalSequenceNumber,
                        SIP_INVITE_METHOD);

                    //This was moved to SipUserAgent:
                    // Need to send an ACK to finish transaction
                    //SipMessage* ackMessage = new SipMessage();
                    //ackMessage->setAckData(inviteMsg);
			        //sipUserAgent->send(ackMessage);
                }
#ifdef TEST_PRINT
                else
                {
                    osPrintf("SipConnection::processMessage Authentication failure does not match last invite\n");
                }
#endif
#ifdef TEST
                osPrintf("SipConnection::processMessage incrementing lastSequeneceNumber\n");
#endif
                // If this was the INVITE we need to update the
                // cached invite so that its cseq is up to date

            }
#ifdef TEST
            else
            {
                osPrintf("SipConnection::processMessage request with AUTHENTICATION_RETRY\n");
            }

            

#endif
        }


		else if(sipMsg->isResponse())
		{
#ifdef TEST_PRINT
            ((SipMessage*)sipMsg)->logTimeEvent("PROCESSING");
#endif
			processedOk = processResponse(sipMsg, callInFocus, onHook);
                //numCodecs, rtpCodecs);
		}
		else
		{
#ifdef TEST_PRINT
            ((SipMessage*)sipMsg)->logTimeEvent("PROCESSING");
#endif
			processedOk = processRequest(sipMsg, callInFocus, onHook);
                //numCodecs, rtpCodecs);
		}

#ifdef TEST_PRINT
        sipMsg->dumpTimeLog();
#endif
        
	}
	else
	{
		processedOk = FALSE;
	}

	return(processedOk);
}

UtlBoolean SipConnection::extendSessionReinvite()
{
    UtlBoolean messageSent = FALSE;
    if(inviteFromThisSide && mSessionReinviteTimer > 0 &&
        inviteMsg && getState() == CONNECTION_ESTABLISHED)
    {
        SipMessage reinvite(*inviteMsg);

        // Up the sequence number and resend
        lastLocalSequenceNumber++;
        reinvite.setCSeqField(lastLocalSequenceNumber, SIP_INVITE_METHOD);
        
        // Reset the transport states
        reinvite.resetTransport();
        reinvite.removeLastVia();

		//remove all routes
		UtlString route;
		while ( reinvite.removeRouteUri(0 , &route)){}

		if ( !mRouteField.isNull())
		{
			//set correct route
 			reinvite.setRouteField(mRouteField);
		}

        messageSent = sipUserAgent->send(reinvite);
        delete inviteMsg;
        inviteMsg = new SipMessage(reinvite);

        // Disallow the other side from ReINVITing until this
        // transaction is complete.
        if(messageSent)
            reinviteState = REINVITING;
#ifdef TEST_PRINT
        osPrintf("Session Timer ReINVITE reinviteState: %d\n",
            reinviteState);
#endif
    }

    // A stray timer expired and the call does not exist.
    else if(inviteMsg == NULL &&
        getState() == CONNECTION_IDLE)
    {
        setState(CONNECTION_FAILED, CONNECTION_REMOTE);
    }

    return(messageSent);
}

UtlBoolean SipConnection::processRequest(const SipMessage* request,
                                        UtlBoolean callInFocus,
                                        UtlBoolean onHook)
{
	UtlString sipMethod;
	UtlBoolean processedOk = TRUE;
	request->getRequestMethod(&sipMethod);

   UtlString name = mpCall->getName();
#ifdef TEST_PRINT
    int requestSequenceNum = 0;
	UtlString requestSeqMethod;
	request->getCSeqField(&requestSequenceNum, &requestSeqMethod);

    osPrintf("SipConnection::processRequest inviteMsg: %x requestSequenceNum: %d lastRemoteSequenceNumber: %d connectionState: %d reinviteState: %d\n",
            inviteMsg, requestSequenceNum, lastRemoteSequenceNumber,
			getState(), reinviteState);
#endif

	// INVITE
	// We are being hailed
	if(strcmp(sipMethod.data(),SIP_INVITE_METHOD) == 0)
	{
		osPrintf("%s in INVITE case\n", name.data());
        processInviteRequest(request);
	}

	// SIP REFER received (transfer)
	else if(strcmp(sipMethod.data(),SIP_REFER_METHOD) == 0)
	{
		osPrintf("SIP REFER method received\n");
        processReferRequest(request);
    }

	// SIP ACK received
	else if(strcmp(sipMethod.data(),SIP_ACK_METHOD) == 0)
	{
		osPrintf("%s SIP ACK method received\n", name.data());
        processAckRequest(request);
	}

	// BYE 
	// The call is being shutdown
	else if(strcmp(sipMethod.data(), SIP_BYE_METHOD)  == 0)
	{
		osPrintf("%s %s method received to close down call\n", 
					name.data(), sipMethod.data());
        processByeRequest(request);
	}

	// CANCEL
	// The call is being shutdown
	else if(strcmp(sipMethod.data(), SIP_CANCEL_METHOD) == 0)
	{
		osPrintf("%s %s method received to close down call\n", 
					name.data(), sipMethod.data());
        processCancelRequest(request);
	}
	
    // NOTIFY
    else if(strcmp(sipMethod.data(), SIP_NOTIFY_METHOD) == 0)
	{
		osPrintf("%s method received\n", 
					sipMethod.data());
        processNotifyRequest(request);
	}

    else
    {
        osPrintf("SipConnection::processRequest %s method NOT HANDLED\n", 
					sipMethod.data());
    }

	return(processedOk);
}

void SipConnection::processInviteRequest(const SipMessage* request)
{
#ifdef TEST_PRINT
    OsSysLog::add(FAC_SIP, PRI_DEBUG,
                  "Entering SipConnection::processInviteRequest inviteMsg=0x%08x ", (int)inviteMsg);
#endif

    UtlString sipMethod;
	request->getRequestMethod(&sipMethod);
	UtlString callId;
    //UtlBoolean receiveCodecSet;
    //UtlBoolean sendCodecSet;
	int requestSequenceNum = 0;
	UtlString requestSeqMethod;
    int tagNum = -1;
	
    	request->getCSeqField(&requestSequenceNum, &requestSeqMethod);

        // Create a media connection if one does not yet exist
        if(mConnectionId < 0)
        {
           // Create a new connection in the flow graph
           mpMediaInterface->createConnection(mConnectionId);
        }

        // It is safer to always set the To field tag regardless
        // of whether there are more than 1 via's
        // If there are more than 1 via's we are suppose to put
        // a tag in the to field
        // UtlString via;
        //if(request->getViaField(&via, 1))
        {
            UtlString toAddr;
            UtlString toProto;
            int toPort;
            UtlString tag;

            request->getToAddress(&toAddr, &toPort, &toProto, NULL,
                NULL, &tag);
            // The tag is not set, add it
            if(tag.isNull())
            {
                tagNum = rand();
            }
        }

         // Replaces is independent of REFER so
         // do not assume there must be a Refer-To or Refer-By
         UtlBoolean hasReplaceHeader = FALSE;
         UtlBoolean doesReplaceCallLegExist = FALSE;
         int       replaceCallLegState = -1 ;
         UtlString replaceCallId;
         UtlString replaceToTag;
         UtlString replaceFromTag;
         hasReplaceHeader = request->getReplacesData(replaceCallId, replaceToTag,
               replaceFromTag);
#ifdef TEST_PRINT
        osPrintf("SipConnection::processInviteRequest - \n\thasReplaceHeader %d \n\treplaceCallId %s \n\treplaceToTag %s\n\treplaceFromTag %s\n", 
                     hasReplaceHeader,
                     replaceCallId.data(),
                     replaceToTag.data(),
                     replaceFromTag.data());
#endif
         if (hasReplaceHeader)
         {
            // Ugly assumption that this is a CpPeerCall
            doesReplaceCallLegExist = 
                  ((CpPeerCall*)mpCall)->getConnectionState(
                  replaceCallId.data(), replaceToTag.data(), 
                  replaceFromTag.data(),
                  replaceCallLegState,
                  TRUE);
         }      

		// If this is previous to the last invite
		if(inviteMsg && requestSequenceNum < lastRemoteSequenceNumber)
		{
			SipMessage sipResponse;
			sipResponse.setBadTransactionData(request);
            if(tagNum >= 0)
            {
                sipResponse.setToFieldTag(tagNum);
            }
			sipUserAgent->send(sipResponse);
		}

		// if this is the same invite 
        else if(inviteMsg && 
            !inviteFromThisSide &&
            requestSequenceNum == lastRemoteSequenceNumber)
        {
            UtlString viaField;
            inviteMsg->getViaField(&viaField, 0);
            UtlString oldInviteBranchId;
            SipMessage::getViaTag(viaField.data(), 
                                  "branch", 
                                  oldInviteBranchId);
            request->getViaField(&viaField, 0);
            UtlString newInviteBranchId;
            SipMessage::getViaTag(viaField.data(), 
                                  "branch", 
                                  newInviteBranchId);

            // from a different branch
            if(!oldInviteBranchId.isNull() &&
               oldInviteBranchId.compareTo(newInviteBranchId) != 0)
            {
                SipMessage sipResponse;
                sipResponse.setLoopDetectedData(request);
                if(tagNum >= 0)
                {
                    sipResponse.setToFieldTag(tagNum);
                }
			    sipUserAgent->send(sipResponse);
            }
            else
            {
                // no-op, ignore duplicate INVITE
                OsSysLog::add(FAC_SIP, PRI_WARNING, 
                    "SipConnection::processInviteRequest received duplicate request");
            }
        }
      else if (hasReplaceHeader && !doesReplaceCallLegExist)
      {
         // has replace header, but it does not match this call, so send 481.
           SipMessage badTransaction;
           badTransaction.setBadTransactionData(request);
           sipUserAgent->send(badTransaction);

           // Bug 3658: as transfer target, call waiting disabled, transferee comes via proxy 
           // manager, sequence of messages is 2 INVITEs from proxy, 486 then this 481.
           // When going from IDLE to DISCONNECTED, set the dropping flag so call will get 
           // cleaned up
           mpCall->setDropState(TRUE);
           setState(CONNECTION_DISCONNECTED, CONNECTION_REMOTE);
           setState(CONNECTION_DISCONNECTED, CONNECTION_LOCAL);

           osPrintf("SipConnection::processInviteRequest - CONNECTION_DISCONNECTED, replace call leg does not match\n");
      }
		// Proceed to offering state 
		else if((getState() == CONNECTION_IDLE && // New call
            mConnectionId > 0 && // Media resources available
            (mLineAvailableBehavior == RING || // really not busy
             mLineAvailableBehavior == RING_SILENT || // pretend to ring
             mLineAvailableBehavior == FORWARD_ON_NO_ANSWER ||
             mLineAvailableBehavior == FORWARD_UNCONDITIONAL ||
             mLineBusyBehavior == FAKE_RING))|| // pretend not busy
            // I do not remember what the follow case is for:
			(getState() == CONNECTION_OFFERING && // not call to self
			mRemoteIsCallee && !request->isSameMessage(inviteMsg)))
		{
			lastRemoteSequenceNumber = requestSequenceNum;

		
			//set the allows field
			if(mAllowedRemote.isNull())
			{
				// Get the methods the other side supports
				request->getAllowField(mAllowedRemote);
			}

			// This should be the first SIP message
			// Set the connection's callId
			getCallId(&callId);
			if(callId.isNull())
			{
				request->getCallIdField(&callId);
				setCallId(callId.data());
			}

		    // Save a copy of the INVITE
		    inviteMsg = new SipMessage(*request);
            inviteFromThisSide = FALSE;
            setCallerId();
            //save line Id
            UtlString uri;
            UtlString angleBracketsUri;
            request->getRequestUri(&uri);
            mLocalContact = uri ;

            int cause = CONNECTION_CAUSE_NORMAL;

            // Replaces is independent of REFER so
            // do not assume there must be a Refer-To or Refer-By
               // Assume the replaces call leg does not exist if the call left
               // state state is not established.  The latest RFC states that 
               // one cannot use replaces for an early dialog
            if (doesReplaceCallLegExist && 
                  (replaceCallLegState != CONNECTION_ESTABLISHED))
            {
               doesReplaceCallLegExist = FALSE ;
            }

            
            // Allow transfer if the call leg exists and the call leg is
            // established.  Transferring a call while in early dialog is
            // illegal.
            if (doesReplaceCallLegExist)
            {
                cause = CONNECTION_CAUSE_TRANSFER;

                // Setup the meta event data
                int metaEventId = mpCallManager->getNewMetaEventId();
                const char* metaEventCallIds[2];
                metaEventCallIds[0] = callId.data();        // target call Id
                metaEventCallIds[1] = replaceCallId.data(); // original call Id
                mpCall->startMetaEvent(metaEventId, 
                        PtEvent::META_CALL_TRANSFERRING,
                        2, metaEventCallIds);
                mpCall->setCallType(CpCall::CP_TRANSFER_TARGET_TARGET_CALL);

#ifdef TEST_PRINT
                osPrintf("SipConnection::processInviteRequest replaceCallId: %s, toTag: %s, fromTag: %s\n", replaceCallId.data(), 
                    replaceToTag.data(), replaceFromTag.data());
#endif
            }
            else
            {
                // This call does not contain a REPLACES header, however, it 
                // may still be part of a blind transfer, so look for the 
                // referred-by or requested-by headers
                UtlString referredBy;
                UtlString requestedBy;
                request->getReferredByField(referredBy);
                request->getRequestedByField(requestedBy);
                if (!referredBy.isNull() || !requestedBy.isNull())
                {
                    cause = CONNECTION_CAUSE_TRANSFER;
                    mpCall->setCallType(CpCall::CP_TRANSFER_TARGET_TARGET_CALL);
                }


                mpCall->startMetaEvent( mpCallManager->getNewMetaEventId(), 
				        PtEvent::META_CALL_STARTING, 
						0, 
						0,
						mRemoteIsCallee);

                if (!mRemoteIsCallee)	// inbound call
				{
					mpCall->setCallState(mResponseCode, mResponseText, PtCall::ACTIVE);
					// setState(CONNECTION_INITIATED, CONNECTION_REMOTE, PtEvent::CAUSE_NEW_CALL);
					setState(CONNECTION_ESTABLISHED, CONNECTION_REMOTE, PtEvent::CAUSE_NEW_CALL);
					setState(CONNECTION_INITIATED, CONNECTION_LOCAL, PtEvent::CAUSE_NEW_CALL);
				}
            }


            // If this is not part of a call leg replaces operation
            // we normally go to offering so that the application
            // can decide to accept, rejecto or redirect
            if(!doesReplaceCallLegExist)
            {
			      //osPrintf("taking call, returning ringing\n");
				  setState(CONNECTION_OFFERING, CONNECTION_LOCAL, cause);
			      //request->getCallIdField(&callId);
            }

            // Always get the remote contact as it may can change over time
            UtlString contactInResponse;
			if (request->getContactUri(0 , &contactInResponse))
			{
				mRemoteContact.remove(0);
				mRemoteContact.append(contactInResponse);
			}
                
            // Get the route for subsequent requests
            request->buildRouteField(&mRouteField);
            osPrintf("INVITE set mRouteField: %s\n", mRouteField.data());

            // Set the to tag if it is not set in the Invite
            if(tagNum >= 0)
            {
                inviteMsg->setToFieldTag(tagNum);

                // Update the cached from field after saving the tag
                inviteMsg->getToUrl(mFromUrl);
            }
//#ifdef TEST_PRINT
			osPrintf("Offering delay: %d\n", mOfferingDelay);
//#endif
            // If we are replacing a call let answer the call
            // immediately do not go to offering first.
			if(doesReplaceCallLegExist)
            {              
                // Go immediately to answer the call
                answer();                           

                // Bob 11/16/01: The following setState was added to close a race between
                // the answer (above) and hangup (below).  The application layer is notified
                // of state changed on on the replies to these messages.  These can lead to
                // dropped transfer if the BYE reponse is received before INVITE response.          
                setState(CONNECTION_ESTABLISHED, CONNECTION_REMOTE, CONNECTION_CAUSE_TRANSFER);

                // Drop the leg to be replaced
                ((CpPeerCall*)mpCall)->hangUp(replaceCallId.data(), 
                                            replaceToTag.data(), 
                                            replaceFromTag.data());
            }
            else if(mOfferingDelay == IMMEDIATE)
			{
                accept(mForwardOnNoAnswerSeconds);
			}
			// The connection stays in Offering state until timeout
			// or told to do otherwise
			else 
			{
				// Should send a Trying response
                //SipMessage offeringTryingMessage;
                //offeringTryingMessage.setTryingResponseData(inviteMsg);
                //sipUserAgent->send(offeringTryingMessage);

                // If the delay is not forever, setup a timer to expire
                if(mOfferingDelay > IMMEDIATE) setOfferingTimer(mOfferingDelay);

                // Other wise do nothing, let it sit in offering forever
            }
		}

        // Automatically Answer
        //else if(getState() == CONNECTION_IDLE &&
        //    ((callInFocus && mLineAvailableBehavior == AUTO_ANSWER) ||
        //    (!callInFocus && mLineBusyBehavior == FORCED_ANSWER)))
        //{
        //    osPrintf("ProcessRequest Automatic Answer not implemented\n");
            // Get the route for subsequent requests
        //    request->buildRouteField(&mRouteField);

        //    setState(CONNECTION_ESTABLISHED);
        //}


        /* This is now done at the CpPeerCall layer when offering expires
        // Not busy and Unconditional forward
        else if(getState() == CONNECTION_IDLE &&
            callInFocus && mLineAvailableBehavior == FORWARD_UNCONDITIONAL &&
            !mForwardUnconditional.isNull())
        {
            osPrintf("Unconditional forwarding of call to \"%s\"\n",
						mForwardUnconditional.data());

            sipResponse = new SipMessage();
			sipResponse->setForwardResponseData(request, 
						mForwardUnconditional.data());
            if(tagNum >= 0)
            {
                sipResponse->setToFieldTag(tagNum);
            }
			sipUserAgent->send(sipResponse);

            // This should be the first SIP message
            // Set the connection's callId
            getCallId(&callId);
            if(callId.isNull())
            {
	            request->getCallIdField(&callId);
	            setCallId(callId.data());
            }
            setState(CONNECTION_FAILED);
			//mpCallUiContext->setDisplayParameter(CALL_STATUS_FIELD,
            //                  "Forwarding call");
        }*/

		// Re-INVITE allowed
		else if(inviteMsg && requestSequenceNum > lastRemoteSequenceNumber &&
			getState() == CONNECTION_ESTABLISHED && 
			reinviteState == ACCEPT_INVITE)
		{
			// Keep track of the last sequence number;
			lastRemoteSequenceNumber = requestSequenceNum;

            
            // Always get the remote contact as it may can change over time
            UtlString contactInResponse;
			if (request->getContactUri(0 , &contactInResponse))
			{
				mRemoteContact.remove(0);
				mRemoteContact.append(contactInResponse);
			}
                
            // The route set is set only on the initial dialog transaction
            //request->buildRouteField(&mRouteField);
            //osPrintf("reINVITE set mRouteField: %s\n", mRouteField.data());

            // Do not allow other Requests until the ReINVITE is complete
            reinviteState = REINVITED;
#ifdef TEST_PRINT
            osPrintf("INVITE reinviteState: %d\n", reinviteState);
#endif

            UtlString rtpAddress;
            int receiveRtpPort;
            SdpCodecFactory supportedCodecs;
            mpMediaInterface->getCapabilities(mConnectionId, 
                                              rtpAddress, 
                                              receiveRtpPort,
                                              supportedCodecs);

            int numMatchingCodecs = 0;
            SdpCodec** matchingCodecs = NULL;

			// Get the RTP info from the message if present
			// Should check the content type first
			if(getInitialSdpCodecs(request, 
                supportedCodecs, numMatchingCodecs, matchingCodecs,
                remoteRtpAddress, remoteRtpPort))
			{
				// If the codecs match send an OK
				if(numMatchingCodecs > 0)
				{
                    // Setup media channel
                    mpMediaInterface->setConnectionDestination(mConnectionId,
                        remoteRtpAddress.data(),
                        remoteRtpPort);
                    osPrintf("ReINVITE RTP SENDING address: %s port: %d\n", remoteRtpAddress.data(), remoteRtpPort);

                    // Far side requested hold
                    if(remoteRtpPort == 0 || 
                        remoteRtpAddress.compareTo("0.0.0.0") == 0)
                    {
                        //receiveRtpPort = 0;
                        // Leave the receive on to drain the buffers
                        //mpMediaInterface->stopRtpReceive(mConnectionId);
					    mpMediaInterface->stopRtpSend(mConnectionId);
                    	mRemoteRequestedHold = TRUE;
					}
                    else if(remoteRtpPort > 0)
                    {
                        //SdpCodec sndCodec((SdpCodec::SdpCodecTypes) sendCodec);
                        //SdpCodec rcvCodec((SdpCodec::SdpCodecTypes) receiveCodec);
                        mpMediaInterface->startRtpSend(mConnectionId,
                            numMatchingCodecs, matchingCodecs);
                        mpMediaInterface->startRtpReceive(mConnectionId,
                            numMatchingCodecs, matchingCodecs);
                    }
                    

                    // Send a INVITE OK response
                    osPrintf("Sending INVITE OK\n");
                    //UtlString rtpAddress;
                    //int receiveRtpPort = 0;

                    // Should not need to do this again
                    //int numCodecs = mpMediaInterface->getNumCodecs(mConnectionId);
                    //SdpCodec* rtpCodecs = new SdpCodec[numCodecs];
                    //mpMediaInterface->getCapabilities(mConnectionId, 
                    //                                rtpAddress, 
                    //                                receiveRtpPort,
                    //                                rtpCodecs, 
                    //                                numCodecs, 
                    //                                numCodecs);


                    if(remoteRtpPort <= 0 ||
                        remoteRtpAddress.compareTo("0.0.0.0") == 0)
                    {
                        rtpAddress.remove(0);
                        rtpAddress.append("0.0.0.0");  // hold address
                    }
                    osPrintf("REINVITE: using RTP address: %s\n",
                        rtpAddress.data());

					SipMessage sipResponse;
					sipResponse.setInviteOkData(request, rtpAddress.data(), 
						receiveRtpPort, numMatchingCodecs, matchingCodecs,
                        mDefaultSessionReinviteTimer);
                    if(tagNum >= 0)
                    {
                        sipResponse.setToFieldTag(tagNum);
                    }
					sipUserAgent->send(sipResponse);

					// Save the invite for future reference
					if(inviteMsg)
					{
						delete inviteMsg;
					}
					inviteMsg = new SipMessage(*request);
                    inviteFromThisSide = FALSE;
                    setCallerId();


                    if(tagNum >= 0)
                    {
                        inviteMsg->setToFieldTag(tagNum);

                        // Update the cached from field after saving the tag
                        inviteMsg->getToUrl(mFromUrl);
                    }

				}

				// If the codecs do not match send an error
				else
				{
					// No common codecs send INVITE error response
					SipMessage sipResponse;
					sipResponse.setInviteBadCodecs(request);
                    if(tagNum >= 0)
                    {
                        sipResponse.setToFieldTag(tagNum);
                    }
					sipUserAgent->send(sipResponse);
				}
			}
			else
			{
				osPrintf("No SDP in reINVITE\n");

                // Send back the whole set of supported codecs
                osPrintf("REINVITE: using RTP address: %s\n",
                        rtpAddress.data());

                // Use the full set of capabilities as the other
                // side did not give SDP to find common/matching
                // codecs
                supportedCodecs.getCodecs(numMatchingCodecs, 
                    matchingCodecs);

				SipMessage sipResponse;
				sipResponse.setInviteOkData(request, rtpAddress.data(), 
					receiveRtpPort, numMatchingCodecs, matchingCodecs,
                    mDefaultSessionReinviteTimer);
                if(tagNum >= 0)
                {
                    sipResponse.setToFieldTag(tagNum);
                }
				sipUserAgent->send(sipResponse);

				// Save the invite for future reference
				if(inviteMsg)
				{
					delete inviteMsg;
				}
				inviteMsg = new SipMessage(*request);
                inviteFromThisSide = FALSE;
                setCallerId();

                if(tagNum >= 0)
                {
                    inviteMsg->setToFieldTag(tagNum);

                    // Update the cached from field after saving the tag
                    inviteMsg->getToUrl(mFromUrl);
                }
			}

            // Free up the codec copies and array
            for(int codecIndex = 0; codecIndex < numMatchingCodecs; codecIndex++)
            {
                delete matchingCodecs[codecIndex];
                matchingCodecs[codecIndex] = NULL;
            }
            delete[] matchingCodecs;
            matchingCodecs = NULL;

		}

        // Busy, but queue call
        else if(getState() == CONNECTION_IDLE &&
            ((/*!callInFocus && */ mLineBusyBehavior == QUEUE_SILENT) ||
            (/*!callInFocus && */ mLineBusyBehavior == QUEUE_ALERT) ||
            (/*!callInFocus && */ mLineBusyBehavior == BUSY && 
                request->isRequestDispositionSet(SIP_DISPOSITION_QUEUE))))
        {
            lastRemoteSequenceNumber = requestSequenceNum;

               osPrintf("Busy, queuing call\n");

            // This should be the first SIP message
            // Set the connection's callId
            getCallId(&callId);
            if(callId.isNull())
            {
	            request->getCallIdField(&callId);
	            setCallId(callId.data());
            }

            // Always get the remote contact as it may can change over time
            UtlString contactInResponse;
            if (request->getContactUri(0 , &contactInResponse))
            {
                mRemoteContact.remove(0);
                mRemoteContact.append(contactInResponse);
            }

            // Get the route for subsequent requests
            request->buildRouteField(&mRouteField);
            osPrintf("queuedINVITE set mRouteField: %s\n", mRouteField.data());

            // Get the capabilities to figure out the matching codecs
            UtlString rtpAddress;
            int receiveRtpPort;
            SdpCodecFactory supportedCodecs;
            mpMediaInterface->getCapabilities(mConnectionId, 
                                             rtpAddress, 
                                             receiveRtpPort,
                                             supportedCodecs);

            // Get the codecs
            int numMatchingCodecs = 0;
            SdpCodec** matchingCodecs = NULL;
            getInitialSdpCodecs(request, 
                supportedCodecs,
                numMatchingCodecs, matchingCodecs,
                remoteRtpAddress, remoteRtpPort);

            // We are not suppose to go into offering state before, after 
            // or at all when queuing a call.


            // Save a copy of the invite for future reference
            inviteMsg = new SipMessage(*request);
            inviteFromThisSide = FALSE;
            setCallerId();

            // Create and send a queued response
            SipMessage sipResponse;
	        sipResponse.setQueuedResponseData(request);
            if(tagNum >= 0)
            {
                sipResponse.setToFieldTag(tagNum);
            }
	        sipUserAgent->send(sipResponse);
            setState(CONNECTION_QUEUED, CONNECTION_LOCAL);

            // Free up the codec copies and array
            for(int codecIndex = 0; codecIndex < numMatchingCodecs; codecIndex++)
            {
                delete matchingCodecs[codecIndex];
                matchingCodecs[codecIndex] = NULL;
            }
            delete[] matchingCodecs;
            matchingCodecs = NULL;
        }

        // Forward on busy
        else if(getState() == CONNECTION_IDLE &&
            /*!callInFocus && */ mLineBusyBehavior == FORWARD_ON_BUSY &&
            !mForwardOnBusy.isNull())
        {
            osPrintf("Busy, forwarding call to \"%s\"\n",
						mForwardOnBusy.data());

            SipMessage sipResponse;
	        sipResponse.setForwardResponseData(request, 
				        mForwardOnBusy.data());
            if(tagNum >= 0)
            {
                sipResponse.setToFieldTag(tagNum);
            }
	        sipUserAgent->send(sipResponse);


           osPrintf("SipConnection::processInviteRequest - CONNECTION_FAILED, cause BUSY : 2390\n");
           setState(CONNECTION_FAILED, CONNECTION_LOCAL, CONNECTION_CAUSE_BUSY);

        }

		// If busy
		else
		{
			osPrintf("Call: busy returning response\n");
            osPrintf("Busy behavior: %d infocus: %d forward on busy URL: %s\n",
                mLineBusyBehavior, -5/*callInFocus*/, mForwardOnBusy.data());

            // This should be the first SIP message
            // Set the connection's callId
            getCallId(&callId);
            if(callId.isNull())
            {
	            request->getCallIdField(&callId);
	            setCallId(callId.data());
            }

			// Send back a busy INVITE response
			SipMessage sipResponse;
			sipResponse.setInviteBusyData(request);
            if(tagNum >= 0)
            {
                sipResponse.setToFieldTag(tagNum);
            }
			sipUserAgent->send(sipResponse);

            // Special case:
            // We sent the invite to our self
            if(inviteMsg && request->isSameMessage(inviteMsg))
            {
                // Do not set state here it will be done when
                // the response comes back
            }

            else
            {
 				if (reinviteState != REINVITING)
 				{
 					osPrintf("SipConnection::processInviteRequest - busy, not HELD state, setting state to CONNECTION_FAILED\n");
 					setState(CONNECTION_FAILED, CONNECTION_LOCAL, CONNECTION_CAUSE_BUSY);
 				}
            }
		}
      
#ifdef TEST_PRINT        
    OsSysLog::add(FAC_SIP, PRI_DEBUG,
                  "Leaving SipConnection::processInviteRequest inviteMsg=0x%08x ", (int)inviteMsg);
#endif
} // End of processInviteRequest

void SipConnection::processReferRequest(const SipMessage* request)
{
	mIsAcceptSent = FALSE;

        UtlString referTo;
        UtlString referredBy;
        request->getReferredByField(referredBy);
        request->getReferToField(referTo);

		//reject Refers to non sip URLs
		Url referToUrl(referTo);
		UtlString protocol;
		referToUrl.getUrlType(protocol);

        int connectionState = getState();
        // Cannot transfer if there is not already a call setup
        if(connectionState != CONNECTION_ESTABLISHED &&
            connectionState != CONNECTION_IDLE)
        {
            SipMessage sipResponse;
			sipResponse.setReferDeclinedData(request);
			sipUserAgent->send(sipResponse);
        }

        // If there is not exactly one Refered-By 
        // or not exactly one Refer-To header
        // or there is already a REFER in progress
        else if(request->getHeaderValue(1, SIP_REFERRED_BY_FIELD) != NULL||
            request->getHeaderValue(1, SIP_REFER_TO_FIELD) != NULL ||
            mReferMessage)
        {
            SipMessage sipResponse;
			sipResponse.setRequestBadRequest(request);
			sipUserAgent->send(sipResponse);
        }

		//if Url is not of type Sip
		else if (protocol.index("SIP" , 0, UtlString::ignoreCase) != 0)
		{
            SipMessage sipResponse;
			sipResponse.setRequestBadUrlType(request);
			sipUserAgent->send(sipResponse);
		}
        // Give the transfer a try.
        else if(connectionState == CONNECTION_ESTABLISHED)
        {
            // Create a second call if it does not exist already
            // Set the target call id in this call
            // Set this call's type to transferee original call
            UtlString targetCallId;
            Url targetUrl(referTo);
            targetUrl.getHeaderParameter(SIP_CALLID_FIELD, targetCallId);
            // targetUrl.removeHeaderParameters();
            targetUrl.toString(referTo);
            //SipMessage::parseParameterFromUri(referTo.data(), "Call-ID",
            //    &targetCallId);
#ifdef TEST_PRINT
            osPrintf("SipConnection::processRequest REFER refer-to: %s callid: %s\n",
                referTo.data(), targetCallId.data());
#endif

            // Setup the meta event data
            int metaEventId = mpCallManager->getNewMetaEventId();
            const char* metaEventCallIds[2];
            UtlString thisCallId;
            getCallId(&thisCallId);
            metaEventCallIds[0] = targetCallId.data();
            metaEventCallIds[1] = thisCallId.data();
            
            // Mark the begining of a transfer meta event in this call
            mpCall->startMetaEvent(metaEventId, 
                PtEvent::META_CALL_TRANSFERRING,
                2, metaEventCallIds);
            
            // I am not sure we want the focus change to
            // be automatic.  It is here for now to make it work
            CpIntMessage yieldFocus(CallManager::CP_YIELD_FOCUS, (int)mpCall);
            mpCallManager->postMessage(yieldFocus);

            // The new call by default assumes focus.
            // Mark the new call as part of this transfer meta event
            mpCallManager->createCall(&targetCallId, metaEventId,
                PtEvent::META_CALL_TRANSFERRING, 2, metaEventCallIds);
            mpCall->setTargetCallId(targetCallId.data());
            mpCall->setCallType(CpCall::CP_TRANSFEREE_ORIGINAL_CALL);

            // Send a message to the target call to create the
            // connection and send the INVITE
            UtlString remoteAddress;
            getRemoteAddress(&remoteAddress);
            CpMultiStringMessage transfereeConnect(CallManager::CP_TRANSFEREE_CONNECTION,
                targetCallId.data(), referTo.data(), referredBy.data(), thisCallId.data(),
                remoteAddress.data());
#ifdef TEST_PRINT
            osPrintf("SipConnection::processRequest posting CP_TRANSFEREE_CONNECTION\n");
#endif
            mpCallManager->postMessage(transfereeConnect);

            // Send an accepted response, a NOTIFY is sent later to
            // provide the resulting outcome
            SipMessage sipResponse;
			sipResponse.setResponseData(request, SIP_ACCEPTED_CODE, 
                SIP_ACCEPTED_TEXT);
			mIsAcceptSent = sipUserAgent->send(sipResponse);

            // Save a copy for the NOTIFY
            mReferMessage = new SipMessage(*request);

        }

        else if(connectionState == CONNECTION_IDLE)
        {
            // Set the identity of this connection
            request->getFromUrl(mToUrl);
            request->getToUrl(mFromUrl);
            UtlString callId;
            request->getCallIdField(&callId);
            setCallId(callId);
            UtlString fromField;
            mToUrl.toString(fromField);

            // Post a message to add a connection to this call
            CpMultiStringMessage transfereeConnect(CallManager::CP_TRANSFEREE_CONNECTION, 
                callId.data(), referTo.data(),
                referredBy.data(), callId.data(), fromField.data());
            mpCallManager->postMessage(transfereeConnect);

            // Assume focus, probably not the right thing
            //mpCallManager->unhold(callId.data());

            // Send back a response
            SipMessage referResponse;
            referResponse.setResponseData(request, SIP_ACCEPTED_CODE, 
                SIP_ACCEPTED_TEXT);
			mIsAcceptSent = sipUserAgent->send(referResponse);

            // Save a copy for the NOTIFY
            mReferMessage = new SipMessage(*request);

            setState(CONNECTION_UNKNOWN, CONNECTION_REMOTE);
        }
} // end of processReferRequest

void SipConnection::processNotifyRequest(const SipMessage* request)
{
    UtlString eventType;
    request->getEventField(eventType);
    
    // IF this is a REFER result notification
    int refIndex = eventType.index(SIP_EVENT_REFER);
    if(refIndex >= 0)
    {
        UtlString contentType;
        request->getContentType(&contentType);
        const HttpBody* body = request->getBody();

        // If we have a body that contains the REFER status/outcome
        if(	body && 
			( contentType.index(CONTENT_TYPE_SIP_APPLICATION, 0, UtlString::ignoreCase) == 0 ||
				contentType.index(CONTENT_TYPE_MESSAGE_SIPFRAG, 0, UtlString::ignoreCase) == 0) )
        {
            // Send a NOTIFY response, we like the content
            // Need to send this ASAP and before the BYE
            SipMessage notifyResponse;
            notifyResponse.setOkResponseData(request);
            sipUserAgent->send(notifyResponse);

            const char* bytes;
            int numBytes;
            body->getBytes(&bytes, &numBytes);

            SipMessage response(bytes, numBytes);

            int state;
            int cause;
            int responseCode = response.getResponseStatusCode();
			mResponseCode = responseCode;
			response.getResponseStatusText(&mResponseText);

            if(responseCode == SIP_OK_CODE)
            {
                state = CONNECTION_ESTABLISHED;
                cause = CONNECTION_CAUSE_NORMAL;
            }
            else if(responseCode == SIP_DECLINE_CODE)
            {
                state = CONNECTION_FAILED;
                cause = CONNECTION_CAUSE_CANCELLED;
            }
            else if(responseCode == SIP_BAD_METHOD_CODE ||
                responseCode == SIP_UNIMPLEMENTED_METHOD_CODE)
            {
                state = CONNECTION_FAILED;
                cause = CONNECTION_CAUSE_INCOMPATIBLE_DESTINATION;
            }
            else if(responseCode == SIP_RINGING_CODE)
            {
                // Note: this is the state for the transferee
                // side of the connection between the transferee
                // and the transfer target (i.e. not the target
                // which is actually ringing)
                state = CONNECTION_OFFERING;
                cause = CONNECTION_CAUSE_NORMAL;
            }
            else if(responseCode == SIP_EARLY_MEDIA_CODE)
            {
                // Note: this is the state for the transferee
                // side of the connection between the transferee
                // and the transfer target (i.e. not the target
                // which is actually ringing)
                state = CONNECTION_ESTABLISHED;
                cause = CONNECTION_CAUSE_UNKNOWN;
            }
            else if (responseCode == SIP_SERVICE_UNAVAILABLE_CODE)
            {
                state = CONNECTION_FAILED;
                cause = CONNECTION_CAUSE_SERVICE_UNAVAILABLE;
            }
            else
            {
                state = CONNECTION_FAILED;
                cause = CONNECTION_CAUSE_BUSY;
            }

            if(responseCode >= SIP_OK_CODE)
            {
                // Signal the connection in the target call with the final status
                UtlString targetCallId;
                UtlString toField;
                mToUrl.toString(toField);
                mpCall->getTargetCallId(targetCallId);
                CpMultiStringMessage transferControllerStatus(CallManager::CP_TRANSFER_CONNECTION_STATUS,
                    targetCallId.data(), toField.data(), 
                    NULL, NULL, NULL,
                    state, cause);
#ifdef TEST_PRINT
                osPrintf("SipConnection::processNotifyRequest posting CP_TRANSFER_CONNECTION_STATUS to call: %s\n",
                    targetCallId.data());
#endif
                mpCallManager->postMessage(transferControllerStatus);

                // Drop this connection, the transfer succeeded
                // Do the drop at the last possible momment so that
                // both calls have some overlap.
                if(responseCode == SIP_OK_CODE) doHangUp();
            }

#ifdef TEST_PRINT
            else
            {
                osPrintf("SipConnection::processNotifyRequest ignoring REFER response %d\n",
                    responseCode);
            }
#endif
        } // End if body in the NOTIFY

        // Unknown NOTIFY content type for this event type REFER 
        else
        {
            // THis probably should be some sort of error response
            // Send a NOTIFY response
            SipMessage notifyResponse;
            notifyResponse.setOkResponseData(request);
            sipUserAgent->send(notifyResponse);
        }

    } // End REFER NOTIFY
} // End of processNotifyRequest

void SipConnection::processAckRequest(const SipMessage* request)
{
    //UtlBoolean receiveCodecSet;
    //UtlBoolean sendCodecSet;
	int requestSequenceNum = 0;
	UtlString requestSeqMethod;

	request->getCSeqField(&requestSequenceNum, &requestSeqMethod);

		// If this ACK belongs to the last INVITE and
		// we are accepting the INVITE
		if(getState() == CONNECTION_ESTABLISHED &&
			(lastRemoteSequenceNumber == requestSequenceNum || mIsAcceptSent))
		{
         UtlString rtpAddress;
         int receiveRtpPort;
         SdpCodecFactory supportedCodecs;
         mpMediaInterface->getCapabilities(mConnectionId, 
                                          rtpAddress, 
                                          receiveRtpPort,
                                          supportedCodecs);

         // If codecs set ACK in SDP
			// If there is an SDP body find the best 
			//codecs, address & port
            int numMatchingCodecs = 0;
            SdpCodec** matchingCodecs = NULL;
            if(getInitialSdpCodecs(request, 
                supportedCodecs, 
                numMatchingCodecs, matchingCodecs,
                remoteRtpAddress, remoteRtpPort) &&
                numMatchingCodecs > 0)
            {
			      // Set up the remote RTP sockets
               mpMediaInterface->setConnectionDestination(mConnectionId,
                  remoteRtpAddress.data(),
                  remoteRtpPort);

               osPrintf("RTP SENDING address: %s port: %d\n", remoteRtpAddress.data(), remoteRtpPort);

               mpMediaInterface->startRtpSend(mConnectionId,
                     numMatchingCodecs, matchingCodecs);
            }
#ifdef TEST_PRINT
            osPrintf("ACK reinviteState: %d\n", reinviteState);
#endif

            // Free up the codec copies and array
            for(int codecIndex = 0; codecIndex < numMatchingCodecs; codecIndex++)
            {
                delete matchingCodecs[codecIndex];
                matchingCodecs[codecIndex] = NULL;
            }
            if(matchingCodecs) delete[] matchingCodecs;
            matchingCodecs = NULL;

			if(reinviteState == ACCEPT_INVITE)
			{
                inviteFromThisSide = FALSE;
                setCallerId();
				setState(CONNECTION_ESTABLISHED, CONNECTION_REMOTE);
                
                // If the other side did not send an Allowed field in
                // the INVITE, send an OPTIONS method to see if the
                // otherside supports methods such as REFER
				if(mAllowedRemote.isNull())
				{
					lastLocalSequenceNumber++;
					SipMessage optionsRequest;
					optionsRequest.setOptionsData(inviteMsg, mRemoteContact, inviteFromThisSide,
						lastLocalSequenceNumber, mRouteField.data());
					sipUserAgent->send(optionsRequest);
				}
			}
			// Reset to allow sub sequent re-INVITE
			else if(reinviteState == REINVITED)
			{
                osPrintf("ReINVITE ACK - ReINVITE allowed again\n");
				reinviteState = ACCEPT_INVITE;
			}

            // If we are in the middle of a transfer meta event
            // on the target phone and target call it ends here
            if(mpCall->getCallType() == 
              CpCall::CP_TRANSFER_TARGET_TARGET_CALL)
            {
                mpCall->setCallType(CpCall::CP_NORMAL_CALL);                
            }
		}

		// Else error response to the ACK
		//getState() != CONNECTION_ESTABLISHED
		// requestSequenceNum != requestSequenceNum
		else
		{
			 osPrintf("Ignoring ACK connectionState: %d request CSeq: %d invite CSeq: %d\n",
				getState(), requestSequenceNum, requestSequenceNum);

             // If there is no invite message then shut down this connection
             if(!inviteMsg) setState(CONNECTION_FAILED, CONNECTION_LOCAL);

			 // ACKs do not get a response
		}

} // End of processAckRequest

void SipConnection::processByeRequest(const SipMessage* request)
{
#ifdef TEST_PRINT
        OsSysLog::add(FAC_SIP, PRI_WARNING,
                    "Entering SipConnection::processByeRequest inviteMsg=0x%08x ", (int)inviteMsg);
#endif
	int requestSequenceNum = 0;
	UtlString requestSeqMethod;

	request->getCSeqField(&requestSequenceNum, &requestSeqMethod);

		if(inviteMsg && lastRemoteSequenceNumber < requestSequenceNum)
		{
			lastRemoteSequenceNumber = requestSequenceNum;

            // Stop the media usage ASAP
            mpMediaInterface->stopRtpSend(mConnectionId);
            // Let the receive go for now to drain the socket
            // in case the other side is still sending RTP
            //mpMediaInterface->stopRtpReceive(mConnectionId);

			// Build an OK response
			SipMessage sipResponse;
			sipResponse.setOkResponseData(request);
			sipUserAgent->send(sipResponse);
			
#ifdef SINGLE_CALL_TRANSFER
            // Check for blind transfer
            int alsoIndex = 0;
            UtlString alsoUri;
            UtlString transferController;
            request->getFromField(&transferController);
            while(request->getAlsoUri(alsoIndex, &alsoUri))
            {
                ((CpPeerCall*)mpCall)->addParty(alsoUri.data(), 
                    transferController.data(), NULL);
                alsoIndex++;
            }

#else
			UtlString thereAreAnyAlsoUri;
            if(request->getAlsoUri(0, &thereAreAnyAlsoUri))
            {

				// Two call model BYE Also transfer

				// Create a second call if it does not exist already
				// Set the target call id in this call
				// Set this call's type to transferee original call
				UtlString targetCallId;

				// I am not sure we want the focus change to
				// be automatic.  It is here for now to make it work
				CpIntMessage yieldFocus(CallManager::CP_YIELD_FOCUS, (int)mpCall);
				mpCallManager->postMessage(yieldFocus);

				// The new call by default assumes focus
				mpCallManager->createCall(&targetCallId);
				mpCall->setTargetCallId(targetCallId.data());
				mpCall->setCallType(CpCall::CP_TRANSFEREE_ORIGINAL_CALL);

				int alsoIndex = 0;
				UtlString alsoUri;
				UtlString transferController;
				request->getFromField(&transferController);
				UtlString remoteAddress;
				getRemoteAddress(&remoteAddress);
				UtlString thisCallId;
				getCallId(&thisCallId);
				UtlString referredBy;
				request->getFromField(&referredBy);
				while(request->getAlsoUri(alsoIndex, &alsoUri))
				{
					//((CpPeerCall*)mpCall)->addParty(alsoUri.data(), 
					//    transferController.data(), NULL);
					alsoIndex++;

#   ifdef TEST_PRINT
					osPrintf("SipConnection::processRequest BYE Also URI: %s callid: %s\n",
						alsoUri.data(), targetCallId.data());
#   endif
					// Send a message to the target call to create the
					// connection and send the INVITE
					CpMultiStringMessage transfereeConnect(CallManager::CP_TRANSFEREE_CONNECTION,
						targetCallId.data(), alsoUri.data(), referredBy.data(), thisCallId.data(),
						remoteAddress.data(), TRUE /* Use BYE Also style INVITE*/ );

#   ifdef TEST_PRINT
					osPrintf("SipConnection::processRequest posting CP_TRANSFEREE_CONNECTION\n");
#   endif
					mpCallManager->postMessage(transfereeConnect);
				}

				// Send a trying response as it is likely to take a while
				//SipMessage sipTryingResponse;
				//sipTryingResponse.setTryingResponseData(request);
				//sipUserAgent->send(sipTryingResponse);
		}
#endif

			setState(CONNECTION_DISCONNECTED, CONNECTION_REMOTE);
      }

		// BYE is not legal in the current state
		else
		{
			// Build an error response
			SipMessage sipResponse;
			sipResponse.setByeErrorData(request);
			sipUserAgent->send(sipResponse);

			// Do not change the state

            // I do not recall the context of the above comment
            // May want to change to failed state in all cases
            if(getState() == CONNECTION_IDLE)
            {
                setState(CONNECTION_FAILED, CONNECTION_LOCAL);
            }
            else if(!inviteMsg) 
            {
               // If an invite was not sent or received something
               // is wrong.  This bye is invalid.
               setState(CONNECTION_FAILED, CONNECTION_LOCAL);
             }
		}
#ifdef TEST_PRINT
        OsSysLog::add(FAC_SIP, PRI_WARNING,
                    "Leaving SipConnection::processByeRequest inviteMsg=0x%08x ", (int)inviteMsg);
#endif
} // End of processByeRequest

void SipConnection::processCancelRequest(const SipMessage* request)
{
	int requestSequenceNum = 0;
	UtlString requestSeqMethod;

	request->getCSeqField(&requestSequenceNum, &requestSeqMethod);

		int calleeState = getState();

		// If it is acceptable to CANCLE the call
		if(lastRemoteSequenceNumber == requestSequenceNum && 
			calleeState != CONNECTION_IDLE &&
			calleeState != CONNECTION_DISCONNECTED &&
			calleeState !=  CONNECTION_FAILED &&
			calleeState != CONNECTION_ESTABLISHED)
		{
			

			// Build a 487 response
			if (!inviteFromThisSide)
			{
                SipMessage sipResponse;
				sipResponse.setRequestTerminatedResponseData(inviteMsg);
				sipUserAgent->send(sipResponse);
			}

			setState(CONNECTION_DISCONNECTED, CONNECTION_REMOTE, CONNECTION_CAUSE_CANCELLED);

			// Build an OK response
            SipMessage cancelResponse;
			cancelResponse.setOkResponseData(request);
			sipUserAgent->send(cancelResponse);
		}

		// CANCEL is not legal in the current state
		else
		{
			// Build an error response
			SipMessage sipResponse;
			sipResponse.setBadTransactionData(request);
			sipUserAgent->send(sipResponse);

			// Do not change the state

         // Do not know where the above comment came from
         // If there was no invite sent or received this is a bad call
         if(!inviteMsg) setState(CONNECTION_FAILED, CONNECTION_LOCAL, CONNECTION_CAUSE_CANCELLED);
		}
} // End of processCancelRequest

UtlBoolean SipConnection::getInitialSdpCodecs(const SipMessage* sdpMessage, 
                                        SdpCodecFactory& supportedCodecsArray,
                                        int& numCodecsInCommon,
                                        SdpCodec** &codecsInCommon,
                                        UtlString& remoteAddress,
                                        int& remotePort) const
{
    // Get the RTP info from the message if present
	// Should check the content type first
	const SdpBody* sdpBody = sdpMessage->getSdpBody();
	if(sdpBody)
	{
		osPrintf("SDP body in INVITE, finding best codec\n");
		sdpBody->getBestAudioCodecs(supportedCodecsArray,
                                    numCodecsInCommon,
                                    codecsInCommon,
                                    remoteAddress,
                                    remotePort);
	}
	else
	{
		osPrintf("No SDP in message\n");
	}

    return(sdpBody != NULL);
}

UtlBoolean SipConnection::processResponse(const SipMessage* response,
                                         UtlBoolean callInFocus, 
                                         UtlBoolean onHook)
{
	int sequenceNum;
	UtlString sequenceMethod;
	UtlBoolean processedOk = TRUE;
	UtlString responseText;
	UtlString contactUri;
    int previousState = getState();
	int responseCode = response->getResponseStatusCode();
	mResponseCode = responseCode;

	response->getResponseStatusText(&responseText);
	mResponseText = responseText;
	response->getCSeqField(&sequenceNum, &sequenceMethod);

	if(!inviteMsg)
	{
      // An invite was not sent or received.  This call is invalid.
      setState(CONNECTION_FAILED, CONNECTION_REMOTE);
	}

	else if(strcmp(sequenceMethod.data(), SIP_INVITE_METHOD) == 0)
	{
        processInviteResponse(response);
   		if (mTerminalConnState == PtTerminalConnection::HELD)
   		{
   			UtlString remoteAddress;
   			getRemoteAddress(&remoteAddress);
   			postTaoListenerMessage(PtEvent::TERMINAL_CONNECTION_HELD, PtEvent::CAUSE_NEW_CALL);
   		}

	} // End INVITE responses

    // REFER responses:
	else if(strcmp(sequenceMethod.data(), SIP_REFER_METHOD) == 0)
	{
        processReferResponse(response);
    }

	// ACK response ???
	else if(strcmp(sequenceMethod.data(), SIP_ACK_METHOD) == 0)
	{
		osPrintf("ACK response ignored: %d %s\n", responseCode, 
				responseText.data());
	}

    // Options response
    else if(strcmp(sequenceMethod.data(), SIP_OPTIONS_METHOD) == 0)
    {
        processOptionsResponse(response);
    }

    // NOTIFY response
    else if(strcmp(sequenceMethod.data(), SIP_NOTIFY_METHOD) == 0)
    {
        processNotifyResponse(response);
    }

	// else
	// BYE, CANCEL responses
	else if(strcmp(sequenceMethod.data(), SIP_BYE_METHOD) == 0 ||
            strcmp(sequenceMethod.data(), SIP_CANCEL_METHOD) == 0)
	{
        // We check the sequence number and method name of the
        // last sent request to make sure this is a response to
        // something that we actually sent
        if(lastLocalSequenceNumber == sequenceNum &&
           sequenceMethod.compareTo(mLastRequestMethod) == 0)
        {
		    osPrintf("%s response: %d %s\n", sequenceMethod.data(),
			    responseCode, responseText.data());
            if(responseCode >= SIP_OK_CODE)
            {
                mpMediaInterface->stopRtpSend(mConnectionId);
                // G729 is too expensive to leave going, stop receive
                // now as well.
                // We used to leave the receive going, to get cleaned up in
                // the delete of the connection so that the socket gets
                // drained.
                mpMediaInterface->stopRtpReceive(mConnectionId);
                //osPrintf("RTP receive not STOPPED\n");
            }

            // If this is the response to a BYE Also transfer
            if(getState() == CONNECTION_ESTABLISHED &&
                responseCode >= SIP_OK_CODE &&
                strcmp(sequenceMethod.data(), SIP_BYE_METHOD) == 0 &&
                !mTargetCallConnectionAddress.isNull() &&
                !isMethodAllowed(SIP_REFER_METHOD))
            {
                // We need to send notification to the target call
                // as to whether the transfer failed or succeeded
                int state;
                int cause;
                if(responseCode == SIP_OK_CODE)
                {
                    state = CONNECTION_ESTABLISHED;
                    //cause = CONNECTION_CAUSE_NORMAL;
                    //setState(CONNECTION_DISCONNECTED, CONNECTION_CAUSE_TRANSFER);
					cause = CONNECTION_CAUSE_TRANSFER;
                }
                else if(responseCode == SIP_BAD_EXTENSION_CODE ||
                    responseCode == SIP_UNIMPLEMENTED_METHOD_CODE)
                {
                    state = CONNECTION_FAILED;
                    cause = CONNECTION_CAUSE_INCOMPATIBLE_DESTINATION;
                }
                else if(responseCode == SIP_DECLINE_CODE)
                {
                    state = CONNECTION_FAILED;
                    cause = CONNECTION_CAUSE_CANCELLED;
                }
                else
                {
                    state = CONNECTION_FAILED;
                    cause = CONNECTION_CAUSE_BUSY;
                }

                setState(state, CONNECTION_REMOTE, cause);
               // Send the message to the target call
                UtlString targetCallId;
                UtlString toField;
                mToUrl.toString(toField);
                mpCall->getTargetCallId(targetCallId);
                CpMultiStringMessage transferControllerStatus(CallManager::CP_TRANSFER_CONNECTION_STATUS,
                    targetCallId.data(), toField.data(), 
                    NULL, NULL, NULL,
                    state, cause);
#ifdef TEST_PRINT
                osPrintf("SipConnection::processResponse BYE posting CP_TRANSFER_CONNECTION_STATUS to call: %s\n",
                    targetCallId.data());
#endif
                mpCallManager->postMessage(transferControllerStatus);

                // Reset mTargetCallConnectionAddress so that if this connection
                // is not disconnected due to transfer failure, it can
                // try another transfer or just disconnect.
                mTargetCallConnectionAddress = "";

            }

			//for BYE
            else if(responseCode >= SIP_OK_CODE && 
                lastLocalSequenceNumber == sequenceNum &&
				(strcmp(sequenceMethod.data(), SIP_BYE_METHOD) == 0))
            {
				   setState(CONNECTION_DISCONNECTED, CONNECTION_REMOTE);

				   // If we are in the middle of a transfer meta event
				   // on the target phone and target call it ends here
				   int metaEventId = 0;
				   int metaEventType = PtEvent::META_EVENT_NONE;
				   int numCalls = 0;
				   const UtlString* metaEventCallIds = NULL;
				   if(mpCall)
				   {
					   mpCall->getMetaEvent(metaEventId, metaEventType, numCalls, 
						   &metaEventCallIds);
					   if(metaEventId > 0 && metaEventType == PtEvent::META_CALL_TRANSFERRING)
						   mpCall->stopMetaEvent();
				   }
            }
			else if(responseCode >= SIP_OK_CODE && 
                lastLocalSequenceNumber == sequenceNum &&
				(strcmp(sequenceMethod.data(), SIP_CANCEL_METHOD) == 0) &&
				previousState != CONNECTION_ESTABLISHED &&
				previousState != CONNECTION_FAILED &&
				previousState != CONNECTION_DISCONNECTED &&
				previousState != CONNECTION_UNKNOWN &&
				previousState != CONNECTION_FAILED)
			{
				//start 32 second timer according to the bis03 draft
				UtlString callId;
				mpCall->getCallId(callId);
				UtlString remoteAddr;
				getRemoteAddress(&remoteAddr);
				CpMultiStringMessage* CancelTimerMessage = 
				new CpMultiStringMessage(CpCallManager::CP_CANCEL_TIMER, callId.data(), remoteAddr.data());
				OsQueuedEvent* queuedEvent = new OsQueuedEvent(*(mpCallManager->getMessageQueue()), (int)CancelTimerMessage);
				OsTimer* timer = new OsTimer(*queuedEvent);
				OsTime timerTime(32,0);
				timer->oneshotAfter(timerTime);

			}	

         else if(sequenceMethod.compareTo(SIP_BYE_METHOD) == 0)
         {
             processByeResponse(response);
         }
         else if(sequenceMethod.compareTo(SIP_CANCEL_METHOD) == 0)
         {
             processCancelResponse(response);
         }
			// else Ignore provisional responses
			else
			{
				osPrintf("%s provisional response ignored: %d %s\n", 
					sequenceMethod.data(), 
					responseCode, 
					responseText.data());
			}
		}
		else
		{
			osPrintf("%s response ignored: %d %s invalid cseq last: %d last method: %s\n", 
				sequenceMethod.data(), 
				responseCode, 
				responseText.data(),
				lastLocalSequenceNumber,
				mLastRequestMethod.data());
		}
	}//END - else if(strcmp(sequenceMethod.data(), SIP_BYE_METHOD) == 0 || strcmp(sequenceMethod.data(), SIP_CANCEL_METHOD) == 0)
	
	// Unknown method response
    else
    {
        osPrintf("%s response ignored: %d %s\n", 
				sequenceMethod.data(), 
				responseCode, 
				responseText.data());
    }

	return(processedOk);
}  // End of processResponse

void SipConnection::processInviteResponse(const SipMessage* response)
{
	int sequenceNum;
	UtlString sequenceMethod;
    int responseCode = response->getResponseStatusCode();
    int previousState = getState();

    	response->getCSeqField(&sequenceNum, &sequenceMethod);

        if(lastLocalSequenceNumber == sequenceNum &&
            inviteMsg)
        {
            UtlString toAddr;
            UtlString toProto;
            int toPort;
            UtlString inviteTag;

            // Check to see if there is a tag set in the To field that
            // should be remembered for future messages.
            inviteMsg->getToAddress(&toAddr, &toPort, &toProto,
                NULL, NULL, &inviteTag);

            // Do not save the tag unless it is a final response
            // This is the stupid/simple thing to do until we need
            // more elaborate tracking of forked/serial branches
            if(inviteTag.isNull() && responseCode >= SIP_OK_CODE)
            {
                response->getToAddress(&toAddr, &toPort, &toProto,
                    NULL, NULL, &inviteTag);

#ifdef TEST_PRINT
                osPrintf("SipConnection::processInviteResponse no invite tag, got response tag: \"%s\"\n",
                    inviteTag.data());
#endif

                if(!inviteTag.isNull())
                {
                    inviteMsg->setToFieldTag(inviteTag.data());

                    // Update the cased to field after saving the tag
                    inviteMsg->getToUrl(mToUrl);
                }
            }            
        }

#ifdef TEST_PRINT
       osPrintf("SipConnection::processInviteResponse responseCode %d\n\t reinviteState %d\n\t sequenceNum %d\n\t lastLocalSequenceNumber%d\n",
           responseCode,
           reinviteState,
           sequenceNum,
           lastLocalSequenceNumber);
#endif
		if((responseCode == SIP_RINGING_CODE ||
            responseCode == SIP_EARLY_MEDIA_CODE) &&
			lastLocalSequenceNumber == sequenceNum &&
			reinviteState == ACCEPT_INVITE)
		{
			UtlBoolean isEarlyMedia = TRUE;
			osPrintf("received Ringing response\n");				
			if( responseCode == SIP_RINGING_CODE && !mIsEarlyMediaFor180)
			{
				isEarlyMedia = FALSE;
			}
            // If there is SDP we have early media or remote ringback
            int cause = CONNECTION_CAUSE_NORMAL;
			if(response->getSdpBody() && isEarlyMedia) 
            {
                cause = CONNECTION_CAUSE_UNKNOWN;

                // If this is the initial INVITE
                if(reinviteState == ACCEPT_INVITE)
                {
                    // Setup the sending of audio
                    UtlString rtpAddress;
                    int receiveRtpPort;
                    SdpCodecFactory supportedCodecs;
                    mpMediaInterface->getCapabilities(mConnectionId, 
                                                      rtpAddress, 
                                                      receiveRtpPort,
                                                      supportedCodecs);
			        // Setup the media channel
			        // The address should be retrieved from the sdpBody
                    int numMatchingCodecs = 0;
                    SdpCodec** matchingCodecs = NULL;
			        getInitialSdpCodecs(response, supportedCodecs,
                        numMatchingCodecs, matchingCodecs,                
                        remoteRtpAddress, remoteRtpPort);

                    if(numMatchingCodecs > 0)
                    {
                        // Set up the remote RTP sockets if we have a legitimate
                        // address to send RTP
                        if(!remoteRtpAddress.isNull() &&
                            remoteRtpAddress.compareTo("0.0.0.0") != 0)
                        {
                            mpMediaInterface->setConnectionDestination(mConnectionId,
                            remoteRtpAddress.data(), remoteRtpPort);
                        }

                        if(!remoteRtpAddress.isNull() &&
                            remoteRtpAddress.compareTo("0.0.0.0") != 0 &&
                            remoteRtpPort > 0 && 
                            mTerminalConnState != PtTerminalConnection::HELD)
				        {
					        mpMediaInterface->startRtpSend(mConnectionId,
                                numMatchingCodecs, 
                                matchingCodecs);
				        }

                    } // End if there are matching codecs

                    // Free up the codec copies and array
                    for(int codecIndex = 0; codecIndex < numMatchingCodecs; codecIndex++)
                    {
                        delete matchingCodecs[codecIndex];
                        matchingCodecs[codecIndex] = NULL;
                    }
                    if(matchingCodecs) delete[] matchingCodecs;
                    matchingCodecs = NULL;

                } // End if this is an initial INVITE
            } // End if this is an early media, provisional response

            setState(CONNECTION_ALERTING, CONNECTION_REMOTE, cause);	
		}

		// Start busy tone the other end is busy
		else if(responseCode == SIP_BUSY_CODE &&
			lastLocalSequenceNumber == sequenceNum &&
			reinviteState == ACCEPT_INVITE)
		{
			osPrintf("received INVITE Busy\n");
			setState(CONNECTION_FAILED, CONNECTION_REMOTE, CONNECTION_CAUSE_BUSY);
			//processedOk = false;

			// ACK gets sent by the SipUserAgent for error responses
			//SipMessage sipRequest;
			//sipRequest.setAckData(response,inviteMsg);
			//sipUserAgent->send(sipRequest);
		}

        // Call queued
		else if(responseCode == SIP_QUEUED_CODE &&
			lastLocalSequenceNumber == sequenceNum &&
			reinviteState == ACCEPT_INVITE)
		{
			osPrintf("received INVITE queued\n");
			setState(CONNECTION_QUEUED, CONNECTION_REMOTE);

            // Should there be a queued tone?

            // Do not send an ACK as this is informational
        }

		// Failed invite
   		else if(responseCode >= SIP_BAD_REQUEST_CODE &&
 			lastLocalSequenceNumber == sequenceNum)
   		{
 			if (reinviteState == ACCEPT_INVITE) 
                    
 			{
 				UtlString responseText;
 				response->getResponseStatusText(&responseText);
 				osPrintf("INVITE failed with response: %d %s\n",
 					responseCode, responseText.data());
 
            int cause = CONNECTION_CAUSE_UNKNOWN;
 				int warningCode;
 
 				switch(responseCode)
 				{
 				case HTTP_UNAUTHORIZED_CODE:
 					cause = CONNECTION_CAUSE_NOT_ALLOWED;
 					break;
   
 				case HTTP_PROXY_UNAUTHORIZED_CODE:
 					cause = CONNECTION_CAUSE_NETWORK_NOT_ALLOWED;
 					break;
   
            case SIP_REQUEST_TIMEOUT_CODE:
               cause = CONNECTION_CAUSE_CANCELLED;
               break;

 				case SIP_BAD_REQUEST_CODE:
 					response->getWarningCode(&warningCode);
 					if(warningCode == SIP_WARN_MEDIA_NAVAIL_CODE)
 					{
 						// incompatable media
 						cause = CONNECTION_CAUSE_INCOMPATIBLE_DESTINATION;
 					}
 					break;
   
 				default:
   
 					if(responseCode < SIP_SERVER_INTERNAL_ERROR_CODE) 
 						cause = CONNECTION_CAUSE_INCOMPATIBLE_DESTINATION;
 					else if(responseCode >= SIP_SERVER_INTERNAL_ERROR_CODE &&
 						responseCode < SIP_GLOBAL_BUSY_CODE) 
 						cause = CONNECTION_CAUSE_NETWORK_NOT_OBTAINABLE;
 					if(responseCode >= SIP_GLOBAL_BUSY_CODE) 
 						cause = CONNECTION_CAUSE_NETWORK_CONGESTION;
 					else cause = CONNECTION_CAUSE_UNKNOWN;
 					break;
   				}

				//in case the response is "487 call terminated" 
				if(responseCode == SIP_REQUEST_TERMINATED_CODE)
            {
               cause = CONNECTION_CAUSE_CANCELLED;
     				setState(CONNECTION_DISCONNECTED, CONNECTION_REMOTE, cause);
            }
            else
 					setState(CONNECTION_FAILED, CONNECTION_REMOTE, cause);

				mbCancelling = FALSE;	// if was cancelling, now cancelled.
   
 				// ACK gets sent by the SipUserAgent for error responses
 				//SipMessage sipRequest;
 				//sipRequest.setAckData(response,inviteMsg);
 				//sipUserAgent->send(sipRequest);
   
 				//processedOk = false;
 			}

 			else if (reinviteState == REINVITING)
 			{
 				// ACK gets sent by the SipUserAgent for error responses
 				//SipMessage sipRequest;
 				//sipRequest.setAckData(response,inviteMsg);
 				//sipUserAgent->send(sipRequest);
   
 				reinviteState = ACCEPT_INVITE;
 				//processedOk = false; 					

                // Temp Fix: If we failed to renegotiate a invite, failed the 
                // connection so that the upper layers can react.  We *SHOULD*
                // fire off a new event to application layer indicating that the
                // reinvite failed -- or make hold/unhold blocking. (Bob 8/14/02)    
                postTaoListenerMessage(CONNECTION_FAILED, CONNECTION_CAUSE_INCOMPATIBLE_DESTINATION, false);
 			}
   		}
   
           // INVITE OK too late
		// The other end picked up, but this end already hungup
		else if(responseCode == SIP_OK_CODE &&
			lastLocalSequenceNumber == sequenceNum &&
            (getState() == CONNECTION_DISCONNECTED || mbCancelling))
		{
         
			mbCancelling = FALSE;	// if was cancelling, now cancelled.
            // Send an ACK
            SipMessage sipAckRequest;
			sipAckRequest.setAckData(response,inviteMsg);
			sipUserAgent->send(sipAckRequest);

            
            // Always get the remote contact as it may can change over time
            UtlString contactInResponse;
			if (response->getContactUri(0 , &contactInResponse))
			{
				mRemoteContact.remove(0);
				mRemoteContact.append(contactInResponse);
			}
                
            // Get the route for subsequent requests
            response->buildRouteField(&mRouteField);
            osPrintf("okINVITE set mRouteField: %s\n", mRouteField.data());

            // Send a BYE
         SipMessage sipByeRequest;
         lastLocalSequenceNumber++;
 		
 		sipByeRequest.setByeData(inviteMsg,
 			mRemoteContact ,
 			TRUE, 
 			lastLocalSequenceNumber,
             mRouteField.data(), 
 			NULL);
            mLastRequestMethod = SIP_BYE_METHOD;
			sipUserAgent->send(sipByeRequest);
            osPrintf("glare: received OK to hungup call, sending ACK & BYE\n");
   
        }

		// INVITE OK
		// The other end picked up
		else if(responseCode == SIP_OK_CODE &&
			    lastLocalSequenceNumber == sequenceNum)

		{
			osPrintf("received INVITE OK\n");
			// connect the call

			//save the contact field in the response to send further reinvites
			UtlString contactInResponse;
			if (response->getContactUri(0 , &contactInResponse))
			{
				mRemoteContact.remove(0);
				mRemoteContact.append(contactInResponse);
			}

            // Get the route for subsequent requests only if this is 
            // the initial transaction
            if(previousState != CONNECTION_ESTABLISHED)
            {
                response->buildRouteField(&mRouteField);
                osPrintf("okINVITE set mRouteField: %s\n", mRouteField.data());
            }

            // Get the session timer if set
            mSessionReinviteTimer = 0;
            response->getSessionExpires(&mSessionReinviteTimer);

            // Set a timer to reINVITE to keep the session up
            if(mSessionReinviteTimer > mDefaultSessionReinviteTimer &&
                mDefaultSessionReinviteTimer != 0) // if default < 0 disallow
            {
                mSessionReinviteTimer = mDefaultSessionReinviteTimer;
            }

            int timerSeconds = mSessionReinviteTimer;
            if(mSessionReinviteTimer > 2) 
            {
                timerSeconds = mSessionReinviteTimer / 2; //safety factor
            }

            // Construct an ACK
            SipMessage sipRequest;
			sipRequest.setAckData(response, inviteMsg, NULL,
                mSessionReinviteTimer);
            // Set the route field
            if(!mRouteField.isNull())
            {
                sipRequest.setRouteField(mRouteField.data());
                osPrintf("Adding route to ACK: %s\n", mRouteField.data());
            }
            else
            {
                osPrintf("No route to add to ACK :%s\n", mRouteField.data());
            }

            // Start the session reINVITE timer
            if(mSessionReinviteTimer > 0)
            {
                SipMessageEvent* sipMsgEvent = 
                    new SipMessageEvent(new SipMessage(sipRequest), 
                            SipMessageEvent::SESSION_REINVITE_TIMER);
                OsQueuedEvent* queuedEvent = 
                    new OsQueuedEvent(*(mpCallManager->getMessageQueue()), 
				        (int)sipMsgEvent);
			    OsTimer* timer = new OsTimer(*queuedEvent);
			    // Convert from mSeconds to uSeconds
			    OsTime timerTime(timerSeconds, 0);
			    timer->oneshotAfter(timerTime);
#ifdef TEST_PRINT
                osPrintf("SipConnection::processInviteResponse timer message type: %d %d", 
                    OsMsg::PHONE_APP, SipMessageEvent::SESSION_REINVITE_TIMER);
#endif
            }

			// Send the ACK message
			sipUserAgent->send(sipRequest);
 			if (mFarEndHoldState == TERMCONNECTION_HOLDING)
 			{
  				setTerminalConnectionState(PtTerminalConnection::HELD, 1);
 				mFarEndHoldState = TERMCONNECTION_HELD;
				mpMediaInterface->stopRtpSend(mConnectionId);

                // The prerequisit hold was completed we
                // can now do the next action/transaction
                switch(mHoldCompleteAction)
                {
                    case CpCallManager::CP_BLIND_TRANSFER:
                        // This hold was performed as a precurser to
                        // A blind transfer.
                        mHoldCompleteAction = CpCallManager::CP_UNSPECIFIED;
                        doBlindRefer();
                        break;

                    default:
                        // Bogus action, reset it
                        mHoldCompleteAction = CpCallManager::CP_UNSPECIFIED;
                        break;
                }
                
 			}
 			else if (mFarEndHoldState == TERMCONNECTION_TALKING)
 			{
  				setTerminalConnectionState(PtTerminalConnection::TALKING, 1);
 				mFarEndHoldState = TERMCONNECTION_NONE;
 			}

#ifdef TEST_PRINT
            osPrintf("200 OK reinviteState: %d\n", reinviteState);
#endif

            UtlString rtpAddress;
            int receiveRtpPort;
            SdpCodecFactory supportedCodecs;
            mpMediaInterface->getCapabilities(mConnectionId, 
                                              rtpAddress, 
                                              receiveRtpPort,
                                              supportedCodecs);
			// Setup the media channel
			// The address should be retrieved from the sdpBody
            int numMatchingCodecs = 0;
            SdpCodec** matchingCodecs = NULL;
			getInitialSdpCodecs(response, supportedCodecs,
                numMatchingCodecs, matchingCodecs,                
                remoteRtpAddress, remoteRtpPort);

			if(numMatchingCodecs > 0)
            {
                // Set up the remote RTP sockets if we have a legitimate
                // address to send RTP
                if(!remoteRtpAddress.isNull() &&
                    remoteRtpAddress.compareTo("0.0.0.0") != 0)
                {
                    mpMediaInterface->setConnectionDestination(mConnectionId,
                    remoteRtpAddress.data(), remoteRtpPort);
                }

				if(reinviteState == ACCEPT_INVITE)
				{
					setState(CONNECTION_ESTABLISHED, CONNECTION_REMOTE);
				}

				//osPrintf("RTP SENDING address: %s port: %d \nRTP LISTENING port: %d\n",
				//			remoteRtpAddress.data(), remoteRtpPort, localRtpPort);

                // No RTP address, stop sending media
                if(remoteRtpAddress.isNull() ||
                    remoteRtpAddress.compareTo("0.0.0.0") == 0)
                {
                    mpMediaInterface->stopRtpSend(mConnectionId);
                }

 				else if(remoteRtpPort > 0 && mTerminalConnState != PtTerminalConnection::HELD)
				{
					mpMediaInterface->startRtpReceive(mConnectionId, 
                        numMatchingCodecs, 
                        matchingCodecs);
					mpMediaInterface->startRtpSend(mConnectionId,
                        numMatchingCodecs, 
                        matchingCodecs);
				}
				else if(mTerminalConnState == PtTerminalConnection::HELD)
				{
                    mpMediaInterface->stopRtpSend(mConnectionId);
                    mpMediaInterface->stopRtpReceive(mConnectionId);
				}
                

			}

			// Original INVITE response with no SDP 
			// cannot setup call
			else if(reinviteState == ACCEPT_INVITE)
			{
				osPrintf("No SDP in INVITE OK response\n");
				setState(CONNECTION_FAILED, CONNECTION_REMOTE, CONNECTION_CAUSE_INCOMPATIBLE_DESTINATION);

				// Send a BYE
                //UtlString directoryServerUri;
				SipMessage sipByeRequest;
                //if(!inviteFromThisSide)
                //{
                //    UtlString dirAddress;
                //    UtlString dirProtocol;
                //    int dirPort;

                //    sipUserAgent->getDirectoryServer(0, &dirAddress,
				//		&dirPort, &dirProtocol);
                //    SipMessage::buildSipUrl(&directoryServerUri, 
                //        dirAddress.data(), dirPort, dirProtocol.data());
                //}
                lastLocalSequenceNumber++;
                osPrintf("no SDP BYE route: %s\n", mRouteField.data());
                sipByeRequest.setByeData(inviteMsg, 
                    mRemoteContact,
                    inviteFromThisSide,
                    lastLocalSequenceNumber, 
                    //directoryServerUri.data(),
                    mRouteField.data(), 
                    NULL );// no alsoUri    
                    mLastRequestMethod = SIP_BYE_METHOD;
				    sipUserAgent->send(sipByeRequest);
				
			}

			// Ignore reINVITE OK responses without SDP
			else
			{
			}

            // Allow ReINVITEs from the other side again
            if(reinviteState == REINVITING)
            {
                reinviteState = ACCEPT_INVITE;
            }

            // If we do not already know what the other side
            // support (i.e. methods)
            if(mAllowedRemote.isNull())
            {
                // Get the methods the other side supports
                response->getAllowField(mAllowedRemote);

             // If the other side did not set the allowed field
             // send an OPTIONS request to see what it supports
             if(mAllowedRemote.isNull())
               {
                 lastLocalSequenceNumber++;
                 SipMessage optionsRequest;
                 optionsRequest.setOptionsData(inviteMsg,
                     mRemoteContact, 
                     inviteFromThisSide,
                     lastLocalSequenceNumber, 
                     mRouteField.data());

                 sipUserAgent->send(optionsRequest);
               }
         }

            // Free up the codec copies and array
            for(int codecIndex = 0; codecIndex < numMatchingCodecs; codecIndex++)
            {
                delete matchingCodecs[codecIndex];
                matchingCodecs[codecIndex] = NULL;
            }
            if(matchingCodecs) delete[] matchingCodecs;
            matchingCodecs = NULL;
		}


		// Redirect
		else if(responseCode >= SIP_MULTI_CHOICE_CODE &&
			responseCode < SIP_BAD_REQUEST_CODE &&
			lastLocalSequenceNumber == sequenceNum)
		{
			osPrintf("Redirect received\n");

			// ACK gets sent by the SipUserAgent for error responses
			//SipMessage sipRequest;
			//sipRequest.setAckData(response, inviteMsg);
			//sipUserAgent->send(sipRequest);

			// If the call has not already failed
			if(getState() != CONNECTION_FAILED)
			{
				// Get the first contact uri
                UtlString contactUri;
				response->getContactUri(0, &contactUri);
				osPrintf("Redirecting to: %s\n", contactUri.data());

				if(!contactUri.isNull() && inviteMsg)
				{
					// Create a new INVITE
					lastLocalSequenceNumber++;
					SipMessage sipRequest(*inviteMsg);
					sipRequest.changeUri(contactUri.data());

                    // Don't use the contact in the to field for redirect
                    // Use the same To field, but clear the tag
                    mToUrl.removeFieldParameter("tag");
                    UtlString toField;
                    mToUrl.toString(toField);
                    sipRequest.setRawToField(toField);
                    //sipRequest.setRawToField(contactUri.data());

                    // Set incremented Cseq
					sipRequest.setCSeqField(lastLocalSequenceNumber, 
						SIP_INVITE_METHOD);

                    // Decrement the max-forwards header
                    int maxForwards;
                    if(!sipRequest.getMaxForwards(maxForwards))
                    {
                        maxForwards = SIP_DEFAULT_MAX_FORWARDS;
                    }
                    maxForwards--;
                    sipRequest.setMaxForwards(maxForwards);

					//reomove all routes
					UtlString route;
					while ( sipRequest.removeRouteUri(0,&route)){}

					//now it is first send
					sipRequest.clearDNSField();
					sipRequest.resetTransport();

					// Get rid of the original invite and save a copy of 
					// the new one
					if(inviteMsg) delete inviteMsg;
					inviteMsg = NULL;
					inviteMsg = new SipMessage(sipRequest);
                    inviteFromThisSide = TRUE;

                    // As the To field is not modified this is superfluous
                    //setCallerId();

					// Send the invite
					if(sipUserAgent->send(sipRequest))
					{
						// Change the state back to Offering
						setState(CONNECTION_OFFERING, CONNECTION_REMOTE, CONNECTION_CAUSE_REDIRECTED);
					}
					else
					{
                        UtlString redirected;
                        int len;
                        sipRequest.getBytes(&redirected, &len);
                        osPrintf("==== Redirected failed ===>%s\n====>\n",
                            redirected.data());
						// The send failed
						setState(CONNECTION_FAILED, CONNECTION_REMOTE, CONNECTION_CAUSE_NETWORK_NOT_OBTAINABLE);
					}
				}

				// don't know how we got here, this is bad
				else
				{
					setState(CONNECTION_FAILED, CONNECTION_REMOTE);
				}
			}
		}

		else
		{
            UtlString responseText;
            response->getResponseStatusText(&responseText);
			osPrintf("Ignoring INVITE response: %d %s\n", responseCode, 
				responseText.data());

			if(responseCode >= SIP_OK_CODE && 
				(lastLocalSequenceNumber == sequenceNum || mIsReferSent))
			{
                // I do not understand what cases go through this else
                // so this is just a error message for now.
                if(responseCode < SIP_3XX_CLASS_CODE)
                {
                    osPrintf("ERROR: SipConnection::processInviteResponse sending ACK for failed INVITE\n");
                }

				// Send an ACK
				SipMessage sipRequest;
				sipRequest.setAckData(response,inviteMsg);
				sipUserAgent->send(sipRequest);
				if(reinviteState == REINVITED)
				{
					osPrintf("ReINVITE ACK - ReINVITE allowed again\n");
					reinviteState = ACCEPT_INVITE;
				}
			}
		}

        // If we have completed a REFER based transfer
        // Send the status back to the original call & connection
        // as to whether it succeeded or not.
#ifdef TEST_PRINT
        UtlString connState;
        getStateString(getState(), &connState);
        osPrintf("SipConnection::processResponse originalConnectionAddress: %s connection state: %s\n",
            mOriginalCallConnectionAddress.data(), connState.data());
        getStateString(previousState, &connState);
        osPrintf("SipConnection::processResponse originalConnectionAddress: %s previous state: %s\n",
            mOriginalCallConnectionAddress.data(), connState.data());
#endif
        int currentState = getState();
        if(previousState != currentState &&
            !mOriginalCallConnectionAddress.isNull())
        {
            if(currentState == CONNECTION_ESTABLISHED ||
                currentState == CONNECTION_FAILED ||
                currentState == CONNECTION_ALERTING)
            {
                UtlString originalCallId;
                mpCall->getOriginalCallId(originalCallId);
                CpMultiStringMessage transfereeStatus(CallManager::CP_TRANSFEREE_CONNECTION_STATUS,
                    originalCallId.data(), 
                    mOriginalCallConnectionAddress.data(),
                    NULL, NULL, NULL,
                    currentState, responseCode);
#ifdef TEST_PRINT
                osPrintf("SipConnection::processResponse posting CP_TRANSFEREE_CONNECTION_STATUS to call: %s\n",
                    originalCallId.data());
#endif
                mpCallManager->postMessage(transfereeStatus);

                // This is the end of the transfer meta event
                // whether successful or not
                //mpCall->stopMetaEvent();

            }
        }
} // End of processInviteResponse

void SipConnection::processReferResponse(const SipMessage* response)
{
        int state = CONNECTION_UNKNOWN;
        int cause = CONNECTION_CAUSE_UNKNOWN;
        int responseCode = response->getResponseStatusCode();

        // 2xx class responses are no-ops as it only indicates
        // the transferee is attempting to INVITE the transfer target
        if(responseCode == SIP_OK_CODE)
        {
            state = CONNECTION_DIALING;
            cause = CONNECTION_CAUSE_NORMAL;
        }
        else if(responseCode == SIP_ACCEPTED_CODE)
        {
            state = CONNECTION_OFFERING;
            cause = CONNECTION_CAUSE_NORMAL;
        }
        else if(responseCode == SIP_DECLINE_CODE)
        {
            state = CONNECTION_FAILED;
            cause = CONNECTION_CAUSE_CANCELLED;
        }
        else if(responseCode == SIP_BAD_METHOD_CODE ||
            responseCode == SIP_UNIMPLEMENTED_METHOD_CODE)
        {
            state = CONNECTION_FAILED;
            cause = CONNECTION_CAUSE_INCOMPATIBLE_DESTINATION;
        }
        else if(responseCode >= SIP_MULTI_CHOICE_CODE)
        {
            state = CONNECTION_FAILED;
            cause = CONNECTION_CAUSE_BUSY;
        }

        // REFER is not supported, try BYE Also
        if(responseCode == SIP_BAD_METHOD_CODE  ||
            responseCode == SIP_UNIMPLEMENTED_METHOD_CODE)
        {
            UtlString thisAddress;
            response->getFromField(&thisAddress);
            doHangUp(mTargetCallConnectionAddress.data(), thisAddress.data());

            // If we do not know what the other side supports
            // set it to BYE only so that we do not try REFER again
            if(mAllowedRemote.isNull()) 
				mAllowedRemote = SIP_BYE_METHOD;
			else
			{
				int pos = mAllowedRemote.index("REFER", 0, UtlString::ignoreCase);

				if (pos >= 0)
				{
					mAllowedRemote.remove(pos, 6);
				}
			}
        }

        // We change state on the target/consultative call
        else if(responseCode >= SIP_OK_CODE)
        {
            // Signal the connection in the target call with the final status
            UtlString targetCallId;
            UtlString toField;
            mToUrl.toString(toField);
            mpCall->getTargetCallId(targetCallId);
            CpMultiStringMessage transferControllerStatus(CallManager::CP_TRANSFER_CONNECTION_STATUS,
                targetCallId.data(), toField.data(), 
                NULL, NULL, NULL,
                state, cause);
    #ifdef TEST_PRINT
            osPrintf("SipConnection::processResponse REFER posting CP_TRANSFER_CONNECTION_STATUS to call: %s\n",
                targetCallId.data());
    #endif
            mpCallManager->postMessage(transferControllerStatus);

            // Drop this connection, the transfer succeeded
            // Do the drop at the last possible momment so that
            // both calls have some overlap.
            if(responseCode == SIP_OK_CODE) doHangUp();
        }

#ifdef TEST_PRINT
        // We ignore provisional response
        // as they do not indicate any state change
        else
        {
            osPrintf("SipConnection::processResponse ignoring REFER response %d\n",
                responseCode);
        }
#endif
} // End of processReferResponse

void SipConnection::processNotifyResponse(const SipMessage* response)
{
#ifdef TEST_PRINT
    int responseCode = response->getResponseStatusCode();
    osPrintf("SipConnection::processNotifyResponse NOTIFY response: %d ignored\n",
        responseCode);
#else
    response->getResponseStatusCode();
#endif
}

void SipConnection::processOptionsResponse(const SipMessage* response)
{
    int responseCode = response->getResponseStatusCode();
    UtlString responseText;
    int sequenceNum;
    UtlString sequenceMethod;

	response->getResponseStatusText(&responseText);
	response->getCSeqField(&sequenceNum, &sequenceMethod);

    if(responseCode == SIP_OK_CODE && 
        lastLocalSequenceNumber == sequenceNum)
        response->getAllowField(mAllowedRemote);

    // It seems the other side does not support OPTIONS
    else if(responseCode > SIP_OK_CODE &&
        lastLocalSequenceNumber == sequenceNum)
    {
        response->getAllowField(mAllowedRemote);
        
        // Assume default minimum
        if(mAllowedRemote.isNull())
            mAllowedRemote = "INVITE, BYE, ACK, CANCEL, REFER";
    }
    else
        osPrintf("%s response ignored: %d %s invalid cseq last: %d\n", 
			sequenceMethod.data(), 
            responseCode, 
			responseText.data(),
            lastLocalSequenceNumber);
} // End of processOptionsResponse

void SipConnection::processByeResponse(const SipMessage* response)
{
    // Set a timer so that if we get a 100 and never get a
    // final response, we still tear down the connection
    int responseCode = response->getResponseStatusCode();
    if(responseCode == SIP_TRYING_CODE)
    {
        UtlString localAddress;
        UtlString remoteAddress;
        UtlString callId;
        getFromField(&localAddress);
        getToField(&remoteAddress);
        getCallId(&callId);

        CpMultiStringMessage* expiredBye = 
            new CpMultiStringMessage(CallManager::CP_FORCE_DROP_CONNECTION,
            callId.data(), remoteAddress.data(), localAddress.data());
        OsQueuedEvent* queuedEvent = 
            new OsQueuedEvent(*(mpCallManager->getMessageQueue()), 
				(int)expiredBye);
		OsTimer* timer = new OsTimer(*queuedEvent);
		// Convert from mSeconds to uSeconds
        osPrintf("Setting BYE timeout to %d seconds\n",
            sipUserAgent->getSipStateTransactionTimeout() / 1000);
		OsTime timerTime(sipUserAgent->getSipStateTransactionTimeout() / 1000, 0);
		timer->oneshotAfter(timerTime);
    }

    else if(responseCode >= SIP_2XX_CLASS_CODE)
    {
        // Stop sending & receiving RTP
        mpMediaInterface->stopRtpSend(mConnectionId);
        mpMediaInterface->stopRtpReceive(mConnectionId);
    }
} // End of processByeResponse

void SipConnection::processCancelResponse(const SipMessage* response)
{
    // Set a timer so that if we get a 100 and never get a
    // final response, we still tear down the connection
    int responseCode = response->getResponseStatusCode();
    if(responseCode == SIP_TRYING_CODE)
    {
        UtlString localAddress;
        UtlString remoteAddress;
        UtlString callId;
        getFromField(&localAddress);
        getToField(&remoteAddress);
        getCallId(&callId);

        CpMultiStringMessage* expiredBye = 
            new CpMultiStringMessage(CallManager::CP_FORCE_DROP_CONNECTION,
            callId.data(), remoteAddress.data(), localAddress.data());
        OsQueuedEvent* queuedEvent = 
            new OsQueuedEvent(*(mpCallManager->getMessageQueue()), 
				(int)expiredBye);
		OsTimer* timer = new OsTimer(*queuedEvent);
		// Convert from mSeconds to uSeconds
        osPrintf("Setting CANCEL timeout to %d seconds\n",
            sipUserAgent->getSipStateTransactionTimeout() / 1000);
		OsTime timerTime(sipUserAgent->getSipStateTransactionTimeout() / 1000, 0);
		timer->oneshotAfter(timerTime);
    }
    else if(responseCode >= SIP_2XX_CLASS_CODE)
    {
        // Stop sending & receiving RTP
        mpMediaInterface->stopRtpSend(mConnectionId);
        mpMediaInterface->stopRtpReceive(mConnectionId);
    }

} // End of processCancelResponse



void SipConnection::setCallerId()
{
   UtlString newCallerId;
   
   if(inviteMsg)
   {
      UtlString user;
      UtlString addr;
      //UtlString fromProtocol;
      Url uri;
      UtlString userLabel;
      //int port;
      if(!inviteFromThisSide)
      {
         inviteMsg->getFromUrl(mToUrl);
         uri = mToUrl;
         inviteMsg->getToUrl(mFromUrl);
         inviteMsg->getRequestUri(&mRemoteUriStr);

#ifdef TEST_PRINT
         UtlString fromString;
         UtlString toString;
         mToUrl.toString(toString);
         mFromUrl.toString(fromString);
         osPrintf("SipConnection::setCallerId INBOUND to: %s from: %s\n",
             toString.data(), fromString.data());
#endif
      }
      else
      {
         inviteMsg->getToUrl(mToUrl);
         uri = mToUrl;
         inviteMsg->getFromUrl(mFromUrl);
         inviteMsg->getRequestUri(&mLocalUriStr);

#ifdef TEST_PRINT
         UtlString fromString;
         UtlString toString;
         mToUrl.toString(toString);
         mFromUrl.toString(fromString);
         osPrintf("SipConnection::setCallerId INBOUND to: %s from: %s\n",
             toString.data(), fromString.data());
#endif
      }

      uri.getHostAddress(addr);
      //port = uri.getHostPort();
      //uri.getUrlParameter("transport", fromProtocol);
      uri.getUserId(user);
      uri.getDisplayName(userLabel);
      // Set the caller ID
      // Use the first that is not empty string of:
      // user label
      // user id
      // host address
      NameValueTokenizer::frontBackTrim(&userLabel, " \t\n\r");
#ifdef TEST_PRINT
      osPrintf("SipConnection::setCallerid label: %s user %s address: %s\n",
          userLabel.data(), user.data(), addr.data());
#endif

      if(!userLabel.isNull())
      {
          newCallerId.append(userLabel.data());
      }
      else
      {
          NameValueTokenizer::frontBackTrim(&user, " \t\n\r");
          if(!user.isNull())
          {
              newCallerId.append(user.data());
          }
          else
          {
              NameValueTokenizer::frontBackTrim(&addr, " \t\n\r");
                  newCallerId.append(addr.data());
          }
      }
   }
   Connection::setCallerId(newCallerId.data());
}

UtlBoolean SipConnection::processNewFinalMessage(SipUserAgent* sipUa,
                                             OsMsg* eventMessage)
{
	UtlBoolean sendSucceeded = FALSE;

	int msgType = eventMessage->getMsgType();
	int msgSubType = eventMessage->getMsgSubType();
	const SipMessage* sipMsg = NULL;

	if(msgType == OsMsg::PHONE_APP &&
      msgSubType == CallManager::CP_SIP_MESSAGE)
	{
      sipMsg = ((SipMessageEvent*)eventMessage)->getMessage();
      int port;
      int sequenceNum;
      UtlString method;
      UtlString address;
      UtlString protocol;
      UtlString user;
      UtlString userLabel;
      UtlString tag;
      UtlString sequenceMethod;
      sipMsg->getToAddress(&address, &port, &protocol, &user, &userLabel, &tag);
      sipMsg->getCSeqField(&sequenceNum, &method);

      int responseCode = sipMsg->getResponseStatusCode();

      // INVITE to create a connection
      //if to tag is already set then return 481 error
      if(method.compareTo(SIP_INVITE_METHOD) == 0 && 
         !tag.isNull() && 
         responseCode == SIP_OK_CODE)
      {
	      UtlString fromField;
	      UtlString toField;
	      UtlString uri;
	      UtlString callId;

	      sipMsg->getFromField(&fromField);
	      sipMsg->getToField(&toField);
         sipMsg->getContactUri( 0 , &uri);
		   if(uri.isNull())
		      uri.append(toField.data());

         sipMsg->getCallIdField(&callId);
         SipMessage* ackMessage = new SipMessage();
         ackMessage->setAckData(uri,
							           fromField,
                                toField,
                                callId,
                                sequenceNum);
         sendSucceeded = sipUa->send(*ackMessage);
         delete ackMessage;

         if (sendSucceeded)
         {
            SipMessage* byeMessage = new SipMessage();
            byeMessage->setByeData(uri,
							              fromField,
                                   toField,
                                   callId,
                                   sequenceNum + 1);
            sendSucceeded = sipUa->send(*byeMessage);
            delete byeMessage;
         }
      }

   }
   return sendSucceeded;
}


/* ============================ ACCESSORS ================================= */

UtlBoolean SipConnection::getRemoteAddress(UtlString* remoteAddress) const
{
    return(getRemoteAddress(remoteAddress, FALSE));
}

UtlBoolean SipConnection::getRemoteAddress(UtlString* remoteAddress,
                                          UtlBoolean leaveFieldParmetersIn) const
{
    // leaveFieldParmetersIn gives the flexability of getting the 
    // tag when the connection is still an early dialog

    int remoteState = getState();
    // If this is an early dialog or we explicily want the 
    // field parameters
    if(leaveFieldParmetersIn || 
       remoteState == CONNECTION_ESTABLISHED ||
       remoteState == CONNECTION_DISCONNECTED ||
       remoteState == CONNECTION_FAILED ||
       remoteState == CONNECTION_UNKNOWN)
    {
        // Cast as the toString method is not const
        ((Url)mToUrl).toString(*remoteAddress);
    }

    else
    {
        Url toNoFieldParameters(mToUrl);
        toNoFieldParameters.removeFieldParameters();
        toNoFieldParameters.toString(*remoteAddress);
    }

#ifdef TEST_PRINT
    osPrintf("SipConnection::getRemoteAddress address: %s\n",
        remoteAddress->data());
#endif

    return(inviteMsg != NULL);
}

UtlBoolean SipConnection::isSameRemoteAddress(Url& remoteAddress) const
{
    return(isSameRemoteAddress(remoteAddress, TRUE));
}

UtlBoolean SipConnection::isSameRemoteAddress(Url& remoteAddress,
                                             UtlBoolean tagsMustMatch) const
{
    UtlBoolean isSame = FALSE;

    int remoteState = getState();
    // If this is an early dialog or we explicily want the 
    // field parameters
    Url mToUrlTmp(mToUrl);

    if(tagsMustMatch || 
       remoteState == CONNECTION_ESTABLISHED ||
       remoteState == CONNECTION_DISCONNECTED ||
       remoteState == CONNECTION_FAILED ||
       remoteState == CONNECTION_UNKNOWN)
    {
        isSame = SipMessage::isSameSession(mToUrlTmp, remoteAddress);
    }
    else
    {
        // The do not requrie a tag in the remote address
        isSame = SipMessage::isSameSession(remoteAddress, mToUrlTmp);
    }

    return(isSame);
}

UtlBoolean SipConnection::getSession(SipSession& session)
{
    UtlString callId;
    getCallId(&callId);
    SipSession ssn;
    ssn.setCallId(callId.data());
    ssn.setLastFromCseq(lastLocalSequenceNumber);
    ssn.setLastToCseq(lastRemoteSequenceNumber);
    ssn.setFromUrl(mFromUrl);
    ssn.setToUrl(mToUrl);
    UtlString localContact;
    buildLocalContact(localContact);
    ssn.setLocalContact(Url(localContact, TRUE));

    if (!mRemoteUriStr.isNull())
      ssn.setRemoteRequestUri(mRemoteUriStr);
    if (!mLocalUriStr.isNull())
      ssn.setLocalRequestUri(mLocalUriStr);

    session = ssn;
    return(TRUE);
}

int SipConnection::getNextCseq()
{
    lastLocalSequenceNumber++;
    return(lastLocalSequenceNumber);
}

OsStatus SipConnection::getFromField(UtlString* fromField)
{
	OsStatus ret = OS_SUCCESS;

    UtlString host;
    mFromUrl.getHostAddress(host);
	if(host.isNull())
  		ret = OS_NOT_FOUND;

    mFromUrl.toString(*fromField);

#ifdef TEST_PRINT
    osPrintf("SipConnection::getFromAddress address: %s\n",
        fromField->data());
#endif

	return ret;
}

OsStatus SipConnection::getToField(UtlString* toField)
{
	OsStatus ret = OS_SUCCESS;
    UtlString host;
    mToUrl.getHostAddress(host);
	if (host.isNull())
  		ret = OS_NOT_FOUND;

    mToUrl.toString(*toField);

#ifdef TEST_PRINT
    osPrintf("SipConnection::getToAddress address: %s\n",
        toField->data());
#endif

	return ret;
}
  

/* ============================ INQUIRY =================================== */

UtlBoolean SipConnection::willHandleMessage(OsMsg& eventMessage) const
{
    int msgType = eventMessage.getMsgType();
	int msgSubType = eventMessage.getMsgSubType();
	UtlBoolean handleMessage = FALSE;
	const SipMessage* sipMsg = NULL;
	int messageType;

    // Do not handle message if marked for deletion
    if (isMarkedForDeletion())
        return false ;

	if(msgType == OsMsg::PHONE_APP &&
		msgSubType == CallManager::CP_SIP_MESSAGE)
	{
		sipMsg = ((SipMessageEvent&)eventMessage).getMessage();
		messageType = ((SipMessageEvent&)eventMessage).getMessageStatus();

        // If the callId, To and From match it belongs to this message
        if(inviteMsg && inviteMsg->isSameSession(sipMsg))
        {
            handleMessage = TRUE;
        }
        else if(inviteMsg)
        {
            // Trick to reverse the To & From fields
            SipMessage toFromReversed;
             toFromReversed.setByeData(inviteMsg,
                 mRemoteContact,
                 FALSE, 1, "", NULL);
            if(toFromReversed.isSameSession(sipMsg))
            {
                handleMessage = TRUE;
            }
        }
    
    }

    return(handleMessage);
}

UtlBoolean SipConnection::isConnection(const char* callId,
                                      const char* toTag,
                                      const char* fromTag,
                                      UtlBoolean  strictCompare) const
{
    UtlBoolean matches = FALSE;

    // Do not handle message if marked for deletion
    if (isMarkedForDeletion())
        return false ;

    if(inviteMsg)
    {
        UtlString thisCallId;
        inviteMsg->getCallIdField(&thisCallId);

        if(thisCallId.compareTo(callId) == 0)
        {
            UtlString thisFromTag;
            UtlString thisToTag;
            mFromUrl.getFieldParameter("tag", thisFromTag);
            mToUrl.getFieldParameter("tag", thisToTag);

            if (strictCompare)
            {
               // for transfer target in a consultative call, 
               // thisFromTag is remote, thisToTag is local
               if((thisFromTag.compareTo(toTag) == 0 &&
                   thisToTag.compareTo(fromTag) == 0 ))
               {
                   matches = TRUE;
               }
            }

            // Do a sloppy comparison
            //  Allow a match either way
            else
            {
               if((thisFromTag.compareTo(fromTag) == 0 &&
                   thisToTag.compareTo(toTag) == 0 ) ||
                   (thisFromTag.compareTo(toTag) == 0 &&
                   thisToTag.compareTo(fromTag) == 0 ))
               {
                   matches = TRUE;
               }
            }
 #ifdef TEST_PRINT
            osPrintf("SipConnection::isConnection toTag=%s\n\t fromTag=%s\n\t thisToTag=%s\n\t thisFromTag=%s\n\t matches=%d\n",
                    toTag, fromTag, thisToTag.data(), thisFromTag.data(), (int)matches) ;
#endif
       }
    }

    return(matches);
}

// Determine if the other side of this connection (remote side)
// supports the given method
UtlBoolean SipConnection::isMethodAllowed(const char* method)
{
    // Eventually we may want to send an OPTIONS request if
    // we do not know.  For now assume that the other side
    // sent an Allowed header field in the final response.
    // If we do not know (mAllowedRemote is NULL) assume
    // it is supported.
    UtlBoolean methodSupported = TRUE;
    int methodIndex = mAllowedRemote.index(method);
    if(methodIndex >=0)
    {
        methodSupported = TRUE;
    }

    // We explicitly know that it does not support this method
    else if(!mAllowedRemote.isNull())
    {
        methodSupported = FALSE;
    }
#ifdef TEST_PRINT
    osPrintf("SipConnection::isMethodAllowed method: %s allowed: %s return: %d index: %d null?: %d\n",
        method, mAllowedRemote.data(), methodSupported, methodIndex,
        mAllowedRemote.isNull());
#endif

    return(methodSupported);
}


/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

void SipConnection::proceedToRinging(const SipMessage* inviteMessage,
                                     SipUserAgent* sipUserAgent, int tagNum,
                                     //UtlBoolean callInFocus,
                                     int availableBehavior,
                                     CpMediaInterface* mediaInterface)
{
   UtlString name = mpCall->getName();
   osPrintf("%s SipConnection::proceedToRinging\n", name.data());
    // Send back a ringing INVITE response
	SipMessage sipResponse;
	sipResponse.setInviteRingingData(inviteMessage);
    if(tagNum >= 0)
    {
        sipResponse.setToFieldTag(tagNum);
    }
	if(sipUserAgent->send(sipResponse))
	{
		osPrintf("INVITE Ringing sent successfully\n");
	}
	else
	{
		osPrintf("INVITE Ringing send failed\n");
	}

    // If not pretending to ring or busy
    //if(!callInFocus || availableBehavior != RING_SILENT)
    //{
		//mediaInterface->startRinger(TRUE, FALSE);
    //}
}

/* ============================ FUNCTIONS ================================= */


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
#include <stdlib.h>
#include <string.h>
#ifdef _VXWORKS
#include <resparse/vxw/hd_string.h>
#endif

//uncomment next line to track the create and destroy of messages
//#define TRACK_LIFE

// APPLICATION INCLUDES
#include <utl/UtlDListIterator.h>
#include <utl/UtlHashBagIterator.h>
#include <net/SipMessage.h>
#include <net/MimeBodyPart.h>
#include <net/NameValueTokenizer.h>
#include <net/Url.h>
#include <os/OsDateTime.h>
#include <os/OsSysLog.h>


// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
#define MAXIMUM_INTEGER_STRING_LENGTH 20

// STATIC VARIABLE INITIALIZATIONS

UtlHashBag SipMessage::shortFieldNames;
UtlHashBag SipMessage::longFieldNames;
UtlHashBag SipMessage::disallowedUrlHeaders;

#ifdef WIN32
#  define strcasecmp stricmp
#  define strncasecmp strnicmp
#endif

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
SipMessage::SipMessage(const char* messageBytes, int byteCount) :
   HttpMessage(messageBytes, byteCount)
{
   mpSipTransaction = NULL;
   replaceShortFieldNames();
#ifdef TRACK_LIFE
   osPrintf("Created SipMessage @ address:%X\n",this);
#endif
}

SipMessage::SipMessage(OsSocket* inSocket, int bufferSize) :
   HttpMessage(inSocket, bufferSize)
{
#ifdef TRACK_LIFE
   osPrintf("Created SipMessage @ address:%X\n",this);
#endif
    mpSipTransaction = NULL;
   replaceShortFieldNames();
}


// Copy constructor
SipMessage::SipMessage(const SipMessage& rSipMessage) :
   HttpMessage(rSipMessage)
{
#ifdef TRACK_LIFE
   osPrintf("Created SipMessage @ address:%X\n",this);
#endif
   replaceShortFieldNames();
   //SDUA
   m_dnsProtocol = rSipMessage.m_dnsProtocol;
   m_dnsAddress = rSipMessage.m_dnsAddress;
   m_dnsPort = rSipMessage.m_dnsPort;
    mpSipTransaction = rSipMessage.mpSipTransaction;
}

// Destructor
SipMessage::~SipMessage()
{
#ifdef TRACK_LIFE
   osPrintf("Deleted SipMessage from address :%X\n",this);
#endif
}

// Assignment operator
SipMessage&
SipMessage::operator=(const SipMessage& rSipMessage)
{
   HttpMessage::operator =((HttpMessage&)rSipMessage);
   if (this != &rSipMessage)
   {
         replaceShortFieldNames();
      //SDUA
      m_dnsProtocol = rSipMessage.m_dnsProtocol;
      m_dnsAddress = rSipMessage.m_dnsAddress;
      m_dnsPort = rSipMessage.m_dnsPort;
       mpSipTransaction = rSipMessage.mpSipTransaction;
   }
   return *this;
}


void SipMessage::initShortNames()
{
   if(longFieldNames.isEmpty())
   {
      SipMessage::longFieldNames.insert(new NameValuePair(SIP_CONTENT_TYPE_FIELD, SIP_SHORT_CONTENT_TYPE_FIELD));
      SipMessage::longFieldNames.insert(new NameValuePair(SIP_CONTENT_ENCODING_FIELD, SIP_SHORT_CONTENT_ENCODING_FIELD));
      SipMessage::longFieldNames.insert(new NameValuePair(SIP_FROM_FIELD, SIP_SHORT_FROM_FIELD));
      SipMessage::longFieldNames.insert(new NameValuePair(SIP_CALLID_FIELD, SIP_SHORT_CALLID_FIELD));
      SipMessage::longFieldNames.insert(new NameValuePair(SIP_CONTACT_FIELD, SIP_SHORT_CONTACT_FIELD));
      SipMessage::longFieldNames.insert(new NameValuePair(SIP_CONTENT_LENGTH_FIELD, SIP_SHORT_CONTENT_LENGTH_FIELD));
      SipMessage::longFieldNames.insert(new NameValuePair(SIP_REFERRED_BY_FIELD, SIP_SHORT_REFERRED_BY_FIELD));
      SipMessage::longFieldNames.insert(new NameValuePair(SIP_REFER_TO_FIELD, SIP_SHORT_REFER_TO_FIELD));
      SipMessage::longFieldNames.insert(new NameValuePair(SIP_SUBJECT_FIELD, SIP_SHORT_SUBJECT_FIELD));
        SipMessage::longFieldNames.insert(new NameValuePair(SIP_SUPPORTED_FIELD, SIP_SHORT_SUPPORTED_FIELD));
      SipMessage::longFieldNames.insert(new NameValuePair(SIP_TO_FIELD, SIP_SHORT_TO_FIELD));
      SipMessage::longFieldNames.insert(new NameValuePair(SIP_VIA_FIELD, SIP_SHORT_VIA_FIELD));
      SipMessage::longFieldNames.insert(new NameValuePair(SIP_EVENT_FIELD, SIP_SHORT_EVENT_FIELD));

      UtlHashBagIterator iterator(longFieldNames);
      NameValuePair* nvPair;
      while ((nvPair = (NameValuePair*) iterator()))
      {
         shortFieldNames.insert(new NameValuePair(nvPair->getValue(),
            nvPair->data()));
      }
   }
}

void SipMessage::initDisallowedUrlHeaders()
{
    // These headers may NOT be passed through in a URL to
    // be set in a message
    SipMessage::disallowedUrlHeaders.insert(new UtlString(SIP_CONTACT_FIELD));
    SipMessage::disallowedUrlHeaders.insert(new UtlString(SIP_SHORT_CONTACT_FIELD));
    SipMessage::disallowedUrlHeaders.insert(new UtlString(SIP_CONTENT_LENGTH_FIELD));
    SipMessage::disallowedUrlHeaders.insert(new UtlString(SIP_SHORT_CONTENT_LENGTH_FIELD));
    SipMessage::disallowedUrlHeaders.insert(new UtlString(SIP_CONTENT_TYPE_FIELD));
    SipMessage::disallowedUrlHeaders.insert(new UtlString(SIP_SHORT_CONTENT_TYPE_FIELD));
    SipMessage::disallowedUrlHeaders.insert(new UtlString(SIP_CONTENT_ENCODING_FIELD));
    SipMessage::disallowedUrlHeaders.insert(new UtlString(SIP_SHORT_CONTENT_ENCODING_FIELD));
    SipMessage::disallowedUrlHeaders.insert(new UtlString(SIP_CSEQ_FIELD));
    SipMessage::disallowedUrlHeaders.insert(new UtlString(SIP_REFER_TO_FIELD));
    SipMessage::disallowedUrlHeaders.insert(new UtlString(SIP_REFERRED_BY_FIELD));
    SipMessage::disallowedUrlHeaders.insert(new UtlString(SIP_TO_FIELD));
    SipMessage::disallowedUrlHeaders.insert(new UtlString(SIP_SHORT_TO_FIELD));
    SipMessage::disallowedUrlHeaders.insert(new UtlString(SIP_USER_AGENT_FIELD));
    SipMessage::disallowedUrlHeaders.insert(new UtlString(SIP_VIA_FIELD));
    SipMessage::disallowedUrlHeaders.insert(new UtlString(SIP_SHORT_VIA_FIELD));
}

/* ============================ MANIPULATORS ============================== */

UtlBoolean SipMessage::getShortName(const char* longFieldName,
                       UtlString* shortFieldName)
{
   NameValuePair longNV(longFieldName);
   UtlBoolean nameFound = FALSE;
   initShortNames();

   shortFieldName->remove(0);
   NameValuePair* shortNV = (NameValuePair*) longFieldNames.find(&longNV);
   if(shortNV)
   {
      shortFieldName->append(shortNV->getValue());
      nameFound = TRUE;
   }

   return(nameFound);
}

UtlBoolean SipMessage::getLongName(const char* shortFieldName,
                      UtlString* longFieldName)
{
   UtlBoolean nameFound = FALSE;

    // Short names are currently only 1 character long
    // If the short name is exactly 1 character long
    if(shortFieldName && shortFieldName[0] &&
        shortFieldName[1] == '\0')
    {
        UtlString shortNV(shortFieldName);

       initShortNames();

       NameValuePair* longNV = (NameValuePair*) shortFieldNames.find(&shortNV);
       if(longNV)
       {
          *longFieldName = longNV->getValue();
          nameFound = TRUE;
       }
        // Optimization in favor of utility
        //else
        //{
        //    longFieldName->remove(0);
        //}
    }
   return(nameFound);
}

void SipMessage::replaceShortFieldNames()
{
   UtlDListIterator iterator(mNameValues);
   NameValuePair* nvPair;
   UtlString longName;

   while ((nvPair = (NameValuePair*) iterator()))
   {
      if(getLongName(nvPair->data(), &longName))
      {
            mHeaderCacheClean = FALSE;
         nvPair->remove(0);
         nvPair->append(longName.data());
      }
   }
}

/* ============================ ACCESSORS ================================= */
void SipMessage::setSipRequestFirstHeaderLine(const char* method,
                                   const char* uri,
                                   const char* protocolVersion)
{
   //fix for bug : 1667 - 12/18/2001
   Url tempRequestUri(uri, TRUE);
   UtlString strRequestUri;
   tempRequestUri.removeUrlParameter("method");
   tempRequestUri.removeAngleBrackets();
   tempRequestUri.getUri(strRequestUri);

   setRequestFirstHeaderLine(method, strRequestUri.data(), protocolVersion);
}

void SipMessage::setRegisterData(const char* registererUri,
                        const char* registerAsUri,
                        const char* registrarServerUri,
                        const char* takeCallsAtUri,
                        const char* callId,
                        int sequenceNumber,
                        int expiresInSeconds)
{
   setRequestData(SIP_REGISTER_METHOD,
                     registrarServerUri, // uri
                     registererUri, // from
                     registerAsUri, // to
                     callId,
                     sequenceNumber);

   setContactField(takeCallsAtUri);
   setExpiresField(expiresInSeconds);
}

void SipMessage::setReinviteData(SipMessage* invite,
                         const char* farEndContact,
                                 const char* contactUrl,
                                 UtlBoolean inviteFromThisSide,
                                 const char* routeField,
                                 const char* rtpAddress, int rtpPort,
                          int sequenceNumber,
                          int numRtpCodecs,
                          SdpCodec* rtpCodecs[],
                                 int sessionReinviteTimer)
{
    UtlString toField;
    UtlString fromField;
    UtlString callId;
   UtlString contactUri;
   UtlString lastResponseContact;


    // Get the to, from and callId fields
    if(inviteFromThisSide)
    {
      invite->getToField(&toField);
      invite->getFromField(&fromField);
    }
    else// Reverse the to from, this invite came from the other side
    {
      invite->getToField(&fromField);
      invite->getFromField(&toField);
    }

   invite->getCallIdField(&callId);

   if (farEndContact)
      lastResponseContact.append(farEndContact);

   if ( !inviteFromThisSide && lastResponseContact.isNull())
   {
      //if invite from other side and LastResponseContact is null because there has not been any
      //final responses from the other side yet ...check the otherside's invite request and get the
      //contact field from the request
       invite->getContactUri(0, &lastResponseContact);
    }

   setInviteData(fromField,
                  toField,
              lastResponseContact,
                  contactUrl,
                  callId,
                  rtpAddress, rtpPort,
                  sequenceNumber,
              numRtpCodecs,
              rtpCodecs,
                  sessionReinviteTimer);

    // Set the route field if present
    if(routeField && routeField[0])
    {
        setRouteField(routeField);
    }

}

void SipMessage::setInviteData(const char* fromField,
                        const char* toField,
                        const char* farEndContact,
                               const char* contactUrl,
                        const char* callId,
                        const char* rtpAddress, int rtpPort,
                        int sequenceNumber,
                        int numRtpCodecs,
                        SdpCodec* rtpCodecs[],
                               int sessionReinviteTimer)
{
   UtlString bodyString;
   UtlString uri;

    Url toUrl(toField);
   // Create the top header line

   // If we have a contact for the other side use it
    // for the URI, otherwise use the To field
   if (farEndContact && *farEndContact)
   {
      uri = farEndContact;
   }
   else
    {
        // Clean out the header field parameters if they exist
        Url uriUrl(toUrl); // copy constructor is more efficient than parsing the toField string
        //uri.append(uriUrl.toString());
        uriUrl.removeHeaderParameters();
        uriUrl.getUri(uri);
    }

    // Check for header fields in the To URL
    UtlString headerName;
    UtlString headerValue;
    int headerIndex = 0;
    // Look through the headers and add them to the message
    while(toUrl.getHeaderParameter(headerIndex, headerName, headerValue))
    {
        // If the header is allowed to be passed through
        if(isUrlHeaderAllowed(headerName.data()))
        {
            addHeaderField(headerName.data(), headerValue.data());
#ifdef TEST_PRINT
            osPrintf("SipMessage::setInviteData: name=%s, value=%s\n",
                    headerName.data(), headerValue.data());
#endif
        }
        else
        {
            osPrintf("WARNING: SipMessage::setInviteData URL header disallowed: %s: %s\n",
                headerName.data(), headerValue.data());
        }

        headerIndex++;
    }

    // Remove the header fields from the URL as them
    // have been added to the message
    toUrl.removeHeaderParameters();
    UtlString toFieldString;
    toUrl.toString(toFieldString);

   setRequestData(SIP_INVITE_METHOD,
        uri, // URI
        fromField,
        toFieldString.data(),
        callId,
        sequenceNumber,
        contactUrl);

    // Set the session timer in seconds
    if(sessionReinviteTimer > 0)
        setSessionExpires(sessionReinviteTimer);

#ifdef TEST
   osPrintf("SipMessage::setInviteData rtpAddress: %s\n", rtpAddress);
#endif
   addSdpBody(rtpAddress, rtpPort, numRtpCodecs,
            rtpCodecs);
}

void SipMessage::addSdpBody(const char* rtpAddress, int rtpPort,
                     int numRtpCodecs, SdpCodec* rtpCodecs[])
{
   if(numRtpCodecs > 0)
   {
      UtlString bodyString;
      int len;

      // Create and add the SDP body
      SdpBody* sdpBody = new SdpBody();
      sdpBody->setStandardHeaderFields("phone-call",
                            NULL, //"dpetrie@pingtel.com",
                            NULL,//"+1 781 938 5306",
                                     rtpAddress); // Originator address
      sdpBody->addAudioCodecs(rtpAddress, rtpPort, numRtpCodecs,
                        rtpCodecs);

      setBody(sdpBody);

      // Add the content type for the body
      setContentType(SDP_CONTENT_TYPE);

      // Add the content length
      sdpBody->getBytes(&bodyString, &len);
      setContentLength(len);
   }
}

const SdpBody* SipMessage::getSdpBody() const
{
   const SdpBody* body = NULL;
   UtlString contentType;
   UtlString sdpType(SDP_CONTENT_TYPE);

   getContentType(&contentType);

   // Make them both lower case so they compare
   contentType.toLower();
   sdpType.toLower();

   // If the body is of SDP type, return it
   if(contentType.compareTo(sdpType) == 0)
   {
      body = (const SdpBody*) getBody();
   }

    // Else if this is a multipart MIME body see
    // if there is an SDP part
    else
    {
        const HttpBody* multipartBody = getBody();
        if(multipartBody  && multipartBody->isMultipart())
        {
            int partIndex = 0;
            const HttpBody* bodyPart = NULL;
            while ((bodyPart = multipartBody->getMultipart(partIndex)))
            {
                if(strcmp(bodyPart->getContentType(), SDP_CONTENT_TYPE) == 0)
                {
                    // Temporarily disable while fixing multipart bodies
                    body = (const SdpBody*) bodyPart;
                    break;
                }
                partIndex++ ;
            }
        }
    }

   return(body);
}

void SipMessage::setRequestData(const char* method, const char* uri,
                     const char* fromField, const char* toField,
                     const char* callId,
                     int sequenceNumber,
                            const char* contactUrl)
{
   // Create the top header line
   setSipRequestFirstHeaderLine(method, uri, SIP_PROTOCOL_VERSION);

   // Add the From field
   setRawFromField(fromField);

   // Add the to field
   setRawToField(toField);

   // Add the call-id field
   setCallIdField(callId);

   // Add the CSeq field
   setCSeqField(sequenceNumber, method);

    if(contactUrl && *contactUrl)
    {
        setContactField(contactUrl);
    }
}

void SipMessage::setResponseData(int statusCode, const char* statusText,
                     const char* fromField, const char* toField,
                     const char* callId,
                     int sequenceNumber, const char* sequenceMethod)
{
   // Create the top header line
   setResponseFirstHeaderLine(SIP_PROTOCOL_VERSION, statusCode,
      statusText);

   // Add the From field
   setRawFromField(fromField);

   // Add the to field
   setRawToField(toField);

   // Add the call-id field
   setCallIdField(callId);

   // Add the CSeq field
   if(sequenceNumber >= 0)
   {
      setCSeqField(sequenceNumber, sequenceMethod);
   }
}

void SipMessage::setTryingResponseData(const SipMessage* request)
{
   setResponseData(request, SIP_TRYING_CODE, SIP_TRYING_TEXT);
}

void SipMessage::setInviteRingingData(const SipMessage* inviteRequest)
{
   setResponseData(inviteRequest, SIP_RINGING_CODE, SIP_RINGING_TEXT);
}

void SipMessage::setQueuedResponseData(const SipMessage* inviteRequest)
{
   setResponseData(inviteRequest, SIP_QUEUED_CODE, SIP_QUEUED_TEXT);
}

void SipMessage::setInviteBusyData(const char* fromField, const char* toField,
               const char* callId,
               int sequenceNumber)
{
      setResponseData(SIP_BUSY_CODE, SIP_BUSY_TEXT,
                     fromField, toField,
                     callId, sequenceNumber, SIP_INVITE_METHOD);
}

void SipMessage::setBadTransactionData(const SipMessage* inviteRequest)
{
   setResponseData(inviteRequest, SIP_BAD_TRANSACTION_CODE,
        SIP_BAD_TRANSACTION_TEXT);
}

void SipMessage::setLoopDetectedData(const SipMessage* inviteRequest)
{
   setResponseData(inviteRequest, SIP_LOOP_DETECTED_CODE,
        SIP_LOOP_DETECTED_TEXT);
}

void SipMessage::setInviteBusyData(const SipMessage* inviteRequest)
{
   UtlString fromField;
   UtlString toField;
   UtlString callId;
   int sequenceNum;
   UtlString sequenceMethod;

   inviteRequest->getFromField(&fromField);
   inviteRequest->getToField(&toField);
   inviteRequest->getCallIdField(&callId);
   inviteRequest->getCSeqField(&sequenceNum, &sequenceMethod);

   setInviteBusyData(fromField.data(), toField.data(),
      callId.data(), sequenceNum);

   setViaFromRequest(inviteRequest);
}

void SipMessage::setForwardResponseData(const SipMessage* request,
                     const char* forwardAddress)
{
   setResponseData(request, SIP_TEMPORARY_MOVE_CODE, SIP_TEMPORARY_MOVE_TEXT);

   // Add the contact field for the forward address
   UtlString contactAddress;
   //UtlString address;
   //UtlString protocol;
   //UtlString user;
   //UtlString userLabel;
   //int port;
   //parseAddressFromUri(forwardAddress, &address, &port,
   // &protocol, &user, &userLabel);
   //buildSipUrl(&contactAddress, address.data(), port,
   // protocol.data(), user.data(), userLabel.data());
    Url contactUrl(forwardAddress);
    contactUrl.removeFieldParameters();
    contactUrl.toString(contactAddress);
   setContactField(contactAddress.data());
}

void SipMessage::setInviteBadCodecs(const SipMessage* inviteRequest)
{
   char warningCodeString[MAXIMUM_INTEGER_STRING_LENGTH + 1];
   UtlString warningField;

   //setInviteBusyData(fromField.data(), toField.data(),
   // callId.data(), sequenceNum);
   setResponseData(inviteRequest, SIP_BAD_REQUEST_CODE,
        SIP_BAD_REQUEST_TEXT);

   // Add a media not available warning
    // The text message must be a quoted string
   sprintf(warningCodeString, "%d \"", SIP_WARN_MEDIA_NAVAIL_CODE);
   warningField.append(warningCodeString);
   warningField.append(SIP_WARN_MEDIA_NAVAIL_TEXT);
    warningField.append("\"");
   addHeaderField(SIP_WARNING_FIELD, warningField.data());
}

void SipMessage::setRequestBadMethod(const SipMessage* request,
                            const char* allowedMethods)
{
   setResponseData(request, SIP_BAD_METHOD_CODE, SIP_BAD_METHOD_TEXT);

   // Add a methods supported field
   addHeaderField(SIP_ALLOW_FIELD, allowedMethods);
}

void SipMessage::setRequestUnimplemented(const SipMessage* request)
{
   setResponseData(request, SIP_UNIMPLEMENTED_METHOD_CODE,
        SIP_UNIMPLEMENTED_METHOD_TEXT);
}

void SipMessage::setRequestBadExtension(const SipMessage* request,
                            const char* disallowedExtension)
{
   setResponseData(request, SIP_BAD_EXTENSION_CODE, SIP_BAD_EXTENSION_TEXT);

   // Add a methods supported field
   addHeaderField(SIP_UNSUPPORTED_FIELD, disallowedExtension);
}

void SipMessage::setRequestBadContentEncoding(const SipMessage* request,
                   const char* allowedEncodings)
{
   setResponseData(request, SIP_BAD_MEDIA_CODE, SIP_BAD_MEDIA_TEXT);

   // Add a encodings supported field
   addHeaderField(SIP_ACCEPT_ENCODING_FIELD, allowedEncodings);
   addHeaderField(SIP_ACCEPT_FIELD, "application/sdp");

   const char* explanation = "Content Encoding value not supported";
   setBody(new HttpBody(explanation, strlen(explanation), CONTENT_TYPE_TEXT_PLAIN));
}

void SipMessage::setRequestBadAddress(const SipMessage* request)
{
   setResponseData(request, SIP_BAD_ADDRESS_CODE, SIP_BAD_ADDRESS_TEXT);
}

void SipMessage::setRequestBadVersion(const SipMessage* request)
{
   setResponseData(request, SIP_BAD_VERSION_CODE, SIP_BAD_VERSION_TEXT);
}

void SipMessage::setRequestBadRequest(const SipMessage* request)
{

   setResponseData(request, SIP_BAD_REQUEST_CODE, SIP_BAD_REQUEST_TEXT);
}

void SipMessage::setRequestBadUrlType(const SipMessage* request)
{

   setResponseData(request, SIP_REQUEST_NOT_ACCEPTED_HERE, SIP_REQUEST_NOT_ACCEPTED_URL_TYPE_TEXT);
}

void SipMessage::setInviteOkData(const char* fromField, const char* toField,
               const char* callId, const SdpBody* inviteSdp,
               const char* rtpAddress, int rtpPort,
               int numRtpCodecs,
               SdpCodec* rtpCodecs[], int sequenceNumber)
{
   SdpBody* sdpBody;
   UtlString bodyString;
   int len;

   setResponseData(SIP_OK_CODE, SIP_OK_TEXT,
                     fromField, toField,
                     callId, sequenceNumber, SIP_INVITE_METHOD);

   // Create and add the SDP body
   sdpBody = new SdpBody();
   sdpBody->setStandardHeaderFields("phone-call",
                            NULL, //"dpetrie@pingtel.com",
                            NULL, //"+1 781 938 5306",
                                     rtpAddress); //originator address

   // If the INVITE SDP is present pick the best
   if(inviteSdp)
   {
            sdpBody->addAudioCodecs(rtpAddress, rtpPort, numRtpCodecs,
                        rtpCodecs, inviteSdp);
   }

   else
   {
      sdpBody->addAudioCodecs(rtpAddress, rtpPort, numRtpCodecs,
                        rtpCodecs);
   }
   setBody(sdpBody);

   // Add the content type
   setContentType(SDP_CONTENT_TYPE);

   // Add the content length
   sdpBody->getBytes(&bodyString, &len);
   setContentLength(len);

}

void SipMessage::setInviteOkData(const SipMessage* inviteRequest,
                const char* rtpAddress, int rtpPort,
                int numRtpCodecs, SdpCodec* rtpCodecs[],
                     int maxSessionExpiresSeconds)
{
   UtlString fromField;
   UtlString toField;
   UtlString callId;
   int sequenceNum;
   UtlString sequenceMethod;
   const SdpBody* inviteSdp = NULL;

   inviteRequest->getFromField(&fromField);
   inviteRequest->getToField(&toField);
   inviteRequest->getCallIdField(&callId);
   inviteRequest->getCSeqField(&sequenceNum, &sequenceMethod);
   inviteSdp = inviteRequest->getSdpBody();

   setInviteOkData(fromField.data(), toField.data(), callId,
               inviteSdp, rtpAddress, rtpPort,
               numRtpCodecs, rtpCodecs,
               sequenceNum);

   setViaFromRequest(inviteRequest);

    UtlString recordRouteField;
    int recordRouteIndex = 0;
    while(inviteRequest->getRecordRouteField(recordRouteIndex,
            &recordRouteField))
    {
        // Don't do this as it will result in the UA at the other
        // end routing to itself.
        // Add the contact (if present) to the end of the record-route
        //UtlString contactUri;
        //if(inviteRequest->getContactUri(0, &contactUri))
        //{
        //    recordRouteField.insert(0, ',');
        //    recordRouteField.insert(0, contactUri.data());
        //}

        setRecordRouteField(recordRouteField.data(), recordRouteIndex);
        recordRouteIndex++;
    }

    int inviteSessionExpires;
    // If max session timer is less than the requested timer length
    // or if the other side did not request a timer, use
    // the max session timer
    if(!inviteRequest->getSessionExpires(&inviteSessionExpires) ||
        (maxSessionExpiresSeconds > 0 &&
        inviteSessionExpires > maxSessionExpiresSeconds))
    {
        inviteSessionExpires = maxSessionExpiresSeconds;
    }
    if(inviteSessionExpires > 0)
    {
        setSessionExpires(inviteSessionExpires);
    }
}

void SipMessage::setOkResponseData(const SipMessage* request)
{
   setResponseData(request, SIP_OK_CODE, SIP_OK_TEXT);
}

void SipMessage::setNotifyData(SipMessage *subscribeRequest,
                               int localCSequenceNumber,
                               const char* route,
                               const char* eventField)
{
   UtlString fromField;
   UtlString toField;
   UtlString uri;
   UtlString callId;
   int dummySequenceNum;
   UtlString sequenceMethod;

   subscribeRequest->getFromField(&fromField);
   subscribeRequest->getToField(&toField);
   subscribeRequest->getCallIdField(&callId);
   subscribeRequest->getCSeqField(&dummySequenceNum, &sequenceMethod);

    // Set the NOTIFY event type
    if(eventField && *eventField)
    {
        setEventField(eventField);
    }
    else
    {
        // Try to get it from the subscribe message
        UtlString subscribeEventField;
        subscribeRequest->getEventField(subscribeEventField);
        if(!subscribeEventField.isNull())
        {
            setEventField(subscribeEventField.data());
        }
    }

    // Set the route for the NOTIFY request
    if(route && *route)
    {
        setRouteField(route);

        // Get the first entry in the route field.
        //getRouteUri(0, &uri);
    }

    // Use contact if present
    if(subscribeRequest->getContactUri(0, &uri) && !uri.isNull())
    {

    }

    // Use the from field as we have nothing better to use
   else
   {
      uri.append(fromField.data());
   }


   setRequestData(SIP_NOTIFY_METHOD, uri.data(),
        toField.data(), fromField.data(), callId, localCSequenceNumber);
}

void SipMessage::setNotifyData(const char* uri,
                                const char* fromField,
                                const char* toField,
                                const char* callId,
                                int lastNotifyCseq,
                                const char* eventField,
                                const char* contact,
                                const char* routeField)
{
    // if uri is not set set it to the toField
    UtlString uriStr;
    if( uri && *uri )
    {
        uriStr.append(uri);
    }
    else if ( toField )
    {
        // check for toField  null
        uriStr.append( toField );
    }

    // if contact is not set set it to the fromField
    UtlString contactStr;
    if( contact && *contact )
    {
        contactStr.append(contact);
    }
    else if ( fromField )
    {
        // check for toField  null
        contactStr.append( fromField );
    }

    // Set the NOTIFY event type
    if( eventField && *eventField )
    {
        setEventField(eventField);
    }

    // Set the route should come from from SUBSCRIBE message
    if( routeField && *routeField )
    {
        setRouteField( routeField );
    }

   setRequestData(
        SIP_NOTIFY_METHOD,
        uriStr.data(),
        fromField,
        toField,
        callId,
        lastNotifyCseq,
        contactStr.data() );
}

void SipMessage::setEnrollmentData(const char* uri,
                       const char* fromField,
                       const char* toField,
                       const char* callId,
                       int CSeq,
                       const char* contactUrl,
                       const char* protocolField,
                       const char* profileField,
                       int expiresInSeconds)
{
    setRequestData(SIP_SUBSCRIBE_METHOD, uri,
                     fromField, toField,
                     callId,
                     CSeq,
                            contactUrl);

    // Set the event type
    setHeaderValue(SIP_EVENT_FIELD, SIP_EVENT_CONFIG, 0);

    // Set the protocols
    setHeaderValue(SIP_CONFIG_ALLOW_FIELD, protocolField, 0);

    // Set the profile
    setHeaderValue(SIP_CONFIG_REQUIRE_FIELD, profileField, 0);

   //setRxpires
   setExpiresField(expiresInSeconds);
}

void SipMessage::setVoicemailData(const char* fromField,
                       const char* toField,
                  const char* uri,
                  const char* contactUrl,
                  const char* callId,
                  int CSeq,
                       int expiresInSeconds)
{
    setRequestData(SIP_SUBSCRIBE_METHOD, uri,
                     fromField, toField,
                     callId,
                     CSeq,
                           contactUrl);
    // Set the event type
    setHeaderValue(SIP_EVENT_FIELD, SIP_EVENT_MESSAGE_SUMMARY, 0);
    // Set the allow field
    setHeaderValue(SIP_ACCEPT_FIELD, CONTENT_TYPE_SIMPLE_MESSAGE_SUMMARY, 0);
   setExpiresField(expiresInSeconds);
}

void SipMessage::setRequestTerminatedResponseData(const SipMessage* request)
{
   setResponseData(request, SIP_REQUEST_TERMINATED_CODE, SIP_REQUEST_TERMINATED_TEXT);
}

void SipMessage::setRequestUnauthorized(const SipMessage* request,
                            const char* authenticationScheme,
                            const char* authenticationRealm,
                            const char* authenticationNonce,
                            const char* authenticationOpaque,
                            enum HttpEndpointEnum authEntity)
{
    if(authEntity == SERVER)
    {
        setResponseData(request,
                        HTTP_UNAUTHORIZED_CODE,
                        HTTP_UNAUTHORIZED_TEXT);
    }
    else
    {
        setResponseData(request,
                        HTTP_PROXY_UNAUTHORIZED_CODE,
                        HTTP_PROXY_UNAUTHORIZED_TEXT);
    }

    setAuthenticationData(authenticationScheme, authenticationRealm,
                          authenticationNonce, authenticationOpaque,
                          NULL,
                          authEntity);
}

// This method is needed to cover the symetrical situation which is
// specific to SIP authorization (i.e. authentication and authorize
// fields may be in either requests or responses
UtlBoolean SipMessage::verifyMd5Authorization(const char* userId,
                                             const char* password,
                                             const char* nonce,
                                             const char* realm,
                                             const char* uri,
                                             enum HttpEndpointEnum authEntity) const
{
    UtlString uriString;
    UtlString method;

    if(isResponse())
    {
        int seqNum;
         // What is the correct URI for a response Authorization
        if(uri)
        {
            uriString.append(uri);
        }
        getCSeqField(&seqNum, &method);
    }
    else
    {
       // If the uri (should be Auth header uri parameter) is
       // passed in use it.
       if(uri)
       {
          uriString.append(uri);
       }
       // Otherwise dig out the request URI.  Note: it is not a good
       // idea to use the request uri to validate the digest hash as
       // it may not exactly match the Auth header uri parameter (in
       // which the validation will fail).
       else
       {
          getRequestUri(&uriString);
          OsSysLog::add(FAC_SIP,PRI_DEBUG, "SipMessage::verifyMd5Authorization using request URI: %s instead of Auth header uri parameter for digest\n",
                        uriString.data());
       }
       getRequestMethod(&method);
    }

#ifdef TEST
    OsSysLog::add(FAC_SIP,PRI_DEBUG, "SipMessage::verifyMd5Authorization - "
         "userId='%s', password='%s', nonce='%s', realm='%s', uri='%s', method='%s' \n",
         userId, password, nonce, realm, uriString.data(), method.data());
#endif

    UtlBoolean isAllowed = FALSE;
    isAllowed = HttpMessage::verifyMd5Authorization(userId,
                                        password,
                                        nonce,
                                        realm,
                                        method.data(),
                                        uriString.data(),
                                        authEntity);
    return isAllowed;
}

void SipMessage::setResponseData(const SipMessage* request,
                         int responseCode,
                         const char* responseText)
{
   UtlString fromField;
   UtlString toField;
   UtlString callId;
   int sequenceNum;
   UtlString sequenceMethod;

   request->getFromField(&fromField);
   request->getToField(&toField);
   request->getCallIdField(&callId);
   request->getCSeqField(&sequenceNum, &sequenceMethod);

   setResponseData(responseCode, responseText,
      fromField.data(), toField.data(), callId,
               sequenceNum, sequenceMethod.data());

   setViaFromRequest(request);
}

void SipMessage::setAckData(const char* uri,
                     const char* fromField,
                     const char* toField,
               const char* callId,
               int sequenceNumber)
{
   setRequestData(SIP_ACK_METHOD, uri,
                     fromField, toField,
                     callId, sequenceNumber);
}

void SipMessage::setAckData(const SipMessage* inviteResponse,
                     const SipMessage* inviteRequest,
                            const char* contact,
                            int sessionTimerExpires)
{
   UtlString fromField;
   UtlString toField;
   UtlString uri;
   UtlString callId;
   int sequenceNum;
   int responseCode ;
   UtlString sequenceMethod;
   UtlString requestContact;
   UtlBoolean SetDNSParameters = FALSE;

   inviteResponse->getFromField(&fromField);
   inviteResponse->getToField(&toField);
   responseCode = inviteResponse->getResponseStatusCode();

   // SDUA
   //Set URI

   //Only for 2xx responses check the record route and contact field of response
   if ( responseCode >= SIP_OK_CODE && responseCode < SIP_MULTI_CHOICE_CODE)
   {
       // Set route field if recordRoute was set.
      UtlString routeField;
      UtlString requestToField;
      if(inviteResponse->buildRouteField(&routeField))
      {
         setRouteField(routeField.data());

      }

        // Loose_route always put contact in URI
        inviteResponse->getContactUri( 0 , &uri);
        if (uri.isNull())
        {
            if(inviteRequest)
            inviteRequest->getRequestUri(&uri);
          else
            uri.append(toField.data());
        }

      //if no record route and no contact filed which is a problem of the
      //other side because they should have a contact field
      //We should be more tolerant and use the request uri of INVITE
      //or the to field
      if(uri.isNull())
      {
         if(inviteRequest)
         {
            inviteRequest->getRequestUri(&uri);
         }
         else
         {
            uri.append(toField.data());
         }
      }
   }
   else
   {
      //set uri from request if no contact field in the response
      UtlString routeField;
      if (inviteRequest)
      {
         inviteRequest->getRequestUri(&uri);
         inviteRequest->getRouteField(&routeField);
         if(!routeField.isNull())
            setRouteField(routeField);
      }
      else
         uri.append(toField.data());

      SetDNSParameters = TRUE;
   }

   //set senders contact
   if(contact && *contact)
   {
      setContactField(contact);
   }
   else if ( inviteRequest)
   {
      if( inviteRequest->getContactUri(0, &requestContact))
      {
         setContactField(requestContact);
      }
   }

   //if no record route or contact add sticky DNS
   if ( SetDNSParameters)
   {
      // set the DNS fields
      UtlString protocol;
      UtlString address;
      UtlString port;

      if ( inviteResponse->getDNSField(&protocol , &address , &port))
      {
         setDNSField(protocol , address , port);
      }
   }

   inviteResponse->getCallIdField(&callId);
   inviteResponse->getCSeqField(&sequenceNum, &sequenceMethod);

   setAckData(uri.data(), fromField.data(), toField.data(), callId, sequenceNum);

    if(sessionTimerExpires > 0)
        setSessionExpires(sessionTimerExpires);
}

void SipMessage::setAckErrorData(const SipMessage* byeRequest)
{
   setResponseData(byeRequest, SIP_BAD_REQUEST_CODE, SIP_BAD_REQUEST_TEXT);
}

void SipMessage::setByeData(const char* uri, const char* fromField, const char* toField,
               const char* callId,
               int sequenceNumber)
{
   setRequestData(SIP_BYE_METHOD, uri,
                     fromField, toField,
                     callId, sequenceNumber);
}

void SipMessage::setByeData(const SipMessage* inviteRequest,
                     const char* remoteContact,
                     UtlBoolean byeToCallee,
                            int localCSequenceNumber,
                            //const char* directoryServerUri,
                            const char* routeField,
                            const char* alsoInviteUri)
{
   UtlString fromField;
   UtlString toField;
   UtlString uri;
   UtlString callId;
   int dummySequenceNum;
   UtlString sequenceMethod;
   UtlString remoteContactString;

   if (remoteContact)
      remoteContactString.append(remoteContact);

   inviteRequest->getFromField(&fromField);
   inviteRequest->getToField(&toField);
   inviteRequest->getCallIdField(&callId);
   inviteRequest->getCSeqField(&dummySequenceNum, &sequenceMethod);

    if(routeField && *routeField)
    {
        setRouteField(routeField);
    }

   //SDUA
    if (!remoteContactString.isNull())
   {
      uri.append(remoteContactString);
   }

    // Use the route uri if set
    if(!uri.isNull())
    {
    }

    // Use the original uri from the INVITE if the INVITE is from
    // this side.
   else if(byeToCallee)
   {
      inviteRequest->getRequestUri(&uri);
   }

    // Use contact if present
    else if(inviteRequest->getContactUri(0, &uri) && !uri.isNull())
    {

    }

   else
   {
      uri.append(fromField.data());
   }

   if(byeToCallee)
   {
      setByeData(uri.data(), fromField.data(), toField.data(), callId, localCSequenceNumber);
   }
   else
   {
      setByeData(uri.data(), toField.data(), fromField.data(), callId, localCSequenceNumber);
   }


    if(alsoInviteUri && *alsoInviteUri)
    {
        if(!isRequireExtensionSet(SIP_CALL_CONTROL_EXTENSION))
        {
            addRequireExtension(SIP_CALL_CONTROL_EXTENSION);
        }
        addAlsoUri(alsoInviteUri);
    }

}


void SipMessage::setReferData(const SipMessage* inviteRequest,
                     UtlBoolean referToCallee,
                            int localCSequenceNumber,
                            const char* routeField,
                            const char* contactUrl,
                            const char* remoteContactUrl,
                            const char* transferTargetAddress,
                            const char* targetCallId)
{
   UtlString fromField;
   UtlString toField;
   UtlString uri;
   UtlString callId;
   int dummySequenceNum;
   UtlString sequenceMethod;

   inviteRequest->getFromField(&fromField);
   inviteRequest->getToField(&toField);
   inviteRequest->getCallIdField(&callId);
   inviteRequest->getCSeqField(&dummySequenceNum, &sequenceMethod);

    if(routeField && *routeField)
    {
        setRouteField(routeField);

        // Get the first entry in the route field.
        //getRouteUri(0, &uri);
    }

    if(remoteContactUrl)
    {
        uri = remoteContactUrl;
    }

    // Use the route uri if set
    else if(!referToCallee)
    {
        inviteRequest->getContactUri(0, &uri);
    }

    // Use the original uri from the INVITE if the INVITE is from
    // this side.
   else if(referToCallee)
   {
      inviteRequest->getRequestUri(&uri);
   }

    // Use contact if present
    else if(inviteRequest->getContactUri(0, &uri) && !uri.isNull())
    {

    }

   else
   {
      uri.append(fromField.data());
   }

    UtlString referredByField;
   if(referToCallee)
   {
      //setByeData(uri.data(), fromField.data(), toField.data(), callId, localCSequenceNumber);
        setRequestData(SIP_REFER_METHOD, uri.data(),
                  fromField.data(), toField.data(),
                  callId, localCSequenceNumber,
                        contactUrl);

        Url referToUrl(fromField);
        referToUrl.removeFieldParameters();
        referToUrl.includeAngleBrackets();
        referToUrl.toString(referredByField);
   }
   else
   {
      //setByeData(uri.data(), toField.data(), fromField.data(), callId, localCSequenceNumber);
        setRequestData(SIP_REFER_METHOD, uri.data(),
                  toField.data(), fromField.data(),
                  callId, localCSequenceNumber,
                        contactUrl);

        Url referToUrl(toField);
        referToUrl.removeFieldParameters();
        referToUrl.includeAngleBrackets();
        referToUrl.toString(referredByField);
   }

    //referredByField.append("; ");
    if(transferTargetAddress && *transferTargetAddress)
    {
        UtlString targetAddress(transferTargetAddress);
        Url targetUrl(targetAddress);
        UtlString referTo;

        if(targetCallId && *targetCallId)
        {
            //targetAddress.append("?Call-ID=");
            //targetAddress.append(targetCallId);
            targetUrl.setHeaderParameter(SIP_CALLID_FIELD, targetCallId);
        }

        // Include angle brackets on the Refer-To header.  We don't
        // need to do this, but it is the friendly thing to do.
        targetUrl.includeAngleBrackets();
        targetUrl.toString(referTo);

        // This stuff went away in the Transfer-05/Refer-02 drafts
        // We need angle brackets for the refer to url in the referred-by
        //targetUrl.includeAngleBrackets();
        //targetUrl.toString(targetAddress);
        //referredByField.append("ref=");
        //referredByField.append(targetAddress);

        setReferredByField(referredByField.data());
        setReferToField(referTo.data());
    }

}

void SipMessage::setReferOkData(const SipMessage* referRequest)
{
   setResponseData(referRequest, SIP_OK_CODE, SIP_OK_TEXT);
}

void SipMessage::setReferDeclinedData(const SipMessage* referRequest)
{
   setResponseData(referRequest, SIP_DECLINE_CODE, SIP_DECLINE_TEXT);
}

void SipMessage::setReferFailedData(const SipMessage* referRequest)
{
   setResponseData(referRequest, SIP_SERVICE_UNAVAILABLE_CODE, SIP_SERVICE_UNAVAILABLE_TEXT);
}

void SipMessage::setOptionsData(const SipMessage* inviteRequest,
                        const char* remoteContact,
                        UtlBoolean optionsToCallee,
                        int localCSequenceNumber,
                        const char* routeField)
{
   UtlString fromField;
   UtlString toField;
   UtlString uri;
   UtlString callId;
   int dummySequenceNum;
   UtlString sequenceMethod;

   inviteRequest->getFromField(&fromField);
   inviteRequest->getToField(&toField);
   inviteRequest->getCallIdField(&callId);
   inviteRequest->getCSeqField(&dummySequenceNum, &sequenceMethod);

    if(routeField && *routeField)
    {
        setRouteField(routeField);

        // We do not do this any more with loose and strict routing
        // The URI is set to the remote contact (target URI) and
        // the route comes directy from the Record-Route field
        // Get the first entry in the route field.
        //getRouteUri(0, &uri);
    }

   //set the uri to the contact uri returned in the last response from the other side
   if (remoteContact && *remoteContact)
   {
      uri.append(remoteContact);
   }

    // Use the remoteContact uri if set
    if(!uri.isNull())
    {
    }

    // Use the original uri from the INVITE if the INVITE is from
    // this side.
   else if(optionsToCallee)
   {
      inviteRequest->getRequestUri(&uri);
   }

    // Use contact if present
    else if(inviteRequest->getContactUri(0, &uri) && !uri.isNull())
    {

    }

   else
   {
      uri.append(fromField.data());
   }

    UtlString referredByField;
   if(optionsToCallee)
   {
      //setByeData(uri.data(), fromField.data(), toField.data(), callId, localCSequenceNumber);
        setRequestData(SIP_OPTIONS_METHOD, uri.data(),
                  fromField.data(), toField.data(),
                  callId, localCSequenceNumber);

        referredByField = fromField;
   }
   else
   {
      //setByeData(uri.data(), toField.data(), fromField.data(), callId, localCSequenceNumber);
        setRequestData(SIP_OPTIONS_METHOD, uri.data(),
                  toField.data(), fromField.data(),
                  callId, localCSequenceNumber);

        referredByField = toField;
   }
}

void SipMessage::setByeErrorData(const SipMessage* byeRequest)
{
   setResponseData(byeRequest, SIP_BAD_REQUEST_CODE, SIP_BAD_REQUEST_TEXT);
}

void SipMessage::setCancelData(const char* fromField, const char* toField,
               const char* callId,
               int sequenceNumber)
{
   setRequestData(SIP_CANCEL_METHOD, toField,
                     fromField, toField,
                     callId, sequenceNumber);
}

void SipMessage::setCancelData(const SipMessage* inviteRequest)
{
    UtlString uri;
   UtlString fromField;
   UtlString toField;
   UtlString callId;
   int sequenceNum;
   UtlString sequenceMethod;

   inviteRequest->getFromField(&fromField);
   inviteRequest->getToField(&toField);
   inviteRequest->getCallIdField(&callId);
   inviteRequest->getCSeqField(&sequenceNum, &sequenceMethod);
    inviteRequest->getRequestUri(&uri);

   //setCancelData(fromField.data(), toField.data(), callId, sequenceNum);
    setRequestData(SIP_CANCEL_METHOD, uri,
                  fromField, toField,
                  callId, sequenceNum);
}


void SipMessage::addVia(const char* domainName,
                        int port,
                        const char* protocol,
                        const char* branchId)
{
   UtlString viaField(SIP_PROTOCOL_VERSION);
   char portString[MAXIMUM_INTEGER_STRING_LENGTH + 1];

   viaField.append("/");
   if(protocol && strlen(protocol))
   {
      viaField.append(protocol);
   }

   // Default the protocol if not set
   else
   {
      viaField.append(SIP_TRANSPORT_TCP);
   }
   viaField.append(" ");
   viaField.append(domainName);
   if(port > 0)
   {
      sprintf(portString, ":%d", port);
      viaField.append(portString);
   }

    if(branchId && *branchId)
    {
        viaField.append(';');
        viaField.append("branch");
        viaField.append('=');
        viaField.append(branchId);
    }

   addViaField(viaField.data());
}

void SipMessage::addViaField(const char* viaField, UtlBoolean afterOtherVias)
{
    mHeaderCacheClean = FALSE;

   NameValuePair* nv = new NameValuePair(SIP_VIA_FIELD, viaField);
    // Look for other via fields
    unsigned int fieldIndex = mNameValues.index(nv);

    //osPrintf("SipMessage::addViaField via: %s after: %s fieldIndex: %d headername: %s\n",
    //    viaField, afterOtherVias ? "true" : "false", fieldIndex,
    //    SIP_VIA_FIELD);


    if(fieldIndex == UTL_NOT_FOUND)
    {
#ifdef TEST_PRINT
        UtlDListIterator iterator(nameValues);

        //remove whole line
        NameValuePair* nv = NULL;
        while(nv = (NameValuePair*) iterator())
        {
            osPrintf("\tName: %s\n", nv->data());
        }
#endif
    }

    mHeaderCacheClean = FALSE;

    if(fieldIndex == UTL_NOT_FOUND || !afterOtherVias)
    {
      mNameValues.insert(nv);
    }
    else
    {
        mNameValues.insertAt(fieldIndex, nv);
    }
}

void SipMessage::setLastViaTag(const char* tagValue,
                               const char* tagName)
{
     UtlString lastVia;
   //get last via field and remove it
    getViaFieldSubField(&lastVia, 0);
   removeLastVia();
   //update the last via and add the updated field
    //setUriParameter(&lastVia, tagName, receivedFromIpAddress);

    //parse all name=value pairs into a collectable object
    UtlSList list;
    parseViaParameters(lastVia,list);

    //create an iterator to walk the list
    UtlSListIterator iterator(list);
   NameValuePair* nvPair;
    UtlString newVia;
    UtlBoolean bFoundTag = FALSE;
    UtlString value;

   while ((nvPair = (NameValuePair*) iterator()))
   {
        value.remove(0);

        //only if we have something in our newVia string do we add a semicolon
        if (newVia.length())
            newVia.append(";");

        //always append the name part of the value pair
        newVia.append(nvPair->data());

        UtlString strPairName = nvPair->data();
        UtlString strTagName = tagName;

        //convert both to upper
        strPairName.toUpper();
        strTagName.toUpper();

        //if the value we are looking for is found, then we are going to replace the value with this value
        if (strTagName == strPairName)
        {
            if (tagValue)
                value = tagValue;
            else
                //the value could come in as NULL.  In this case make it be an empty string
                value = "";

            bFoundTag = TRUE;
        }
        else
            //ok we didn't find the one we are looking for
            value = nvPair->getValue();

        //if the value has a length then append it
        if (value.length())
        {
            newVia.append("=");
            newVia.append(value);
        }

   }

    //if we didn't find the tag we are looking for after looping, then
    //we should add the name and value pair at the end
    if (!bFoundTag)
    {
        //add a semicolon before our new name value pair is added
        newVia.append(";");
        newVia.append(tagName);

        //only if it is non-NULL and has a length do we add the equal and value
        //So, if the value is "" we will only put the name (without equal)
        if (tagValue  && strlen(tagValue))
        {
            newVia.append("=");
            newVia.append(tagValue);
        }
    }

    addViaField(newVia);

    list.destroyAll();
}

void SipMessage::setViaFromRequest(const SipMessage* request)
{
   UtlString viaSubField;
   int subFieldindex = 0;

   while(request->getViaFieldSubField(&viaSubField, subFieldindex ))
   {
#ifdef TEST
      osPrintf("Adding via field: %s\n", viaSubField.data());
#endif
      addViaField(viaSubField.data(), FALSE);
      subFieldindex++;
   }
}

void SipMessage::setCallIdField(const char* callId)
{
    setHeaderValue(SIP_CALLID_FIELD, callId);
}

void SipMessage::setCSeqField(int sequenceNumber, const char* method)
{
   UtlString value;
   char numString[HTTP_LONG_INT_CHARS];

   sprintf(numString, "%d", sequenceNumber);

   value.append(numString);
   value.append(SIP_SUBFIELD_SEPARATOR);
   value.append(method);

    setHeaderValue(SIP_CSEQ_FIELD, value.data());
}

void SipMessage::incrementCSeqNumber()
{
    int seqNum;
    UtlString seqMethod;
    if(!getCSeqField(&seqNum, &seqMethod))
    {
        seqNum = 1;
        seqMethod.append("UNKNOWN");
    }
    seqNum++;
    setCSeqField(seqNum, seqMethod.data());
}

void SipMessage::buildSipUrl(UtlString* url, const char* address, int port,
                      const char* protocol, const char* user,
                      const char* userLabel, const char* tag)
{
   char portString[MAXIMUM_INTEGER_STRING_LENGTH + 1];
   url->remove(0);
   UtlString tmpAddress(address);
   tmpAddress.toUpper();

   if(userLabel && strlen(userLabel))
   {
      url->append(userLabel);
      url->append("<");
   }

   // If the SIP url type label is not already in the address
   int sipLabelIndex = tmpAddress.index(SIP_URL_TYPE);
   //osPrintf("Found \"%s\" in \"%s\" at index: %d\n",
   // SIP_URL_TYPE, tmpAddress.data(),
   //   sipLabelIndex);

   if(sipLabelIndex < 0 && !tmpAddress.isNull())
   {
      // Use lower case for more likely interoperablity
      UtlString sipLabel(SIP_URL_TYPE);
      sipLabel.toLower();
      url->append(sipLabel.data());
   }
   //else
   //{
   // osPrintf("SIP: not added index: %d\n", sipLabelIndex);
   //}

   if(!strstr(address, "@") && user && strlen(user))
   {
      url->append(user);
      url->append('@');
   }

   url->append(address);

   if(port > 0)
   {
      sprintf(portString, ":%d", port);
      url->append(portString);
   }

   if(protocol && strlen(protocol) > 0)
   {
      url->append(";transport=");
      url->append(protocol);
   }
   if(userLabel && strlen(userLabel))
   {
      url->append(">");
   }

   if(tag && strlen(tag))
   {
      url->append(";tag=");
      url->append(tag);
   }

   tmpAddress.remove(0);
}

void SipMessage::setFromField(const char* url)
{
   UtlString value;
   UtlString address;
   UtlString protocol;
   UtlString user;
   UtlString userLabel;
   int port;

   parseAddressFromUri(url, &address, &port, &protocol, &user,
      &userLabel);
   buildSipUrl(&value, address.data(), port, protocol.data(),
      user.data(), userLabel.data());

   // IF the field exists change it, if does not exist create it
   setHeaderValue(SIP_FROM_FIELD, value.data(), 0);
}

void SipMessage::setFromField(const char* address, int port,
                       const char* protocol,
                       const char* user, const char* userLabel)
{
   UtlString url;
   //UtlString field;

   buildSipUrl(&url, address, port, protocol, user, userLabel);


   //buildSipToFromField(&field, url.data(), label);

   // IF the field exists change it, if does not exist create it
   setHeaderValue(SIP_FROM_FIELD, url.data(), 0);
}

void SipMessage::setRawToField(const char* url)
{
   //UtlString value;
   //UtlString address;
   //UtlString protocol;
   //UtlString user;
   //UtlString userLabel;
   //UtlString tag;
   //int port;

   //parseAddressFromUri(url, &address, &port, &protocol, &user,
   // &userLabel, &tag);
   //buildSipUrl(&value, address.data(), port, protocol.data(),
   // user.data(), userLabel.data(), tag.data());

   // IF the field exists change it, if does not exist create it
   setHeaderValue(SIP_TO_FIELD, url, 0);
}

void SipMessage::setRawFromField(const char* url)
{
   //UtlString value;
   //UtlString address;
   //UtlString protocol;
   //UtlString user;
   //UtlString userLabel;
   //UtlString tag;
   //int port;

   //parseAddressFromUri(url, &address, &port, &protocol, &user,
   // &userLabel, &tag);
   //buildSipUrl(&value, address.data(), port, protocol.data(),
   // user.data(), userLabel.data(), tag.data());

   // IF the field exists change it, if does not exist create it
   setHeaderValue(SIP_FROM_FIELD, url, 0);
}

void SipMessage::setToField(const char* address, int port,
                       const char* protocol,
                       const char* user,
                       const char* userLabel)
{
   UtlString url;

   buildSipUrl(&url, address, port, protocol, user, userLabel);

   // IF the field exists change it, if does not exist create it
   setHeaderValue(SIP_TO_FIELD, url.data(), 0);
}

void SipMessage::setExpiresField(int expiresInSeconds)
{
   char secondsString[MAXIMUM_INTEGER_STRING_LENGTH];
   sprintf(secondsString, "%d", expiresInSeconds);

   // IF the field exists change it, if does not exist create it
   setHeaderValue(SIP_EXPIRES_FIELD, secondsString, 0);
}

void SipMessage::setContactField(const char* contactField, int index)
{
   // IF the field exists change it, if does not exist create it
   setHeaderValue(SIP_CONTACT_FIELD, contactField, index);
}

void SipMessage::setRequestDispositionField(const char* dispositionField)
{
   // IF the field exists change it, if does not exist create it
   setHeaderValue(SIP_REQUEST_DISPOSITION_FIELD, dispositionField, 0);
}

void SipMessage::addRequestDisposition(const char* dispositionToken)
{
    // Append to the field value already there, if it exists
    UtlString field;
    getRequestDispositionField(&field);
    if(!field.isNull())
    {
        field.append(' ');
    }

    field.append(dispositionToken);
    setRequestDispositionField(field.data());
}


void SipMessage::changeUri(const char* newUri)
{
   //UtlString address;
   //UtlString protocol;
   //int port;
   UtlString uriString;
    //UtlString user;

   //parseAddressFromUri(newUri, &address, &port, &protocol, &user);
   //buildSipUrl(&uriString, address.data(), port, protocol.data(),
    //    user.data());

    // Remove the stuff that should not be in a URI
    Url cleanUri(newUri);
    cleanUri.getUri(uriString);
   changeRequestUri(uriString.data());
}

UtlBoolean SipMessage::getMaxForwards(int& maxForwards) const
{
    const char* value = getHeaderValue(0, SIP_MAX_FORWARDS_FIELD);

    if(value)
    {
        maxForwards = atoi(value);
    }
    return(value != NULL);
}


void SipMessage::setMaxForwards(int maxForwards)
{
    char buf[64];
    sprintf(buf, "%d", maxForwards);
    setHeaderValue(SIP_MAX_FORWARDS_FIELD,buf, 0);
}

void SipMessage::decrementMaxForwards()
{
    int maxForwards;
    if(!getMaxForwards(maxForwards))
    {
        maxForwards = SIP_DEFAULT_MAX_FORWARDS;
    }
    maxForwards--;
    setMaxForwards(maxForwards);
}

void SipMessage::getFromField(UtlString* field) const
{
   const char* value = getHeaderValue(0, SIP_FROM_FIELD);

   if(value)
   {
        *field = value;
   }
    else
    {
        field->remove(0);
    }
}

void SipMessage::getToField(UtlString* field) const
{
   const char* value = getHeaderValue(0, SIP_TO_FIELD);

   if(value)
   {
        *field = value;
   }
    else
    {
        field->remove(0);
    }
}

void SipMessage::getToUrl(Url& toUrl) const
{
    //UtlString toField;
    //getToField(&toField);
    const char* toField = getHeaderValue(0, SIP_TO_FIELD);

    toUrl = toField ? toField : "";
}

void SipMessage::getFromUrl(Url& fromUrl) const
{
    //UtlString fromField;
    //getFromField(&fromField);
    const char* fromField = getHeaderValue(0, SIP_FROM_FIELD);

    fromUrl = fromField ? fromField : "";
}

void SipMessage::getFromLabel(UtlString* label) const
{
   UtlString field;
   int labelEnd;

   getFromField(&field);

   label->remove(0);

   if(!field.isNull())
   {
      labelEnd = field.index(" <");
      if(labelEnd >= 0)
      {
         label->append(field.data());
         label->remove(labelEnd);
      }
   }
}

void SipMessage::getToLabel(UtlString* label) const
{
   UtlString field;
   int labelEnd;

   getToField(&field);

   label->remove(0);

   if(!field.isNull())
   {
      labelEnd = field.index(" <");
      if(labelEnd >= 0)
      {
         label->append(field.data());
         label->remove(labelEnd);
      }
   }
}

UtlBoolean SipMessage::parseParameterFromUri(const char* uri,
                                            const char* parameterName,
                                            UtlString* parameterValue)
{
    UtlString parameterString(parameterName);
    UtlString uriString(uri);
    parameterString.append("=");
    // This may need to be changed to be make case insensative
    int parameterStart = uriString.index(parameterString.data());
    // 0, UtlString::ignoreCase);

    parameterValue->remove(0);

    //osPrintf("SipMessage::parseParameterFromUri uri: %s parameter: %s index: %d\n",
    //    uriString.data(), parameterString.data(), parameterStart);
    if(parameterStart >= 0)
    {
        parameterStart += parameterString.length();
        uriString.remove(0, parameterStart);
        NameValueTokenizer::frontTrim(&uriString, " \t");
        //osPrintf("SipMessage::parseParameterFromUri uriString: %s index: %d\n",
        //  uriString.data(), parameterStart);
        NameValueTokenizer::getSubField(uriString.data(), 0,
            " \t;>", parameterValue);

    }

    return(parameterStart >= 0);
}

void SipMessage::parseAddressFromUri(const char* uri,
                            UtlString* address,
                            int* port,
                            UtlString* protocol,
                            UtlString* user,
                            UtlString* userLabel,
                            UtlString* tag)
{
   // A SIP url looks like the following:
   // "user label <SIP:user@address:port ;tranport=protocol> ;tag=nnnn"
   // labelEnd ===^
   // sipUrlLabelEnd =^
   // userEnd =============^
   // startIndex ===========^
   // endIndex =========================^
   // parameterIndex ====================^
   // protocolIndex ===============================^
   // endAddrSpecIndex ====================================^
   // tagIndex ===================================================^
   // tagEnd =========================================================^


   int startIndex = -1;
   int endIndex;
    UtlString uriString(uri ? uri : "");
   int parameterIndex;
   int protocolIndex;
   int endAddrSpecIndex;
   int tagIndex;
   int tagEnd;

   *port = 0;

   if(userLabel)
   {
      userLabel->remove(0);
   }

   int labelEnd = uriString.index("<");
   if(labelEnd > 0 && userLabel)
   {
      userLabel->append(uriString, labelEnd);
      NameValueTokenizer::frontBackTrim(userLabel, " \t\"\n\r");
   }
#ifdef TEST_PRINT
    if( userLabel)
    {
        osPrintf("SipMessage::parseAddressFromUri labelEnd: %d userLabel: \"%s\"\n",
            labelEnd, userLabel->data());
    }
#endif

   //UtlString tmpUri(uriString);
   //tmpUri.toUpper();
   int sipUrlLabelEnd = uriString.index(SIP_URL_TYPE, 0, UtlString::ignoreCase);
   if(sipUrlLabelEnd >= 0)
   {
      sipUrlLabelEnd += strlen(SIP_URL_TYPE);
   }

   //osPrintf("SipMessage::parseAddressFromUri uri: %s\n", uri);
   // Check for user terminator
   int userEnd = uriString.index("@");
    endAddrSpecIndex = uriString.index(">");
    parameterIndex = uriString.index(";", userEnd >=0 ? userEnd : 0);

    // if endAddrSpecIndex or parameterIndex where found (i.e. < or ;)
    // and they occur before the @, ignore the @ as it is not the
    // user id terminator
    if(userEnd >= 0 &&
        ((endAddrSpecIndex >= 0 && endAddrSpecIndex < userEnd) ||
        (parameterIndex >= 0 && parameterIndex < userEnd)))
        userEnd = -1;
#ifdef TEST_PRINT
    osPrintf("SipMessage::parseAddressFromUri userEnd: %d endAddrSpecIndex: %d parameterIndex: %d\n",
        userEnd, endAddrSpecIndex, parameterIndex);
#endif

   if(userEnd >= 0)
   {
      if(user)
      {
            user->remove(0);
         user->append(uriString, userEnd);
         if(labelEnd >= 0 && labelEnd > sipUrlLabelEnd)
         {
            user->remove(0, labelEnd + 1);
         }
         else if(sipUrlLabelEnd > 0)
         {
            user->remove(0, sipUrlLabelEnd);
         }
      }

      startIndex = userEnd + 1;
   }
    else if(user)
    {
        user->remove(0);
    }

   //osPrintf("user startIndex: %d\n", startIndex);

   // Found no user terminator, check for SIP url label
   if(startIndex < 0)
   {
      if(sipUrlLabelEnd >= 0)
      {
         startIndex = sipUrlLabelEnd;
      }
      else if(labelEnd >= 0)
      {
         startIndex = labelEnd + 1;
      }
#ifdef TEST_PRINT
      osPrintf("SIP url startIndex: %d\n", startIndex);
#endif
   }

   // Remove the stuff before the address
   if(startIndex > 0)
   {
      uriString.remove(0, startIndex);
   }

   //osPrintf("front trimmed uri: %s\n", uriString.data());

   // Check for port
   endIndex = uriString.index(":");
   if(endIndex >= 0)
   {
      //osPrintf("found port seperator index: %d\n", endIndex);

      // Get the address
        address->remove(0);
      address->append(uriString, endIndex);

      // Remove the address from the working string
      uriString.remove(0, endIndex + 1);

      // Get the port
        int numPortChars;
        int subFieldEnd = uriString.index(SIP_SUBFIELD_SEPARATOR);
        int addrSpecEnd = uriString.index(">");
        parameterIndex = uriString.index(";");
        numPortChars = uriString.length();
        numPortChars = ( subFieldEnd > 0
                    ? (  numPortChars > subFieldEnd
                       ? subFieldEnd : numPortChars
                       )
                    : numPortChars
                    );
        numPortChars = ( addrSpecEnd > 0
                    ? (  numPortChars > addrSpecEnd
                       ? addrSpecEnd : numPortChars
                       )
                    : numPortChars
                    );
        numPortChars = ( parameterIndex > 0
                    ? (  numPortChars > parameterIndex
                       ? parameterIndex : numPortChars
                       )
                    : numPortChars
                    );

        char portBuffer[10];
        if(numPortChars > 5)
        {
            OsSysLog::add(FAC_SIP, PRI_WARNING,
                          "SipMessage::parseAddresFromUri port should not be more than 5 digits: %s len: %d.\nTruncating to 5 digits.\n",
                          uriString.data(), numPortChars
                          );
            numPortChars = 5;
        }
        memcpy(portBuffer, uriString.data(), numPortChars);
        portBuffer[numPortChars] = '\0';
        *port = atoi(portBuffer);
   }

   // No Port
   else
   {
      *address = uriString;
      // Check for end of field
      endIndex = uriString.index(SIP_SUBFIELD_SEPARATOR);
      if(endIndex > 0)
      {
         address->remove(endIndex);
      }
      parameterIndex = address->index(";");
      if(parameterIndex >= 0)
      {
         address->remove(parameterIndex);
      }
      endAddrSpecIndex = address->index(">");
      if(endAddrSpecIndex >= 0)
      {
         address->remove(endAddrSpecIndex);
      }
#ifdef TEST_PRINT
      osPrintf("field endIndex: %d parameterIndex: %d\n addrSpecEndIndex: %d",
         endIndex, parameterIndex, endAddrSpecIndex);
#endif

   }

   // Look for protocol parameter
   if(protocol)
   {
      protocol->remove(0);
      protocolIndex = uriString.index("transport=", 0, UtlString::ignoreCase);

      if(protocolIndex >= 0 && protocol)
      {
         protocol->append(&((uriString.data())[protocolIndex + 10]));
         protocol->remove(3);
         protocol->toUpper();
#ifdef TEST_PRINT
         osPrintf("Found protocol: %s\n", protocol->data());
#endif
      }
   }

   // Look for tag parameter
   if(tag)
   {
      tag->remove(0);
      tagIndex = uriString.index("tag=", 0, UtlString::ignoreCase);
#ifdef TEST_PRINT
        osPrintf("SipMessage::parseAddressFromUri tag uriString: \"%s\" tagIndex: %d\n",
            uriString.data(), tagIndex);
#endif

      if(tagIndex >= 0)
      {
         tag->append(&((uriString.data())[tagIndex + 4]));
         NameValueTokenizer::frontTrim(tag, " \t");
         tagEnd = tag->index(" ");
         if(tagEnd > 0)
         {
            tag->remove(tagEnd);
         }
         tagEnd = tag->index("\t");
         if(tagEnd > 0)
         {
            tag->remove(tagEnd);
         }
         tagEnd = tag->index(";");
         if(tagEnd > 0)
         {
            tag->remove(tagEnd);
         }
            tagEnd = tag->index(">");
         if(tagEnd > 0)
         {
            tag->remove(tagEnd);
         }
#ifdef TEST_PRINT
         osPrintf("Found tag: \"%s\" start: %d end: %d\n", tag->data(),
                tagIndex, tagEnd);
#endif
      }
   }
#ifdef TEST_PRINT
    osPrintf("SipMessage::parseAddressFromUri NULL tag\n");
#endif


#ifdef TEST_PRINT
   osPrintf("SipMessage::parseAddressFromUri address: %s port: %d\n",
      address->data(), *port);
#endif

}

void SipMessage::setUriParameter(UtlString* uri, const char* parameterName,
                                 const char* parameterValue)
{
    UtlString parameterString(parameterName);

    //only append the '=' if not null and has a length
    if (parameterValue && *parameterValue != '\0')
        parameterString.append('=');

   int tagIndex = uri->index(parameterString.data());

   // Tag already exists, replace it
   if(tagIndex >= 0)
   {
      //osPrintf("Found tag at index: %d\n", tagIndex);
        // Minimally start after the tag name & equal sign
      tagIndex+= parameterString.length();
      int tagSpace = uri->index(' ', tagIndex);
      int tagTab = uri->index('\t', tagIndex);
      int tagSemi = uri->index(';', tagIndex);
      int tagEnd = tagSpace;
      if(tagTab >= tagIndex && (tagTab < tagEnd || tagEnd < tagIndex))
      {
         tagEnd = tagTab;
      }
      if(tagSemi >= tagIndex && (tagSemi < tagEnd || tagEnd < tagIndex))
      {
         tagEnd = tagSemi;
      }

      // Remove up to the separator
      if(tagEnd >= tagIndex)
      {
         uri->remove(tagIndex, tagEnd - tagIndex);
      }
      // Remove to the end, no separator found
      else
      {
         uri->remove(tagIndex);
      }

        //only insert the value if not null and has a length
        if (parameterValue && *parameterValue != '\0')
          uri->insert(tagIndex, parameterValue);
   }

   // Tag does not exist append it
   else
   {
      //osPrintf("Found no tag appending to the end\n");
      uri->append(";");
        uri->append(parameterString.data());

        //only add the value if not null and has a length
        if (parameterValue && *parameterValue != '\0')
         uri->append(parameterValue);
   }
}

void SipMessage::setToFieldTag(const char* tagValue)
{
   UtlString toField;
   getToField(&toField);
   //osPrintf("To field before: \"%s\"\n", toField.data());
   setUriTag(&toField, tagValue);
   //osPrintf("To field after: \"%s\"\n", toField.data());
   setRawToField(toField.data());
}

void SipMessage::setToFieldTag(int tagValue)
{
    char tagString[MAXIMUM_INTEGER_STRING_LENGTH];
    sprintf(tagString, "%d", tagValue);
    setToFieldTag(tagString);
}

void SipMessage::setUriTag(UtlString* uri, const char* tagValue)
{
    setUriParameter(uri, "tag", tagValue);
}

void SipMessage::getUri(UtlString* address, int* port,
                     UtlString* protocol,
                     UtlString* user) const
{

   UtlString uriField;
   getRequestUri(&uriField);

   if( !uriField.isNull())
   {
      //Uri field will only have URL parameters . So add angle backets around the
      //whole string. else the url parameters will be trated as filed and header parameters

      Url uriUrl(uriField, TRUE); // is addrSpec

      if(address)
      {
          uriUrl.getHostAddress(*address);
      }
      if(protocol)
      {
          uriUrl.getUrlParameter("transport", *protocol);
      }
      if(port)
      {
          *port = uriUrl.getHostPort();
      }

      if(user)
      {
         uriUrl.getUserId(*user);
      }
   }
// parseAddressFromUri(uriField.data(), address, port, protocol, user);
}

void SipMessage::getFromAddress(UtlString* address, int* port, UtlString* protocol,
                        UtlString* user, UtlString* userLabel,
                        UtlString* tag) const
{
   UtlString uri;
   getFromField(&uri);

   parseAddressFromUri(uri.data(), address, port, protocol, user, userLabel,
      tag);
}

void SipMessage::getToAddress(UtlString* address, int* port, UtlString* protocol,
                       UtlString* user, UtlString* userLabel,
                       UtlString* tag) const
{
   UtlString uri;
   getToField(&uri);

   parseAddressFromUri(uri.data(), address, port, protocol, user, userLabel,
      tag);
}

void SipMessage::getFromUri(UtlString* uri) const
{
   UtlString field;
   int labelEnd;

   getFromField(&field);

   unsigned int uriEnd;

   uri->remove(0);

   if(!field.isNull())
   {
      labelEnd = field.index("<");
      // Look for a label terminator
      if(labelEnd >= 0)
      {
         // Remove the label and terminator
         labelEnd += 1;
         field.remove(0, labelEnd);

         // Find the URI terminator
         uriEnd = field.index(">");

         // No URI terminator found, assume the whole thing
         if(uriEnd == UTL_NOT_FOUND)
         {
            uri->append(field.data());
         }
         // Remove the terminator
         else
         {
            field.remove(uriEnd);
            uri->append(field.data());
         }
      }
      // There is no label take the whole thing as the URI
      else
      {
         uri->append(field.data());
      }
   }
}

UtlBoolean SipMessage::getResponseSendAddress(UtlString& address,
                                             int& port,
                                             UtlString& protocol) const
{
    int receivedPort;
    UtlBoolean receivedSet;
    UtlBoolean maddrSet;
    UtlBoolean receivedPortSet;

    // use the via as the place to send the response
    getLastVia(&address, &port, &protocol, &receivedPort,
        &receivedSet, &maddrSet, &receivedPortSet);

    // If the sender of the request indicated support of
    // rport (i.e. received port) send this response back to
    // the same port it came from
    if(receivedSet && receivedPortSet && receivedPort > 0)
    {
        OsSysLog::add(FAC_SIP, PRI_DEBUG, "SipMessage::getResponseSendAddress response to receivedPort %s:%d not %d\n",
            address.data(), receivedPort, port);
        port = receivedPort;
    }

    // No  via, use the from
    if(address.isNull())
    {
        OsSysLog::add(FAC_SIP, PRI_WARNING, "SipMessage::getResponseSendAddress No VIA address, using To address\n");

        getFromAddress(&address, &port, &protocol);
    }

    return(!address.isNull());
}

void SipMessage::convertProtocolStringToEnum(const char* protocolString,
                        enum OsSocket::SocketProtocolTypes& protocolEnum)
{
    if(strcasecmp(protocolString, SIP_TRANSPORT_UDP) == 0)
    {
        protocolEnum = OsSocket::UDP;
    }
    else if(strcasecmp(protocolString, SIP_TRANSPORT_TCP) == 0)
    {
        protocolEnum = OsSocket::TCP;
    }

    else if(strcasecmp(protocolString, SIP_TRANSPORT_TLS) == 0)
    {
        protocolEnum = OsSocket::SSL_SOCKET;
    }
    else
    {
        OsSysLog::add(FAC_SIP, PRI_ERR, "SipMessage::convertProtocolStringToEnum protocol not specified: %s",
            protocolString);
        protocolEnum = OsSocket::UNKNOWN;
    }

}

void SipMessage::convertProtocolEnumToString(enum OsSocket::SocketProtocolTypes protocolEnum,
                                            UtlString& protocolString)
{
    switch(protocolEnum)
    {
    case OsSocket::UDP:
        protocolString = SIP_TRANSPORT_UDP;
            ;
        break;

    case OsSocket::TCP:
        protocolString = SIP_TRANSPORT_TCP;
            ;

        break;

    case OsSocket::SSL_SOCKET:
        protocolString = SIP_TRANSPORT_TLS;
            ;
        break;

    default:
        protocolString = SIP_TRANSPORT_UDP;

        break;
    }
}

void SipMessage::getToUri(UtlString* uri) const
{
   UtlString field;
   int labelEnd;

   getToField(&field);

   int uriEnd;

   uri->remove(0);

   if(!field.isNull())
   {
      labelEnd = field.index("<");
      // Look for a label terminator
      if(labelEnd >= 0)
      {
         // Remove the label and terminator
         labelEnd += 1;
         field.remove(0, labelEnd);

         // Find the URI terminator
         uriEnd = field.index(">", labelEnd);

         // No URI terminator found, assume the whole thing
         if(uriEnd < 0)
         {
            uri->append(field.data());
         }
         // Remove the terminator
         else
         {
            field.remove(uriEnd);
            uri->append(field.data());
         }
      }
      // There is no label take the whole thing as the URI
      else
      {
         uri->append(field.data());
      }
   }
}

UtlBoolean SipMessage::getWarningCode(int* warningCode, int index) const
{
   const char* value = getHeaderValue(index, SIP_WARNING_FIELD);
   UtlString warningField;
   int endOfCode;

   *warningCode = 0;

   if(value)
   {
      warningField.append(value);
      endOfCode = warningField.index(SIP_SUBFIELD_SEPARATOR);
      if(endOfCode > 0)
      {
         warningField.remove(endOfCode);
         *warningCode = atoi(warningField.data());
      }
   }
   return(value != NULL);
}

UtlBoolean SipMessage::removeLastVia()
{
   //do not remove the whole via header line . Remove only the first subfield
   UtlBoolean fieldFound = FALSE;
   UtlString NewViaHeader;
   UtlString viaField;

   if ( getViaField( &viaField , 0))
   {
      unsigned int posSubField = viaField.first(SIP_MULTIFIELD_SEPARATOR);
      if (posSubField != UTL_NOT_FOUND)
      {
         viaField.remove(0, posSubField + strlen(SIP_MULTIFIELD_SEPARATOR));
         NewViaHeader = viaField.strip(UtlString::both , ' ');
      }
   }


   NameValuePair viaHeaderField(SIP_VIA_FIELD);

   //remove whole line
   NameValuePair* nv = (NameValuePair*) mNameValues.find(&viaHeaderField);
   if(nv)
   {
        mHeaderCacheClean = FALSE;
      mNameValues.destroy(nv);
      nv = NULL;
      fieldFound = TRUE;
   }
   //add updated line
   if ( !NewViaHeader.isNull())
   {
      addViaField( NewViaHeader);
   }
   return(fieldFound);
}

UtlBoolean SipMessage::getViaField(UtlString* viaField, int index) const
{
   const char* value = getHeaderValue(index, SIP_VIA_FIELD);

   viaField->remove(0);
   if(value)
   {
      viaField->append(value);
   }
   return(value != NULL);
}

UtlBoolean SipMessage::getViaFieldSubField(UtlString* viaSubField, int subFieldIndex) const
{
   UtlBoolean retVal = FALSE;
   UtlString Via;
   if (getFieldSubfield(SIP_VIA_FIELD, subFieldIndex, &Via) )
   {
      viaSubField->remove(0);
      if(!Via.isNull())
      {
         viaSubField->append(Via);
         retVal = TRUE;
      }
   }
   return retVal;
}

void SipMessage::getLastVia(UtlString* address,
                            int* port,
                            UtlString* protocol,
                            int* receivedPort,
                            UtlBoolean* receivedSet,
                            UtlBoolean* maddrSet,
                            UtlBoolean* receivePortSet) const
{
   UtlString Via;

   UtlString sipProtocol;
   UtlString url;
   UtlString receivedAddress;
   UtlString receivedPortString;
   UtlString maddr;
   int index;
   address->remove(0);
   *port = 0;
   protocol->remove(0);

   if (receivedSet)
   {
      *receivedSet = FALSE;
   }
   if (maddrSet)
   {
      *maddrSet = FALSE;
   }
   if (receivePortSet)
   {
      *receivePortSet = FALSE;
   }

   
   if (getFieldSubfield(SIP_VIA_FIELD, 0, &Via))
   {
      NameValueTokenizer::getSubField(Via, 0, SIP_SUBFIELD_SEPARATORS,
                                      &sipProtocol);
      NameValueTokenizer::getSubField(Via, 1, SIP_SUBFIELD_SEPARATORS,
                                      &url);

      index = sipProtocol.index('/');
      if(index >= 0)
      {
         sipProtocol.remove(0, index + 1);
         index = sipProtocol.index('/');
      }

      if(index >= 0)
      {
         sipProtocol.remove(0, index + 1);
      }
      protocol->remove(0);
      protocol->append(sipProtocol.data());

      Url viaParam(url,TRUE);
      viaParam.getHostAddress(*address);
      *port = viaParam.getHostPort();

      UtlBoolean receivedFound = viaParam.getUrlParameter("received",receivedAddress);
      UtlBoolean maddrFound = viaParam.getUrlParameter("maddr",maddr);
      UtlBoolean receivedPortFound = viaParam.getUrlParameter("rport",receivedPortString);

      // The maddr takes precidence over the received by address
      if(!maddr.isNull())
      {
         *address = maddr;
      }

      // The received address takes precidence over the sent-by address
      else if(!receivedAddress.isNull())
      {
         address->remove(0);
         address->append(receivedAddress.data());
      }

      if(receivedPort
         && !receivedPortString.isNull())
      {
         *receivedPort = atoi(receivedPortString.data());
      }
      else if(receivedPort)
      {
         *receivedPort = 0;
      }

      if(receivedSet)
      {
         *receivedSet = receivedFound;
      }
      if(maddrSet)
      {
         *maddrSet = maddrFound;
      }
      
      if(receivePortSet)
      {
         *receivePortSet = receivedPortFound;
      }
   }
}

UtlBoolean SipMessage::getViaTag(const char* viaField,
                                const char* tagName,
                                UtlString& tagValue)
{
    UtlString strNameValuePair;
    UtlBoolean tagFound = FALSE;
    UtlHashBag viaParameters;

    parseViaParameters(viaField,viaParameters);
    UtlString nameMatch(tagName);
    NameValuePair *pair = (NameValuePair *)viaParameters.find(&nameMatch);

    if (pair)
    {
        tagValue = pair->getValue();
        tagFound = TRUE;
    }
    else
        tagValue.remove(0);

    viaParameters.destroyAll();

    return(tagFound);
}


void SipMessage::getCallIdField(UtlString* callId) const
{
   const char* value = getHeaderValue(0, SIP_CALLID_FIELD);

   if(value)
   {
        *callId = value;
    }
    else
    {
        callId->remove(0);
    }
}

UtlBoolean SipMessage::getCSeqField(int* sequenceNum, UtlString* sequenceMethod) const
{
   const char* value = getHeaderValue(0, SIP_CSEQ_FIELD);
   if(value)
   {
        // Too slow:
       /*UtlString sequenceNumString;
      NameValueTokenizer::getSubField(value, 0,
               SIP_SUBFIELD_SEPARATORS, &sequenceNumString);
      *sequenceNum = atoi(sequenceNumString.data());

      NameValueTokenizer::getSubField(value, 1,
               SIP_SUBFIELD_SEPARATORS, sequenceMethod);*/

        // Ignore white space in the begining
        int valueStart = strspn(value, SIP_SUBFIELD_SEPARATORS);

        // Find the end of the sequence number
        int numStringLen = strcspn(&value[valueStart], SIP_SUBFIELD_SEPARATORS)
            - valueStart;

        // Get the method
        if(sequenceMethod)
        {
            *sequenceMethod = &value[numStringLen + valueStart];
            NameValueTokenizer::frontBackTrim(sequenceMethod, SIP_SUBFIELD_SEPARATORS);

            if(numStringLen > MAXIMUM_INTEGER_STRING_LENGTH)
            {
                osPrintf("WARNING: SipMessage::getCSeqField CSeq number %d characters: %s.\nTruncating to %d\n",
                    numStringLen, &value[valueStart], MAXIMUM_INTEGER_STRING_LENGTH);
                numStringLen = MAXIMUM_INTEGER_STRING_LENGTH;
            }
        }

        if(sequenceNum)
        {
            // Convert the sequence number
            char numBuf[MAXIMUM_INTEGER_STRING_LENGTH + 1];
            memcpy(numBuf, &value[valueStart], numStringLen);
            numBuf[numStringLen] = '\0';
            *sequenceNum = atoi(numBuf);
        }
   }
    else
    {
        if(sequenceNum)
        {
            *sequenceNum = -1;
        }

        if(sequenceMethod)
        {
            sequenceMethod->remove(0);
        }
    }

    return(value != NULL);
}

UtlBoolean SipMessage::getContactUri(int addressIndex, UtlString* uri) const
{
    UtlBoolean uriFound = getContactEntry(addressIndex, uri);
    if(uriFound)
    {
        int trimIndex = uri->index('<');
        if(trimIndex >= 0) uri->remove(0, trimIndex + 1);
        trimIndex = uri->index('>');
        if(trimIndex > 0) uri->remove(trimIndex);
    }
    return(uriFound);
}

UtlBoolean SipMessage::getContactField(int addressIndex, UtlString& contactField) const
{
    const char* value = getHeaderValue(addressIndex, SIP_CONTACT_FIELD);
    contactField = value ? value : "";

    return(value != NULL);
}


// Make sure that the getContactEntry does the right thing for


UtlBoolean SipMessage::getContactEntry(int addressIndex, UtlString* uriAndParameters) const
{
   // return(getFieldSubfield(SIP_CONTACT_FIELD, addressIndex, uriAndParameters));

   UtlBoolean contactFound = FALSE;
   int currentHeaderFieldValue = 0;
   int currentEntryValue = 0;
   const char* value = NULL;

   while ( (value = getHeaderValue(currentHeaderFieldValue, SIP_CONTACT_FIELD)) &&
      (currentEntryValue <= addressIndex) )
   {
      uriAndParameters->remove(0);
      if(value)
      {
            #ifdef TEST_PRINT
                    osPrintf("SipMessage::getContactEntry addressIndex: %d\n", addressIndex);
            #endif
           //NameValueTokenizer::getSubField(value, addressIndex, ",",
         //    uriAndParameters);
           int addressStart = 0;
           int addressCount = 0;
           int charIndex = 0;
           int doubleQuoteCount = 0;

           while(1)
           {
               if(value[charIndex] == '"')
               {
                   doubleQuoteCount++;
                  #ifdef TEST_PRINT
                                  osPrintf("SipMessage::getContactEntry doubleQuoteCount parity:%d",
                                      doubleQuoteCount % 2);
                  #endif
               }

               // We found a comma that is not in the middle of a quoted string
               if(   (value[charIndex] == ',' || value[charIndex] == '\0')
                  && !(doubleQuoteCount % 2)
                  )
               {
                   if(currentEntryValue == addressIndex)
                   {

                       uriAndParameters->append(&value[addressStart], charIndex - addressStart);
                        #ifdef TEST_PRINT
                                            osPrintf("SipMessage::getContactEntry found contact[%d] starting: %d ending: %d \"%s\"\n",
                                                addressIndex, addressStart, charIndex, uriAndParameters->data());
                        #endif
                       currentEntryValue ++;
                       contactFound = TRUE;
                       break;
                   }
                   currentEntryValue ++;
                   addressStart = charIndex + 1;
                   addressCount++;
               }

               if(value[charIndex] == '\0') break;
               charIndex++;
           }
      }
      currentHeaderFieldValue++;
   }
   return(contactFound);
}


UtlBoolean SipMessage::getContactAddress(int addressIndex,
                                        UtlString* contactAddress,
                                        int* contactPort,
                                        UtlString* protocol,
                                    UtlString* user,
                                        UtlString* userLabel) const
{
    UtlString uri;
    UtlBoolean foundUri = getContactUri(addressIndex, &uri);

    if(foundUri) parseAddressFromUri(uri.data(), contactAddress,
                  contactPort, protocol,
                  user,
                  userLabel);

    return(foundUri);
}

UtlBoolean SipMessage::getRequireExtension(int extensionIndex,
                                UtlString* extension) const
{
   return(getFieldSubfield(SIP_REQUIRE_FIELD, extensionIndex, extension));
}

void SipMessage::addRequireExtension(const char* extension)
{
    addHeaderField(SIP_REQUIRE_FIELD, extension);
}

UtlBoolean SipMessage::getEventField(UtlString& eventField) const
{
   const char* value = getHeaderValue(0, SIP_EVENT_FIELD);
    eventField.remove(0);

   if(value)
   {
      eventField.append(value);
   }

   return(value != NULL);
}

void SipMessage::setEventField(const char* eventField)
{
    setHeaderValue(SIP_EVENT_FIELD, eventField, 0);
}

UtlBoolean SipMessage::getExpiresField(int* expiresInSeconds) const
{
   const char* fieldValue = getHeaderValue(0, SIP_EXPIRES_FIELD);
   if(fieldValue)
   {
        UtlString subfieldText;
        NameValueTokenizer::getSubField(fieldValue, 1,
               " \t:;,", &subfieldText);

        //
        if(subfieldText.isNull())
        {
            *expiresInSeconds = atoi(fieldValue);
        }
        // If there is more than one token assume it is a text date
        else
        {
            long dateExpires = OsDateTime::convertHttpDateToEpoch(fieldValue);
            long dateSent = 0;
            // If the date was not set in the message
            if(!getDateField(&dateSent))
            {
#ifdef TEST
                osPrintf("Date field not set\n");
#endif

                // Assume date sent is now
                dateSent = OsDateTime::getSecsSinceEpoch();
            }
#ifdef TEST_PRINT
            osPrintf("Expires date: %ld\n", dateExpires);
            osPrintf("Current time: %ld\n", dateSent);
#endif

            *expiresInSeconds = dateExpires - dateSent;
        }
   }
   else
   {
      *expiresInSeconds = -1;
   }

   return(fieldValue != NULL);
}

UtlBoolean SipMessage::getRequestDispositionField(UtlString* dispositionField) const
{
   const char* value = getHeaderValue(0, SIP_REQUEST_DISPOSITION_FIELD);
    dispositionField->remove(0);

   if(value)
   {
      dispositionField->append(value);
   }

   return(value != NULL);
}

UtlBoolean SipMessage::getRequestDisposition(int tokenIndex,
                                UtlString* dispositionToken) const
{
   return(getFieldSubfield(SIP_REQUEST_DISPOSITION_FIELD, tokenIndex,
        dispositionToken));
}

UtlBoolean SipMessage::getRecordRouteField(int index, UtlString* recordRouteField) const
{
    const char* fieldValue = getHeaderValue(index, SIP_RECORD_ROUTE_FIELD);
    recordRouteField->remove(0);
    if(fieldValue) recordRouteField->append(fieldValue);
    return(fieldValue != NULL);
}

UtlBoolean SipMessage::getRecordRouteUri(int index, UtlString* recordRouteUri) const
{
    //UtlString recordRouteField;
    //UtlBoolean fieldExists = getRecordRouteField(&recordRouteField);
    //NameValueTokenizer::getSubField(recordRouteField.data(), index,
   //          ",", recordRouteUri);
    UtlBoolean fieldExists = getFieldSubfield(SIP_RECORD_ROUTE_FIELD, index, recordRouteUri);
    NameValueTokenizer::frontBackTrim(recordRouteUri, " \t");
    return(fieldExists && !recordRouteUri->isNull());
}

void SipMessage::setRecordRouteField(const char* recordRouteField,
                                     int index)
{
    setHeaderValue(SIP_RECORD_ROUTE_FIELD, recordRouteField, index);
}

void SipMessage::addRecordRouteUri(const char* recordRouteUri)
{
    UtlString recordRouteField;
    UtlString recordRouteUriString;

    /*int recordRouteIndex = 0;
    while(getRecordRouteField(recordRouteIndex, &recordRouteField))
    {
        recordRouteIndex++;
    }
    // Try to format the same as it came in either all the record route
    // URLs on the same line or a different field for each
    if(recordRouteIndex == 1)
    {
        recordRouteIndex--;
        getRecordRouteField(recordRouteIndex, &recordRouteField);
    }

    if(recordRouteUri && !recordRouteField.isNull())
    {
        recordRouteField.insert(0, ',');
    }*/
    if(recordRouteUri && strchr(recordRouteUri, '<') == NULL)
    {
        recordRouteUriString.append('<');
        recordRouteUriString.append(recordRouteUri);
        recordRouteUriString.append('>');
    }
    else if(recordRouteUri)
    {
        recordRouteUriString.append(recordRouteUri);
    }

    recordRouteField.insert(0, recordRouteUriString);
    //setRecordRouteField(recordRouteField.data(), recordRouteIndex);

    // Record route is always added on the top
    NameValuePair* headerField =
        new NameValuePair(SIP_RECORD_ROUTE_FIELD,
        recordRouteUriString.data());

    mHeaderCacheClean = FALSE;
   mNameValues.insertAt(0, headerField);
}

// isClientMsgStrictRouted returns whether or not a message
//    is set up such that it requires strict routing.
//    This is appropriate only when acting as a UAC
UtlBoolean SipMessage::isClientMsgStrictRouted() const
{
    UtlBoolean result;
    UtlString routeField;

    if ( getRouteField( &routeField ) )
    {
        Url routeUrl( routeField, TRUE );
        UtlString valueIgnored;

        // there is a route header, so see if it is loose routed or not
        result = ! routeUrl.getUrlParameter( "lr", valueIgnored );
    }
    else
    {
        result = FALSE;
    }

    return result;
}


UtlBoolean SipMessage::getRouteField(UtlString* routeField) const
{
    const char* fieldValue = getHeaderValue(0, SIP_ROUTE_FIELD);
    routeField->remove(0);
    if(fieldValue) routeField->append(fieldValue);
    return(fieldValue != NULL);
}

void SipMessage::addRouteUri(const char* routeUri)
{
   UtlString routeField;
   UtlString routeParameter;
    const char* bracketPtr = strchr(routeUri, '<');
   if(bracketPtr == NULL) routeParameter.append('<');
   routeParameter.append(routeUri);
    bracketPtr = strchr(routeUri, '>');
   if(bracketPtr == NULL) routeParameter.append('>');

    // If there is already a route header
   if(getRouteField( &routeField))
    {
      routeParameter.append(SIP_MULTIFIELD_SEPARATOR);

       // Remove the entire header
        removeHeader(SIP_ROUTE_FIELD, 0);
    }

   //add the updated header
   routeField.insert(0,routeParameter);
   setRouteField(routeField);
}

void SipMessage::addLastRouteUri(const char* routeUri)
{
    if(routeUri && *routeUri)
    {
        // THis is the cheap brut force way to do this
        int index = 0;
        const char* routeField = NULL;
        while ((routeField = getHeaderValue(index, SIP_ROUTE_FIELD)))
        {
            index++;
        }

        UtlString routeString(routeField ? routeField : "");
        if(routeField)
        {
            // Add a field separator
            routeString.append(SIP_MULTIFIELD_SEPARATOR);
        }
        // Make sure the route is in name-addr format
        if(strstr(routeUri,"<") <= 0)
        {
            routeString.append("<");
        }

        routeString.append(routeUri);
        if(strstr(routeUri, ">") <= 0)
        {
            routeString.append(">");
        }

        setHeaderValue(SIP_ROUTE_FIELD, routeString.data(), index);
    }
}

UtlBoolean SipMessage::getLastRouteUri(UtlString& routeUri,
                                      int& lastIndex)
{
    int index = 0;

    UtlString tempRoute;
    while(getFieldSubfield(SIP_ROUTE_FIELD, index, &tempRoute))
    {
        index++;
        routeUri = tempRoute;
    }

    index--;
    lastIndex = index;

    return(!routeUri.isNull());
}

UtlBoolean SipMessage::getRouteUri(int index, UtlString* routeUri) const
{
    UtlString routeField;
    UtlBoolean fieldExists = getFieldSubfield(SIP_ROUTE_FIELD, index, routeUri);
    return(fieldExists && !routeUri->isNull());
}

UtlBoolean SipMessage::removeRouteUri(int index, UtlString* routeUri)
{
    UtlString newRouteField;
    UtlString aRouteUri;
    UtlBoolean uriFound = FALSE;
    int uriIndex = 0;
    while(getFieldSubfield(SIP_ROUTE_FIELD, uriIndex, &aRouteUri))
    {
#ifdef TEST_PRINT
        osPrintf("removeRouteUri::routeUri[%d]: %s\n", uriIndex, newRouteField.data());
#endif
        if(uriIndex == index)
        {
            *routeUri = aRouteUri;
            uriFound = TRUE;
        }
        else
        {
            if(!newRouteField.isNull())
            {
                newRouteField.append(',');
            }
            int routeUriIndex = aRouteUri.index('<');
            if(routeUriIndex < 0)
            {
                aRouteUri.insert(0, '<');
                aRouteUri.append('>');
            }
            newRouteField.append(aRouteUri.data());
        }
#ifdef TEST_PRINT
        osPrintf("removeRouteUri::newRouteField: %s\n", newRouteField.data());
#endif
        uriIndex++;
    }

    // Remove all the old route headers.
    while(removeHeader(SIP_ROUTE_FIELD, 0))
    {
    }

    // Set the route field to contain the uri list with the indicated
    // uri removed.
    if(!newRouteField.isNull())
    {
        insertHeaderField(SIP_ROUTE_FIELD, newRouteField.data());
    }

    return(uriFound);
}

void SipMessage::setRouteField(const char* routeField)
{
#ifdef TEST_PRINT
    osPrintf("setRouteField: %s\n", routeField);
#endif
    setHeaderValue(SIP_ROUTE_FIELD, routeField, 0);
}

UtlBoolean SipMessage::buildRouteField(UtlString* routeFld) const
{
    UtlBoolean recordRouteFound = FALSE;
    UtlString contactUri;
   UtlString routeField;

    // If request build from recordRoute verbatum
    if(!isResponse())
    {
#ifdef TEST_PRINT
        osPrintf("SipMessage::buildRouteField recordRoute verbatum\n");
#endif
        //recordRouteFound = getRecordRouteField(routeField);
        int recordRouteIndex = 0;
        UtlString recordRouteUri;
        while(getRecordRouteUri(recordRouteIndex, &recordRouteUri))
        {
            if(!routeField.isNull())
            {
                routeField.append(',');
            }
            routeField.append(recordRouteUri.data());
#ifdef TEST_PRINT
            osPrintf("SipMessage::buildRouteField recordRouteUri[%d] %s\n",
                recordRouteIndex, recordRouteUri.data());
#endif
            recordRouteIndex++;
        }
        if(recordRouteIndex) recordRouteFound = TRUE;
    }

    // If response build from recordeRoute in reverse order
    else
    {
#ifdef TEST_PRINT
        osPrintf("SipMessage::buildRouteField recordRoute reverse\n");
#endif
        UtlString recordRouteUri;
        routeField.remove(0);
        int index = 0;
        int recordRouteUriIndex;
        while(getRecordRouteUri(index, &recordRouteUri))
        {
            recordRouteFound = TRUE;
            if(index > 0)
            {
                routeField.insert(0, ", ");
            }

            recordRouteUriIndex = recordRouteUri.index('<');
            if(recordRouteUriIndex < 0)
            {
                recordRouteUri.insert(0, '<');
                recordRouteUri.append('>');
            }

#ifdef TEST_PRINT
            osPrintf("SipMessage::buildRouteField recordRouteUri[%d] %s\n",
                index, recordRouteUri.data());
#endif
            routeField.insert(0, recordRouteUri.data());
            index++;
        }
    }

#ifdef LOOSE_ROUTE
    // In either case if contact is present add to the end of the route list.
    if(recordRouteFound && getContactUri(0, &contactUri))
    {
        routeField.append(", ");
        int contactUriIndex = contactUri.index('<');
        if(contactUriIndex < 0)
        {
            contactUri.insert(0, '<');
            contactUri.append('>');
        }
        routeField.append(contactUri.data());
    }
#endif

#ifdef TEST_PRINT
    osPrintf("buildRouteField: %s\n", routeField.data());
#endif

   if (recordRouteFound)
   {
      //clear the previous recourd route field and set it to the new one
      routeFld->remove(0);
      routeFld->append(routeField);
   }
    return(recordRouteFound);
}

void SipMessage::buildReplacesField(UtlString& replacesField,
                            const char* callId,
                            const char* fromField,
                            const char* toField)
{
    replacesField = callId;

    replacesField.append(";to-tag=");
    Url toUrl(toField);
    UtlString toTag;
    toUrl.getFieldParameter("tag", toTag);
    replacesField.append(toTag);

    replacesField.append(";from-tag=");
    Url fromUrl(fromField);
    UtlString fromTag;
    fromUrl.getFieldParameter("tag", fromTag);
    replacesField.append(fromTag);
}

UtlBoolean SipMessage::getFieldSubfield(const char* fieldName, int addressIndex, UtlString* uri) const
{
   UtlBoolean uriFound = FALSE;
   UtlString url;
   int fieldIndex = 0;
   int subFieldIndex = 0;
   int index = 0;
   const char* value = getHeaderValue(fieldIndex, fieldName);

   uri->remove(0);

   while(value && index <= addressIndex)
   {
      subFieldIndex = 0;
      NameValueTokenizer::getSubField(value, subFieldIndex, SIP_MULTIFIELD_SEPARATOR, &url);
#ifdef TEST
      osPrintf("Got field: \"%s\" subfield[%d]: %s\n", fieldName, fieldIndex, url.data());
#endif

      while(!url.isNull() && index < addressIndex)
      {
         subFieldIndex++;
         index++;
         NameValueTokenizer::getSubField(value, subFieldIndex,
            SIP_MULTIFIELD_SEPARATOR, &url);
#ifdef TEST
         osPrintf("Got field: \"%s\" subfield[%d]: %s\n", fieldName, fieldIndex, url.data());
#endif
      }

      if(index == addressIndex && !url.isNull())
      {
         uri->append(url.data());
         uriFound = TRUE;
            break;
      }
      else if(index > addressIndex)
      {
         break;
      }

      fieldIndex++;
      value = getHeaderValue(fieldIndex, fieldName);
   }
   return(uriFound);
}

/*UtlBoolean SipMessage::setFieldSubfield(const char* fieldName,
                              int addressIndex, const char* subFieldValue) const
{
   UtlBoolean uriFound = FALSE;
   UtlString url;
   int fieldIndex = 0;
   int subFieldIndex = 0;
   int index = 0;
   const char* value = getHeaderValue(fieldIndex, fieldName);

   uri->remove(0);

   while(value && index <= addressIndex)
   {
      subFieldIndex = 0;
      NameValueTokenizer::getSubField(value, subFieldIndex,
         SIP_MULTIFIELD_SEPARATOR, &url);
#ifdef TEST
      osPrintf("Got field: \"%s\" subfield[%d]: %s\n", fieldName,
         fieldIndex, url.data());
#endif

      while(!url.isNull() && index < addressIndex)
      {
         subFieldIndex++;
         index++;
         NameValueTokenizer::getSubField(value, subFieldIndex,
            SIP_MULTIFIELD_SEPARATOR, &url);
#ifdef TEST
         osPrintf("Got field: \"%s\" subfield[%d]: %s\n", fieldName,
            fieldIndex, url.data());
#endif
      }

      if(index == addressIndex && !url.isNull())
      {
         url.remove(0);
         url.append(subFieldValue.data());

         uriFound = TRUE;
            break;
      }
      else if(index > addressIndex)
      {
         break;
      }

      fieldIndex++;
      value = getHeaderValue(fieldIndex, fieldName);
   }

   return(uriFound);
}
*/

UtlBoolean SipMessage::getContentEncodingField(UtlString* contentEncodingField) const
{
   const char* value = getHeaderValue(0, SIP_CONTENT_ENCODING_FIELD);

   contentEncodingField->remove(0);
   if(value)
   {
      contentEncodingField->append(value);
   }
    return(value != NULL);
}

UtlBoolean SipMessage::getSessionExpires(int* sessionExpiresSeconds) const
{
    const char* value = getHeaderValue(0, SIP_SESSION_EXPIRES_FIELD);

   if(value)
   {
      *sessionExpiresSeconds = atoi(value);
   }
    else
    {
        *sessionExpiresSeconds = 0;
    }
    return(value != NULL);
}

void SipMessage::setSessionExpires(int sessionExpiresSeconds)
{
   char numString[HTTP_LONG_INT_CHARS];

   sprintf(numString, "%d", sessionExpiresSeconds);
    setHeaderValue(SIP_SESSION_EXPIRES_FIELD, numString);
}

UtlBoolean SipMessage::getSupportedField(UtlString& supportedField) const
{
    return(getFieldSubfield(SIP_SUPPORTED_FIELD, 0, &supportedField));
}

void SipMessage::setSupportedField(const char* supportedField)
{
    setHeaderValue(SIP_SUPPORTED_FIELD, supportedField);
}

// Call control accessors
UtlBoolean SipMessage::getAlsoUri(int index, UtlString* alsoUri) const
{
    return(getFieldSubfield(SIP_ALSO_FIELD, index, alsoUri));
}

UtlBoolean SipMessage::getAlsoField(UtlString* alsoField) const
{
    const char* value = getHeaderValue(0, SIP_ALSO_FIELD);
    alsoField->remove(0);
    if(value) alsoField->append(value);
    return(value != NULL);
}

void SipMessage::setAlsoField(const char* alsoField)
{
    setHeaderValue(SIP_ALSO_FIELD, alsoField);
}

void SipMessage::addAlsoUri(const char* alsoUri)
{
    // Append to the field value already there, if it exists
    UtlString field;
    if(getAlsoField(&field) && !field.isNull())
    {
        field.append(SIP_MULTIFIELD_SEPARATOR);
      field.append(SIP_SINGLE_SPACE);
    }

    if(!strchr(alsoUri, '<')) field.append('<');
    field.append(alsoUri);
    if(!strchr(alsoUri, '>')) field.append('>');
    setAlsoField(field.data());
}

void SipMessage::setRequestedByField(const char* requestedByUri)
{
    setHeaderValue(SIP_REQUESTED_BY_FIELD, requestedByUri);
}

UtlBoolean SipMessage::getRequestedByField(UtlString& requestedByField) const
{
    const char* value = getHeaderValue(0, SIP_REQUESTED_BY_FIELD);
    requestedByField.remove(0);
    if(value) requestedByField.append(value);
    return(value != NULL);
}

void SipMessage::setReferToField(const char* referToField)
{
    setHeaderValue(SIP_REFER_TO_FIELD, referToField);
}

UtlBoolean SipMessage::getReferToField(UtlString& referToField) const
{
    const char* value = getHeaderValue(0, SIP_REFER_TO_FIELD);
    referToField.remove(0);
    if(value) referToField.append(value);
    return(value != NULL);
}

void SipMessage::setReferredByField(const char* referredByField)
{
    setHeaderValue(SIP_REFERRED_BY_FIELD, referredByField);
}

UtlBoolean SipMessage::getReferredByField(UtlString& referredByField) const
{
    const char* value = getHeaderValue(0, SIP_REFERRED_BY_FIELD);
    referredByField.remove(0);
    if(value) referredByField.append(value);
    return(value != NULL);
}

UtlBoolean SipMessage::getReferredByUrls(UtlString* referrerUrl,
                                        UtlString* referredToUrl) const
{
    if(referrerUrl) referrerUrl->remove(0);
    if(referredToUrl) referredToUrl->remove(0);
    const char* value = getHeaderValue(0, SIP_REFERRED_BY_FIELD);
    if(value)
    {
        // The first element is the referrer URL
        if(referrerUrl) NameValueTokenizer::getSubField(value, 0,
            ";", referrerUrl);

        // The second element is the referred to URL
        if(referredToUrl) NameValueTokenizer::getSubField(value, 1,
            ";", referredToUrl);
    }
    return(value != NULL);
}

UtlBoolean SipMessage::getReplacesData(UtlString& callId,
                                      UtlString& toTag,
                                      UtlString& fromTag) const
{
    const char* replacesField = getHeaderValue(0, SIP_REPLACES_FIELD);

    UtlString parameter;
    UtlString name;
    UtlString value("");
    int parameterIndex = 1;

   if (replacesField)
    {
        // Get the callId
       NameValueTokenizer::getSubField(replacesField, 0,
                   ";", &callId);
       NameValueTokenizer::frontBackTrim(&callId, " \t");

       // Look through the rest of the parameters
       do
       {
          // Get a name value pair
          NameValueTokenizer::getSubField(replacesField, parameterIndex,
                   ";", &parameter);


          // Parse out the parameter name
          NameValueTokenizer::getSubField(parameter.data(), 0,
                                  "=", &name);
          name.toLower();
          NameValueTokenizer::frontBackTrim(&name, " \t");

          // Parse out the parameter value
          NameValueTokenizer::getSubField(parameter.data(), 1,
                                  "=", &value);
          NameValueTokenizer::frontBackTrim(&value, " \t");

          // Set the to and from tags when we find them
          if(name.compareTo("to-tag") == 0)
          {
             toTag = value;
          }
          else if(name.compareTo("from-tag") == 0)
          {
             fromTag = value;
          }

          parameterIndex++;
       } while(!parameter.isNull());
    }

    return (replacesField != NULL) ;
}

void SipMessage::setAllowField(const char* allowField)
{
    setHeaderValue(SIP_ALLOW_FIELD, allowField);
}

UtlBoolean SipMessage::getAllowField(UtlString& allowField) const
{
    const char* value;
    int allowIndex = 0;
    allowField.remove(0);
    while ((value = getHeaderValue(allowIndex, SIP_ALLOW_FIELD)))
    {
        if(value && *value)
        {
            if(!allowField.isNull()) allowField.append(", ");
            allowField.append(value);
        }
        allowIndex++;
    }
    return(value != NULL);
}
/* ============================ INQUIRY =================================== */

UtlBoolean SipMessage::isResponse() const
{
   UtlBoolean responseType = FALSE;
   //UtlString firstHeaderField;

   //getFirstHeaderLinePart(0, &firstHeaderField);
   if(mFirstHeaderLine.index(SIP_PROTOCOL_VERSION) == 0)
   {
      responseType = TRUE;
   }

   return(responseType);
}

UtlBoolean SipMessage::isServerTransaction(UtlBoolean isOutgoing) const
{
    UtlBoolean returnCode;

    if(isResponse())
    {
        if(isOutgoing)
        {
            returnCode = TRUE;
        }
        else
        {
            returnCode = FALSE;
        }
    }
    else
    {
        if(isOutgoing)
        {
            returnCode = FALSE;
        }
        else
        {
            returnCode = TRUE;
        }
    }

    return(returnCode);
}

UtlBoolean SipMessage::isSameMessage(const SipMessage* message,
                           UtlBoolean responseCodesMustMatch) const
{
   UtlBoolean isSame = FALSE;
   UtlString thisMethod, thatMethod;
   int thisSequenceNum, thatSequenceNum;
   UtlString thisSequenceMethod, thatSequenceMethod;

   if(message)
   {
      // Compare the method, To, From, CallId, Sequence number and
      // sequence method
      UtlBoolean thatIsResponse = message->isResponse();
      UtlBoolean thisIsResponse = isResponse();
      int thisResponseCode = 38743;
      int thatResponseCode = 49276;

      // Both are responses or requests
      if(thatIsResponse == thisIsResponse)
      {
         if(!thisIsResponse)
         {
            getRequestMethod(&thisMethod);
            message->getRequestMethod(&thatMethod);
         }
         else
         {
            thisResponseCode = getResponseStatusCode();
            thatResponseCode = message->getResponseStatusCode();
         }
         if( (thisIsResponse && !responseCodesMustMatch) ||
            (thisIsResponse && responseCodesMustMatch &&
               thisResponseCode == thatResponseCode) ||
            (!thisIsResponse && thisMethod.compareTo(thatMethod) == 0))
         {
            if(isSameSession(message))
            {
               getCSeqField(&thisSequenceNum, &thisSequenceMethod);
               message->getCSeqField(&thatSequenceNum, &thatSequenceMethod);
               if(thisSequenceNum == thatSequenceNum &&
                  thisSequenceMethod.compareTo(thatSequenceMethod) == 0)
               {
                  isSame = TRUE;
               }

            }
         }
      }
   }

   return(isSame);
}

UtlBoolean SipMessage::isSameSession(const SipMessage* message) const
{
   UtlBoolean isSame = FALSE;
    UtlBoolean isSameFrom = FALSE;
   UtlString thisTo, thatTo;
   UtlString thisFrom, thatFrom;
   UtlString thisCallId, thatCallId;

   // Messages from the same session have the same To, From and CallId
   if(message)
   {
      getCallIdField(&thisCallId);
      message->getCallIdField(&thatCallId);
      if(thisCallId.compareTo(thatCallId) == 0)
      {
         getFromField(&thisFrom);
         message->getFromField(&thatFrom);
         if(thisFrom.compareTo(thatFrom) == 0)
         {
                isSameFrom = TRUE;

            }
            else
            {
                UtlString thisAddress;
            UtlString thatAddress;
            int thisPort;
            int thatPort;
            UtlString thisProtocol;
            UtlString thatProtocol;
            UtlString thisUser;
            UtlString thatUser;
            UtlString thisUserLabel;
            UtlString thatUserLabel;
            UtlString thisTag;
            UtlString thatTag;
            getFromAddress(&thisAddress, &thisPort, &thisProtocol,
               &thisUser, &thisUserLabel, &thisTag);
            message->getFromAddress(&thatAddress, &thatPort, &thatProtocol,
               &thatUser, &thatUserLabel, &thatTag);

            if(thisAddress.compareTo(thatAddress) == 0 &&
            (thisPort == thatPort ||
               (thisPort == 0 && thatPort == SIP_PORT) ||
                  (thisPort == SIP_PORT && thatPort == 0)) &&
            thisProtocol.compareTo(thatProtocol) == 0 &&
            thisUser.compareTo(thatUser) == 0 &&
            (thisTag.compareTo(thatTag , UtlString::ignoreCase) == 0 ||
            (thisTag.isNull() && !isResponse()) ||
            (thatTag.isNull() && !message->isResponse())))
            {
               isSameFrom = TRUE;
            }
                else
                {
#ifdef TEST_PRINT
                    osPrintf("ERROR: From field did not match: \nAddr: (%s!=%s)\nPort: %d!=%d\nUser: (%s!=%s)\nTag:  (%s!=%s)\n",
                        thisAddress.data(), thatAddress.data(),
                        thisPort, thatPort,
                        thisUser.data(), thatUser.data(),
                        thisTag.data(), thatTag.data());
#endif
                }
            }

         getToField(&thisTo);
         message->getToField(&thatTo);
         if(isSameFrom && thisTo.compareTo(thatTo ) == 0)
         {
            isSame = TRUE;
         }
         // Check for tag
         else if(isSameFrom)
         {
            UtlString thisAddress;
            UtlString thatAddress;
            int thisPort;
            int thatPort;
            UtlString thisProtocol;
            UtlString thatProtocol;
            UtlString thisUser;
            UtlString thatUser;
            UtlString thisUserLabel;
            UtlString thatUserLabel;
            UtlString thisTag;
            UtlString thatTag;
            getToAddress(&thisAddress, &thisPort, &thisProtocol,
               &thisUser, &thisUserLabel, &thisTag);
            message->getToAddress(&thatAddress, &thatPort, &thatProtocol,
               &thatUser, &thatUserLabel, &thatTag);

            // Everything must match with the exception that the
            // request may not have the tag set.
            if(thisAddress.compareTo(thatAddress) == 0 &&
            (thisPort == thatPort ||
               (thisPort == 0 && thatPort == SIP_PORT) ||
                  (thisPort == SIP_PORT && thatPort == 0)) &&
            thisProtocol.compareTo(thatProtocol) == 0 &&
            thisUser.compareTo(thatUser) == 0 &&
            (thisTag.compareTo(thatTag , UtlString::ignoreCase) == 0 ||
            (thisTag.isNull() && !isResponse()) ||
            (thatTag.isNull() && !message->isResponse())))

            {
               isSame = TRUE;
            }
                else
                {
#ifdef TEST_PRINT
                    osPrintf("ERROR: To field did not match:\n: (%s!=%s)\nPort: %d!=%d\nUser: (%s!=%s)\nTag:  (%s!=%s)\n",
                        thisAddress.data(), thatAddress.data(),
                        thisPort, thatPort,
                        thisUser.data(), thatUser.data(),
                        thisTag.data(), thatTag.data());
#endif
                }
         }
      }
   }
   return(isSame);
}

UtlBoolean SipMessage::isSameSession(Url& oldUrl, Url& newUrl)
{
   UtlBoolean isSame = FALSE;


    UtlString thisAddress;
   UtlString thatAddress;
   int thisPort;
   int thatPort;
   UtlString thisProtocol;
   UtlString thatProtocol;
   UtlString thisUser;
   UtlString thatUser;
   UtlString thisTag;
   UtlString thatTag;
    oldUrl.getHostAddress(thisAddress);
    newUrl.getHostAddress(thatAddress);
    thisPort = oldUrl.getHostPort();
    thatPort = newUrl.getHostPort();
    oldUrl.getUserId(thisUser);
    newUrl.getUserId(thatUser);
    oldUrl.getUrlParameter("transport", thisProtocol);
    newUrl.getUrlParameter("transport", thatProtocol);
    oldUrl.getFieldParameter("tag", thisTag);
    newUrl.getFieldParameter("tag", thatTag);

   if(thisAddress.compareTo(thatAddress) == 0 &&
   (thisPort == thatPort ||
      (thisPort == 0 && thatPort == SIP_PORT) ||
         (thisPort == SIP_PORT && thatPort == 0)) &&
   thisProtocol.compareTo(thatProtocol) == 0 &&
   thisUser.compareTo(thatUser) == 0 &&
   (thisTag.compareTo(thatTag , UtlString::ignoreCase) == 0 || thisTag.isNull()))
         // Allow the old tag to be NULL
            // We do not allow only the new tag to be NULL as
            // this will cause some false matches.  Both may be NULL.*/
   {
      isSame = TRUE;
   }
    else
    {
#ifdef TEST_PRINT
        osPrintf("SipMessage::isSameSession Url did not match: \nAddr: (%s!=%s)\nPort: %d!=%d\nUser: (%s!=%s)\nTag:  (%s!=%s)\n",
            thisAddress.data(), thatAddress.data(),
            thisPort, thatPort,
            thisUser.data(), thatUser.data(),
            thisTag.data(), thatTag.data());
#endif
    }

   return(isSame);
}

UtlBoolean SipMessage::isResponseTo(const SipMessage* request) const
{
   UtlBoolean isPair = FALSE;
   UtlString thisMethod, thatMethod;
   int thisSequenceNum, thatSequenceNum;
   UtlString thisSequenceMethod, thatSequenceMethod;

   // If this is a response and request is a request
   if(request && !request->isResponse() && isResponse())
   {
      // Compare the To, From, CallId, Sequence number and
      // sequence method
      if(isSameSession(request))
      {
         getCSeqField(&thisSequenceNum, &thisSequenceMethod);
         request->getCSeqField(&thatSequenceNum, &thatSequenceMethod);
         if(thisSequenceNum == thatSequenceNum &&
            thisSequenceMethod.compareTo(thatSequenceMethod) == 0)
         {
            isPair = TRUE;
         }

      }

   }

   return(isPair);
}

UtlBoolean SipMessage::isAckFor(SipMessage* inviteResponse) const
{
   UtlBoolean isPair = FALSE;
   UtlString thisMethod;
   int thisSequenceNum, thatSequenceNum;
   UtlString thisSequenceMethod, thatSequenceMethod;

   // If this is an ACK request and that is an INVITE response
   if(inviteResponse && inviteResponse->isResponse() && !isResponse())
   {
      getRequestMethod(&thisMethod);
      // Compare the To, From, CallId, Sequence number and  sequence method
      if(thisMethod.compareTo(SIP_ACK_METHOD) == 0 && isSameSession(inviteResponse))
      {
         getCSeqField(&thisSequenceNum, &thisSequenceMethod);
         inviteResponse->getCSeqField(&thatSequenceNum, &thatSequenceMethod);
         if(thisSequenceNum == thatSequenceNum &&
            thatSequenceMethod.compareTo(SIP_INVITE_METHOD) == 0)
         {
            isPair = TRUE;
         }
      }
   }

   return(isPair);
}
//SDUA
UtlBoolean SipMessage::isInviteFor(SipMessage* cancelRequest) const
{
   UtlBoolean isPair = FALSE;
   UtlString thisMethod;
   // If this is an CANCEL request and that is an INVITE response
   if(cancelRequest && !isResponse())
   {
      getRequestMethod(&thisMethod);
      // Compare the To, From, CallId, Sequence number and  sequence method
      if(thisMethod.compareTo( SIP_INVITE_METHOD) == 0 && isSameTransaction(cancelRequest))
         isPair = TRUE;
   }
   return(isPair);
}

UtlBoolean SipMessage::isSameTransaction(const SipMessage* message) const
{
   // Compare the To, From, CallId, Sequence number and  sequence method
   UtlBoolean isPair = FALSE;
   int thisSequenceNum, thatSequenceNum;
   UtlString thisSequenceMethod, thatSequenceMethod;

   if( isSameSession(message))
   {
      getCSeqField(&thisSequenceNum, &thisSequenceMethod);
      message->getCSeqField(&thatSequenceNum, &thatSequenceMethod);
      if(thisSequenceNum == thatSequenceNum )
      {
         isPair = TRUE;
      }
   }
   return(isPair);
}


UtlBoolean SipMessage::isRequestDispositionSet(const char* dispositionToken) const
{
    UtlString field;
    int tokenIndex = 0;
    UtlBoolean matchFound = FALSE;
    while(getRequestDisposition(tokenIndex, &field))
    {
        field.toUpper();
        if(field.compareTo(dispositionToken) == 0)
        {
            matchFound = TRUE;
            break;
        }
    }

    return(matchFound);
}

UtlBoolean SipMessage::isRequireExtensionSet(const char* extension) const
{
    UtlString extensionString;
    UtlBoolean alreadySet = FALSE;
    int extensionIndex = 0;
    while(getRequireExtension(extensionIndex, &extensionString))
    {
        extensionString.toLower();
        if(extensionString.compareTo(extension) == 0)
        {
            alreadySet = TRUE;
        }

    }
    return(alreadySet);
}

UtlBoolean SipMessage::isUrlHeaderAllowed(const char* headerFieldName)
{
    UtlString name(headerFieldName);
    name.toUpper();
    UtlString nameCollectable(name);

    return(NULL == disallowedUrlHeaders.find(&nameCollectable));
}

//SDUA
UtlBoolean SipMessage::getDNSField( UtlString * Protocol , UtlString * Address, UtlString * Port) const
{

   //protocol can be empty by default
   if( !m_dnsAddress.isNull() && !m_dnsPort.isNull())
   {
      Protocol->remove(0);
      Address->remove(0);
      Port->remove(0);

      Protocol->append(m_dnsProtocol);
      Address->append(m_dnsAddress);
      Port->append(m_dnsPort);
      return (true);
   }
   else
   {
      return (false);
   }
}

void SipMessage::setDNSField( const char* Protocol , const char* Address, const char* Port)
{
   m_dnsProtocol.remove(0);
   m_dnsAddress.remove(0);
   m_dnsPort.remove(0);

   m_dnsProtocol.append(Protocol);
   m_dnsAddress.append(Address);
   m_dnsPort.append(Port);
}

void SipMessage::clearDNSField()
{
   m_dnsProtocol.remove(0);
   m_dnsAddress.remove(0);
   m_dnsPort.remove(0);
}

void SipMessage::setTransaction(SipTransaction* transaction)
{
    mpSipTransaction = transaction;
}

SipTransaction* SipMessage::getSipTransaction() const
{
    return(mpSipTransaction);
}

void SipMessage::ParseContactFields(const SipMessage *registerResponse,
                                    const SipMessage *SipRequest,
                                    const UtlString &subField,
                                    int& subFieldRetVal)
{
   //get the request contact value ...so that we can find out the expires subfield value
   // for this contact from the list of contacts returned byt the Rgister server
   UtlString RequestContactValue;
   SipRequest->getContactEntry(0 , &RequestContactValue);

   UtlString contactField;
   int indexContactField = 0;

   while (registerResponse->getContactEntry(indexContactField , &contactField))
   {
      if ( strstr(contactField, RequestContactValue ) != NULL)
      {
         UtlString subfieldText;
         int subfieldIndex = 0;
         UtlString subfieldName;
         UtlString subfieldValue;
         NameValueTokenizer::getSubField(contactField.data(), subfieldIndex, ";", &subfieldText);
         while(!subfieldText.isNull())
         {
            NameValueTokenizer::getSubField(subfieldText.data(), 0, "=", &subfieldName);
            NameValueTokenizer::getSubField(subfieldText.data(), 1, "=", &subfieldValue);
#               ifdef TEST_PRINT
            osPrintf("ipMessage::ParseContactFields found contact parameter[%d]: \"%s\" value: \"%s\"\n",
               subfieldIndex, subfieldName.data(), subfieldValue.data());
#               endif
            subfieldName.toUpper();
            if(subfieldName.compareTo(subField, UtlString::ignoreCase) == 0 &&
               subField.compareTo(SIP_EXPIRES_FIELD, UtlString::ignoreCase)== 0)
            {

               //see if more than one token in the expire value
               NameValueTokenizer::getSubField(subfieldValue, 1,
               " \t:;,", &subfieldText);

               // if not ...time is in seconds
               if(subfieldText.isNull())
               {
                  subFieldRetVal = atoi(subfieldValue);
               }
               // If there is more than one token assume it is a text date
               else
               {
                  // Get the expiration date
                  long dateExpires = OsDateTime::convertHttpDateToEpoch(subfieldValue);
                  long dateSent = 0;
                  // If the date was not set in the message
                  if(!registerResponse->getDateField(&dateSent))
                  {
                     #ifdef TEST_PRINT
                     osPrintf("Date field not set\n");
                     #endif
                     // Assume date sent is now
                     dateSent = OsDateTime::getSecsSinceEpoch();
                  }
                  #ifdef TEST_PRINT
                  osPrintf("Contact expires date: %ld\n", dateExpires); osPrintf("Current time: %ld\n", dateSent);
                  #endif
                  subFieldRetVal = dateExpires - dateSent;
               }
               break;
            }//any other field
            else if(subfieldName.compareTo(subField, UtlString::ignoreCase) == 0)
            {
               subFieldRetVal = atoi(subfieldValue);
            }

            subfieldIndex++;
            NameValueTokenizer::getSubField(contactField.data(), subfieldIndex, ";", &subfieldText);
         }
      }
      indexContactField ++;
   }
   return ;

}

/// Get the name/value pairs for a Via field
///
///
void SipMessage::parseViaParameters( const char* viaField
                                    ,UtlContainer& viaParamList
                                    )

{
    const char* pairSeparator = ";";
    const char* namValueSeparator = "=";

    const char* nameAndValuePtr;
    int nameAndValueLength;
    const char* namePtr;
    int nameLength;
    int nameValueIndex = 0;
   UtlString value;
    int lastCharIndex = 0;
    int relativeIndex;
    int nameValueRelativeIndex;
    int viaFieldLength = strlen(viaField);

    do
    {
#       ifdef  TEST_PRINT
        osPrintf("SipMessage::parseViaParameters: \"%s\" lastCharIndex: %d",
                 &(viaField[lastCharIndex]), lastCharIndex);
#       endif
        // Pull out a name value pair
        NameValueTokenizer::getSubField(&(viaField[lastCharIndex]),
                                        viaFieldLength - lastCharIndex,
                                        0,
                                        pairSeparator,
                                        nameAndValuePtr,
                                        nameAndValueLength,
                                        &relativeIndex);
        lastCharIndex += relativeIndex;

        if(nameAndValuePtr && nameAndValueLength > 0)
        {
            // Separate the name and value
            NameValueTokenizer::getSubField(nameAndValuePtr,
                                            nameAndValueLength,
                                            0,
                                            namValueSeparator,
                                            namePtr,
                                            nameLength,
                                            &nameValueRelativeIndex);

            // Get rid of leading white space in the name
            while(nameLength > 0 &&
                  (*namePtr == ' ' ||
                   *namePtr == '\t'))
            {
                nameLength--;
                namePtr++;
            }

            if(nameLength > 0)
            {
                int valueSeparatorOffset = strspn(&(namePtr[nameLength]),
                                                  namValueSeparator);
                const char* valuePtr = &(namePtr[nameLength]) + valueSeparatorOffset;
                int valueLength = nameAndValueLength -
                    (valuePtr - nameAndValuePtr);

                // If there is a value
                if(valueSeparatorOffset <= 0 ||
                   *valuePtr == '\0' ||
                   valueLength <= 0)
                {
                    valuePtr = NULL;
                    valueLength = 0;
                }

                NameValuePair* newNvPair = new NameValuePair("");
                newNvPair->append(namePtr, nameLength);
                if(valuePtr)
                {
                    value.remove(0);
                    value.append(valuePtr, valueLength);
                    NameValueTokenizer::frontBackTrim(&value, " \t\n\r");
                    newNvPair->setValue(value);
                }
                else
                {
                    newNvPair->setValue("");
                }

                NameValueTokenizer::frontBackTrim(newNvPair, " \t\n\r");

                // Add a name, value pair to the list
                viaParamList.insert(newNvPair);

                nameValueIndex++;
            }
        }
    } while(   nameAndValuePtr
            && nameAndValueLength > 0
            && viaField[lastCharIndex] != '\0'
            );
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */




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

// APPLICATION INCLUDES
#include <net/SipMessageEvent.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
SipMessageEvent::SipMessageEvent(SipMessage* message, int status) :
OsMsg(OsMsg::PHONE_APP, SipMessage::NET_SIP_MESSAGE)
{
   messageStatus = status;
   sipMessage = message;
}

// Copy constructor
SipMessageEvent::SipMessageEvent(const SipMessageEvent& rSipMessageEvent):
OsMsg(OsMsg::PHONE_APP, SipMessage::NET_SIP_MESSAGE)
{
        messageStatus = rSipMessageEvent.messageStatus;
}

// Destructor
SipMessageEvent::~SipMessageEvent()
{
        if(sipMessage)
        {
                delete sipMessage;
                sipMessage = NULL;
        }
}

OsMsg* SipMessageEvent::createCopy() const
{
        // Ineffient but easy coding way to copy message
        SipMessage* sipMsg = NULL;

        if(sipMessage)
        {
                sipMsg = new SipMessage(*sipMessage);
        }

        return(new SipMessageEvent(sipMsg, messageStatus));
}
/* ============================ MANIPULATORS ============================== */

// Assignment operator
SipMessageEvent&
SipMessageEvent::operator=(const SipMessageEvent& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;

   OsMsg::operator=(rhs);
        messageStatus = rhs.messageStatus;
        if(sipMessage)
        {
                delete sipMessage;
                sipMessage = NULL;
        }

        if(rhs.sipMessage)
        {
                sipMessage = new SipMessage(*(rhs.sipMessage));
        }

   return *this;
}

/* ============================ ACCESSORS ================================= */

const SipMessage* SipMessageEvent::getMessage()
{
        return(sipMessage);
}

void SipMessageEvent::setMessageStatus(int status)
{
        messageStatus = status;
}

int SipMessageEvent::getMessageStatus() const
{
        return(messageStatus);
}

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

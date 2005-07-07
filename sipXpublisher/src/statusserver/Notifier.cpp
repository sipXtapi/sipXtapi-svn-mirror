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
#include "os/OsStatus.h"
#include "os/OsDateTime.h"
#include "net/SipUserAgent.h"
#include "sipdb/SIPDBManager.h"
#include "sipdb/ResultSet.h"
#include "sipdb/SubscriptionDB.h"
#include "statusserver/Notifier.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
// STATIC INITIALIZERS
UtlString Notifier::sUriKey ("uri");
UtlString Notifier::sCallidKey ("callid");
UtlString Notifier::sContactKey ("contact");
UtlString Notifier::sExpiresKey ("expires");
UtlString Notifier::sSubscribecseqKey ("subscribecseq");
UtlString Notifier::sEventtypeKey ("eventtype");
UtlString Notifier::sToKey ("to");
UtlString Notifier::sFromKey ("from");
UtlString Notifier::sFileKey ("file");
UtlString Notifier::sKeyKey ("key");
UtlString Notifier::sRecordrouteKey ("recordroute");
UtlString Notifier::sNotifycseqKey ("notifycseq");

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Notifier::Notifier(SipUserAgent* sipUserAgent)
{
    mpSipUserAgent = sipUserAgent;
    mpSubscriptionDB = SubscriptionDB::getInstance();
}

Notifier::~Notifier()
{
    if( mpSipUserAgent )
    {
        mpSipUserAgent = NULL;
    }
}

/* ============================ PUBLIC ==================================== */

SipUserAgent* Notifier::getUserAgent()
{
    return(mpSipUserAgent);
}

void
Notifier::sendNotifyForSubscription (
    const char* key,
    const char* event,
    SipMessage& notify )
{
    // this is where we should send back a single notify
    // rather than notifying all phones
    sendNotifyForeachSubscription (
        key,
        event,
        notify );
}

void
Notifier::sendNotifyForeachSubscription (
    const char* key,
    const char* event,
    SipMessage& notify )
{
    ResultSet subscriptions;

    int timeNow = OsDateTime::getSecsSinceEpoch();

    // Get all subcriptions associated with this identity
    mpSubscriptionDB->
        getUnexpiredSubscriptions(
            key, event, timeNow, subscriptions );

    int numSubscriptions = subscriptions.getSize();

    if (  numSubscriptions > 0 )
    {
        OsSysLog::add( FAC_SIP, PRI_DEBUG, "Notifier::sendNotifyForeachSubscription - "
                       "Got at least %d unexpired subscription", numSubscriptions );

        // There may be any number of subscriptions
        // for the same identity and event type!
        // send a notify to each
        for (int i = 0; i<numSubscriptions; i++ )
        {
            UtlHashMap record;
            subscriptions.getIndex( i, record );

            UtlString uri        = *((UtlString*)record.findValue(&sUriKey));
            UtlString callid     = *((UtlString*)record.findValue(&sCallidKey));
            UtlString contact    = *((UtlString*)record.findValue(&sContactKey));
                                  ((UtlInt*)record.findValue(&sExpiresKey))->getValue();
                                  ((UtlInt*)record.findValue(&sSubscribecseqKey))->getValue();
            UtlString eventtype  = *((UtlString*)record.findValue(&sEventtypeKey));
            UtlString to         = *((UtlString*)record.findValue(&sToKey));
            UtlString from       = *((UtlString*)record.findValue(&sFromKey));
            UtlString file       = *((UtlString*)record.findValue(&sFileKey));
            UtlString key        = *((UtlString*)record.findValue(&sKeyKey));
            UtlString recordroute= *((UtlString*)record.findValue(&sRecordrouteKey));
            int notifycseq      = ((UtlInt*)record.findValue(&sNotifycseqKey))->getValue();

            // the recordRoute column in the database is optional
            // and can be set to null, the IMDB does not support null columns
            // so look to see if the field is set to "%" a special single character
            // reserved value we use to null columns in the IMDB
            if ( recordroute.compareTo(SPECIAL_IMDB_NULL_VALUE)==0 )
            {
                recordroute.remove(0);
            }

            // Make a copy of the message
            SipMessage notifyRequest(notify);

            UtlString statusContact;
            mpSipUserAgent->getContactUri(&statusContact);

            // increment the outbound cseq sent with the notify
            // this will guarantee that duplicate messages are rejected
            notifycseq += 1;

            // Set the NOTIFY request headers 
            // swapping the to and from fields
            notifyRequest.setNotifyData (
                contact,        // uri (final destination where we send the message)
                to,             // fromField
                from,           // toField
                callid,         // callId
                notifycseq,     // already incremeted
                event,          // eventtype
                statusContact,  // contact
                recordroute);   // added the missing route field

            // Send the NOTIFY message via the user agent with
            // the incremented notify csequence value
            mpSipUserAgent->send( notifyRequest );

            // Update the Notify sequence number (CSeq) in the IMDB
            mpSubscriptionDB->updateUnexpiredSubscription (
               to, from, callid, event, timeNow, notifycseq );
        }
    }
}



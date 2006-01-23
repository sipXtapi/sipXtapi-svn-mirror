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


// APPLICATION INCLUDES
#include "os/OsDateTime.h"
#include "os/OsQueuedEvent.h"
#include "os/OsTimer.h"
#include "os/OsEventMsg.h"
#include "utl/UtlRegex.h"
#include "utl/PluginHooks.h"
#include "net/SipUserAgent.h"
#include "net/NetMd5Codec.h"
#include "net/NameValueTokenizer.h"
#include "sipdb/RegistrationBinding.h"
#include "sipdb/RegistrationDB.h"
#include "sipdb/ResultSet.h"
#include "sipdb/SIPDBManager.h"
#include "sipdb/CredentialDB.h"
#include "sipdb/RegistrationDB.h"
#include "SipRegistrar.h"
#include "SipRegistrarServer.h"
#include "SyncRpc.h"
#include "registry/RegisterPlugin.h"

// DEFINES

/*
 * GRUUs are constructed by hashing the AOR, the IID, and the primary
 * SIP domain.  The SIP domain is included so that GRUUs constructed by
 * different systems will be different, but that any registrars that
 * form a redundant set will generate the same GRUU for an AOR/IID pair.
 * (Strictly, we use the entire value of the +sip.instance field parameter
 * on the Contact: header, which should be <...> around the IID, but there
 * is no point enforcing or parsing that rule.)
 */

// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
const RegEx RegQValue("^(0(\\.\\d{0,3})?|1(\\.0{0,3})?)$"); // syntax for a valid q parameter value

#define MIN_EXPIRES_TIME 300
#define HARD_MINIMUM_EXPIRATION 60
const char DEFAULT_EXPIRATION_TIME[] = "7200";

// STRUCTS
// TYPEDEFS

// FORWARD DECLARATIONS
// GLOBAL VARIABLES
// Static Initializers

static UtlString gUriKey("uri");
static UtlString gCallidKey("callid");
static UtlString gContactKey("contact");
static UtlString gExpiresKey("expires");
static UtlString gCseqKey("cseq");
static UtlString gQvalueKey("qvalue");
static UtlString gInstanceIdKey("instance_id");
static UtlString gGruuKey("gruu");

OsMutex         SipRegistrarServer::sLockMutex(OsMutex::Q_FIFO);

SipRegistrarServer::SipRegistrarServer(SipRegistrar& registrar) :
    OsServerTask("SipRegistrarServer", NULL, SIPUA_DEFAULT_SERVER_OSMSG_QUEUE_SIZE),
    mRegistrar(registrar),
    mIsStarted(FALSE),
    mSipUserAgent(NULL),
    mDefaultRegistryPeriod(),
    mNonceExpiration(5*60)
{
   // Recover the largest update number from the database for this primary
   //   (don't need to take the lock here, because we are not changing the database
   //    and no one has the reference to SipRegistrarServer yet).
   RegistrationDB* imdb = mRegistrar.getRegistrationDB();
   mDbUpdateNumber = imdb->getMaxUpdateNumberForRegistrar(mRegistrar.primaryName());
}

void
SipRegistrarServer::initialize(
   OsConfigDb*   pOsConfigDb,      ///< Configuration parameters
   SipUserAgent* pSipUserAgent)    ///< User Agent to use when sending responses
{
    mSipUserAgent = pSipUserAgent;

    // Minimum Registration Time
    pOsConfigDb->get("SIP_REGISTRAR_MIN_EXPIRES", mMinExpiresTimeStr);
    if ( mMinExpiresTimeStr.isNull() )
    {
        mMinExpiresTimeint = atoi(mMinExpiresTimeStr.data());

        if ( mMinExpiresTimeint < 60 )
        {
           OsSysLog::add(FAC_SIP, PRI_WARNING,
                         "SipRegistrarServer "
                         "configured minimum (%d) < hard minimum (%d); set to hard minimum",
                         mMinExpiresTimeint, HARD_MINIMUM_EXPIRATION
                         );
            mMinExpiresTimeint = HARD_MINIMUM_EXPIRATION;
            char min[10];
            sprintf(min, "%d", HARD_MINIMUM_EXPIRATION);
            mMinExpiresTimeStr = min;
        }
    }

    // Maximum/Default Registration Time
    UtlString maxExpiresTimeStr;
    pOsConfigDb->get("SIP_REGISTRAR_MAX_EXPIRES", maxExpiresTimeStr);
    if ( maxExpiresTimeStr.isNull() )
    {
       maxExpiresTimeStr = DEFAULT_EXPIRATION_TIME;
    }
    int maxExpiresTime = atoi(maxExpiresTimeStr.data());
    if ( maxExpiresTime >= mMinExpiresTimeint )
    {
        mDefaultRegistryPeriod = maxExpiresTime;
    }
    else 
    {
       OsSysLog::add(FAC_SIP, PRI_WARNING,
                     "SipRegistrarServer "
                     "configured maximum (%d) < minimum (%d); set to minimum",
                     maxExpiresTime, mMinExpiresTimeint
                     );
       mDefaultRegistryPeriod = mMinExpiresTimeint;
    }

    // Domain Name
    pOsConfigDb->get("SIP_REGISTRAR_DOMAIN_NAME", mDefaultDomain);
    if ( mDefaultDomain.isNull() )
    {
       OsSocket::getHostIp(&mDefaultDomain);
       OsSysLog::add(FAC_SIP, PRI_CRIT,
                     "SIP_REGISTRAR_DOMAIN_NAME not configured using IP '%s'",
                     mDefaultDomain.data()
                     );
    }
    // get the url parts for the domain
    Url defaultDomainUrl(mDefaultDomain);
    mDefaultDomainPort = defaultDomainUrl.getHostPort();
    defaultDomainUrl.getHostAddress(mDefaultDomainHost);
    // make sure that the unspecified domain name is also valid
    addValidDomain(mDefaultDomainHost, mDefaultDomainPort);

    // Domain Aliases
    //   (other domain names that this registrar accepts as valid in the request URI)
    UtlString domainAliases;
    pOsConfigDb->get("SIP_REGISTRAR_DOMAIN_ALIASES", domainAliases);
    
    UtlString aliasString;
    int aliasIndex = 0;
    while(NameValueTokenizer::getSubField(domainAliases.data(), aliasIndex,
                                          ", \t", &aliasString))
    {
       Url aliasUrl(aliasString);
       UtlString hostAlias;
       aliasUrl.getHostAddress(hostAlias);
       int port = aliasUrl.getHostPort();

       addValidDomain(hostAlias,port);
       aliasIndex++;
    }

    // Authentication Realm Name
    pOsConfigDb->get("SIP_REGISTRAR_AUTHENTICATE_REALM", mRealm);

    mProxyNormalPort = pOsConfigDb->getPort("SIP_REGISTRAR_PROXY_PORT");
    if (mProxyNormalPort == PORT_DEFAULT)
    {
       mProxyNormalPort = SIP_PORT;
    }
    
    // Authentication Scheme:  NONE | DIGEST
    UtlString authenticateScheme;
    pOsConfigDb->get("SIP_REGISTRAR_AUTHENTICATE_SCHEME", authenticateScheme);
    mUseCredentialDB = (authenticateScheme.compareTo("NONE" , UtlString::ignoreCase) != 0);

    /*
     * Unused Authentication Directives
     *
     * These directives are in the configuration files but are not used:
     *
     *   pOsConfigDb->get("SIP_REGISTRAR_AUTHENTICATE_ALGORITHM", authAlgorithm); 
     *     there may someday be a reason to use that one, since MD5 is aging.
     *
     *   pOsConfigDb->get("SIP_REGISTRAR_AUTHENTICATE_QOP", authQop);
     *     the qop will probably never be used - removed from current config file
     */

    // Registration Plugins
    mpSipRegisterPlugins = new PluginHooks( RegisterPlugin::Factory
                                           ,RegisterPlugin::Prefix
                                           );
    mpSipRegisterPlugins->readConfig(*pOsConfigDb);
   
}

int SipRegistrarServer::pullUpdates(
   const UtlString& registrarName,
   intll            updateNumber,
   UtlSList&        updates)
{
   // Critical Section here
   OsLock lock(sLockMutex);

   RegistrationDB* imdb = RegistrationDB::getInstance();
   int numUpdates = imdb->getNewUpdatesForRegistrar(registrarName, updateNumber, updates);
   return numUpdates;
}


/// Apply valid changes to the database
///
/// Checks the message against the database, and if it is allowed by
/// those checks, applies the requested changes.
SipRegistrarServer::RegisterStatus
SipRegistrarServer::applyRegisterToDirectory( const Url& toUrl
                                             ,const int timeNow
                                             ,const SipMessage& registerMessage
                                             )
{
    // Critical Section here
    OsLock lock(sLockMutex);

    RegisterStatus returnStatus = REGISTER_SUCCESS;
    UtlBoolean removeAll = FALSE;
    UtlBoolean isExpiresheader = FALSE;
    int longestRequested = -1; // for calculating the spread expiration time
    int commonExpires = -1;
    UtlString registerToStr;
    toUrl.getIdentity(registerToStr);
    
    // get the expires header from the register message
    // this may be overridden by the expires parameter on each contact
    if ( registerMessage.getExpiresField( &commonExpires ) )
    {
        isExpiresheader = TRUE; // only for use in testing '*'
    }
    else
    {
        commonExpires = mDefaultRegistryPeriod;
    }

    // get the header 'callid' from the register message
    UtlString registerCallidStr;
    registerMessage.getCallIdField( &registerCallidStr );

    // get the cseq and the method (should be REGISTER)
    // from the register message
    UtlString method;
    int registerCseqInt = 0;
    registerMessage.getCSeqField( &registerCseqInt, &method );

    RegistrationDB* imdb = RegistrationDB::getInstance();

    // Check that this call-id and cseq are newer than what we have in the db
    if (! imdb->isOutOfSequence(toUrl, registerCallidStr, registerCseqInt))
    {
        // ****************************************************************
        // We now make two passes over all the contacts - the first pass
        // checks each contact for validity, and if valid, stores it in the
        // ResultSet registrations.
        // We act on the stored registrations only if all checks pass, in
        // a second iteration over the ResultSet below.
        // ****************************************************************

        ResultSet registrations; // built up during validation, acted on if all is well

        int contactIndexCount;
        UtlString registerContactStr;
        for ( contactIndexCount = 0;
              (   REGISTER_SUCCESS == returnStatus
               && registerMessage.getContactEntry ( contactIndexCount
                                                   ,&registerContactStr
                                                   )
               );
              contactIndexCount++
             )
        {
           OsSysLog::add( FAC_SIP, PRI_WARNING,
                          "SipRegistrarServer::applyRegisterToDirectory - processing '%s'\n",
                          registerContactStr.data()
                               );
            if ( registerContactStr.compareTo("*") != 0 ) // is contact == "*" ?
            {
                // contact != "*"; a normal contact
                int expires;
                UtlString qStr, expireStr;
                Url registerContactURL( registerContactStr );

                // Check for an (optional) Expires field parameter in the Contact
                registerContactURL.getFieldParameter( SIP_EXPIRES_FIELD, expireStr );
                if ( expireStr.isNull() )
                {
                    // no expires parameter on the contact
                    // use the default established above
                    expires = commonExpires;
                }
                else
                {
                    // contact has its own expires parameter, which takes precedence
                    char  expireVal[12]; // more digits than we need...
                    char* end;
                    strncpy( expireVal, expireStr.data(), 10 );
                    expireVal[11] = '\0';// ensure it is null terminated.
                    expires = strtol(expireVal,&end,/*base*/10);
                    if ( '\0' != *end )
                    {
                        // expires value not a valid base 10 number
                        returnStatus = REGISTER_INVALID_REQUEST;
                        OsSysLog::add( FAC_SIP, PRI_WARNING,
                                      "SipRegistrarServer::applyRegisterToDirectory"
                                      " invalid expires parameter value '%s'\n",
                                      expireStr.data()
                                      );
                    }
                }

                if ( REGISTER_SUCCESS == returnStatus )
                {
                    // Ensure that the expires value is within allowed limits
                    if ( 0 == expires )
                    {
                        // unbind this mapping; ok
                    }
                    else if ( expires < mMinExpiresTimeint ) // lower bound
                    {
                        returnStatus = REGISTER_LESS_THAN_MINEXPIRES;
                    }
                    else if ( expires > mDefaultRegistryPeriod ) // upper bound
                    {
                        // our default is also the maximum we'll allow
                        expires = mDefaultRegistryPeriod;
                    }

                    if ( REGISTER_SUCCESS == returnStatus )
                    {
                        // Get the qValue from the register message.
                        UtlString registerQvalueStr;
                        registerContactURL.getFieldParameter( SIP_Q_FIELD, registerQvalueStr );
                        // Get the Instance ID (if any) from the REGISTER message Contact field.
                        UtlString instanceId;
                        registerContactURL.getFieldParameter( "+sip.instance", instanceId );

                        // track longest expiration requested in the set
                        if ( expires > longestRequested )
                        {
                           longestRequested = expires;
                        }

                        // See if the Contact field has a "gruu" URI field..
                        // :TODO: Have to check whether the gruu URI parameter
                        // is still favored.  And what exactly this check is about.
                        UtlBoolean gruuPresent;
                        UtlString gruuDummy;
                        gruuPresent = registerContactURL.getUrlParameter( "gruu", gruuDummy );
                        OsSysLog::add( FAC_SIP, PRI_DEBUG,
                                       "SipRegistrarServer::applyRegisterToDirectory instance ID = '%s'",
                                       instanceId.data());

                        // remove the parameter fields - they are not part of the contact itself
                        registerContactURL.removeFieldParameters();
                        UtlString contactWithoutExpires = registerContactURL.toString();

                        // Build the row for the validated contacts hash
                        UtlHashMap registrationRow;

                        // value strings
                        UtlString* contactValue =
                            new UtlString ( contactWithoutExpires );
                        UtlInt* expiresValue =
                            new UtlInt ( expires );
                        UtlString* qvalueValue =
                            new UtlString ( registerQvalueStr );
                        UtlString* instanceIdValue = new UtlString ( instanceId );
                        UtlString* gruuValue;

                        // key strings - make shallow copies of static keys
                        UtlString* contactKey = new UtlString( gContactKey );
                        UtlString* expiresKey = new UtlString( gExpiresKey );
                        UtlString* qvalueKey = new UtlString( gQvalueKey );
                        UtlString* instanceIdKey = new UtlString( gInstanceIdKey );
                        UtlString* gruuKey = new UtlString( gGruuKey );

                        // Calculate GRUU if gruu is in Supported, +sip.instance is provided, but
                        // gruu is not a URI parameter.
                        if (!instanceId.isNull() &&
                            !gruuPresent &&
                            registerMessage.isInSupportedField("gruu"))
                        {
                           // Hash the GRUU base, the AOR, and IID to
                           // get the variable part of the GRUU.
                           NetMd5Codec encoder;
                           UtlString temp;
                           // Use the trick that the MD5 of a series of null-
                           // separated strings is effectively a unique function
                           // of all of the strings.
                           // Include "sipX" as the signature of this software.
                           temp.append("sipX", 5);
                           temp.append(mDefaultDomain);
                           temp.append("\0", 1);
                           temp.append(toUrl.toString());
                           temp.append("\0", 1);
                           temp.append(instanceId);
                           UtlString hash;
                           encoder.encode(temp, hash);
                           hash.remove(16);
                           // Now construct the GRUU URI,
                           // "gruu~XXXXXXXXXXXXXXXX@[principal SIP domain]".
                           // That is what we store in IMDB, so it can be
                           // searched for by the redirector, since it searches
                           // for the "identity" part of the URI, which does
                           // not contain the scheme.
                           gruuValue = new UtlString(GRUU_PREFIX);
                           gruuValue->append(hash);
                           gruuValue->append("@");
                           gruuValue->append(mDefaultDomain);
                           OsSysLog::add(FAC_SIP, PRI_DEBUG,
                                         "SipRegistrarServer::applyRegisterToDirectory gruu = '%s'",
                                         gruuValue->data());
                        }
                        else
                        {
                           gruuValue = new UtlString( "" );
                        }

                        registrationRow.insertKeyAndValue( contactKey, contactValue );
                        registrationRow.insertKeyAndValue( expiresKey, expiresValue );
                        registrationRow.insertKeyAndValue( qvalueKey, qvalueValue );
                        registrationRow.insertKeyAndValue( instanceIdKey, instanceIdValue );
                        registrationRow.insertKeyAndValue( gruuKey, gruuValue );

                        registrations.addValue( registrationRow );
                    }
                }
            }
            else
            {
                // Asterisk '*' requests that we unregister all contacts for the AOR
                removeAll = TRUE;
            }
        } // iteration over Contact entries

        // Now that we've gone over all the Contacts
        // act on them only if all was kosher
        if ( REGISTER_SUCCESS == returnStatus )
        {
            if ( 0 == contactIndexCount )
            {   // no contact entries were found - this is just a query
                returnStatus = REGISTER_QUERY;
            }
            else
            {
               int spreadExpirationTime = 0;

               // Increment the update number so as to assign a single fresh update
               // number to all the database changes we're about to make.  In some
               // cases we might not actually make any changes.  That's OK, having
               // an update number with no associated changes won't break anything.
               mDbUpdateNumber++;

                if ( removeAll )
                {
                    // Contact: * special case
                    //  - request to remove all bindings for toUrl
                    if (   isExpiresheader
                        && 0 == commonExpires
                        && 1 == contactIndexCount
                        )
                    {
                        // Expires: 0 && Contact: * - clear all contacts
                        imdb->expireAllBindings( toUrl
                                                ,registerCallidStr
                                                ,registerCseqInt
                                                ,timeNow
                                                ,primaryName()
                                                ,mDbUpdateNumber
                                                );
                    }
                    else
                    {
                        // not allowed per rfc 3261
                        //  - must use Expires header of zero with Contact: *
                        //  - must not combine this with other contacts
                        returnStatus = REGISTER_INVALID_REQUEST;
                    }
                }
                else
                {
                    // Normal REGISTER request, no Contact: * entry
                    int numRegistrations = registrations.getSize();

                    // act on each valid contact
                    if ( numRegistrations > 0 )
                    {
                        /*
                         * Calculate the maximum expiration time for this set.
                         * The idea is to spread registrations by choosing a random
                         * expiration time.
                         */
                       int spreadFloor = mMinExpiresTimeint*2;
                       if ( longestRequested > spreadFloor )
                        {
                           // a normal (long) registration
                           // - spread it between twice the min and the longest they asked for
                           spreadExpirationTime = (  (rand() % (longestRequested - spreadFloor))
                                                   + spreadFloor);
                        }
                        else if ( longestRequested > mMinExpiresTimeint )
                        {
                           // a short but not minimum registration
                           // - spread it between the min and the longest they asked for
                           spreadExpirationTime = (  (rand()
                                                      % (longestRequested - mMinExpiresTimeint)
                                                      )
                                                   + mMinExpiresTimeint
                                                   );
                        }
                        else // longestExpiration == mMinExpiresTimeint
                        {
                           // minimum - can't randomize because we can't shorten or lengthen it
                           spreadExpirationTime = mMinExpiresTimeint;
                        }

                        for ( int i = 0; i<numRegistrations; i++ )
                        {
                            UtlHashMap record;
                            registrations.getIndex( i, record );

                            int      expires = ((UtlInt*)record.findValue(&gExpiresKey))->getValue();
                            UtlString contact(*((UtlString*)record.findValue(&gContactKey)));

                            int expirationTime;
                            if ( expires == 0 )
                            {
                                // Unbind this binding
                                //
                                // To cancel a contact, we expire it one second ago.
                                // This allows it to stay in the database until the
                                // explicit cleanAndPersist method cleans it out (which
                                // will be when it is more than one maximum registration
                                // time in the past).
                                // This prevents the problem of an expired registration
                                // being recreated by an old REGISTER request coming in
                                // whose cseq is lower than the one that unregistered it,
                                // which, if we had actually removed the entry would not
                                // be there to compare the out-of-order message to.
                                expirationTime = timeNow-1;

                                OsSysLog::add( FAC_SIP, PRI_DEBUG,
                                              "SipRegistrarServer::applyRegisterToDirectory - Expiring map %s->%s\n",
                                              registerToStr.data(), contact.data()
                                              );
                            }
                            else
                            {
                                // expires > 0, so add the registration
                                expirationTime = (  expires < spreadExpirationTime
                                                  ? expires
                                                  : spreadExpirationTime
                                                  ) + timeNow;

                                OsSysLog::add( FAC_SIP, PRI_DEBUG,
                                       "SipRegistrarServer::applyRegisterToDirectory - Adding map %s->%s\n",
                                       registerToStr.data(), contact.data() );
                            }

                            UtlString qvalue(*(UtlString*)record.findValue(&gQvalueKey));
                            UtlString instance_id(*(UtlString*)record.findValue(&gInstanceIdKey));
                            UtlString gruu(*(UtlString*)record.findValue(&gGruuKey));

                            imdb->updateBinding( toUrl, contact, qvalue
                                                ,registerCallidStr, registerCseqInt
                                                ,expirationTime
                                                ,instance_id
                                                ,gruu
                                                ,primaryName()
                                                ,mDbUpdateNumber
                                                );

                        } // iterate over good contact entries

                        // If there were any bindings not dealt with explicitly in this
                        // message that used the same callid, then expire them.
                        imdb->expireOldBindings( toUrl, registerCallidStr, registerCseqInt, 
                                                 timeNow, primaryName(),
                                                 mDbUpdateNumber );
                    }
                    else
                    {
                        OsSysLog::add( FAC_SIP, PRI_ERR
                                      ,"SipRegistrarServer::applyRegisterToDirectory contact count mismatch %d != %d"
                                      ,contactIndexCount, numRegistrations
                                      );
                        returnStatus = REGISTER_QUERY;
                    }
                }
                
                // Only persist the xml and do hooks if this was a good registration
                if ( REGISTER_SUCCESS == returnStatus )
                {
                    // something changed - garbage collect and persist the database
                    int oldestTimeToKeep = timeNow - mDefaultRegistryPeriod;
                    imdb->cleanAndPersist( oldestTimeToKeep );

                    // give each RegisterPlugin a chance to do its thing
                    PluginIterator plugins(*mpSipRegisterPlugins);
                    RegisterPlugin* plugin;
                    while ((plugin = static_cast<RegisterPlugin*>(plugins.next())))
                    {
                       plugin->takeAction(registerMessage, spreadExpirationTime, mSipUserAgent );
                    }
                }
            }
        }
    }
    else
    {
        OsSysLog::add( FAC_SIP, PRI_WARNING,
                      "SipRegistrarServer::applyRegisterToDirectory request out of order\n"
                      "  To: '%s'\n"
                      "  Call-Id: '%s'\n"
                      "  Cseq: %d",
                      registerToStr.data(), registerCallidStr.data(), registerCseqInt
                     );

        returnStatus = REGISTER_OUT_OF_ORDER;
    }

    return returnStatus;
}


intll
SipRegistrarServer::applyUpdatesToDirectory(
   int timeNow,
   const UtlSList& updates)
{
    // Critical Section here
    OsLock lock(sLockMutex);

   // Loop over the updates.  For each update, if the call ID and cseq are newer than what
   // we have in the DB, then apply the update to the DB.
   UtlSListIterator updateIter(updates);
   RegistrationBinding* reg;
   bool updated = false;    // set to true if we make any updates
   RegistrationDB* imdb = RegistrationDB::getInstance();
   while ((reg = dynamic_cast<RegistrationBinding*>(updateIter())))
   {
      if (!imdb->isOutOfSequence(*(reg->getUri()), *(reg->getCallId()), reg->getCseq()))
      {
         imdb->updateBinding(*reg);
         updated = true;
      
      }
      else
      {
         OsSysLog::add(
            FAC_SIP, PRI_WARNING,
            "SipRegistrarServer::applyUpdateToDirectory update out of cseq order\n"
            "  To: '%s'\n"
            "  Call-Id: '%s'\n"
            "  Cseq: %d",
            reg->getUri()->toString().data(), reg->getCallId()->data(), reg->getCseq()
            );
      }
   }

   // If we accepted any updates, then garbage collect and persist the database
   if (updated)
   {
      int oldestTimeToKeep = timeNow - mDefaultRegistryPeriod;
      imdb->cleanAndPersist(oldestTimeToKeep);
   }

   // :HA: Update PeerReceivedDbUpdateNumber to updateNumber if updateNumber is bigger.
   // If updateNumber is out of order, then log a warning in case there is a problem.
   // Return PeerReceivedDbUpdateNumber.
   return reg ? reg->getUpdateNumber() : 0;    // for now, just return the update number
}


//functions
UtlBoolean
SipRegistrarServer::handleMessage( OsMsg& eventMessage )
{
    int msgType = eventMessage.getMsgType();
    int msgSubType = eventMessage.getMsgSubType();

    // Timer event
    if(msgType == OsMsg::OS_EVENT &&
                msgSubType == OsEventMsg::NOTIFY)
    {
        // Garbage collect nonces
        //osPrintf("Garbage collecting nonces\n");

        OsTime time;
        OsDateTime::getCurTimeSinceBoot(time);
        long now = time.seconds();
        // Remove nonces more than 5 minutes old
        long oldTime = now - mNonceExpiration;
        mNonceDb.removeOldNonces(oldTime);
    }

    // SIP message event
    else if (msgType == OsMsg::PHONE_APP)
    {
        // osPrintf("SipRegistrarServer::handleMessage() - Start processing REGISTER Message\n");
        OsSysLog::add( FAC_SIP, PRI_DEBUG, "SipRegistrarServer::handleMessage() - "
                "Start processing REGISTER Message\n" );

        const SipMessage& message = *((SipMessageEvent&)eventMessage).getMessage();
        UtlString userKey, uri;
        SipMessage finalResponse;

        if ( isValidDomain( message, finalResponse ) )
        {
           // get the header 'to' field from the register
           // message and construct a URL with it
           // this is also called the Address of record
           UtlString registerToStr;
           message.getToUri( &registerToStr );
           Url toUrl( registerToStr );

           /*
            * Normalize the port in the Request URI
            *   This is not strictly kosher, but it solves interoperability problems.
            *   Technically, user@foo:5060 != user@foo , but many implementations
            *   insist on including the explicit port even when they should not, and
            *   it causes registration mismatches, so we normalize the URI when inserting
            *   and looking up in the database so that if explicit port is the same as
            *   the proxy listening port, then we remove it.
            *   (Since our proxy has mProxyNormalPort open, no other SIP entity
            *   can use sip:user@domain:mProxyNormalPort, so this normalization
            *   cannot interfere with valid addresses.)
            *
            * For the strict rules, set the configuration parameter
            *   SIP_REGISTRAR_PROXY_PORT : PORT_NONE
            */
           if (   mProxyNormalPort != PORT_NONE
               && toUrl.getHostPort() == mProxyNormalPort
               )
           {
              toUrl.setHostPort(PORT_NONE);
           }
           
           // check in credential database if authentication needed
           if ( isAuthorized( toUrl, message, finalResponse ) )
            {
                int port;
                int tagNum = 0;
                UtlString address, protocol, tag;
                message.getToAddress( &address, &port, &protocol, NULL, NULL, &tag );
                if ( tag.isNull() )
                {
                    tagNum = rand();     //generate to tag for response;
                }

                // process REQUIRE Header Field
                // add new contact values - update or insert
                int timeNow = OsDateTime::getSecsSinceEpoch();

                RegisterStatus applyStatus
                   = applyRegisterToDirectory( toUrl, timeNow, message );

                switch (applyStatus)
                {
                    case REGISTER_SUCCESS:
                    case REGISTER_QUERY:
                    {
                        OsSysLog::add( FAC_SIP, PRI_DEBUG, "SipRegistrarServer::handleMessage() - "
                               "contact successfully added\n");

                        //create response - 200 ok reseponse
                        finalResponse.setOkResponseData(&message);

                        // get the call-id from the register message for context test below
                        UtlString registerCallId;
                        message.getCallIdField(&registerCallId);

                        //get all current contacts now for the response
                        ResultSet registrations;

                        RegistrationDB::getInstance()->
                            getUnexpiredContacts(
                                toUrl, timeNow, registrations );

                        int numRegistrations = registrations.getSize();
                        for ( int i = 0 ; i<numRegistrations; i++ )
                        {
                          UtlHashMap record;
                          registrations.getIndex( i, record );

                          // Check if this contact should be returned in this context
                          // for REGISTER_QUERY, return all contacts
                          // for REGISTER_SUCCESS, return only the contacts with the same
                          //     call-id.  This is because we've seen too many phones that
                          //     don't expect to see contacts other than the one they sent,
                          //     and get upset by contact parameters meant for others
                          //     In particular, when some other contact is about to expire,
                          //     they think that they got a short time and try again - which
                          //     loops until the other contact expires or is refreshed by
                          //     someone else - not good.
                          UtlString* contactCallId
                             = dynamic_cast<UtlString*>(record.findValue(&gCallidKey));
                          if (   REGISTER_QUERY == applyStatus
                              || (   contactCallId
                                  && registerCallId.isEqual(contactCallId)
                                  )
                              )
                          {
                            UtlString contactKey("contact");
                            UtlString expiresKey("expires");
                            UtlString qvalueKey("qvalue");
                            UtlString instanceIdKey("instance_id");
                            UtlString gruuKey("gruu");
                            UtlString contact = *((UtlString*)record.findValue(&contactKey));
                            UtlString qvalue = *((UtlString*)record.findValue(&qvalueKey));
                            int expires = ((UtlInt*)record.findValue(&expiresKey))->getValue();
                            expires = expires - timeNow;

                            OsSysLog::add( FAC_SIP, PRI_DEBUG, "SipRegistrarServer::handleMessage - "
                               "processing contact '%s'", contact.data());
                            Url contactUri( contact );

                            char buffexpires[32];
                            sprintf(buffexpires, "%d", expires);

                            contactUri.setFieldParameter(SIP_EXPIRES_FIELD, buffexpires);
                            if ( !qvalue.isNull() && qvalue.compareTo(SPECIAL_IMDB_NULL_VALUE)!=0 )
                            {
                               OsSysLog::add( FAC_SIP, PRI_DEBUG, "SipRegistrarServer::handleMessage - "
                               "adding q '%s'", qvalue.data());

                               //check if q value is numeric and between the range 0.0 and 1.0
                               RegEx qValueValid(RegQValue); 
                               if (qValueValid.Search(qvalue.data()))
                               {
                                  contactUri.setFieldParameter(SIP_Q_FIELD, qvalue);
                               }
                            }

                            // Add the +sip.instance and gruu
                            // parameters if an instance ID is recorded.
                            UtlString* instance_id = dynamic_cast<UtlString*> (record.findValue(&instanceIdKey));
                            OsSysLog::add( FAC_SIP, PRI_DEBUG, "SipRegistrarServer::handleMessage - value %p, instance_id %p, instanceIdKey = '%s'", 
                                           record.findValue(&instanceIdKey),
                                           instance_id, instanceIdKey.data());
                            if (instance_id && !instance_id->isNull() &&
                                instance_id->compareTo(SPECIAL_IMDB_NULL_VALUE) !=0 )
                            {
                               OsSysLog::add( FAC_SIP, PRI_DEBUG, "SipRegistrarServer::handleMessage - add instance '%s'", instance_id->data());
                               contactUri.setFieldParameter("+sip.instance",
                                                            *instance_id);
                               // Prepend "sip:" to the GRUU, since it is stored
                               // in the database in identity form.
                               UtlString temp("sip:");
                               temp.append(
                                  *(dynamic_cast<UtlString*>
                                    (record.findValue(&gruuKey))));
                               contactUri.setFieldParameter("gruu", temp);
                            }

                            finalResponse.setContactField(contactUri.toString(),i);
                          }
                        }
                    }
                    break;

                    case REGISTER_OUT_OF_ORDER:
                        finalResponse.setResponseData(&message,SIP_5XX_CLASS_CODE,"Out Of Order");
                        break;

                    case REGISTER_LESS_THAN_MINEXPIRES:
                        //send 423 Registration Too Brief response
                        //must contain Min-Expires header field
                        finalResponse.setResponseData(&message,SIP_TOO_BRIEF_CODE,SIP_TOO_BRIEF_TEXT);
                        finalResponse.setHeaderValue(SIP_MIN_EXPIRES_FIELD, mMinExpiresTimeStr, 0);
                        break;

                    case REGISTER_INVALID_REQUEST:
                        finalResponse.setResponseData(&message,SIP_BAD_REQUEST_CODE, SIP_BAD_REQUEST_TEXT);
                        break;

                    case REGISTER_FORBIDDEN:
                        finalResponse.setResponseData(&message,SIP_FORBIDDEN_CODE, SIP_FORBIDDEN_TEXT);
                        break;

                    case REGISTER_NOT_FOUND:
                        finalResponse.setResponseData(&message,SIP_NOT_FOUND_CODE, SIP_NOT_FOUND_TEXT);
                        break;

                    default:
                       OsSysLog::add( FAC_SIP, PRI_ERR, 
                                     "Invalid result %d from applyRegisterToDirectory",
                                     applyStatus
                                     );
                        finalResponse.setResponseData(&message,SIP_SERVER_INTERNAL_ERROR_CODE, SIP_SERVER_INTERNAL_ERROR_TEXT);
                        break;
                }

                if ( tag.isNull() )
                {
                    finalResponse.setToFieldTag(tagNum);
                }
            }
           else
            {
               // authentication error - response data was set in isAuthorized
            }
        }
        else   
        {
           // Invalid domain for registration - response data was set in isValidDomain
        }

        if (OsSysLog::willLog(FAC_SIP, PRI_DEBUG))
        {
           UtlString finalMessageStr;
           int finalMessageLen;
           finalResponse.getBytes(&finalMessageStr, &finalMessageLen);
           OsSysLog::add( FAC_SIP, PRI_DEBUG, "\n----------------------------------\n"
                         "Sending final response\n%s\n", finalMessageStr.data());
        }
        
        mSipUserAgent->send(finalResponse);
    }
    return TRUE;
}


UtlBoolean
SipRegistrarServer::isAuthorized (
    const Url&  toUrl,
    const SipMessage& message,
    SipMessage& responseMessage )
{
    UtlString fromUri;
    UtlBoolean isAuthorized = FALSE;

    message.getFromUri(&fromUri);
    Url fromUrl(fromUri);

    UtlString identity;
    toUrl.getIdentity(identity);
    
    if ( !mUseCredentialDB )
    {
        OsSysLog::add( FAC_AUTH, PRI_DEBUG, "SipRegistrarServer::isAuthorized() "
                ":: No Credential DB - request is always AUTHORIZED\n" );
        isAuthorized = TRUE;
    }
    else
    {
        // realm and auth type should be default for server!!!!!!!!!!
        // if URI not defined in DB, the user is not authorized to modify bindings - NOT DOING ANYMORE
        // check if we requested authentication and this is the req with
        // authorization,validate the authorization
        OsSysLog::add( FAC_AUTH, PRI_DEBUG, "SipRegistrarServer::isAuthorized()"
                ": fromUri='%s', toUri='%s', realm='%s' \n", fromUri.data(), toUrl.toString().data(), mRealm.data() );

        UtlString requestNonce, requestRealm, requestUser, uriParam;
        int requestAuthIndex = 0;
        UtlString callId;
        UtlString fromTag;

        message.getCallIdField(&callId);
        fromUrl.getFieldParameter("tag", fromTag);

        while ( ! isAuthorized
               && message.getDigestAuthorizationData(
                   &requestUser, &requestRealm, &requestNonce,
                   NULL, NULL, &uriParam,
                   HttpMessage::SERVER, requestAuthIndex)
               )
        {
           OsSysLog::add( FAC_AUTH, PRI_DEBUG, "Message Authorization received: "
                    "reqRealm='%s', reqUser='%s'\n", requestRealm.data() , requestUser.data());

            if ( mRealm.compareTo(requestRealm) == 0 ) // case sensitive check that realm is correct
            {
                OsSysLog::add(FAC_AUTH, PRI_DEBUG, "SipRegistrarServer::isAuthorized() Realm Matches\n");

                // need the request URI to validate the nonce
                UtlString reqUri;
                message.getRequestUri(&reqUri);
                UtlString authTypeDB;
                UtlString passTokenDB;

                // validate the nonce
                if (mNonceDb.isNonceValid(requestNonce, callId, fromTag,
                                          reqUri, mRealm, mNonceExpiration))
                {
                    Url discardUriFromDB;

                    // then get the credentials for this user & realm
                    if (CredentialDB::getInstance()->getCredential( toUrl
                                                                   ,requestRealm
                                                                   ,requestUser
                                                                   ,passTokenDB
                                                                   ,authTypeDB
                                                                   ))
                    {
                      // only DIGEST is used, so the authTypeDB above is ignored
                      if ((isAuthorized = message.verifyMd5Authorization(requestUser.data(),
                                                                         passTokenDB.data(),
                                                                         requestNonce,
                                                                         requestRealm.data(),
                                                                         uriParam)
                           ))
                        {
                          OsSysLog::add(FAC_AUTH, PRI_DEBUG,
                                        "SipRegistrarServer::isAuthorized() "
                                        "response auth hash matches\n");
                        }
                      else
                        {
                          OsSysLog::add(FAC_AUTH, PRI_ERR,
                                        "Response auth hash does not match (bad password?)"
                                        "\n toUrl='%s' requestUser='%s'",
                                        toUrl.toString().data(), requestUser.data());
                        }
                    }
                    else // failed to get credentials
                    {
                        OsSysLog::add(FAC_AUTH, PRI_ERR,
                                      "Unable to get credentials for '%s'\nrealm='%s'\nuser='%s'",
                                      identity.data(), mRealm.data(), requestUser.data());
                    }
                }
                else // nonce is not valid
                {
                    OsSysLog::add(FAC_AUTH, PRI_ERR,
                                  "Invalid nonce for '%s'\nnonce='%s'\ncallId='%s'\nreqUri='%s'",
                                  identity.data(), requestNonce.data(), callId.data(), reqUri.data());
                }
            }
            requestAuthIndex++;
        } //end while

        if ( !isAuthorized )
        {
           // Generate a new challenge
            UtlString newNonce;
            UtlString challangeRequestUri;
            message.getRequestUri(&challangeRequestUri);
            UtlString opaque;

            mNonceDb.createNewNonce(callId,
                                    fromTag,
                                    challangeRequestUri,
                                    mRealm,
                                    newNonce);

            responseMessage.setRequestUnauthorized ( &message, HTTP_DIGEST_AUTHENTICATION, mRealm,
                                                    newNonce, NULL // opaque
                                                    );
            
        }
    }
    return isAuthorized;
}

void
SipRegistrarServer::addValidDomain(const UtlString& host, int port)
{
   UtlString* valid = new UtlString(host);
   valid->toLower();

   char explicitPort[20];
   sprintf(explicitPort,":%d", PORT_NONE==port ? SIP_PORT : port );
   valid->append(explicitPort);
   
   OsSysLog::add(FAC_AUTH, PRI_DEBUG, "SipRegistrarServer::addValidDomain(%s)",valid->data()) ;

   mValidDomains.insert(valid);
}

UtlBoolean
SipRegistrarServer::isValidDomain(
    const SipMessage& message,
    SipMessage& responseMessage )
{
    UtlBoolean isValid;
   
    // Fetch the domain and port from the request URI
    UtlString lookupDomain, requestUri;
    message.getRequestUri( &requestUri );
    Url reqUri(requestUri);

    reqUri.getHostAddress(lookupDomain);
    lookupDomain.toLower();

    int port = reqUri.getHostPort();
    if (port == PORT_NONE)
    {
       port = SIP_PORT;
    }
    char portNum[15];
    sprintf(portNum,"%d",port);

    lookupDomain.append(":");
    lookupDomain.append(portNum);

    if ( mValidDomains.contains(&lookupDomain) )
    {
        OsSysLog::add(FAC_AUTH, PRI_DEBUG,
                      "SipRegistrarServer::isValidDomain(%s) VALID\n",
                      lookupDomain.data()) ;
        isValid = TRUE;
    }
    else
    {
       UtlString requestedDomain;
       reqUri.getHostAddress(requestedDomain);

       OsSysLog::add(FAC_AUTH, PRI_WARNING,
                     "SipRegistrarServer::isValidDomain('%s' == '%s') Invalid\n",
                     requestedDomain.data(), lookupDomain.data()) ;

       UtlString responseText;
       responseText.append("Domain '");
       responseText.append(requestedDomain);
       responseText.append("' is not valid at this registrar");
       responseMessage.setResponseData(&message, SIP_NOT_FOUND_CODE, responseText.data() );
       isValid = FALSE;
    }

    return isValid;
}

const char* SipRegistrarServer::primaryName()
{
   return mRegistrar.primaryName();
}

/// Reset the upper half of the DbUpdateNumber to the epoch time.
void SipRegistrarServer::resetDbUpdateNumberEpoch()
{
   int timeNow = OsDateTime::getSecsSinceEpoch();
   intll newEpoch;
   newEpoch = timeNow;
   newEpoch <<= 32;

   { // lock before changing the epoch update number
      OsLock lock(sLockMutex);
            
      mDbUpdateNumber = newEpoch;
   } // release lock before logging
   
   OsSysLog::add(FAC_SIP, PRI_INFO,
                 "SipRegistrarServer::resetDbUpdateNumberEpoch to %llx",
                 newEpoch
                 );
}

    

SipRegistrarServer::~SipRegistrarServer()
{
   mValidDomains.destroyAll();
}

void RegisterPlugin::takeAction( const SipMessage&   registerMessage  
                                ,const unsigned int  registrationDuration 
                                ,SipUserAgent*       sipUserAgent
                                )
{
   assert(false);
   
   OsSysLog::add(FAC_SIP, PRI_ERR,
                 "RegisterPlugin::takeAction not resolved by configured hook"
                 );
}

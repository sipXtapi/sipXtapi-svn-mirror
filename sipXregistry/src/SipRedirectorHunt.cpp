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
#include "os/OsFS.h"
#include "os/OsLock.h"
#include "os/OsSysLog.h"
#include "net/Url.h"
#include "net/SipMessage.h"
#include "sipdb/HuntgroupDB.h"
#include "sipdb/ResultSet.h"
#include "SipRegistrar.h"
#include "SipRedirectorHunt.h"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
UtlBoolean   SipRedirectorHunt::sHuntGroupsDefined (FALSE);

/* Hunt group q values are in the range 0.400 <= q < 0.600 */
#define HUNT_GROUP_MAX_Q 600
#define HUNT_RANGE_SIZE  200
#define HUNT_GROUP_MAX_CONTACTS 40

// Constructor
SipRedirectorHunt::SipRedirectorHunt()
{
}

// Destructor
SipRedirectorHunt::~SipRedirectorHunt()
{
}

// Initializer
OsStatus
SipRedirectorHunt::initialize(const UtlHashMap& configParameters,
                              OsConfigDb& configDb,
                              SipUserAgent* pSipUserAgent,
                              int redirectorNo)
{
   // Determine whether we have huntgroup supported
   // if the XML file contains 0 rows or the file
   // does not exist then disable huntgroup support
   ResultSet resultSet;
   HuntgroupDB::getInstance()->getAllRows(resultSet);
   sHuntGroupsDefined = (resultSet.getSize() > 0);

   return OS_SUCCESS;
}

// Finalizer
void
SipRedirectorHunt::finalize()
{
}

SipRedirector::LookUpStatus
SipRedirectorHunt::lookUp(
   const SipMessage& message,
   const UtlString& requestString,
   const Url& requestUri,
   const UtlString& method,
   SipMessage& response,
   RequestSeqNo requestSeqNo,
   int redirectorNo,
   SipRedirectorPrivateStorage*& privateStorage)
{
   // Avoid doing any work if there are no hunt groups defined.
   if (!sHuntGroupsDefined)
   {
      return SipRedirector::LOOKUP_SUCCESS;
   }

   // Return immediately if the method is SUBSCRIBE, as the q values will
   // be stripped later anyway.
   if (method.compareTo(SIP_SUBSCRIBE_METHOD, UtlString::ignoreCase) == 0)
   {
      return SipRedirector::LOOKUP_SUCCESS;
   }

   // Avoid doing any work if the request URI is not a hunt group.
   if (!HuntgroupDB::getInstance()->isHuntGroup(requestUri))
   {
      return SipRedirector::LOOKUP_SUCCESS;
   }

   int numContacts = response.getCountHeaderFields(SIP_CONTACT_FIELD);

   numContacts = ( (numContacts <= HUNT_GROUP_MAX_CONTACTS)
                  ? numContacts
                  : HUNT_GROUP_MAX_CONTACTS);

   int* qDeltas = new int[numContacts]; // records random deltas already used.
   int deltasSet = 0; // count of entries filled in qDeltas 

   UtlString thisContact;
   for (int contactNum = 0;
        response.getContactField(contactNum, thisContact);
        contactNum++)
   {
      Url contactUri( thisContact );
      UtlString qValue;

      if (!contactUri.getFieldParameter(SIP_Q_FIELD, qValue))
      {
         // this contact is not explicitly ordered, so generate a q value for it

         if (deltasSet < HUNT_GROUP_MAX_CONTACTS) // we only randomize this many
         {
            // pick a random delta, ensure it has not been used already
            bool duplicate=false;
            do 
            {
               // The rand man page says not to use low order bits this way, but
               // we're not doing security here, just sorting, so this is good enough,
               // and it's much faster than the floating point they suggest.
               int thisDelta = 1 + (rand() % HUNT_RANGE_SIZE); // 0 < thisDelta <= HUNT_RANGE_SIZE

               // check to see that thisDelta is unique in qDeltas
               int i;
               for (i = 0, duplicate=false; (!duplicate) && (i < deltasSet); i++)
               {
                  duplicate = (thisDelta == qDeltas[i]);
               }

               if (!duplicate)
               {
                  // it was unique, so use it
                  qDeltas[deltasSet] = thisDelta;
                  deltasSet++;

                  char temp[6];
                  sprintf(temp, "0.%03d", HUNT_GROUP_MAX_Q - thisDelta);
                  contactUri.setFieldParameter(SIP_Q_FIELD, temp);
                  response.setContactField(contactUri.toString(), contactNum);

                  OsSysLog::add( FAC_SIP, PRI_INFO,
                                 "SipRedirectorHunt::lookUp set q-value "
                                 "'%s' on '%s'\n",
                                 temp, thisContact.data());
               }
            } while(duplicate);
         }
         else
         {
            // We've randomized HUNT_GROUP_MAX_CONTACTS,
            // so from here on just set the q value to 0.0
            contactUri.setFieldParameter(SIP_Q_FIELD, "0.0");
            response.setContactField(contactUri.toString(), contactNum);
            OsSysLog::add( FAC_SIP, PRI_WARNING,
                           "SipRedirectorHunt::lookUp overflow - "
                           "set q=0.0 on '%s'\n",
                           thisContact.data());
         }
      }
      else
      {
         // thisContact had a q value set - do not modify it
      }
   } // for all contacts

   delete[] qDeltas;

   return SipRedirector::LOOKUP_SUCCESS;
}

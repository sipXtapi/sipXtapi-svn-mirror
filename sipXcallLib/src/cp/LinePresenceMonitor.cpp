// 
// Copyright (C) 2005 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
// 
// Copyright (C) 2005 Pingtel Corp.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// $$
//////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES

// APPLICATION INCLUDES
#include <os/OsSysLog.h>
#include <utl/UtlSListIterator.h>
#include <net/SipRefreshManager.h>
#include <net/SipSubscribeClient.h>
#include <net/XmlRpcRequest.h>
#include <net/SipPresenceEvent.h>
#include <net/NetMd5Codec.h>
#include <cp/LinePresenceMonitor.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
#define DEFAULT_REFRESH_INTERVAL      180000
#define CONFIG_ETC_DIR                SIPX_CONFDIR

// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
LinePresenceMonitor::LinePresenceMonitor(int userAgentPort,
                                         UtlString& domainName,
                                         UtlString& groupName,
                                         bool local,
                                         Url& remoteServer,
                                         Url& presenceServer)
   : mLock(OsBSem::Q_PRIORITY, OsBSem::FULL)
{
   // Bind the SIP user agent to a port and start it up
   mpUserAgent = new SipUserAgent(userAgentPort, userAgentPort);
   mpUserAgent->start();
   
   mGroupName = groupName;
   mLocal = local;
   mDomainName = domainName;

   if (mLocal)
   {
      // Create a local Sip Dialog Monitor
      mpDialogMonitor = new SipDialogMonitor(mpUserAgent,
                                             domainName,
                                             userAgentPort,
                                             DEFAULT_REFRESH_INTERVAL,
                                             false);
      
      // Add itself to the dialog monitor for state change notification
      mpDialogMonitor->addStateChangeNotifier("Line_Presence_Monitor", this);

      presenceServer.getIdentity(mPresenceServer);
   }
   else
   {
      mRemoteServer = remoteServer;
   }

   // Create the SIP Subscribe Client for subscribing both dialog event and presence event
   mpRefreshMgr = new SipRefreshManager(*mpUserAgent, mDialogManager);
   mpRefreshMgr->start();
   
   mpSipSubscribeClient = new SipSubscribeClient(*mpUserAgent, mDialogManager, *mpRefreshMgr);
   mpSipSubscribeClient->start();
   
   UtlString localAddress;
   OsSocket::getHostIp(&localAddress);
   
   Url url(localAddress);
   url.setHostPort(userAgentPort);
   mContact = url.toString();    
}


// Destructor
LinePresenceMonitor::~LinePresenceMonitor()
{
   if (mpRefreshMgr)
   {
      delete mpRefreshMgr;
   }
   
   if (mpSipSubscribeClient)
   {
      mpSipSubscribeClient->endAllSubscriptions();
      delete mpSipSubscribeClient;
   }

   // Shut down the sipUserAgent
   mpUserAgent->shutdown(FALSE);

   while(!mpUserAgent->isShutdownDone())
   {
      ;
   }

   delete mpUserAgent;
   
   if (mpDialogMonitor)
   {
      // Remove itself to the dialog monitor
      mpDialogMonitor->removeStateChangeNotifier("Line_Presence_Monitor");

      delete mpDialogMonitor;
   }
   
   if (!mSubscribeList.isEmpty())
   {
      mSubscribeList.destroyAll();
   }
}

/* ============================ MANIPULATORS ============================== */


// Assignment operator
LinePresenceMonitor&
LinePresenceMonitor::operator=(const LinePresenceMonitor& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;
}

/* ============================ ACCESSORS ================================= */
bool LinePresenceMonitor::setStatus(const Url& aor, const Status value)
{
   bool result = false;
   
   mLock.acquire();
   
   // We can only use userId to identify the line
   UtlString contact;
   aor.getUserId(contact);
   
   UtlVoidPtr* container = dynamic_cast <UtlVoidPtr *> (mSubscribeList.findValue(&contact));
   LinePresenceBase* line = (LinePresenceBase *) container->getValue();
   if (line)
   {
      // Set the state value in LinePresenceBase
      switch (value)
      {
      case StateChangeNotifier::ON_HOOK:
         if (!line->getState(LinePresenceBase::ON_HOOK))
         {
            line->updateState(LinePresenceBase::ON_HOOK, true);
            result = true;
         }
         
         break;
         
      case StateChangeNotifier::OFF_HOOK:
         if (line->getState(LinePresenceBase::ON_HOOK))
         {
            line->updateState(LinePresenceBase::ON_HOOK, false);
            result = true;
         }
         
         break;

      case StateChangeNotifier::RINGING:
         if (line->getState(LinePresenceBase::ON_HOOK))
         {
            line->updateState(LinePresenceBase::ON_HOOK, false);
            result = true;
         }
         
         break;

      case StateChangeNotifier::PRESENT:
         if (!line->getState(LinePresenceBase::PRESENT))
         {
            line->updateState(LinePresenceBase::PRESENT, true);
            result = true;
         }
         
         break;
         
      case StateChangeNotifier::AWAY:
         if (line->getState(LinePresenceBase::PRESENT))
         {
            line->updateState(LinePresenceBase::PRESENT, false);
            result = false;
         }
         
         break;
         
      default:
      
         OsSysLog::add(FAC_SIP, PRI_ERR, "LinePresenceMonitor::setStatus tried to set an unsupported value %d",
                       value);
      }
   }
      
   mLock.release();
   
   return result;
}

OsStatus LinePresenceMonitor::subscribe(LinePresenceBase* line)
{
   OsStatus result = OS_FAILED;
   mLock.acquire();
   Url* lineUrl = line->getUri();
   
   OsSysLog::add(FAC_SIP, PRI_DEBUG, "LinePresenceMonitor::subscribe subscribing for line %s",
                 lineUrl->toString().data()); 

   if (mLocal)
   {
      if(mpDialogMonitor->addExtension(mGroupName, *lineUrl))
      {
         result = OS_SUCCESS;
      }
      else
      {
         result = OS_FAILED;
      }
   }
   else
   {
      // Use XML-RPC to communicate with the sipX dialog monitor
      XmlRpcRequest request(mRemoteServer, "addExtension");
      
      request.addParam(&mGroupName);
      UtlString contact = lineUrl->toString();
      request.addParam(&contact);

      XmlRpcResponse response;
      if (request.execute(response))
      {
         result = OS_SUCCESS;
      }
      else
      {
         result = OS_FAILED;
      }      
   }
   
   // Send out the SUBSCRIBE to the presence server
   UtlString contactId, resourceId;
   lineUrl->getUserId(contactId);
   if (!mPresenceServer.isNull())
   {
      resourceId = contactId + "@" + mPresenceServer;
      OsSysLog::add(FAC_SIP, PRI_DEBUG,
                    "LinePresenceMonitor::subscribe Sending out the SUBSCRIBE to contact %s",
                    resourceId.data());
   
      
      UtlString toUrl;
      lineUrl->toString(toUrl);
         
      UtlString fromUri = "linePresenceMonitor@" + mDomainName;
      UtlString dialogHandle;
               
      UtlBoolean status = mpSipSubscribeClient->addSubscription(resourceId.data(),
                                                                PRESENCE_EVENT_TYPE,
                                                                fromUri.data(),
                                                                toUrl.data(),
                                                                mContact.data(),
                                                                DEFAULT_REFRESH_INTERVAL,
                                                                (void *) this,
                                                                LinePresenceMonitor::subscriptionStateCallback,
                                                                LinePresenceMonitor::notifyEventCallback,
                                                                dialogHandle);
                  
      if (!status)
      {
         OsSysLog::add(FAC_SIP, PRI_ERR,
                       "LinePresenceMonitor::subscribe Subscription failed to contact %s.",
                       resourceId.data());
      }
      else
      {
         mDialogHandleList.insertKeyAndValue(new UtlString(contactId), new UtlString(dialogHandle));
      }
   }

   // Insert the line to the Subscribe Map
   mSubscribeList.insertKeyAndValue(new UtlString(contactId),
                                    new UtlVoidPtr(line));

   mLock.release();
   
   return result;
}

OsStatus LinePresenceMonitor::unsubscribe(LinePresenceBase* line)
{
   OsStatus result = OS_FAILED;
   mLock.acquire();
   Url* lineUrl = line->getUri();
   
   OsSysLog::add(FAC_SIP, PRI_DEBUG, "LinePresenceMonitor::unsubscribe unsubscribing for line %s",
                 lineUrl->toString().data());
                  
   if (mLocal)
   {
      if (mpDialogMonitor->removeExtension(mGroupName, *lineUrl))
      {
         result = OS_SUCCESS;
      }
      else
      {
         result = OS_FAILED;
      }
   }
   else
   {
      // Use XML-RPC to communicate with the sipX dialog monitor
      XmlRpcRequest request(mRemoteServer, "removeExtension");
      
      request.addParam(&mGroupName);
      UtlString contact = lineUrl->toString();
      request.addParam(&contact);

      XmlRpcResponse response;
      if (request.execute(response))
      {
         result = OS_SUCCESS;
      }
      else
      {
         result = OS_FAILED;
      }      
   }
      
   // Remove the line from the Subscribe Map
   UtlString contact;
   lineUrl->getUserId(contact);
   
   if (!mPresenceServer.isNull())
   {
      UtlString* dialogHandle = dynamic_cast <UtlString *> (mDialogHandleList.findValue(&contact));
      UtlBoolean status = mpSipSubscribeClient->endSubscription(dialogHandle->data());
                  
      if (!status)
      {
         OsSysLog::add(FAC_SIP, PRI_ERR,
                       "LinePresenceMonitor::unsubscribe Unsubscription failed for %s.",
                       contact.data());
      }
      
      mDialogHandleList.destroy(&contact);
   }
   
   mSubscribeList.destroy(&contact);
   
   mLock.release();
   return result;
}

OsStatus LinePresenceMonitor::subscribe(UtlSList& list)
{
   OsStatus result = OS_FAILED;
   mLock.acquire();
   UtlSListIterator iterator(list);
   LinePresenceBase* line;
   while ((line = dynamic_cast <LinePresenceBase *> (iterator())) != NULL)
   {
      subscribe(line);
   }

   mLock.release();
   
   return result;
}

OsStatus LinePresenceMonitor::unsubscribe(UtlSList& list)
{
   OsStatus result = OS_FAILED;
   mLock.acquire();
   UtlSListIterator iterator(list);
   LinePresenceBase* line;
   while ((line = dynamic_cast <LinePresenceBase *> (iterator())) != NULL)
   {
      unsubscribe(line);
   }

   mLock.release();
   
   return result;
}

void LinePresenceMonitor::subscriptionStateCallback(SipSubscribeClient::SubscriptionState newState,
                                                    const char* earlyDialogHandle,
                                                    const char* dialogHandle,
                                                    void* applicationData,
                                                    int responseCode,
                                                    const char* responseText,
                                                    long expiration,
                                                    const SipMessage* subscribeResponse)
{
   OsSysLog::add(FAC_SIP, PRI_DEBUG, "LinePresenceMonitor::subscriptionStateCallback is called with responseCode = %d (%s)",
                 responseCode, responseText); 
}                                            


void LinePresenceMonitor::notifyEventCallback(const char* earlyDialogHandle,
                                              const char* dialogHandle,
                                              void* applicationData,
                                              const SipMessage* notifyRequest)
{
   // Receive the notification and process the message
   LinePresenceMonitor* pThis = (LinePresenceMonitor *) applicationData;
   
   pThis->handleNotifyMessage(notifyRequest);
}


void LinePresenceMonitor::handleNotifyMessage(const SipMessage* notifyMessage)
{
   Url fromUrl;
   notifyMessage->getFromUrl(fromUrl);
   UtlString contact;
   fromUrl.getIdentity(contact);
   
   OsSysLog::add(FAC_SIP, PRI_DEBUG, "LinePresenceMonitor::handleNotifyMessage receiving a notify message from %s",
                 contact.data()); 
   
   const HttpBody* notifyBody = notifyMessage->getBody();
   
   if (notifyBody)
   {
      UtlString messageContent;
      int bodyLength;
      
      notifyBody->getBytes(&messageContent, &bodyLength);
      
      // Parse the content and store it in a SipPresenceEvent object
      SipPresenceEvent* sipPresenceEvent = new SipPresenceEvent(contact, messageContent);
      
      UtlString id;
      NetMd5Codec::encode(contact, id);
      Tuple* tuple = sipPresenceEvent->getTuple(id);
      
      UtlString status;
      tuple->getStatus(status);
      
      Url contactUrl(contact);
      if (status.compareTo(STATUS_CLOSE) == 0)
      {
         setStatus(contactUrl, StateChangeNotifier::AWAY);
      }
      else
      {     
         setStatus(contactUrl, StateChangeNotifier::PRESENT);
      }
      
      delete sipPresenceEvent;
   }
   else
   {
      OsSysLog::add(FAC_SIP, PRI_DEBUG, "LinePresenceMonitor::handleNotifyMessage receiving an empty notify body from %s",
                    contact.data()); 
   }
}

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */


/* ============================ FUNCTIONS ================================= */


// 
// 
// Copyright (C) 2004 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
// 
// Copyright (C) 2004 Pingtel Corp.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// $$
// Author: Dan Petrie (dpetrie AT SIPez DOT com)

//////////////////////////////////////////////////////////////////////////////
#ifndef _SipSubscribeClient_h_
#define _SipSubscribeClient_h_

// SYSTEM INCLUDES

// APPLICATION INCLUDES

#include <os/OsDefs.h>
#include <os/OsServerTask.h>
#include <utl/UtlHashMap.h>
#include <net/SipRefreshManager.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// FORWARD DECLARATIONS
class SipMessage;
class SipUserAgent;
class SipDialogMgr;
class SipRefreshManager;
class SubscribeClientState;

// TYPEDEFS

//! Class for containing SIP dialog state information
/*! In SIP a dialog is defined by the SIP Call-Id
 *  header and the tag parameter from the SIP To
 *  and From header fields.  An early dialog has
 *  has only the tag set on one side, the transaction
 *  originator side.  In the initial transaction the
 *  the originator tag in in the From header field.
 *  The final destination sets the To header field
 *  tag in the initial transaction.
 *
 * \par Local and Remote
 *  As the To and From fields get swapped depending
 *  upon which side initiates a transaction (i.e.
 *  sends a request) local and remote are used in
 *  SipDialog to label tags, fields and information.
 *  Local and Remote are unabiquous when used in
 *  an end point.  In a proxy context the SipDialog
 *  can still be used.  One can visualize the
 *  sides of the dialog by thinking Left and Right
 *  instead of local and remote.
 */
class SipSubscribeClient : public OsServerTask
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

    enum SubscriptionState
    {
        SUBSCRIPTION_UNKNOWN,
        SUBSCRIPTION_INITIATED, // Early dialog
        SUBSCRIPTION_SETUP,     // Established dialog
        SUBSCRIPTION_FAILED,    // Failed dialog setup or refresh
        SUBSCRIPTION_TERMINATED // Ended dialog
    };

typedef void (*SubscriptionStateCallback) (SipSubscribeClient::SubscriptionState newState,
                                           const char* earlyDialogHandle,
                                           const char* dialogHandle,
                                           void* applicationData,
                                           int responseCode,
                                           const char* responseText,
                                           long expiration,
                                           const SipMessage* subscribeResponse);

typedef void (*NotifyEventCallback) (const char* earlyDialogHandle,
                                     const char* dialogHandle,
                                     void* applicationData,
                                     const SipMessage* notifyRequest);

/* ============================ CREATORS ================================== */

    //! Default Dialog constructor
    SipSubscribeClient(SipUserAgent& userAgent, 
                       SipDialogMgr& dialogMgr,
                       SipRefreshManager& refreshMgr);


    //! Destructor
    virtual
    ~SipSubscribeClient();


/* ============================ MANIPULATORS ============================== */

    //! Create a SIP event subscription for the given SUBSCRIBE header information
    /*! 
     *  Returns TRUE if the SUBSCRIBE request was sent and the 
     *  Subscription state proceeded to SUBSCRIPTION_INITIATED.
     *  Returns FALSE if the SUBSCRIBE request was not able to
     *  be sent, the subscription state is set to SUSCRIPTION_FAILED.
     */
    UtlBoolean addSubscription(const char* resourceId,
                               const char* eventHeaderValue,
                               const char* fromFieldValue,
                               const char* toFieldValue,
                               const char* contactFieldValue,
                               int subscriptionPeriodSeconds,
                               void* applicationData,
                               const SubscriptionStateCallback subscriptionStateCallback,
                               const NotifyEventCallback notifyEventsCallback,
                               UtlString& earlyDialogHandle);

    //! Create a SIP event subscription for the given SUBSCRIBE request
    /*! 
     *  Returns TRUE if the SUBSCRIBE request was sent and the 
     *  Subscription state proceeded to SUBSCRIPTION_INITIATED.
     *  Returns FALSE if the SUBSCRIBE request was not able to
     *  be sent, the subscription state is set to SUSCRIPTION_FAILED.
     */
    UtlBoolean addSubscription(SipMessage& subscriptionRequest,
                               void* applicationData,
                               const SubscriptionStateCallback subscriptionStateCallback,
                               const NotifyEventCallback notifyEventsCallback,
                               UtlString& earlyDialogHandle);

    //! End the SIP event subscription indicated by the dialog handle
    /*! If the given dialogHandle is an early dialog it will end any
     *  established or early dialog subscriptions.  Typically the
     *  application SHOULD use the established dialog handle.  This
     *  method can also be used to end one of the dialogs if multiple
     *  subsription dialogs were created as a result of a single 
     *  subscribe request.  The application will get multiple 
     *  SUBSCRIPTION_SETUP SubscriptionStateCallback events when
     *  multiple dialogs are created as a result of a single SUBSCRIBE.
     *  To end one of the subscriptions the application should use
     *  the setup dialogHandle provided by the SubscriptionStateCallback.
     */
    UtlBoolean endSubscription(const char* dialogHandle);


    //! End all subscriptions
    void endAllSubscriptions();

    //! Handler for NOTIFY requests
    UtlBoolean handleMessage(OsMsg &eventMessage);

/* ============================ ACCESSORS ================================= */

    //! Create a debug dump of all the client states
    int dumpStates(UtlString& dumpString);

    //! Get a string representation of the client state enumeration
    static void getSubscriptionStateEnumString(enum SubscriptionState stateValue, 
                                               UtlString& stateString);

/* ============================ INQUIRY =================================== */

    //! Get a count of the subscriptions which have been added
    int countSubscriptions();

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
    //! Callback to handle subscription state changes from the refresh manager
    /*! RefreshStateCallback
     */
    static void refreshCallback(SipRefreshManager::RefreshRequestState newState,
                               const char* earlyDialogHandle,
                               const char* dialogHandle,
                               void* subscribeClientPtr,
                               int responseCode,
                               const char* responseText,
                               long expiration, // epoch seconds
                               const SipMessage* subscribeResponse);

    //! Handle incoming notify request
    void handleNotifyRequest(const SipMessage& notifyRequest);

    //! Add the client state to the container
    void addState(SubscribeClientState& clientState);

    //! find the state from the container that matches the dialog
    /* Assumes external locking
     */
    SubscribeClientState* getState(const UtlString& dialogHandle);

    //! remove the state from the container that matches the dialog
    /* Assumes external locking
     */
    SubscribeClientState* removeState(UtlString& dialogHandle);

    //! lock for single thread use
    void lock();
    //! lock for single thread use
    void unlock();

    //! Construct a call-id
    void getNextCallId(const char* resourceId, 
                       const char* eventHeaderValue, 
                       const char* fromFieldValue, 
                       const char* contactFieldValue,
                       UtlString& callId);

    //! generate a from tag
    void getNextFromTag(const char* resourceId, 
                        const char* eventHeaderValue, 
                        const char* fromFieldValue, 
                        const char* contactFieldValue,
                        UtlString& fromTag);

    //! Copy constructor NOT ALLOWED
    SipSubscribeClient(const SipSubscribeClient& rSipSubscribeClient);

    //! Assignment operator NOT ALLOWED
    SipSubscribeClient& operator=(const SipSubscribeClient& rhs);

    SipUserAgent* mpUserAgent;
    SipDialogMgr* mpDialogMgr;
    SipRefreshManager* mpRefreshMgr;
    UtlHashMap mSubscriptions; // state info. for each subscription
    UtlHashMap mEventTypes; // SIP event types that we want NOTIFY requests for
    OsMutex mSubcribeClientMutex;
    int mCallIdCount;
    int mTagCount;
};

/* ============================ INLINE METHODS ============================ */

#endif  // _SipSubscribeClient_h_

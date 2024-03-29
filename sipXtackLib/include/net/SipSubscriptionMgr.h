//
// Copyright (C) 2007-2008 SIPez LLC  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2004-2008 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////
// Author: Dan Petrie (dpetrie AT SIPez DOT com)

#ifndef _SipSubscriptionMgr_h_
#define _SipSubscriptionMgr_h_

// SYSTEM INCLUDES

// APPLICATION INCLUDES

#include <os/OsDefs.h>
#include <os/OsMsgQ.h>
#include <os/OsMutex.h>
#include <utl/UtlDefs.h>
#include <utl/UtlHashMap.h>
#include <utl/UtlHashBag.h>
#include <net/SipDialogMgr.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// FORWARD DECLARATIONS
class SipMessage;
class UtlString;
class SipDialogMgr;

// TYPEDEFS

//! Class for maintaining SUBSCRIBE dialog information in subscription server
/*! 
 *
 * \par 
 */
class SipSubscriptionMgr
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:



/* ============================ CREATORS ================================== */

    //! Default constructor
    SipSubscriptionMgr();

    //! Destructor
    virtual
    ~SipSubscriptionMgr();

/* ============================ MANIPULATORS ============================== */

    //! Add/Update subscription for the given SUBSCRIBE request
    virtual UtlBoolean updateDialogInfo(const SipMessage& subscribeRequest,
                                        const UtlString& resourceId,
                                        const UtlString& eventTypeKey,
                                        OsMsgQ* subscriptionTimeoutQueue,
                                        UtlString& subscribeDialogHandle,
                                        UtlBoolean& isNew,
                                        UtlBoolean& isExpired,
                                        SipMessage& subscribeResponse);

    //! Set the subscription dialog information and cseq for the next NOTIFY request
    virtual UtlBoolean getNotifyDialogInfo(const UtlString& subscribeDialogHandle,
                                           SipMessage& notifyRequest);

    //! Construct a NOTIFY request for each subscription/dialog subscribed to the given resourceId and eventTypeKey
    /*! Allocates a SipMessage* array and allocates a SipMessage and sets the
     * dialog information for the NOTIFY request for each subscription.
     *  \param numNotifiesCreated - number of pointers to NOTIFY requests in
     *         the returned notifyArray
     *  \param notifyArray - if numNotifiesCreated > 0 array is allocated of
     *         sufficient size to hold a SipMessage for each subscription.
     */
    virtual UtlBoolean createNotifiesDialogInfo(const char* resourceId,
                                                const char* eventTypeKey,
                                                int& numNotifiesCreated,
                                                UtlString**& acceptHeaderValuesArray,
                                                SipMessage**& notifyArray);

    //! frees up the notifies created in createNotifiesDialogInfo
    virtual void freeNotifies(int numNotifies,
                              UtlString** acceptHeaderValues,
                              SipMessage** notifiesArray);

    //! End the dialog for the subscription indicated, by the dialog handle
    /*! Finds a matching dialog and expires the subscription if it has
     *  not already expired.
     *  \param dialogHandle - a fully established SIP dialog handle
     *  Returns TRUE if a matching dialog was found regardless of
     *  whether the subscription was already expired or not.
     */
    virtual UtlBoolean endSubscription(const UtlString& dialogHandle);

    //! Dump to syslog states considered old
    int dumpOldSubscriptions(long oldEpochTimeSeconds);

    //! Remove old subscriptions that expired before given date
    int removeOldSubscriptions(long oldEpochTimeSeconds);

    //! Set maximum subscription period in seconds
    void setMaxExpiration(int expiresSeconds);

/* ============================ ACCESSORS ================================= */


    //! Get the dialog manager
    /*! WARNING: the application must be aware of the lifetime of
     *  the dialog manager, as no reference counting is done on
     *  the dialog manager.  The application is responsible for
     *  knowing when the dialog manager will go way.
     */
    SipDialogMgr* getDialogMgr();

    //! Get count of subscription states/dialogs being managed
    int getStateCount();

/* ============================ INQUIRY =================================== */

    //! inquire if the dialog exists
    virtual UtlBoolean dialogExists(UtlString& dialogHandle);

    //! inquire if the dialog has already expired
    virtual UtlBoolean isExpired(UtlString& dialogHandle);

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
    //! Copy constructor NOT ALLOWED
    SipSubscriptionMgr(const SipSubscriptionMgr& rSipSubscriptionMgr);

    //! Assignment operator NOT ALLOWED
    SipSubscriptionMgr& operator=(const SipSubscriptionMgr& rhs);

    //! lock for single thread use
    void lock();

    //! unlock for use
    void unlock();

    int mEstablishedDialogCount;
    OsMutex mSubscriptionMgrMutex;
    SipDialogMgr mDialogMgr;
    int mMinExpiration;
    int mDefaultExpiration;
    int mMaxExpiration;

    // Container for the subscritption states
    UtlHashMap mSubscriptionStatesByDialogHandle;

    // Index to subscription states in mSubscriptionStatesByDialogHandle
    // indexed by the resourceId and eventTypeKey
    UtlHashBag mSubscriptionStateResourceIndex;
};

/* ============================ INLINE METHODS ============================ */

#endif  // _SipSubscriptionMgr_h_

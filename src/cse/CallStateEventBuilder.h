//
// Copyright (C) 2004 SIPfoundry Inc.
// License by SIPfoundry under the LGPL license.
// 
// Copyright (C) 2004 Pingtel Corp.
// Licensed to SIPfoundry under a Contributor Agreement.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _CallStateEventBuilder_h_
#define _CallStateEventBuilder_h_

// SYSTEM INCLUDES

class UtlString;

// APPLICATION INCLUDES
// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

/**
 * The base class for any means of building a record of a call state event.  
 * Classes derived from this one may build events in different representations for
 * different purposes.
 *
 * Before any call events are generated, an observer event indicating the start
 * of a new sequence event should be called:
 *  - observerEvent(0, universalEpochTime, ObserverReset, "AuthProxy Restart");
 *
 * Call Events are generated by a sequence of calls, begining with one of the call*Event
 * methods and ending with callEventComplete.  The specific calls required for each
 * event type are documented with the method that begins the sequence.
 */
class CallStateEventBuilder 
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
  public:

/* ============================ CREATORS ================================== */

   /// Instantiate an event builder and set the observer name for its events
   CallStateEventBuilder(const char* observerDnsName ///< the DNS name to be recorded in all events
                         );

   /// Destructor
   ~CallStateEventBuilder();


   /// Meta-events about the observer itself
   typedef enum
      {
         ObserverReset = 0, ///< starts a new sequence number stream
         ObserverError      ///< an internal error was detected in the observer
                            ///  some events may have been lost.
      } ObserverEvent;

   /**
    * Generate a metadata event.
    * This method generates a complete event - it does not require that the callEventComplete method be called.
    */
   void observerEvent(int sequenceNumber, ///< for ObserverReset, this should be zero
                      int timestamp,      ///< UTC in seconds since the unix epoch
                      ObserverEvent eventCode,
                      const char* eventMsg ///< for human consumption
                      );

   /// Begin a Call Request Event - an INVITE without a to tag has been observed
   /**
    * Requires:
    *   - callRequestEvent
    *   - addCallData (the toTag in the addCallRequest will be a null string)
    *   - addEventVia (at least for via index zero)
    *   - completeCallEvent
    */
   void callRequestEvent(int sequenceNumber,
                         int timestamp,
                         const UtlString& contact
                         );

   /// Begin a Call Setup Event - a 2xx response to an INVITE has been observed
   /**
    * Requires:
    *   - callSetupEvent
    *   - addCallData
    *   - addEventVia (at least for via index zero)
    *   - completeCallEvent
    */
   void callSetupEvent(int sequenceNumber,
                       int timestamp,
                       const UtlString& contact
                       );

   /// Begin a Call Failure Event - an error response to an INVITE has been observed
   /**
    * Requires:
    *   - callFailureEvent
    *   - addCallData
    *   - addEventVia (at least for via index zero)
    *   - completeCallEvent
    */
   void callFailureEvent(int sequenceNumber,
                         int timestamp,
                         int statusCode,
                         const UtlString& statusMsg
                         );

   /// Begin a Call End Event - a BYE request has been observed
   /**
    * Requires:
    *   - callEndEvent
    *   - addCallData
    *   - addEventVia (at least for via index zero)
    *   - completeCallEvent
    */
   void callEndEvent(const int sequenceNumber,
                     const int timestamp
                     );

   /// Add the dialog and call information for the event being built.
   void addCallData(const UtlString& callId,
                    const UtlString& fromTag,  /// may be a null string
                    const UtlString& toTag,    /// may be a null string
                    const UtlString& fromField,
                    const UtlString& toField
                    );
   
   /// Add a via element for the event
   /**
    * Record the specified Via from the message for this event, where 0
    * indicates the Via inserted by the message originator.  At least that
    * via should be added for any event.
    */
   void addEventVia(int index,
                    const UtlString& via
                    );

   /// Indicates that all information for the current call event has been added.
   void completeCallEvent();

/* //////////////////////////// PROTECTED ///////////////////////////////// */
  protected:
   const char* observerName;

   /**
    * Input events for the builderStateOk FSM.
    * Each of these events is passed to the state machine from the
    * correspondingly named public builder method.
    */
   typedef enum
      {
         BuilderReset,    ///< for observerEvent(ObserverError)
         BuilderStart,    ///< for observerEvent(ObserverReset)
         CallRequestEvent,
         CallSetupEvent,
         CallFailureEvent,
         CallEndEvent,
         AddCallData,
         AddVia,
         CompleteCallEvent
      } BuilderMethod;

   /// Event validity checking state machine
   /**
    * Each of the public builder methods calls this finite state machine to determine
    * whether or not the call is valid.  This allows all builders to share the same
    * rules for what calls are allowed and required when constructing each event type
    * (for this reason, this should not be redefined by derived classes).
    *
    * In the initial state, the only valid call is builderStateOk(BuilderStart)
    *
    * @returns
    *   - true if method is valid now
    *   - false if method is not valid now
    *
    * If this routine returns false, the caller should clear all saved state and then
    * call this routine again, passing BuilderReset.  The finite state machine is then
    * reset to the initial state.
    */
   bool builderStateIsOk(BuilderMethod method);

/* //////////////////////////// PRIVATE /////////////////////////////////// */
  private:
   int  buildState; // used by builderStateIsOk

   /// no copy constructor
   CallStateEventBuilder(const CallStateEventBuilder&);

   /// no assignment operator
   CallStateEventBuilder& operator=(const CallStateEventBuilder&);
   
} ;

/* ============================ INLINE METHODS ============================ */

#endif    // _CallStateEventBuilder_h_


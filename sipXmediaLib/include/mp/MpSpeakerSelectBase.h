//
// Copyright (C) 2008 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2008 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// Author: Sergey Kostanbaev <Sergey DOT Kostanbaev AT sipez DOT com>

#ifndef _MpSpeakerSelectBase_h_
#define _MpSpeakerSelectBase_h_

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlDefs.h>
#include <os/OsStatus.h>
#include <utl/UtlString.h>
#include "mp/MpTypes.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

/**
*  Base class for all Speaker Selection (SS) algorithms.
*
*  To create concrete class you could directly instantiate it or use
*  MpSpeakerSelectBase::createInstance() static method for greater flexibility.
*
*  @nosubgrouping
*/

class MpSpeakerSelectBase
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

/* ============================ CREATORS ================================== */
///@name Creators
//@{

     /// Initialize SS to initial state with zero conferee
   virtual OsStatus init(int maxParticipants) = 0;
     /**<
     *  @param[in] maxParticipants - maximum number of participants in conference
     *
     *  Should be called before any other class methods. All participants
     *  after initialization are disabled, to enable it call
     *  enableParticipant()
     */

     /// Factory method for SS algorithms creation.
   static MpSpeakerSelectBase *createInstance(const UtlString &name = "");
     /**<
     *  @param[in] ssName - name of SS algorithm to use. Use empty string
     *             to get default algorithm.
     *
     *  @note To date we have no available SS algorithms in open-source,
     *        so NULL is always returned.
     *
     *  @returns If appropriate SS algorithm is not found, default one is
     *           returned.
     */

     /// Destructor
   virtual ~MpSpeakerSelectBase() {};

//@}

/* ============================ MANIPULATORS ============================== */
///@name Manipulators
//@{

     /// Reset algorithm state to initial and prepare for processing of new data.
   virtual OsStatus reset() = 0;
     /**<
     *  It's supposed that init() will not be called after reset(). So reset()
     *  must turn algorithm to the state as right after calling init().
     *  Maximum number of participants intentionally is not changed, to prevent
     *  memory reallocation.
     */

     /// Change status of selected participant
   virtual OsStatus enableParticipant(int num, UtlBoolean newState) = 0;
     /**<
     * Use this method to enable processing for newly added participants and 
     * disable processing for removed participants. Data from disabled 
     * participants are just ignored
     *
     * @param[in] num - number of participant, starting from zero
     * @param[in] newState - pass TRUE to enable processing of this participant,
     *                       FALSE to disable.
     *
     * @returns Method returns OS_SUCCESS if processing is ok
     */

     /// Detect speech presence
   virtual OsStatus processFrame(MpSpeechParams* speechParams[],
                                 int frameSize) = 0;
     /**<
     * @param[in] speechParams - parameters of bridges
     * @param[in] frameSize - number of milliseconds in frame
     *
     * @returns Method returns OS_SUCCESS if processing is ok,
     *          otherwise OS_FAILED
     */

     /// Set algorithmic parameter
   virtual OsStatus setParam(const char* paramName, void* value) = 0;
     /**<
     * @param[in] paramName - name of parameter.
     * @param[in] value - pointer to a value.
     *
     * @returns Method returns OS_SUCCESS if parameter has been set,
     *          otherwise OS_FAILED
     */

//@}

/* ============================ ACCESSORS ================================= */
///@name Accessors
//@{

//@}

/* ============================ INQUIRY =================================== */
///@name Inquiry
//@{

//@}

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // _MpSpeakerSelectBase_h_

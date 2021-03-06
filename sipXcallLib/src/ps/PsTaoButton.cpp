//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
#include <assert.h>

// APPLICATION INCLUDES
#include "ps/PsTaoButton.h"
#include <os/OsLock.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
PsTaoButton::PsTaoButton() :
mbNotSetBefore(FALSE),
mpAssocLamp(NULL)
{
}

PsTaoButton::PsTaoButton(const UtlString& rComponentName, int componentType) :
PsTaoComponent(rComponentName, componentType),
mbNotSetBefore(FALSE),
mpAssocLamp(NULL)
{
}

// Copy constructor
PsTaoButton::PsTaoButton(const PsTaoButton& rPsTaoButton) :
mbNotSetBefore(FALSE),
mpAssocLamp(NULL)
{
}

// Destructor
PsTaoButton::~PsTaoButton()
{
        mButtonInfo.remove(0);
}

/* ============================ MANIPULATORS ============================== */

// Assignment operator
PsTaoButton&
PsTaoButton::operator=(const PsTaoButton& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;

   return *this;
}

void PsTaoButton::buttonDown(void)
{
        mButtonState = DOWN;
}

void PsTaoButton::buttonUp(void)
{
        mButtonState = UP;
}

void PsTaoButton::buttonPress(void)
{
}

UtlBoolean PsTaoButton::setInfo(const UtlString& rInfo)
{
        if (mbNotSetBefore || mButtonInfo != rInfo)
        {
                mButtonInfo = UtlString(rInfo);
                mbNotSetBefore = FALSE;
                return TRUE;
        }
        return FALSE;
}

/* ============================ ACCESSORS ================================= */

PsTaoLamp* PsTaoButton::getAssociatedPhoneLamp(void)
{
        return mpAssocLamp;
}

void PsTaoButton::getInfo(UtlString& rInfo)
{
        rInfo = UtlString(mButtonInfo);
}

/* ============================ INQUIRY =================================== */

UtlBoolean PsTaoButton::isButtonDown(void)
{
        return (mButtonState == DOWN);
}

UtlBoolean PsTaoButton::isButtonRepeating(void)
{
        return mIsRepeating;
}
/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

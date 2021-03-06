//
// Copyright (C) 2006-2012 SIPez LLC.  All rights reserved.
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

#include <string.h>
#include <ctype.h>
#include <stdio.h>

// APPLICATION INCLUDES
#include <net/NameValuePair.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
int NameValuePair::count = 0;
OsMutex   NameValuePair::mCountLock(OsMutex::Q_PRIORITY);

int getNVCount()
{
        return NameValuePair::count;
}
/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
NameValuePair::NameValuePair(const char* name, const char* value) :
        UtlString(name)
{
   valueString = NULL;
   setValue(value);

#ifdef TEST_ACCOUNT
        mCountLock.acquire();
   count++;
        mCountLock.release();
#endif
}

// Copy constructor
NameValuePair::NameValuePair(const NameValuePair& rNameValuePair) :
UtlString(rNameValuePair)
{
    // Slow copy does implicit const./dest of UtlString
    //((UtlString) *this) = rNameValuePair;

    // Use parent copy constructor
    this->UtlString::operator=(rNameValuePair);

    valueString = NULL;
    setValue(rNameValuePair.valueString);
}

// Destructor
NameValuePair::~NameValuePair()
{
   if(valueString)
   {
                delete[] valueString;
                valueString = 0;
   }

#ifdef TEST_ACCOUNT
        mCountLock.acquire();
   count--;
        mCountLock.release();
#endif
}

/* ============================ MANIPULATORS ============================== */

// Assignment operator
NameValuePair&
NameValuePair::operator=(const NameValuePair& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;

   ((UtlString&) *this) = rhs.data();
   setValue(rhs.valueString);

   return *this;
}

/* ============================ ACCESSORS ================================= */
const char* NameValuePair::getValue()
{
        return(valueString);
}

void NameValuePair::setValue(const char* newValue)
{
        if(newValue)
        {
                int len = strlen(newValue);

                if(valueString && len > (int) strlen(valueString))
                {
                        delete[] valueString;
                        valueString = new char[len + 1];
                }
                else
                if (!valueString)
                        valueString = new char[len + 1];

                strcpy(valueString, newValue);
        }
        else if(valueString)
        {
                delete[] valueString;
                valueString = 0;
        }
}

/* ============================ INQUIRY =================================== */

UtlBoolean NameValuePair::isInstanceOf(const UtlContainableType type) const
{
    // Check if it is my type and the defer parent type comparisons to parent
    return(areSameTypes(type, NameValuePair::TYPE) ||
           UtlString::isInstanceOf(type));
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

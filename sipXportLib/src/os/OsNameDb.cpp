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

// Keep OsNameDbInit.h as the first include!
// See OsNameDbInit class description for more information.
#include "os/OsNameDbInit.h"

#include "os/OsNameDb.h"
#include "os/OsReadLock.h"
#include "os/OsWriteLock.h"
#include "utl/UtlIntPtr.h"
#include "utl/UtlString.h"

// EXTERNAL FUNCTIONS

// EXTERNAL VARIABLES

// CONSTANTS
static const int DEFAULT_NAMEDB_SIZE = 100;

// STATIC VARIABLE INITIALIZATIONS
OsNameDb* OsNameDb::spInstance;

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Return a pointer to the singleton object
OsNameDb* OsNameDb::getNameDb()
{
   return spInstance;
}

// Destructor
// Since the name database is a singleton object, this destructor should
// not get called unless we are shutting down the system.
OsNameDb::~OsNameDb()
{
   spInstance = NULL;
}

/* ============================ MANIPULATORS ============================== */

// Add the key-value pair to the name database
// Return OS_SUCCESS if successful, OS_NAME_IN_USE if the key is already in
// the database.
OsStatus OsNameDb::insert(const UtlString& rKey,
                          const intptr_t value)
{
   OsWriteLock          lock(mRWLock);
   UtlString* pDictKey;
   UtlIntPtr* pDictValue;
   UtlString* pInsertedKey;

   pDictKey   = new UtlString(rKey);
   pDictValue = new UtlIntPtr(value);
   pInsertedKey = (UtlString*)
                  mDict.insertKeyAndValue(pDictKey, pDictValue);

   if (pInsertedKey == NULL)
   {                             // insert failed
      delete pDictKey;           // clean up the key and value objects
      delete pDictValue;

      return OS_NAME_IN_USE;
   }
   else
   {
      return OS_SUCCESS;
   }
}

// Remove the indicated key-value pair from the name database.
// If pValue is non-NULL, the value for the key-value pair is returned
// via pValue.
// Return OS_SUCCESS if the lookup is successful, return OS_NOT_FOUND
// if there is no match for the specified key.
OsStatus OsNameDb::remove(const UtlString& rKey,
                          intptr_t* pValue)
{
   OsWriteLock          lock(mRWLock);
   OsStatus   result = OS_NOT_FOUND;
   UtlString* pDictKey;
   UtlIntPtr* pDictValue;

   pDictKey =
      (UtlString*)
      mDict.removeKeyAndValue(&rKey, (UtlContainable*&) pDictValue);

   // If a value was found and removed ...
   if (pDictKey != NULL)
   {
      // If the caller provided a pointer through which to return the
      // integer value, do so.
      if (pValue != NULL)
      {
         *pValue = pDictValue->getValue();
      }

      // Delete the key and value objects.
      delete pDictKey;
      delete pDictValue;

      result =  OS_SUCCESS;
   }

   //  Return success or failure as appropriate.
   return result;
}
   
/* ============================ ACCESSORS ================================= */

// Retrieve the value associated with the specified key.
// If pValue is non-NULL, the value is returned via pValue.
// Return OS_SUCCESS if the lookup is successful, return OS_NOT_FOUND if
// there is no match for the specified key.
OsStatus OsNameDb::lookup(const UtlString& rKey,
                          intptr_t* pValue)
{
   OsReadLock lock(mRWLock);
   OsStatus   result = OS_NOT_FOUND;
   UtlIntPtr* pDictValue;

   pDictValue = (UtlIntPtr*)
                mDict.findValue(&rKey); // perform the lookup

   if (pDictValue != NULL)
   {
      if (pValue != NULL)       // if we have a valid pointer,
      {                         //  return the corresponding value
         *pValue = pDictValue->getValue();
      }
      result = OS_SUCCESS;
   }

   return result;
}

// Return the number of key-value pairs in the name database
int OsNameDb::numEntries(void)
{
   OsReadLock lock(mRWLock);

   return mDict.entries();
}

/* ============================ INQUIRY =================================== */

// Return TRUE if the name database is empty
UtlBoolean OsNameDb::isEmpty(void)
{
   return (numEntries() == 0);
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

// Constructor (only available internal to the class)
OsNameDb::OsNameDb() :
   mDict(),
   mRWLock(OsRWMutex::Q_PRIORITY)
{
   // no other work required
}

/* ============================ FUNCTIONS ================================= */



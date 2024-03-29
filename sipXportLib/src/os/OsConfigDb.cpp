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
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

// APPLICATION INCLUDES
#include "os/OsConfigDb.h"
#include "os/OsReadLock.h"
#include "os/OsWriteLock.h"
#include "os/OsStatus.h"
#include "os/OsUtil.h"
#include "os/OsFS.h"
#include "os/OsSysLog.h"
#include "os/OsConfigEncryption.h"
#include "utl/UtlSortedListIterator.h"
#include "utl/UtlSList.h"
#include "utl/UtlSListIterator.h"
#include <utl/UtlHashBag.h>
#include <utl/UtlTokenizer.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
#define MAX_FILELINE_SIZE 1024
#define DB_LINE_FORMAT "%s : %s\r\n"

// STATIC VARIABLE INITIALIZATIONS
static OsConfigEncryption *gEncryption = NULL;
const UtlContainableType DbEntry::TYPE = "DbEntry";

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Default Constructor
OsConfigDb::OsConfigDb()
    :  mRWMutex(OsRWMutex::Q_PRIORITY),
       mDb(),
       mCapitalizeName(FALSE)
{
}

// Destructor
OsConfigDb::~OsConfigDb()
{
    OsWriteLock lock(mRWMutex);    // take lock for writing
    mDb.destroyAll();
}

/* ============================ MANIPULATORS ============================== */

OsStatus OsConfigDb::loadFromFile(const char *filename)
{
   if (filename == NULL)
   {
       return OS_INVALID_ARGUMENT;
   }

   OsStatus retval = OS_UNSPECIFIED;
   OsWriteLock lock(mRWMutex);    // take lock for writing while the database
                                  //  is loaded

   setIdentityLabel(filename);

   OsConfigEncryption *encryption = getEncryption();
   if (encryption != NULL && encryption->isNormallyEncrypted(this))
   {
       retval = loadFromEncryptedFile(filename);
   }
   else
   {
       FILE *fp = fopen(filename,"rb");
       if (fp)
       {
           retval = loadFromUnencryptedFile(fp);
           fclose(fp);
       }
   }

   return retval;
}

void OsConfigDb::dump()
{
    for (unsigned int i = 0; i < mDb.entries(); i++)
    {
        DbEntry *e = (DbEntry *)mDb.at(i);
        osPrintf(DB_LINE_FORMAT, e->key.data(), e->value.data());
    }
}

// Buffer CANNOT be encrypted
OsStatus OsConfigDb::loadFromBuffer(const char *buff)
{
    if (buff == NULL)
        return OS_INVALID_ARGUMENT;

    OsWriteLock lock(mRWMutex);    // take lock for writing while the database
                                  //  is loaded

    return loadFromUnencryptedBuffer(buff);
}

OsStatus OsConfigDb::loadFromFile(FILE* fp)
{
    OsWriteLock lock(mRWMutex);     // take lock for reading while the database
                                   //  is stored

    // TODO: Support encrypted files loaded via FILE instance. Note however, this is
    // of little use because no file name is generally known and encryption/decryption
    // rules generally rely on this.  If callers know the filename, try calling
    // loadFromFile with filename
    return loadFromUnencryptedFile(fp);
}


   // Undo the previous hack
#  if defined(_VXWORKS)
#  undef OK
#  endif

UtlBoolean OsConfigDb::parseLine(const char* fileLine, UtlBoolean capitalizeName, const char* fileLabel,
                                 UtlString& parameterName, UtlString& parameterValue)
{
   UtlBoolean isParameter = FALSE;

   /* Format of a config line is:
    *     whitespace name whitespace : whitespace value whitespace EOL
    */

   const char* p = fileLine;    // Scanning pointer.

   // Skip initial white space.
   while (*p != '\0' && isspace(*p))
   {
      p++;
   }

   // If the first non-whitespace character is '#', this is a comment line.
   // Similarly, if it is NUL, this line is empty.
   // In either case, ignore this line.
   if (*p != '#' && *p != '\0')
   {
      // Save start of name.
      const char* name_start = p;

      // The name continues till EOL, whitespace, or colon.
      while (*p != '\0' && !isspace(*p) && *p != ':')
      {
         p++;
      }

      // Save length of name.
      ptrdiff_t name_len = p - name_start;

      // If the found name is null, do nothing.
      // (Probably due to an empty line.)
      if (name_len != 0)
      {
         // Skip whitespace.
         while (*p != '\0' && isspace(*p))
         {
            p++;
         }
         // Skip colon, if any.
         // (There should be a colon, but if the line's format is bad,
         // it might not be there.)
         if (*p == ':')
         {
            p++;

            // Skip whitespace.
            while (*p != '\0' && isspace(*p))
            {
               p++;
            }

            // Save start of value.
            const char* value_start = p;

            // Scan string back from the end skipping whitespace.
            p = fileLine + strlen(fileLine);
            while (p > value_start && isspace(p[-1]))
            {
               p--;
            }

            // Save length of value.
            ptrdiff_t value_len = p - value_start;

            // Construct UtlString's, which insertEntry(*, *) needs as arguments.
            UtlString name(name_start, name_len);
            UtlString value(value_start, value_len);

            if(!name.isNull() || !value.isNull())
            {
               isParameter = TRUE;
            }

            // Capitalize the name if required.
            if (capitalizeName)
            {
               name.toUpper();
            }

            parameterName = name;
            parameterValue = value;
            // Insert the entry.
            //insertEntry(name, value);
         }
         else
         {
            // The colon was not found.
            OsSysLog::add(FAC_KERNEL, PRI_CRIT,
                          "Invalid config line format in file '%s', "
                          "no colon found: '%s'",
                          fileLabel,
                          fileLine);
         }
      }
      else
      {
         // The colon was not found.
         OsSysLog::add(FAC_KERNEL, PRI_CRIT,
                       "Invalid config line format in file '%s', "
                       "name is missing: '%s'",
                       fileLabel,
                       fileLine);
      }
   }

   return(isParameter);
}

void OsConfigDb::setCapitalizeName(UtlBoolean capitalizeName)
{
    mCapitalizeName = capitalizeName;
}

OsStatus OsConfigDb::updateFile(const char* filename) const
{
   UtlString originalFileContents;
   long fileLength = OsFile::openAndRead(filename, originalFileContents);
   const char* unparsedBits = originalFileContents;
   int unparsedLength = originalFileContents.length();

   // Loop through and try to preserve comments, space and order
   int lineStart = 0;
   int lineEnd = 0;
   UtlHashBag writtenNames;
   UtlString newFileContents;
   UtlString name;
   UtlString value;
   UtlString newValue;

   while(lineStart < unparsedLength)
   {
      lineEnd = UtlTokenizer::nextDelim(unparsedBits, lineStart, unparsedLength, "\n\r");

      //printf("start: %d end: %d length: %d\n", lineStart, lineEnd, unparsedLength);

      UtlString oneLine(&unparsedBits[lineStart], lineEnd - lineStart);

      //printf("Line: <%s>\n", oneLine.data());

      // If line contains a parameter
      if(parseLine(oneLine, mCapitalizeName, filename, name, value))
      {
         //printf("name<%s> value<%s>\n", name.data(), value.data());
         if(get(name, newValue) == OS_SUCCESS &&
            !writtenNames.contains(&name))
         {
            //printf("Wrote name<%s>\n", name.data());
            // The parameter still exists in the configDb and we have not yet
            // written it out, write the potentially changed value
            newFileContents.appendFormat("%s : %s\n", name.data(), newValue.data());

            // Save names/parameters written so that we can figure out what has not
            // been written out
            writtenNames.insert(new UtlString(name));
         }
         // else the parameter was removed, do nothing
      }
   
      // The line was a comment or blank line, write it back out the same
      else
      {
         newFileContents.appendFormat("%s\n", oneLine.data());
      }

      lineStart = lineEnd + 1;
   }

   int paramIndex;
   int paramCount = numEntries();
   DbEntry* paramEntry;

   for (paramIndex = 0; paramIndex < paramCount; paramIndex++)
   {
      paramEntry = (DbEntry*) mDb.at(paramIndex);

      removeNewlineReturns(paramEntry->key);
      removeNewlineReturns(paramEntry->value);

      // We have not written the value yet
      if(!writtenNames.contains(&(paramEntry->key)))
      {
          newFileContents.appendFormat("%s : %s\n", paramEntry->key.data(), paramEntry->value.data());
          writtenNames.insert(new UtlString(paramEntry->key));
      }
   }

   fileLength = OsFile::openAndWrite(filename, newFileContents);
 
   writtenNames.destroyAll();

   return(fileLength > 0 ? OS_SUCCESS : OS_INVALID_ARGUMENT);
}

OsStatus OsConfigDb::storeToFile(const char *filename)
{
   if (filename == NULL)
   {
       return OS_INVALID_ARGUMENT;
   }

   OsStatus retval = OS_UNSPECIFIED;
   OsReadLock lock(mRWMutex);    // take lock for writing while the database
                                  //  is loaded

   setIdentityLabel(filename);
   OsConfigEncryption *encryption = getEncryption();
   if (encryption != NULL && encryption->isWriteEncryptedEnabled() &&
           encryption->isNormallyEncrypted(this))
   {
       retval = storeToEncryptedFile(filename);
   }
   else
   {
       FILE *fp = fopen(filename,"wb");
       if (!fp)
       {
           osPrintf("Could not open %s.  errno = %d\n",filename,errno);
           return retval;
       }
       retval = storeToFile(fp);

       fclose(fp);
   }

   return retval;
}


// Remove the key/value pair associated with rKey.
// Return OS_SUCCESS if the key was found in the database, return
// OS_NOT_FOUND otherwise.
OsStatus OsConfigDb::remove(const UtlString& rKey)
{
   OsWriteLock lock(mRWMutex);
   DbEntry    lookupPair(rKey);
   DbEntry*   pEntryToRemove;
   size_t i = mDb.index(&lookupPair);
   if (i == UTL_NOT_FOUND)
   {
      return OS_NOT_FOUND;
   }
   else
   {
      pEntryToRemove = (DbEntry *)mDb.at(i);
      mDb.removeAt(i);
      delete pEntryToRemove;

      return OS_SUCCESS;
   }
}

// Remove all the key/value pairs starting with the designated prefix
OsStatus OsConfigDb::removeByPrefix(const UtlString& rPrefix) 
{
   OsWriteLock lock(mRWMutex);
   DbEntry* pEntry ;

   UtlSortedListIterator itor(mDb) ;
   while ((pEntry = (DbEntry*) itor()))
   {
       if (pEntry->key.length() >= rPrefix.length())
       {
           UtlString keyPrefix = pEntry->key ;
           keyPrefix.remove(rPrefix.length()) ;
           if (keyPrefix.compareTo(rPrefix, UtlString::ignoreCase) == 0)
           {
               remove(pEntry->key) ;
           }
       }      
   }
   
   return OS_SUCCESS ;
}

// Insert the key/value pair into the config database.
// If the database already contains an entry for this key, then replace it
// with the new key/value pair.
void OsConfigDb::set(const UtlString& rKey, const UtlString& rNewValue)
{
   OsWriteLock lock(mRWMutex);

   if (rKey.length() > 0) {
      insertEntry(rKey, rNewValue);
   }
}

// Insert the key/value pair into the config database If the
// database already contains an entry for this key, then set the
// value for the existing entry to iNewValue.
void OsConfigDb::set(const UtlString& rKey, const int iNewValue) 
{
    UtlString newValue ;

    // Convert to String
    char cTemp[64] ;
    sprintf(cTemp, "%d", iNewValue);

    // Set
    newValue = cTemp ;    
    set(rKey, newValue) ;
}

/* ============================ ACCESSORS ================================= */

void OsConfigDb::setIdentityLabel(const char *idLabel)
{
    if (idLabel != NULL)
    {
        mIdentityLabel = idLabel;
        //strncpy(mIdentityLabel, idLabel, sizeof(mIdentityLabel));
    }
    else
    {
        mIdentityLabel.resize(0);
    }
}

// filename, url, etc that would help identity the source of this config
// return NULL if no idenity was set
const char *OsConfigDb::getIdentityLabel() const
{
    return mIdentityLabel.data();
}

// Encryption Rules for this instance.
// SUBCLASS NOTE:  no setter method, requires subclassing to override instance
// only rules
OsConfigEncryption *OsConfigDb::getEncryption() const
{
    return getStaticEncryption();
}

// Set encryption Rules for all instances
void OsConfigDb::setStaticEncryption(OsConfigEncryption *encryption)
{
    gEncryption = encryption;
}

// Get encryption Rules for all instances
OsConfigEncryption *OsConfigDb::getStaticEncryption()
{
    return gEncryption;
}

// Sets rValue to the value in the database associated with rKey.
// If rKey is found in the database, returns OS_SUCCESS.  Otherwise,
// returns OS_NOT_FOUND and sets rValue to the empty string.
OsStatus OsConfigDb::get(const UtlString& rKey, UtlString& rValue) const
{
   OsReadLock lock(mRWMutex);
   DbEntry   lookupPair(rKey);
   DbEntry*  pEntry;
   size_t i = mDb.index(&lookupPair);
   if (i == UTL_NOT_FOUND)
   {
      rValue = "";     // entry not found
      return OS_NOT_FOUND;
   }
   else
   {
      pEntry = (DbEntry *)mDb.at(i);
      rValue = pEntry->value;
   }

   return OS_SUCCESS;
}

// Gets a db subset of name value pairs which have the given hashSubKey as
// a prefix for the key
OsStatus OsConfigDb::getSubHash(const UtlString& rHashSubKey,
                                OsConfigDb& rSubDb) const
{
   UtlSortedListIterator itor(const_cast<UtlSortedList&>(mDb));

   DbEntry* entry;
   // Skip the initial entries in the list that do not match.
   while ((entry = static_cast <DbEntry*> (itor())))
   {
      if (strncmp(entry->key.data(), rHashSubKey.data(),
                  rHashSubKey.length()) >= 0)
      {
         break;
      }
   }

   // Process the entries in the list that do match.
   for (; entry &&
           strncmp(entry->key.data(), rHashSubKey.data(),
                   rHashSubKey.length()) == 0;
        entry = static_cast <DbEntry*> (itor()))
   {
      // Construct and add the entry to the subhash.
      // Make temporary UtlString, because that's what insertEntry demands
      // as an argument.
      UtlString key(&entry->key.data()[rHashSubKey.length()]);
      rSubDb.insertEntry(key, entry->value);
   }

   return OS_SUCCESS;
}

// Sets rValue to the value in the database associated with rKey.
// If rKey is found in the database, returns OS_SUCCESS.  Otherwise,
// returns OS_NOT_FOUND and sets rValue to -1.
OsStatus OsConfigDb::get(const UtlString& rKey, int& rValue) const
{
   UtlString value;
   OsStatus returnStatus = get(rKey, value);

   if(returnStatus == OS_SUCCESS)
   {
      rValue = atoi(value.data());
   }
   else
   {
      rValue = -1;
   }

   value.remove(0);
   return(returnStatus);
}

// Relative to rKey, return the key and value associated with
// next (lexicographically ordered) key/value pair stored in the
// database.  If rKey is the empty string, key and value associated
// with the first entry in the database will be returned.
// Returns
//   OS_SUCCESS if there is a "next" entry
//   OS_NOT_FOUND if rKey is not found in the database and is not the
//      empty string
//   OS_NO_MORE_DATA if there is no "next" entry
OsStatus OsConfigDb::getNext(const UtlString& rKey,
                             UtlString& rNextKey,
                             UtlString& rNextValue) const
{
   OsReadLock  lock(mRWMutex);
   UtlBoolean  foundMatch;
   size_t      nextIdx = 0;
   DbEntry     lookupPair(rKey);
   DbEntry*    pEntry;

   foundMatch = FALSE;
   if (rKey.compareTo("") == 0)
   {
      foundMatch = TRUE;         // if the key is the empty string, then
      nextIdx = 0;               // return the first entry in the database
   }
   else
   {
      size_t idx = mDb.index(&lookupPair);
      if (idx != UTL_NOT_FOUND)
      {
         foundMatch = TRUE;
         nextIdx = idx + 1;
      }
   }

   if (foundMatch && (((int)nextIdx) < numEntries()))
   {
      pEntry     = (DbEntry *)mDb.at(nextIdx);
      rNextKey   = pEntry->key;
      rNextValue = pEntry->value;

      return OS_SUCCESS;
   }

   rNextKey   = "";
   rNextValue = "";

   if (!foundMatch)
      return OS_NOT_FOUND;
   else
      return OS_NO_MORE_DATA;
}


// Stores a list of strings to the configuration datadase using the 
// designated prefix as the base for the list items. 
void OsConfigDb::addList(const UtlString& rPrefix,
                         UtlSList& rList) 
{
    OsWriteLock lock(mRWMutex);
    size_t iNumEntries ;
    UtlString key ;
    UtlString* pValue ;

    // First remove all items start with the specified prefix
    removeByPrefix(rPrefix) ;

    // Next add all of the new items
    iNumEntries = rList.entries() ;
    if (iNumEntries > 0)
    {
        key = rPrefix ;
        key.append(".COUNT") ;
        set(key, iNumEntries) ;

        UtlSListIterator itor(rList) ;        
        int iCount = 1 ;
        char cTemp[64] ;
        while ((pValue = (UtlString*) itor()))
        {
            sprintf(cTemp, "%d", iCount++);   
            key = rPrefix ;
            key.append(".") ;
            key.append(cTemp) ;

            set(key, *pValue) ;            
        }
    }
}


// Loads a list of strings from the configuration datadase using the 
// designated prefix as the base for the list items.
int OsConfigDb::loadList(const UtlString& rPrefix,
                         UtlSList& rList) const
{
    OsReadLock lock(mRWMutex);
    int iNumEntries ;
    int rc = 0 ; 
    UtlString key ;
    UtlString value ;
    char cTemp[64] ;

    // Get number of items
    key = rPrefix ;
    key.append(".COUNT") ;    
    if (get(key, iNumEntries) == OS_SUCCESS)
    {
        for (int i = 0; i < iNumEntries; i++)
        {            
            sprintf(cTemp, "%d", i+1);
            key = rPrefix ;
            key.append(".") ;
            key.append(cTemp) ;

            if (get(key, value) == OS_SUCCESS)
            {
                rList.append(new UtlString(value)) ;
                rc++ ;
            }
        }        
    }

    return rc ;
}


// Get a port number from the configuration database
int OsConfigDb::getPort(const char* szKey) const
{
    assert(szKey) ;

    UtlString value ;
    int port = PORT_NONE ;

    if (get(szKey, value) == OS_SUCCESS)
    {
        // If the value is null, leave port == PORT_NONE.
        if (value.length())
        {
           if (value.compareTo("DEFAULT", UtlString::ignoreCase) == 0)
           {
              port = PORT_DEFAULT;
           }
           else if (value.compareTo("NONE", UtlString::ignoreCase) == 0)
           {
              port = PORT_NONE;
           }
           else
           {
              port = atoi(value.data());
              if (!portIsValid(port))
              {
                 port = PORT_NONE;
                 OsSysLog::add(FAC_KERNEL, PRI_CRIT,
                               "Invalid port number value '%s' for config variable '%s' in file '%s'.",
                               value.data(), szKey, mIdentityLabel.data());
              }
           }
        }
    }

    return port ;
}

// Delete all entries from the configuration database
void OsConfigDb::clear() 
{
   OsReadLock lock(mRWMutex);
   
    while (mDb.entries() > 0)   
    {
        DbEntry *e = (DbEntry *)mDb.at(0);
        mDb.removeAt(0) ;
        delete e ;
    }
}

/* ============================ INQUIRY =================================== */

// Return TRUE if the database is empty, otherwise FALSE.
UtlBoolean OsConfigDb::isEmpty(void) const
{
   OsReadLock lock(mRWMutex);

   return (mDb.entries() == 0);
}

// Return the number of entries in the config database
int OsConfigDb::numEntries(void) const
{
   OsReadLock lock(mRWMutex);

   return mDb.entries();
}

void OsConfigDb::storeToBuffer(char *buff) const
{
    char *p = buff;
    int n = numEntries();
    for (int i = 0; i < n; i++)
    {
        DbEntry *pEntry = (DbEntry *)mDb.at(i);
        removeChars(&pEntry->key, '\r');
        removeChars(&pEntry->value, '\n');

        sprintf(p, DB_LINE_FORMAT, (char *)pEntry->key.data(),
                (char *)pEntry->value.data());

        p = buff + strlen(buff);
    }
}

// important that it be big enough, not so important it's exact
int OsConfigDb::calculateBufferSize() const
{
    int n = numEntries();
    size_t size = n * strlen(DB_LINE_FORMAT);
    for (int i = 0; i < n; i++)
    {
        DbEntry *pEntry = (DbEntry *)mDb.at(i);
        size += pEntry->key.length() + pEntry->value.length();
    }
    return size;
}

void OsConfigDb::removeChars(UtlString *s, char c)
{
    size_t x = 0;
    while (x != UTL_NOT_FOUND)
    {
        x = s->first(c);
        if (x != UTL_NOT_FOUND)
        {
            s->remove(x, 1);
        }
    }
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

// write buffer to disk, replace existing file
OsStatus OsConfigDb::storeBufferToFile(const char *filename, const char *buff, unsigned long buffLen)
{
    OsStatus retval = OS_FAILED;

    if (buff != NULL && buffLen > 0)
    {
        UtlString filepath(filename);
        if (OsFileSystem::exists(filepath))
        {
            OsFileSystem::remove(filepath);

            OsFile file(filepath);
            file.open(OsFile::CREATE);
            unsigned long writtenLen;
            file.write(buff, buffLen, writtenLen);
            file.close();
            if (writtenLen == buffLen)
            {
                retval = OS_SUCCESS;
            }
        }
    }

    return retval;
}

OsStatus OsConfigDb::storeToEncryptedFile(const char *filename)
{
    OsStatus retval = OS_SUCCESS;

    // store to buffer
    size_t buffLen = calculateBufferSize();
    char *buff = new char[buffLen];
    storeToBuffer(buff);
    buffLen = strlen(buff);
    OsEncryption e;
    retval = getEncryption()->encrypt(this, &e, buff, buffLen);
    if (retval == OS_SUCCESS)
    {
        retval = storeBufferToFile(filename, (const char *)e.getResults(), e.getResultsLen());
    }

    return retval;
}

// Store the config database to a file
OsStatus OsConfigDb::storeToFile(FILE* fp)
{
   OsStatus retval = OS_SUCCESS;
   int        i;
   int        cnt;
   DbEntry*  pEntry;

   // step through the database writing out one entry per line
   // each entry is of the form "%s: %s\n"
   cnt = numEntries();
   for (i=0; i < cnt; i++)
   {
      pEntry = (DbEntry *)mDb.at(i);

      removeNewlineReturns(pEntry->key);
      removeNewlineReturns(pEntry->value);

      fprintf(fp, "%s : %s\r\n",
              (char*) pEntry->key.data(),
              (char*) pEntry->value.data());
   }

   fflush(fp);
   // The following line probably should never have been here.
   // It was causing segfaults under Linux, and it should have
   // on VxWorks and Windows NT as well.
   //fclose(fp);
   return retval;
}


/* //////////////////////////// PRIVATE /////////////////////////////////// */

void OsConfigDb::removeNewlineReturns(UtlString& stringData)
{
    //remove any  \n or \r at the ends of the lines
    size_t remove_char_loc = 0;
    while (remove_char_loc != UTL_NOT_FOUND)
    {
        remove_char_loc = stringData.first('\r');
        if (remove_char_loc != UTL_NOT_FOUND)
        {
            stringData.remove(remove_char_loc,1);
        }
    }

    remove_char_loc = 0;
    while (remove_char_loc != UTL_NOT_FOUND)
    {
        remove_char_loc = stringData.first('\n');
        if (remove_char_loc != UTL_NOT_FOUND)
        {
            stringData.remove(remove_char_loc,1);
        }
    }
}

OsStatus OsConfigDb::loadFromEncryptedFile(const char *file)
{
    OsStatus retval = OS_UNSPECIFIED;
    if (file == NULL)
    {
        return retval;
    }

    OsFile osfile(file);
    retval = osfile.open(OsFile::READ_ONLY);
    if (retval == OS_SUCCESS)
    {
        unsigned long buffLen = 0;
        osfile.getLength(buffLen);
        char *buff = new char[buffLen + 1];
        memset(buff, 0, buffLen + 1);

        unsigned long bytesRead;
        retval = osfile.read(buff, buffLen, bytesRead);
        if (bytesRead != buffLen || retval != OS_SUCCESS)
        {
            OsSysLog::add(FAC_KERNEL, PRI_ERR, "Error reading config file or \
mismatch in expected size  %s\n", getIdentityLabel());
            retval = OS_FAILED;
        }
        else
        {
            retval = loadFromEncryptedBuffer(buff, buffLen);
        }

        delete [] buff;
        osfile.close();
    }

    return retval;
}

// Load the configuration database from a file
OsStatus OsConfigDb::loadFromUnencryptedFile(FILE* fp)
{
   //int  result;
   //char name[81];
   //char value[81];
   char fileLine[MAX_FILELINE_SIZE + 1];
   OsStatus retval = OS_SUCCESS;

   // The following #define is needed in order for the feof() macro to work
   // properly under VxWorks
#  if defined(_VXWORKS)
#  define OK VX_OK
#  endif

   UtlString name;
   UtlString value;
   // step through the file reading one entry per line
   // each entry is of the form "%s: %s\n"
   while (!feof(fp))
   {
      //result = fscanf(fp, "%80s : %80s", name, value);
      //if (result == 2)
      if(fgets(fileLine, MAX_FILELINE_SIZE, fp))
      {
         if(parseLine(fileLine, mCapitalizeName, mIdentityLabel, name, value))
         {
            insertEntry(name, value);
         }
      }
      else if(ferror(fp))
      {
         perror("OsConfigDb::loadFromFile read error");
         retval = OS_UNSPECIFIED;
         break;
      }
   }

   return retval;
}

OsStatus OsConfigDb::loadFromEncryptedBuffer(char *buf, int bufLen)
{
    OsStatus retval = OS_SUCCESS;

    if (getEncryption()->isEncrypted(this, (const char *)buf, bufLen))
    {
        OsEncryption e;
        getEncryption()->decrypt(this, &e, buf, bufLen);
        if (getEncryption()->isEncrypted(this, (const char *)e.getResults(), e.getResultsLen()))
        {
            // if decryption did not produce an unencrypted version
            // then probably bad password, or test for encryption is bad
            // to distinguish internal errors from bad password, use unauthorized
            retval = OS_UNAUTHORIZED;
        }
        else
        {
            // yes it's null terminated
            retval = loadFromUnencryptedBuffer((const char*)e.getResults());
        }
    }
    else if (retval == OS_SUCCESS)
    {
        retval = loadFromUnencryptedBuffer((const char*)buf);
    }

    return retval;
}

OsStatus OsConfigDb::loadFromUnencryptedBuffer(const char *buf)
{
   if (buf == NULL)
      return OS_INVALID_ARGUMENT;

   char configLine[MAX_FILELINE_SIZE + 1];
   OsStatus retval = OS_SUCCESS;
   UtlString config = UtlString(buf);

   // The following #define is needed in order for the feof() macro to work
   // properly under VxWorks
#  if defined(_VXWORKS)
#  define OK VX_OK
#  endif

   // step through the file writing out one entry per line
   // each entry is of the form "%s: %s\n"
   size_t start = 0;
   size_t size ;
   size_t pos;
   UtlString name;
   UtlString value;

   while (1)
   {
      pos = config.first('\n');
      if ((pos != UTL_NOT_FOUND) || (pos == UTL_NOT_FOUND && !config.isNull()))
      {
         if (pos == UTL_NOT_FOUND)
         {
            pos = config.length();
         }

         // Do not allow lines greater than MAX_FILELINE_SIZE
         size = pos;
         if (size > MAX_FILELINE_SIZE)
         {
#ifdef TEST
             osPrintf("Warning: max line length exceeded in config db file");
#endif //TEST
             size = MAX_FILELINE_SIZE;
         }

         strncpy(configLine, config.data(), size) ;
         configLine[size] = 0 ;

         start = pos + 1;
         size_t len = config.length();
         if (len > start)
            config = config(start, len - start);
         else
            config = OsUtil::NULL_OS_STRING;

         if (strlen(configLine) == 0) continue;

         if(parseLine(configLine, mCapitalizeName, mIdentityLabel, name, value))
         {
            insertEntry(name, value);
         }
      }
      else
      {
         break;         // end of buffer reached.
      }
   }

   return retval;
}


// Helper method for inserting a key/value pair into the database.
// The write lock for the database should be taken before calling this
// method. If the database already contains an entry for this key, then
// set the value for the existing entry to rNewValue.
void OsConfigDb::insertEntry(const UtlString& rKey,
                              const UtlString& rNewValue)
{
   DbEntry  tempEntry(rKey, rNewValue);
   DbEntry* pOldEntry;
   size_t i = mDb.index(&tempEntry);
   if (i != UTL_NOT_FOUND)
   {                             // we already have an entry with this key
          pOldEntry = (DbEntry *)mDb.at(i);        //  just change its value
      // osPrintf("OsConfigDb::iNVP(%X,%s) - FOUND %s replaces %s\n",
                  // this, rKey.data(), pOldEntry->value.data(), rNewValue.data());
      pOldEntry->value = rNewValue;
   }
   else
   {
      DbEntry* pNewEntry = new DbEntry(rKey, rNewValue);

      // osPrintf("OsConfigDb::iNVP(%X,%s) - ADDING (%s)\n",
                  // this, rKey.data(), rNewValue.data());
      mDb.insert(pNewEntry);
   }
}

//----------------DbEntry ------------------------

DbEntry::DbEntry(const UtlString &rKey, const UtlString &rValue)
{
    key = rKey;
    value = rValue;
}

DbEntry::DbEntry(const UtlString &rKey)
{
    key = rKey;
}

DbEntry::~DbEntry()
{
}

int DbEntry::compareTo(const UtlContainable *b) const
{
   return key.compareTo(((DbEntry *)b)->key);
}

unsigned int DbEntry::hash() const
{
    return key.hash();
}

const UtlContainableType DbEntry::getContainableType() const
{
    return TYPE;
}

/* ============================ FUNCTIONS ================================= */

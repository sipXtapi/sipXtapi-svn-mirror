//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////
// Author: Dan Petrie (dpetrie AT SIPez DOT com)

// SYSTEM INCLUDES

// APPLICATION INCLUDES
#include <net/SipDialogMgr.h>
#include <net/SipDialog.h>
#include <net/SipMessage.h>
#include <os/OsSysLog.h>
#include <utl/UtlHashBagIterator.h>


// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
SipDialogMgr::SipDialogMgr()
: mDialogMgrMutex(OsMutex::Q_FIFO)
{
}


// Copy constructor NOT IMPLEMENTED
SipDialogMgr::SipDialogMgr(const SipDialogMgr& rSipDialogMgr)
: mDialogMgrMutex(OsMutex::Q_FIFO)
{
}


// Destructor
SipDialogMgr::~SipDialogMgr()
{
    // Iterate through and delete all the dialogs
    // TODO:
}

/* ============================ MANIPULATORS ============================== */

// Assignment operator
SipDialogMgr& 
SipDialogMgr::operator=(const SipDialogMgr& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;

   return *this;
}

UtlBoolean SipDialogMgr::createDialog(const SipMessage& message, 
                                      UtlBoolean messageIsFromLocalSide,
                                      const char* dialogHandle)
{
    UtlBoolean createdDialog = FALSE;
    UtlString handle(dialogHandle ? dialogHandle : "");
    // If the dialog handle was not set, get it from the message
    if(handle.isNull())
    {
        message.getDialogHandle(handle);
    }

    // Check to see if the dialog exists
    if(dialogExists(handle) ||
        earlyDialogExistsFor(handle))
    {
        // Should not try to create a dialog for one that
        // already exists
        OsSysLog::add(FAC_SIP, PRI_ERR,
            "SipDialogMgr::createDialog called with handle: %s for existing dialog",
            handle.data());
    }

    // Dialog needs to be created
    else
    {
        createdDialog = TRUE;
        SipDialog* dialog = new SipDialog(&message, messageIsFromLocalSide);
        lock();
        mDialogs.insert(dialog);
        unlock();
    }

    return(createdDialog);
}

UtlBoolean SipDialogMgr::updateDialog(const SipMessage& message, 
                                      const char* dialogHandle)
{
    UtlString handle(dialogHandle ? dialogHandle : "");
    // If the dialog handle was not set, get it from the message
    if(handle.isNull())
    {
        message.getDialogHandle(handle);
    }

    lock();
    
    SipDialog* dialog = findDialog(handle,
                                   TRUE, // if established handle, find early dialog
                                   FALSE); // do not want established dialogs for early handle
    if(dialog)
    {
#ifdef TEST_PRINT
        UtlString dialogDump;
        dialog->toString(dialogDump);
        printf("SipDialogMgr::updateDialog dialog before:\n%s\n",
               dialogDump.data());
#endif

        dialog->updateDialogData(message);

#ifdef TEST_PRINT
        dialog->toString(dialogDump);
        printf("SipDialogMgr::updateDialog dialog after:\n%s\n",
               dialogDump.data());
#endif
    }


    unlock();

    return(dialog != NULL);
}

UtlBoolean SipDialogMgr::setNextLocalTransactionInfo(SipMessage& request,
                                                     const char* method,
                                                     const char* dialogHandle)
{
    UtlBoolean requestSet = FALSE;
    UtlString dialogHandleString(dialogHandle ? dialogHandle : "");
    if(dialogHandleString.isNull())
    {
        request.getDialogHandle(dialogHandleString);
    }

    lock();
    SipDialog* dialog = findDialog(dialogHandleString,
                                   FALSE, // If established only want exact match  dialogs 
                                   TRUE); // If message is from a prior transaction
                                          // when the dialog was in an early state
                                          // allow it to match and established 
                                          // dialog
    if(dialog)
    {
        dialog->setRequestData(request, method);
        requestSet = TRUE;

#ifdef TEST_PRINT
        UtlString dialogDump;
        dialog->toString(dialogDump);
        printf("SipDialogMgr::setNextLocalTransactionInfo dialog:\n%s\n",
               dialogDump.data());
#endif

    }
    else
    {
        OsSysLog::add(FAC_SIP,
                      PRI_WARNING, 
                      "SipDialogMgr::setNextLocalTransactionInfo could not find dialog with handle %s",
                      dialogHandle);
    }

    unlock();

    return(requestSet);
}

/* ============================ ACCESSORS ================================= */

UtlBoolean SipDialogMgr::getEarlyDialogHandleFor(const char* establishedDialogHandle, 
                                                 UtlString& earlyDialogHandle)
{
    UtlBoolean foundDialog = FALSE;
    UtlString handle(establishedDialogHandle ? establishedDialogHandle : "");

    lock();
    SipDialog* dialog = findDialog(handle,
                                   TRUE, // if established, match early dialog
                                   FALSE); // if early, match established dialog
    if(dialog)
    {
        dialog->getEarlyHandle(earlyDialogHandle);
        foundDialog = TRUE;
    }
    else
    {
        earlyDialogHandle = "";
    }
    unlock();

    return(foundDialog);
}

UtlBoolean SipDialogMgr::getEstablishedDialogHandleFor(const char* earlyDialogHandle,
                                             UtlString& establishedDialogHandle)
{
    UtlBoolean foundDialog = FALSE;
    UtlString handle(earlyDialogHandle ? earlyDialogHandle : "");
    lock();
    // Looking for an dialog that matches this earlyHandle, if there
    // is not an exact match see if there is an established dialog
    // that matches
    SipDialog* dialog = findDialog(handle,
                                   FALSE, // if established, match early dialog
                                   TRUE); // if early, match established dialog
    if(dialog && !dialog->isEarlyDialog())
    {
        dialog->getHandle(establishedDialogHandle);
        foundDialog = TRUE;
    }
    else
    {
        establishedDialogHandle = "";
    }
    unlock();

    return(foundDialog);

}

int SipDialogMgr::countDialogs() const
{
    return(mDialogs.entries());
}

int SipDialogMgr::toString(UtlString& dumpString)
{
    int dialogCount = 0;
    dumpString = "";
    UtlString oneDialogDump;
    SipDialog* dialog = NULL;

    UtlHashBagIterator iterator(mDialogs);
    // Look at all the dialogs with the same call-id
    while((dialog = (SipDialog*) iterator()))
    {
        if(dialogCount)
        {
            dumpString.append('\n');
        }
        dialog->toString(oneDialogDump);
        dumpString.append(oneDialogDump);

        dialogCount++;
    }

    return(dialogCount);
}

/* ============================ INQUIRY =================================== */

UtlBoolean SipDialogMgr::earlyDialogExists(const char* dialogHandle)
{
    UtlBoolean foundDialog = FALSE;
    UtlString handle(dialogHandle ? dialogHandle : "");
    lock();
    // Looking for an dialog that matches this handle, if there
    // is not an exact match see if there is an early dialog
    // that matches the given presumably established dialog handle
    SipDialog* dialog = findDialog(handle,
                                   TRUE, // if established, match early dialog
                                   FALSE); // if early, match established dialog

    // We only want early dialogs
    if(dialog && dialog->isEarlyDialog())
    {
        foundDialog = TRUE;
    }

    unlock();

    return(foundDialog);
}

UtlBoolean SipDialogMgr::earlyDialogExistsFor(const char* establishedDialogHandle)
{
    UtlBoolean foundDialog = FALSE;
    UtlString handle(establishedDialogHandle ? establishedDialogHandle : "");

    // If we have an established dialog handle
    if(!SipDialog::isEarlyDialog(handle))
    {
        lock();
        // Looking for an dialog that matches this handle, if there
        // is not an exact match see if there is an early dialog
        // that matches the given presumably established dialog handle
        SipDialog* dialog = findDialog(handle,
                                       TRUE, // if established, match early dialog
                                       FALSE); // if early, match established dialog

        if(dialog && !dialog->isEarlyDialog())
        {
            foundDialog = TRUE;
        }
        unlock();
    }

    return(foundDialog);
}

UtlBoolean SipDialogMgr::dialogExists(const char* dialogHandle)
{
    UtlBoolean foundDialog = FALSE;
    UtlString handle(dialogHandle ? dialogHandle : "");
    lock();
    // Looking for an dialog that exactly matches this handle
    SipDialog* dialog = findDialog(handle,
                                   FALSE, // if established, match early dialog
                                   FALSE); // if early, match established dialog

    if(dialog)
    {
        foundDialog = TRUE;
    }

    unlock();

    return(foundDialog);
}

UtlBoolean SipDialogMgr::isLastLocalTransaction(const SipMessage& message, 
                                                const char* dialogHandle)
{
    UtlBoolean matchesTransaction = FALSE;
    UtlString handle(dialogHandle ? dialogHandle : "");
    // If the dialog handle was not set, get it from the message
    if(handle.isNull())
    {
        message.getDialogHandle(handle);
    }

    UtlString callId;
    UtlString fromTag;
    UtlString toTag;
    SipDialog::parseHandle(handle, callId, fromTag, toTag);

    lock();
    // Looking for any dialog that matches this handle
    SipDialog* dialog = findDialog(handle,
                                   TRUE, // if established, match early dialog
                                   TRUE); // if early, match established dialog

    if(dialog && 
       dialog->isTransactionLocallyInitiated(callId, fromTag, toTag) &&
       dialog->isSameLocalCseq(message))
    {
        matchesTransaction = TRUE;
    }

    unlock();
    
    return(matchesTransaction);
}

UtlBoolean SipDialogMgr::isNewRemoteTransaction(const SipMessage& message)
{
    UtlBoolean matchesTransaction = FALSE;
    UtlString handle;
    message.getDialogHandle(handle);

    UtlString callId;
    UtlString fromTag;
    UtlString toTag;
    SipDialog::parseHandle(handle, callId, fromTag, toTag);

    lock();
    // Looking for any dialog that matches this handle
    SipDialog* dialog = findDialog(handle,
                                   TRUE, // if established, match early dialog
                                   TRUE); // if early, match established dialog

    if(dialog && 
       dialog->isTransactionRemotelyInitiated(callId, fromTag, toTag) &&
       dialog->isNextRemoteCseq(message))
    {
        matchesTransaction = TRUE;
    }

    unlock();
    
    return(matchesTransaction);
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

SipDialog* SipDialogMgr::findDialog(UtlString& dailogHandle,
                                    UtlBoolean ifHandleEstablishedFindEarlyDialog,
                                    UtlBoolean ifHandleEarlyFindEstablishedDialog)
{
    UtlString callId;
    UtlString localTag;
    UtlString remoteTag;
    SipDialog::parseHandle(dailogHandle, callId, localTag, remoteTag);

    return(findDialog(callId, localTag, remoteTag,
                      ifHandleEstablishedFindEarlyDialog,
                      ifHandleEarlyFindEstablishedDialog));
}

SipDialog* SipDialogMgr::findDialog(UtlString& callId,
                                  UtlString& localTag,
                                  UtlString& remoteTag,
                                  UtlBoolean ifHandleEstablishedFindEarlyDialog,
                                  UtlBoolean ifHandleEarlyFindEstablishedDialog)
{
    SipDialog* dialog = NULL;
    UtlHashBagIterator iterator(mDialogs, &callId);

    // Look at all the dialogs with the same call-id
    while((dialog = (SipDialog*) iterator()))
    {
        // Check for exact match on the dialog handle
        if(dialog->isSameDialog(callId,localTag, remoteTag))
        {
            break;
        }
    }

    // Check if this established dialog handle matches an early dialog
    if(!dialog && ifHandleEstablishedFindEarlyDialog)
    {
        iterator.reset();
        while((dialog = (SipDialog*) iterator()))
        {
            // Check for match on the early dialog for the handle
            if(dialog->isEarlyDialogFor(callId, localTag, remoteTag))
            {
                break;
            }
        }
    }

    // Check if this early dialog handle matches an established dialog
    if(!dialog && ifHandleEarlyFindEstablishedDialog)
    {
        iterator.reset();
        while((dialog = (SipDialog*) iterator()))
        {
            // Check for match on the early dialog for the handle
            if(dialog->wasEarlyDialogFor(callId, localTag, remoteTag))
            {
                break;
            }
        }
    }

    return(dialog);
}

UtlBoolean SipDialogMgr::deleteDialog(const char* dialogHandle)
{
    UtlBoolean dialogRemoved = FALSE;
    UtlString handle(dialogHandle ? dialogHandle : "");
    lock();
    // Not sure if it should match all flavors of dialog, especially the
    // last one (i.e. ealy handle matching an established
    SipDialog* dialog = findDialog(handle,
                                   TRUE, // match early dialogs for handle
                                   TRUE); // if early, match established

    if(dialog)
    {
        dialogRemoved = TRUE;
        mDialogs.removeReference(dialog);
        delete dialog;
        dialog = NULL;
    }

    unlock();

    return(dialogRemoved);
}

void SipDialogMgr::lock()
{
    mDialogMgrMutex.acquire();
}

void SipDialogMgr::unlock()
{
    mDialogMgrMutex.release();
}

/* ============================ FUNCTIONS ================================= */


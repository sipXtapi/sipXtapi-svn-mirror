// 
//
// Copyright (c) PingTel Corp. (work in progress)
//
// This is an unpublished work containing PingTel Corporation's confidential
// and proprietary information.  Disclosure, use or reproduction without
// written authorization of PingTel Corp. is prohibited.
//
//! lib=mailbox
//////////////////////////////////////////////////////////////////////////////
#ifndef GetNavWebCGI_H
#define GetNavWebCGI_H

// SYSTEM INCLUDES
//#include <...>

// APPLICATION INCLUDES
#include "utl/UtlString.h"
#include "mailboxmgr/CGICommand.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

/**
 * Mailbox Class
 *
 * @author John P. Coffey
 * @version 1.0
 */
class GetNavWebCGI : public CGICommand
{
public:
    /**
     * Ctor
     */
    GetNavWebCGI (	const UtlString& mailboxIdentity );

    /**
     * Virtual Dtor
     */
    virtual ~GetNavWebCGI();

    /** This does the work */
    virtual OsStatus execute ( UtlString* out = NULL );


protected:

private:
	UtlString m_mailboxIdentity;
};

#endif //GetNavWebCGI_H


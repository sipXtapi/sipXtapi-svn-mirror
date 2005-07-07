// 
// 
// Copyright (C) 2004 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
// 
// Copyright (C) 2004 Pingtel Corp.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// $$
//////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
//#include <...>

// APPLICATION INCLUDES
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <os/OsDefs.h>
#include "utl/UtlString.h"

#ifdef _WIN32
#include <assert.h>
#endif

// EXTERNAL FUNCTIONS

#if defined(_WIN32)
#include <windows.h>

void PrintIt(const char *s)
{
    printf("%s",s);
    OutputDebugString(s);
}

#else
#define PrintIt(x) printf("%s", (char *) (x))
#endif

// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// FUNCTIONS


// Layer of indirection over printf

extern "C" void osPrintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    /* Guess we need no more than 128 bytes. */
    int n, size = 128;
    char *p;

    p = (char*) malloc(size) ;
     
    while (p != NULL)
    {
        /* Try to print in the allocated space. */
#ifdef _WIN32
        n = _vsnprintf (p, size, format, args);
#else
        n = vsnprintf (p, size, format, args);
#endif

        /* If that worked, return the string. */
        if (n > -1 && n < size)
        {
            break;
        }
        /* Else try again with more space. */
        if (n > -1)    /* glibc 2.1 */
            size = n+1; /* precisely what is needed */
        else           /* glibc 2.0 */
            size *= 2;  /* twice the old size */

        if ((p = (char*) realloc (p, size)) == NULL)
        {
            break;
        }
    }

    if (p != NULL)
    {
        PrintIt(p) ;
        free(p) ;
    }

    va_end(args) ;
}


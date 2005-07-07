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
#include <assert.h>
#include <stdio.h>

#if defined(_WIN32)
#   include <winsock.h>
#elif defined(_VXWORKS)
#   include <inetLib.h>
#   include <netdb.h>
#   include <resolvLib.h>
#   include <sockLib.h>
#elif defined(__pingtel_on_posix__)
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <netdb.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#else
#error Unsupported target platform.
#endif

// APPLICATION INCLUDES
#include <os/OsConnectionSocket.h>
#include "os/OsUtil.h"
#include <os/OsSysLog.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES

// CONSTANTS
#if defined(_VXWORKS) || defined(__pingtel_on_posix__)
static const int INVALID_SOCKET = -1;
#endif

// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
OsConnectionSocket::OsConnectionSocket(int connectedSocketDescriptor)
{
	socketDescriptor = connectedSocketDescriptor;

	// Should query and setup:
   	// remoteHostPort
	// remoteHostName
}

// Constructor
OsConnectionSocket::OsConnectionSocket(int serverPort, 
                                       const char* serverName,
                                       UtlBoolean blockingConnect)
{
  int error = 0;
	UtlBoolean isIp;
	struct in_addr* serverAddr;
	struct hostent* server = NULL;
	struct sockaddr_in serverSockAddr;
  UtlString temp_output_address;

	socketDescriptor = OS_INVALID_SOCKET_DESCRIPTOR;

	remoteHostPort = serverPort;

	// Connect to a remote host if given
	if(! serverName || strlen(serverName) == 0)
	{
#if defined(_VXWORKS)
		serverName = "127.0.0.1";
#elif defined(__pingtel_on_posix__)
		serverName = "localhost";
#elif defined(WIN32)
		unsigned long address_val = OsSocket::getDefaultBindAddress();
		if (address_val == htonl(INADDR_ANY))
			serverName = "localhost";
		else
		{
			struct in_addr in;
			in.S_un.S_addr = address_val;
	
			serverName = inet_ntoa(in);
		}
#else
#error Unsupported target platform.
#endif

	}
    if(serverName)
    {
    	remoteHostName.append(serverName);
    }

	if(!socketInit())
	{
		goto EXIT;
	}

#	if defined(_VXWORKS)
	char hostentBuf[512];
#	endif

	// Create the socket
	socketDescriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(socketDescriptor == INVALID_SOCKET)
	{
		error = OsSocketGetERRNO();
		socketDescriptor = OS_INVALID_SOCKET_DESCRIPTOR;
		perror("call to socket failed in OsConnectionSocket::OsConnectionSocket\n");
		OsSysLog::add(FAC_SIP, PRI_ERR, "socket call failed with error: 0x%x in OsConnectionSocket::OsConnectionSocket\n", error);
		goto EXIT;
	}

        if(!blockingConnect)
        {
            makeNonblocking();
        }
	
	isIp = isIp4Address(serverName);
	if(!isIp)
  {
#	if defined(_WIN32) || defined(__pingtel_on_posix__)
		server = gethostbyname(serverName);
#	elif defined(_VXWORKS)
	  server = resolvGetHostByName((char*) serverName,
                                 hostentBuf, sizeof(hostentBuf));
#	else
#	error Unsupported target platform.
#	endif //_VXWORKS
  }

	if(!isIp && !server)
	{
		close();
		OsSysLog::add(FAC_SIP, PRI_ERR, "DNS failed to lookup host: %s\n", serverName);
		goto EXIT;
	}

	if (!isIp)
	{
		inet_ntoa_pt(*((in_addr*) (server->h_addr)),temp_output_address);
                OsSysLog::add(FAC_SIP, PRI_DEBUG,
                              "connecting to host at: %s, port: %d\n",
                              temp_output_address.data(), serverPort);
		serverAddr = (in_addr*) (server->h_addr);
		serverSockAddr.sin_family = server->h_addrtype;
		serverSockAddr.sin_port = htons(serverPort);
		serverSockAddr.sin_addr.s_addr = (serverAddr->s_addr);
	}
	else
	{
		serverSockAddr.sin_family = AF_INET;
		serverSockAddr.sin_port = htons(serverPort);
		serverSockAddr.sin_addr.s_addr = inet_addr(serverName);
	}

	// Set the default destination address for the socket
    int connectReturn;
#	if defined(_WIN32) || defined(__pingtel_on_posix__)
    connectReturn = connect(socketDescriptor,
                            (const struct sockaddr*) &serverSockAddr, 
                            sizeof(serverSockAddr));
#	elif defined(_VXWORKS)
    connectReturn = connect(socketDescriptor,
                            (struct sockaddr*) &serverSockAddr, 
                            sizeof(serverSockAddr));
#	else
#	error Unsupported target platform.
#	endif

    error = OsSocketGetERRNO();

#if defined(_WIN32)
    if(error == WSAEWOULDBLOCK && 
       !blockingConnect)
    {
        error = 0;
        connectReturn = 0;
    }
#elif defined(_VXWORKS)
    if(error == EWOULDBLOCK && 
       !blockingConnect)
    {
        error = 0;
        connectReturn = 0;
    }
#endif

	if(connectReturn &&
       error)
    {
        char* msgBuf;
        // WIN32: 10061 WSAECONNREFUSED the other end did not accept the socket
        close();
        // perror("OsConnection: call to connect failed\n");
        // osPrintf("connect call failed with error: %d\n", error);
        msgBuf = strerror(error);
        OsSysLog::add(FAC_SIP, PRI_INFO, "OsConnection(%s:%d): call to connect() failed: %s\n"
	        "connect call failed with error: %d %d\n",
	        serverName, serverPort, msgBuf, error, connectReturn);
        goto EXIT;
	}
    mIsConnected = TRUE;

EXIT:
	return;
}


// Destructor
OsConnectionSocket::~OsConnectionSocket()
{
	remoteHostName = OsUtil::NULL_OS_STRING;
	close();
}

/* ============================ MANIPULATORS ============================== */

// Assignment operator
OsConnectionSocket& 
OsConnectionSocket::operator=(const OsConnectionSocket& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;

   return *this;
}

UtlBoolean OsConnectionSocket::reconnect()
{
	OsSysLog::add(FAC_SIP, PRI_WARNING, " reconnect NOT implemented!\n");
	return(FALSE);
}

// Because we have overided one read method, we
// must implement them all in OsConnectionSocket or
// we end up hiding some of the methods.
int OsConnectionSocket::read(char* buffer, int bufferLength)
{
    // Use base class implementation
    int bytesRead = OsSocket::read(buffer, bufferLength);
    return(bytesRead);
}

int OsConnectionSocket::read(char* buffer, 
                             int bufferLength,
                             UtlString* ipAddress, 
                             int* port)
{
    // Overide base class version as recvfrom does not
    // seem to return host info correctly for TCP
    // Use base class version without the remote host info
    int bytesRead = OsSocket::read(buffer, bufferLength);

    // Explicitly get the remote host info.
    getRemoteHostIp(ipAddress, port);

    return(bytesRead);
}

// Because we have overided one read method, we
// must implement them all in OsConnectionSocket or
// we end up hiding some of the methods.
int OsConnectionSocket::read(char* buffer, 
                            int bufferLength, 
                            long waitMilliseconds)
{
    // Use base class implementation
    int bytesRead = OsSocket::read(buffer, bufferLength, waitMilliseconds);
    return(bytesRead);
}

/* ============================ ACCESSORS ================================= */
int OsConnectionSocket::getIpProtocol() const
{
	return(TCP);
}
/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */



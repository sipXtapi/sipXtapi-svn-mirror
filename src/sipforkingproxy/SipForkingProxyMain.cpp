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

// System includes
#include <stdio.h>
#include <signal.h>
#if defined(_WIN32)
#include <windows.h>
#elif defined(__pingtel_on_posix__)
#include <unistd.h>
#endif

#include <os/OsFS.h>
#include <os/OsConfigDb.h>
#include <os/OsSocket.h>
#include <os/OsTask.h>
#include <net/SipUserAgent.h>
#include <net/NameValueTokenizer.h>
#include <xmlparser/tinyxml.h>

#include <SipRouter.h>
#include <ForwardRules.h>

//uncomment next line to enable bound checker checking with 'b' key
//#define BOUNDS_CHECKER

#ifdef BOUNDS_CHECKER
	#include "D:\Program Files\Compuware\BoundsChecker\ERptApi\apilib.h"
	#pragma comment(lib, "D:\\Program Files\\Compuware\\BoundsChecker\\ERptApi\\nmapi.lib")
#endif

#define CONFIG_ETC_DIR SIPX_CONFDIR
#define FORWARDING_RULES_FILENAME "forwardingrules.xml"
#define SIP_PROXY_LOG "sipproxy.log"
#define CONFIG_LOG_DIR SIPX_LOGDIR
#define LOG_FACILITY   FAC_SIP

// Configuration names pulled from config-file
#define CONFIG_SETTING_LOG_LEVEL      "SIP_PROXY_LOG_LEVEL"
#define CONFIG_SETTING_LOG_CONSOLE    "SIP_PROXY_LOG_CONSOLE"
#define CONFIG_SETTING_LOG_DIR        "SIP_PROXY_LOG_DIR"

#define PRINT_ROUTE_RULE(APPEND_STRING, FROM_HOST, TO_HOST) \
    APPEND_STRING.append("\t<route mappingType=\"local\">\n\t\t<routeFrom>"); \
    APPEND_STRING.append(FROM_HOST); \
    APPEND_STRING.append("</routeFrom>\n\t\t<routeTo>"); \
    APPEND_STRING.append(TO_HOST); \
    APPEND_STRING.append("</routeTo>\n\t</route>\n");

// TYPEDEFS
typedef void (*sighandler_t)(int);

// FUNCTIONS
extern "C" {
    void  sigHandler( int sig_num );
    sighandler_t pt_signal( int sig_num, sighandler_t handler );
}

// GLOBALS
UtlBoolean gShutdownFlag = FALSE;

/**
 * Description:
 * This is a replacement for signal() which registers a signal handler but sets
 * a flag causing system calls ( namely read() or getchar() ) not to bail out 
 * upon recepit of that signal. We need this behavior, so we must call 
 * sigaction() manually.
 */
sighandler_t 
pt_signal( int sig_num, sighandler_t handler)
{
#if defined(__pingtel_on_posix__)
    struct sigaction action[2];
    action[0].sa_handler = handler;
    sigemptyset(&action[0].sa_mask);
    action[0].sa_flags = 0;
    sigaction ( sig_num, &action[0], &action[1] );
    return action[1].sa_handler;
#else
    return signal( sig_num, handler );
#endif
}

/** 
 * Description: 
 * This is the signal handler, When called this sets the 
 * global gShutdownFlag allowing the main processing
 * loop to exit cleanly.
 */
void 
sigHandler( int sig_num )
{
    // set a global shutdown flag
    gShutdownFlag = TRUE;

    // Unregister interest in the signal to prevent recursive callbacks
    pt_signal( sig_num, SIG_DFL );

    // Minimize the chance that we loose log data
    OsSysLog::flush();
    OsSysLog::add( LOG_FACILITY, PRI_CRIT, "sigHandler: caught signal: %d", sig_num );
    OsSysLog::add( LOG_FACILITY, PRI_CRIT, "sigHandler: closing IMDB connections" );
    OsSysLog::flush();
}

// This might eventially get moved to SipRouter
// Though a real container probably needs to be created
// for the route rules as the logic will soon outgrow
// the OsConfigDb
/*
void initRoutes(const char* routeRuleFileName,
                int udpPort,
                OsConfigDb& routeMaps)
{
   TiXmlDocument xmlDoc =  TiXmlDocument(routeRuleFileName);

   // There is no routing rules file. Create the defaults
   if(!xmlDoc.LoadFile())
   {
       UtlString defaultXml("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n<routes>\n");

       UtlString ipAddress;
       OsSocket::getHostIp(&ipAddress);

       // Default registry/redirect server address
       UtlString rrServer(ipAddress);
       rrServer.append(":5070");

       // Map Explicit IP address
       char thisPort[64];
       sprintf(thisPort, ":%d", udpPort);
       UtlString explicitIp(ipAddress);
       explicitIp.append(thisPort);
       PRINT_ROUTE_RULE(defaultXml, explicitIp, rrServer);
       routeMaps.set(explicitIp, rrServer);

       // Map non-Fully qualified host name
       UtlString hostName;
       OsSocket::getHostName(&hostName);
       UtlString hostNamePort(hostName);
       hostNamePort.append(thisPort);
       PRINT_ROUTE_RULE(defaultXml, hostNamePort, rrServer);
       routeMaps.set(hostNamePort, rrServer);

       // Map The local domain (e.g. assumes DNS SRV mapping
       UtlString localDomain;
       OsSocket::getDomainName(localDomain);
       UtlString localDomainPort(localDomain);
       localDomainPort.append(thisPort);
       PRINT_ROUTE_RULE(defaultXml, localDomainPort, rrServer);
       routeMaps.set(localDomainPort, rrServer);

       // Map Fully qualified host name (FQHN)
       UtlString fqhn(hostName);
       fqhn.append('.');
       fqhn.append(localDomain);
       fqhn.append(thisPort);
       PRINT_ROUTE_RULE(defaultXml, fqhn, rrServer);
       routeMaps.set(fqhn, rrServer);

       // Default config server address
       UtlString configServer(ipAddress);
       configServer.append(":5090");

       // Map Non-fully qualified config server name
       PRINT_ROUTE_RULE(defaultXml, "sipuaconfig", configServer);
       routeMaps.set("sipuaconfig", configServer);

       // Map Fully qualified (FQHN) config server name
       UtlString fqhnConfig("sipuaconfig.");
       fqhnConfig.append(localDomain);
       fqhnConfig.append(thisPort);
       PRINT_ROUTE_RULE(defaultXml, fqhnConfig, configServer);
       routeMaps.set(fqhnConfig, configServer);
       defaultXml.append("</routes>\n");

       UtlString parseError = xmlDoc.ErrorDesc();
       osPrintf("%s: %s\nSetting defaults:\n", 
           parseError.data(),
           routeRuleFileName);
       osPrintf("%s", defaultXml.data());
       OsSysLog::add(FAC_SIP, PRI_ERR, "%s: %s\nSetting defaults:", 
           parseError.data(),
           routeRuleFileName);
       OsSysLog::add(FAC_SIP, PRI_INFO, "%s", defaultXml.data());
   }

   // There is a routing rules file.  Parse the contents
   else
   {
       // Find the routes container
       TiXmlNode* routesContainer = xmlDoc.FirstChild("routes");
       if(routesContainer)
       {
           TiXmlNode* routeNode = NULL;
           UtlString fromHost;
           UtlString toHost;
           int routeCount = 0;
           // Loop through all the route elements
           while ((routeNode = routesContainer->IterateChildren(routeNode)))
           {
               // Skip over attributes
               if(routeNode->Type() != TiXmlNode::ELEMENT)
               {
                  continue;
               }

               // Skip over non-route elements
               TiXmlElement* routeElement = routeNode->ToElement();
               UtlString tagValue = routeElement->Value();
               if(tagValue.compareTo("route", UtlString::ignoreCase) != 0 )
               {
                  continue;
               }

               //osPrintf("Found a route\n");
               routeCount++;

               // Find the first routeFrom
               fromHost.remove(0);
               TiXmlNode* fromNode = routeElement->FirstChild("routeFrom");
               TiXmlElement* fromElement = fromNode ? fromNode->ToElement() : NULL;
               if(fromElement)
               {
                   TiXmlNode* hostNode = fromElement->FirstChild();
                   if(hostNode && hostNode->Type() == TiXmlNode::TEXT)
                   {
                       TiXmlText* hostText = hostNode->ToText();
                       if (hostText)
                       {
                           fromHost = hostText->Value();
                           //osPrintf("\tfound a routeFrom: %s\n", fromHost.data());
                       }
                   }
               }

               if(fromHost.isNull())
               {
                   OsSysLog::add(FAC_SIP, PRI_WARNING, "WARNING: route container %d ignored. no routeFrom element",
                       routeCount);
                   continue;
               }

               // Find the first routeTo
               toHost.remove(0);
               TiXmlNode* toNode = fromNode->NextSibling("routeTo");
               TiXmlElement* toElement = toNode ? toNode->ToElement() : NULL;
               if(toElement)
               {
                   TiXmlNode* hostNode = toElement->FirstChild();
                   if(hostNode && hostNode->Type() == TiXmlNode::TEXT)
                   {
                       TiXmlText* hostText = hostNode->ToText();
                       if (hostText)
                       {
                           toHost = hostText->Value();
                           //osPrintf("\tfound a routeTo: %s\n", toHost.data());
                       }
                   }
               }

               if(toHost.isNull())
               {
                   OsSysLog::add(FAC_SIP, PRI_WARNING, "WARNING: route container %d ignored. no routeTo element",
                       routeCount);
                   continue;
               }

               //osPrintf("Found route from: %s to: %s\n",
               //    fromHost.data(), toHost.data());
               routeMaps.set(fromHost, toHost);

           } // End loop through route containers

           OsSysLog::add(FAC_SIP, PRI_INFO, "found %d route rules in %s", 
               routeCount, routeRuleFileName);
       }

       // No routes container found
       else
       {
           OsSysLog::add(FAC_SIP, PRI_WARNING, "WARNING: no routes found in %s", 
               routeRuleFileName);
       }
   }
}
*/

// Initialize the OsSysLog
void initSysLog(OsConfigDb* pConfig)
{
   UtlString logLevel;               // Controls Log Verbosity
   UtlString consoleLogging;         // Enable console logging by default?
   UtlString fileTarget;             // Path to store log file.
   UtlBoolean bSpecifiedDirError ;   // Set if the specified log dir does not 
                                    // exist
   struct tagPrioriotyLookupTable
   {
      const char*      pIdentity;
      OsSysLogPriority ePriority;
   };

   struct tagPrioriotyLookupTable lkupTable[] =
   {
      { "DEBUG",   PRI_DEBUG},
      { "INFO",    PRI_INFO},
      { "NOTICE",  PRI_NOTICE},
      { "WARNING", PRI_WARNING},
      { "ERR",     PRI_ERR},
      { "CRIT",    PRI_CRIT},
      { "ALERT",   PRI_ALERT},
      { "EMERG",   PRI_EMERG},
   };
   OsSysLog::initialize(0, "SipProxy");


   //
   // Get/Apply Log Filename
   //
   fileTarget.remove(0) ;
   if ((pConfig->get(CONFIG_SETTING_LOG_DIR, fileTarget) != OS_SUCCESS) || 
      fileTarget.isNull() || !OsFileSystem::exists(fileTarget))
   {
      bSpecifiedDirError = !fileTarget.isNull() ;

      // If the log file directory exists use that, otherwise place the log
      // in the current directory
      OsPath workingDirectory;
      if (OsFileSystem::exists(CONFIG_LOG_DIR))
      {
         fileTarget = CONFIG_LOG_DIR;
         OsPath path(fileTarget);
         path.getNativePath(workingDirectory);

         osPrintf("%s : %s\n", CONFIG_SETTING_LOG_DIR, workingDirectory.data()) ;
         OsSysLog::add(FAC_SIP, PRI_INFO, "%s : %s", CONFIG_SETTING_LOG_DIR, workingDirectory.data()) ;
      }
      else
      {
         OsPath path;
         OsFileSystem::getWorkingDirectory(path);
         path.getNativePath(workingDirectory);

         osPrintf("%s : %s\n", CONFIG_SETTING_LOG_DIR, workingDirectory.data()) ;
         OsSysLog::add(FAC_SIP, PRI_INFO, "%s : %s", CONFIG_SETTING_LOG_DIR, workingDirectory.data()) ;
      }

      fileTarget = workingDirectory + 
         OsPathBase::separator +
         SIP_PROXY_LOG;         
   }
   else
   {
      bSpecifiedDirError = false ;
      osPrintf("%s : %s\n", CONFIG_SETTING_LOG_DIR, fileTarget.data()) ;
      OsSysLog::add(FAC_SIP, PRI_INFO, "%s : %s", CONFIG_SETTING_LOG_DIR, fileTarget.data()) ;

      fileTarget = fileTarget + 
         OsPathBase::separator +
         SIP_PROXY_LOG;
   }
   OsSysLog::setOutputFile(0, fileTarget) ;


   //
   // Get/Apply Log Level
   //
   if ((pConfig->get(CONFIG_SETTING_LOG_LEVEL, logLevel) != OS_SUCCESS) ||
         logLevel.isNull())
   {
      logLevel = "ERR";
   }
   logLevel.toUpper();
   OsSysLogPriority priority = PRI_ERR;
   int iEntries = sizeof(lkupTable)/sizeof(struct tagPrioriotyLookupTable);
   for (int i=0; i<iEntries; i++)
   {
      if (logLevel == lkupTable[i].pIdentity)
      {
         priority = lkupTable[i].ePriority;
         osPrintf("%s : %s\n", CONFIG_SETTING_LOG_LEVEL, lkupTable[i].pIdentity) ;
         OsSysLog::add(FAC_SIP, PRI_INFO, "%s : %s", CONFIG_SETTING_LOG_LEVEL, lkupTable[i].pIdentity) ;
         break;
      }
   }
   OsSysLog::setLoggingPriority(priority);

   //
   // Get/Apply console logging
   //
   if ((pConfig->get(CONFIG_SETTING_LOG_CONSOLE, consoleLogging) == 
         OS_SUCCESS))
   {
      consoleLogging.toUpper();
      if (consoleLogging == "ENABLE")
      {
         OsSysLog::enableConsoleOutput(true);        
         osPrintf("%s : %s\n", CONFIG_SETTING_LOG_CONSOLE, "ENABLE") ;
         OsSysLog::add(FAC_SIP, PRI_INFO, "%s : %s", CONFIG_SETTING_LOG_CONSOLE, "ENABLE") ;
      }
      else
      {
         osPrintf("%s : %s\n", CONFIG_SETTING_LOG_CONSOLE, "DISABLE") ;
         OsSysLog::add(FAC_SIP, PRI_INFO, "%s : %s", CONFIG_SETTING_LOG_CONSOLE, "DISABLE") ;
      }
   }   

   if (bSpecifiedDirError)
   {
      OsSysLog::add(FAC_LOG, PRI_CRIT, "Cannot access %s directory; please check configuration.", CONFIG_SETTING_LOG_DIR);
   }
}

/** The main entry point to the proxy */
int
main(int argc, char* argv[])
{
    // Register Signal handlers to close IMDB
    pt_signal(SIGINT,   sigHandler);    // Trap Ctrl-C on NT
    pt_signal(SIGILL,   sigHandler); 
    pt_signal(SIGABRT,  sigHandler);    // Abort signal 6
    pt_signal(SIGFPE,   sigHandler);    // Floading Point Exception
    pt_signal(SIGSEGV,  sigHandler);    // Address access violations signal 11 
    pt_signal(SIGTERM,  sigHandler);    // Trap kill -15 on UNIX
#if defined(__pingtel_on_posix__)
    pt_signal(SIGHUP,   sigHandler);    // Hangup
    pt_signal(SIGQUIT,  sigHandler); 
    pt_signal(SIGPIPE,  sigHandler);    // Handle TCP Failure
    pt_signal(SIGBUS,   sigHandler); 
    pt_signal(SIGSYS,   sigHandler); 
    pt_signal(SIGXCPU,  sigHandler); 
    pt_signal(SIGXFSZ,  sigHandler);
    pt_signal(SIGUSR1,  sigHandler); 
    pt_signal(SIGUSR2,  sigHandler); 
#endif

    UtlString argString;
    UtlBoolean interactiveSet = false;

    for(int argIndex = 1; argIndex < argc; argIndex++)
    {
        osPrintf("arg[%d]: %s\n", argIndex, argv[argIndex]);
        argString = argv[argIndex];
        NameValueTokenizer::frontBackTrim(&argString, "\t ");
        if(argString.compareTo("-v") == 0)
        {
            osPrintf("Version: %s\n", SIPX_VERSION);
            return(1);
        }
        else if( argString.compareTo("-i") == 0)
        {
           interactiveSet = true;
           osPrintf("Entering Interactive Mode\n");
        }
         else
         {
            osPrintf("usage: %s [-v] [-i]\nwhere:\n -v provides the software version\n"
               " -i starts the server in an interactive mode\n",
            argv[0]);
            return(1);
         }
    }
   

    int proxyTcpPort;
    int proxyUdpPort;
    UtlString domainName;
    UtlString proxyRecordRoute;
    int maxForwards;
    OsConfigDb configDb;
    UtlString ipAddress;

    OsSocket::getHostIp(&ipAddress);
 /*Config files which are specific to a component 
   (e.g. mappingrules.xml is to sipregistrar) Use the 
   following logic: 
   1) If  directory ../etc exists: 
       The path to the data file is as follows 
       ../etc/<data-file-name> 

   2) Else the path is assumed to be: 
      ./<data-file-name> 
   */
   
   OsPath workingDirectory;
   if ( OsFileSystem::exists( CONFIG_ETC_DIR ) )
   {
      workingDirectory = CONFIG_ETC_DIR;
      OsPath path(workingDirectory);
      path.getNativePath(workingDirectory);

   } 
   else
   {
      OsPath path;
      OsFileSystem::getWorkingDirectory(path);
      path.getNativePath(workingDirectory);
   }

    UtlString ConfigfileName =  workingDirectory + 
      OsPathBase::separator +
      "proxy-config";

    if(configDb.loadFromFile(ConfigfileName) == OS_SUCCESS)
    {      
      osPrintf("Found config file: %s\n", ConfigfileName.data());
    }
    else
    {
        configDb.set("SIP_PROXY_UDP_PORT", "");
        configDb.set("SIP_PROXY_TCP_PORT", "");
        //configDb.set("SIP_PROXY_DOMAIN_NAME", "");
        //configDb.set("SIP_PROXY_RECORD_ROUTE", "DISABLE");
        configDb.set("SIP_PROXY_MAX_FORWARDS", "");
        configDb.set("SIP_PROXY_USE_AUTH_SERVER", "");
        configDb.set("SIP_PROXY_AUTH_SERVER", "");
        configDb.set("SIP_PROXY_DEFAULT_EXPIRES", "");
        configDb.set("SIP_PROXY_DEFAULT_SERIAL_EXPIRES", "");
        configDb.set("SIP_PROXY_HOST_ALIASES", "");
        //configDb.set("SIP_PROXY_BRANCH_TIMEOUT", "");
        configDb.set("SIP_PROXY_STALE_TCP_TIMEOUT", ""); 
        configDb.set(CONFIG_SETTING_LOG_DIR, "");
        configDb.set(CONFIG_SETTING_LOG_LEVEL, "");
        configDb.set(CONFIG_SETTING_LOG_CONSOLE, "");

        if(configDb.storeToFile(ConfigfileName) != OS_SUCCESS)
        {
           osPrintf("Could not write config file: %s\n", ConfigfileName.data());
        }
    }

    // Initialize the OsSysLog...
    initSysLog(&configDb);    

    configDb.get("SIP_PROXY_DOMAIN_NAME", domainName);
    if(domainName.isNull())
    {
        domainName = ipAddress;
    }
    OsSysLog::add(FAC_SIP, PRI_INFO, "SIP_PROXY_DOMAIN_NAME : %s", domainName.data());
    osPrintf("SIP_PROXY_DOMAIN_NAME : %s\n", domainName.data());

    configDb.get("SIP_PROXY_UDP_PORT", proxyUdpPort);
    if(proxyUdpPort <=0) proxyUdpPort = 5060;
    OsSysLog::add(FAC_SIP, PRI_INFO, "SIP_PROXY_UDP_PORT : %d", proxyUdpPort);
    osPrintf("SIP_PROXY_UDP_PORT : %d\n", proxyUdpPort);


    configDb.get("SIP_PROXY_TCP_PORT", proxyTcpPort);
    if(proxyTcpPort <=0) proxyTcpPort = 5060;
    OsSysLog::add(FAC_SIP, PRI_INFO, "SIP_PROXY_TCP_PORT : %d", proxyTcpPort);
    osPrintf("SIP_PROXY_TCP_PORT : %d\n", proxyTcpPort);

    configDb.get("SIP_PROXY_RECORD_ROUTE", proxyRecordRoute);
    UtlBoolean recordRouteEnabled = FALSE;
    proxyRecordRoute.toLower();
    if(proxyRecordRoute.compareTo("enable") == 0)
    {
        recordRouteEnabled = TRUE;
        OsSysLog::add(FAC_SIP, PRI_INFO, "SIP_PROXY_RECORD_ROUTE : ENABLE");
        osPrintf("SIP_PROXY_RECORD_ROUTE : ENABLE\n");
    }    

    configDb.get("SIP_PROXY_MAX_FORWARDS", maxForwards);
    if(maxForwards <= 0) maxForwards = SIP_DEFAULT_MAX_FORWARDS;
    OsSysLog::add(FAC_SIP, PRI_INFO, "SIP_PROXY_MAX_FORWARDS : %d", maxForwards);
    osPrintf("SIP_PROXY_MAX_FORWARDS : %d\n", maxForwards);

    int branchTimeout = -1;
    configDb.get("SIP_PROXY_BRANCH_TIMEOUT", branchTimeout);
    if(branchTimeout < 4)
    {
        branchTimeout = 24;
    }

    UtlBoolean authEnabled = TRUE;
    UtlString authServerEnabled;
    configDb.get("SIP_PROXY_USE_AUTH_SERVER", authServerEnabled);
    authServerEnabled.toLower();
    if(authServerEnabled.compareTo("disable") == 0)
    {
        authEnabled = FALSE;
    }
    OsSysLog::add(FAC_SIP, PRI_INFO, "SIP_PROXY_USE_AUTH_SERVER : %s", authEnabled ? "ENABLE" : "DISABLE");
    osPrintf("SIP_PROXY_USE_AUTH_SERVER : %s\n", authEnabled ? "ENABLE" : "DISABLE");

    UtlString authServer;
    configDb.get("SIP_PROXY_AUTH_SERVER", authServer);
    if(authEnabled &&
       authServer.isNull())
    {
        authServer = ipAddress;
        authServer.append(":5080");
    }
    OsSysLog::add(FAC_SIP, PRI_INFO, "SIP_PROXY_AUTH_SERVER : %s", authServer.data());
    osPrintf("SIP_PROXY_AUTH_SERVER : %s\n", authServer.data());

    int defaultExpires;
    int defaultSerialExpires;
    configDb.get("SIP_PROXY_DEFAULT_EXPIRES", defaultExpires);
    configDb.get("SIP_PROXY_DEFAULT_SERIAL_EXPIRES", defaultSerialExpires);
    if(defaultExpires <= 0 ||
       defaultExpires > 180) defaultExpires = 180;
    if(defaultSerialExpires <= 0 ||
       defaultSerialExpires >= defaultExpires) defaultSerialExpires = 20;
    OsSysLog::add(FAC_SIP, PRI_INFO, "SIP_PROXY_DEFAULT_EXPIRES : %d", defaultExpires);
    osPrintf("SIP_PROXY_DEFAULT_EXPIRES : %d\n", defaultExpires);
    OsSysLog::add(FAC_SIP, PRI_INFO, "SIP_PROXY_DEFAULT_SERIAL_EXPIRES : %d", defaultSerialExpires);
    osPrintf("SIP_PROXY_DEFAULT_SERIAL_EXPIRES : %d\n", defaultSerialExpires);

    // Set the maximum amount of time that TCP connections can
    // stay around when they are not used.
    int      staleTcpTimeout = 3600;
    UtlString staleTcpTimeoutStr;

    // Check for missing parameter or empty value
    configDb.get("SIP_PROXY_STALE_TCP_TIMEOUT", staleTcpTimeoutStr);
    if (staleTcpTimeoutStr.isNull())
    {
        staleTcpTimeout = 3600;
    }
    else
    {
        // get the parameter value as an integer
        configDb.get("SIP_PROXY_STALE_TCP_TIMEOUT", staleTcpTimeout);
    }

    if(staleTcpTimeout <= 0) staleTcpTimeout = -1;
    else if(staleTcpTimeout < 180) staleTcpTimeout = 180;
    OsSysLog::add(FAC_SIP, PRI_INFO, "SIP_PROXY_STALE_TCP_TIMEOUT : %d",
                  staleTcpTimeout);
    osPrintf("SIP_PROXY_STALE_TCP_TIMEOUT : %d\n", staleTcpTimeout);

    int maxNumSrvRecords = -1;
    configDb.get("SIP_PROXY_DNSSRV_MAX_DESTS", maxNumSrvRecords);
    OsSysLog::add(FAC_SIP, PRI_INFO, "SIP_PROXY_DNSSRV_MAX_DESTS : %d",
              maxNumSrvRecords);
    // If explicitly set to a valid number
    if(maxNumSrvRecords > 0)
    {
        osPrintf("SIP_PROXY_DNSSRV_MAX_DESTS : %d\n", maxNumSrvRecords);
    }
    else
    {
        maxNumSrvRecords = 4;
    }

    int dnsSrvTimeout = -1; //seconds
    configDb.get("SIP_PROXY_DNSSRV_TIMEOUT", dnsSrvTimeout);
        OsSysLog::add(FAC_SIP, PRI_INFO, "SIP_PROXY_DNSSRV_TIMEOUT : %d",
                  dnsSrvTimeout);
    // If explicitly set to a valid number
    if(dnsSrvTimeout > 0)
    {
        osPrintf("SIP_PROXY_DNSSRV_TIMEOUT : %d\n", dnsSrvTimeout);
    }
    else
    {
        dnsSrvTimeout = 4;
    }

    UtlString hostAliases;
    configDb.get("SIP_PROXY_HOST_ALIASES", hostAliases);
    if(hostAliases.isNull())
    {
        hostAliases = ipAddress;
        char portBuf[20];
        sprintf(portBuf, ":%d", proxyUdpPort);
        hostAliases.append(portBuf);
    }
    OsSysLog::add(FAC_SIP, PRI_INFO, "SIP_PROXY_HOST_ALIASES : %s",
                  hostAliases.data());
    osPrintf("SIP_PROXY_HOST_ALIASES : %s\n", hostAliases.data());

    // This is an obnoxious special option to work around a 
    // problem with Sonus gateways.  The Sonus proxy or  redirect
    // server gives a list of possible gateways to recurse in a
    // 300 response.  It does not assign any Q values so the proxy
    // gets the impression that it should fork them all in parallel.
    // When this option is enabled we recurse only the one with the
    // highest Q value.
    UtlString recurseOnlyOne300String;
    configDb.get("SIP_PROXY_SPECIAL_300", recurseOnlyOne300String);
    recurseOnlyOne300String.toLower();
    UtlBoolean recurseOnlyOne300 = FALSE;
    if(recurseOnlyOne300String.compareTo("enable") == 0) 
    {
        recurseOnlyOne300 = TRUE;
        OsSysLog::add(FAC_SIP, PRI_INFO, "SIP_PROXY_SPECIAL_300 : ENABLE");
        osPrintf("SIP_PROXY_SPECIAL_300 : ENABLE\n");
    }

    // Get the mapped and local domains
    OsConfigDb mappedDomains ;
    configDb.getSubHash("SIP_DOMAINS.", mappedDomains);
    if(mappedDomains.isEmpty())
    {
        //UtlString proxydomain(ipAddress);
        //proxydomain.append(":5060");
        //UtlString registryDomain(ipAddress);
        //registryDomain.append(":4000");
        //mappedDomains.set(proxydomain, registryDomain.data());
    }
    else
    {
        OsSysLog::add(FAC_SIP, PRI_WARNING, "WARNING: SIP_DOMAINS. parameters IGNORED");
    }

    // Initialize the domaim mapping from the routeRules XML
    // file
    //OsConfigDb mapRulesDb;
   

    UtlString fileName ;
    fileName =  workingDirectory + 
      OsPathBase::separator +
      FORWARDING_RULES_FILENAME  ;

    ForwardRules forwardingRules;

    OsFile ruleFile(fileName);
    UtlBoolean useDefaultRules = FALSE;
    if(ruleFile.exists())
    {
        if(OS_SUCCESS != forwardingRules.loadMappings(fileName))
        {
            OsSysLog::add(FAC_SIP, PRI_WARNING, "WARNING: Failed to load: %s",
                fileName.data());
            osPrintf("WARNING: Failed to load: %s\n", fileName.data());
            useDefaultRules = TRUE;
        }
    }
    else
    {
        OsSysLog::add(FAC_SIP, PRI_INFO, "%s not found",
            fileName.data());
        osPrintf("%s not found\n",
            fileName.data());
        useDefaultRules = TRUE;
    }

    if(useDefaultRules)
    {
        OsSysLog::add(FAC_SIP, PRI_INFO, "using default forwarding rules");
        osPrintf("using default forwarding rules\n");

        UtlString localDomain;
        OsSocket::getDomainName(localDomain);
        UtlString hostName;
        OsSocket::getHostName(&hostName);
        UtlString ipAddress;
        OsSocket::getHostIp(&ipAddress);
        UtlString fqhn(hostName);
        fqhn.append('.');
        fqhn.append(localDomain);

        forwardingRules.buildDefaultRules(localDomain.data(),
                                          hostName.data(),
                                          ipAddress.data(),
                                          fqhn,
                                          proxyUdpPort);
    }

#ifdef TEST_PRINT
    { // scope the test stuff
        SipMessage foo;
        const char* uri = "sip:10.1.20.3:5100";
        const char* method = "ACK"; //SIP_SUBSCRIBE_METHOD;
        const char* eventType = "sip-config"; //SIP_EVENT_CONFIG;
        foo.setRequestData(method, 
                           uri, //uri, 
                           "sip:1234@doty.com", // fromField, 
                           "\"lajdflsdk ff\"<sip:laksdjf@1234.x.com>", // toField, 
                           "lkadsj902387", // callId, 
                           123, // CSeq,
                           "sip:10.1.1.123");// contactUrl

        // Set the event type
        foo.setHeaderValue(SIP_EVENT_FIELD, 
                            eventType, // event type
                            0);

        Url msgUrl(uri);
        UtlString routeTo;
        UtlString routeType;
        OsStatus routeStatus = forwardingRules.getRoute(msgUrl, 
                                                        foo, 
                                                        routeTo, 
                                                        routeType);

        Url msgRouteToUri(routeTo);
        osPrintf("Message:\n\tmethod: %s\n\turi: %s\n\tevent: %s\nRouted:\n\tstring: %s\n\turi: %s\n\ttype: %s\n",
            method, uri, eventType, routeTo.data(), msgRouteToUri.toString().data(), routeType.data());
        if(routeStatus != OS_SUCCESS) 
            osPrintf("forwardingRules.getRoute returned: %d\n",
                    routeStatus);
    }
#endif // TEST_PRINT


    //initRoutes(fileName, proxyUdpPort, mapRulesDb);

    // Start the sip stack
    SipUserAgent sipUserAgent(proxyTcpPort, 
        proxyUdpPort,
        NULL, // public IP address (nopt used in proxy)
        NULL, // default user (not used in proxy)
        NULL, // default SIP address (not used in proxy)
        (authEnabled && !authServer.isNull()) ? authServer.data() : NULL,
        NULL, // directory server
        NULL, // registry server
        NULL, // auth scheme
        NULL, //auth realm
        NULL, // auth DB
        NULL, // auth user IDs
        NULL, // auth passwords
        NULL, // nat ping URL
        0, // nat ping frequency
        "PING", // nat ping method
        NULL, // line mgr
        SIP_DEFAULT_RTT, // first resend timeout
        TRUE, // default to UA transaction
        SIPUA_DEFAULT_SERVER_UDP_BUFFER_SIZE, // socket layer read buffer size
        SIPUA_DEFAULT_SERVER_OSMSG_QUEUE_SIZE // OsServerTask message queue size
        );
    sipUserAgent.setIsUserAgent(FALSE);
    sipUserAgent.setMaxForwards(maxForwards);
    sipUserAgent.setDnsSrvTimeout(dnsSrvTimeout);
    sipUserAgent.setMaxSrvRecords(maxNumSrvRecords);

    // No need for this log as it goes in the syslog as well
    //sipUserAgent.startMessageLog(100000);

    sipUserAgent.setDefaultExpiresSeconds(defaultExpires);
    sipUserAgent.setDefaultSerialExpiresSeconds(defaultSerialExpires);
    sipUserAgent.setMaxTcpSocketIdleTime(staleTcpTimeout);
    sipUserAgent.setHostAliases(hostAliases);
    sipUserAgent.setRecurseOnlyOne300Contact(recurseOnlyOne300);
    sipUserAgent.start();

    UtlString buffer;

    // Create and start a router to route stuff either
    // to a local server or on out to the real world
    SipRouter router(sipUserAgent, 
                    forwardingRules,
                    authEnabled, 
                    authServer.data(), 
                    recordRouteEnabled);
    router.start();

    // Do not exit, let the proxy do its stuff
    while( !gShutdownFlag )
    {
      if( interactiveSet)
      {
         int charCode = getchar();

         if(charCode != '\n' && charCode != '\r')
         {
            if( charCode == 'e')
            {
               OsSysLog::enableConsoleOutput(TRUE);
            }
            else if( charCode == 'd')
            {
               OsSysLog::enableConsoleOutput(FALSE);
            }
#ifdef BOUNDS_CHECKER
            else if( charCode == 'b')
            {
              NMMemPopup( );
            }
#endif
            else
            {
               sipUserAgent.printStatus();
               sipUserAgent.getMessageLog(buffer);
               printf("=================>\n%s\n", buffer.data());
            }
         }
      }
      else
         OsTask::delay(2000);
    }

    // Flush the log file
    OsSysLog::flush();

    return(1);
}

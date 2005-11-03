// 
// 
// Copyright (C) 2005 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
// 
// Copyright (C) 2005 Pingtel Corp.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// $$
//////////////////////////////////////////////////////////////////////////////
#ifndef _CONFIGRPC_H_
#define _CONFIGRPC_H_

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "utl/UtlHashBag.h"
#include "net/HttpRequestContext.h"
#include "net/XmlRpcMethod.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
class OsRWMutex;
class XmlRpcDispatch;
class ConfigRPC_Callback;


/// XML-RPC interface to an OsConfigDb
/**
 * The ConfigRPC object holds the state of the connection between XMLRPC and a
 * particular OsConfigDb store.  The name of the OsConfigDb is stored in the
 * parent UtlString, and is used by the XMLRPC objects to locate the appropriate
 * database.
 *
 * The connection to the controlling application is through a ConfigRPC_Callback
 * object passed to the constructor.  This object provides methods that are called
 * by ConfigRPC to control access to the database, to indicate that the database has
 * been modified, and to indicate that the database has been deleted.
 */
class ConfigRPC : public UtlString
{
  public:

   /// Construct an instance to allow RPC access to a database.
   ConfigRPC( const char*         dbName     ///< dbName known to XMLRPC methods
             ,const char*         versionId  ///< version of this database
             ,const UtlString&    dbPath     ///< path to persistent store for this db
             ,ConfigRPC_Callback* callback   ///< connection to controlling application
             );
   /**
    * To allow use of an OsConfigDb via this module, create a ConfigRPC object:
    * @code
    * // start the rpc dispatcher
    * XmlRpcDispatch rpc( httpPort, true );
    *
    * // connect the dataset to XmlRpc
    * UtlString databasePath("/path/to/facility-config");
    * ConfigRPC_Callback defaultCallbacks;
    * ConfigRPC ConfigDbAccess("facility-config", databasePath, facilityVersion, defaultCallbacks);
    *
    * // enter the connector RPC methods in the XmlRpcDispatch table
    * ConfigRPC::registerMethods(rpc);
    * @endcode
    * The object passed in to provide callbacks may be a subclass of ConfigRPC_Callback that
    * implements more intelligent actions.
    */

   typedef enum
      {
         loadFailed  = 100,
         storeFailed = 101,
         invalidType = 102,
         nameNotFound = 103,
         emptyRequestList = 104         
      } FailureCode;

   /// Destroy the instance to disconnect access to the database.
   ~ConfigRPC();

   /// Must be called once to connect the configurationParameter methods
   static void registerMethods(XmlRpcDispatch&     rpc /**< xmlrpc dispatch service to use */);
   
  protected:

   /// Locate a ConfigRPC by its dbName - caller must hold at least a read lock on spDatabaseLock
   static ConfigRPC* find(const UtlString& dbName)
      {
         return dynamic_cast<ConfigRPC*>(sDatabases.find(&dbName));
      }

   /// Open and load the associated OsConfigDb dataset
   OsStatus load(OsConfigDb& dataset);

   /// Write the contents of the associated OsConfigDb dataset to its file
   OsStatus store(OsConfigDb& dataset);
   
   friend class ConfigRPC_version;
   friend class ConfigRPC_set;
   friend class ConfigRPC_get;
   
  private:

   static OsRWMutex* spDatabaseLock; ///< protects access to sDatabases and sRegistered
   static UtlHashBag sDatabases;     ///< locates the ConfigRPC object for a db name
   static bool       sRegistered;    /**< whether or not the ConfigRPC methods have been
                                      *   registered with XmlRpcDispatch
                                      */
   
   UtlString           mVersion;     ///< database version identifier
   UtlString           mPath;        ///< path to persistent store for the database
   ConfigRPC_Callback* mCallback;

   /// no copy constructor
   ConfigRPC(const ConfigRPC& nocopy);

   /// no assignment operator
   ConfigRPC& operator=(const ConfigRPC& noassignment);
   
};

/// Base class for callbacks from ConfigRPC
/**
 * The facility whose configuration is being accessed using XML-RPC through ConfigRPC
 * may derive a class from this that implements additional actions.
 *
 * The default actions provided by the base class are to allow all access (:TODO: this will chnage)
 * and no-ops for the modified and deleted callbacks.
 */
class ConfigRPC_Callback
{
  public:

   ConfigRPC_Callback();

   /// Method identifiers for accessAllowed
   typedef enum
      {
         Version,         ///< configurationParameter.version
         Get,             ///< configurationParameter.get
         Set,             ///< configurationParameter.set
         Delete,          ///< configurationParameter.delete
         datasetDelete    ///< configurationParameter.datasetDelete
      } Method;

   /// Access check function 
   virtual XmlRpcMethod::ExecutionStatus accessAllowed( const HttpRequestContext& requestContext
                                                       ,Method                    method
                                                       );
   /**<
    * @returns XmlRpcMethod::OK if allowed, XmlRpcMethod::FAILED if not allowed,
    * and XmlRpcMethod::REQUIRE_AUTHENTICATION if authentication is missing or invalid.
    */

   /// Invoked after the database has been modified
   virtual void modified();
   
   /// Invoked after the database has been deleted
   virtual void deleted();

   virtual ~ConfigRPC_Callback();

  private:
   /// no copy constructor
   ConfigRPC_Callback(const ConfigRPC_Callback& nocopy);

   /// no assignment operator
   ConfigRPC_Callback& operator=(const ConfigRPC_Callback& noassignment);
   
};

#endif // _CONFIGRPC_H_

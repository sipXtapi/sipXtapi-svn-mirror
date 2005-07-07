// 
// Copyright (C) 2005 Pingtel Corp.
// 
//////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
#include "assert.h"

// APPLICATION INCLUDES
#include "utl/UtlString.h"
#include "utl/UtlHashMap.h"
#include "utl/PluginHook.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
class TestPlugin;


extern "C" TestPlugin* getHook(const UtlString& name);

/**
 * TestPlugin defines the interface for plugins to the PluginHooksTest.
 *
 */
class TestPlugin : public PluginHook
{
public:

   virtual ~TestPlugin();

   /// Read (or re-read) whatever configuration the hook requires.
   void readConfig( OsConfigDb& configDb );

   /// Return the integer value for a given configuration key
   virtual bool getConfiguredValueFor(const UtlString& key, UtlString& value ) const;

   /// Set type to the unique library name
   virtual void pluginName(UtlString& name) const;

   static const char* LibraryName;   

private:
   friend TestPlugin* getHook(const UtlString& name);
   
   /// The constructor is called from getHook factory method.
   TestPlugin(const UtlString& hookName);

   UtlHashMap  mConfiguration;
   bool        mConfigured;
};


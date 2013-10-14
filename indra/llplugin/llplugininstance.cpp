/** 
 * @file llplugininstance.cpp
 * @brief LLPluginInstance handles loading the dynamic library of a plugin and setting up its entry points for message passing.
 *
 * @cond
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 * @endcond
 */

#include "linden_common.h"

#include "llplugininstance.h"

#include "llapr.h"

#if LL_WINDOWS
#include "direct.h"	// needed for _chdir()
#endif

/** Virtual destructor. */
LLPluginInstanceMessageListener::~LLPluginInstanceMessageListener()
{
}

/** 
 * TODO:DOC describe how it's used
 */
const char *LLPluginInstance::PLUGIN_INIT_FUNCTION_NAME = "LLPluginInitEntryPoint";

/** 
 * Constructor.
 *
 * @param[in] owner Plugin instance. TODO:DOC is this a good description of what "owner" is?
 */
LLPluginInstance::LLPluginInstance(LLPluginInstanceMessageListener *owner) :
	mDSOHandle(NULL),
	mPluginUserData(NULL),
	mPluginSendMessageFunction(NULL)
{
	mOwner = owner;
}

/** 
 * Destructor.
 */
LLPluginInstance::~LLPluginInstance()
{
	if(mDSOHandle != NULL)
	{
		apr_dso_unload(mDSOHandle);
		mDSOHandle = NULL;
	}
}

/** 
 * Dynamically loads the plugin and runs the plugin's init function.
 *
 * @param[in] plugin_file Name of plugin dll/dylib/so. TODO:DOC is this correct? see .h
 * @return 0 if successful, APR error code or error code from the plugin's init function on failure.
 */
int LLPluginInstance::load(const std::string& plugin_dir, std::string &plugin_file)
{
	pluginInitFunction init_function = NULL;
	
	if ( plugin_dir.length() )
	{
#if LL_WINDOWS
		// VWR-21275:
		// *SOME* Windows systems fail to load the Qt plugins if the current working
		// directory is not the same as the directory with the Qt DLLs in.
		// This should not cause any run time issues since we are changing the cwd for the
		// plugin shell process and not the viewer.
		// Changing back to the previous directory is not necessary since the plugin shell
		// quits once the plugin exits.
		_chdir( plugin_dir.c_str() );	
#endif
	};

	int result = apr_dso_load(&mDSOHandle,
					  plugin_file.c_str(),
					  gAPRPoolp);
	if(result != APR_SUCCESS)
	{
		char buf[1024];
		apr_dso_error(mDSOHandle, buf, sizeof(buf));

		LL_WARNS("Plugin") << "apr_dso_load of " << plugin_file << " failed with error " << result << " , additional info string: " << buf << LL_ENDL;
		
	}
	
	if(result == APR_SUCCESS)
	{
		result = apr_dso_sym((apr_dso_handle_sym_t*)&init_function,
						 mDSOHandle,
						 PLUGIN_INIT_FUNCTION_NAME);

		if(result != APR_SUCCESS)
		{
			LL_WARNS("Plugin") << "apr_dso_sym failed with error " << result << LL_ENDL;
		}
	}
	
	if(result == APR_SUCCESS)
	{
		result = init_function(staticReceiveMessage, (void*)this, &mPluginSendMessageFunction, &mPluginUserData);

		if(result != APR_SUCCESS)
		{
			LL_WARNS("Plugin") << "call to init function failed with error " << result << LL_ENDL;
		}
	}
	
	return (int)result;
}

/** 
 * Sends a message to the plugin.
 *
 * @param[in] message Message
 */
void LLPluginInstance::sendMessage(const std::string &message)
{
	if(mPluginSendMessageFunction)
	{
		LL_DEBUGS("Plugin") << "sending message to plugin: \"" << message << "\"" << LL_ENDL;
		mPluginSendMessageFunction(message.c_str(), &mPluginUserData);
	}
	else
	{
		LL_WARNS("Plugin") << "dropping message: \"" << message << "\"" << LL_ENDL;
	}
}

/**
 * Idle. TODO:DOC what's the purpose of this?
 *
 */
void LLPluginInstance::idle(void)
{
}

// static
void LLPluginInstance::staticReceiveMessage(const char *message_string, void **user_data)
{
	// TODO: validate that the user_data argument is still a valid LLPluginInstance pointer
	// we could also use a key that's looked up in a map (instead of a direct pointer) for safety, but that's probably overkill
	LLPluginInstance *self = (LLPluginInstance*)*user_data;
	self->receiveMessage(message_string);
}

/**
 * Plugin receives message from plugin loader shell.
 *
 * @param[in] message_string Message
 */
void LLPluginInstance::receiveMessage(const char *message_string)
{
	if(mOwner)
	{
		LL_DEBUGS("Plugin") << "processing incoming message: \"" << message_string << "\"" << LL_ENDL;		
		mOwner->receivePluginMessage(message_string);
	}
	else
	{
		LL_WARNS("Plugin") << "dropping incoming message: \"" << message_string << "\"" << LL_ENDL;		
	}	
}

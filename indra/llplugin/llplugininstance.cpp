/** 
 * @file llplugininstance.cpp
 * @brief LLPluginInstance handles loading the dynamic library of a plugin and setting up its entry points for message passing.
 *
 * @cond
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 *
 * Copyright (c) 2008, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 * @endcond
 */

#include "linden_common.h"

#include "llplugininstance.h"

#include "llapr.h"

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
int LLPluginInstance::load(std::string &plugin_file)
{
	pluginInitFunction init_function = NULL;
	
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

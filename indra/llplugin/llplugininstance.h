/** 
 * @file llplugininstance.h
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

#ifndef LL_LLPLUGININSTANCE_H
#define LL_LLPLUGININSTANCE_H

#include "llstring.h"
#include "llapr.h"

#include "apr_dso.h"

/**
 * @brief LLPluginInstanceMessageListener receives messages sent from the plugin loader shell to the plugin.
 */
class LLPluginInstanceMessageListener
{
public:
	virtual ~LLPluginInstanceMessageListener();
   /** Plugin receives message from plugin loader shell. */
	virtual void receivePluginMessage(const std::string &message) = 0;
};

/**
 * @brief LLPluginInstance handles loading the dynamic library of a plugin and setting up its entry points for message passing.
 */
class LLPluginInstance
{
	LOG_CLASS(LLPluginInstance);
public:
	LLPluginInstance(LLPluginInstanceMessageListener *owner);
	virtual ~LLPluginInstance();
	
	// Load a plugin dll/dylib/so
	// Returns 0 if successful, APR error code or error code returned from the plugin's init function on failure.
	int load(const std::string& plugin_dir, std::string &plugin_file);
	
	// Sends a message to the plugin.
	void sendMessage(const std::string &message);
	
   // TODO:DOC is this comment obsolete? can't find "send_count" anywhere in indra tree.
	// send_count is the maximum number of message to process from the send queue.  If negative, it will drain the queue completely.
	// The receive queue is always drained completely.
	// Returns the total number of messages processed from both queues.
	void idle(void);
	
	/** The signature of the function for sending a message from plugin to plugin loader shell.
    *
	 * @param[in] message_string Null-terminated C string 
    * @param[in] user_data The opaque reference that the callee supplied during setup.
    */
	typedef void (*sendMessageFunction) (const char *message_string, void **user_data);

	/** The signature of the plugin init function. TODO:DOC check direction (pluging loader shell to plugin?)
    *
    * @param[in] host_user_data Data from plugin loader shell.
    * @param[in] plugin_send_function Function for sending from the plugin loader shell to plugin.
    */
	typedef int (*pluginInitFunction) (sendMessageFunction host_send_func, void *host_user_data, sendMessageFunction *plugin_send_func, void **plugin_user_data);
	
   /** Name of plugin init function */
	static const char *PLUGIN_INIT_FUNCTION_NAME;
	
private:
	static void staticReceiveMessage(const char *message_string, void **user_data);
	void receiveMessage(const char *message_string);

	apr_dso_handle_t *mDSOHandle;
	
	void *mPluginUserData;
	sendMessageFunction mPluginSendMessageFunction;
	
	LLPluginInstanceMessageListener *mOwner;
};

#endif // LL_LLPLUGININSTANCE_H

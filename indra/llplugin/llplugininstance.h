/** 
 * @file llplugininstance.h
 * @brief LLPluginInstance handles loading the dynamic library of a plugin and setting up its entry points for message passing.
 *
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
 */

#ifndef LL_LLPLUGININSTANCE_H
#define LL_LLPLUGININSTANCE_H

#include "llstring.h"
#include "llapr.h"

#include "apr_dso.h"

class LLPluginInstanceMessageListener
{
public:
	virtual ~LLPluginInstanceMessageListener();
	virtual void receivePluginMessage(const std::string &message) = 0;
};

class LLPluginInstance
{
	LOG_CLASS(LLPluginInstance);
public:
	LLPluginInstance(LLPluginInstanceMessageListener *owner);
	virtual ~LLPluginInstance();
	
	// Load a plugin dll/dylib/so
	// Returns 0 if successful, APR error code or error code returned from the plugin's init function on failure.
	int load(std::string &plugin_file);
	
	// Sends a message to the plugin.
	void sendMessage(const std::string &message);
	
	// send_count is the maximum number of message to process from the send queue.  If negative, it will drain the queue completely.
	// The receive queue is always drained completely.
	// Returns the total number of messages processed from both queues.
	void idle(void);
	
	// this is the signature of the "send a message" function.  
	// message_string is a null-terminated C string
	// user_data is the opaque reference that the callee supplied during setup.
	typedef void (*sendMessageFunction) (const char *message_string, void **user_data);

	// signature of the plugin init function
	typedef int (*pluginInitFunction) (sendMessageFunction host_send_func, void *host_user_data, sendMessageFunction *plugin_send_func, void **plugin_user_data);
	
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

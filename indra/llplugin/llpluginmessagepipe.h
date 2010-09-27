/** 
 * @file llpluginmessagepipe.h
 * @brief Classes that implement connections from the plugin system to pipes/pumps.
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

#ifndef LL_LLPLUGINMESSAGEPIPE_H
#define LL_LLPLUGINMESSAGEPIPE_H

#include "lliosocket.h"
#include "llthread.h"

class LLPluginMessagePipe;

// Inherit from this to be able to receive messages from the LLPluginMessagePipe
class LLPluginMessagePipeOwner
{
	LOG_CLASS(LLPluginMessagePipeOwner);
public:
	LLPluginMessagePipeOwner();
	virtual ~LLPluginMessagePipeOwner();
	// called with incoming messages
	virtual void receiveMessageRaw(const std::string &message) = 0;
	// called when the socket has an error
	virtual apr_status_t socketError(apr_status_t error);

	// called from LLPluginMessagePipe to manage the connection with LLPluginMessagePipeOwner -- do not use!
	virtual void setMessagePipe(LLPluginMessagePipe *message_pipe);

protected:
	// returns false if writeMessageRaw() would drop the message
	bool canSendMessage(void);
	// call this to send a message over the pipe
	bool writeMessageRaw(const std::string &message);
	// call this to close the pipe
	void killMessagePipe(void);
	
	LLPluginMessagePipe *mMessagePipe;
	apr_status_t mSocketError;
};

class LLPluginMessagePipe
{
	LOG_CLASS(LLPluginMessagePipe);
public:
	LLPluginMessagePipe(LLPluginMessagePipeOwner *owner, LLSocket::ptr_t socket);
	virtual ~LLPluginMessagePipe();
	
	bool addMessage(const std::string &message);
	void clearOwner(void);
	
	bool pump(F64 timeout = 0.0f);
	bool pumpOutput();
	bool pumpInput(F64 timeout = 0.0f);
		
protected:	
	void processInput(void);

	// used internally by pump()
	void setSocketTimeout(apr_interval_time_t timeout_usec);
	
	LLMutex mInputMutex;
	std::string mInput;
	LLMutex mOutputMutex;
	std::string mOutput;

	LLPluginMessagePipeOwner *mOwner;
	LLSocket::ptr_t mSocket;
};

#endif // LL_LLPLUGINMESSAGE_H

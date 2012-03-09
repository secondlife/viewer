/** 
 * @file llpluginmessagepipe.h
 * @brief Classes that implement connections from the plugin system to pipes/pumps.
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
	std::string::size_type mOutputStartIndex;

	LLPluginMessagePipeOwner *mOwner;
	LLSocket::ptr_t mSocket;
};

#endif // LL_LLPLUGINMESSAGE_H

/** 
 * @file llpluginprocesschild.h
 * @brief LLPluginProcessChild handles the child side of the external-process plugin API. 
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

#ifndef LL_LLPLUGINPROCESSCHILD_H
#define LL_LLPLUGINPROCESSCHILD_H

#include "llpluginmessage.h"
#include "llpluginmessagepipe.h"
#include "llplugininstance.h"
#include "llhost.h"
#include "llpluginsharedmemory.h"

class LLPluginInstance;

class LLPluginProcessChild: public LLPluginMessagePipeOwner, public LLPluginInstanceMessageListener
{
	LOG_CLASS(LLPluginProcessChild);
public:
	LLPluginProcessChild();
	~LLPluginProcessChild();

	void init(U32 launcher_port);
	void idle(void);
	void sleep(F64 seconds);
	void pump();

	// returns true if the plugin is in the steady state (processing messages)
	bool isRunning(void);
	
	// returns true if the plugin is unloaded or we're in an unrecoverable error state.	
	bool isDone(void);
	
	void killSockets(void);

	F64 getSleepTime(void) const { return mSleepTime; };
	
	void sendMessageToPlugin(const LLPluginMessage &message);
	void sendMessageToParent(const LLPluginMessage &message);
	
	// Inherited from LLPluginMessagePipeOwner
	/* virtual */ void receiveMessageRaw(const std::string &message);

	// Inherited from LLPluginInstanceMessageListener
	/* virtual */ void receivePluginMessage(const std::string &message);
	
private:

	enum EState
	{
		STATE_UNINITIALIZED,
		STATE_INITIALIZED,			// init() has been called
		STATE_CONNECTED,			// connected back to launcher
		STATE_PLUGIN_LOADING,		// plugin library needs to be loaded
		STATE_PLUGIN_LOADED,		// plugin library has been loaded
		STATE_PLUGIN_INITIALIZING,	// plugin is processing init message
		STATE_RUNNING,				// steady state (processing messages)
		STATE_UNLOADING,			// plugin has sent shutdown_response and needs to be unloaded
		STATE_UNLOADED,				// plugin has been unloaded
		STATE_ERROR,				// generic bailout state
		STATE_DONE					// state machine will sit in this state after either error or normal termination.
	};
	void setState(EState state);

	EState mState;
	
	LLHost mLauncherHost;
	LLSocket::ptr_t mSocket;
	
	std::string mPluginFile;
	std::string mPluginDir;

	LLPluginInstance *mInstance;

	typedef std::map<std::string, LLPluginSharedMemory*> sharedMemoryRegionsType;
	sharedMemoryRegionsType mSharedMemoryRegions;
	
	LLTimer mHeartbeat;
	F64		mSleepTime;
	F64		mCPUElapsed;
	bool	mBlockingRequest;
	bool	mBlockingResponseReceived;
	std::queue<std::string> mMessageQueue;
	
	void deliverQueuedMessages();
	
};

#endif // LL_LLPLUGINPROCESSCHILD_H

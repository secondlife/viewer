/** 
 * @file llpluginprocesschild.h
 * @brief LLPluginProcessChild handles the child side of the external-process plugin API. 
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

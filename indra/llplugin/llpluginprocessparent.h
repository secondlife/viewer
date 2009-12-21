/** 
 * @file llpluginprocessparent.h
 * @brief LLPluginProcessParent handles the parent side of the external-process plugin API.
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

#ifndef LL_LLPLUGINPROCESSPARENT_H
#define LL_LLPLUGINPROCESSPARENT_H

#include "llapr.h"
#include "llprocesslauncher.h"
#include "llpluginmessage.h"
#include "llpluginmessagepipe.h"
#include "llpluginsharedmemory.h"

#include "lliosocket.h"

class LLPluginProcessParentOwner
{
public:
	virtual ~LLPluginProcessParentOwner();
	virtual void receivePluginMessage(const LLPluginMessage &message) = 0;
	// This will only be called when the plugin has died unexpectedly 
	virtual void pluginLaunchFailed() {};
	virtual void pluginDied() {};
};

class LLPluginProcessParent : public LLPluginMessagePipeOwner
{
	LOG_CLASS(LLPluginProcessParent);
public:
	LLPluginProcessParent(LLPluginProcessParentOwner *owner);
	~LLPluginProcessParent();
		
	void init(const std::string &launcher_filename, const std::string &plugin_filename, bool debug, const std::string &user_data_path);
	void idle(void);
	
	// returns true if the plugin is on its way to steady state
	bool isLoading(void);

	// returns true if the plugin is in the steady state (processing messages)
	bool isRunning(void);

	// returns true if the process has exited or we've had a fatal error
	bool isDone(void);	
	
	void killSockets(void);
	
	// Go to the proper error state
	void errorState(void);

	void setSleepTime(F64 sleep_time, bool force_send = false);
	F64 getSleepTime(void) const { return mSleepTime; };

	void sendMessage(const LLPluginMessage &message);
	
	void receiveMessage(const LLPluginMessage &message);
	
	// Inherited from LLPluginMessagePipeOwner
	void receiveMessageRaw(const std::string &message);
	
	// This adds a memory segment shared with the client, generating a name for the segment.  The name generated is guaranteed to be unique on the host.
	// The caller must call removeSharedMemory first (and wait until getSharedMemorySize returns 0 for the indicated name) before re-adding a segment with the same name.
	std::string addSharedMemory(size_t size);
	// Negotiates for the removal of a shared memory segment.  It is the caller's responsibility to ensure that nothing touches the memory
	// after this has been called, since the segment will be unmapped shortly thereafter.
	void removeSharedMemory(const std::string &name);
	size_t getSharedMemorySize(const std::string &name);
	void *getSharedMemoryAddress(const std::string &name);
	
	// Returns the version string the plugin indicated for the message class, or an empty string if that class wasn't in the list.
	std::string getMessageClassVersion(const std::string &message_class);

	std::string getPluginVersion(void);
	
	bool getDisableTimeout() { return mDisableTimeout; };
	void setDisableTimeout(bool disable) { mDisableTimeout = disable; };
	
	void setLaunchTimeout(F32 timeout) { mPluginLaunchTimeout = timeout; };
	void setLockupTimeout(F32 timeout) { mPluginLockupTimeout = timeout; };

	F64 getCPUUsage() { return mCPUUsage; };

private:

	enum EState
	{
		STATE_UNINITIALIZED,
		STATE_INITIALIZED,		// init() has been called
		STATE_LISTENING,		// listening for incoming connection
		STATE_LAUNCHED,			// process has been launched
		STATE_CONNECTED,		// process has connected
		STATE_HELLO,			// first message from the plugin process has been received
		STATE_LOADING,			// process has been asked to load the plugin
		STATE_RUNNING,			// 
		STATE_LAUNCH_FAILURE,	// Failure before plugin loaded
		STATE_ERROR,			// generic bailout state
		STATE_CLEANUP,			// clean everything up
		STATE_EXITING,			// Tried to kill process, waiting for it to exit
		STATE_DONE				//

	};
	EState mState;
	void setState(EState state);
	
	bool pluginLockedUp();
	bool pluginLockedUpOrQuit();

	bool accept();
		
	LLSocket::ptr_t mListenSocket;
	LLSocket::ptr_t mSocket;
	U32 mBoundPort;
	
	LLProcessLauncher mProcess;
	
	std::string mPluginFile;

	std::string mUserDataPath;

	LLPluginProcessParentOwner *mOwner;
	
	typedef std::map<std::string, LLPluginSharedMemory*> sharedMemoryRegionsType;
	sharedMemoryRegionsType mSharedMemoryRegions;
	
	LLSD mMessageClassVersions;
	std::string mPluginVersionString;
	
	LLTimer mHeartbeat;
	F64		mSleepTime;
	F64		mCPUUsage;
	
	bool mDisableTimeout;
	bool mDebug;

	LLProcessLauncher mDebugger;
	
	F32 mPluginLaunchTimeout;		// Somewhat longer timeout for initial launch.
	F32 mPluginLockupTimeout;		// If we don't receive a heartbeat in this many seconds, we declare the plugin locked up.

};

#endif // LL_LLPLUGINPROCESSPARENT_H

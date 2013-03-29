/** 
 * @file llpluginprocessparent.h
 * @brief LLPluginProcessParent handles the parent side of the external-process plugin API.
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

#ifndef LL_LLPLUGINPROCESSPARENT_H
#define LL_LLPLUGINPROCESSPARENT_H

#include "llapr.h"
#include "llprocess.h"
#include "llpluginmessage.h"
#include "llpluginmessagepipe.h"
#include "llpluginsharedmemory.h"

#include "lliosocket.h"
#include "llthread.h"
#include "llsd.h"

class LLPluginProcessParentOwner
{
public:
	virtual ~LLPluginProcessParentOwner();
	virtual void receivePluginMessage(const LLPluginMessage &message) = 0;
	virtual bool receivePluginMessageEarly(const LLPluginMessage &message) {return false;};
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
		
	void init(const std::string &launcher_filename, 
			  const std::string &plugin_dir,
			  const std::string &plugin_filename, 
			  bool debug);

	void idle(void);
	
	// returns true if the plugin is on its way to steady state
	bool isLoading(void);

	// returns true if the plugin is in the steady state (processing messages)
	bool isRunning(void);

	// returns true if the process has exited or we've had a fatal error
	bool isDone(void);	
	
	// returns true if the process is currently waiting on a blocking request
	bool isBlocked(void) { return mBlocked; };
	
	void killSockets(void);
	
	// Go to the proper error state
	void errorState(void);

	void setSleepTime(F64 sleep_time, bool force_send = false);
	F64 getSleepTime(void) const { return mSleepTime; };

	void sendMessage(const LLPluginMessage &message);
	
	void receiveMessage(const LLPluginMessage &message);
	
	// Inherited from LLPluginMessagePipeOwner
	/*virtual*/ void receiveMessageRaw(const std::string &message);
	/*virtual*/ void receiveMessageEarly(const LLPluginMessage &message);
	/*virtual*/ void setMessagePipe(LLPluginMessagePipe *message_pipe) ;
	
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
	
	static void poll(F64 timeout);
	static bool canPollThreadRun() { return (sPollSet || sPollsetNeedsRebuild || sUseReadThread); };
	static void setUseReadThread(bool use_read_thread);
	static bool getUseReadThread() { return sUseReadThread; };
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

	LLProcess::Params mProcessParams;
	LLProcessPtr mProcess;

	std::string mPluginFile;
	std::string mPluginDir;

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
	bool mBlocked;
	bool mPolledInput;

	LLProcessPtr mDebugger;
	
	F32 mPluginLaunchTimeout;		// Somewhat longer timeout for initial launch.
	F32 mPluginLockupTimeout;		// If we don't receive a heartbeat in this many seconds, we declare the plugin locked up.

	static bool sUseReadThread;
	apr_pollfd_t mPollFD;
	static apr_pollset_t *sPollSet;
	static bool sPollsetNeedsRebuild;
	static LLMutex *sInstancesMutex;
	static std::list<LLPluginProcessParent*> sInstances;
	static void dirtyPollSet();
	static void updatePollset();
	void servicePoll();
	static LLThread *sReadThread;
	
	LLMutex mIncomingQueueMutex;
	std::queue<LLPluginMessage> mIncomingQueue;
};

#endif // LL_LLPLUGINPROCESSPARENT_H

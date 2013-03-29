/** 
 * @file llpluginprocessparent.cpp
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

#include "linden_common.h"

#include "llpluginprocessparent.h"
#include "llpluginmessagepipe.h"
#include "llpluginmessageclasses.h"
#include "stringize.h"

#include "llapr.h"

//virtual 
LLPluginProcessParentOwner::~LLPluginProcessParentOwner()
{
	
}

bool LLPluginProcessParent::sUseReadThread = false;
apr_pollset_t *LLPluginProcessParent::sPollSet = NULL;
bool LLPluginProcessParent::sPollsetNeedsRebuild = false;
LLMutex *LLPluginProcessParent::sInstancesMutex;
std::list<LLPluginProcessParent*> LLPluginProcessParent::sInstances;
LLThread *LLPluginProcessParent::sReadThread = NULL;


class LLPluginProcessParentPollThread: public LLThread
{
public:
	LLPluginProcessParentPollThread() :
		LLThread("LLPluginProcessParentPollThread", gAPRPoolp)
	{
	}
protected:
	// Inherited from LLThread
	/*virtual*/ void run(void)
	{
		while(!isQuitting() && LLPluginProcessParent::getUseReadThread())
		{
			LLPluginProcessParent::poll(0.1f);
			checkPause();
		}
		
		// Final poll to clean up the pollset, etc.
		LLPluginProcessParent::poll(0.0f);
	} 

	// Inherited from LLThread
	/*virtual*/ bool runCondition(void)
	{
		return(LLPluginProcessParent::canPollThreadRun());
	}

};

LLPluginProcessParent::LLPluginProcessParent(LLPluginProcessParentOwner *owner):
	mIncomingQueueMutex(gAPRPoolp)
{
	if(!sInstancesMutex)
	{
		sInstancesMutex = new LLMutex(gAPRPoolp);
	}
	
	mOwner = owner;
	mBoundPort = 0;
	mState = STATE_UNINITIALIZED;
	mSleepTime = 0.0;
	mCPUUsage = 0.0;
	mDisableTimeout = false;
	mDebug = false;
	mBlocked = false;
	mPolledInput = false;
	mPollFD.client_data = NULL;

	mPluginLaunchTimeout = 60.0f;
	mPluginLockupTimeout = 15.0f;
	
	// Don't start the timer here -- start it when we actually launch the plugin process.
	mHeartbeat.stop();
	
	// Don't add to the global list until fully constructed.
	{
		LLMutexLock lock(sInstancesMutex);
		sInstances.push_back(this);
	}
}

LLPluginProcessParent::~LLPluginProcessParent()
{
	LL_DEBUGS("Plugin") << "destructor" << LL_ENDL;

	// Remove from the global list before beginning destruction.
	{
		// Make sure to get the global mutex _first_ here, to avoid a possible deadlock against LLPluginProcessParent::poll()
		LLMutexLock lock(sInstancesMutex);
		{
			LLMutexLock lock2(&mIncomingQueueMutex);
			sInstances.remove(this);
		}
	}

	// Destroy any remaining shared memory regions
	sharedMemoryRegionsType::iterator iter;
	while((iter = mSharedMemoryRegions.begin()) != mSharedMemoryRegions.end())
	{
		// destroy the shared memory region
		iter->second->destroy();
		
		// and remove it from our map
		mSharedMemoryRegions.erase(iter);
	}

	LLProcess::kill(mProcess);
	killSockets();
}

void LLPluginProcessParent::killSockets(void)
{
	{
		LLMutexLock lock(&mIncomingQueueMutex);
		killMessagePipe();
	}

	mListenSocket.reset();
	mSocket.reset();
}

void LLPluginProcessParent::errorState(void)
{
	if(mState < STATE_RUNNING)
		setState(STATE_LAUNCH_FAILURE);
	else
		setState(STATE_ERROR);
}

void LLPluginProcessParent::init(const std::string &launcher_filename, const std::string &plugin_dir, const std::string &plugin_filename, bool debug)
{	
	mProcessParams.executable = launcher_filename;
	mProcessParams.cwd = plugin_dir;
	mPluginFile = plugin_filename;
	mPluginDir = plugin_dir;
	mCPUUsage = 0.0f;
	mDebug = debug;	
	setState(STATE_INITIALIZED);
}

bool LLPluginProcessParent::accept()
{
	bool result = false;
	
	apr_status_t status = APR_EGENERAL;
	apr_socket_t *new_socket = NULL;
	
	status = apr_socket_accept(
		&new_socket,
		mListenSocket->getSocket(),
		gAPRPoolp);

	
	if(status == APR_SUCCESS)
	{
//		llinfos << "SUCCESS" << llendl;
		// Success.  Create a message pipe on the new socket

		// we MUST create a new pool for the LLSocket, since it will take ownership of it and delete it in its destructor!
		apr_pool_t* new_pool = NULL;
		status = apr_pool_create(&new_pool, gAPRPoolp);

		mSocket = LLSocket::create(new_socket, new_pool);
		new LLPluginMessagePipe(this, mSocket);

		result = true;
	}
	else if(APR_STATUS_IS_EAGAIN(status))
	{
//		llinfos << "EAGAIN" << llendl;

		// No incoming connections.  This is not an error.
		status = APR_SUCCESS;
	}
	else
	{
//		llinfos << "Error:" << llendl;
		ll_apr_warn_status(status);
		
		// Some other error.
		errorState();
	}
	
	return result;	
}

void LLPluginProcessParent::idle(void)
{
	bool idle_again;

	do
	{
		// process queued messages
		mIncomingQueueMutex.lock();
		while(!mIncomingQueue.empty())
		{
			LLPluginMessage message = mIncomingQueue.front();
			mIncomingQueue.pop();
			mIncomingQueueMutex.unlock();
				
			receiveMessage(message);
			
			mIncomingQueueMutex.lock();
		}

		mIncomingQueueMutex.unlock();
		
		// Give time to network processing
		if(mMessagePipe)
		{
			// Drain any queued outgoing messages
			mMessagePipe->pumpOutput();
			
			// Only do input processing here if this instance isn't in a pollset.
			if(!mPolledInput)
			{
				mMessagePipe->pumpInput();
			}
		}
		
		if(mState <= STATE_RUNNING)
		{
			if(APR_STATUS_IS_EOF(mSocketError))
			{
				// Plugin socket was closed.  This covers both normal plugin termination and plugin crashes.
				errorState();
			}
			else if(mSocketError != APR_SUCCESS)
			{
				// The socket is in an error state -- the plugin is gone.
				LL_WARNS("Plugin") << "Socket hit an error state (" << mSocketError << ")" << LL_ENDL;
				errorState();
			}
		}	
		
		// If a state needs to go directly to another state (as a performance enhancement), it can set idle_again to true after calling setState().
		// USE THIS CAREFULLY, since it can starve other code.  Specifically make sure there's no way to get into a closed cycle and never return.
		// When in doubt, don't do it.
		idle_again = false;
		switch(mState)
		{
			case STATE_UNINITIALIZED:
			break;

			case STATE_INITIALIZED:
			{
	
				apr_status_t status = APR_SUCCESS;
				apr_sockaddr_t* addr = NULL;
				mListenSocket = LLSocket::create(gAPRPoolp, LLSocket::STREAM_TCP);
				mBoundPort = 0;
				
				// This code is based on parts of LLSocket::create() in lliosocket.cpp.
				
				status = apr_sockaddr_info_get(
					&addr,
					"127.0.0.1",
					APR_INET,
					0,	// port 0 = ephemeral ("find me a port")
					0,
					gAPRPoolp);
					
				if(ll_apr_warn_status(status))
				{
					killSockets();
					errorState();
					break;
				}

				// This allows us to reuse the address on quick down/up. This is unlikely to create problems.
				ll_apr_warn_status(apr_socket_opt_set(mListenSocket->getSocket(), APR_SO_REUSEADDR, 1));
				
				status = apr_socket_bind(mListenSocket->getSocket(), addr);
				if(ll_apr_warn_status(status))
				{
					killSockets();
					errorState();
					break;
				}

				// Get the actual port the socket was bound to
				{
					apr_sockaddr_t* bound_addr = NULL;
					if(ll_apr_warn_status(apr_socket_addr_get(&bound_addr, APR_LOCAL, mListenSocket->getSocket())))
					{
						killSockets();
						errorState();
						break;
					}
					mBoundPort = bound_addr->port;	

					if(mBoundPort == 0)
					{
						LL_WARNS("Plugin") << "Bound port number unknown, bailing out." << LL_ENDL;
						
						killSockets();
						errorState();
						break;
					}
				}
				
				LL_DEBUGS("Plugin") << "Bound tcp socket to port: " << addr->port << LL_ENDL;

				// Make the listen socket non-blocking
				status = apr_socket_opt_set(mListenSocket->getSocket(), APR_SO_NONBLOCK, 1);
				if(ll_apr_warn_status(status))
				{
					killSockets();
					errorState();
					break;
				}

				apr_socket_timeout_set(mListenSocket->getSocket(), 0);
				if(ll_apr_warn_status(status))
				{
					killSockets();
					errorState();
					break;
				}
				
				// If it's a stream based socket, we need to tell the OS
				// to keep a queue of incoming connections for ACCEPT.
				status = apr_socket_listen(
					mListenSocket->getSocket(),
					10); // FIXME: Magic number for queue size
					
				if(ll_apr_warn_status(status))
				{
					killSockets();
					errorState();
					break;
				}
				
				// If we got here, we're listening.
				setState(STATE_LISTENING);
			}
			break;
			
			case STATE_LISTENING:
			{
				// Launch the plugin process.
				
				// Only argument to the launcher is the port number we're listening on
				mProcessParams.args.add(stringize(mBoundPort));
				if (! (mProcess = LLProcess::create(mProcessParams)))
				{
					errorState();
				}
				else
				{
					if(mDebug)
					{
						#if LL_DARWIN
						// If we're set to debug, start up a gdb instance in a new terminal window and have it attach to the plugin process and continue.
						
						// The command we're constructing would look like this on the command line:
						// osascript -e 'tell application "Terminal"' -e 'set win to do script "gdb -pid 12345"' -e 'do script "continue" in win' -e 'end tell'

						LLProcess::Params params;
						params.executable = "/usr/bin/osascript";
						params.args.add("-e");
						params.args.add("tell application \"Terminal\"");
						params.args.add("-e");
						params.args.add(STRINGIZE("set win to do script \"gdb -pid "
												  << mProcess->getProcessID() << "\""));
						params.args.add("-e");
						params.args.add("do script \"continue\" in win");
						params.args.add("-e");
						params.args.add("end tell");
						mDebugger = LLProcess::create(params);

						#endif
					}
					
					// This will allow us to time out if the process never starts.
					mHeartbeat.start();
					mHeartbeat.setTimerExpirySec(mPluginLaunchTimeout);
					setState(STATE_LAUNCHED);
				}
			}
			break;

			case STATE_LAUNCHED:
				// waiting for the plugin to connect
				if(pluginLockedUpOrQuit())
				{
					errorState();
				}
				else
				{
					// Check for the incoming connection.
					if(accept())
					{
						// Stop listening on the server port
						mListenSocket.reset();
						setState(STATE_CONNECTED);
					}
				}
			break;
			
			case STATE_CONNECTED:
				// waiting for hello message from the plugin

				if(pluginLockedUpOrQuit())
				{
					errorState();
				}
			break;

			case STATE_HELLO:
				LL_DEBUGS("Plugin") << "received hello message" << LL_ENDL;
				
				// Send the message to load the plugin
				{
					LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "load_plugin");
					message.setValue("file", mPluginFile);
					message.setValue("dir", mPluginDir);
					sendMessage(message);
				}

				setState(STATE_LOADING);
			break;
			
			case STATE_LOADING:
				// The load_plugin_response message will kick us from here into STATE_RUNNING
				if(pluginLockedUpOrQuit())
				{
					errorState();
				}
			break;
			
			case STATE_RUNNING:
				if(pluginLockedUpOrQuit())
				{
					errorState();
				}
			break;
			
			case STATE_EXITING:
				if (! LLProcess::isRunning(mProcess))
				{
					setState(STATE_CLEANUP);
				}
				else if(pluginLockedUp())
				{
					LL_WARNS("Plugin") << "timeout in exiting state, bailing out" << LL_ENDL;
					errorState();
				}
			break;

			case STATE_LAUNCH_FAILURE:
				if(mOwner != NULL)
				{
					mOwner->pluginLaunchFailed();
				}
				setState(STATE_CLEANUP);
			break;

			case STATE_ERROR:
				if(mOwner != NULL)
				{
					mOwner->pluginDied();
				}
				setState(STATE_CLEANUP);
			break;
			
			case STATE_CLEANUP:
				LLProcess::kill(mProcess);
				killSockets();
				setState(STATE_DONE);
			break;
			
			
			case STATE_DONE:
				// just sit here.
			break;
			
		}
	
	} while (idle_again);
}

bool LLPluginProcessParent::isLoading(void)
{
	bool result = false;
	
	if(mState <= STATE_LOADING)
		result = true;
		
	return result;
}

bool LLPluginProcessParent::isRunning(void)
{
	bool result = false;
	
	if(mState == STATE_RUNNING)
		result = true;
		
	return result;
}

bool LLPluginProcessParent::isDone(void)
{
	bool result = false;
	
	if(mState == STATE_DONE)
		result = true;
		
	return result;
}

void LLPluginProcessParent::setSleepTime(F64 sleep_time, bool force_send)
{
	if(force_send || (sleep_time != mSleepTime))
	{
		// Cache the time locally
		mSleepTime = sleep_time;
		
		if(canSendMessage())
		{
			// and send to the plugin.
			LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "sleep_time");
			message.setValueReal("time", mSleepTime);
			sendMessage(message);
		}
		else
		{
			// Too early to send -- the load_plugin_response message will trigger us to send mSleepTime later.
		}
	}
}

void LLPluginProcessParent::sendMessage(const LLPluginMessage &message)
{
	if(message.hasValue("blocking_response"))
	{
		mBlocked = false;

		// reset the heartbeat timer, since there will have been no heartbeats while the plugin was blocked.
		mHeartbeat.setTimerExpirySec(mPluginLockupTimeout);
	}
	
	std::string buffer = message.generate();
	LL_DEBUGS("Plugin") << "Sending: " << buffer << LL_ENDL;	
	writeMessageRaw(buffer);
	
	// Try to send message immediately.
	if(mMessagePipe)
	{
		mMessagePipe->pumpOutput();
	}
}

//virtual 
void LLPluginProcessParent::setMessagePipe(LLPluginMessagePipe *message_pipe)
{
	bool update_pollset = false;
	
	if(mMessagePipe)
	{
		// Unsetting an existing message pipe -- remove from the pollset		
		mPollFD.client_data = NULL;

		// pollset needs an update
		update_pollset = true;
	}
	if(message_pipe != NULL)
	{
		// Set up the apr_pollfd_t
		mPollFD.p = gAPRPoolp;
		mPollFD.desc_type = APR_POLL_SOCKET;
		mPollFD.reqevents = APR_POLLIN|APR_POLLERR|APR_POLLHUP;
		mPollFD.rtnevents = 0;
		mPollFD.desc.s = mSocket->getSocket();
		mPollFD.client_data = (void*)this;	
		
		// pollset needs an update
		update_pollset = true;
	}

	mMessagePipe = message_pipe;
	
	if(update_pollset)
	{
		dirtyPollSet();
	}
}

//static 
void LLPluginProcessParent::dirtyPollSet()
{
	sPollsetNeedsRebuild = true;
	
	if(sReadThread)
	{
		LL_DEBUGS("PluginPoll") << "unpausing read thread " << LL_ENDL;
		sReadThread->unpause();
	}
}

void LLPluginProcessParent::updatePollset()
{
	if(!sInstancesMutex)
	{
		// No instances have been created yet.  There's no work to do.
		return;
	}
		
	LLMutexLock lock(sInstancesMutex);

	if(sPollSet)
	{
		LL_DEBUGS("PluginPoll") << "destroying pollset " << sPollSet << LL_ENDL;
		// delete the existing pollset.
		apr_pollset_destroy(sPollSet);
		sPollSet = NULL;
	}
	
	std::list<LLPluginProcessParent*>::iterator iter;
	int count = 0;
	
	// Count the number of instances that want to be in the pollset
	for(iter = sInstances.begin(); iter != sInstances.end(); iter++)
	{
		(*iter)->mPolledInput = false;
		if((*iter)->mPollFD.client_data)
		{
			// This instance has a socket that needs to be polled.
			++count;
		}
	}

	if(sUseReadThread && sReadThread && !sReadThread->isQuitting())
	{
		if(!sPollSet && (count > 0))
		{
#ifdef APR_POLLSET_NOCOPY
			// The pollset doesn't exist yet.  Create it now.
			apr_status_t status = apr_pollset_create(&sPollSet, count, gAPRPoolp, APR_POLLSET_NOCOPY);
			if(status != APR_SUCCESS)
			{
#endif // APR_POLLSET_NOCOPY
				LL_WARNS("PluginPoll") << "Couldn't create pollset.  Falling back to non-pollset mode." << LL_ENDL;
				sPollSet = NULL;
#ifdef APR_POLLSET_NOCOPY
			}
			else
			{
				LL_DEBUGS("PluginPoll") << "created pollset " << sPollSet << LL_ENDL;
				
				// Pollset was created, add all instances to it.
				for(iter = sInstances.begin(); iter != sInstances.end(); iter++)
				{
					if((*iter)->mPollFD.client_data)
					{
						status = apr_pollset_add(sPollSet, &((*iter)->mPollFD));
						if(status == APR_SUCCESS)
						{
							(*iter)->mPolledInput = true;
						}
						else
						{
							LL_WARNS("PluginPoll") << "apr_pollset_add failed with status " << status << LL_ENDL;
						}
					}
				}
			}
#endif // APR_POLLSET_NOCOPY
		}
	}
}

void LLPluginProcessParent::setUseReadThread(bool use_read_thread)
{
	if(sUseReadThread != use_read_thread)
	{
		sUseReadThread = use_read_thread;
		
		if(sUseReadThread)
		{
			if(!sReadThread)
			{
				// start up the read thread
				LL_INFOS("PluginPoll") << "creating read thread " << LL_ENDL;

				// make sure the pollset gets rebuilt.
				sPollsetNeedsRebuild = true;
				
				sReadThread = new LLPluginProcessParentPollThread;
				sReadThread->start();
			}
		}
		else
		{
			if(sReadThread)
			{
				// shut down the read thread
				LL_INFOS("PluginPoll") << "destroying read thread " << LL_ENDL;
				delete sReadThread;
				sReadThread = NULL;
			}
		}

	}
}

void LLPluginProcessParent::poll(F64 timeout)
{
	if(sPollsetNeedsRebuild || !sUseReadThread)
	{
		sPollsetNeedsRebuild = false;
		updatePollset();
	}
	
	if(sPollSet)
	{
		apr_status_t status;
		apr_int32_t count;
		const apr_pollfd_t *descriptors;
		status = apr_pollset_poll(sPollSet, (apr_interval_time_t)(timeout * 1000000), &count, &descriptors);
		if(status == APR_SUCCESS)
		{
			// One or more of the descriptors signalled.  Call them.
			for(int i = 0; i < count; i++)
			{
				LLPluginProcessParent *self = (LLPluginProcessParent *)(descriptors[i].client_data);
				// NOTE: the descriptor returned here is actually a COPY of the original (even though we create the pollset with APR_POLLSET_NOCOPY).
				// This means that even if the parent has set its mPollFD.client_data to NULL, the old pointer may still there in this descriptor.
				// It's even possible that the old pointer no longer points to a valid LLPluginProcessParent.
				// This means that we can't safely dereference the 'self' pointer here without some extra steps...
				if(self)
				{
					// Make sure this pointer is still in the instances list
					bool valid = false;
					{
						LLMutexLock lock(sInstancesMutex);
						for(std::list<LLPluginProcessParent*>::iterator iter = sInstances.begin(); iter != sInstances.end(); ++iter)
						{
							if(*iter == self)
							{
								// Lock the instance's mutex before unlocking the global mutex.  
								// This avoids a possible race condition where the instance gets deleted between this check and the servicePoll() call.
								self->mIncomingQueueMutex.lock();
								valid = true;
								break;
							}
						}
					}
					
					if(valid)
					{
						// The instance is still valid.
						// Pull incoming messages off the socket
						self->servicePoll();
						self->mIncomingQueueMutex.unlock();
					}
					else
					{
						LL_DEBUGS("PluginPoll") << "detected deleted instance " << self << LL_ENDL;
					}

				}
			}
		}
		else if(APR_STATUS_IS_TIMEUP(status))
		{
			// timed out with no incoming data.  Just return.
		}
		else if(status == EBADF)
		{
			// This happens when one of the file descriptors in the pollset is destroyed, which happens whenever a plugin's socket is closed.
			// The pollset has been or will be recreated, so just return.
			LL_DEBUGS("PluginPoll") << "apr_pollset_poll returned EBADF" << LL_ENDL;
		}
		else if(status != APR_SUCCESS)
		{
			LL_WARNS("PluginPoll") << "apr_pollset_poll failed with status " << status << LL_ENDL;
		}
	}
}

void LLPluginProcessParent::servicePoll()
{
	bool result = true;
	
	// poll signalled on this object's socket.  Try to process incoming messages.
	if(mMessagePipe)
	{
		result = mMessagePipe->pumpInput(0.0f);
	}

	if(!result)
	{
		// If we got a read error on input, remove this pipe from the pollset
		apr_pollset_remove(sPollSet, &mPollFD);

		// and tell the code not to re-add it
		mPollFD.client_data = NULL;
	}
}

void LLPluginProcessParent::receiveMessageRaw(const std::string &message)
{
	LL_DEBUGS("Plugin") << "Received: " << message << LL_ENDL;
	
	LLPluginMessage parsed;
	if(parsed.parse(message) != -1)
	{
		if(parsed.hasValue("blocking_request"))
		{
			mBlocked = true;
		}

		if(mPolledInput)
		{
			// This is being called on the polling thread -- only do minimal processing/queueing.
			receiveMessageEarly(parsed);
		}
		else
		{
			// This is not being called on the polling thread -- do full message processing at this time.
			receiveMessage(parsed);
		}
	}
}

void LLPluginProcessParent::receiveMessageEarly(const LLPluginMessage &message)
{
	// NOTE: this function will be called from the polling thread.  It will be called with mIncomingQueueMutex _already locked_. 

	bool handled = false;
	
	std::string message_class = message.getClass();
	if(message_class == LLPLUGIN_MESSAGE_CLASS_INTERNAL)
	{
		// no internal messages need to be handled early.
	}
	else
	{
		// Call out to the owner and see if they to reply
		// TODO: Should this only happen when blocked?
		if(mOwner != NULL)
		{
			handled = mOwner->receivePluginMessageEarly(message);
		}
	}
	
	if(!handled)
	{
		// any message that wasn't handled early needs to be queued.
		mIncomingQueue.push(message);
	}
}

void LLPluginProcessParent::receiveMessage(const LLPluginMessage &message)
{
	std::string message_class = message.getClass();
	if(message_class == LLPLUGIN_MESSAGE_CLASS_INTERNAL)
	{
		// internal messages should be handled here
		std::string message_name = message.getName();
		if(message_name == "hello")
		{
			if(mState == STATE_CONNECTED)
			{
				// Plugin host has launched.  Tell it which plugin to load.
				setState(STATE_HELLO);
			}
			else
			{
				LL_WARNS("Plugin") << "received hello message in wrong state -- bailing out" << LL_ENDL;
				errorState();
			}
			
		}
		else if(message_name == "load_plugin_response")
		{
			if(mState == STATE_LOADING)
			{
				// Plugin has been loaded. 
				
				mPluginVersionString = message.getValue("plugin_version");
				LL_INFOS("Plugin") << "plugin version string: " << mPluginVersionString << LL_ENDL;

				// Check which message classes/versions the plugin supports.
				// TODO: check against current versions
				// TODO: kill plugin on major mismatches?
				mMessageClassVersions = message.getValueLLSD("versions");
				LLSD::map_iterator iter;
				for(iter = mMessageClassVersions.beginMap(); iter != mMessageClassVersions.endMap(); iter++)
				{
					LL_INFOS("Plugin") << "message class: " << iter->first << " -> version: " << iter->second.asString() << LL_ENDL;
				}
				
				// Send initial sleep time
				llassert_always(mSleepTime != 0.f);
				setSleepTime(mSleepTime, true);			

				setState(STATE_RUNNING);
			}
			else
			{
				LL_WARNS("Plugin") << "received load_plugin_response message in wrong state -- bailing out" << LL_ENDL;
				errorState();
			}
		}
		else if(message_name == "heartbeat")
		{
			// this resets our timer.
			mHeartbeat.setTimerExpirySec(mPluginLockupTimeout);

			mCPUUsage = message.getValueReal("cpu_usage");

			LL_DEBUGS("Plugin") << "cpu usage reported as " << mCPUUsage << LL_ENDL;
			
		}
		else if(message_name == "shm_add_response")
		{
			// Nothing to do here.
		}
		else if(message_name == "shm_remove_response")
		{
			std::string name = message.getValue("name");
			sharedMemoryRegionsType::iterator iter = mSharedMemoryRegions.find(name);
			
			if(iter != mSharedMemoryRegions.end())
			{
				// destroy the shared memory region
				iter->second->destroy();
				
				// and remove it from our map
				mSharedMemoryRegions.erase(iter);
			}
		}
		else
		{
			LL_WARNS("Plugin") << "Unknown internal message from child: " << message_name << LL_ENDL;
		}
	}
	else
	{
		if(mOwner != NULL)
		{
			mOwner->receivePluginMessage(message);
		}
	}
}

std::string LLPluginProcessParent::addSharedMemory(size_t size)
{
	std::string name;
	
	LLPluginSharedMemory *region = new LLPluginSharedMemory;

	// This is a new region
	if(region->create(size))
	{
		name = region->getName();
		
		mSharedMemoryRegions.insert(sharedMemoryRegionsType::value_type(name, region));
		
		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "shm_add");
		message.setValue("name", name);
		message.setValueS32("size", (S32)size);
		sendMessage(message);
	}
	else
	{
		LL_WARNS("Plugin") << "Couldn't create a shared memory segment!" << LL_ENDL;

		// Don't leak
		delete region;
	}

	return name;
}

void LLPluginProcessParent::removeSharedMemory(const std::string &name)
{
	sharedMemoryRegionsType::iterator iter = mSharedMemoryRegions.find(name);
	
	if(iter != mSharedMemoryRegions.end())
	{
		// This segment exists.  Send the message to the child to unmap it.  The response will cause the parent to unmap our end.
		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "shm_remove");
		message.setValue("name", name);
		sendMessage(message);
	}
	else
	{
		LL_WARNS("Plugin") << "Request to remove an unknown shared memory segment." << LL_ENDL;
	}
}
size_t LLPluginProcessParent::getSharedMemorySize(const std::string &name)
{
	size_t result = 0;
	
	sharedMemoryRegionsType::iterator iter = mSharedMemoryRegions.find(name);
	if(iter != mSharedMemoryRegions.end())
	{
		result = iter->second->getSize();
	}
	
	return result;
}
void *LLPluginProcessParent::getSharedMemoryAddress(const std::string &name)
{
	void *result = NULL;
	
	sharedMemoryRegionsType::iterator iter = mSharedMemoryRegions.find(name);
	if(iter != mSharedMemoryRegions.end())
	{
		result = iter->second->getMappedAddress();
	}
	
	return result;
}

std::string LLPluginProcessParent::getMessageClassVersion(const std::string &message_class)
{
	std::string result;
	
	if(mMessageClassVersions.has(message_class))
	{
		result = mMessageClassVersions[message_class].asString();
	}
	
	return result;
}

std::string LLPluginProcessParent::getPluginVersion(void)
{
	return mPluginVersionString;
}

void LLPluginProcessParent::setState(EState state)
{
	LL_DEBUGS("Plugin") << "setting state to " << state << LL_ENDL;
	mState = state; 
};

bool LLPluginProcessParent::pluginLockedUpOrQuit()
{
	bool result = false;
	
	if (! LLProcess::isRunning(mProcess))
	{
		LL_WARNS("Plugin") << "child exited" << LL_ENDL;
		result = true;
	}
	else if(pluginLockedUp())
	{
		LL_WARNS("Plugin") << "timeout" << LL_ENDL;
		result = true;
	}
	
	return result;
}

bool LLPluginProcessParent::pluginLockedUp()
{
	if(mDisableTimeout || mDebug || mBlocked)
	{
		// Never time out a plugin process in these cases.
		return false;
	}
	
	// If the timer is running and has expired, the plugin has locked up.
	return (mHeartbeat.getStarted() && mHeartbeat.hasExpired());
}


/** 
 * @file llpluginprocessparent.cpp
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

#include "linden_common.h"

#include "llpluginprocessparent.h"
#include "llpluginmessagepipe.h"
#include "llpluginmessageclasses.h"

#include "llapr.h"

//virtual 
LLPluginProcessParentOwner::~LLPluginProcessParentOwner()
{
	
}

LLPluginProcessParent::LLPluginProcessParent(LLPluginProcessParentOwner *owner)
{
	mOwner = owner;
	mBoundPort = 0;
	mState = STATE_UNINITIALIZED;
	mSleepTime = 0.0;
	mCPUUsage = 0.0;
	mDisableTimeout = false;
	mDebug = false;

	mPluginLaunchTimeout = 60.0f;
	mPluginLockupTimeout = 15.0f;
	
	// Don't start the timer here -- start it when we actually launch the plugin process.
	mHeartbeat.stop();
}

LLPluginProcessParent::~LLPluginProcessParent()
{
	LL_DEBUGS("Plugin") << "destructor" << LL_ENDL;

	// Destroy any remaining shared memory regions
	sharedMemoryRegionsType::iterator iter;
	while((iter = mSharedMemoryRegions.begin()) != mSharedMemoryRegions.end())
	{
		// destroy the shared memory region
		iter->second->destroy();
		
		// and remove it from our map
		mSharedMemoryRegions.erase(iter);
	}
	
	// orphaning the process means it won't be killed when the LLProcessLauncher is destructed.
	// This is what we want -- it should exit cleanly once it notices the sockets have been closed.
	mProcess.orphan();
	killSockets();
}

void LLPluginProcessParent::killSockets(void)
{
	killMessagePipe();
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

void LLPluginProcessParent::init(const std::string &launcher_filename, const std::string &plugin_filename, bool debug)
{	
	mProcess.setExecutable(launcher_filename);
	mPluginFile = plugin_filename;
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
		// Give time to network processing
		if(mMessagePipe)
		{
			if(!mMessagePipe->pump())
			{
//				LL_WARNS("Plugin") << "Message pipe hit an error state" << LL_ENDL;
				errorState();
			}
		}

		if((mSocketError != APR_SUCCESS) && (mState <= STATE_RUNNING))
		{
			// The socket is in an error state -- the plugin is gone.
			LL_WARNS("Plugin") << "Socket hit an error state (" << mSocketError << ")" << LL_ENDL;
			errorState();
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
				std::stringstream stream;
				stream << mBoundPort;
				mProcess.addArgument(stream.str());
				if(mProcess.launch() != 0)
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

						std::stringstream cmd;
						
						mDebugger.setExecutable("/usr/bin/osascript");
						mDebugger.addArgument("-e");
						mDebugger.addArgument("tell application \"Terminal\"");
						mDebugger.addArgument("-e");
						cmd << "set win to do script \"gdb -pid " << mProcess.getProcessID() << "\"";
						mDebugger.addArgument(cmd.str());
						mDebugger.addArgument("-e");
						mDebugger.addArgument("do script \"continue\" in win");
						mDebugger.addArgument("-e");
						mDebugger.addArgument("end tell");
						mDebugger.launch();

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
				LL_DEBUGS("Plugin") << "received hello message" << llendl;
				
				// Send the message to load the plugin
				{
					LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "load_plugin");
					message.setValue("file", mPluginFile);
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
				if(!mProcess.isRunning())
				{
					setState(STATE_CLEANUP);
				}
				else if(pluginLockedUp())
				{
					LL_WARNS("Plugin") << "timeout in exiting state, bailing out" << llendl;
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
				// Don't do a kill here anymore -- closing the sockets is the new 'kill'.
				mProcess.orphan();
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
	
	std::string buffer = message.generate();
	LL_DEBUGS("Plugin") << "Sending: " << buffer << LL_ENDL;	
	writeMessageRaw(buffer);
}


void LLPluginProcessParent::receiveMessageRaw(const std::string &message)
{
	LL_DEBUGS("Plugin") << "Received: " << message << LL_ENDL;

	// FIXME: should this go into a queue instead?
	
	LLPluginMessage parsed;
	if(parsed.parse(message) != -1)
	{
		receiveMessage(parsed);
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
	
	if(!mDisableTimeout && !mDebug)
	{
		if(!mProcess.isRunning())
		{
			LL_WARNS("Plugin") << "child exited" << llendl;
			result = true;
		}
		else if(pluginLockedUp())
		{
			LL_WARNS("Plugin") << "timeout" << llendl;
			result = true;
		}
	}
	
	return result;
}

bool LLPluginProcessParent::pluginLockedUp()
{
	// If the timer is running and has expired, the plugin has locked up.
	return (mHeartbeat.getStarted() && mHeartbeat.hasExpired());
}


/** 
 * @file llpluginprocesschild.cpp
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

#include "linden_common.h"

#include "llpluginprocesschild.h"
#include "llplugininstance.h"
#include "llpluginmessagepipe.h"
#include "llpluginmessageclasses.h"

static const F32 HEARTBEAT_SECONDS = 1.0f;
static const F32 PLUGIN_IDLE_SECONDS = 1.0f / 100.0f;  // Each call to idle will give the plugin this much time.

LLPluginProcessChild::LLPluginProcessChild()
{
	mInstance = NULL;
	mSocket = LLSocket::create(gAPRPoolp, LLSocket::STREAM_TCP);
	mSleepTime = PLUGIN_IDLE_SECONDS;	// default: send idle messages at 100Hz
	mCPUElapsed = 0.0f;
}

LLPluginProcessChild::~LLPluginProcessChild()
{
	if(mInstance != NULL)
	{
		sendMessageToPlugin(LLPluginMessage("base", "cleanup"));

		// IMPORTANT: under some (unknown) circumstances the apr_dso_unload() triggered when mInstance is deleted 
		// appears to fail and lock up which means that a given instance of the slplugin process never exits. 
		// This is bad, especially when users try to update their version of SL - it fails because the slplugin 
		// process as well as a bunch of plugin specific files are locked and cannot be overwritten.
		exit( 0 );
		//delete mInstance;
		//mInstance = NULL;
	}
}

void LLPluginProcessChild::killSockets(void)
{
	killMessagePipe();
	mSocket.reset();
}

void LLPluginProcessChild::init(U32 launcher_port)
{
	mLauncherHost = LLHost("127.0.0.1", launcher_port);
	setState(STATE_INITIALIZED);
}

void LLPluginProcessChild::idle(void)
{
	bool idle_again;
	do
	{
		if(mSocketError != APR_SUCCESS)
		{
			LL_INFOS("Plugin") << "message pipe is in error state, moving to STATE_ERROR"<< LL_ENDL;
			setState(STATE_ERROR);
		}	

		if((mState > STATE_INITIALIZED) && (mMessagePipe == NULL))
		{
			// The pipe has been closed -- we're done.
			// TODO: This could be slightly more subtle, but I'm not sure it needs to be.
			LL_INFOS("Plugin") << "message pipe went away, moving to STATE_ERROR"<< LL_ENDL;
			setState(STATE_ERROR);
		}
	
		// If a state needs to go directly to another state (as a performance enhancement), it can set idle_again to true after calling setState().
		// USE THIS CAREFULLY, since it can starve other code.  Specifically make sure there's no way to get into a closed cycle and never return.
		// When in doubt, don't do it.
		idle_again = false;
		
		if(mInstance != NULL)
		{
			// Provide some time to the plugin
			mInstance->idle();
		}
		
		switch(mState)
		{
			case STATE_UNINITIALIZED:
			break;

			case STATE_INITIALIZED:
				if(mSocket->blockingConnect(mLauncherHost))
				{
					// This automatically sets mMessagePipe
					new LLPluginMessagePipe(this, mSocket);
					
					setState(STATE_CONNECTED);
				}
				else
				{
					// connect failed
					setState(STATE_ERROR);
				}
			break;
			
			case STATE_CONNECTED:
				sendMessageToParent(LLPluginMessage(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "hello"));
				setState(STATE_PLUGIN_LOADING);
			break;
						
			case STATE_PLUGIN_LOADING:
				if(!mPluginFile.empty())
				{
					mInstance = new LLPluginInstance(this);
					if(mInstance->load(mPluginFile) == 0)
					{
						mHeartbeat.start();
						mHeartbeat.setTimerExpirySec(HEARTBEAT_SECONDS);
						mCPUElapsed = 0.0f;
						setState(STATE_PLUGIN_LOADED);
					}
					else
					{
						setState(STATE_ERROR);
					}
				}
			break;
			
			case STATE_PLUGIN_LOADED:
				{
					setState(STATE_PLUGIN_INITIALIZING);
					LLPluginMessage message("base", "init");
					message.setValue("user_data_path", mUserDataPath);
					sendMessageToPlugin(message);
				}
			break;
			
			case STATE_PLUGIN_INITIALIZING:
				// waiting for init_response...
			break;
			
			case STATE_RUNNING:
				if(mInstance != NULL)
				{
					// Provide some time to the plugin
					LLPluginMessage message("base", "idle");
					message.setValueReal("time", PLUGIN_IDLE_SECONDS);
					sendMessageToPlugin(message);
					
					mInstance->idle();
					
					if(mHeartbeat.hasExpired())
					{
						
						// This just proves that we're not stuck down inside the plugin code.
						LLPluginMessage heartbeat(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "heartbeat");
						
						// Calculate the approximage CPU usage fraction (floating point value between 0 and 1) used by the plugin this heartbeat cycle.
						// Note that this will not take into account any threads or additional processes the plugin spawns, but it's a first approximation.
						// If we could write OS-specific functions to query the actual CPU usage of this process, that would be a better approximation.
						heartbeat.setValueReal("cpu_usage", mCPUElapsed / mHeartbeat.getElapsedTimeF64());
						
						sendMessageToParent(heartbeat);

						mHeartbeat.reset();
						mHeartbeat.setTimerExpirySec(HEARTBEAT_SECONDS);
						mCPUElapsed = 0.0f;
					}
				}
				// receivePluginMessage will transition to STATE_UNLOADING
			break;

			case STATE_UNLOADING:
				if(mInstance != NULL)
				{
					sendMessageToPlugin(LLPluginMessage("base", "cleanup"));
					delete mInstance;
					mInstance = NULL;
				}
				setState(STATE_UNLOADED);
			break;
			
			case STATE_UNLOADED:
				killSockets();
				setState(STATE_DONE);
			break;

			case STATE_ERROR:
				// Close the socket to the launcher
				killSockets();				
				// TODO: Where do we go from here?  Just exit()?
				setState(STATE_DONE);
			break;
			
			case STATE_DONE:
				// just sit here.
			break;
		}
	
	} while (idle_again);
}

void LLPluginProcessChild::sleep(F64 seconds)
{
	if(mMessagePipe)
	{
		mMessagePipe->pump(seconds);
	}
	else
	{
		ms_sleep((int)(seconds * 1000.0f));
	}
}

void LLPluginProcessChild::pump(void)
{
	if(mMessagePipe)
	{
		mMessagePipe->pump(0.0f);
	}
	else
	{
		// Should we warn here?
	}
}


bool LLPluginProcessChild::isRunning(void)
{
	bool result = false;
	
	if(mState == STATE_RUNNING)
		result = true;
		
	return result;
}

bool LLPluginProcessChild::isDone(void)
{
	bool result = false;
	
	switch(mState)
	{
		case STATE_DONE:
		result = true;
		break;
		default:
		break;
	}
		
	return result;
}

void LLPluginProcessChild::sendMessageToPlugin(const LLPluginMessage &message)
{
	std::string buffer = message.generate();

	LL_DEBUGS("Plugin") << "Sending to plugin: " << buffer << LL_ENDL;
	LLTimer elapsed;
	
	mInstance->sendMessage(buffer);

	mCPUElapsed += elapsed.getElapsedTimeF64();
}

void LLPluginProcessChild::sendMessageToParent(const LLPluginMessage &message)
{
	std::string buffer = message.generate();

	LL_DEBUGS("Plugin") << "Sending to parent: " << buffer << LL_ENDL;

	writeMessageRaw(buffer);
}

void LLPluginProcessChild::receiveMessageRaw(const std::string &message)
{
	// Incoming message from the TCP Socket

	LL_DEBUGS("Plugin") << "Received from parent: " << message << LL_ENDL;

	bool passMessage = true;
	
	// FIXME: how should we handle queueing here?
	
	{
		// Decode this message
		LLPluginMessage parsed;
		parsed.parse(message);
		
		std::string message_class = parsed.getClass();
		if(message_class == LLPLUGIN_MESSAGE_CLASS_INTERNAL)
		{
			passMessage = false;
			
			std::string message_name = parsed.getName();
			if(message_name == "load_plugin")
			{
				mPluginFile = parsed.getValue("file");
				mUserDataPath = parsed.getValue("user_data_path");
			}
			else if(message_name == "shm_add")
			{
				std::string name = parsed.getValue("name");
				size_t size = (size_t)parsed.getValueS32("size");
				
				sharedMemoryRegionsType::iterator iter = mSharedMemoryRegions.find(name);
				if(iter != mSharedMemoryRegions.end())
				{
					// Need to remove the old region first
					LL_WARNS("Plugin") << "Adding a duplicate shared memory segment!" << LL_ENDL;
				}
				else
				{
					// This is a new region
					LLPluginSharedMemory *region = new LLPluginSharedMemory;
					if(region->attach(name, size))
					{
						mSharedMemoryRegions.insert(sharedMemoryRegionsType::value_type(name, region));
						
						std::stringstream addr;
						addr << region->getMappedAddress();
						
						// Send the add notification to the plugin
						LLPluginMessage message("base", "shm_added");
						message.setValue("name", name);
						message.setValueS32("size", (S32)size);
						message.setValuePointer("address", region->getMappedAddress());
						sendMessageToPlugin(message);
						
						// and send the response to the parent
						message.setMessage(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "shm_add_response");
						message.setValue("name", name);
						sendMessageToParent(message);
					}
					else
					{
						LL_WARNS("Plugin") << "Couldn't create a shared memory segment!" << LL_ENDL;
						delete region;
					}
				}
				
			}
			else if(message_name == "shm_remove")
			{
				std::string name = parsed.getValue("name");
				sharedMemoryRegionsType::iterator iter = mSharedMemoryRegions.find(name);
				if(iter != mSharedMemoryRegions.end())
				{
					// forward the remove request to the plugin -- its response will trigger us to detach the segment.
					LLPluginMessage message("base", "shm_remove");
					message.setValue("name", name);
					sendMessageToPlugin(message);
				}
				else
				{
					LL_WARNS("Plugin") << "shm_remove for unknown memory segment!" << LL_ENDL;
				}
			}
			else if(message_name == "sleep_time")
			{
				mSleepTime = parsed.getValueReal("time");
			}
			else if(message_name == "crash")
			{
				// Crash the plugin
				LL_ERRS("Plugin") << "Plugin crash requested." << LL_ENDL;
			}
			else if(message_name == "hang")
			{
				// Hang the plugin
				LL_WARNS("Plugin") << "Plugin hang requested." << LL_ENDL;
				while(1)
				{
					// wheeeeeeeee......
				}
			}
			else
			{
				LL_WARNS("Plugin") << "Unknown internal message from parent: " << message_name << LL_ENDL;
			}
		}
	}
	
	if(passMessage && mInstance != NULL)
	{
		LLTimer elapsed;

		mInstance->sendMessage(message);

		mCPUElapsed += elapsed.getElapsedTimeF64();
	}
}

/* virtual */ 
void LLPluginProcessChild::receivePluginMessage(const std::string &message)
{
	LL_DEBUGS("Plugin") << "Received from plugin: " << message << LL_ENDL;

	// Incoming message from the plugin instance
	bool passMessage = true;

	// FIXME: how should we handle queueing here?
	
	// Intercept certain base messages (responses to ones sent by this class)
	{
		// Decode this message
		LLPluginMessage parsed;
		parsed.parse(message);
		std::string message_class = parsed.getClass();
		if(message_class == "base")
		{
			std::string message_name = parsed.getName();
			if(message_name == "init_response")
			{
				// The plugin has finished initializing.
				setState(STATE_RUNNING);

				// Don't pass this message up to the parent
				passMessage = false;
				
				LLPluginMessage new_message(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "load_plugin_response");
				LLSD versions = parsed.getValueLLSD("versions");
				new_message.setValueLLSD("versions", versions);
				
				if(parsed.hasValue("plugin_version"))
				{
					std::string plugin_version = parsed.getValue("plugin_version");
					new_message.setValueLLSD("plugin_version", plugin_version);
				}

				// Let the parent know it's loaded and initialized.
				sendMessageToParent(new_message);
			}
			else if(message_name == "shm_remove_response")
			{
				// Don't pass this message up to the parent
				passMessage = false;

				std::string name = parsed.getValue("name");
				sharedMemoryRegionsType::iterator iter = mSharedMemoryRegions.find(name);				
				if(iter != mSharedMemoryRegions.end())
				{
					// detach the shared memory region
					iter->second->detach();
					
					// and remove it from our map
					mSharedMemoryRegions.erase(iter);
					
					// Finally, send the response to the parent.
					LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "shm_remove_response");
					message.setValue("name", name);
					sendMessageToParent(message);
				}
				else
				{
					LL_WARNS("Plugin") << "shm_remove_response for unknown memory segment!" << LL_ENDL;
				}
			}
		}
	}
	
	if(passMessage)
	{
		LL_DEBUGS("Plugin") << "Passing through to parent: " << message << LL_ENDL;
		writeMessageRaw(message);
	}
}


void LLPluginProcessChild::setState(EState state)
{
	LL_DEBUGS("Plugin") << "setting state to " << state << LL_ENDL;
	mState = state; 
};

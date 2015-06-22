/**
* @file media_plugin_cef.cpp
* @brief CEF (Chromium Embedding Framework) plugin for LLMedia API plugin system
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

#include "llgl.h"
#include "llplugininstance.h"
#include "llpluginmessage.h"
#include "llpluginmessageclasses.h"
#include "media_plugin_base.h"

#include "boost/function.hpp"
#include "boost/bind.hpp"
#include "llCEFLib.h"

#include <time.h>  // remove me

////////////////////////////////////////////////////////////////////////////////
//
class MediaPluginCEF :
	public MediaPluginBase
{
public:
	MediaPluginCEF(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data);
	~MediaPluginCEF();

	/*virtual*/
	void receiveMessage(const char* message_string);

private:
	bool init();

	// TODO FIX ME
	void pageChangedCallback(unsigned char* pixels, int width, int height)
	{
		if (mPixels && pixels)
		{
			if (mWidth == width && mHeight == height)
			{
				memcpy(mPixels, pixels, mWidth * mHeight * mDepth);
			}
			setDirty(0, 0, mWidth, mHeight);
		}
	}

	void postDebugMessage(const std::string& msg);

	bool mEnableMediaPluginDebugging;
	LLCEFLib* mLLCEFLib;
};

////////////////////////////////////////////////////////////////////////////////
//
MediaPluginCEF::MediaPluginCEF(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data) :
MediaPluginBase(host_send_func, host_user_data)
{
	mWidth = 0;
	mHeight = 0;
	mDepth = 4;
	mPixels = 0;
	mEnableMediaPluginDebugging = true;

	mLLCEFLib = new LLCEFLib();
}

////////////////////////////////////////////////////////////////////////////////
//
MediaPluginCEF::~MediaPluginCEF()
{
	mLLCEFLib->reset();
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::postDebugMessage(const std::string& msg)
{
	//if (mEnableMediaPluginDebugging)
	{
		std::stringstream str;
		str << "@Media Msg> " << msg;

		LLPluginMessage debug_message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "debug_message");
		debug_message.setValue("message_text", str.str());
		debug_message.setValue("message_level", "info");
		sendMessage(debug_message);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::receiveMessage(const char* message_string)
{
	//  std::cerr << "MediaPluginWebKit::receiveMessage: received message: \"" << message_string << "\"" << std::endl;
	LLPluginMessage message_in;

	if (message_in.parse(message_string) >= 0)
	{
		std::string message_class = message_in.getClass();
		std::string message_name = message_in.getName();
		if (message_class == LLPLUGIN_MESSAGE_CLASS_BASE)
		{
			if (message_name == "init")
			{
				LLPluginMessage message("base", "init_response");
				LLSD versions = LLSD::emptyMap();
				versions[LLPLUGIN_MESSAGE_CLASS_BASE] = LLPLUGIN_MESSAGE_CLASS_BASE_VERSION;
				versions[LLPLUGIN_MESSAGE_CLASS_MEDIA] = LLPLUGIN_MESSAGE_CLASS_MEDIA_VERSION;
				versions[LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER] = LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER_VERSION;
				message.setValueLLSD("versions", versions);

				std::string plugin_version = "Example plugin 1.0..0";
				message.setValue("plugin_version", plugin_version);
				sendMessage(message);
			}
			else if (message_name == "idle")
			{
				mLLCEFLib->update();
			}
			else if (message_name == "cleanup")
			{
			}
			else if (message_name == "shm_added")
			{
				SharedSegmentInfo info;
				info.mAddress = message_in.getValuePointer("address");
				info.mSize = (size_t)message_in.getValueS32("size");
				std::string name = message_in.getValue("name");

				mSharedSegments.insert(SharedSegmentMap::value_type(name, info));

			}
			else if (message_name == "shm_remove")
			{
				std::string name = message_in.getValue("name");

				SharedSegmentMap::iterator iter = mSharedSegments.find(name);
				if (iter != mSharedSegments.end())
				{
					if (mPixels == iter->second.mAddress)
					{
						mPixels = NULL;
						mTextureSegmentName.clear();
					}
					mSharedSegments.erase(iter);
				}
				else
				{
					//std::cerr << "MediaPluginWebKit::receiveMessage: unknown shared memory region!" << std::endl;
				}

				LLPluginMessage message("base", "shm_remove_response");
				message.setValue("name", name);
				sendMessage(message);
			}
			else
			{
				//std::cerr << "MediaPluginWebKit::receiveMessage: unknown base message: " << message_name << std::endl;
			}
		}
		else if (message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA)
		{
			if (message_name == "init")
			{

				mLLCEFLib->setPageChangedCallback(boost::bind(&MediaPluginCEF::pageChangedCallback, this, _1, _2, _3));

	            LLCEFLibSettings settings;
	            settings.inital_width = 1024;
	            settings.inital_height = 1024;
	            settings.javascript_enabled = true;
	            settings.cookies_enabled = true;
				bool result = mLLCEFLib->init(settings);
				if (!result)
				{
					//MessageBoxA(0, "FAIL INIT", 0, 0);
				}

				// Plugin gets to decide the texture parameters to use.
				mDepth = 4;
				LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "texture_params");
				message.setValueS32("default_width", 1024);
				message.setValueS32("default_height", 1024);
				message.setValueS32("depth", mDepth);
				message.setValueU32("internalformat", GL_RGB);
				message.setValueU32("format", GL_BGRA);
				message.setValueU32("type", GL_UNSIGNED_BYTE);
				message.setValueBoolean("coords_opengl", false);
				sendMessage(message);
			}
			else if (message_name == "size_change")
			{
				std::string name = message_in.getValue("name");
				S32 width = message_in.getValueS32("width");
				S32 height = message_in.getValueS32("height");
				S32 texture_width = message_in.getValueS32("texture_width");
				S32 texture_height = message_in.getValueS32("texture_height");

				if (!name.empty())
				{
					// Find the shared memory region with this name
					SharedSegmentMap::iterator iter = mSharedSegments.find(name);
					if (iter != mSharedSegments.end())
					{
						mPixels = (unsigned char*)iter->second.mAddress;
						mWidth = width;
						mHeight = height;

						mTextureWidth = texture_width;
						mTextureHeight = texture_height;
					};
				};

				mLLCEFLib->setSize(mWidth, mHeight);

				LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "size_change_response");
				message.setValue("name", name);
				message.setValueS32("width", width);
				message.setValueS32("height", height);
				message.setValueS32("texture_width", texture_width);
				message.setValueS32("texture_height", texture_height);
				sendMessage(message);

			}
			else if (message_name == "load_uri")
			{
				std::string uri = message_in.getValue("uri");
				mLLCEFLib->navigate(uri);
			}
			else if (message_name == "mouse_event")
			{
				std::string event = message_in.getValue("event");
				//S32 button = message_in.getValueS32("button");
				S32 x = message_in.getValueS32("x");
				S32 y = message_in.getValueS32("y");

				//std::string modifiers = message_in.getValue("modifiers");

				if (event == "down")
				{
					mLLCEFLib->mouseButton(0, true, x, y);
					mLLCEFLib->setFocus(true);
					std::stringstream str;
					str << "Mouse down at = " << x << ", " << y;
					postDebugMessage(str.str());
				}
				else if (event == "up")
				{
					mLLCEFLib->mouseButton(0, false, x, y);
				}
				else if (event == "double_click")
				{
				}
				else
				{
					mLLCEFLib->mouseMove(x, y);
				}
			}
			else if (message_name == "scroll_event")
			{
				S32 y = message_in.getValueS32("y");
				const int scaling_factor = 40;
				y *= -scaling_factor;

				mLLCEFLib->mouseWheel(y);
			}
			else if (message_name == "text_event")
			{
				std::string event = message_in.getValue("event");
				S32 key = message_in.getValue("text")[0];
				std::string modifiers = message_in.getValue("modifiers");
				LLSD native_key_data = message_in.getValueLLSD("native_key_data");

				//int native_scan_code = (uint32_t)(native_key_data["scan_code"].asInteger());

				//if (event == "down")
				{
					mLLCEFLib->keyPress(key, true);
				}
				//else
				//if (event == "up")
				{
					mLLCEFLib->keyPress(key, false);
				}
			}
			else if (message_name == "key_event")
			{
				std::string event = message_in.getValue("event");
				//S32 key = message_in.getValueS32("key");
				std::string modifiers = message_in.getValue("modifiers");
				LLSD native_key_data = message_in.getValueLLSD("native_key_data");

				int native_scan_code = (uint32_t)(native_key_data["scan_code"].asInteger());
				native_scan_code = 8;

				if (event == "down")
				{
					mLLCEFLib->keyPress(native_scan_code, true);
				}
				else
				if (event == "up")
				{
					mLLCEFLib->keyPress(native_scan_code, false);
				}
			}
			else if (message_name == "enable_media_plugin_debugging")
			{
				mEnableMediaPluginDebugging = message_in.getValueBoolean("enable");
			}
		}
		else
		{
			//std::cerr << "MediaPluginWebKit::receiveMessage: unknown message class: " << message_class << std::endl;
		};
	}
}

////////////////////////////////////////////////////////////////////////////////
//
bool MediaPluginCEF::init()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "name_text");
	message.setValue("name", "Example Plugin");
	sendMessage(message);

	return true;
};

////////////////////////////////////////////////////////////////////////////////
//
int init_media_plugin(LLPluginInstance::sendMessageFunction host_send_func,
	void* host_user_data,
	LLPluginInstance::sendMessageFunction *plugin_send_func,
	void **plugin_user_data)
{
	MediaPluginCEF* self = new MediaPluginCEF(host_send_func, host_user_data);
	*plugin_send_func = MediaPluginCEF::staticReceiveMessage;
	*plugin_user_data = (void*)self;

	return 0;
}

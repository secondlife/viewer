/** 
 * @file media_plugin_base.cpp
 * @brief Media plugin base class for LLMedia API plugin system
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
#include "media_plugin_base.h"


// TODO: Make sure that the only symbol exported from this library is LLPluginInitEntryPoint
////////////////////////////////////////////////////////////////////////////////
//

MediaPluginBase::MediaPluginBase(
	LLPluginInstance::sendMessageFunction host_send_func,
	void *host_user_data )
{
	mHostSendFunction = host_send_func;
	mHostUserData = host_user_data;
	mDeleteMe = false;
	mPixels = 0;
	mWidth = 0;
	mHeight = 0;
	mTextureWidth = 0;
	mTextureHeight = 0;
	mDepth = 0;
	mStatus = STATUS_NONE;
}

std::string MediaPluginBase::statusString()
{
	std::string result;
	
	switch(mStatus)
	{
		case STATUS_LOADING:	result = "loading";		break;
		case STATUS_LOADED:		result = "loaded";		break;
		case STATUS_ERROR:		result = "error";		break;
		case STATUS_PLAYING:	result = "playing";		break;
		case STATUS_PAUSED:		result = "paused";		break;
		case STATUS_DONE:		result = "done";		break;
		default:
			// keep the empty string
		break;
	}
	
	return result;
}
	
void MediaPluginBase::setStatus(EStatus status)
{
	if(mStatus != status)
	{
		mStatus = status;
		sendStatus();
	}
}


void MediaPluginBase::staticReceiveMessage(const char *message_string, void **user_data)
{
	MediaPluginBase *self = (MediaPluginBase*)*user_data;

	if(self != NULL)
	{
		self->receiveMessage(message_string);

		// If the plugin has processed the delete message, delete it.
		if(self->mDeleteMe)
		{
			delete self;
			*user_data = NULL;
		}
	}
}

void MediaPluginBase::sendMessage(const LLPluginMessage &message)
{
	std::string output = message.generate();
	mHostSendFunction(output.c_str(), &mHostUserData);
}

void MediaPluginBase::setDirty(int left, int top, int right, int bottom)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "updated");

	message.setValueS32("left", left);
	message.setValueS32("top", top);
	message.setValueS32("right", right);
	message.setValueS32("bottom", bottom);
	
	sendMessage(message);
}

void MediaPluginBase::sendStatus()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "media_status");

	message.setValue("status", statusString());
	
	sendMessage(message);
}


#if LL_WINDOWS
# define LLSYMEXPORT __declspec(dllexport)
#elif LL_LINUX
# define LLSYMEXPORT __attribute__ ((visibility("default")))
#else
# define LLSYMEXPORT /**/
#endif

extern "C"
{
	LLSYMEXPORT int LLPluginInitEntryPoint(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data, LLPluginInstance::sendMessageFunction *plugin_send_func, void **plugin_user_data);
}

LLSYMEXPORT int
LLPluginInitEntryPoint(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data, LLPluginInstance::sendMessageFunction *plugin_send_func, void **plugin_user_data)
{
	return init_media_plugin(host_send_func, host_user_data, plugin_send_func, plugin_user_data);
}

#ifdef WIN32
int WINAPI DllEntryPoint( HINSTANCE hInstance, unsigned long reason, void* params )
{
	return 1;
}
#endif

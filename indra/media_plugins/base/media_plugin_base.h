/** 
 * @file media_plugin_base.h
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

#include "llplugininstance.h"
#include "llpluginmessage.h"
#include "llpluginmessageclasses.h"


class MediaPluginBase
{
public:
	MediaPluginBase(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data);
	virtual ~MediaPluginBase() {}

	virtual void receiveMessage(const char *message_string) = 0;
	
	static void staticReceiveMessage(const char *message_string, void **user_data);

protected:

	typedef enum 
	{
		STATUS_NONE,
		STATUS_LOADING,
		STATUS_LOADED,
		STATUS_ERROR,
		STATUS_PLAYING,
		STATUS_PAUSED,
		STATUS_DONE
	} EStatus;

	class SharedSegmentInfo
	{
	public:
		void *mAddress;
		size_t mSize;
	};

	void sendMessage(const LLPluginMessage &message);
	void sendStatus();
	std::string statusString();
	void setStatus(EStatus status);		
	
	// The quicktime plugin overrides this to add current time and duration to the message...
	virtual void setDirty(int left, int top, int right, int bottom);

	typedef std::map<std::string, SharedSegmentInfo> SharedSegmentMap;

	
	LLPluginInstance::sendMessageFunction mHostSendFunction;
	void *mHostUserData;
	bool mDeleteMe;
	unsigned char* mPixels;
	std::string mTextureSegmentName;
	int mWidth;
	int mHeight;
	int mTextureWidth;
	int mTextureHeight;
	int mDepth;
	EStatus mStatus;
	SharedSegmentMap mSharedSegments;

};

// The plugin must define this function to create its instance.
int init_media_plugin(
	LLPluginInstance::sendMessageFunction host_send_func, 
	void *host_user_data, 
	LLPluginInstance::sendMessageFunction *plugin_send_func, 
	void **plugin_user_data);

// It should look something like this:
/*
{
	MediaPluginFoo *self = new MediaPluginFoo(host_send_func, host_user_data);
	*plugin_send_func = MediaPluginFoo::staticReceiveMessage;
	*plugin_user_data = (void*)self;

	return 0;
}
*/


/** 
 * @file llfacebookconnect.h
 * @author Merov, Cho, Gil
 * @brief Connection to Facebook Service
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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
 */

#ifndef LL_LLFACEBOOKCONNECT_H
#define LL_LLFACEBOOKCONNECT_H

#include "llsingleton.h"
#include "llimage.h"

class LLEventPump;

/**
 * @class LLFacebookConnect
 *
 * Manages authentication to, and interaction with, a web service allowing the
 * the viewer to get Facebook OpenGraph data.
 */
class LLFacebookConnect : public LLSingleton<LLFacebookConnect>
{
	LOG_CLASS(LLFacebookConnect);
public:
    enum EConnectionState
	{
		FB_NOT_CONNECTED = 0,
		FB_CONNECTION_IN_PROGRESS = 1,
		FB_CONNECTED = 2,
		FB_CONNECTION_FAILED = 3,
		FB_POSTING = 4,
		FB_POST_FAILED = 5
	};
	
	typedef boost::function<void(bool ok)> share_callback_t;
	typedef boost::function<void()> content_updated_callback_t;

	void connectToFacebook(const std::string& auth_code = "");	// Initiate the complete FB connection. Please use checkConnectionToFacebook() in normal use.
	void disconnectFromFacebook();								// Disconnect from the FBC service.
    void checkConnectionToFacebook(bool auto_connect = false);	// Check if an access token is available on the FBC service. If not, call connectToFacebook().
    
    void loadFacebookFriends();
	void postCheckin(const std::string& location, const std::string& name, const std::string& description, const std::string& picture, const std::string& message);
    void sharePhoto(const std::string& image_url, const std::string& caption);
	void sharePhoto(LLPointer<LLImageFormatted> image, const std::string& caption);
	void updateStatus(const std::string& message);
	
	void setPostCheckinCallback(share_callback_t cb) { mPostCheckinCallback = cb; }
	void setSharePhotoCallback(share_callback_t cb) { mSharePhotoCallback = cb; }
	void setUpdateStatusCallback(share_callback_t cb) { mUpdateStatusCallback = cb; }
	void setContentUpdatedCallback(content_updated_callback_t cb) { mContentUpdatedCallback = cb;}

    void clearContent();
	void storeContent(const LLSD& content);
    const LLSD& getContent() const;
    
    void setConnectionState(EConnectionState connection_state);
	bool isConnected() { return ((mConnectionState == FB_CONNECTED) || (mConnectionState == FB_POSTING)); }
    S32  generation() { return mGeneration; }
    
    void openFacebookWeb(std::string url);

private:
	friend class LLSingleton<LLFacebookConnect>;

	LLFacebookConnect();
	~LLFacebookConnect() {};
 	std::string getFacebookConnectURL(const std::string& route = "");
   
    EConnectionState mConnectionState;
    LLSD mContent;
    S32  mGeneration;
	
	share_callback_t mPostCheckinCallback;
	share_callback_t mSharePhotoCallback;
	share_callback_t mUpdateStatusCallback;
	content_updated_callback_t mContentUpdatedCallback;

	static boost::scoped_ptr<LLEventPump> sStateWatcher;
};

#endif // LL_LLFACEBOOKCONNECT_H

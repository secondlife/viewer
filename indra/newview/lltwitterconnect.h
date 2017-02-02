/** 
 * @file lltwitterconnect.h
 * @author Merov, Cho
 * @brief Connection to Twitter Service
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

#ifndef LL_LLTWITTERCONNECT_H
#define LL_LLTWITTERCONNECT_H

#include "llsingleton.h"
#include "llimage.h"
#include "llcoros.h"
#include "lleventcoro.h"

class LLEventPump;

/**
 * @class LLTwitterConnect
 *
 * Manages authentication to, and interaction with, a web service allowing the
 * the viewer to post status updates and upload photos to Twitter.
 */
class LLTwitterConnect : public LLSingleton<LLTwitterConnect>
{
	LLSINGLETON(LLTwitterConnect);
	LOG_CLASS(LLTwitterConnect);
public:
    enum EConnectionState
	{
		TWITTER_NOT_CONNECTED = 0,
		TWITTER_CONNECTION_IN_PROGRESS = 1,
		TWITTER_CONNECTED = 2,
		TWITTER_CONNECTION_FAILED = 3,
		TWITTER_POSTING = 4,
		TWITTER_POSTED = 5,
		TWITTER_POST_FAILED = 6,
		TWITTER_DISCONNECTING = 7,
		TWITTER_DISCONNECT_FAILED = 8
	};
	
	void connectToTwitter(const std::string& request_token = "", const std::string& oauth_verifier = "");	// Initiate the complete Twitter connection. Please use checkConnectionToTwitter() in normal use.
	void disconnectFromTwitter();																			// Disconnect from the Twitter service.
    void checkConnectionToTwitter(bool auto_connect = false);												// Check if an access token is available on the Twitter service. If not, call connectToTwitter().
    
	void loadTwitterInfo();
	void uploadPhoto(const std::string& image_url, const std::string& status);
	void uploadPhoto(LLPointer<LLImageFormatted> image, const std::string& status);
	void updateStatus(const std::string& status);
	
	void storeInfo(const LLSD& info);
	const LLSD& getInfo() const;
	void clearInfo();
	void setDataDirty();
    
    void setConnectionState(EConnectionState connection_state);
	void setConnected(bool connected);
	bool isConnected() { return mConnected; }
	bool isTransactionOngoing() { return ((mConnectionState == TWITTER_CONNECTION_IN_PROGRESS) || (mConnectionState == TWITTER_POSTING) || (mConnectionState == TWITTER_DISCONNECTING)); }
    EConnectionState getConnectionState() { return mConnectionState; }
    
    void openTwitterWeb(std::string url);

private:

 	std::string getTwitterConnectURL(const std::string& route = "", bool include_read_from_master = false);

    EConnectionState mConnectionState;
	BOOL mConnected;
	LLSD mInfo;
	bool mRefreshInfo;
	bool mReadFromMaster;
	
	static boost::scoped_ptr<LLEventPump> sStateWatcher;
	static boost::scoped_ptr<LLEventPump> sInfoWatcher;
	static boost::scoped_ptr<LLEventPump> sContentWatcher;

    bool testShareStatus(LLSD &result);
    void twitterConnectCoro(std::string requestToken, std::string oauthVerifier);
    void twitterDisconnectCoro();
    void twitterConnectedCoro(bool autoConnect);
    void twitterInfoCoro();
    void twitterShareCoro(std::string route, LLSD share);
    void twitterShareImageCoro(LLPointer<LLImageFormatted> image, std::string status);
};

#endif // LL_LLTWITTERCONNECT_H

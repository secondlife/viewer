/** 
 * @file llflickrconnect.h
 * @author Merov, Cho
 * @brief Connection to Flickr Service
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

#ifndef LL_LLFLICKRCONNECT_H
#define LL_LLFLICKRCONNECT_H

#include "llsingleton.h"
#include "llimage.h"
#include "llcoros.h"
#include "lleventcoro.h"

class LLEventPump;

/**
 * @class LLFlickrConnect
 *
 * Manages authentication to, and interaction with, a web service allowing the
 * the viewer to upload photos to Flickr.
 */
class LLFlickrConnect : public LLSingleton<LLFlickrConnect>
{
	LLSINGLETON(LLFlickrConnect);
	~LLFlickrConnect() {};
	LOG_CLASS(LLFlickrConnect);
public:
    enum EConnectionState
	{
		FLICKR_NOT_CONNECTED = 0,
		FLICKR_CONNECTION_IN_PROGRESS = 1,
		FLICKR_CONNECTED = 2,
		FLICKR_CONNECTION_FAILED = 3,
		FLICKR_POSTING = 4,
		FLICKR_POSTED = 5,
		FLICKR_POST_FAILED = 6,
		FLICKR_DISCONNECTING = 7,
		FLICKR_DISCONNECT_FAILED = 8
	};
	
	void connectToFlickr(const std::string& request_token = "", const std::string& oauth_verifier = "");	// Initiate the complete Flickr connection. Please use checkConnectionToFlickr() in normal use.
	void disconnectFromFlickr();																			// Disconnect from the Flickr service.
    void checkConnectionToFlickr(bool auto_connect = false);												// Check if an access token is available on the Flickr service. If not, call connectToFlickr().
    
	void loadFlickrInfo();
	void uploadPhoto(const std::string& image_url, const std::string& title, const std::string& description, const std::string& tags, int safety_level);
	void uploadPhoto(LLPointer<LLImageFormatted> image, const std::string& title, const std::string& description, const std::string& tags, int safety_level);
	
	void storeInfo(const LLSD& info);
	const LLSD& getInfo() const;
	void clearInfo();
	void setDataDirty();
    
    void setConnectionState(EConnectionState connection_state);
	void setConnected(bool connected);
	bool isConnected() { return mConnected; }
	bool isTransactionOngoing() { return ((mConnectionState == FLICKR_CONNECTION_IN_PROGRESS) || (mConnectionState == FLICKR_POSTING) || (mConnectionState == FLICKR_DISCONNECTING)); }
    EConnectionState getConnectionState() { return mConnectionState; }
    
    void openFlickrWeb(std::string url);

private:

 	std::string getFlickrConnectURL(const std::string& route = "", bool include_read_from_master = false);

    EConnectionState mConnectionState;
	BOOL mConnected;
	LLSD mInfo;
	bool mRefreshInfo;
	bool mReadFromMaster;
	
	static boost::scoped_ptr<LLEventPump> sStateWatcher;
	static boost::scoped_ptr<LLEventPump> sInfoWatcher;
	static boost::scoped_ptr<LLEventPump> sContentWatcher;

    bool testShareStatus(LLSD &result);
    void flickrConnectCoro(std::string requestToken, std::string oauthVerifier);
    void flickrShareCoro(LLSD share);
    void flickrShareImageCoro(LLPointer<LLImageFormatted> image, std::string title, std::string description, std::string tags, int safetyLevel);
    void flickrDisconnectCoro();
    void flickrConnectedCoro(bool autoConnect);
    void flickrInfoCoro();

};

#endif // LL_LLFLICKRCONNECT_H

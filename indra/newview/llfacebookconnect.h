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
	void connectToFacebook(const std::string& auth_code = "");
	void disconnectFromFacebook();
	void tryToReconnectToFacebook();
    void getConnectionToFacebook();
    
    void loadFacebookFriends();
	void postCheckin(const std::string& location, const std::string& name, const std::string& description, const std::string& picture, const std::string& message);
    void postCheckinMessage(const std::string& message, const std::string& link, const std::string& name, const std::string& caption, const std::string& description, const std::string& picture);
	void sharePhoto(const std::string& image_url, const std::string& caption);
	void updateStatus(const std::string& message);

    void clearContent();
	void storeContent(const LLSD& content);
    const LLSD& getContent() const;
    
    void setConnected(bool connected) { mConnectedToFbc = connected; }
    bool getConnected() { return mConnectedToFbc; }
    S32  generation() { return mGeneration; }
    
    void openFacebookWeb(std::string url);
private:

	friend class LLSingleton<LLFacebookConnect>;

	LLFacebookConnect();
	~LLFacebookConnect() {};
 	std::string getFacebookConnectURL(const std::string& route = "");
   
    bool mConnectedToFbc;
    LLSD mContent;
    S32  mGeneration;
};

#endif // LL_LLFACEBOOKCONNECT_H

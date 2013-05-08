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
	/*
	 * Performs initial setup, by requesting config data from the web service if
	 * it has not already been received.
	 */
	void init();

    void loadFacebookFriends();
    void hideFacebookFriends();
	void connectToFacebook(const std::string& auth_code = "");
	void showFacebookFriends(const LLSD& friends);
    
    void setConnected(bool connected) { mConnectedToFbc = connected; }

private:

	friend class LLSingleton<LLFacebookConnect>;

	LLFacebookConnect();
	~LLFacebookConnect() {};
    
    bool mConnectedToFbc;
};

#endif // LL_LLFACEBOOKCONNECT_H

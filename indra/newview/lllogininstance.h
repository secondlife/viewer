/** 
 * @file lllogininstance.h
 * @brief A host for the viewer's login connection.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
 */

#ifndef LL_LLLOGININSTANCE_H
#define LL_LLLOGININSTANCE_H

#include "lleventdispatcher.h"
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>
#include "llsecapi.h"
class LLLogin;
class LLEventStream;
class LLNotificationsInterface;

// This class hosts the login module and is used to 
// negotiate user authentication attempts.
class LLLoginInstance : public LLSingleton<LLLoginInstance>
{
	LLSINGLETON(LLLoginInstance);
	~LLLoginInstance();

public:
	class Disposable;

	void connect(LLPointer<LLCredential> credentials); // Connect to the current grid choice.
	void connect(const std::string& uri, LLPointer<LLCredential> credentials);	// Connect to the given uri.
	void reconnect(); // reconnect using the current credentials.
	void disconnect();

	bool authFailure() { return mAttemptComplete && mLoginState == "offline"; }
	bool authSuccess() { return mAttemptComplete && mLoginState == "online"; }

	const std::string& getLoginState() { return mLoginState; }
	LLSD getResponse(const std::string& key) { return getResponse()[key]; }
	LLSD getResponse();

	// Only valid when authSuccess == true.
	const F64 getLastTransferRateBPS() { return mTransferRate; }
	void setSerialNumber(const std::string& sn) { mSerialNumber = sn; }
	void setLastExecEvent(int lee) { mLastExecEvent = lee; }
	void setLastExecDuration(S32 duration) { mLastExecDuration = duration; }
	void setPlatformInfo(const std::string platform, const std::string platform_version, const std::string platform_name);

	void setNotificationsInterface(LLNotificationsInterface* ni) { mNotifications = ni; }
	LLNotificationsInterface& getNotificationsInterface() const { return *mNotifications; }

private:
	void constructAuthParams(LLPointer<LLCredential> user_credentials);
	void updateApp(bool mandatory, const std::string& message);
	bool updateDialogCallback(const LLSD& notification, const LLSD& response);

	bool handleLoginEvent(const LLSD& event);
	void handleLoginFailure(const LLSD& event);
	void handleLoginSuccess(const LLSD& event);
	void handleDisconnect(const LLSD& event);
	void handleIndeterminate(const LLSD& event);
    void handleLoginDisallowed(const LLSD& notification, const LLSD& response);

	bool handleTOSResponse(bool v, const std::string& key);

	void attemptComplete() { mAttemptComplete = true; } // In the future an event?

	boost::scoped_ptr<LLLogin> mLoginModule;
	LLNotificationsInterface* mNotifications;

	std::string mLoginState;
	LLSD mRequestData;
	LLSD mResponseData;
	bool mAttemptComplete;
	F64 mTransferRate;
	std::string mSerialNumber;
	int mLastExecEvent;
	S32 mLastExecDuration;
	std::string mPlatform;
	std::string mPlatformVersion;
	std::string mPlatformVersionName;
	LLEventDispatcher mDispatcher;
};

#endif

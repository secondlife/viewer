/** 
 * @file lllogininstance.h
 * @brief A host for the viewer's login connection.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLLOGININSTANCE_H
#define LL_LLLOGININSTANCE_H

#include "lleventdispatcher.h"
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>
class LLLogin;
class LLEventStream;
class LLNotificationsInterface;

// This class hosts the login module and is used to 
// negotiate user authentication attempts.
class LLLoginInstance : public LLSingleton<LLLoginInstance>
{
public:
	LLLoginInstance();
	~LLLoginInstance();

	void connect(const LLSD& credential); // Connect to the current grid choice.
	void connect(const std::string& uri, const LLSD& credential);	// Connect to the given uri.
	void reconnect(); // reconnect using the current credentials.
	void disconnect();

	bool authFailure() { return mAttemptComplete && mLoginState == "offline"; }
	bool authSuccess() { return mAttemptComplete && mLoginState == "online"; }

	const std::string& getLoginState() { return mLoginState; }
	LLSD getResponse(const std::string& key) { return getResponse()[key]; }
	LLSD getResponse();

	// Only valid when authSuccess == true.
	const F64 getLastTransferRateBPS() { return mTransferRate; }

		// Set whether this class will drive user interaction.
	// If not, login failures like 'need tos agreement' will 
	// end the login attempt.
	void setUserInteraction(bool state) { mUserInteraction = state; } 
	bool getUserInteraction() { return mUserInteraction; }

	// Whether to tell login to skip optional update request.
	// False by default.
	void setSkipOptionalUpdate(bool state) { mSkipOptionalUpdate = state; }
	void setSerialNumber(const std::string& sn) { mSerialNumber = sn; }
	void setLastExecEvent(int lee) { mLastExecEvent = lee; }

	void setNotificationsInterface(LLNotificationsInterface* ni) { mNotifications = ni; }

	typedef boost::function<void()> UpdaterLauncherCallback;
	void setUpdaterLauncher(const UpdaterLauncherCallback& ulc) { mUpdaterLauncher = ulc; }

private:
	void constructAuthParams(const LLSD& credentials); 
	void updateApp(bool mandatory, const std::string& message);
	bool updateDialogCallback(const LLSD& notification, const LLSD& response);

	bool handleLoginEvent(const LLSD& event);
	void handleLoginFailure(const LLSD& event);
	void handleLoginSuccess(const LLSD& event);
	void handleDisconnect(const LLSD& event);
	void handleIndeterminate(const LLSD& event);

	bool handleTOSResponse(bool v, const std::string& key);

	void attemptComplete() { mAttemptComplete = true; } // In the future an event?

	boost::scoped_ptr<LLLogin> mLoginModule;
	LLNotificationsInterface* mNotifications;

	std::string mLoginState;
	LLSD mRequestData;
	LLSD mResponseData;
	bool mUserInteraction; 
	bool mSkipOptionalUpdate;
	bool mAttemptComplete;
	F64 mTransferRate;
	std::string mSerialNumber;
	int mLastExecEvent;
	UpdaterLauncherCallback mUpdaterLauncher;
	LLEventDispatcher mDispatcher;
};

#endif

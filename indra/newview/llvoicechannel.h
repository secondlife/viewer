/** 
 * @file llvoicechannel.h
 * @brief Voice channel related classes
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_VOICECHANNEL_H
#define LL_VOICECHANNEL_H

#include "llhandle.h"
#include "llvoiceclient.h"

class LLPanel;

class LLVoiceChannel : public LLVoiceClientStatusObserver
{
public:
	typedef enum e_voice_channel_state
	{
		STATE_NO_CHANNEL_INFO,
		STATE_ERROR,
		STATE_HUNG_UP,
		STATE_READY,
		STATE_CALL_STARTED,
		STATE_RINGING,
		STATE_CONNECTED
	} EState;

	typedef enum e_voice_channel_direction
	{
		INCOMING_CALL,
		OUTGOING_CALL
	} EDirection;

	typedef boost::signals2::signal<void(const EState& old_state, const EState& new_state, const EDirection& direction, bool ended_by_agent)> state_changed_signal_t;

	// on current channel changed signal
	typedef boost::function<void(const LLUUID& session_id)> channel_changed_callback_t;
	typedef boost::signals2::signal<void(const LLUUID& session_id)> channel_changed_signal_t;
	static channel_changed_signal_t sCurrentVoiceChannelChangedSignal;
	static boost::signals2::connection setCurrentVoiceChannelChangedCallback(channel_changed_callback_t cb, bool at_front = false);


	LLVoiceChannel(const LLUUID& session_id, const std::string& session_name);
	virtual ~LLVoiceChannel();

	/*virtual*/ void onChange(EStatusType status, const std::string &channelURI, bool proximal);

	virtual void handleStatusChange(EStatusType status);
	virtual void handleError(EStatusType status);
	virtual void deactivate();
	virtual void activate();
	virtual void setChannelInfo(
		const std::string& uri,
		const std::string& credentials);
	virtual void getChannelInfo();
	virtual BOOL isActive();
	virtual BOOL callStarted();

	// Session name is a UI label used for feedback about which person,
	// group, or phone number you are talking to
	const std::string& getSessionName() const { return mSessionName; }

	boost::signals2::connection setStateChangedCallback(const state_changed_signal_t::slot_type& callback)
	{ return mStateChangedCallback.connect(callback); }

	const LLUUID getSessionID() { return mSessionID; }
	EState getState() { return mState; }

	void updateSessionID(const LLUUID& new_session_id);
	const LLSD& getNotifyArgs() { return mNotifyArgs; }

	void setCallDirection(EDirection direction) {mCallDirection = direction;}
	EDirection getCallDirection() {return mCallDirection;}

	static LLVoiceChannel* getChannelByID(const LLUUID& session_id);
	static LLVoiceChannel* getChannelByURI(std::string uri);
	static LLVoiceChannel* getCurrentVoiceChannel();
	
	static void initClass();
	
	static void suspend();
	static void resume();

protected:
	virtual void setState(EState state);
	/**
	 * Use this method if you want mStateChangedCallback to be executed while state is changed
	 */
	void doSetState(const EState& state);
	void setURI(std::string uri);

	// there can be two directions INCOMING and OUTGOING
	EDirection mCallDirection;

	std::string	mURI;
	std::string	mCredentials;
	LLUUID		mSessionID;
	EState		mState;
	std::string	mSessionName;
	LLSD mNotifyArgs;
	LLSD mCallDialogPayload;
	// true if call was ended by agent
	bool mCallEndedByAgent;
	BOOL		mIgnoreNextSessionLeave;
	LLHandle<LLPanel> mLoginNotificationHandle;

	typedef std::map<LLUUID, LLVoiceChannel*> voice_channel_map_t;
	static voice_channel_map_t sVoiceChannelMap;

	typedef std::map<std::string, LLVoiceChannel*> voice_channel_map_uri_t;
	static voice_channel_map_uri_t sVoiceChannelURIMap;

	static LLVoiceChannel* sCurrentVoiceChannel;
	static LLVoiceChannel* sSuspendedVoiceChannel;
	static BOOL sSuspended;

private:
	state_changed_signal_t mStateChangedCallback;
};

class LLVoiceChannelGroup : public LLVoiceChannel
{
public:
	LLVoiceChannelGroup(const LLUUID& session_id, const std::string& session_name);

	/*virtual*/ void handleStatusChange(EStatusType status);
	/*virtual*/ void handleError(EStatusType status);
	/*virtual*/ void activate();
	/*virtual*/ void deactivate();
	/*vritual*/ void setChannelInfo(
		const std::string& uri,
		const std::string& credentials);
	/*virtual*/ void getChannelInfo();

protected:
	virtual void setState(EState state);

private:
	U32 mRetries;
	BOOL mIsRetrying;
};

class LLVoiceChannelProximal : public LLVoiceChannel, public LLSingleton<LLVoiceChannelProximal>
{
public:
	LLVoiceChannelProximal();

	/*virtual*/ void onChange(EStatusType status, const std::string &channelURI, bool proximal);
	/*virtual*/ void handleStatusChange(EStatusType status);
	/*virtual*/ void handleError(EStatusType status);
	/*virtual*/ BOOL isActive();
	/*virtual*/ void activate();
	/*virtual*/ void deactivate();

};

class LLVoiceChannelP2P : public LLVoiceChannelGroup
{
public:
	LLVoiceChannelP2P(const LLUUID& session_id, const std::string& session_name, const LLUUID& other_user_id);

	/*virtual*/ void handleStatusChange(EStatusType status);
	/*virtual*/ void handleError(EStatusType status);
    /*virtual*/ void activate();
	/*virtual*/ void getChannelInfo();

	void setSessionHandle(const std::string& handle, const std::string &inURI);

protected:
	virtual void setState(EState state);

private:

	/**
	* Add the caller to the list of people with which we've recently interacted
	*
	**/
	void addToTheRecentPeopleList();

	std::string	mSessionHandle;
	LLUUID		mOtherUserID;
	BOOL		mReceivedCall;
};

#endif  // LL_VOICECHANNEL_H

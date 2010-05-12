/** 
 * @file llvoicechannel.h
 * @brief Voice channel related classes
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

	// there can be two directions ICOMING and OUTGOING
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
	std::string	mSessionHandle;
	LLUUID		mOtherUserID;
	BOOL		mReceivedCall;
};

#endif  // LL_VOICECHANNEL_H

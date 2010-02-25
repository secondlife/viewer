/** 
 * @file llspeakers.h
 * @brief Management interface for muting and controlling volume of residents currently speaking
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#ifndef LL_LLSPEAKERS_H
#define LL_LLSPEAKERS_H

#include "llevent.h"
#include "llspeakers.h"
#include "llvoicechannel.h"

class LLSpeakerMgr;

// data for a given participant in a voice channel
class LLSpeaker : public LLRefCount, public LLOldEvents::LLObservable, public LLHandleProvider<LLSpeaker>, public boost::signals2::trackable
{
public:
	typedef enum e_speaker_type
	{
		SPEAKER_AGENT,
		SPEAKER_OBJECT,
		SPEAKER_EXTERNAL	// Speaker that doesn't map to an avatar or object (i.e. PSTN caller in a group)
	} ESpeakerType;

	typedef enum e_speaker_status
	{
		STATUS_SPEAKING,
		STATUS_HAS_SPOKEN,
		STATUS_VOICE_ACTIVE,
		STATUS_TEXT_ONLY,
		STATUS_NOT_IN_CHANNEL,
		STATUS_MUTED
	} ESpeakerStatus;


	LLSpeaker(const LLUUID& id, const std::string& name = LLStringUtil::null, const ESpeakerType type = SPEAKER_AGENT);
	~LLSpeaker() {};
	void lookupName();

	void onAvatarNameLookup(const LLUUID& id, const std::string& first, const std::string& last, BOOL is_group);

	bool isInVoiceChannel();

	ESpeakerStatus	mStatus;			// current activity status in speech group
	F32				mLastSpokeTime;		// timestamp when this speaker last spoke
	F32				mSpeechVolume;		// current speech amplitude (timea average rms amplitude?)
	std::string		mDisplayName;		// cache user name for this speaker
	LLFrameTimer	mActivityTimer;	// time out speakers when they are not part of current voice channel
	BOOL			mHasSpoken;			// has this speaker said anything this session?
	BOOL			mHasLeftCurrentCall;	// has this speaker left the current voice call?
	LLColor4		mDotColor;
	LLUUID			mID;
	BOOL			mTyping;
	S32				mSortIndex;
	ESpeakerType	mType;
	BOOL			mIsModerator;
	BOOL			mModeratorMutedVoice;
	BOOL			mModeratorMutedText;
};

class LLSpeakerUpdateModeratorEvent : public LLOldEvents::LLEvent
{
public:
	LLSpeakerUpdateModeratorEvent(LLSpeaker* source);
	/*virtual*/ LLSD getValue();
private:
	const LLUUID& mSpeakerID;
	BOOL mIsModerator;
};

class LLSpeakerTextModerationEvent : public LLOldEvents::LLEvent
{
public:
	LLSpeakerTextModerationEvent(LLSpeaker* source);
	/*virtual*/ LLSD getValue();
};

class LLSpeakerVoiceModerationEvent : public LLOldEvents::LLEvent
{
public:
	LLSpeakerVoiceModerationEvent(LLSpeaker* source);
	/*virtual*/ LLSD getValue();
};

class LLSpeakerListChangeEvent : public LLOldEvents::LLEvent
{
public:
	LLSpeakerListChangeEvent(LLSpeakerMgr* source, const LLUUID& speaker_id);
	/*virtual*/ LLSD getValue();

private:
	const LLUUID& mSpeakerID;
};

class LLSpeakerMgr : public LLOldEvents::LLObservable
{
public:
	LLSpeakerMgr(LLVoiceChannel* channelp);
	virtual ~LLSpeakerMgr();

	LLPointer<LLSpeaker> findSpeaker(const LLUUID& avatar_id);
	void update(BOOL resort_ok);
	void setSpeakerTyping(const LLUUID& speaker_id, BOOL typing);
	void speakerChatted(const LLUUID& speaker_id);
	LLPointer<LLSpeaker> setSpeaker(const LLUUID& id, 
					const std::string& name = LLStringUtil::null, 
					LLSpeaker::ESpeakerStatus status = LLSpeaker::STATUS_TEXT_ONLY, 
					LLSpeaker::ESpeakerType = LLSpeaker::SPEAKER_AGENT);

	BOOL isVoiceActive();

	typedef std::vector<LLPointer<LLSpeaker> > speaker_list_t;
	void getSpeakerList(speaker_list_t* speaker_list, BOOL include_text);
	LLVoiceChannel* getVoiceChannel() { return mVoiceChannel; }
	const LLUUID getSessionID();

protected:
	virtual void updateSpeakerList();

	typedef std::map<LLUUID, LLPointer<LLSpeaker> > speaker_map_t;
	speaker_map_t		mSpeakers;

	speaker_list_t		mSpeakersSorted;
	LLFrameTimer		mSpeechTimer;
	LLVoiceChannel*		mVoiceChannel;
};

class LLIMSpeakerMgr : public LLSpeakerMgr
{
public:
	LLIMSpeakerMgr(LLVoiceChannel* channel);
	
	void updateSpeakers(const LLSD& update);
	void setSpeakers(const LLSD& speakers);

	void toggleAllowTextChat(const LLUUID& speaker_id);

	/**
	 * Mutes/Unmutes avatar for current group voice chat.
	 *
	 * It only marks avatar as muted for session and does not use local Agent's Block list.
	 * It does not mute Agent itself.
	 *
	 * @param[in] avatar_id UUID of avatar to be processed
	 * @param[in] unmute if false - specified avatar will be muted, otherwise - unmuted.
	 *
	 * @see moderateVoiceOtherParticipants()
	 */
	void moderateVoiceParticipant(const LLUUID& avatar_id, bool unmute);

	/**
	 * Mutes/Unmutes all avatars except specified for current group voice chat.
	 *
	 * It only marks avatars as muted for session and does not use local Agent's Block list.
	 * It based call moderateVoiceParticipant() for each avatar should be muted/unmuted.
	 *
	 * @param[in] excluded_avatar_id UUID of avatar NOT to be processed
	 * @param[in] unmute_everyone_else if false - avatars will be muted, otherwise - unmuted.
	 *
	 * @see moderateVoiceParticipant()
	 */
	void moderateVoiceOtherParticipants(const LLUUID& excluded_avatar_id, bool unmute_everyone_else);

	void processSessionUpdate(const LLSD& session_update);

protected:
	virtual void updateSpeakerList();

	void moderateVoiceSession(const LLUUID& session_id, bool disallow_voice);

	LLUUID mReverseVoiceModeratedAvatarID;
};

class LLActiveSpeakerMgr : public LLSpeakerMgr, public LLSingleton<LLActiveSpeakerMgr>
{
public:
	LLActiveSpeakerMgr();
protected:
	virtual void updateSpeakerList();
};

class LLLocalSpeakerMgr : public LLSpeakerMgr, public LLSingleton<LLLocalSpeakerMgr>
{
public:
	LLLocalSpeakerMgr();
	~LLLocalSpeakerMgr ();
protected:
	virtual void updateSpeakerList();
};

#endif // LL_LLSPEAKERS_H

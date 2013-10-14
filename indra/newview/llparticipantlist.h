/** 
 * @file llparticipantlist.h
 * @brief LLParticipantList : model of a conversation session with added speaker events handling
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

#ifndef LL_PARTICIPANTLIST_H
#define LL_PARTICIPANTLIST_H

#include "llviewerprecompiledheaders.h"
#include "llconversationmodel.h"

class LLSpeakerMgr;
class LLUICtrl;
class LLAvalineUpdater;

class LLParticipantList : public LLConversationItemSession
{
	LOG_CLASS(LLParticipantList);
public:

	typedef boost::function<bool (const LLUUID& speaker_id)> validate_speaker_callback_t;

	LLParticipantList(LLSpeakerMgr* data_source, LLFolderViewModelInterface& root_view_model);
	~LLParticipantList();

	/**
	 * Adds specified avatar ID to the existing list if it is not Agent's ID
	 *
	 * @param[in] avatar_id - Avatar UUID to be added into the list
	 */
	void addAvatarIDExceptAgent(const LLUUID& avatar_id);

	/**
	 * Refreshes the participant list.
	 */
	void update();

	/**
	 * Set a callback to be called before adding a speaker. Invalid speakers will not be added.
	 *
	 * If the callback is unset all speakers are considered as valid.
	 *
	 * @see onAddItemEvent()
	 */
	void setValidateSpeakerCallback(validate_speaker_callback_t cb);

protected:
	/**
	 * LLSpeakerMgr event handlers
	 */
	bool onAddItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	bool onRemoveItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	bool onClearListEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	bool onModeratorUpdateEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	bool onSpeakerUpdateEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	bool onSpeakerMuteEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);

	/**
	 * List of listeners implementing LLOldEvents::LLSimpleListener.
	 * There is no way to handle all the events in one listener as LLSpeakerMgr registers
	 * listeners in such a way that one listener can handle only one type of event
	 **/
	class BaseSpeakerListener : public LLOldEvents::LLSimpleListener
	{
	public:
		BaseSpeakerListener(LLParticipantList& parent) : mParent(parent) {}
	protected:
		LLParticipantList& mParent;
	};

	class SpeakerAddListener : public BaseSpeakerListener
	{
	public:
		SpeakerAddListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}
		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};

	class SpeakerRemoveListener : public BaseSpeakerListener
	{
	public:
		SpeakerRemoveListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}
		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};

	class SpeakerClearListener : public BaseSpeakerListener
	{
	public:
		SpeakerClearListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}
		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};

	class SpeakerUpdateListener : public BaseSpeakerListener
	{
	public:
		SpeakerUpdateListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}
		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};
	
	class SpeakerModeratorUpdateListener : public BaseSpeakerListener
	{
	public:
		SpeakerModeratorUpdateListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}
		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};
		
	class SpeakerMuteListener : public BaseSpeakerListener
	{
	public:
		SpeakerMuteListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}

		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};

private:
	void onAvalineCallerFound(const LLUUID& participant_id);
	void onAvalineCallerRemoved(const LLUUID& participant_id);

	/**
	 * Adjusts passed participant to work properly.
	 *
	 * Adds SpeakerMuteListener to process moderation actions.
	 */
	void adjustParticipant(const LLUUID& speaker_id);

	LLSpeakerMgr*		mSpeakerMgr;

	std::set<LLUUID>	mModeratorList;
	std::set<LLUUID>	mModeratorToRemoveList;

	LLPointer<SpeakerAddListener>				mSpeakerAddListener;
	LLPointer<SpeakerRemoveListener>			mSpeakerRemoveListener;
	LLPointer<SpeakerClearListener>				mSpeakerClearListener;
	LLPointer<SpeakerUpdateListener>	        mSpeakerUpdateListener;
	LLPointer<SpeakerModeratorUpdateListener>	mSpeakerModeratorListener;
	LLPointer<SpeakerMuteListener>				mSpeakerMuteListener;

	validate_speaker_callback_t mValidateSpeakerCallback;
	LLAvalineUpdater* mAvalineUpdater;
};

#endif // LL_PARTICIPANTLIST_H

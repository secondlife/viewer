/** 
 * @file llcallfloater.h
 * @author Mike Antipov
 * @brief Voice Control Panel in a Voice Chats (P2P, Group, Nearby...).
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

#ifndef LL_LLCALLFLOATER_H
#define LL_LLCALLFLOATER_H

#include "lltransientdockablefloater.h"
#include "llvoicechannel.h"
#include "llvoiceclient.h"

class LLAvatarList;
class LLAvatarListItem;
class LLAvatarName;
class LLNonAvatarCaller;
class LLOutputMonitorCtrl;
class LLParticipantList;
class LLSpeakerMgr;
class LLSpeakersDelayActionsStorage;

/**
 * The Voice Control Panel is an ambient window summoned by clicking the flyout chevron
 * on the Speak button. It can be torn-off and freely positioned onscreen.
 *
 * When the Resident is engaged in Voice Chat, the Voice Control Panel provides control
 * over the audible volume of each of the other participants, the Resident's own Voice
 * Morphing settings (if she has subscribed to enable the feature), and Voice Recording.
 *
 * When the Resident is engaged in any chat except Nearby Chat, the Voice Control Panel
 * also provides a 'Leave Call' button to allow the Resident to leave that voice channel.
 */
class LLCallFloater : public LLTransientDockableFloater, LLVoiceClientParticipantObserver
{
public:

	LOG_CLASS(LLCallFloater);

	LLCallFloater(const LLSD& key);
	~LLCallFloater();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void draw();

	/**
	 * Is called by LLVoiceClient::notifyParticipantObservers when voice participant list is changed.
	 *
	 * Refreshes list to display participants not in voice as disabled.
	 */
	/*virtual*/ void onParticipantsChanged();

	static void sOnCurrentChannelChanged(const LLUUID& session_id);

private:
	typedef enum e_voice_controls_type
	{
		VC_LOCAL_CHAT,
		VC_GROUP_CHAT,
		VC_AD_HOC_CHAT,
		VC_PEER_TO_PEER,
		VC_PEER_TO_PEER_AVALINE
	}EVoiceControls;

	typedef enum e_speaker_state
	{
		STATE_UNKNOWN,
		STATE_INVITED,
		STATE_JOINED,
		STATE_LEFT,
	} ESpeakerState;

	typedef std::map<LLUUID, ESpeakerState> speaker_state_map_t;

	void leaveCall();

	/**
	 * Updates mSpeakerManager and list according to current Voice Channel
	 *
	 * It compares mSpeakerManager & current Voice Channel session IDs.
	 * If they are different gets Speaker manager related to current channel and updates channel participant list.
	 */
	void updateSession();

	/**
	 * Refreshes participant list according to current Voice Channel
	 */
	void refreshParticipantList();

	/**
	 * Handles event on avatar list is refreshed after it was marked dirty.
	 *
	 * It sets initial participants voice states (once after the first refreshing)
	 * and updates voice states each time anybody is joined/left voice chat in session.
	 */
	void onAvatarListRefreshed();

	/**
	 * Updates window title with an avatar name
	 */
	void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name);
	
	void updateTitle();
	void initAgentData();
	void setModeratorMutedVoice(bool moderator_muted);
	void updateAgentModeratorState();
	void onModeratorNameCache(const LLAvatarName& av_name);

	/**
	 * Sets initial participants voice states in avatar list (Invited, Joined, Has Left).
	 *
	 * @see refreshParticipantList()
	 * @see onAvatarListRefreshed()
	 * @see mInitParticipantsVoiceState
	 */
	void initParticipantsVoiceState();

	/**
	 * Updates participants voice states in avatar list (Invited, Joined, Has Left).
	 *
	 * @see onAvatarListRefreshed()
	 * @see onChanged()
	 */
	void updateParticipantsVoiceState();

	/**
	 * Updates voice state of participant not in current voice channel depend on its current state.
	 */
	void updateNotInVoiceParticipantState(LLAvatarListItem* item);
	void setState(LLAvatarListItem* item, ESpeakerState state);
	void setState(const LLUUID& speaker_id, ESpeakerState state)
	{
		lldebugs << "Storing state: " << speaker_id << ", " << state << llendl;
		mSpeakerStateMap[speaker_id] = state;
	}

	ESpeakerState getState(const LLUUID& speaker_id)
	{
		lldebugs << "Getting state: " << speaker_id << ", " << mSpeakerStateMap[speaker_id] << llendl;

		return mSpeakerStateMap[speaker_id];
	}

	/**
	 * Instantiates new LLAvatarListItemRemoveTimer and adds it into the map if it is not already created.
	 *
	 * @param voice_speaker_id LLUUID of Avatar List item to be removed from the list when timer expires.
	 */
	void setVoiceRemoveTimer(const LLUUID& voice_speaker_id);

	/**
	 * Removes specified by UUID Avatar List item.
	 *
	 * @param voice_speaker_id LLUUID of Avatar List item to be removed from the list.
	 */
	bool removeVoiceLeftParticipant(const LLUUID& voice_speaker_id);

	/**
	 * Deletes all timers from the list to prevent started timers from ticking after destruction
	 * and after switching on another voice channel.
	 */
	void resetVoiceRemoveTimers();

	/**
	 * Removes specified by UUID timer from the map.
	 *
	 * @param voice_speaker_id LLUUID of Avatar List item whose timer should be removed from the map.
	 */
	void removeVoiceRemoveTimer(const LLUUID& voice_speaker_id);

	/**
	 * Called by LLParticipantList before adding a speaker to the participant list.
	 *
	 * If false is returned, the speaker will not be added to the list.
	 *
	 * @param speaker_id Speaker to validate.
	 * @return true if this is a valid speaker, false otherwise.
	 */
	bool validateSpeaker(const LLUUID& speaker_id);

	/**
	 * Connects to passed channel to be updated according to channel's voice states.
	 */
	void connectToChannel(LLVoiceChannel* channel);

	/**
	 * Callback to process changing of voice channel's states.
	 */
	void onVoiceChannelStateChanged(const LLVoiceChannel::EState& old_state, const LLVoiceChannel::EState& new_state);

	/**
	 * Updates floater according to passed channel's voice state.
	 */
	void updateState(const LLVoiceChannel::EState& new_state);

	/**
	 * Resets floater to be ready to show voice participants.
	 *
	 * Clears all data from the latest voice session.
	 */
	void reset(const LLVoiceChannel::EState& new_state);

private:
	speaker_state_map_t mSpeakerStateMap;
	LLSpeakerMgr* mSpeakerManager;
	LLParticipantList* mParticipants;
	LLAvatarList* mAvatarList;
	LLNonAvatarCaller* mNonAvatarCaller;
	EVoiceControls mVoiceType;
	LLPanel* mAgentPanel;
	LLOutputMonitorCtrl* mSpeakingIndicator;
	bool mIsModeratorMutedVoice;

	/**
	 * Flag indicated that participants voice states should be initialized.
	 *
	 * It is used due to Avatar List has delayed refreshing after it content is changed.
	 * Real initializing is performed when Avatar List is first time refreshed.
	 *
	 * @see onAvatarListRefreshed()
	 * @see initParticipantsVoiceState()
	 */
	bool mInitParticipantsVoiceState;

	boost::signals2::connection mAvatarListRefreshConnection;


	/**
	 * time out speakers when they are not part of current session
	 */
	LLSpeakersDelayActionsStorage* mSpeakerDelayRemover;

	/**
	 * Stores reference to current voice channel.
	 *
	 * Is used to ignore voice channel changed callback for the same channel.
	 *
	 * @see sOnCurrentChannelChanged()
	 */
	static LLVoiceChannel* sCurrentVoiceChannel;

	/* virtual */
	LLTransientFloaterMgr::ETransientGroup getGroup() { return LLTransientFloaterMgr::IM; }

	boost::signals2::connection mVoiceChannelStateChangeConnection;
};


#endif //LL_LLCALLFLOATER_H


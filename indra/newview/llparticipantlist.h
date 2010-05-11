/** 
 * @file llparticipantlist.h
 * @brief LLParticipantList intended to update view(LLAvatarList) according to incoming messages
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

#include "llviewerprecompiledheaders.h"
#include "llevent.h"
#include "llpanelpeoplemenus.h"
#include "llavatarlist.h" // for LLAvatarItemRecentSpeakerComparator

class LLSpeakerMgr;
class LLAvatarList;
class LLUICtrl;
class LLAvalineUpdater;

class LLParticipantList
{
	LOG_CLASS(LLParticipantList);
	public:

		typedef boost::function<bool (const LLUUID& speaker_id)> validate_speaker_callback_t;

		LLParticipantList(LLSpeakerMgr* data_source, LLAvatarList* avatar_list, bool use_context_menu = true, bool exclude_agent = true, bool can_toggle_icons = true);
		~LLParticipantList();
		void setSpeakingIndicatorsVisible(BOOL visible);

		typedef enum e_participant_sort_oder {
			E_SORT_BY_NAME = 0,
			E_SORT_BY_RECENT_SPEAKERS = 1,
		} EParticipantSortOrder;

		/**
		 * Adds specified avatar ID to the existing list if it is not Agent's ID
		 *
		 * @param[in] avatar_id - Avatar UUID to be added into the list
		 */
		void addAvatarIDExceptAgent(const LLUUID& avatar_id);

		/**
		 * Set and sort Avatarlist by given order
		 */
		void setSortOrder(EParticipantSortOrder order = E_SORT_BY_NAME);
		EParticipantSortOrder getSortOrder();

		/**
		 * Refreshes the participant list if it's in sort by recent speaker order.
		 */
		void updateRecentSpeakersOrder();

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
		bool onSpeakerMuteEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);

		/**
		 * Sorts the Avatarlist by stored order
		 */
		void sort();

		//List of listeners implementing LLOldEvents::LLSimpleListener.
		//There is no way to handle all the events in one listener as LLSpeakerMgr registers listeners in such a way
		//that one listener can handle only one type of event
		class BaseSpeakerListner : public LLOldEvents::LLSimpleListener
		{
		public:
			BaseSpeakerListner(LLParticipantList& parent) : mParent(parent) {}
		protected:
			LLParticipantList& mParent;
		};

		class SpeakerAddListener : public BaseSpeakerListner
		{
		public:
			SpeakerAddListener(LLParticipantList& parent) : BaseSpeakerListner(parent) {}
			/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
		};

		class SpeakerRemoveListener : public BaseSpeakerListner
		{
		public:
			SpeakerRemoveListener(LLParticipantList& parent) : BaseSpeakerListner(parent) {}
			/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
		};

		class SpeakerClearListener : public BaseSpeakerListner
		{
		public:
			SpeakerClearListener(LLParticipantList& parent) : BaseSpeakerListner(parent) {}
			/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
		};

		class SpeakerModeratorUpdateListener : public BaseSpeakerListner
		{
		public:
			SpeakerModeratorUpdateListener(LLParticipantList& parent) : BaseSpeakerListner(parent) {}
			/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
		};
		
		class SpeakerMuteListener : public BaseSpeakerListner
		{
		public:
			SpeakerMuteListener(LLParticipantList& parent) : BaseSpeakerListner(parent) {}

			/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
		};

		/**
		 * Menu used in the participant list.
		 */
		class LLParticipantListMenu : public LLPanelPeopleMenus::ContextMenu
		{
		public:
			LLParticipantListMenu(LLParticipantList& parent):mParent(parent){};
			/*virtual*/ LLContextMenu* createMenu();
			/*virtual*/ void show(LLView* spawning_view, const uuid_vec_t& uuids, S32 x, S32 y);
		protected:
			LLParticipantList& mParent;
		private:
			bool enableContextMenuItem(const LLSD& userdata);
			bool enableModerateContextMenuItem(const LLSD& userdata);
			bool checkContextMenuItem(const LLSD& userdata);

			void sortParticipantList(const LLSD& userdata);
			void toggleAllowTextChat(const LLSD& userdata);
			void toggleMute(const LLSD& userdata, U32 flags);
			void toggleMuteText(const LLSD& userdata);
			void toggleMuteVoice(const LLSD& userdata);
		
			/**
			 * Return true if Agent is group moderator(and moderator of group call).
			 */
			bool isGroupModerator();

			// Voice moderation support
			/**
			 * Check whether specified by argument avatar is muted for group chat or not.
			 */
			bool isMuted(const LLUUID& avatar_id);

			/**
			 * Processes Voice moderation menu items.
			 *
			 * It calls either moderateVoiceParticipant() or moderateVoiceParticipant() depend on
			 * passed parameter.
			 *
			 * @param userdata can be "selected" or "others".
			 *
			 * @see moderateVoiceParticipant()
			 * @see moderateVoiceOtherParticipants()
			 */
			void moderateVoice(const LLSD& userdata);

			/**
			 * Mutes/Unmutes avatar for current group voice chat.
			 *
			 * It only marks avatar as muted for session and does not use local Agent's Block list.
			 * It does not mute Agent itself.
			 *
			 * @param[in] avatar_id UUID of avatar to be processed
			 * @param[in] unmute if true - specified avatar will be muted, otherwise - unmuted.
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
			 * @param[in] unmute if true - avatars will be muted, otherwise - unmuted.
			 *
			 * @see moderateVoiceParticipant()
			 */
			void moderateVoiceOtherParticipants(const LLUUID& excluded_avatar_id, bool unmute);
		};

		/**
		 * Comparator for comparing avatar items by last spoken time
		 */
		class LLAvatarItemRecentSpeakerComparator : public LLAvatarItemNameComparator, public LLRefCount
		{
			LOG_CLASS(LLAvatarItemRecentSpeakerComparator);
		  public:
			LLAvatarItemRecentSpeakerComparator(LLParticipantList& parent):mParent(parent){};
			virtual ~LLAvatarItemRecentSpeakerComparator() {};
		  protected:
			virtual bool doCompare(const LLAvatarListItem* avatar_item1, const LLAvatarListItem* avatar_item2) const;
		  private:
			LLParticipantList& mParent;
		};

	private:
		void onAvatarListDoubleClicked(LLUICtrl* ctrl);
		void onAvatarListRefreshed(LLUICtrl* ctrl, const LLSD& param);

		void onAvalineCallerFound(const LLUUID& participant_id);
		void onAvalineCallerRemoved(const LLUUID& participant_id);

		/**
		 * Adjusts passed participant to work properly.
		 *
		 * Adds SpeakerMuteListener to process moderation actions.
		 */
		void adjustParticipant(const LLUUID& speaker_id);

		LLSpeakerMgr*		mSpeakerMgr;
		LLAvatarList*		mAvatarList;

		std::set<LLUUID>	mModeratorList;
		std::set<LLUUID>	mModeratorToRemoveList;

		LLPointer<SpeakerAddListener>				mSpeakerAddListener;
		LLPointer<SpeakerRemoveListener>			mSpeakerRemoveListener;
		LLPointer<SpeakerClearListener>				mSpeakerClearListener;
		LLPointer<SpeakerModeratorUpdateListener>	mSpeakerModeratorListener;
		LLPointer<SpeakerMuteListener>				mSpeakerMuteListener;

		LLParticipantListMenu*    mParticipantListMenu;

		EParticipantSortOrder	mSortOrder;
		/*
		 * This field manages an adding  a new avatar_id in the mAvatarList
		 * If true, then agent_id wont  be added into mAvatarList
		 * Also by default this field is controlling a sort procedure, @c sort() 
		 */
		bool mExcludeAgent;

		// boost::connections
		boost::signals2::connection mAvatarListDoubleClickConnection;
		boost::signals2::connection mAvatarListRefreshConnection;
		boost::signals2::connection mAvatarListReturnConnection;
		boost::signals2::connection mAvatarListToggleIconsConnection;

		LLPointer<LLAvatarItemRecentSpeakerComparator> mSortByRecentSpeakers;
		validate_speaker_callback_t mValidateSpeakerCallback;
		LLAvalineUpdater* mAvalineUpdater;
};

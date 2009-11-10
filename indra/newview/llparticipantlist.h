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
#include "llimview.h"
#include "llavatarlist.h"

class LLSpeakerMgr;
class LLUICtrl;

class LLParticipantList
{
	LOG_CLASS(LLParticipantList);
	public:
		LLParticipantList(LLSpeakerMgr* data_source, LLAvatarList* avatar_list);
		~LLParticipantList();
		void setSpeakingIndicatorsVisible(BOOL visible){ mAvatarList->setSpeakingIndicatorsVisible(visible); };

		typedef enum e_participant_sort_oder {
			E_SORT_BY_NAME = 0,
		} EParticipantSortOrder;

		/**
		  * Set and sort Avatarlist by given order
		  */
		void setSortOrder(EParticipantSortOrder order = E_SORT_BY_NAME);

	protected:
		/**
		 * LLSpeakerMgr event handlers
		 */
		bool onAddItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
		bool onRemoveItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
		bool onClearListEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
		bool onModeratorUpdateEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);

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
		
		/**
		 * Menu used in the participant list.
		 */
		class LLParticipantListMenu : public LLPanelPeopleMenus::ContextMenu
		{
		public:
			LLParticipantListMenu(LLParticipantList& parent):mParent(parent){};
			/*virtual*/ LLContextMenu* createMenu();
		protected:
			LLParticipantList& mParent;
		private:
			bool enableContextMenuItem(const LLSD& userdata);
			bool checkContextMenuItem(const LLSD& userdata);

			void toggleAllowTextChat(const LLSD& userdata);
			void toggleMuteText(const LLSD& userdata);
		
		};

	private:
		void onAvatarListDoubleClicked(LLAvatarList* list);
		void onAvatarListRefreshed(LLUICtrl* ctrl, const LLSD& param);

		LLSpeakerMgr*		mSpeakerMgr;
		LLAvatarList*		mAvatarList;

		std::set<LLUUID>	mModeratorList;
		std::set<LLUUID>	mModeratorToRemoveList;

		LLPointer<SpeakerAddListener>				mSpeakerAddListener;
		LLPointer<SpeakerRemoveListener>			mSpeakerRemoveListener;
		LLPointer<SpeakerClearListener>				mSpeakerClearListener;
		LLPointer<SpeakerModeratorUpdateListener>	mSpeakerModeratorListener;

		LLParticipantListMenu*    mParticipantListMenu;

		EParticipantSortOrder	mSortOrder;
};

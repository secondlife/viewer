/** 
 * @file llimfloatercontainer.h
 * @brief Multifloater containing active IM sessions in separate tab container tabs
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

#ifndef LL_LLIMFLOATERCONTAINER_H
#define LL_LLIMFLOATERCONTAINER_H

#include <map>
#include <vector>

#include "llimview.h"
#include "llevents.h"
#include "llfloater.h"
#include "llmultifloater.h"
#include "llavatarpropertiesprocessor.h"
#include "llgroupmgr.h"
#include "lltrans.h"
#include "llconversationmodel.h"
#include "llconversationview.h"

class LLButton;
class LLLayoutPanel;
class LLLayoutStack;
class LLTabContainer;
class LLIMFloaterContainer;
class LLSpeaker;
class LLSpeakerMgr;

class LLIMFloaterContainer
	: public LLMultiFloater
	, public LLIMSessionObserver
{
public:
	LLIMFloaterContainer(const LLSD& seed);
	virtual ~LLIMFloaterContainer();
	
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void draw();
	/*virtual*/ void setVisible(BOOL visible);
	void onCloseFloater(LLUUID& id);

	/*virtual*/ void addFloater(LLFloater* floaterp, 
								BOOL select_added_floater, 
								LLTabContainer::eInsertionPoint insertion_point = LLTabContainer::END);
    
    void showConversation(const LLUUID& session_id);
    void setConvItemSelect(const LLUUID& session_id);
	/*virtual*/ void tabClose();

	static LLFloater* getCurrentVoiceFloater();
	static LLIMFloaterContainer* findInstance();
	static LLIMFloaterContainer* getInstance();

	static void onCurrentChannelChanged(const LLUUID& session_id);

	virtual void setMinimized(BOOL b);

	void collapseMessagesPane(bool collapse);
	
	// Callbacks
	static void idle(void* user_data);

	// LLIMSessionObserver observe triggers
	/*virtual*/ void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id);
    /*virtual*/ void sessionActivated(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id);
	/*virtual*/ void sessionVoiceOrIMStarted(const LLUUID& session_id);
	/*virtual*/ void sessionRemoved(const LLUUID& session_id);
	/*virtual*/ void sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id);

	LLConversationViewModel& getRootViewModel() { return mConversationViewModel; }
    LLUUID getSelectedSession() { return mSelectedSession; }
    void setSelectedSession(LLUUID sessionID) { mSelectedSession = sessionID; }

private:
	typedef std::map<LLUUID,LLFloater*> avatarID_panel_map_t;
	avatarID_panel_map_t mSessions;
	boost::signals2::connection mNewMessageConnection;

	/*virtual*/ void computeResizeLimits(S32& new_min_width, S32& new_min_height);

	void onNewMessageReceived(const LLSD& data);

	void onExpandCollapseButtonClicked();
	void processParticipantsStyleUpdate();

	void collapseConversationsPane(bool collapse);

	void updateState(bool collapse, S32 delta_width);

	void onAddButtonClicked();
	void onAvatarPicked(const uuid_vec_t& ids);

	BOOL isActionChecked(const LLSD& userdata);
	void onCustomAction (const LLSD& userdata);
	void setSortOrderSessions(const LLConversationFilter::ESortOrderType order);
	void setSortOrderParticipants(const LLConversationFilter::ESortOrderType order);
	void setSortOrder(const LLConversationSort& order);

    void getSelectedUUIDs(uuid_vec_t& selected_uuids);
    const LLConversationItem * getCurSelectedViewModelItem();
    void getParticipantUUIDs(uuid_vec_t& selected_uuids);
    void doToSelected(const LLSD& userdata);
    void doToSelectedConversation(const std::string& command, uuid_vec_t& selectedIDS);
    void doToParticipants(const std::string& item, uuid_vec_t& selectedIDS);
    void doToSelectedGroup(const LLSD& userdata);
    bool checkContextMenuItem(const LLSD& userdata);
    bool enableContextMenuItem(const LLSD& userdata);

	static void confirmMuteAllCallback(const LLSD& notification, const LLSD& response);
	bool enableModerateContextMenuItem(const std::string& userdata);
	LLSpeaker * getSpeakerOfSelectedParticipant(LLSpeakerMgr * speaker_managerp);
	LLSpeakerMgr * getSpeakerMgrForSelectedParticipant();
	bool isGroupModerator();
	bool isMuted(const LLUUID& avatar_id);
	void moderateVoice(const std::string& command, const LLUUID& userID);
	void moderateVoiceAllParticipants(bool unmute);
	void moderateVoiceParticipant(const LLUUID& avatar_id, bool unmute);
	void toggleAllowTextChat(const LLUUID& participant_uuid);
	void openNearbyChat();

	LLButton* mExpandCollapseBtn;
	LLLayoutPanel* mMessagesPane;
	LLLayoutPanel* mConversationsPane;
	LLLayoutStack* mConversationsStack;
	
	bool mInitialized;

	LLUUID mSelectedSession;

	// Conversation list implementation
public:
	bool removeConversationListItem(const LLUUID& uuid, bool change_focus = true);
	void addConversationListItem(const LLUUID& uuid, bool isWidgetSelected = false);
	void setTimeNow(const LLUUID& session_id, const LLUUID& participant_id);
	void setNearbyDistances();

private:
	LLConversationViewSession* createConversationItemWidget(LLConversationItem* item);
	LLConversationViewParticipant* createConversationViewParticipant(LLConversationItem* item);
	bool onConversationModelEvent(const LLSD& event);

	// Conversation list data
	LLPanel* mConversationsListPanel;	// This is the main widget we add conversation widget to
	conversations_items_map mConversationsItems;
	conversations_widgets_map mConversationsWidgets;
	LLConversationViewModel mConversationViewModel;
	LLFolderView* mConversationsRoot;
	LLEventStream mConversationsEventStream; 
};

#endif // LL_LLIMFLOATERCONTAINER_H

/** 
 * @file llfloaterimcontainer.h
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

#ifndef LL_LLFLOATERIMCONTAINER_H
#define LL_LLFLOATERIMCONTAINER_H

#include <map>
#include <vector>

#include "llimview.h"
#include "llevents.h"
#include "../llui/llfloater.h"
#include "../llui/llmultifloater.h"
#include "llavatarpropertiesprocessor.h"
#include "llgroupmgr.h"
#include "../llui/lltrans.h"
#include "llconversationmodel.h"
#include "llconversationview.h"

class LLButton;
class LLLayoutPanel;
class LLLayoutStack;
class LLTabContainer;
class LLFloaterIMContainer;
class LLSpeaker;
class LLSpeakerMgr;

class LLFloaterIMContainer
	: public LLMultiFloater
	, public LLIMSessionObserver
{
public:
	LLFloaterIMContainer(const LLSD& seed, const Params& params = getDefaultParams());
	virtual ~LLFloaterIMContainer();
	
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void draw();
	/*virtual*/ void setMinimized(BOOL b);
	/*virtual*/ void setVisible(BOOL visible);
	/*virtual*/ void setVisibleAndFrontmost(BOOL take_focus=TRUE, const LLSD& key = LLSD());
	/*virtual*/ void updateResizeLimits();
	/*virtual*/ void handleReshape(const LLRect& rect, bool by_user);

	void onCloseFloater(LLUUID& id);

	/*virtual*/ void addFloater(LLFloater* floaterp, 
								BOOL select_added_floater, 
								LLTabContainer::eInsertionPoint insertion_point = LLTabContainer::END);
	void returnFloaterToHost();
    void showConversation(const LLUUID& session_id);
    void selectConversation(const LLUUID& session_id);
	void selectNextConversationByID(const LLUUID& session_id);
    BOOL selectConversationPair(const LLUUID& session_id, bool select_widget, bool focus_floater = true);
    void clearAllFlashStates();
	bool selectAdjacentConversation(bool focus_selected);
    bool selectNextorPreviousConversation(bool select_next, bool focus_selected = true);
    void expandConversation();

	/*virtual*/ void tabClose();
	void showStub(bool visible);

	static LLFloater* getCurrentVoiceFloater();
	static LLFloaterIMContainer* findInstance();
	static LLFloaterIMContainer* getInstance();

	static void onCurrentChannelChanged(const LLUUID& session_id);

	void collapseMessagesPane(bool collapse);
	bool isMessagesPaneCollapsed();
	bool isConversationsPaneCollapsed();
	
	// Callbacks
	static void idle(void* user_data);

	// LLIMSessionObserver observe triggers
	/*virtual*/ void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id, BOOL has_offline_msg);
    /*virtual*/ void sessionActivated(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id);
	/*virtual*/ void sessionVoiceOrIMStarted(const LLUUID& session_id);
	/*virtual*/ void sessionRemoved(const LLUUID& session_id);
	/*virtual*/ void sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id);

	LLConversationViewModel& getRootViewModel() { return mConversationViewModel; }
    LLUUID getSelectedSession() { return mSelectedSession; }
    void setSelectedSession(LLUUID sessionID) { mSelectedSession = sessionID; }
	LLConversationItem* getSessionModel(const LLUUID& session_id) { return get_ptr_in_map(mConversationsItems,session_id); }
	LLConversationSort& getSortOrder() { return mConversationViewModel.getSorter(); }

	// Handling of lists of participants is public so to be common with llfloatersessiontab
	// *TODO : Find a better place for this.
    bool checkContextMenuItem(const std::string& item, uuid_vec_t& selectedIDS);
    bool enableContextMenuItem(const std::string& item, uuid_vec_t& selectedIDS);
    void doToParticipants(const std::string& item, uuid_vec_t& selectedIDS);

	void assignResizeLimits();
	virtual BOOL handleKeyHere(KEY key, MASK mask );
	/*virtual*/ void closeFloater(bool app_quitting = false);
    void closeAllConversations();
    void closeSelectedConversations(const uuid_vec_t& ids);
	/*virtual*/ BOOL isFrontmost();


private:
	typedef std::map<LLUUID,LLFloater*> avatarID_panel_map_t;
	avatarID_panel_map_t mSessions;
	boost::signals2::connection mNewMessageConnection;

	/*virtual*/ void computeResizeLimits(S32& new_min_width, S32& new_min_height) {}

	void onNewMessageReceived(const LLSD& data);

	void onExpandCollapseButtonClicked();
	void onStubCollapseButtonClicked();
	void processParticipantsStyleUpdate();
	void onSpeakButtonPressed();
	void onSpeakButtonReleased();
	/*virtual*/ void onClickCloseBtn(bool app_quitting = false);
	/*virtual*/ void closeHostedFloater();

	void collapseConversationsPane(bool collapse, bool save_is_allowed=true);

	void reshapeFloaterAndSetResizeLimits(bool collapse, S32 delta_width);

	void onAddButtonClicked();
	void onAvatarPicked(const uuid_vec_t& ids);

	BOOL isActionChecked(const LLSD& userdata);
	void onCustomAction (const LLSD& userdata);
	void setSortOrderSessions(const LLConversationFilter::ESortOrderType order);
	void setSortOrderParticipants(const LLConversationFilter::ESortOrderType order);
	void setSortOrder(const LLConversationSort& order);

    void getSelectedUUIDs(uuid_vec_t& selected_uuids, bool participant_uuids = true);
    const LLConversationItem * getCurSelectedViewModelItem();
    void getParticipantUUIDs(uuid_vec_t& selected_uuids);
    void doToSelected(const LLSD& userdata);
	bool checkContextMenuItem(const LLSD& userdata);
	bool enableContextMenuItem(const LLSD& userdata);
	bool visibleContextMenuItem(const LLSD& userdata);
    void doToSelectedConversation(const std::string& command, uuid_vec_t& selectedIDS);
    void doToSelectedGroup(const LLSD& userdata);

	static void confirmMuteAllCallback(const LLSD& notification, const LLSD& response);
	bool enableModerateContextMenuItem(const std::string& userdata);
	LLSpeaker * getSpeakerOfSelectedParticipant(LLSpeakerMgr * speaker_managerp);
	LLSpeakerMgr * getSpeakerMgrForSelectedParticipant();
	bool isGroupModerator();
	bool haveAbilityToBan();
	bool canBanSelectedMember(const LLUUID& participant_uuid);
	LLUUID getGroupUIIDForSelectedParticipant();
	bool isMuted(const LLUUID& avatar_id);
	void moderateVoice(const std::string& command, const LLUUID& userID);
	void moderateVoiceAllParticipants(bool unmute);
	void moderateVoiceParticipant(const LLUUID& avatar_id, bool unmute);
	void toggleAllowTextChat(const LLUUID& participant_uuid);
	void banSelectedMember(const LLUUID& participant_uuid);
	void openNearbyChat();
	bool isParticipantListExpanded();

	void idleUpdate(); // for convenience (self) from static idle

	LLButton* mExpandCollapseBtn;
	LLButton* mStubCollapseBtn;
    LLButton* mSpeakBtn;
	LLPanel* mStubPanel;
	LLTextBox* mStubTextBox;
	LLLayoutPanel* mMessagesPane;
	LLLayoutPanel* mConversationsPane;
	LLLayoutStack* mConversationsStack;
	
	bool mInitialized;
	bool mIsFirstLaunch;

	bool mIsFirstOpen;

	LLUUID mSelectedSession;
	std::string mGeneralTitle;

	// Conversation list implementation
public:
	bool removeConversationListItem(const LLUUID& uuid, bool change_focus = true);
	LLConversationItem* addConversationListItem(const LLUUID& uuid, bool isWidgetSelected = false);
	void setTimeNow(const LLUUID& session_id, const LLUUID& participant_id);
	void setNearbyDistances();
	void reSelectConversation();
	void updateSpeakBtnState();
	static bool isConversationLoggingAllowed();
	void flashConversationItemWidget(const LLUUID& session_id, bool is_flashes);
	void highlightConversationItemWidget(const LLUUID& session_id, bool is_highlighted);
	bool isScrolledOutOfSight(LLConversationViewSession* conversation_item_widget);
	boost::signals2::connection mMicroChangedSignal;
	S32 getConversationListItemSize() { return mConversationsWidgets.size(); }
	typedef std::list<LLFloater*> floater_list_t;
	void getDetachedConversationFloaters(floater_list_t& floaters);

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

	LLTimer mParticipantRefreshTimer;
};

#endif // LL_LLFLOATERIMCONTAINER_H

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
    void setConvItemSelect(const LLUUID& session_id);
	/*virtual*/ void tabClose();

	static LLFloater* getCurrentVoiceFloater();

	static LLIMFloaterContainer* findInstance();

	static LLIMFloaterContainer* getInstance();

	virtual void setMinimized(BOOL b);

	void collapseMessagesPane(bool collapse);
	
	// Callbacks
	static void idle(void* user_data);

	// LLIMSessionObserver observe triggers
	/*virtual*/ void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id);
	/*virtual*/ void sessionVoiceOrIMStarted(const LLUUID& session_id);
	/*virtual*/ void sessionRemoved(const LLUUID& session_id);
	/*virtual*/ void sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id);

	LLConversationViewModel& getRootViewModel() { return mConversationViewModel; }

private:
	typedef std::map<LLUUID,LLFloater*> avatarID_panel_map_t;
	avatarID_panel_map_t mSessions;
	boost::signals2::connection mNewMessageConnection;

	/*virtual*/ void computeResizeLimits(S32& new_min_width, S32& new_min_height);

	void onNewMessageReceived(const LLSD& data);

	void onExpandCollapseButtonClicked();

	void collapseConversationsPane(bool collapse);

	void updateState(bool collapse, S32 delta_width);
	void repositioningWidgets();

	void onAddButtonClicked();
	void onAvatarPicked(const uuid_vec_t& ids);

	BOOL isActionChecked(const LLSD& userdata);
	void onCustomAction (const LLSD& userdata);
	void setSortOrderSessions(const LLConversationFilter::ESortOrderType order);
	void setSortOrderParticipants(const LLConversationFilter::ESortOrderType order);
	void setSortOrder(const LLConversationSort& order);

	LLButton* mExpandCollapseBtn;
	LLLayoutPanel* mMessagesPane;
	LLLayoutPanel* mConversationsPane;
	LLLayoutStack* mConversationsStack;
	
	bool mInitialized;

	LLUUID mSelectedSession;

	// Conversation list implementation
public:
	void removeConversationListItem(const LLUUID& uuid, bool change_focus = true);
	void addConversationListItem(const LLUUID& uuid);

private:
	LLConversationViewSession* createConversationItemWidget(LLConversationItem* item);
	LLConversationViewParticipant* createConversationViewParticipant(LLConversationItem* item);

	// Conversation list data
	LLPanel* mConversationsListPanel;	// This is the main widget we add conversation widget to
	conversations_items_map mConversationsItems;
	conversations_widgets_map mConversationsWidgets;
	LLConversationViewModel mConversationViewModel;
	LLFolderView* mConversationsRoot;
};

#endif // LL_LLIMFLOATERCONTAINER_H

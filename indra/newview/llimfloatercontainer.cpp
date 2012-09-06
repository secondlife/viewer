/** 
 * @file llimfloatercontainer.cpp
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


#include "llviewerprecompiledheaders.h"

#include "llimfloater.h"
#include "llimfloatercontainer.h"

#include "llfloaterreg.h"
#include "lllayoutstack.h"
#include "llnearbychat.h"

#include "llagent.h"
#include "llavataractions.h"
#include "llavatariconctrl.h"
#include "llavatarnamecache.h"
#include "llgroupiconctrl.h"
#include "llfloateravatarpicker.h"
#include "llfloaterpreference.h"
#include "llimview.h"
#include "lltransientfloatermgr.h"
#include "llviewercontrol.h"
#include "llconversationview.h"

//
// LLIMFloaterContainer
//
LLIMFloaterContainer::LLIMFloaterContainer(const LLSD& seed)
:	LLMultiFloater(seed),
	mExpandCollapseBtn(NULL),
	mConversationsRoot(NULL)
{
	mCommitCallbackRegistrar.add("IMFloaterContainer.Action", boost::bind(&LLIMFloaterContainer::onCustomAction,  this, _2));

	// Firstly add our self to IMSession observers, so we catch session events
    LLIMMgr::getInstance()->addSessionObserver(this);

	mAutoResize = FALSE;
	LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::IM, this);
}

LLIMFloaterContainer::~LLIMFloaterContainer()
{
	mNewMessageConnection.disconnect();
	LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::IM, this);

	gSavedPerAccountSettings.setBOOL("ConversationsListPaneCollapsed", mConversationsPane->isCollapsed());
	gSavedPerAccountSettings.setBOOL("ConversationsMessagePaneCollapsed", mMessagesPane->isCollapsed());

	if (!LLSingleton<LLIMMgr>::destroyed())
	{
		LLIMMgr::getInstance()->removeSessionObserver(this);
	}
}

void LLIMFloaterContainer::sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id)
{
	LLIMFloater::addToIMContainer(session_id);
	addConversationListItem(session_id);
}

void LLIMFloaterContainer::sessionVoiceOrIMStarted(const LLUUID& session_id)
{
	LLIMFloater::addToIMContainer(session_id);
	addConversationListItem(session_id);
}

void LLIMFloaterContainer::sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id)
{
	removeConversationListItem(old_session_id);
	addConversationListItem(new_session_id);
}

void LLIMFloaterContainer::sessionRemoved(const LLUUID& session_id)
{
	removeConversationListItem(session_id);
}

BOOL LLIMFloaterContainer::postBuild()
{
	mNewMessageConnection = LLIMModel::instance().mNewMsgSignal.connect(boost::bind(&LLIMFloaterContainer::onNewMessageReceived, this, _1));
	// Do not call base postBuild to not connect to mCloseSignal to not close all floaters via Close button
	// mTabContainer will be initialized in LLMultiFloater::addChild()
	
	setTabContainer(getChild<LLTabContainer>("im_box_tab_container"));

	mConversationsStack = getChild<LLLayoutStack>("conversations_stack");
	mConversationsPane = getChild<LLLayoutPanel>("conversations_layout_panel");
	mMessagesPane = getChild<LLLayoutPanel>("messages_layout_panel");
	
	mConversationsListPanel = getChild<LLPanel>("conversations_list_panel");

	// CHUI-98 : View Model for conversations
	LLConversationItem* base_item = new LLConversationItem(getRootViewModel());
	LLFolderView::Params p;
	p.view_model = &mConversationViewModel;
	p.parent_panel = mConversationsListPanel;
	p.rect = mConversationsListPanel->getLocalRect();
	p.follows.flags = FOLLOWS_ALL;
	p.listener = base_item;
	p.root = NULL;

	mConversationsRoot = LLUICtrlFactory::create<LLFolderView>(p);
	mConversationsListPanel->addChild(mConversationsRoot);

	addConversationListItem(LLUUID()); // manually add nearby chat

	mExpandCollapseBtn = getChild<LLButton>("expand_collapse_btn");
	mExpandCollapseBtn->setClickedCallback(boost::bind(&LLIMFloaterContainer::onExpandCollapseButtonClicked, this));

	childSetAction("add_btn", boost::bind(&LLIMFloaterContainer::onAddButtonClicked, this));

	collapseMessagesPane(gSavedPerAccountSettings.getBOOL("ConversationsMessagePaneCollapsed"));
	collapseConversationsPane(gSavedPerAccountSettings.getBOOL("ConversationsListPaneCollapsed"));
	LLAvatarNameCache::addUseDisplayNamesCallback(
			boost::bind(&LLIMConversation::processChatHistoryStyleUpdate));

	return TRUE;
}

void LLIMFloaterContainer::onOpen(const LLSD& key)
{
	LLMultiFloater::onOpen(key);
}

// virtual
void LLIMFloaterContainer::addFloater(LLFloater* floaterp, 
									BOOL select_added_floater, 
									LLTabContainer::eInsertionPoint insertion_point)
{
	if(!floaterp) return;

	// already here
	if (floaterp->getHost() == this)
	{
		openFloater(floaterp->getKey());
		return;
	}
	
	// Make sure the message panel is open when adding a floater or it stays mysteriously hidden
	collapseMessagesPane(false);

	// Add the floater
	LLMultiFloater::addFloater(floaterp, select_added_floater, insertion_point);

	LLUUID session_id = floaterp->getKey();
	
	LLIconCtrl* icon = 0;

	if(gAgent.isInGroup(session_id, TRUE))
	{
		LLGroupIconCtrl::Params icon_params;
		icon_params.group_id = session_id;
		icon = LLUICtrlFactory::instance().create<LLGroupIconCtrl>(icon_params);

		mSessions[session_id] = floaterp;
		floaterp->mCloseSignal.connect(boost::bind(&LLIMFloaterContainer::onCloseFloater, this, session_id));
	}
	else
	{
		LLUUID avatar_id = LLIMModel::getInstance()->getOtherParticipantID(session_id);

		LLAvatarIconCtrl::Params icon_params;
		icon_params.avatar_id = avatar_id;
		icon = LLUICtrlFactory::instance().create<LLAvatarIconCtrl>(icon_params);

		mSessions[session_id] = floaterp;
		floaterp->mCloseSignal.connect(boost::bind(&LLIMFloaterContainer::onCloseFloater, this, session_id));
	}

	// forced resize of the floater
	LLRect wrapper_rect = this->mTabContainer->getLocalRect();
	floaterp->setRect(wrapper_rect);

	mTabContainer->setTabImage(floaterp, icon);
}


void LLIMFloaterContainer::onCloseFloater(LLUUID& id)
{
	mSessions.erase(id);
	setFocus(TRUE);
}

// virtual
void LLIMFloaterContainer::computeResizeLimits(S32& new_min_width, S32& new_min_height)
{
	bool is_left_pane_expanded = !mConversationsPane->isCollapsed();
	bool is_right_pane_expanded = !mMessagesPane->isCollapsed();

	S32 conversations_pane_min_dim = mConversationsPane->getMinDim();

	if (is_right_pane_expanded)
	{
		S32 conversations_pane_width =
				(is_left_pane_expanded ? gSavedPerAccountSettings.getS32("ConversationsListPaneWidth") : conversations_pane_min_dim);

		// possibly increase minimum size constraint due to children's minimums.
		for (S32 tab_idx = 0; tab_idx < mTabContainer->getTabCount(); ++tab_idx)
		{
			LLFloater* floaterp = dynamic_cast<LLFloater*>(mTabContainer->getPanelByIndex(tab_idx));
			if (floaterp)
			{
				new_min_width = llmax(new_min_width,
						floaterp->getMinWidth() + conversations_pane_width + LLPANEL_BORDER_WIDTH * 2);
				new_min_height = llmax(new_min_height, floaterp->getMinHeight());
			}
		}
	}
	else
	{
		new_min_width = conversations_pane_min_dim;
	}
}

void LLIMFloaterContainer::onNewMessageReceived(const LLSD& data)
{
	LLUUID session_id = data["session_id"].asUUID();
	LLFloater* floaterp = get_ptr_in_map(mSessions, session_id);
	LLFloater* current_floater = LLMultiFloater::getActiveFloater();

	if(floaterp && current_floater && floaterp != current_floater)
	{
		if(LLMultiFloater::isFloaterFlashing(floaterp))
			LLMultiFloater::setFloaterFlashing(floaterp, FALSE);
		LLMultiFloater::setFloaterFlashing(floaterp, TRUE);
	}
}

void LLIMFloaterContainer::onExpandCollapseButtonClicked()
{
	if (mConversationsPane->isCollapsed() && mMessagesPane->isCollapsed()
			&& gSavedPerAccountSettings.getBOOL("ConversationsExpandMessagePaneFirst"))
	{
		// Expand the messages pane from ultra minimized state
		// if it was collapsed last in order.
		collapseMessagesPane(false);
	}
	else
	{
		collapseConversationsPane(!mConversationsPane->isCollapsed());
	}
}

LLIMFloaterContainer* LLIMFloaterContainer::findInstance()
{
	return LLFloaterReg::findTypedInstance<LLIMFloaterContainer>("im_container");
}

LLIMFloaterContainer* LLIMFloaterContainer::getInstance()
{
	return LLFloaterReg::getTypedInstance<LLIMFloaterContainer>("im_container");
}

void LLIMFloaterContainer::setMinimized(BOOL b)
{
	if (isMinimized() == b) return;
	
	LLMultiFloater::setMinimized(b);

	if (isMinimized()) return;

	if (getActiveFloater())
	{
		getActiveFloater()->setVisible(TRUE);
	}
}

void LLIMFloaterContainer::draw()
{
	// CHUI Notes
	// Currently, the model is not responsible for creating the view which is a good thing. This means that
	// the model could change substantially and the view could decide to echo only a portion of this model.
	// Consequently, the participant views need to be created either by the session view or by the container panel.
	// For the moment, we create them here (which makes for complicated code...) to conform to the pattern
	// implemented in llinventorypanel.cpp (see LLInventoryPanel::buildNewViews()).
	// The best however would be to have an observer on the model so that we would not pool on each draw to know
	// if the view needs refresh. The current implementation (testing for change on draw) is less
	// efficient perf wise than a listener/observer scheme. We will implement that shortly.
	
	// On each session in mConversationsItems
	for (conversations_items_map::iterator it_session = mConversationsItems.begin(); it_session != mConversationsItems.end(); it_session++)
	{
		// Get the current session descriptors
		LLConversationItem* session_model = it_session->second;
		LLUUID session_id = it_session->first;
		LLConversationViewSession* session_view = dynamic_cast<LLConversationViewSession*>(mConversationsWidgets[session_id]);
		// If the session model has been changed, refresh the corresponding view
		if (session_model->needsRefresh())
		{
			session_view->refresh();
		}
		// Iterate through each model participant child
		LLFolderViewModelItemCommon::child_list_t::const_iterator current_participant_model = session_model->getChildrenBegin();
		LLFolderViewModelItemCommon::child_list_t::const_iterator end_participant_model = session_model->getChildrenEnd();
		while (current_participant_model != end_participant_model)
		{
			LLConversationItem* participant_model = dynamic_cast<LLConversationItem*>(*current_participant_model);
			LLUUID participant_id = participant_model->getUUID();
			LLConversationViewParticipant* participant_view = session_view->findParticipant(participant_id);
			// Is there a corresponding view? If not create it
			if (!participant_view)
			{
				participant_view = createConversationViewParticipant(participant_model);
				participant_view->addToFolder(session_view);
				mConversationsListPanel->addChild(participant_view);
				participant_view->setVisible(TRUE);
			}
			else
			// Else, see if it needs refresh
			{
				if (participant_model->needsRefresh())
				{
					participant_view->refresh();
				}
			}
			// Reset the need for refresh
			session_model->resetRefresh();
			// Next participant
			current_participant_model++;
		}
	}
	
	if (mTabContainer->getTabCount() == 0)
	{
		// Do not close the container when every conversation is torn off because the user
		// still needs the conversation list. Simply collapse the message pane in that case.
		collapseMessagesPane(true);
	}
	LLFloater::draw();

	repositioningWidgets();
}

void LLIMFloaterContainer::tabClose()
{
	if (mTabContainer->getTabCount() == 0)
	{
		// Do not close the container when every conversation is torn off because the user
		// still needs the conversation list. Simply collapse the message pane in that case.
		collapseMessagesPane(true);
	}
}

void LLIMFloaterContainer::setVisible(BOOL visible)
{
	if (visible)
	{
		// Make sure we have the Nearby Chat present when showing the conversation container
		LLIMConversation* nearby_chat = LLFloaterReg::findTypedInstance<LLIMConversation>("nearby_chat");
		if (nearby_chat == NULL)
		{
			// If not found, force the creation of the nearby chat conversation panel
			// *TODO: find a way to move this to XML as a default panel or something like that
			LLSD name("nearby_chat");
			LLFloaterReg::toggleInstanceOrBringToFront(name);
		}
	}

	// We need to show/hide all the associated conversations that have been torn off
	// (and therefore, are not longer managed by the multifloater),
	// so that they show/hide with the conversations manager.
	conversations_widgets_map::iterator widget_it = mConversationsWidgets.begin();
	for (;widget_it != mConversationsWidgets.end(); ++widget_it)
	{
		LLConversationViewSession* widget = dynamic_cast<LLConversationViewSession*>(widget_it->second);
		widget->setVisibleIfDetached(visible);
	}
	
	// Now, do the normal multifloater show/hide
	LLMultiFloater::setVisible(visible);
	
}

void LLIMFloaterContainer::collapseMessagesPane(bool collapse)
{
	if (mMessagesPane->isCollapsed() == collapse)
	{
		return;
	}

	if (collapse)
	{
		// Save the messages pane width before collapsing it.
		gSavedPerAccountSettings.setS32("ConversationsMessagePaneWidth", mMessagesPane->getRect().getWidth());

		// Save the order in which the panels are closed to reverse user's last action.
		gSavedPerAccountSettings.setBOOL("ConversationsExpandMessagePaneFirst", mConversationsPane->isCollapsed());
	}

	// Show/hide the messages pane.
	mConversationsStack->collapsePanel(mMessagesPane, collapse);

	updateState(collapse, gSavedPerAccountSettings.getS32("ConversationsMessagePaneWidth"));
}

void LLIMFloaterContainer::collapseConversationsPane(bool collapse)
{
	if (mConversationsPane->isCollapsed() == collapse)
	{
		return;
	}

	LLView* button_panel = getChild<LLView>("conversations_pane_buttons_expanded");
	button_panel->setVisible(!collapse);
	mExpandCollapseBtn->setImageOverlay(getString(collapse ? "expand_icon" : "collapse_icon"));

	if (collapse)
	{
		// Save the conversations pane width before collapsing it.
		gSavedPerAccountSettings.setS32("ConversationsListPaneWidth", mConversationsPane->getRect().getWidth());

		// Save the order in which the panels are closed to reverse user's last action.
		gSavedPerAccountSettings.setBOOL("ConversationsExpandMessagePaneFirst", !mMessagesPane->isCollapsed());
	}

	mConversationsStack->collapsePanel(mConversationsPane, collapse);

	S32 collapsed_width = mConversationsPane->getMinDim();
	updateState(collapse, gSavedPerAccountSettings.getS32("ConversationsListPaneWidth") - collapsed_width);
}

void LLIMFloaterContainer::updateState(bool collapse, S32 delta_width)
{
	LLRect floater_rect = getRect();
	floater_rect.mRight += ((collapse ? -1 : 1) * delta_width);

	// Set by_user = true so that reshaped rect is saved in user_settings.
	setShape(floater_rect, true);

	updateResizeLimits();

	bool is_left_pane_expanded = !mConversationsPane->isCollapsed();
	bool is_right_pane_expanded = !mMessagesPane->isCollapsed();

	setCanResize(is_left_pane_expanded || is_right_pane_expanded);
	setCanMinimize(is_left_pane_expanded || is_right_pane_expanded);

    // restore floater's resize limits (prevent collapse when left panel is expanded)
	if (is_left_pane_expanded && !is_right_pane_expanded)
	{
		S32 expanded_min_size = mConversationsPane->getExpandedMinDim();
        setResizeLimits(expanded_min_size, expanded_min_size);
	}

}

void LLIMFloaterContainer::onAddButtonClicked()
{
    LLView * button = findChild<LLView>("conversations_pane_buttons_expanded")->findChild<LLButton>("add_btn");
    LLFloater* root_floater = gFloaterView->getParentFloater(this);
    LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(boost::bind(&LLIMFloaterContainer::onAvatarPicked, this, _1), TRUE, TRUE, TRUE, root_floater->getName(), button);
    
    if (picker && root_floater)
    {
        root_floater->addDependentFloater(picker);
    }
}

void LLIMFloaterContainer::onAvatarPicked(const uuid_vec_t& ids)
{
    if (ids.size() == 1)
    {
        LLAvatarActions::startIM(ids.back());
    }
    else
    {
        LLAvatarActions::startConference(ids);
    }
}

void LLIMFloaterContainer::onCustomAction(const LLSD& userdata)
{
	std::string command = userdata.asString();

	if ("chat_preferences" == command)
	{
		LLFloaterPreference* floater_prefs = LLFloaterReg::showTypedInstance<LLFloaterPreference>("preferences");
		if (floater_prefs)
		{
			LLTabContainer* tab_container = floater_prefs->getChild<LLTabContainer>("pref core");
			LLPanel* chat_panel = tab_container->getPanelByName("chat");
			if (tab_container && chat_panel)
			{
				tab_container->selectTabPanel(chat_panel);
			}
		}
	}
}

void LLIMFloaterContainer::repositioningWidgets()
{
	LLRect panel_rect = mConversationsListPanel->getRect();
	S32 item_height = 16;
	int index = 0;
	for (conversations_widgets_map::iterator widget_it = mConversationsWidgets.begin();
		 widget_it != mConversationsWidgets.end();
		 widget_it++)
	{
		LLFolderViewFolder* widget = dynamic_cast<LLFolderViewFolder*>(widget_it->second);
		widget->setVisible(TRUE);
		widget->setRect(LLRect(0,
							   panel_rect.getHeight() - item_height*index,
							   panel_rect.getWidth(),
							   panel_rect.getHeight() - item_height*(index+1)));
		index++;
		// Reposition the children as well
		// Merov : This is highly suspiscious but gets the debug hack to work. This needs to be revised though.
		if (widget->getItemsCount() != 0)
		{
			BOOL is_open = widget->isOpen();
			widget->setOpen(TRUE);
			LLFolderViewFolder::items_t::const_iterator current = widget->getItemsBegin();
			LLFolderViewFolder::items_t::const_iterator end = widget->getItemsEnd();
			while (current != end)
			{
				LLFolderViewItem* item = (*current);
				item->setVisible(TRUE);
				item->setRect(LLRect(0,
									   panel_rect.getHeight() - item_height*index,
									   panel_rect.getWidth(),
									   panel_rect.getHeight() - item_height*(index+1)));
				index++;
				current++;
			}
			widget->setOpen(is_open);
		}
	}
}

void LLIMFloaterContainer::addConversationListItem(const LLUUID& uuid)
{
	bool is_nearby_chat = uuid.isNull();
	
	std::string display_name = is_nearby_chat ? LLTrans::getString("NearbyChatTitle") : LLIMModel::instance().getName(uuid);

	// Check if the item is not already in the list, exit if it is and has the same name and uuid (nothing to do)
	// Note: this happens often, when reattaching a torn off conversation for instance
	conversations_items_map::iterator item_it = mConversationsItems.find(uuid);
	if (item_it != mConversationsItems.end())
	{
		return;
	}

	// Remove the conversation item that might exist already: it'll be recreated anew further down anyway
	// and nothing wrong will happen removing it if it doesn't exist
	removeConversationListItem(uuid,false);

	// Create a conversation session model
	LLConversationItem* item = NULL;
	LLSpeakerMgr* speaker_manager = (is_nearby_chat ? (LLSpeakerMgr*)(LLLocalSpeakerMgr::getInstance()) : LLIMModel::getInstance()->getSpeakerManager(uuid));
	if (speaker_manager)
	{
		item = new LLParticipantList(speaker_manager, NULL, getRootViewModel(), true, false);
	}
	if (!item)
	{
		llinfos << "Merov debug : Couldn't create conversation session item : " << display_name << llendl;
		return;
	}
	// *TODO: Should we flag LLConversationItemSession with a mIsNearbyChat?
	item->renameItem(display_name);
	
	mConversationsItems[uuid] = item;

	// Create a widget from it
	LLConversationViewSession* widget = createConversationItemWidget(item);
	mConversationsWidgets[uuid] = widget;

	// Add a new conversation widget to the root folder of the folder view
	widget->addToFolder(mConversationsRoot);

	// Add it to the UI
	mConversationsListPanel->addChild(widget);
	widget->setVisible(TRUE);
	
	// Create the participants widgets now
	// Note: usually, we do not get an updated avatar list at that point
	LLFolderViewModelItemCommon::child_list_t::const_iterator current_participant_model = item->getChildrenBegin();
	LLFolderViewModelItemCommon::child_list_t::const_iterator end_participant_model = item->getChildrenEnd();
	llinfos << "Merov debug : create participant, children size = " << item->getChildrenCount() << llendl;
	while (current_participant_model != end_participant_model)
	{
		LLConversationItem* participant_model = dynamic_cast<LLConversationItem*>(*current_participant_model);
		LLConversationViewParticipant* participant_view = createConversationViewParticipant(participant_model);
		participant_view->addToFolder(widget);
		mConversationsListPanel->addChild(participant_view);
		participant_view->setVisible(TRUE);
		current_participant_model++;
	}

	repositioningWidgets();
	
	return;
}

void LLIMFloaterContainer::removeConversationListItem(const LLUUID& uuid, bool change_focus)
{
	// Delete the widget and the associated conversation item
	// Note : since the mConversationsItems is also the listener to the widget, deleting 
	// the widget will also delete its listener
	conversations_widgets_map::iterator widget_it = mConversationsWidgets.find(uuid);
	if (widget_it != mConversationsWidgets.end())
	{
		LLFolderViewItem* widget = widget_it->second;
		widget->destroyView();
	}
	
	// Suppress the conversation items and widgets from their respective maps
	mConversationsItems.erase(uuid);
	mConversationsWidgets.erase(uuid);

	repositioningWidgets();
	
	// Don't let the focus fall IW, select and refocus on the first conversation in the list
	if (change_focus)
	{
		setFocus(TRUE);
		conversations_widgets_map::iterator widget_it = mConversationsWidgets.begin();
		if (widget_it != mConversationsWidgets.end())
		{
			LLFolderViewItem* widget = widget_it->second;
			widget->selectItem();
		}
	}
}

LLConversationViewSession* LLIMFloaterContainer::createConversationItemWidget(LLConversationItem* item)
{
	LLConversationViewSession::Params params;
	
	params.name = item->getDisplayName();
	//params.icon = bridge->getIcon();
	//params.icon_open = bridge->getOpenIcon();
	//params.creation_date = bridge->getCreationDate();
	params.root = mConversationsRoot;
	params.listener = item;
	params.rect = LLRect (0, 0, 0, 0);
	params.tool_tip = params.name;
	params.container = this;
	
	return LLUICtrlFactory::create<LLConversationViewSession>(params);
}

LLConversationViewParticipant* LLIMFloaterContainer::createConversationViewParticipant(LLConversationItem* item)
{
	LLConversationViewParticipant::Params params;
	
	params.name = item->getDisplayName();
	//params.icon = bridge->getIcon();
	//params.icon_open = bridge->getOpenIcon();
	//params.creation_date = bridge->getCreationDate();
	params.root = mConversationsRoot;
	params.listener = item;
	params.rect = LLRect (0, 0, 0, 0);
	params.tool_tip = params.name;
	params.participant_id = item->getUUID();
	
	return LLUICtrlFactory::create<LLConversationViewParticipant>(params);
}

// EOF

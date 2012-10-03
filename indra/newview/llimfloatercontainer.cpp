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
#include "llcallbacklist.h"
#include "llgroupactions.h"
#include "llgroupiconctrl.h"
#include "llfloateravatarpicker.h"
#include "llfloaterpreference.h"
#include "llimview.h"
#include "lltransientfloatermgr.h"
#include "llviewercontrol.h"
#include "llconversationview.h"
#include "llcallbacklist.h"
#include "llworld.h"

#include "llsdserialize.h"
//
// LLIMFloaterContainer
//
LLIMFloaterContainer::LLIMFloaterContainer(const LLSD& seed)
:	LLMultiFloater(seed),
	mExpandCollapseBtn(NULL),
	mConversationsRoot(NULL),
	mStream("ConversationsEvents"),
	mInitialized(false)
{
    mEnableCallbackRegistrar.add("IMFloaterContainer.Check", boost::bind(&LLIMFloaterContainer::isActionChecked, this, _2));
	mCommitCallbackRegistrar.add("IMFloaterContainer.Action", boost::bind(&LLIMFloaterContainer::onCustomAction,  this, _2));
	
    mEnableCallbackRegistrar.add("Avatar.CheckItem",  boost::bind(&LLIMFloaterContainer::checkContextMenuItem,	this, _2));
    mEnableCallbackRegistrar.add("Avatar.EnableItem", boost::bind(&LLIMFloaterContainer::enableContextMenuItem,	this, _2));
    mCommitCallbackRegistrar.add("Avatar.DoToSelected", boost::bind(&LLIMFloaterContainer::doToSelected, this, _2));
    
    mCommitCallbackRegistrar.add("Group.DoToSelected", boost::bind(&LLIMFloaterContainer::doToSelectedGroup, this, _2));

	// Firstly add our self to IMSession observers, so we catch session events
    LLIMMgr::getInstance()->addSessionObserver(this);

	mAutoResize = FALSE;
	LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::IM, this);
}

LLIMFloaterContainer::~LLIMFloaterContainer()
{
	gIdleCallbacks.deleteFunction(idle, this);

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
	LLIMFloater::addToHost(session_id, true);
	addConversationListItem(session_id);
}

void LLIMFloaterContainer::sessionVoiceOrIMStarted(const LLUUID& session_id)
{
	LLIMFloater::addToHost(session_id, true);
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

// static
void LLIMFloaterContainer::onCurrentChannelChanged(const LLUUID& session_id)
{
    if (session_id != LLUUID::null)
    {
    	LLIMFloater::show(session_id);
    }
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

	// Create the root model and view for all conversation sessions
	LLConversationItem* base_item = new LLConversationItem(getRootViewModel());

    LLFolderView::Params p(LLUICtrlFactory::getDefaultParams<LLFolderView>());
    p.name = getName();
    p.title = getLabel();
    p.rect = LLRect(0, 0, getRect().getWidth(), 0);
    p.parent_panel = mConversationsListPanel;
    p.tool_tip = p.name;
    p.listener = base_item;
    p.view_model = &mConversationViewModel;
    p.root = NULL;
    p.use_ellipses = true;
    p.options_menu = "menu_conversation.xml";
	mConversationsRoot = LLUICtrlFactory::create<LLFolderView>(p);
    mConversationsRoot->setCallbackRegistrar(&mCommitCallbackRegistrar);

	// Add listener to conversation model events
	mStream.listen("ConversationsRefresh", boost::bind(&LLIMFloaterContainer::onConversationModelEvent, this, _1));

	// a scroller for folder view
	LLRect scroller_view_rect = mConversationsListPanel->getRect();
	scroller_view_rect.translate(-scroller_view_rect.mLeft, -scroller_view_rect.mBottom);
	LLScrollContainer::Params scroller_params(LLUICtrlFactory::getDefaultParams<LLFolderViewScrollContainer>());
	scroller_params.rect(scroller_view_rect);

	LLScrollContainer* scroller = LLUICtrlFactory::create<LLFolderViewScrollContainer>(scroller_params);
	scroller->setFollowsAll();
	mConversationsListPanel->addChild(scroller);
	scroller->addChild(mConversationsRoot);
	mConversationsRoot->setScrollContainer(scroller);
	mConversationsRoot->setFollowsAll();
	mConversationsRoot->addChild(mConversationsRoot->mStatusTextBox);

	addConversationListItem(LLUUID()); // manually add nearby chat

	mExpandCollapseBtn = getChild<LLButton>("expand_collapse_btn");
	mExpandCollapseBtn->setClickedCallback(boost::bind(&LLIMFloaterContainer::onExpandCollapseButtonClicked, this));

	childSetAction("add_btn", boost::bind(&LLIMFloaterContainer::onAddButtonClicked, this));

	collapseMessagesPane(gSavedPerAccountSettings.getBOOL("ConversationsMessagePaneCollapsed"));
	collapseConversationsPane(gSavedPerAccountSettings.getBOOL("ConversationsListPaneCollapsed"));
	LLAvatarNameCache::addUseDisplayNamesCallback(boost::bind(&LLIMConversation::processChatHistoryStyleUpdate));

	if (! mMessagesPane->isCollapsed())
	{
		S32 list_width = gSavedPerAccountSettings.getS32("ConversationsListPaneWidth");
		LLRect list_size = mConversationsPane->getRect();
        S32 left_pad = mConversationsListPanel->getRect().mLeft;
		list_size.mRight = list_size.mLeft + list_width - left_pad;

        mConversationsPane->handleReshape(list_size, TRUE);
	}

	// Init the sort order now that the root had been created
	setSortOrder(LLConversationSort(gSavedSettings.getU32("ConversationSortOrder")));
	
	mInitialized = true;

	// Add callbacks:
	// We'll take care of view updates on idle
	gIdleCallbacks.addFunction(idle, this);
	// When display name option change, we need to reload all participant names
	LLAvatarNameCache::addUseDisplayNamesCallback(boost::bind(&LLIMFloaterContainer::processParticipantsStyleUpdate, this));

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
	{   LLUUID avatar_id = session_id.notNull()?
		    LLIMModel::getInstance()->getOtherParticipantID(session_id) : LLUUID();

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

// Update all participants in the conversation lists
void LLIMFloaterContainer::processParticipantsStyleUpdate()
{
	// On each session in mConversationsItems
	for (conversations_items_map::iterator it_session = mConversationsItems.begin(); it_session != mConversationsItems.end(); it_session++)
	{
		// Get the current session descriptors
		LLConversationItem* session_model = it_session->second;
		// Iterate through each model participant child
		LLFolderViewModelItemCommon::child_list_t::const_iterator current_participant_model = session_model->getChildrenBegin();
		LLFolderViewModelItemCommon::child_list_t::const_iterator end_participant_model = session_model->getChildrenEnd();
		while (current_participant_model != end_participant_model)
		{
			LLConversationItemParticipant* participant_model = dynamic_cast<LLConversationItemParticipant*>(*current_participant_model);
			// Get the avatar name for this participant id from the cache and update the model
			LLUUID participant_id = participant_model->getUUID();
			LLAvatarName av_name;
			LLAvatarNameCache::get(participant_id,&av_name);
			// Avoid updating the model though if the cache is still waiting for its first update
			if (!av_name.mDisplayName.empty())
			{
				participant_model->onAvatarNameCache(av_name);
			}
			// Bind update to the next cache name signal
			LLAvatarNameCache::get(participant_id, boost::bind(&LLConversationItemParticipant::onAvatarNameCache, participant_model, _2));
			// Next participant
			current_participant_model++;
		}
	}
}

// static
void LLIMFloaterContainer::idle(void* user_data)
{
	LLIMFloaterContainer* self = static_cast<LLIMFloaterContainer*>(user_data);
	
	// Update the distance to agent in the nearby chat session if required
	// Note: it makes no sense of course to update the distance in other session
	if (self->mConversationViewModel.getSorter().getSortOrderParticipants() == LLConversationFilter::SO_DISTANCE)
	{
		self->setNearbyDistances();
	}
	self->mConversationsRoot->update();
}

bool LLIMFloaterContainer::onConversationModelEvent(const LLSD& event)
{
	// For debug only
	//std::ostringstream llsd_value;
	//llsd_value << LLSDOStreamer<LLSDNotationFormatter>(event) << std::endl;
	//llinfos << "Merov debug : onConversationModelEvent, event = " << llsd_value.str() << llendl;
	// end debug
	
	// Note: In conversations, the model is not responsible for creating the view, which is a good thing. This means that
	// the model could change substantially and the view could echo only a portion of this model (though currently the 
	// conversation view does echo the conversation model 1 to 1).
	// Consequently, the participant views need to be created either by the session view or by the container panel.
	// For the moment, we create them here, at the container level, to conform to the pattern implemented in llinventorypanel.cpp 
	// (see LLInventoryPanel::buildNewViews()).

	// Note: For the moment, we're not very smart about the event parameter and we just refresh the whole set of views/widgets
	// according to the current state of the whole model.
	// We should at least analyze the event payload and do things differently for a handful of cases:
	// - add session or participant
	// - remove session or participant
	// - update session or participant (e.g. rename, change sort order, etc...)
	// Please see LLConversationItem::postEvent() for the payload formatting.
	// *TODO: Add handling for various event signatures (add, remove, update, resort)

	std::string type = event.get("type").asString();
	if (type == "remove_participant")
	{
		LLUUID session_id = event.get("session_uuid").asUUID();
		LLConversationViewSession* session_view = dynamic_cast<LLConversationViewSession*>(mConversationsWidgets[session_id]);
		LLUUID participant_id = event.get("participant_uuid").asUUID();
		LLConversationViewParticipant* participant_view = session_view->findParticipant(participant_id);
		if (participant_view)
		{
			session_view->extractItem(participant_view);
			delete participant_view;
			session_view->refresh();
			mConversationsRoot->arrangeAll();
		}
	}
	else 
	{
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
			mConversationViewModel.requestSortAll();
			mConversationsRoot->arrangeAll();
			// Next participant
			current_participant_model++;
		}
	}
	}
	
	return false;
}

void LLIMFloaterContainer::draw()
{
	if (mTabContainer->getTabCount() == 0)
	{
		// Do not close the container when every conversation is torn off because the user
		// still needs the conversation list. Simply collapse the message pane in that case.
		collapseMessagesPane(true);
	}
	LLFloater::draw();
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
{	LLNearbyChat* nearby_chat;
	if (visible)
	{
		// Make sure we have the Nearby Chat present when showing the conversation container
		nearby_chat = LLFloaterReg::findTypedInstance<LLNearbyChat>("nearby_chat");
		if (nearby_chat == NULL)
		{
			// If not found, force the creation of the nearby chat conversation panel
			// *TODO: find a way to move this to XML as a default panel or something like that
			LLSD name("nearby_chat");
			LLFloaterReg::toggleInstanceOrBringToFront(name);
		}
	}

	nearby_chat = LLFloaterReg::findTypedInstance<LLNearbyChat>("nearby_chat");
	if (nearby_chat && !nearby_chat->isHostSet())
	{
		nearby_chat->addToHost();
	}

	// We need to show/hide all the associated conversations that have been torn off
	// (and therefore, are not longer managed by the multifloater),
	// so that they show/hide with the conversations manager.
	conversations_widgets_map::iterator widget_it = mConversationsWidgets.begin();
	for (;widget_it != mConversationsWidgets.end(); ++widget_it)
	{
		LLConversationViewSession* widget = dynamic_cast<LLConversationViewSession*>(widget_it->second);
		if (widget)
		{
		    widget->setVisibleIfDetached(visible);
		}
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

	for (conversations_widgets_map::iterator widget_it = mConversationsWidgets.begin();
			widget_it != mConversationsWidgets.end(); ++widget_it)
	{
		LLConversationViewSession* widget = dynamic_cast<LLConversationViewSession*>(widget_it->second);
		if (widget)
		{
		    widget->toggleMinimizedMode(collapse);

		    // force closing all open conversations when collapsing to minimized state
		    if (collapse)
		    {
		    	widget->setOpen(false);
		    }
}
	}
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

    // force set correct size for the title after show/hide minimize button
	LLRect cur_rect = getRect();
	LLRect force_rect = cur_rect;
	force_rect.mRight = cur_rect.mRight + 1;
    setRect(force_rect);
    setRect(cur_rect);

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

	if ("sort_sessions_by_type" == command)
	{
		setSortOrderSessions(LLConversationFilter::SO_SESSION_TYPE);
	}
	if ("sort_sessions_by_name" == command)
	{
		setSortOrderSessions(LLConversationFilter::SO_NAME);
	}
	if ("sort_sessions_by_recent" == command)
	{
		setSortOrderSessions(LLConversationFilter::SO_DATE);
	}
	if ("sort_participants_by_name" == command)
	{
		setSortOrderParticipants(LLConversationFilter::SO_NAME);
	}
	if ("sort_participants_by_recent" == command)
	{
		setSortOrderParticipants(LLConversationFilter::SO_DATE);
	}
	if ("sort_participants_by_distance" == command)
	{
		setSortOrderParticipants(LLConversationFilter::SO_DISTANCE);
	}
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

BOOL LLIMFloaterContainer::isActionChecked(const LLSD& userdata)
{
	LLConversationSort order = mConversationViewModel.getSorter();
	std::string command = userdata.asString();
	if ("sort_sessions_by_type" == command)
	{
		return (order.getSortOrderSessions() == LLConversationFilter::SO_SESSION_TYPE);
	}
	if ("sort_sessions_by_name" == command)
	{
		return (order.getSortOrderSessions() == LLConversationFilter::SO_NAME);
	}
	if ("sort_sessions_by_recent" == command)
	{
		return (order.getSortOrderSessions() == LLConversationFilter::SO_DATE);
	}
	if ("sort_participants_by_name" == command)
	{
		return (order.getSortOrderParticipants() == LLConversationFilter::SO_NAME);
	}
	if ("sort_participants_by_recent" == command)
	{
		return (order.getSortOrderParticipants() == LLConversationFilter::SO_DATE);
	}
	if ("sort_participants_by_distance" == command)
	{
		return (order.getSortOrderParticipants() == LLConversationFilter::SO_DISTANCE);
	}
	
	return FALSE;
}

void LLIMFloaterContainer::setSortOrderSessions(const LLConversationFilter::ESortOrderType order)
{
	LLConversationSort old_order = mConversationViewModel.getSorter();
	if (order != old_order.getSortOrderSessions())
	{
		old_order.setSortOrderSessions(order);
		setSortOrder(old_order);
	}
}

void LLIMFloaterContainer::setSortOrderParticipants(const LLConversationFilter::ESortOrderType order)
{
	LLConversationSort old_order = mConversationViewModel.getSorter();
	if (order != old_order.getSortOrderParticipants())
	{
		old_order.setSortOrderParticipants(order);
		setSortOrder(old_order);
	}
}

void LLIMFloaterContainer::setSortOrder(const LLConversationSort& order)
{
	mConversationViewModel.setSorter(order);
	mConversationsRoot->arrangeAll();
	// try to keep selection onscreen, even if it wasn't to start with
	mConversationsRoot->scrollToShowSelection();
	gSavedSettings.setU32("ConversationSortOrder", (U32)order);
}

void LLIMFloaterContainer::getSelectedUUIDs(uuid_vec_t& selected_uuids)
{
    const std::set<LLFolderViewItem*> selectedItems = mConversationsRoot->getSelectionList();

    std::set<LLFolderViewItem*>::const_iterator it = selectedItems.begin();
    const std::set<LLFolderViewItem*>::const_iterator it_end = selectedItems.end();
    LLConversationItem * conversationItem;

    for (; it != it_end; ++it)
    {
        conversationItem = static_cast<LLConversationItem *>((*it)->getViewModelItem());
        selected_uuids.push_back(conversationItem->getUUID());
    }
}

const LLConversationItem * LLIMFloaterContainer::getCurSelectedViewModelItem()
{
    LLConversationItem * conversationItem = NULL;

    if(mConversationsRoot && 
        mConversationsRoot->getCurSelectedItem() && 
        mConversationsRoot->getCurSelectedItem()->getViewModelItem())
    {
        conversationItem = static_cast<LLConversationItem *>(mConversationsRoot->getCurSelectedItem()->getViewModelItem());
    }

    return conversationItem;
}

void LLIMFloaterContainer::doToUsers(const std::string& command, uuid_vec_t selectedIDS)
{
    LLUUID userID;
    userID = selectedIDS.front();

    if ("view_profile" == command)
    {
        LLAvatarActions::showProfile(userID);
    }
    else if("im" == command)
    {
        LLAvatarActions::startIM(userID);
    }
    else if("offer_teleport" == command)
    {
        LLAvatarActions::offerTeleport(selectedIDS);
    }
    else if("voice_call" == command)
    {
        LLAvatarActions::startCall(userID);
    }
    else if("chat_history" == command)
    {
        LLAvatarActions::viewChatHistory(userID);
    }
    else if("add_friend" == command)
    {
        LLAvatarActions::requestFriendshipDialog(userID);
    }
    else if("remove_friend" == command)
    {
        LLAvatarActions::removeFriendDialog(userID);
    }
    else if("invite_to_group" == command)
    {
        LLAvatarActions::inviteToGroup(userID);
    }
    else if("map" == command)
    {
        LLAvatarActions::showOnMap(userID);
    }
    else if("share" == command)
    {
        LLAvatarActions::share(userID);
    }
    else if("pay" == command)
    {
        LLAvatarActions::pay(userID);
    }
    else if("block_unblock" == command)
    {
        LLAvatarActions::toggleBlock(userID);
    }
}

void LLIMFloaterContainer::doToSelectedParticipant(const std::string& command)
{
    uuid_vec_t selected_uuids;
    getSelectedUUIDs(selected_uuids);
   
    doToUsers(command, selected_uuids);
}

void LLIMFloaterContainer::doToSelectedConversation(const std::string& command)
{
    LLUUID participantID;

    //Find the conversation floater associated with the selected id
    const LLConversationItem * conversationItem = getCurSelectedViewModelItem();
    LLIMFloater *conversationFloater = LLIMFloater::findInstance(conversationItem->getUUID());

    if(conversationFloater)
    {
        //When a one-on-one conversation exists, retrieve the participant id from the conversation floater b/c
        //selected_uuids.front() does not pertain to the UUID of the person you are having the conversation with.
        if(conversationItem->getType() == LLConversationItem::CONV_SESSION_1_ON_1)
        {
            participantID = conversationFloater->getOtherParticipantUUID();
        }

        //Close the selected conversation
        if("close_conversation" == command)
        {
            LLFloater::onClickClose(conversationFloater);
        }
        else if("open_voice_conversation" == command)
        {
            gIMMgr->startCall(conversationItem->getUUID());
        }
        else if("disconnect_from_voice" == command)
        {
            gIMMgr->endCall(conversationItem->getUUID());
        }
        else if("chat_history" == command)
        {
            LLAvatarActions::viewChatHistory(conversationItem->getUUID());
        }
        else
        {
            uuid_vec_t selected_uuids;
            selected_uuids.push_back(participantID);
            doToUsers(command, selected_uuids);
        }
    }
}

void LLIMFloaterContainer::doToSelected(const LLSD& userdata)
{
    std::string command = userdata.asString();
    const LLConversationItem * conversationItem = getCurSelectedViewModelItem();

    if(conversationItem->getType() == LLConversationItem::CONV_SESSION_1_ON_1 ||
        conversationItem->getType() == LLConversationItem::CONV_SESSION_GROUP ||
        conversationItem->getType() == LLConversationItem::CONV_SESSION_AD_HOC)
    {
        doToSelectedConversation(command);
    }
    else
    {
        doToSelectedParticipant(command);
    }
}

void LLIMFloaterContainer::doToSelectedGroup(const LLSD& userdata)
{
    std::string action = userdata.asString();
    LLUUID selected_group = getCurSelectedViewModelItem()->getUUID();

    if (action == "group_profile")
    {
        LLGroupActions::show(selected_group);
    }
    else if (action == "activate_group")
    {
        LLGroupActions::activate(selected_group);
    }
    else if (action == "leave_group")
    {
        LLGroupActions::leave(selected_group);
    }
}

bool LLIMFloaterContainer::enableContextMenuItem(const LLSD& userdata)
{
    std::string item = userdata.asString();
    uuid_vec_t mUUIDs;
    getSelectedUUIDs(mUUIDs);

    // Note: can_block and can_delete is used only for one person selected menu
    // so we don't need to go over all uuids.

    if (item == std::string("can_block"))
    {
        const LLUUID& id = mUUIDs.front();
        return LLAvatarActions::canBlock(id);
    }
    else if (item == std::string("can_add"))
    {
        // We can add friends if:
        // - there are selected people
        // - and there are no friends among selection yet.

        //EXT-7389 - disable for more than 1
        if(mUUIDs.size() > 1)
        {
            return false;
        }

        bool result = (mUUIDs.size() > 0);

        uuid_vec_t::const_iterator
            id = mUUIDs.begin(),
            uuids_end = mUUIDs.end();

        for (;id != uuids_end; ++id)
        {
            if ( LLAvatarActions::isFriend(*id) )
            {
                result = false;
                break;
            }
        }

        return result;
    }
    else if (item == std::string("can_delete"))
    {
        // We can remove friends if:
        // - there are selected people
        // - and there are only friends among selection.

        bool result = (mUUIDs.size() > 0);

        uuid_vec_t::const_iterator
            id = mUUIDs.begin(),
            uuids_end = mUUIDs.end();

        for (;id != uuids_end; ++id)
        {
            if ( !LLAvatarActions::isFriend(*id) )
            {
                result = false;
                break;
            }
        }

        return result;
    }
    else if (item == std::string("can_call"))
    {
        return LLAvatarActions::canCall();
    }
    else if (item == std::string("can_show_on_map"))
    {
        const LLUUID& id = mUUIDs.front();

        return (LLAvatarTracker::instance().isBuddyOnline(id) && is_agent_mappable(id))
            || gAgent.isGodlike();
    }
    else if(item == std::string("can_offer_teleport"))
    {
        return LLAvatarActions::canOfferTeleport(mUUIDs);
    }
    return false;
}

bool LLIMFloaterContainer::checkContextMenuItem(const LLSD& userdata)
{
    std::string item = userdata.asString();
    const LLUUID& id = getCurSelectedViewModelItem()->getUUID();

    if (item == std::string("is_blocked"))
    {
        return LLAvatarActions::isBlocked(id);
    }

    return false;
}

void LLIMFloaterContainer::setConvItemSelect(const LLUUID& session_id)
{
	LLFolderViewItem* widget = mConversationsWidgets[session_id];
	if (widget && mSelectedSession != session_id)
	{
		mSelectedSession = session_id;
		(widget->getRoot())->setSelection(widget, FALSE, FALSE);
	}
}

void LLIMFloaterContainer::setTimeNow(const LLUUID& session_id, const LLUUID& participant_id)
{
	conversations_items_map::iterator item_it = mConversationsItems.find(session_id);
	if (item_it != mConversationsItems.end())
	{
		LLConversationItemSession* item = dynamic_cast<LLConversationItemSession*>(item_it->second);
		if (item)
		{
			item->setTimeNow(participant_id);
			mConversationViewModel.requestSortAll();
			mConversationsRoot->arrangeAll();
		}
	}
}

void LLIMFloaterContainer::setNearbyDistances()
{
	// Get the nearby chat session: that's the one with uuid nul in mConversationsItems
	conversations_items_map::iterator item_it = mConversationsItems.find(LLUUID());
	if (item_it != mConversationsItems.end())
	{
		LLConversationItemSession* item = dynamic_cast<LLConversationItemSession*>(item_it->second);
		if (item)
		{
			// Get the positions of the nearby avatars and their ids
			std::vector<LLVector3d> positions;
			uuid_vec_t avatar_ids;
			LLWorld::getInstance()->getAvatars(&avatar_ids, &positions, gAgent.getPositionGlobal(), gSavedSettings.getF32("NearMeRange"));
			// Get the position of the agent
			const LLVector3d& me_pos = gAgent.getPositionGlobal();
			// For each nearby avatar, compute and update the distance
			int avatar_count = positions.size();
			for (int i = 0; i < avatar_count; i++)
			{
				F64 dist = dist_vec_squared(positions[i], me_pos);
				item->setDistance(avatar_ids[i],dist);
			}
			// Also does it for the agent itself
			item->setDistance(gAgent.getID(),0.0f);
			// Request resort
			mConversationViewModel.requestSortAll();
			mConversationsRoot->arrangeAll();
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
		llwarns << "Couldn't create conversation session item : " << display_name << llendl;
		return;
	}
	item->renameItem(display_name);
	
	mConversationsItems[uuid] = item;

	// Create a widget from it
	LLConversationViewSession* widget = createConversationItemWidget(item);
	mConversationsWidgets[uuid] = widget;

	// Add a new conversation widget to the root folder of the folder view
	widget->addToFolder(mConversationsRoot);
	widget->requestArrange();
	
	// Create the participants widgets now
	// Note: usually, we do not get an updated avatar list at that point
	LLFolderViewModelItemCommon::child_list_t::const_iterator current_participant_model = item->getChildrenBegin();
	LLFolderViewModelItemCommon::child_list_t::const_iterator end_participant_model = item->getChildrenEnd();
	while (current_participant_model != end_participant_model)
	{
		LLConversationItem* participant_model = dynamic_cast<LLConversationItem*>(*current_participant_model);
		LLConversationViewParticipant* participant_view = createConversationViewParticipant(participant_model);
		participant_view->addToFolder(widget);
		current_participant_model++;
	}

	setConvItemSelect(uuid);
	
	// set the widget to minimized mode if conversations pane is collapsed
	widget->toggleMinimizedMode(mConversationsPane->isCollapsed());

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
		if (widget)
		{
			widget->destroyView();
		}
	}
	
	// Suppress the conversation items and widgets from their respective maps
	mConversationsItems.erase(uuid);
	mConversationsWidgets.erase(uuid);
	
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
	params.root = mConversationsRoot;
	params.listener = item;
	params.tool_tip = params.name;
	params.container = this;
	
	return LLUICtrlFactory::create<LLConversationViewSession>(params);
}

LLConversationViewParticipant* LLIMFloaterContainer::createConversationViewParticipant(LLConversationItem* item)
{
	LLConversationViewParticipant::Params params;
    LLRect panel_rect = mConversationsListPanel->getRect();
	
	params.name = item->getDisplayName();
	//params.icon = bridge->getIcon();
	//params.icon_open = bridge->getOpenIcon();
	//params.creation_date = bridge->getCreationDate();
	params.root = mConversationsRoot;
	params.listener = item;

    //24 is the the current hight of an item (itemHeight) loaded from conversation_view_participant.xml.
	params.rect = LLRect (0, 24, panel_rect.getWidth(), 0);
	params.tool_tip = params.name;
	params.participant_id = item->getUUID();
	
	return LLUICtrlFactory::create<LLConversationViewParticipant>(params);
}

// EOF

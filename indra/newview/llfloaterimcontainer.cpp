/** 
 * @file llfloaterimcontainer.cpp
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

#include "llfloaterimsession.h"
#include "llfloaterimcontainer.h"

#include "llfloaterreg.h"
#include "lllayoutstack.h"
#include "llfloaterimnearbychat.h"

#include "llagent.h"
#include "llavataractions.h"
#include "llavatariconctrl.h"
#include "llavatarnamecache.h"
#include "llcallbacklist.h"
#include "lldonotdisturbnotificationstorage.h"
#include "llgroupactions.h"
#include "llgroupiconctrl.h"
#include "llflashtimer.h"
#include "llfloateravatarpicker.h"
#include "llfloaterpreference.h"
#include "llimview.h"
#include "llnotificationsutil.h"
#include "lltoolbarview.h"
#include "lltransientfloatermgr.h"
#include "llviewercontrol.h"
#include "llconversationview.h"
#include "llcallbacklist.h"
#include "llworld.h"
#include "llsdserialize.h"
#include "llviewerobjectlist.h"
#include "boost/foreach.hpp"

//
// LLFloaterIMContainer
//
LLFloaterIMContainer::LLFloaterIMContainer(const LLSD& seed, const Params& params /*= getDefaultParams()*/)
:	LLMultiFloater(seed, params),
	mExpandCollapseBtn(NULL),
	mConversationsRoot(NULL),
	mConversationsEventStream("ConversationsEvents"),
	mInitialized(false),
	mIsFirstLaunch(true)
{
    mEnableCallbackRegistrar.add("IMFloaterContainer.Check", boost::bind(&LLFloaterIMContainer::isActionChecked, this, _2));
	mCommitCallbackRegistrar.add("IMFloaterContainer.Action", boost::bind(&LLFloaterIMContainer::onCustomAction,  this, _2));
	
	mEnableCallbackRegistrar.add("Avatar.CheckItem",  boost::bind(&LLFloaterIMContainer::checkContextMenuItem,	this, _2));
	mEnableCallbackRegistrar.add("Avatar.EnableItem", boost::bind(&LLFloaterIMContainer::enableContextMenuItem,	this, _2));
	mEnableCallbackRegistrar.add("Avatar.VisibleItem", boost::bind(&LLFloaterIMContainer::visibleContextMenuItem,	this, _2));
    mCommitCallbackRegistrar.add("Avatar.DoToSelected", boost::bind(&LLFloaterIMContainer::doToSelected, this, _2));
    
    mCommitCallbackRegistrar.add("Group.DoToSelected", boost::bind(&LLFloaterIMContainer::doToSelectedGroup, this, _2));

	// Firstly add our self to IMSession observers, so we catch session events
    LLIMMgr::getInstance()->addSessionObserver(this);

	mAutoResize = FALSE;
	LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::IM, this);
}

LLFloaterIMContainer::~LLFloaterIMContainer()
{
	mConversationsEventStream.stopListening("ConversationsRefresh");
	gIdleCallbacks.deleteFunction(idle, this);
	mNewMessageConnection.disconnect();
	LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::IM, this);
	
	if (mMicroChangedSignal.connected())
	{
		mMicroChangedSignal.disconnect();
	}

	gSavedPerAccountSettings.setBOOL("ConversationsListPaneCollapsed", mConversationsPane->isCollapsed());
	gSavedPerAccountSettings.setBOOL("ConversationsMessagePaneCollapsed", mMessagesPane->isCollapsed());
	gSavedPerAccountSettings.setBOOL("ConversationsParticipantListCollapsed", !isParticipantListExpanded());

	if (!LLSingleton<LLIMMgr>::destroyed())
	{
		LLIMMgr::getInstance()->removeSessionObserver(this);
	}
}

void LLFloaterIMContainer::sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id, BOOL has_offline_msg)
{
	addConversationListItem(session_id);
	LLFloaterIMSessionTab::addToHost(session_id);
}

void LLFloaterIMContainer::sessionActivated(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id)
{
	if(!isInVisibleChain())
	{
		setVisibleAndFrontmost(false);
	}
	selectConversationPair(session_id, true);
	collapseMessagesPane(false);
}

void LLFloaterIMContainer::sessionVoiceOrIMStarted(const LLUUID& session_id)
{
	addConversationListItem(session_id);
	LLFloaterIMSessionTab::addToHost(session_id);
}

void LLFloaterIMContainer::sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id)
{
	// The general strategy when a session id is modified is to delete all related objects and create them anew.
	
	// Note however that the LLFloaterIMSession has its session id updated through a call to sessionInitReplyReceived() 
	// and do not need to be deleted and recreated (trying this creates loads of problems). We do need however to suppress 
	// its related mSessions record as it's indexed with the wrong id.
	// Grabbing the updated LLFloaterIMSession and readding it in mSessions will eventually be done by addConversationListItem().
	mSessions.erase(old_session_id);

	// Delete the model and participants related to the old session
	bool change_focus = removeConversationListItem(old_session_id);

	// Create a new conversation with the new id
	addConversationListItem(new_session_id, change_focus);
	LLFloaterIMSessionTab::addToHost(new_session_id);
}

void LLFloaterIMContainer::sessionRemoved(const LLUUID& session_id)
{
	removeConversationListItem(session_id);
}

// static
void LLFloaterIMContainer::onCurrentChannelChanged(const LLUUID& session_id)
{
    if (session_id != LLUUID::null)
    {
    	LLFloaterIMContainer::getInstance()->showConversation(session_id);
    }
}

BOOL LLFloaterIMContainer::postBuild()
{
	mOrigMinWidth = getMinWidth();
	mOrigMinHeight = getMinHeight();

	mNewMessageConnection = LLIMModel::instance().mNewMsgSignal.connect(boost::bind(&LLFloaterIMContainer::onNewMessageReceived, this, _1));
	// Do not call base postBuild to not connect to mCloseSignal to not close all floaters via Close button
	// mTabContainer will be initialized in LLMultiFloater::addChild()
	
	setTabContainer(getChild<LLTabContainer>("im_box_tab_container"));
	mStubPanel = getChild<LLPanel>("stub_panel");
    mStubTextBox = getChild<LLTextBox>("stub_textbox");
    mStubTextBox->setURLClickedCallback(boost::bind(&LLFloaterIMContainer::returnFloaterToHost, this));

	mConversationsStack = getChild<LLLayoutStack>("conversations_stack");
	mConversationsPane = getChild<LLLayoutPanel>("conversations_layout_panel");
	mMessagesPane = getChild<LLLayoutPanel>("messages_layout_panel");
	
	mConversationsListPanel = getChild<LLPanel>("conversations_list_panel");

	// Open IM session with selected participant on double click event
	mConversationsListPanel->setDoubleClickCallback(boost::bind(&LLFloaterIMContainer::doToSelected, this, LLSD("im")));

	// The resize limits for LLFloaterIMContainer should be updated, based on current values of width of conversation and message panels
	mConversationsPane->getResizeBar()->setResizeListener(boost::bind(&LLFloaterIMContainer::assignResizeLimits, this));

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
	mConversationsEventStream.listen("ConversationsRefresh", boost::bind(&LLFloaterIMContainer::onConversationModelEvent, this, _1));

	// a scroller for folder view
	LLRect scroller_view_rect = mConversationsListPanel->getRect();
	scroller_view_rect.translate(-scroller_view_rect.mLeft, -scroller_view_rect.mBottom);
	scroller_view_rect.mBottom += getChild<LLLayoutStack>("conversations_pane_buttons_stack")->getRect().getHeight();
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
	mExpandCollapseBtn->setClickedCallback(boost::bind(&LLFloaterIMContainer::onExpandCollapseButtonClicked, this));
	mStubCollapseBtn = getChild<LLButton>("stub_collapse_btn");
	mStubCollapseBtn->setClickedCallback(boost::bind(&LLFloaterIMContainer::onStubCollapseButtonClicked, this));
    mSpeakBtn = getChild<LLButton>("speak_btn");

	mSpeakBtn->setMouseDownCallback(boost::bind(&LLFloaterIMContainer::onSpeakButtonPressed, this));
	mSpeakBtn->setMouseUpCallback(boost::bind(&LLFloaterIMContainer::onSpeakButtonReleased, this));

	childSetAction("add_btn", boost::bind(&LLFloaterIMContainer::onAddButtonClicked, this));

	collapseMessagesPane(gSavedPerAccountSettings.getBOOL("ConversationsMessagePaneCollapsed"));
	collapseConversationsPane(gSavedPerAccountSettings.getBOOL("ConversationsListPaneCollapsed"), false);
	LLAvatarNameCache::addUseDisplayNamesCallback(boost::bind(&LLFloaterIMSessionTab::processChatHistoryStyleUpdate, false));
	mMicroChangedSignal = LLVoiceClient::getInstance()->MicroChangedCallback(boost::bind(&LLFloaterIMContainer::updateSpeakBtnState, this));

	if (! mMessagesPane->isCollapsed() && ! mConversationsPane->isCollapsed())
	{
		S32 conversations_panel_width = gSavedPerAccountSettings.getS32("ConversationsListPaneWidth");
		LLRect conversations_panel_rect = mConversationsPane->getRect();
		conversations_panel_rect.mRight = conversations_panel_rect.mLeft + conversations_panel_width;
        mConversationsPane->handleReshape(conversations_panel_rect, TRUE);
	}

	// Init the sort order now that the root had been created
	setSortOrder(LLConversationSort(gSavedSettings.getU32("ConversationSortOrder")));
	
	//We should expand nearby chat participants list for the new user
	if(gAgent.isFirstLogin() || !gSavedPerAccountSettings.getBOOL("ConversationsParticipantListCollapsed"))
	{
		expandConversation();
	}
	// Keep the xml set title around for when we have to overwrite it
	mGeneralTitle = getTitle();
	
	mInitialized = true;

	// Add callbacks:
	// We'll take care of view updates on idle
	gIdleCallbacks.addFunction(idle, this);
	// When display name option change, we need to reload all participant names
	LLAvatarNameCache::addUseDisplayNamesCallback(boost::bind(&LLFloaterIMContainer::processParticipantsStyleUpdate, this));

	return TRUE;
}

void LLFloaterIMContainer::onOpen(const LLSD& key)
{
	LLMultiFloater::onOpen(key);
	reSelectConversation();
	assignResizeLimits();
}

// virtual
void LLFloaterIMContainer::addFloater(LLFloater* floaterp,
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

	LLUUID session_id = floaterp->getKey();
	
	// Add the floater
	LLMultiFloater::addFloater(floaterp, select_added_floater, insertion_point);


	
	LLIconCtrl* icon = 0;

	if(gAgent.isInGroup(session_id, TRUE))
	{
		LLGroupIconCtrl::Params icon_params;
		icon_params.group_id = session_id;
		icon = LLUICtrlFactory::instance().create<LLGroupIconCtrl>(icon_params);

		mSessions[session_id] = floaterp;
		floaterp->mCloseSignal.connect(boost::bind(&LLFloaterIMContainer::onCloseFloater, this, session_id));
	}
	else
	{   LLUUID avatar_id = session_id.notNull()?
		    LLIMModel::getInstance()->getOtherParticipantID(session_id) : LLUUID();

		LLAvatarIconCtrl::Params icon_params;
		icon_params.avatar_id = avatar_id;
		icon = LLUICtrlFactory::instance().create<LLAvatarIconCtrl>(icon_params);

		mSessions[session_id] = floaterp;
		floaterp->mCloseSignal.connect(boost::bind(&LLFloaterIMContainer::onCloseFloater, this, session_id));
	}

	// forced resize of the floater
	LLRect wrapper_rect = this->mTabContainer->getLocalRect();
	floaterp->setRect(wrapper_rect);

	mTabContainer->setTabImage(floaterp, icon);
}


void LLFloaterIMContainer::onCloseFloater(LLUUID& id)
{
	mSessions.erase(id);
	setFocus(TRUE);
}

void LLFloaterIMContainer::onNewMessageReceived(const LLSD& data)
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

void LLFloaterIMContainer::onStubCollapseButtonClicked()
{
	collapseMessagesPane(true);
}

void LLFloaterIMContainer::onSpeakButtonPressed()
{
	LLVoiceClient::getInstance()->inputUserControlState(true);
	updateSpeakBtnState();
}

void LLFloaterIMContainer::onSpeakButtonReleased()
{
	LLVoiceClient::getInstance()->inputUserControlState(false);
	updateSpeakBtnState();
}

void LLFloaterIMContainer::onExpandCollapseButtonClicked()
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
	reSelectConversation();
}

LLFloaterIMContainer* LLFloaterIMContainer::findInstance()
{
	return LLFloaterReg::findTypedInstance<LLFloaterIMContainer>("im_container");
}

LLFloaterIMContainer* LLFloaterIMContainer::getInstance()
{
	return LLFloaterReg::getTypedInstance<LLFloaterIMContainer>("im_container");
}

// Update all participants in the conversation lists
void LLFloaterIMContainer::processParticipantsStyleUpdate()
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
			participant_model->updateName();
			// Next participant
			current_participant_model++;
		}
	}
}

// static
void LLFloaterIMContainer::idle(void* user_data)
{
	LLFloaterIMContainer* self = static_cast<LLFloaterIMContainer*>(user_data);
	
	// Update the distance to agent in the nearby chat session if required
	// Note: it makes no sense of course to update the distance in other session
	if (self->mConversationViewModel.getSorter().getSortOrderParticipants() == LLConversationFilter::SO_DISTANCE)
	{
		self->setNearbyDistances();
	}
	self->mConversationsRoot->update();
}

bool LLFloaterIMContainer::onConversationModelEvent(const LLSD& event)
{
	// For debug only
	//std::ostringstream llsd_value;
	//llsd_value << LLSDOStreamer<LLSDNotationFormatter>(event) << std::endl;
	//LL_INFOS() << "LLFloaterIMContainer::onConversationModelEvent, event = " << llsd_value.str() << LL_ENDL;
	// end debug
	
	// Note: In conversations, the model is not responsible for creating the view, which is a good thing. This means that
	// the model could change substantially and the view could echo only a portion of this model (though currently the 
	// conversation view does echo the conversation model 1 to 1).
	// Consequently, the participant views need to be created either by the session view or by the container panel.
	// For the moment, we create them here, at the container level, to conform to the pattern implemented in llinventorypanel.cpp 
	// (see LLInventoryPanel::buildNewViews()).

	std::string type = event.get("type").asString();
	LLUUID session_id = event.get("session_uuid").asUUID();
	LLUUID participant_id = event.get("participant_uuid").asUUID();

	LLConversationViewSession* session_view = dynamic_cast<LLConversationViewSession*>(get_ptr_in_map(mConversationsWidgets,session_id));
	if (!session_view)
	{
		// We skip events that are not associated with a session
		return false;
	}
	LLConversationViewParticipant* participant_view = session_view->findParticipant(participant_id);
    LLFloaterIMSessionTab *conversation_floater = (session_id.isNull() ?
    		(LLFloaterIMSessionTab*)(LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat"))
    		: (LLFloaterIMSessionTab*)(LLFloaterIMSession::findInstance(session_id)));

	if (type == "remove_participant")
	{
		// Remove a participant view from the hierarchical conversation list
		if (participant_view)
		{
			session_view->extractItem(participant_view);
			delete participant_view;
			session_view->refresh();
			mConversationsRoot->arrangeAll();
		}
		// Remove a participant view from the conversation floater 
		if (conversation_floater)
		{
			conversation_floater->removeConversationViewParticipant(participant_id);
		}
	}
	else if (type == "add_participant")
	{
		LLConversationItemSession* session_model = dynamic_cast<LLConversationItemSession*>(mConversationsItems[session_id]);
		LLConversationItemParticipant* participant_model = (session_model ? session_model->findParticipant(participant_id) : NULL);
		if (!participant_view && session_model && participant_model)
		{
			LLIMModel::LLIMSession * im_sessionp = LLIMModel::getInstance()->findIMSession(session_id);
			if (session_id.isNull() || (im_sessionp && !im_sessionp->isP2PSessionType()))
			{
				participant_view = createConversationViewParticipant(participant_model);
				participant_view->addToFolder(session_view);
				participant_view->setVisible(TRUE);
			}
		}
		// Add a participant view to the conversation floater 
		if (conversation_floater && participant_model)
		{
			conversation_floater->addConversationViewParticipant(participant_model);
		}
	}
	else if (type == "update_participant")
	{
		// Update the participant view in the hierarchical conversation list
		if (participant_view)
		{
			participant_view->refresh();
		}
		// Update the participant view in the conversation floater 
		if (conversation_floater)
		{
			conversation_floater->updateConversationViewParticipant(participant_id);
		}
	}
	else if (type == "update_session")
	{
		session_view->refresh();
	}
	
	mConversationViewModel.requestSortAll();
	mConversationsRoot->arrangeAll();
	if (conversation_floater)
	{
		conversation_floater->refreshConversation();
	}
	
	return false;
}

void LLFloaterIMContainer::draw()
{
	if (mTabContainer->getTabCount() == 0)
	{
		// Do not close the container when every conversation is torn off because the user
		// still needs the conversation list. Simply collapse the message pane in that case.
		collapseMessagesPane(true);
	}
	
	const LLConversationItem *current_session = getCurSelectedViewModelItem();
	if (current_session)
	{
		// Update moderator options visibility
		LLFolderViewModelItemCommon::child_list_t::const_iterator current_participant_model = current_session->getChildrenBegin();
		LLFolderViewModelItemCommon::child_list_t::const_iterator end_participant_model = current_session->getChildrenEnd();
		while (current_participant_model != end_participant_model)
		{
			LLConversationItemParticipant* participant_model = dynamic_cast<LLConversationItemParticipant*>(*current_participant_model);
			participant_model->setModeratorOptionsVisible(isGroupModerator() && participant_model->getUUID() != gAgentID);
			participant_model->setGroupBanVisible(haveAbilityToBan() && participant_model->getUUID() != gAgentID);

			current_participant_model++;
		}
		// Update floater's title as required by the currently selected session or use the default title
		LLFloaterIMSession * conversation_floaterp = LLFloaterIMSession::findInstance(current_session->getUUID());
		setTitle(conversation_floaterp && conversation_floaterp->needsTitleOverwrite() ? conversation_floaterp->getTitle() : mGeneralTitle);
	}

    // "Manually" resize of mConversationsPane: same as temporarity cancellation of the flag "auto_resize=false" for it
	if (!mConversationsPane->isCollapsed() && mMessagesPane->isCollapsed())
	{
		LLRect stack_rect = mConversationsStack->getRect();
		mConversationsPane->reshape(stack_rect.getWidth(), stack_rect.getHeight(), true);
	}

	LLFloater::draw();
}

void LLFloaterIMContainer::tabClose()
{
	if (mTabContainer->getTabCount() == 0)
	{
		// Do not close the container when every conversation is torn off because the user
		// still needs the conversation list. Simply collapse the message pane in that case.
		collapseMessagesPane(true);
	}
}

//Shows/hides the stub panel when a conversation floater is torn off
void LLFloaterIMContainer::showStub(bool stub_is_visible)
{
    S32 tabCount = 0;
    LLPanel * tabPanel = NULL;

    if(stub_is_visible)
    {
        tabCount = mTabContainer->getTabCount();

        //Hide all tabs even stub
        for(S32 i = 0; i < tabCount; ++i)
        {
            tabPanel = mTabContainer->getPanelByIndex(i);

            if(tabPanel)
            {
                tabPanel->setVisible(false);
            }
        }

        //Set the index to the stub panel since we will be showing the stub
        mTabContainer->setCurrentPanelIndex(0);
    }

    //Now show/hide the stub
	mStubPanel->setVisible(stub_is_visible);
}

// listener for click on mStubTextBox2
void LLFloaterIMContainer::returnFloaterToHost()
{
	LLUUID session_id = this->getSelectedSession();
	LLFloaterIMSessionTab* floater = LLFloaterIMSessionTab::getConversation(session_id);
	floater->onTearOffClicked();
}

void LLFloaterIMContainer::setMinimized(BOOL b)
{
	bool was_minimized = isMinimized();
	LLMultiFloater::setMinimized(b);

	//Switching from minimized to un-minimized
	if(was_minimized && !b)
	{
		gToolBarView->flashCommand(LLCommandId("chat"), false);
		LLFloaterIMSessionTab* session_floater = LLFloaterIMSessionTab::findConversation(mSelectedSession);

		if(session_floater && !session_floater->isTornOff())
		{
			//When in DND mode, remove stored IM notifications
			//Nearby chat (Null) IMs are not stored while in DND mode, so can ignore removal
			if(gAgent.isDoNotDisturb() && mSelectedSession.notNull())
			{
				LLDoNotDisturbNotificationStorage::getInstance()->removeNotification(LLDoNotDisturbNotificationStorage::toastName, mSelectedSession);
			}
		}
	}
}

void LLFloaterIMContainer::setVisible(BOOL visible)
{	LLFloaterIMNearbyChat* nearby_chat;
	if (visible)
	{
		// Make sure we have the Nearby Chat present when showing the conversation container
		nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
		if (nearby_chat == NULL)
		{
			// If not found, force the creation of the nearby chat conversation panel
			// *TODO: find a way to move this to XML as a default panel or something like that
			LLSD name("nearby_chat");
			LLFloaterReg::toggleInstanceOrBringToFront(name);
            selectConversationPair(LLUUID(NULL), false, false);
		}
		flashConversationItemWidget(mSelectedSession,false);

		LLFloaterIMSessionTab* session_floater = LLFloaterIMSessionTab::findConversation(mSelectedSession);
		if(session_floater && !session_floater->isMinimized())
		{
			//When in DND mode, remove stored IM notifications
			//Nearby chat (Null) IMs are not stored while in DND mode, so can ignore removal
			if(gAgent.isDoNotDisturb() && mSelectedSession.notNull())
			{
				LLDoNotDisturbNotificationStorage::getInstance()->removeNotification(LLDoNotDisturbNotificationStorage::toastName, mSelectedSession);
			}
		}
	}

	nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
	if (nearby_chat)
	{
		LLFloaterIMSessionTab::addToHost(LLUUID());
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
			LLFloater* session_floater = widget->getSessionFloater();
			if (session_floater != nearby_chat)
			{
		    widget->setVisibleIfDetached(visible);
		}
	}
	}
	
	// Now, do the normal multifloater show/hide
	LLMultiFloater::setVisible(visible);
}

void LLFloaterIMContainer::getDetachedConversationFloaters(floater_list_t& floaters)
{
	typedef conversations_widgets_map::value_type conv_pair;
	LLFloaterIMNearbyChat *nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");

	BOOST_FOREACH(conv_pair item, mConversationsWidgets)
	{
		LLConversationViewSession* widget = dynamic_cast<LLConversationViewSession*>(item.second);
		if (widget)
		{
			LLFloater* session_floater = widget->getSessionFloater();

			// Exclude nearby chat from output, as it should be handled separately 
			if (session_floater && session_floater->isDetachedAndNotMinimized() 
				&& session_floater != nearby_chat)
			{
				floaters.push_back(session_floater);
			}
		}
	}
}

void LLFloaterIMContainer::setVisibleAndFrontmost(BOOL take_focus, const LLSD& key)
{
	LLMultiFloater::setVisibleAndFrontmost(take_focus, key);
	// Do not select "Nearby Chat" conversation, since it will bring its window to front
	// Only select other sessions
	if (!getSelectedSession().isNull())
	{
    selectConversationPair(getSelectedSession(), false, take_focus);
	}
	if (mInitialized && mIsFirstLaunch)
	{
		collapseMessagesPane(gSavedPerAccountSettings.getBOOL("ConversationsMessagePaneCollapsed"));
		mIsFirstLaunch = false;
}
}

void LLFloaterIMContainer::updateResizeLimits()
{
	LLMultiFloater::updateResizeLimits();
	assignResizeLimits();
}

bool LLFloaterIMContainer::isMessagesPaneCollapsed()
{
	return mMessagesPane->isCollapsed();
}

bool LLFloaterIMContainer::isConversationsPaneCollapsed()
{
	return mConversationsPane->isCollapsed();
}

void LLFloaterIMContainer::collapseMessagesPane(bool collapse)
{
	if (mMessagesPane->isCollapsed() == collapse)
	{
		return;
	}

	// Save current width of panels before collapsing/expanding right pane.
	S32 conv_pane_width = mConversationsPane->getRect().getWidth();
    S32 msg_pane_width = mMessagesPane->getRect().getWidth();

	if (collapse)
	{
		// Save the messages pane width before collapsing it.
		gSavedPerAccountSettings.setS32("ConversationsMessagePaneWidth", msg_pane_width);

		// Save the order in which the panels are closed to reverse user's last action.
		gSavedPerAccountSettings.setBOOL("ConversationsExpandMessagePaneFirst", mConversationsPane->isCollapsed());
	}

	mConversationsPane->setIgnoreReshape(collapse);

	// Show/hide the messages pane.
	mConversationsStack->collapsePanel(mMessagesPane, collapse);

	// Make sure layout is updated before resizing conversation pane.
	mConversationsStack->updateLayout();

	reshapeFloaterAndSetResizeLimits(collapse, gSavedPerAccountSettings.getS32("ConversationsMessagePaneWidth"));

	if (!collapse)
	{
		// Restore conversation's pane previous width after expanding messages pane.
		mConversationsPane->setTargetDim(conv_pane_width);
	}
}

void LLFloaterIMContainer::collapseConversationsPane(bool collapse, bool save_is_allowed /*=true*/)
{
	if (mConversationsPane->isCollapsed() == collapse)
	{
		return;
	}

	LLView* button_panel = getChild<LLView>("conversations_pane_buttons_expanded");
	button_panel->setVisible(!collapse);
	mExpandCollapseBtn->setImageOverlay(getString(collapse ? "expand_icon" : "collapse_icon"));

	// Save current width of Conversation panel before collapsing/expanding right pane.
	S32 conv_pane_width = mConversationsPane->getRect().getWidth();

	if (collapse && save_is_allowed)
	{
		// Save the conversations pane width before collapsing it.
		gSavedPerAccountSettings.setS32("ConversationsListPaneWidth", conv_pane_width);

		// Save the order in which the panels are closed to reverse user's last action.
		gSavedPerAccountSettings.setBOOL("ConversationsExpandMessagePaneFirst", !mMessagesPane->isCollapsed());
	}

	mConversationsStack->collapsePanel(mConversationsPane, collapse);
	if (!collapse)
	{
		// Make sure layout is updated before resizing conversation pane.
		mConversationsStack->updateLayout();
		// Restore conversation's pane previous width.
		mConversationsPane->setTargetDim(gSavedPerAccountSettings.getS32("ConversationsListPaneWidth"));
	}

	S32 delta_width = gSavedPerAccountSettings.getS32("ConversationsListPaneWidth") 
		- mConversationsPane->getMinDim() - mConversationsStack->getPanelSpacing() + 1;

	reshapeFloaterAndSetResizeLimits(collapse, delta_width);

	for (conversations_widgets_map::iterator widget_it = mConversationsWidgets.begin();
			widget_it != mConversationsWidgets.end(); ++widget_it)
	{
		LLConversationViewSession* widget = dynamic_cast<LLConversationViewSession*>(widget_it->second);
		if (widget)
		{
		    widget->toggleCollapsedMode(collapse);

		    // force closing all open conversations when collapsing to minimized state
		    if (collapse)
		    {
		    	widget->setOpen(false);
		    }
		    widget->requestArrange();
        }
	}
}

void LLFloaterIMContainer::reshapeFloaterAndSetResizeLimits(bool collapse, S32 delta_width)
{
	LLRect floater_rect = getRect();
	floater_rect.mRight += ((collapse ? -1 : 1) * delta_width);

	// Set by_user = true so that reshaped rect is saved in user_settings.
	setShape(floater_rect, true);
	updateResizeLimits();

	bool at_least_one_panel_is_expanded =
			! (mConversationsPane->isCollapsed() && mMessagesPane->isCollapsed());

	setCanResize(at_least_one_panel_is_expanded);
	setCanMinimize(at_least_one_panel_is_expanded);

    assignResizeLimits();
}

void LLFloaterIMContainer::assignResizeLimits()
{
	bool is_conv_pane_expanded = !mConversationsPane->isCollapsed();
	bool is_msg_pane_expanded = !mMessagesPane->isCollapsed();

    S32 summary_width_of_visible_borders = (is_msg_pane_expanded ? mConversationsStack->getPanelSpacing() : 0) + 1;

	S32 conv_pane_target_width = is_conv_pane_expanded
		? ( is_msg_pane_expanded?mConversationsPane->getRect().getWidth():mConversationsPane->getExpandedMinDim() )
			: mConversationsPane->getMinDim();

	S32 msg_pane_min_width  = is_msg_pane_expanded ? mMessagesPane->getExpandedMinDim() : 0;
	S32 new_min_width = conv_pane_target_width + msg_pane_min_width + summary_width_of_visible_borders;

	setResizeLimits(new_min_width, getMinHeight());

	mConversationsStack->updateLayout();
}

void LLFloaterIMContainer::onAddButtonClicked()
{
    LLView * button = findChild<LLView>("conversations_pane_buttons_expanded")->findChild<LLButton>("add_btn");
    LLFloater* root_floater = gFloaterView->getParentFloater(this);
    LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(boost::bind(&LLFloaterIMContainer::onAvatarPicked, this, _1), TRUE, TRUE, TRUE, root_floater->getName(), button);
    
    if (picker && root_floater)
    {
        root_floater->addDependentFloater(picker);
    }
}

void LLFloaterIMContainer::onAvatarPicked(const uuid_vec_t& ids)
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

void LLFloaterIMContainer::onCustomAction(const LLSD& userdata)
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
		LLFloaterPreference * floater_prefp = LLFloaterReg::showTypedInstance<LLFloaterPreference>("preferences");
		if (floater_prefp)
		{
			floater_prefp->selectChatPanel();
		}
	}
	if ("privacy_preferences" == command)
	{
		LLFloaterPreference * floater_prefp = LLFloaterReg::showTypedInstance<LLFloaterPreference>("preferences");
		if (floater_prefp)
		{
			floater_prefp->selectPrivacyPanel();
		}
	}
	if ("Translating.Toggle" == command)
	{
		gSavedSettings.setBOOL("TranslateChat", !gSavedSettings.getBOOL("TranslateChat"));
	}
}

BOOL LLFloaterIMContainer::isActionChecked(const LLSD& userdata)
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
	if ("Translating.Enabled" == command)
	{
		return gSavedPerAccountSettings.getBOOL("TranslatingEnabled");
	}
	if ("Translating.On" == command)
	{
		return gSavedSettings.getBOOL("TranslateChat");
	}
	return FALSE;
}

void LLFloaterIMContainer::setSortOrderSessions(const LLConversationFilter::ESortOrderType order)
{
	LLConversationSort old_order = mConversationViewModel.getSorter();
	if (order != old_order.getSortOrderSessions())
	{
		old_order.setSortOrderSessions(order);
		setSortOrder(old_order);
	}
}

void LLFloaterIMContainer::setSortOrderParticipants(const LLConversationFilter::ESortOrderType order)
{
	LLConversationSort old_order = mConversationViewModel.getSorter();
	if (order != old_order.getSortOrderParticipants())
	{
		old_order.setSortOrderParticipants(order);
		setSortOrder(old_order);
	}
}

void LLFloaterIMContainer::setSortOrder(const LLConversationSort& order)
{
	mConversationViewModel.setSorter(order);
	mConversationsRoot->arrangeAll();
	// try to keep selection onscreen, even if it wasn't to start with
	mConversationsRoot->scrollToShowSelection();
	
	// Notify all conversation (torn off or not) of the change to the sort order
	// Note: For the moment, the sort order is *unique* across all conversations. That might change in the future.
	for (conversations_items_map::iterator it_session = mConversationsItems.begin(); it_session != mConversationsItems.end(); it_session++)
	{
		LLUUID session_id = it_session->first;
		LLFloaterIMSessionTab *conversation_floater = (session_id.isNull() ? (LLFloaterIMSessionTab*)(LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat")) : (LLFloaterIMSessionTab*)(LLFloaterIMSession::findInstance(session_id)));
		if (conversation_floater)
		{
			conversation_floater->setSortOrder(order);
		}
	}
	
	gSavedSettings.setU32("ConversationSortOrder", (U32)order);
}

void LLFloaterIMContainer::getSelectedUUIDs(uuid_vec_t& selected_uuids, bool participant_uuids/* = true*/)
{
    const std::set<LLFolderViewItem*> selectedItems = mConversationsRoot->getSelectionList();

    std::set<LLFolderViewItem*>::const_iterator it = selectedItems.begin();
    const std::set<LLFolderViewItem*>::const_iterator it_end = selectedItems.end();
    LLConversationItem * conversationItem;

    for (; it != it_end; ++it)
    {
        conversationItem = static_cast<LLConversationItem *>((*it)->getViewModelItem());
      
		//When a one-on-one conversation exists, retrieve the participant id from the conversation floater
		if(conversationItem->getType() == LLConversationItem::CONV_SESSION_1_ON_1 && participant_uuids)
		{
			LLFloaterIMSession * conversation_floaterp = LLFloaterIMSession::findInstance(conversationItem->getUUID());
			LLUUID participant_id = conversation_floaterp->getOtherParticipantUUID();
			selected_uuids.push_back(participant_id);
		}
		else
		{
			selected_uuids.push_back(conversationItem->getUUID());
		}
    }
}

const LLConversationItem * LLFloaterIMContainer::getCurSelectedViewModelItem()
{
    LLConversationItem * conversation_item = NULL;

    if(mConversationsRoot && 
        mConversationsRoot->getCurSelectedItem() && 
        mConversationsRoot->getCurSelectedItem()->getViewModelItem())
    {
		LLFloaterIMSessionTab *selected_session_floater = LLFloaterIMSessionTab::getConversation(mSelectedSession);
		if (selected_session_floater && !selected_session_floater->getHost() && selected_session_floater->getCurSelectedViewModelItem())
		{
			conversation_item = selected_session_floater->getCurSelectedViewModelItem();
		}
		else
		{
			conversation_item = static_cast<LLConversationItem *>(mConversationsRoot->getCurSelectedItem()->getViewModelItem());
		}
	}

    return conversation_item;
}

void LLFloaterIMContainer::getParticipantUUIDs(uuid_vec_t& selected_uuids)
{
    //Find the conversation floater associated with the selected id
    const LLConversationItem * conversation_item = getCurSelectedViewModelItem();

	if (NULL == conversation_item)
	{
		return;
	}

	getSelectedUUIDs(selected_uuids);  
}

void LLFloaterIMContainer::doToParticipants(const std::string& command, uuid_vec_t& selectedIDS)
{
	if (selectedIDS.size() == 1)
	{
		const LLUUID& userID = selectedIDS.front();
		if ("view_profile" == command)
		{
			LLAvatarActions::showProfile(userID);
		}
		else if ("im" == command)
		{
			if (gAgent.getID() != userID)
			{
				LLAvatarActions::startIM(userID);
			}
		}
		else if ("offer_teleport" == command)
		{
			LLAvatarActions::offerTeleport(selectedIDS);
		}
		else if ("request_teleport" == command)
		{
			LLAvatarActions::teleportRequest(selectedIDS.front());
		}
		else if ("voice_call" == command)
		{
			LLAvatarActions::startCall(userID);
		}
		else if ("chat_history" == command)
		{
			LLAvatarActions::viewChatHistory(userID);
		}
		else if ("add_friend" == command)
		{
			LLAvatarActions::requestFriendshipDialog(userID);
		}
		else if ("remove_friend" == command)
		{
			LLAvatarActions::removeFriendDialog(userID);
		}
		else if ("invite_to_group" == command)
		{
			LLAvatarActions::inviteToGroup(userID);
		}
		else if ("zoom_in" == command)
		{
			handle_zoom_to_object(userID);
		}
		else if ("map" == command)
		{
			LLAvatarActions::showOnMap(userID);
		}
		else if ("share" == command)
		{
			LLAvatarActions::share(userID);
		}
		else if ("pay" == command)
		{
			LLAvatarActions::pay(userID);
		}
		else if ("block_unblock" == command)
		{
			toggleMute(userID, LLMute::flagVoiceChat);
		}
		else if ("mute_unmute" == command)
		{
			toggleMute(userID, LLMute::flagTextChat);
		}
		else if ("selected" == command || "mute_all" == command || "unmute_all" == command)
		{
			moderateVoice(command, userID);
		}
		else if ("toggle_allow_text_chat" == command)
		{
			toggleAllowTextChat(userID);
		}
		else if ("ban_member" == command)
		{
			banSelectedMember(userID);
		}
	}
	else if (selectedIDS.size() > 1)
	{
		if ("im" == command)
		{
			LLAvatarActions::startConference(selectedIDS);
		}
		else if ("offer_teleport" == command)
		{
			LLAvatarActions::offerTeleport(selectedIDS);
		}
		else if ("voice_call" == command)
		{
			LLAvatarActions::startAdhocCall(selectedIDS);
		}
		else if ("remove_friend" == command)
		{
			LLAvatarActions::removeFriendsDialog(selectedIDS);
		}
	}
}

void LLFloaterIMContainer::doToSelectedConversation(const std::string& command, uuid_vec_t& selectedIDS)
{
    //Find the conversation floater associated with the selected id
    const LLConversationItem * conversationItem = getCurSelectedViewModelItem();
    LLFloaterIMSession *conversationFloater = LLFloaterIMSession::findInstance(conversationItem->getUUID());

    if(conversationFloater)
    {
        //Close the selected conversation
        if("close_conversation" == command)
        {
            LLFloater::onClickClose(conversationFloater);
        }
        else if("close_selected_conversations" == command)
        {
        	getSelectedUUIDs(selectedIDS,false);
        	closeSelectedConversations(selectedIDS);
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
			if (selectedIDS.size() > 0)
			{
				LLAvatarActions::viewChatHistory(selectedIDS.front());
			}
        }
        else
        {
        	if(conversationItem->getType() == LLConversationItem::CONV_SESSION_1_ON_1)
        	{
        		doToParticipants(command, selectedIDS);
        	}
        }
    }
    //if there is no LLFloaterIMSession* instance for selected conversation it might be Nearby chat
    else
    {
    	if(conversationItem->getType() == LLConversationItem::CONV_SESSION_NEARBY)
    	{
    		if("chat_history" == command)
    	    {
    	      	LLFloaterReg::showInstance("preview_conversation", LLSD(LLUUID::null), true);
    	    }
}
    }
}

void LLFloaterIMContainer::doToSelected(const LLSD& userdata)
{
    std::string command = userdata.asString();
    const LLConversationItem * conversationItem = getCurSelectedViewModelItem();
    uuid_vec_t selected_uuids;

    if(conversationItem != NULL)
    {
    	getParticipantUUIDs(selected_uuids);
		
    	if(conversationItem->getType() == LLConversationItem::CONV_PARTICIPANT)
    	{
    		doToParticipants(command, selected_uuids);
    	}
    	else
    	{
    		doToSelectedConversation(command, selected_uuids);
    	}
    }
}

void LLFloaterIMContainer::doToSelectedGroup(const LLSD& userdata)
{
    std::string action = userdata.asString();

    if (action == "group_profile")
    {
        LLGroupActions::show(mSelectedSession);
    }
    else if (action == "activate_group")
    {
        LLGroupActions::activate(mSelectedSession);
    }
    else if (action == "leave_group")
    {
        LLGroupActions::leave(mSelectedSession);
    }
}

bool LLFloaterIMContainer::enableContextMenuItem(const LLSD& userdata)
{
    const std::string& item = userdata.asString();
	uuid_vec_t uuids;
	getParticipantUUIDs(uuids);


	//If there is group or ad-hoc chat in multiselection, everything needs to be disabled
	if(uuids.size() > 1)
	{
		const std::set<LLFolderViewItem*> selectedItems = mConversationsRoot->getSelectionList();
		LLConversationItem * conversationItem;
		for(std::set<LLFolderViewItem*>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it)
		{
			conversationItem = static_cast<LLConversationItem *>((*it)->getViewModelItem());
			if((conversationItem->getType() == LLConversationItem::CONV_SESSION_GROUP) || (conversationItem->getType() == LLConversationItem::CONV_SESSION_AD_HOC))
			{
				return false;
			}
		}
	}

	if ("conversation_log" == item)
	{
		return gSavedPerAccountSettings.getS32("KeepConversationLogTranscripts") > 0;
	}

	//Enable Chat history item for ad-hoc and group conversations
	if ("can_chat_history" == item && uuids.size() > 0)
	{
		//Disable menu item if selected participant is user agent
		if(uuids.front() != gAgentID)
		{
			if (getCurSelectedViewModelItem()->getType() == LLConversationItem::CONV_SESSION_NEARBY)
			{
				return LLLogChat::isNearbyTranscriptExist();
			}
			else
			{
				bool is_group = (getCurSelectedViewModelItem()->getType() == LLConversationItem::CONV_SESSION_GROUP);
				return LLLogChat::isTranscriptExist(uuids.front(),is_group);
			}
		}
	}

	// If nothing is selected(and selected item is not group chat), everything needs to be disabled
	if (uuids.size() <= 0)
	{
		if(getCurSelectedViewModelItem())
		{
			return getCurSelectedViewModelItem()->getType() == LLConversationItem::CONV_SESSION_GROUP;
		}
		return false;
	}

	if("can_activate_group" == item)
    {
    	LLUUID selected_group_id = getCurSelectedViewModelItem()->getUUID();
    	return gAgent.getGroupID() != selected_group_id;
    }
	
	return enableContextMenuItem(item, uuids);
}

bool LLFloaterIMContainer::enableContextMenuItem(const std::string& item, uuid_vec_t& uuids)
{
	// Extract the single select info
	bool is_single_select = (uuids.size() == 1);
	const LLUUID& single_id = uuids.front();

	if ("can_chat_history" == item && is_single_select)
	{
		return LLLogChat::isTranscriptExist(uuids.front(),false);
	}

	// Handle options that are applicable to all including the user agent
    if ("can_view_profile" == item)
    {
		return is_single_select;
	}
	
	// Beyond that point, if only the user agent is selected, everything is disabled
	if (is_single_select && (single_id == gAgentID))
	{
		return false;
	}

	// If the user agent is selected with others, everything is disabled
	for (uuid_vec_t::const_iterator id = uuids.begin(); id != uuids.end(); ++id)
	{
		if (gAgent.getID() == *id)
		{
			return false;
		}
	}

	// Handle all other options
	if (("can_invite" == item) || ("can_chat_history" == item) || ("can_share" == item) || ("can_pay" == item))
	{
		// Those menu items are enable only if a single avatar is selected
		return is_single_select;
	}
    else if ("can_block" == item)
    {
        return (is_single_select ? LLAvatarActions::canBlock(single_id) : false);
    }
    else if ("can_add" == item)
    {
        // We can add friends if:
        // - there is only 1 selected avatar (EXT-7389)
        // - this avatar is not already a friend
        return (is_single_select ? !LLAvatarActions::isFriend(single_id) : false);
    }
    else if ("can_delete" == item)
    {
        // We can remove friends if there are only friends among the selection
        bool result = true;
        for (uuid_vec_t::const_iterator id = uuids.begin(); id != uuids.end(); ++id)
        {
			result &= LLAvatarActions::isFriend(*id);
        }
        return result;
    }
    else if ("can_call" == item)
    {
        return LLAvatarActions::canCall();
    }
    else if ("can_open_voice_conversation" == item)
    {
    	return is_single_select && LLAvatarActions::canCall();
    }
    else if ("can_open_voice_conversation" == item)
    {
    	return is_single_select && LLAvatarActions::canCall();
    }
	else if ("can_zoom_in" == item)
	{
		return is_single_select && gObjectList.findObject(single_id);
	}
    else if ("can_show_on_map" == item)
    {
        return (is_single_select ? (LLAvatarTracker::instance().isBuddyOnline(single_id) && is_agent_mappable(single_id)) || gAgent.isGodlike() : false);
    }
    else if ("can_offer_teleport" == item)
    {
		return LLAvatarActions::canOfferTeleport(uuids);
    }
    else if ("can_ban_member" == item)
    {
   		return canBanSelectedMember(single_id);
    }
	else if (("can_moderate_voice" == item) || ("can_allow_text_chat" == item) || ("can_mute" == item) || ("can_unmute" == item))
	{
		// *TODO : get that out of here...
		return enableModerateContextMenuItem(item);
	}

	// By default, options that not explicitely disabled are enabled
    return true;
}

bool LLFloaterIMContainer::checkContextMenuItem(const LLSD& userdata)
{
    std::string item = userdata.asString();
	uuid_vec_t uuids;
	getParticipantUUIDs(uuids);
	
	return checkContextMenuItem(item, uuids);
}

bool LLFloaterIMContainer::checkContextMenuItem(const std::string& item, uuid_vec_t& uuids)
{
    if (uuids.size() == 1)
    {
		if ("is_blocked" == item)
		{
			return LLMuteList::getInstance()->isMuted(uuids.front(), LLMute::flagVoiceChat);
		}
		else if (item == "is_muted")
		{
		    return LLMuteList::getInstance()->isMuted(uuids.front(), LLMute::flagTextChat);
	    }
		else if ("is_allowed_text_chat" == item)
		{
			const LLSpeaker * speakerp = getSpeakerOfSelectedParticipant(getSpeakerMgrForSelectedParticipant());

			if (NULL != speakerp)
			{
				return !speakerp->mModeratorMutedText;
			}
		}
    }

    return false;
}

bool LLFloaterIMContainer::visibleContextMenuItem(const LLSD& userdata)
{
	const std::string& item = userdata.asString();

	if ("show_mute" == item)
	{
		return !isMuted(getCurSelectedViewModelItem()->getUUID());
	}
	else if ("show_unmute" == item)
	{
		return isMuted(getCurSelectedViewModelItem()->getUUID());
	}

	return true;
}

void LLFloaterIMContainer::showConversation(const LLUUID& session_id)
{
    setVisibleAndFrontmost(false);
    selectConversationPair(session_id, true);

    LLFloaterIMSessionTab* session_floater = LLFloaterIMSessionTab::findConversation(session_id);
    if (session_floater)
    {
        session_floater->restoreFloater();
    }
}

void LLFloaterIMContainer::clearAllFlashStates()
{
	conversations_widgets_map::iterator widget_it = mConversationsWidgets.begin();
	for (;widget_it != mConversationsWidgets.end(); ++widget_it)
	{
		LLConversationViewSession* widget = dynamic_cast<LLConversationViewSession*>(widget_it->second);
		if (widget)
		{
			widget->setFlashState(false);
		}
	}
}

void LLFloaterIMContainer::selectConversation(const LLUUID& session_id)
{
    selectConversationPair(session_id, true);
}

// Select the conversation *after* (or before if none after) the passed uuid conversation
// Used to change the selection on key hits
void LLFloaterIMContainer::selectNextConversationByID(const LLUUID& uuid)
{
	bool new_selection = false;
	selectConversation(uuid);
	new_selection = selectNextorPreviousConversation(true);
	if (!new_selection)
	{
		selectNextorPreviousConversation(false);
	}
}

// Synchronous select the conversation item and the conversation floater
BOOL LLFloaterIMContainer::selectConversationPair(const LLUUID& session_id, bool select_widget, bool focus_floater/*=true*/)
{
    BOOL handled = TRUE;
    LLFloaterIMSessionTab* session_floater = LLFloaterIMSessionTab::findConversation(session_id);
	
    /* widget processing */
    if (select_widget && mConversationsRoot->getSelectedCount() <= 1)
    {
		LLFolderViewItem* widget = get_ptr_in_map(mConversationsWidgets,session_id);
    	if (widget && widget->getParentFolder())
    	{
    		widget->getParentFolder()->setSelection(widget, FALSE, FALSE);
    		mConversationsRoot->scrollToShowSelection();
    	}
    }

    /* floater processing */

	if (NULL != session_floater)
	{
		if (session_id != getSelectedSession())
		{
			// Store the active session
			setSelectedSession(session_id);

		

			if (session_floater->getHost())
			{
				// Always expand the message pane if the panel is hosted by the container
				collapseMessagesPane(false);
				// Switch to the conversation floater that is being selected
				selectFloater(session_floater);
			}
			else
			{
				showStub(true);
			}

			//When in DND mode, remove stored IM notifications
			//Nearby chat (Null) IMs are not stored while in DND mode, so can ignore removal
			if(gAgent.isDoNotDisturb() && session_id.notNull())
			{
				LLDoNotDisturbNotificationStorage::getInstance()->removeNotification(LLDoNotDisturbNotificationStorage::toastName, session_id);
			}
		}

		// Set the focus on the selected floater
		if (!session_floater->hasFocus() && !session_floater->isMinimized())
		{
			session_floater->setFocus(focus_floater);
		}
	}
	flashConversationItemWidget(session_id,false);
    return handled;
}

void LLFloaterIMContainer::setTimeNow(const LLUUID& session_id, const LLUUID& participant_id)
{
	LLConversationItemSession* item = dynamic_cast<LLConversationItemSession*>(get_ptr_in_map(mConversationsItems,session_id));
	if (item)
	{
		item->setTimeNow(participant_id);
		mConversationViewModel.requestSortAll();
		mConversationsRoot->arrangeAll();
	}
}

void LLFloaterIMContainer::setNearbyDistances()
{
	// Get the nearby chat session: that's the one with uuid nul
	LLConversationItemSession* item = dynamic_cast<LLConversationItemSession*>(get_ptr_in_map(mConversationsItems,LLUUID()));
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

LLConversationItem* LLFloaterIMContainer::addConversationListItem(const LLUUID& uuid, bool isWidgetSelected /*= false*/)
{
	bool is_nearby_chat = uuid.isNull();

    // Stores the display name for the conversation line item
	std::string display_name = is_nearby_chat ? LLTrans::getString("NearbyChatLabel") : LLIMModel::instance().getName(uuid);

	// Check if the item is not already in the list, exit (nothing to do)
	// Note: this happens often, when reattaching a torn off conversation for instance
	conversations_items_map::iterator item_it = mConversationsItems.find(uuid);
	if (item_it != mConversationsItems.end())
	{
		return item_it->second;
	}

	// Create a conversation session model
	LLConversationItemSession* item = NULL;
	LLSpeakerMgr* speaker_manager = (is_nearby_chat ? (LLSpeakerMgr*)(LLLocalSpeakerMgr::getInstance()) : LLIMModel::getInstance()->getSpeakerManager(uuid));
	if (speaker_manager)
	{
		item = new LLParticipantList(speaker_manager, getRootViewModel());
	}
	if (!item)
	{
		LL_WARNS() << "Couldn't create conversation session item : " << display_name << LL_ENDL;
		return NULL;
	}
	item->renameItem(display_name);
	item->updateName(NULL);
	
	mConversationsItems[uuid] = item;

	// Create a widget from it
	LLConversationViewSession* widget = createConversationItemWidget(item);
	mConversationsWidgets[uuid] = widget;

	// Add a new conversation widget to the root folder of the folder view
	widget->addToFolder(mConversationsRoot);
	widget->requestArrange();

	LLIMModel::LLIMSession * im_sessionp = LLIMModel::getInstance()->findIMSession(uuid);

	// Create the participants widgets now
	// Note: usually, we do not get an updated avatar list at that point
	if (uuid.isNull() || (im_sessionp && !im_sessionp->isP2PSessionType()))
	{
		LLFolderViewModelItemCommon::child_list_t::const_iterator current_participant_model = item->getChildrenBegin();
		LLFolderViewModelItemCommon::child_list_t::const_iterator end_participant_model = item->getChildrenEnd();
		while (current_participant_model != end_participant_model)
		{
			LLConversationItem* participant_model = dynamic_cast<LLConversationItem*>(*current_participant_model);
			LLConversationViewParticipant* participant_view = createConversationViewParticipant(participant_model);
			participant_view->addToFolder(widget);
			current_participant_model++;
		}
	}

	if (uuid.notNull() && im_sessionp->isP2PSessionType())
	{
		item->fetchAvatarName(false);
	}

	// Do that too for the conversation dialog
    LLFloaterIMSessionTab *conversation_floater = (uuid.isNull() ? (LLFloaterIMSessionTab*)(LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat")) : (LLFloaterIMSessionTab*)(LLFloaterIMSession::findInstance(uuid)));
	if (conversation_floater)
	{
		conversation_floater->buildConversationViewParticipant();
	}

	// set the widget to minimized mode if conversations pane is collapsed
	widget->toggleCollapsedMode(mConversationsPane->isCollapsed());

	if (isWidgetSelected || 0 == mConversationsRoot->getSelectedCount())
	{
		selectConversationPair(uuid, true);
		widget->requestArrange();

		// scroll to newly added item
		mConversationsRoot->scrollToShowSelection();
	}

	return item;
}

bool LLFloaterIMContainer::removeConversationListItem(const LLUUID& uuid, bool change_focus)
{
	// Delete the widget and the associated conversation item
	// Note : since the mConversationsItems is also the listener to the widget, deleting 
	// the widget will also delete its listener
	bool is_widget_selected = false;
	LLFolderViewItem* new_selection = NULL;
	LLFolderViewItem* widget = get_ptr_in_map(mConversationsWidgets,uuid);
	if (widget)
	{
		is_widget_selected = widget->isSelected();
		new_selection = mConversationsRoot->getNextFromChild(widget, FALSE);
		if (!new_selection)
		{
			new_selection = mConversationsRoot->getPreviousFromChild(widget, FALSE);
		}
		widget->destroyView();
	}
	
	// Suppress the conversation items and widgets from their respective maps
	mConversationsItems.erase(uuid);
	mConversationsWidgets.erase(uuid);
	
	// Don't let the focus fall IW, select and refocus on the first conversation in the list
	if (change_focus)
	{
		setFocus(TRUE);
		if (new_selection)
		{
			if (mConversationsWidgets.size() == 1)
			{
				// If only one widget is left, it has to be the Nearby Chat. Select it directly.
				selectConversationPair(LLUUID(NULL), true);
			}
			else
			{
				LLConversationItem* vmi = dynamic_cast<LLConversationItem*>(new_selection->getViewModelItem());
				if (vmi)
				{
					selectConversationPair(vmi->getUUID(), true);
				}
			}
		}
	}
	return is_widget_selected;
}

LLConversationViewSession* LLFloaterIMContainer::createConversationItemWidget(LLConversationItem* item)
{
	LLConversationViewSession::Params params;
	
	params.name = item->getDisplayName();
	params.root = mConversationsRoot;
	params.listener = item;
	params.tool_tip = params.name;
	params.container = this;
	
    //Indentation for aligning the p2p converstation image with the nearby chat arrow
    if(item->getType() == LLConversationItem::CONV_SESSION_1_ON_1)
    {
        params.folder_indentation = 3;
    }

	return LLUICtrlFactory::create<LLConversationViewSession>(params);
}

LLConversationViewParticipant* LLFloaterIMContainer::createConversationViewParticipant(LLConversationItem* item)
{
	LLConversationViewParticipant::Params params;
    LLRect panel_rect = mConversationsListPanel->getRect();
	
	params.name = item->getDisplayName();
	params.root = mConversationsRoot;
	params.listener = item;

    //24 is the the current hight of an item (itemHeight) loaded from conversation_view_participant.xml.
	params.rect = LLRect (0, 24, panel_rect.getWidth(), 0);
	params.tool_tip = params.name;
	params.participant_id = item->getUUID();
    params.folder_indentation = 27;

	return LLUICtrlFactory::create<LLConversationViewParticipant>(params);
}

bool LLFloaterIMContainer::enableModerateContextMenuItem(const std::string& userdata)
{
	// only group moderators can perform actions related to this "enable callback"
	if (!isGroupModerator())
	{
		return false;
	}

	LLSpeaker * speakerp = getSpeakerOfSelectedParticipant(getSpeakerMgrForSelectedParticipant());
	if (NULL == speakerp)
	{
		return false;
	}

	bool voice_channel = speakerp->isInVoiceChannel();

	if ("can_moderate_voice" == userdata)
	{
		return voice_channel;
	}
	else if ("can_mute" == userdata)
	{
		return voice_channel && !isMuted(getCurSelectedViewModelItem()->getUUID());
	}
	else if ("can_unmute" == userdata)
	{
		return voice_channel && isMuted(getCurSelectedViewModelItem()->getUUID());
	}

	// The last invoke is used to check whether the "can_allow_text_chat" will enabled
	return LLVoiceClient::getInstance()->isParticipantAvatar(getCurSelectedViewModelItem()->getUUID());
}

bool LLFloaterIMContainer::isGroupModerator()
{
	LLSpeakerMgr * speaker_manager = getSpeakerMgrForSelectedParticipant();
	if (NULL == speaker_manager)
	{
		LL_WARNS() << "Speaker manager is missing" << LL_ENDL;
		return false;
	}

	// Is session a group call/chat?
	if(gAgent.isInGroup(speaker_manager->getSessionID()))
	{
		LLSpeaker * speaker = speaker_manager->findSpeaker(gAgentID).get();

		// Is agent a moderator?
		return speaker && speaker->mIsModerator;
	}

	return false;
}

bool LLFloaterIMContainer::haveAbilityToBan()
{
	LLSpeakerMgr * speaker_manager = getSpeakerMgrForSelectedParticipant();
	if (NULL == speaker_manager)
	{
		LL_WARNS() << "Speaker manager is missing" << LL_ENDL;
		return false;
	}
	LLUUID group_uuid = speaker_manager->getSessionID();

	return gAgent.isInGroup(group_uuid) && gAgent.hasPowerInGroup(group_uuid, GP_GROUP_BAN_ACCESS);
}

bool LLFloaterIMContainer::canBanSelectedMember(const LLUUID& participant_uuid)
{
	LLSpeakerMgr * speaker_manager = getSpeakerMgrForSelectedParticipant();
	if (NULL == speaker_manager)
	{
		LL_WARNS() << "Speaker manager is missing" << LL_ENDL;
		return false;
	}
	LLUUID group_uuid = speaker_manager->getSessionID();
	LLGroupMgrGroupData* gdatap	= LLGroupMgr::getInstance()->getGroupData(group_uuid);
	if(!gdatap)
	{
		LL_WARNS("Groups") << "Unable to get group data for group " << group_uuid << LL_ENDL;
		return false;
	}

	if (!gdatap->mMembers.size())
	{
		return false;
	}

	LLGroupMgrGroupData::member_list_t::iterator mi = gdatap->mMembers.find((participant_uuid));
	if (mi == gdatap->mMembers.end())
	{
		return false;
	}

	LLGroupMemberData* member_data = (*mi).second;
	// Is the member an owner?
	if ( member_data && member_data->isInRole(gdatap->mOwnerRole) )
	{
		return false;
	}

	if(	gAgent.hasPowerInGroup(group_uuid, GP_ROLE_REMOVE_MEMBER) &&
		gAgent.hasPowerInGroup(group_uuid, GP_GROUP_BAN_ACCESS)	)
	{
		return true;
	}

	return false;
}

void LLFloaterIMContainer::banSelectedMember(const LLUUID& participant_uuid)
{
	LLSpeakerMgr * speaker_manager = getSpeakerMgrForSelectedParticipant();
	if (NULL == speaker_manager)
	{
		LL_WARNS() << "Speaker manager is missing" << LL_ENDL;
		return;
	}

	LLUUID group_uuid = speaker_manager->getSessionID();
	LLGroupMgrGroupData* gdatap	= LLGroupMgr::getInstance()->getGroupData(group_uuid);
	if(!gdatap)
	{
		LL_WARNS("Groups") << "Unable to get group data for group " << group_uuid << LL_ENDL;
		return;
	}
	std::vector<LLUUID> ids;
	ids.push_back(participant_uuid);

	LLGroupBanData ban_data;
	gdatap->createBanEntry(participant_uuid, ban_data);
	LLGroupMgr::getInstance()->sendGroupBanRequest(LLGroupMgr::REQUEST_POST, group_uuid, LLGroupMgr::BAN_CREATE, ids);
	LLGroupMgr::getInstance()->sendGroupMemberEjects(group_uuid, ids);
	LLGroupMgr::getInstance()->sendGroupMembersRequest(group_uuid);
	LLSD args;
	std::string name;
	gCacheName->getFullName(participant_uuid, name);
	args["AVATAR_NAME"] = name;
	args["GROUP_NAME"] = gdatap->mName;
	LLNotifications::instance().add(LLNotification::Params("EjectAvatarFromGroup").substitutions(args));

}

void LLFloaterIMContainer::moderateVoice(const std::string& command, const LLUUID& userID)
{
	if (!gAgent.getRegion()) return;

	if (command.compare("selected"))
	{
		moderateVoiceAllParticipants(command.compare("mute_all"));
	}
	else
	{
		moderateVoiceParticipant(userID, isMuted(userID));
	}
}

bool LLFloaterIMContainer::isMuted(const LLUUID& avatar_id)
{
	const LLSpeaker * speakerp = getSpeakerOfSelectedParticipant(getSpeakerMgrForSelectedParticipant());
	return NULL == speakerp ? true : speakerp->mStatus == LLSpeaker::STATUS_MUTED;
}

void LLFloaterIMContainer::moderateVoiceAllParticipants(bool unmute)
{
	LLIMSpeakerMgr * speaker_managerp = dynamic_cast<LLIMSpeakerMgr*>(getSpeakerMgrForSelectedParticipant());

	if (NULL != speaker_managerp)
	{
		if (!unmute)
		{
			LLSD payload;
			payload["session_id"] = speaker_managerp->getSessionID();
			LLNotificationsUtil::add("ConfirmMuteAll", LLSD(), payload, confirmMuteAllCallback);
			return;
		}

		speaker_managerp->moderateVoiceAllParticipants(unmute);
	}
}

// static
void LLFloaterIMContainer::confirmMuteAllCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	// if Cancel pressed
	if (option == 1)
	{
		return;
	}

	const LLSD& payload = notification["payload"];
	const LLUUID& session_id = payload["session_id"];

	LLIMSpeakerMgr * speaker_manager = dynamic_cast<LLIMSpeakerMgr*> (
		LLIMModel::getInstance()->getSpeakerManager(session_id));
	if (speaker_manager)
	{
		speaker_manager->moderateVoiceAllParticipants(false);
	}

	return;
}

void LLFloaterIMContainer::moderateVoiceParticipant(const LLUUID& avatar_id, bool unmute)
{
	LLIMSpeakerMgr * speaker_managerp = dynamic_cast<LLIMSpeakerMgr *>(getSpeakerMgrForSelectedParticipant());

	if (NULL != speaker_managerp)
	{
		speaker_managerp->moderateVoiceParticipant(avatar_id, unmute);
	}
}

LLSpeakerMgr * LLFloaterIMContainer::getSpeakerMgrForSelectedParticipant()
{
	LLFolderViewItem *selectedItem = mConversationsRoot->getCurSelectedItem();
	if (NULL == selectedItem)
	{
		LL_WARNS() << "Current selected item is null" << LL_ENDL;
		return NULL;
	}

	conversations_widgets_map::const_iterator iter = mConversationsWidgets.begin();
	conversations_widgets_map::const_iterator end = mConversationsWidgets.end();
	const LLUUID * conversation_uuidp = NULL;
	while(iter != end)
	{
		if (iter->second == selectedItem || iter->second == selectedItem->getParentFolder())
		{
			conversation_uuidp = &iter->first;
			break;
		}
		++iter;
	}
	if (NULL == conversation_uuidp)
	{
		LL_WARNS() << "Cannot find conversation item widget" << LL_ENDL;
		return NULL;
	}

	return conversation_uuidp->isNull() ? (LLSpeakerMgr *)LLLocalSpeakerMgr::getInstance()
		: LLIMModel::getInstance()->getSpeakerManager(*conversation_uuidp);
}

LLSpeaker * LLFloaterIMContainer::getSpeakerOfSelectedParticipant(LLSpeakerMgr * speaker_managerp)
{
	if (NULL == speaker_managerp)
	{
		LL_WARNS() << "Speaker manager is missing" << LL_ENDL;
		return NULL;
	}

	const LLConversationItem * participant_itemp = getCurSelectedViewModelItem();
	if (NULL == participant_itemp)
	{
		LL_WARNS() << "Cannot evaluate current selected view model item" << LL_ENDL;
		return NULL;
	}

	return speaker_managerp->findSpeaker(participant_itemp->getUUID());
}

void LLFloaterIMContainer::toggleAllowTextChat(const LLUUID& participant_uuid)
{
	LLIMSpeakerMgr * speaker_managerp = dynamic_cast<LLIMSpeakerMgr*>(getSpeakerMgrForSelectedParticipant());
	if (NULL != speaker_managerp)
	{
		speaker_managerp->toggleAllowTextChat(participant_uuid);
	}
}

void LLFloaterIMContainer::toggleMute(const LLUUID& participant_id, U32 flags)
{
        BOOL is_muted = LLMuteList::getInstance()->isMuted(participant_id, flags);
        std::string name;
        gCacheName->getFullName(participant_id, name);
        LLMute mute(participant_id, name, LLMute::AGENT);

        if (!is_muted)
        {
                LLMuteList::getInstance()->add(mute, flags);
        }
        else
        {
                LLMuteList::getInstance()->remove(mute, flags);
        }
}

void LLFloaterIMContainer::openNearbyChat()
{
	// If there's only one conversation in the container and that conversation is the nearby chat
	//(which it should be...), open it so to make the list of participants visible. This happens to be the most common case when opening the Chat floater.
	if((mConversationsItems.size() == 1)&&(!mConversationsPane->isCollapsed()))
	{
		LLConversationViewSession* nearby_chat = dynamic_cast<LLConversationViewSession*>(get_ptr_in_map(mConversationsWidgets,LLUUID()));
		if (nearby_chat)
		{
			reSelectConversation();
			nearby_chat->setOpen(TRUE);
		}
	}
}

void LLFloaterIMContainer::reSelectConversation()
{
	LLFloaterIMSessionTab* session_floater = LLFloaterIMSessionTab::getConversation(mSelectedSession);
	if (session_floater->getHost())
	{
		selectFloater(session_floater);
	}
}

void LLFloaterIMContainer::updateSpeakBtnState()
{
	mSpeakBtn->setToggleState(LLVoiceClient::getInstance()->getUserPTTState());
	mSpeakBtn->setEnabled(LLAgent::isActionAllowed("speak"));
}

bool LLFloaterIMContainer::isConversationLoggingAllowed()
{
	return gSavedPerAccountSettings.getS32("KeepConversationLogTranscripts") > 0;
}

void LLFloaterIMContainer::flashConversationItemWidget(const LLUUID& session_id, bool is_flashes)
{
    //Finds the conversation line item to flash using the session_id
	LLConversationViewSession * widget = dynamic_cast<LLConversationViewSession *>(get_ptr_in_map(mConversationsWidgets,session_id));

	if (widget)
	{
		widget->setFlashState(is_flashes);
	}
}

void LLFloaterIMContainer::highlightConversationItemWidget(const LLUUID& session_id, bool is_highlighted)
{
	//Finds the conversation line item to highlight using the session_id
	LLConversationViewSession * widget = dynamic_cast<LLConversationViewSession *>(get_ptr_in_map(mConversationsWidgets,session_id));

	if (widget)
	{
		widget->setHighlightState(is_highlighted);
	}
}

bool LLFloaterIMContainer::isScrolledOutOfSight(LLConversationViewSession* conversation_item_widget)
{
	llassert(conversation_item_widget != NULL);

	// make sure the widget is actually in the right spot first
	mConversationsRoot->arrange(NULL, NULL);

	// check whether the widget is in the visible portion of the scroll container
	LLRect widget_rect;
	conversation_item_widget->localRectToOtherView(conversation_item_widget->getLocalRect(), &widget_rect, mConversationsRoot);
	return !mConversationsRoot->getVisibleRect().overlaps(widget_rect);
}

BOOL LLFloaterIMContainer::handleKeyHere(KEY key, MASK mask )
{
	BOOL handled = FALSE;

	if(mask == MASK_ALT)
	{
		if (KEY_RETURN == key )
		{
			expandConversation();
			handled = TRUE;
		}

		if ((KEY_DOWN == key ) || (KEY_RIGHT == key))
		{
			selectNextorPreviousConversation(true);
			handled = TRUE;
		}
		if ((KEY_UP == key) || (KEY_LEFT == key))
		{
			selectNextorPreviousConversation(false);
			handled = TRUE;
		}
	}
	return handled;
}

bool LLFloaterIMContainer::selectAdjacentConversation(bool focus_selected)
{
	bool selectedAdjacentConversation = selectNextorPreviousConversation(true, focus_selected);

	if(!selectedAdjacentConversation)
	{
		selectedAdjacentConversation = selectNextorPreviousConversation(false, focus_selected);
	}

	return selectedAdjacentConversation;
}

bool LLFloaterIMContainer::selectNextorPreviousConversation(bool select_next, bool focus_selected)
{
	if (mConversationsWidgets.size() > 1)
	{
		LLFolderViewItem* new_selection = NULL;
		LLFolderViewItem* widget = get_ptr_in_map(mConversationsWidgets,getSelectedSession());
		if (widget)
		{
			if(select_next)
			{
				new_selection = mConversationsRoot->getNextFromChild(widget, FALSE);
			}
			else
			{
				new_selection = mConversationsRoot->getPreviousFromChild(widget, FALSE);
			}
			if (new_selection)
			{
				LLConversationItem* vmi = dynamic_cast<LLConversationItem*>(new_selection->getViewModelItem());
				if (vmi)
				{
					selectConversationPair(vmi->getUUID(), true, focus_selected);
					return true;
				}
			}
		}
	}
	return false;
}

void LLFloaterIMContainer::expandConversation()
{
	if(!mConversationsPane->isCollapsed())
	{
		LLConversationViewSession* widget = dynamic_cast<LLConversationViewSession*>(get_ptr_in_map(mConversationsWidgets,getSelectedSession()));
		if (widget)
		{
			widget->setOpen(!widget->isOpen());
		}
	}
}
bool LLFloaterIMContainer::isParticipantListExpanded()
{
	bool is_expanded = false;
	if(!mConversationsPane->isCollapsed())
	{
		LLConversationViewSession* widget = dynamic_cast<LLConversationViewSession*>(get_ptr_in_map(mConversationsWidgets,getSelectedSession()));
		if (widget)
		{
			is_expanded = widget->isOpen();
		}
	}
	return is_expanded;
}

// By default, if torn off session is currently frontmost, LLFloater::isFrontmost() will return FALSE, which can lead to some bugs
// So LLFloater::isFrontmost() is overriden here to check both selected session and the IM floater itself
// Exclude "Nearby Chat" session from the check, as "Nearby Chat" window and "Conversations" floater can be brought
// to front independently
/*virtual*/
BOOL LLFloaterIMContainer::isFrontmost()
{
	LLFloaterIMSessionTab* selected_session = LLFloaterIMSessionTab::getConversation(mSelectedSession);
	LLFloaterIMNearbyChat* nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
	return (selected_session && selected_session->isFrontmost() && (selected_session != nearby_chat))
		|| LLFloater::isFrontmost();
}

// For conversations, closeFloater() (linked to Ctrl-W) does not actually close the floater but the active conversation.
// This is intentional so it doesn't confuse the user. onClickCloseBtn() closes the whole floater.
void LLFloaterIMContainer::onClickCloseBtn(bool app_quitting/* = false*/)
{
	LLMultiFloater::closeFloater(app_quitting);
}

void LLFloaterIMContainer::closeHostedFloater()
{
	onClickCloseBtn();
}

void LLFloaterIMContainer::closeAllConversations()
{
	std::vector<LLUUID> ids;
	for (conversations_items_map::iterator it_session = mConversationsItems.begin(); it_session != mConversationsItems.end(); it_session++)
	{
		LLUUID session_id = it_session->first;
		if (session_id != LLUUID())
		{
			ids.push_back(session_id);
		}
	}

	for (std::vector<LLUUID>::const_iterator it = ids.begin(); it != ids.end(); 	++it)
	{
		LLFloaterIMSession *conversationFloater = LLFloaterIMSession::findInstance(*it);
		LLFloater::onClickClose(conversationFloater);
	}
}

void LLFloaterIMContainer::closeSelectedConversations(const uuid_vec_t& ids)
{
	for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		//We don't need to close Nearby chat, so skip it
		if (*it != LLUUID())
		{
			LLFloaterIMSession *conversationFloater = LLFloaterIMSession::findInstance(*it);
			if(conversationFloater)
			{
				LLFloater::onClickClose(conversationFloater);
			}
		}
	}
}
void LLFloaterIMContainer::closeFloater(bool app_quitting/* = false*/)
{
	if(app_quitting)
	{
		closeAllConversations();
		onClickCloseBtn(app_quitting);
	}
	else
	{
		// Check for currently active session
		LLUUID session_id = getSelectedSession();
		// If current session is Nearby Chat or there is only one session remaining, close the floater
		if (mConversationsItems.size() == 1 || session_id == LLUUID() || app_quitting)
		{
			onClickCloseBtn();
		}
		else
		{
			// Otherwise, close current conversation
			LLFloaterIMSessionTab* active_conversation = LLFloaterIMSessionTab::getConversation(session_id);
			if (active_conversation)
			{
				active_conversation->closeFloater();
			}
		}
	}
}

void LLFloaterIMContainer::handleReshape(const LLRect& rect, bool by_user)
{
	LLMultiFloater::handleReshape(rect, by_user);
	storeRectControl();
}

// EOF

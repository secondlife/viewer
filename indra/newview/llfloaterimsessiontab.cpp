/**
 * @file llfloaterimsessiontab.cpp
 * @brief LLFloaterIMSessionTab class implements the common behavior of LNearbyChatBar
 * @brief and LLFloaterIMSession for hosting both in LLIMContainer
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "llfloaterimsessiontab.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llavataractions.h"
#include "llchatentry.h"
#include "llchathistory.h"
#include "llchiclet.h"
#include "llchicletbar.h"
#include "lldraghandle.h"
#include "llfloaterreg.h"
#include "llfloaterimsession.h"
#include "llfloaterimcontainer.h" // to replace separate IM Floaters with multifloater container
#include "lllayoutstack.h"
#include "lltoolbarview.h"
#include "llfloaterimnearbychat.h"

const F32 REFRESH_INTERVAL = 1.0f;

void cb_group_do_nothing()
{
}

LLFloaterIMSessionTab::LLFloaterIMSessionTab(const LLSD& session_id)
:	LLTransientDockableFloater(NULL, false, session_id),
	mIsP2PChat(false),
	mExpandCollapseBtn(NULL),
	mTearOffBtn(NULL),
	mCloseBtn(NULL),
	mSessionID(session_id.asUUID()),
	mConversationsRoot(NULL),
	mScroller(NULL),
	mChatHistory(NULL),
	mInputEditor(NULL),
	mInputEditorPad(0),
	mRefreshTimer(new LLTimer()),
	mIsHostAttached(false),
	mHasVisibleBeenInitialized(false),
	mIsParticipantListExpanded(true),
	mChatLayoutPanel(NULL),
	mInputPanels(NULL),
	mChatLayoutPanelHeight(0)
{
    setAutoFocus(FALSE);
	mSession = LLIMModel::getInstance()->findIMSession(mSessionID);

	mCommitCallbackRegistrar.add("IMSession.Menu.Action",
			boost::bind(&LLFloaterIMSessionTab::onIMSessionMenuItemClicked,  this, _2));
	mEnableCallbackRegistrar.add("IMSession.Menu.CompactExpandedModes.CheckItem",
			boost::bind(&LLFloaterIMSessionTab::onIMCompactExpandedMenuItemCheck, this, _2));
	mEnableCallbackRegistrar.add("IMSession.Menu.ShowModes.CheckItem",
			boost::bind(&LLFloaterIMSessionTab::onIMShowModesMenuItemCheck,   this, _2));
	mEnableCallbackRegistrar.add("IMSession.Menu.ShowModes.Enable",
			boost::bind(&LLFloaterIMSessionTab::onIMShowModesMenuItemEnable,  this, _2));

	// Right click menu handling
    mEnableCallbackRegistrar.add("Avatar.CheckItem",  boost::bind(&LLFloaterIMSessionTab::checkContextMenuItem,	this, _2));
    mEnableCallbackRegistrar.add("Avatar.EnableItem", boost::bind(&LLFloaterIMSessionTab::enableContextMenuItem, this, _2));
    mCommitCallbackRegistrar.add("Avatar.DoToSelected", boost::bind(&LLFloaterIMSessionTab::doToSelected, this, _2));
    mCommitCallbackRegistrar.add("Group.DoToSelected", boost::bind(&cb_group_do_nothing));
}

LLFloaterIMSessionTab::~LLFloaterIMSessionTab()
{
	delete mRefreshTimer;
}

//static
LLFloaterIMSessionTab* LLFloaterIMSessionTab::findConversation(const LLUUID& uuid)
{
	LLFloaterIMSessionTab* conv;

	if (uuid.isNull())
	{
		conv = LLFloaterReg::findTypedInstance<LLFloaterIMSessionTab>("nearby_chat");
	}
	else
	{
		conv = LLFloaterReg::findTypedInstance<LLFloaterIMSessionTab>("impanel", LLSD(uuid));
	}

	return conv;
};

//static
LLFloaterIMSessionTab* LLFloaterIMSessionTab::getConversation(const LLUUID& uuid)
{
	LLFloaterIMSessionTab* conv;

	if (uuid.isNull())
	{
		conv = LLFloaterReg::getTypedInstance<LLFloaterIMSessionTab>("nearby_chat");
	}
	else
	{
		conv = LLFloaterReg::getTypedInstance<LLFloaterIMSessionTab>("impanel", LLSD(uuid));
		conv->setOpenPositioning(LLFloaterEnums::POSITIONING_RELATIVE);
	}

	return conv;
};

void LLFloaterIMSessionTab::setVisible(BOOL visible)
{
	if(visible && !mHasVisibleBeenInitialized)
	{
		mHasVisibleBeenInitialized = true;
		if(!gAgentCamera.cameraMouselook())
		{
			LLFloaterReg::getTypedInstance<LLFloaterIMContainer>("im_container")->setVisible(true);
		}
		LLFloaterIMSessionTab::addToHost(mSessionID);
		LLFloaterIMSessionTab* conversp = LLFloaterIMSessionTab::getConversation(mSessionID);

		if (conversp && conversp->isNearbyChat() && gSavedPerAccountSettings.getBOOL("NearbyChatIsNotCollapsed"))
		{
			onCollapseToLine(this);
		}
		mInputButtonPanel->setVisible(isTornOff());
	}

	LLTransientDockableFloater::setVisible(visible);
}

/*virtual*/
void LLFloaterIMSessionTab::setFocus(BOOL focus)
{
	LLTransientDockableFloater::setFocus(focus);

    //Redirect focus to input editor
    if (focus)
	{
    	updateMessages();

        if (mInputEditor)
        {
    	    mInputEditor->setFocus(TRUE);
        }
	}
}


void LLFloaterIMSessionTab::addToHost(const LLUUID& session_id)
{
	if ((session_id.notNull() && !gIMMgr->hasSession(session_id))
			|| !LLFloaterIMSessionTab::isChatMultiTab())
	{
		return;
	}

	// Get the floater: this will create the instance if it didn't exist
	LLFloaterIMSessionTab* conversp = LLFloaterIMSessionTab::getConversation(session_id);
	if (conversp)
	{
		LLFloaterIMContainer* floater_container = LLFloaterIMContainer::getInstance();

		// Do not add again existing floaters
		if (floater_container && !conversp->isHostAttached())
		{
			conversp->setHostAttached(true);

			if (!conversp->isNearbyChat()
					|| gSavedPerAccountSettings.getBOOL("NearbyChatIsNotTornOff"))
			{
				floater_container->addFloater(conversp, false, LLTabContainer::RIGHT_OF_CURRENT);
			}
			else
			{
				// setting of the "potential" host for Nearby Chat: this sequence sets
				// LLFloater::mHostHandle = NULL (a current host), but
				// LLFloater::mLastHostHandle = floater_container (a "future" host)
				conversp->setHost(floater_container);
				conversp->setHost(NULL);

				conversp->forceReshape();
			}
			// Added floaters share some state (like sort order) with their host
			conversp->setSortOrder(floater_container->getSortOrder());
		}
	}
}

void LLFloaterIMSessionTab::assignResizeLimits()
{
	bool is_participants_pane_collapsed = mParticipantListPanel->isCollapsed();

    // disable a layoutstack's functionality when participant list panel is collapsed
	mRightPartPanel->setIgnoreReshape(is_participants_pane_collapsed);

    S32 participants_pane_target_width = is_participants_pane_collapsed?
    		0 : (mParticipantListPanel->getRect().getWidth() + mParticipantListAndHistoryStack->getPanelSpacing());

    S32 new_min_width = participants_pane_target_width + mRightPartPanel->getExpandedMinDim() + mFloaterExtraWidth;

	setResizeLimits(new_min_width, getMinHeight());

	this->mParticipantListAndHistoryStack->updateLayout();
}

BOOL LLFloaterIMSessionTab::postBuild()
{
	BOOL result;

	mBodyStack = getChild<LLLayoutStack>("main_stack");
    mParticipantListAndHistoryStack = getChild<LLLayoutStack>("im_panels");

	mCloseBtn = getChild<LLButton>("close_btn");
	mCloseBtn->setCommitCallback(boost::bind(&LLFloater::onClickClose, this));

	mExpandCollapseBtn = getChild<LLButton>("expand_collapse_btn");
	mExpandCollapseBtn->setClickedCallback(boost::bind(&LLFloaterIMSessionTab::onSlide, this));

	mExpandCollapseLineBtn = getChild<LLButton>("minz_btn");
	mExpandCollapseLineBtn->setClickedCallback(boost::bind(&LLFloaterIMSessionTab::onCollapseToLine, this));

	mTearOffBtn = getChild<LLButton>("tear_off_btn");
	mTearOffBtn->setCommitCallback(boost::bind(&LLFloaterIMSessionTab::onTearOffClicked, this));

	mGearBtn = getChild<LLButton>("gear_btn");
    mAddBtn = getChild<LLButton>("add_btn");
	mVoiceButton = getChild<LLButton>("voice_call_btn");
    mTranslationCheckBox = getChild<LLUICtrl>("translate_chat_checkbox_lp");
    
	mParticipantListPanel = getChild<LLLayoutPanel>("speakers_list_panel");
	mRightPartPanel = getChild<LLLayoutPanel>("right_part_holder");

	mToolbarPanel = getChild<LLLayoutPanel>("toolbar_panel");
	mContentPanel = getChild<LLLayoutPanel>("body_panel");
	mInputButtonPanel = getChild<LLLayoutPanel>("input_button_layout_panel");
	mInputButtonPanel->setVisible(false);
	// Add a scroller for the folder (participant) view
	LLRect scroller_view_rect = mParticipantListPanel->getRect();
	scroller_view_rect.translate(-scroller_view_rect.mLeft, -scroller_view_rect.mBottom);
	LLScrollContainer::Params scroller_params(LLUICtrlFactory::getDefaultParams<LLFolderViewScrollContainer>());
	scroller_params.rect(scroller_view_rect);
	mScroller = LLUICtrlFactory::create<LLFolderViewScrollContainer>(scroller_params);
	mScroller->setFollowsAll();

	// Insert that scroller into the panel widgets hierarchy
	mParticipantListPanel->addChild(mScroller);	
	
	mChatHistory = getChild<LLChatHistory>("chat_history");

	mInputEditor = getChild<LLChatEntry>("chat_editor");

	mChatLayoutPanel = getChild<LLLayoutPanel>("chat_layout_panel");
	mInputPanels = getChild<LLLayoutStack>("input_panels");
	
	mInputEditor->setTextExpandedCallback(boost::bind(&LLFloaterIMSessionTab::reshapeChatLayoutPanel, this));
	mInputEditor->setMouseUpCallback(boost::bind(&LLFloaterIMSessionTab::onInputEditorClicked, this));
	mInputEditor->setCommitOnFocusLost( FALSE );
	mInputEditor->setPassDelete(TRUE);
	mInputEditor->setFont(LLViewerChat::getChatFont());

	mChatLayoutPanelHeight = mChatLayoutPanel->getRect().getHeight();
	mInputEditorPad = mChatLayoutPanelHeight - mInputEditor->getRect().getHeight();

	setOpenPositioning(LLFloaterEnums::POSITIONING_RELATIVE);

	mSaveRect = isNearbyChat()
					&&  !gSavedPerAccountSettings.getBOOL("NearbyChatIsNotTornOff");
	initRectControl();

	if (isChatMultiTab())
	{
		result = LLFloater::postBuild();
	}
	else
	{
		result = LLDockableFloater::postBuild();
	}

	// Create the root using an ad-hoc base item
	LLConversationItem* base_item = new LLConversationItem(mSessionID, mConversationViewModel);
    LLFolderView::Params p(LLUICtrlFactory::getDefaultParams<LLFolderView>());
    p.rect = LLRect(0, 0, getRect().getWidth(), 0);
    p.parent_panel = mParticipantListPanel;
    p.listener = base_item;
    p.view_model = &mConversationViewModel;
    p.root = NULL;
    p.use_ellipses = true;
    p.options_menu = "menu_conversation.xml";
    p.name = "root";
	mConversationsRoot = LLUICtrlFactory::create<LLFolderView>(p);
    mConversationsRoot->setCallbackRegistrar(&mCommitCallbackRegistrar);
	// Attach that root to the scroller
	mScroller->addChild(mConversationsRoot);
	mConversationsRoot->setScrollContainer(mScroller);
	mConversationsRoot->setFollowsAll();
	mConversationsRoot->addChild(mConversationsRoot->mStatusTextBox);

	setMessagePaneExpanded(true);

	buildConversationViewParticipant();
	refreshConversation();

	// Zero expiry time is set only once to allow initial update.
	mRefreshTimer->setTimerExpirySec(0);
	mRefreshTimer->start();
	initBtns();

	if (mIsParticipantListExpanded != (bool)gSavedSettings.getBOOL("IMShowControlPanel"))
	{
		LLFloaterIMSessionTab::onSlide(this);
	}

	// The resize limits for LLFloaterIMSessionTab should be updated, based on current values of width of conversation and message panels
	mParticipantListPanel->getResizeBar()->setResizeListener(boost::bind(&LLFloaterIMSessionTab::assignResizeLimits, this));
	mFloaterExtraWidth =
			getRect().getWidth()
			- mParticipantListAndHistoryStack->getRect().getWidth()
			- (mParticipantListPanel->isCollapsed()? 0 : LLPANEL_BORDER_WIDTH);

	assignResizeLimits();

	return result;
}

LLParticipantList* LLFloaterIMSessionTab::getParticipantList()
{
	return dynamic_cast<LLParticipantList*>(LLFloaterIMContainer::getInstance()->getSessionModel(mSessionID));
}

void LLFloaterIMSessionTab::draw()
{
	if (mRefreshTimer->hasExpired())
	{
		LLParticipantList* item = getParticipantList();
		if (item)
		{
			// Update all model items
			item->update();
			// If the model and view list diverge in count, rebuild
			// Note: this happens sometimes right around init (add participant events fire but get dropped) and is the cause
			// of missing participants, often, the user agent itself. As there will be no other event fired, there's
			// no other choice but get those inconsistencies regularly (and lightly) checked and scrubbed.
			if (item->getChildrenCount() != mConversationsWidgets.size())
			{
				buildConversationViewParticipant();
			}
			refreshConversation();
		}

		// Restart the refresh timer
		mRefreshTimer->setTimerExpirySec(REFRESH_INTERVAL);
	}

	LLTransientDockableFloater::draw();
}

void LLFloaterIMSessionTab::enableDisableCallBtn()
{
    mVoiceButton->setEnabled(
    		mSessionID.notNull()
    		&& mSession
    		&& mSession->mSessionInitialized
    		&& LLVoiceClient::getInstance()->voiceEnabled()
    		&& LLVoiceClient::getInstance()->isVoiceWorking()
    		&& mSession->mCallBackEnabled);
}

void LLFloaterIMSessionTab::onFocusReceived()
{
	setBackgroundOpaque(true);

	if (mSessionID.notNull() && isInVisibleChain())
	{
		LLIMModel::instance().sendNoUnreadMessages(mSessionID);
	}

	LLTransientDockableFloater::onFocusReceived();
}

void LLFloaterIMSessionTab::onFocusLost()
{
	setBackgroundOpaque(false);
	LLTransientDockableFloater::onFocusLost();
}

void LLFloaterIMSessionTab::onInputEditorClicked()
{
	LLFloaterIMContainer* im_box = LLFloaterIMContainer::findInstance();
	if (im_box)
	{
		im_box->flashConversationItemWidget(mSessionID,false);
	}
	gToolBarView->flashCommand(LLCommandId("chat"), false);
}

std::string LLFloaterIMSessionTab::appendTime()
{
	time_t utc_time;
	utc_time = time_corrected();
	std::string timeStr ="["+ LLTrans::getString("TimeHour")+"]:["
		+LLTrans::getString("TimeMin")+"]";

	LLSD substitution;

	substitution["datetime"] = (S32) utc_time;
	LLStringUtil::format (timeStr, substitution);

	return timeStr;
}

void LLFloaterIMSessionTab::appendMessage(const LLChat& chat, const LLSD &args)
{

	// Update the participant activity time
	LLFloaterIMContainer* im_box = LLFloaterIMContainer::findInstance();
	if (im_box)
	{
		im_box->setTimeNow(mSessionID,chat.mFromID);
	}
	

	LLChat& tmp_chat = const_cast<LLChat&>(chat);

	if(tmp_chat.mTimeStr.empty())
		tmp_chat.mTimeStr = appendTime();

	if (!chat.mMuted)
	{
		tmp_chat.mFromName = chat.mFromName;
		LLSD chat_args;
		if (args) chat_args = args;
		chat_args["use_plain_text_chat_history"] =
				gSavedSettings.getBOOL("PlainTextChatHistory");
		chat_args["show_time"] = gSavedSettings.getBOOL("IMShowTime");
		chat_args["show_names_for_p2p_conv"] =
				!mIsP2PChat || gSavedSettings.getBOOL("IMShowNamesForP2PConv");

		if (mChatHistory)
		{
			mChatHistory->appendMessage(chat, chat_args);
		}
	}
}


void LLFloaterIMSessionTab::buildConversationViewParticipant()
{
	// Clear the widget list since we are rebuilding afresh from the model
	conversations_widgets_map::iterator widget_it = mConversationsWidgets.begin();
	while (widget_it != mConversationsWidgets.end())
	{
		removeConversationViewParticipant(widget_it->first);
		// Iterators are invalidated by erase so we need to pick begin again
		widget_it = mConversationsWidgets.begin();
	}
	
	// Get the model list
	LLParticipantList* item = getParticipantList();
	if (!item)
	{
		// Nothing to do if the model list is inexistent
		return;
	}
	
	// Create the participants widgets now
	LLFolderViewModelItemCommon::child_list_t::const_iterator current_participant_model = item->getChildrenBegin();
	LLFolderViewModelItemCommon::child_list_t::const_iterator end_participant_model = item->getChildrenEnd();
	while (current_participant_model != end_participant_model)
	{
		LLConversationItem* participant_model = dynamic_cast<LLConversationItem*>(*current_participant_model);
		addConversationViewParticipant(participant_model);
		current_participant_model++;
	}
}

void LLFloaterIMSessionTab::addConversationViewParticipant(LLConversationItem* participant_model)
{
	// Check if the model already has an associated view
	LLUUID uuid = participant_model->getUUID();
	LLFolderViewItem* widget = get_ptr_in_map(mConversationsWidgets,uuid);
	
	// If not already present, create the participant view and attach it to the root, otherwise, just refresh it
	if (widget)
	{
		updateConversationViewParticipant(uuid); // overkill?
	}
	else
	{
		LLConversationViewParticipant* participant_view = createConversationViewParticipant(participant_model);
		mConversationsWidgets[uuid] = participant_view;
		participant_view->addToFolder(mConversationsRoot);
		participant_view->addToSession(mSessionID);
		participant_view->setVisible(TRUE);
	}
}

void LLFloaterIMSessionTab::removeConversationViewParticipant(const LLUUID& participant_id)
{
	LLFolderViewItem* widget = get_ptr_in_map(mConversationsWidgets,participant_id);
	if (widget)
	{
		mConversationsRoot->extractItem(widget);
		delete widget;
		mConversationsWidgets.erase(participant_id);
	}
}

void LLFloaterIMSessionTab::updateConversationViewParticipant(const LLUUID& participant_id)
{
	LLFolderViewItem* widget = get_ptr_in_map(mConversationsWidgets,participant_id);
	if (widget)
	{
		widget->refresh();
	}
}

void LLFloaterIMSessionTab::refreshConversation()
{
	// Note: We collect participants names to change the session name only in the case of ad-hoc conversations
	bool is_ad_hoc = (mSession ? mSession->isAdHocSessionType() : false);
	uuid_vec_t participants_uuids; // uuids vector for building the added participants name string
	// For P2P chat, we still need to update the session name who may have changed (switch display name for instance)
	if (mIsP2PChat && mSession)
	{
		participants_uuids.push_back(mSession->mOtherParticipantID);
	}

	conversations_widgets_map::iterator widget_it = mConversationsWidgets.begin();
	while (widget_it != mConversationsWidgets.end())
	{
		// Add the participant to the list except if it's the agent itself (redundant)
		if (is_ad_hoc && (widget_it->first != gAgentID))
		{
			participants_uuids.push_back(widget_it->first);
		}
		widget_it->second->refresh();
		widget_it->second->setVisible(TRUE);
		++widget_it;
	}
	if (is_ad_hoc || mIsP2PChat)
	{
		// Build the session name and update it
		std::string session_name;
		if (participants_uuids.size() != 0)
		{
			LLAvatarActions::buildResidentsString(participants_uuids, session_name);
		}
		else
		{
			session_name = LLIMModel::instance().getName(mSessionID);
		}
		updateSessionName(session_name);
	}

	if (mSessionID.notNull())
	{
		LLParticipantList* participant_list = getParticipantList();
		if (participant_list)
		{
			LLFolderViewModelItemCommon::child_list_t::const_iterator current_participant_model = participant_list->getChildrenBegin();
			LLFolderViewModelItemCommon::child_list_t::const_iterator end_participant_model = participant_list->getChildrenEnd();
			LLIMSpeakerMgr *speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(mSessionID);
			while (current_participant_model != end_participant_model)
			{
				LLConversationItemParticipant* participant_model = dynamic_cast<LLConversationItemParticipant*>(*current_participant_model);
				if (speaker_mgr && participant_model)
				{
					LLSpeaker *participant_speaker = speaker_mgr->findSpeaker(participant_model->getUUID());
					LLSpeaker *agent_speaker = speaker_mgr->findSpeaker(gAgentID);
					if (participant_speaker && agent_speaker)
					{
						participant_model->setDisplayModeratorRole(agent_speaker->mIsModerator && participant_speaker->mIsModerator);
					}
				}
				current_participant_model++;
			}
		}
	}
	
	mConversationViewModel.requestSortAll();
	if(mConversationsRoot != NULL)
	{
		mConversationsRoot->arrangeAll();
		mConversationsRoot->update();
	}
	updateHeaderAndToolbar();
	refresh();
}

// Copied from LLFloaterIMContainer::createConversationViewParticipant(). Refactor opportunity!
LLConversationViewParticipant* LLFloaterIMSessionTab::createConversationViewParticipant(LLConversationItem* item)
{
    LLRect panel_rect = mParticipantListPanel->getRect();
	
	LLConversationViewParticipant::Params params;
	params.name = item->getDisplayName();
	params.root = mConversationsRoot;
	params.listener = item;
	params.rect = LLRect (0, 24, panel_rect.getWidth(), 0); // *TODO: use conversation_view_participant.xml itemHeight value in lieu of 24
	params.tool_tip = params.name;
	params.participant_id = item->getUUID();
	
	return LLUICtrlFactory::create<LLConversationViewParticipant>(params);
}

void LLFloaterIMSessionTab::setSortOrder(const LLConversationSort& order)
{
	mConversationViewModel.setSorter(order);
	mConversationsRoot->arrangeAll();
	refreshConversation();
}

void LLFloaterIMSessionTab::onIMSessionMenuItemClicked(const LLSD& userdata)
{
	std::string item = userdata.asString();

	if (item == "compact_view" || item == "expanded_view")
	{
		gSavedSettings.setBOOL("PlainTextChatHistory", item == "compact_view");
	}
	else
	{
		bool prev_value = gSavedSettings.getBOOL(item);
		gSavedSettings.setBOOL(item, !prev_value);
	}

	LLFloaterIMSessionTab::processChatHistoryStyleUpdate();
}

bool LLFloaterIMSessionTab::onIMCompactExpandedMenuItemCheck(const LLSD& userdata)
{
	std::string item = userdata.asString();
	bool is_plain_text_mode = gSavedSettings.getBOOL("PlainTextChatHistory");

	return is_plain_text_mode? item == "compact_view" : item == "expanded_view";
}


bool LLFloaterIMSessionTab::onIMShowModesMenuItemCheck(const LLSD& userdata)
{
	return gSavedSettings.getBOOL(userdata.asString());
}

// enable/disable states for the "show time" and "show names" items of the show-modes menu
bool LLFloaterIMSessionTab::onIMShowModesMenuItemEnable(const LLSD& userdata)
{
	std::string item = userdata.asString();
	bool plain_text = gSavedSettings.getBOOL("PlainTextChatHistory");
	bool is_not_names = (item != "IMShowNamesForP2PConv");
	return (plain_text && (is_not_names || mIsP2PChat));
}

void LLFloaterIMSessionTab::hideOrShowTitle()
{
	const LLFloater::Params& default_params = LLFloater::getDefaultParams();
	S32 floater_header_size = default_params.header_height;
	LLView* floater_contents = getChild<LLView>("contents_view");

	LLRect floater_rect = getLocalRect();
	S32 top_border_of_contents = floater_rect.mTop - (isTornOff()? floater_header_size : 0);
	LLRect handle_rect (0, floater_rect.mTop, floater_rect.mRight, top_border_of_contents);
	LLRect contents_rect (0, top_border_of_contents, floater_rect.mRight, floater_rect.mBottom);
	mDragHandle->setShape(handle_rect);
	mDragHandle->setVisible(isTornOff());
	floater_contents->setShape(contents_rect);
}

void LLFloaterIMSessionTab::updateSessionName(const std::string& name)
{
	mInputEditor->setLabel(LLTrans::getString("IM_to_label") + " " + name);
}

void LLFloaterIMSessionTab::hideAllStandardButtons()
{
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		if (mButtons[i])
		{
			// Hide the standard header buttons in a docked IM floater.
			mButtons[i]->setVisible(false);
		}
	}
}

void LLFloaterIMSessionTab::updateHeaderAndToolbar()
{
	// prevent start conversation before its container
    LLFloaterIMContainer::getInstance();

	bool is_not_torn_off = !checkIfTornOff();
	if (is_not_torn_off)
	{
		hideAllStandardButtons();
	}

	hideOrShowTitle();

	// Participant list should be visible only in torn off floaters.
	bool is_participant_list_visible =
			!is_not_torn_off
			&& mIsParticipantListExpanded
			&& !mIsP2PChat;

	mParticipantListAndHistoryStack->collapsePanel(mParticipantListPanel, !is_participant_list_visible);
    mParticipantListPanel->setVisible(is_participant_list_visible);

	// Display collapse image (<<) if the floater is hosted
	// or if it is torn off but has an open control panel.
	bool is_expanded = is_not_torn_off || is_participant_list_visible;
    
	mExpandCollapseBtn->setImageOverlay(getString(is_expanded ? "collapse_icon" : "expand_icon"));
	mExpandCollapseBtn->setToolTip(
			is_not_torn_off?
				getString("expcol_button_not_tearoff_tooltip") :
				(is_expanded?
					getString("expcol_button_tearoff_and_expanded_tooltip") :
					getString("expcol_button_tearoff_and_collapsed_tooltip")));

	// toggle floater's drag handle and title visibility
	if (mDragHandle)
	{
		mDragHandle->setTitleVisible(!is_not_torn_off);
	}

	// The button (>>) should be disabled for torn off P2P conversations.
	mExpandCollapseBtn->setEnabled(is_not_torn_off || !mIsP2PChat);

	mTearOffBtn->setImageOverlay(getString(is_not_torn_off? "tear_off_icon" : "return_icon"));
	mTearOffBtn->setToolTip(getString(is_not_torn_off? "tooltip_to_separate_window" : "tooltip_to_main_window"));


	mCloseBtn->setVisible(is_not_torn_off && !mIsNearbyChat);

	enableDisableCallBtn();

	showTranslationCheckbox();
}
 
void LLFloaterIMSessionTab::forceReshape()
{
    LLRect floater_rect = getRect();
    reshape(llmax(floater_rect.getWidth(), this->getMinWidth()),
    		llmax(floater_rect.getHeight(), this->getMinHeight()),
    		true);
}


void LLFloaterIMSessionTab::reshapeChatLayoutPanel()
{
	mChatLayoutPanel->reshape(mChatLayoutPanel->getRect().getWidth(), mInputEditor->getRect().getHeight() + mInputEditorPad, FALSE);
}

void LLFloaterIMSessionTab::showTranslationCheckbox(BOOL show)
{
	mTranslationCheckBox->setVisible(mIsNearbyChat && show);
}

// static
void LLFloaterIMSessionTab::processChatHistoryStyleUpdate(bool clean_messages/* = false*/)
{
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("impanel");
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin();
			iter != inst_list.end(); ++iter)
	{
		LLFloaterIMSession* floater = dynamic_cast<LLFloaterIMSession*>(*iter);
		if (floater)
		{
			floater->reloadMessages(clean_messages);
		}
	}

	LLFloaterIMNearbyChat* nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
	if (nearby_chat)
	{
             nearby_chat->reloadMessages(clean_messages);
	}
}

// static
void LLFloaterIMSessionTab::reloadEmptyFloaters()
{
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("impanel");
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin();
		iter != inst_list.end(); ++iter)
	{
		LLFloaterIMSession* floater = dynamic_cast<LLFloaterIMSession*>(*iter);
		if (floater && floater->getLastChatMessageIndex() == -1)
		{
			floater->reloadMessages(true);
		}
	}

	LLFloaterIMNearbyChat* nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
	if (nearby_chat && nearby_chat->getMessageArchiveLength() == 0)
	{
		nearby_chat->reloadMessages(true);
	}
}

void LLFloaterIMSessionTab::updateCallBtnState(bool callIsActive)
{
	mVoiceButton->setImageOverlay(callIsActive? getString("call_btn_stop") : getString("call_btn_start"));
	mVoiceButton->setToolTip(callIsActive? getString("end_call_button_tooltip") : getString("start_call_button_tooltip"));

	enableDisableCallBtn();
}

void LLFloaterIMSessionTab::onSlide(LLFloaterIMSessionTab* self)
{
	LLFloaterIMContainer* host_floater = dynamic_cast<LLFloaterIMContainer*>(self->getHost());
	bool should_be_expanded = false;
	if (host_floater)
	{
		// Hide the messages pane if a floater is hosted in the Conversations
		host_floater->collapseMessagesPane(true);
	}
	else ///< floater is torn off
	{
		if (!self->mIsP2PChat)
		{
            // The state must toggle the collapsed state of the panel
           should_be_expanded = self->mParticipantListPanel->isCollapsed();

			// Update the expand/collapse flag of the participant list panel and save it
            gSavedSettings.setBOOL("IMShowControlPanel", should_be_expanded);
            self->mIsParticipantListExpanded = should_be_expanded;
            
            // Refresh for immediate feedback
            self->refreshConversation();
		}
	}

	self->assignResizeLimits();
	if (should_be_expanded)
	{
		self->forceReshape();
	}
}

void LLFloaterIMSessionTab::onCollapseToLine(LLFloaterIMSessionTab* self)
{
	LLFloaterIMContainer* host_floater = dynamic_cast<LLFloaterIMContainer*>(self->getHost());
	if (!host_floater)
	{
		bool expand = self->isMessagePaneExpanded();
		self->mExpandCollapseLineBtn->setImageOverlay(self->getString(expand ? "collapseline_icon" : "expandline_icon"));
		self->mContentPanel->setVisible(!expand);
		self->mToolbarPanel->setVisible(!expand);
		self->mInputEditor->enableSingleLineMode(expand);
		self->reshapeFloater(expand);
		self->setMessagePaneExpanded(!expand);
	}
}

void LLFloaterIMSessionTab::reshapeFloater(bool collapse)
{
	LLRect floater_rect = getRect();

	if(collapse)
	{
		mFloaterHeight = floater_rect.getHeight();
		S32 height = mContentPanel->getRect().getHeight() + mToolbarPanel->getRect().getHeight()
			+ mChatLayoutPanel->getRect().getHeight() - mChatLayoutPanelHeight + 2;
		floater_rect.mTop -= height;
	}
	else
	{
		floater_rect.mTop = floater_rect.mBottom + mFloaterHeight;
	}

	enableResizeCtrls(true, true, !collapse);

	saveCollapsedState();
	setShape(floater_rect, true);
	mBodyStack->updateLayout();
}

void LLFloaterIMSessionTab::restoreFloater()
{
	if(!isMessagePaneExpanded())
	{
		if(isMinimized())
		{
			setMinimized(false);
		}
		mContentPanel->setVisible(true);
		mToolbarPanel->setVisible(true);
		LLRect floater_rect = getRect();
		floater_rect.mTop = floater_rect.mBottom + mFloaterHeight;
		setShape(floater_rect, true);
		mBodyStack->updateLayout();
		mExpandCollapseLineBtn->setImageOverlay(getString("expandline_icon"));
		setMessagePaneExpanded(true);
		saveCollapsedState();
		mInputEditor->enableSingleLineMode(false);
		enableResizeCtrls(true, true, true);
	}
}

/*virtual*/
void LLFloaterIMSessionTab::onOpen(const LLSD& key)
{
	if (!checkIfTornOff())
	{
		LLFloaterIMContainer* host_floater = dynamic_cast<LLFloaterIMContainer*>(getHost());
		// Show the messages pane when opening a floater hosted in the Conversations
		host_floater->collapseMessagesPane(false);
	}

	mInputButtonPanel->setVisible(isTornOff());
}


void LLFloaterIMSessionTab::onTearOffClicked()
{
	restoreFloater();
	setFollows(isTornOff()? FOLLOWS_ALL : FOLLOWS_NONE);
    mSaveRect = isTornOff();
    initRectControl();
	LLFloater::onClickTearOff(this);
	LLFloaterIMContainer* container = LLFloaterReg::findTypedInstance<LLFloaterIMContainer>("im_container");

	if (isTornOff())
	{
		container->selectAdjacentConversation(false);
		forceReshape();
	}
	//Upon re-docking the torn off floater, select the corresponding conversation line item
	else
	{
		container->selectConversation(mSessionID);

	}
	mInputButtonPanel->setVisible(isTornOff());

	refreshConversation();
	updateGearBtn();
}

void LLFloaterIMSessionTab::updateGearBtn()
{

	BOOL prevVisibility = mGearBtn->getVisible();
	mGearBtn->setVisible(checkIfTornOff() && mIsP2PChat);


	// Move buttons if Gear button changed visibility
	if(prevVisibility != mGearBtn->getVisible())
	{
		LLRect gear_btn_rect =  mGearBtn->getRect();
		LLRect add_btn_rect = mAddBtn->getRect();
		LLRect call_btn_rect = mVoiceButton->getRect();
		S32 gap_width = call_btn_rect.mLeft - add_btn_rect.mRight;
		S32 right_shift = gear_btn_rect.getWidth() + gap_width;
		if(mGearBtn->getVisible())
		{
			// Move buttons to the right to give space for Gear button
			add_btn_rect.translate(right_shift,0);
			call_btn_rect.translate(right_shift,0);
		}
		else
		{
			add_btn_rect.translate(-right_shift,0);
			call_btn_rect.translate(-right_shift,0);
		}
		mAddBtn->setRect(add_btn_rect);
		mVoiceButton->setRect(call_btn_rect);
	}
}

void LLFloaterIMSessionTab::initBtns()
{
	LLRect gear_btn_rect =  mGearBtn->getRect();
	LLRect add_btn_rect = mAddBtn->getRect();
	LLRect call_btn_rect = mVoiceButton->getRect();
	S32 gap_width = call_btn_rect.mLeft - add_btn_rect.mRight;
	S32 right_shift = gear_btn_rect.getWidth() + gap_width;

	add_btn_rect.translate(-right_shift,0);
	call_btn_rect.translate(-right_shift,0);

	mAddBtn->setRect(add_btn_rect);
	mVoiceButton->setRect(call_btn_rect);
}

// static
bool LLFloaterIMSessionTab::isChatMultiTab()
{
	// Restart is required in order to change chat window type.
	return true;
}

bool LLFloaterIMSessionTab::checkIfTornOff()
{
	bool isTorn = !getHost();
	
	if (isTorn != isTornOff())
	{
		setTornOff(isTorn);
		refreshConversation();
	}

	return isTorn;
}

void LLFloaterIMSessionTab::doToSelected(const LLSD& userdata)
{
	// Get the list of selected items in the tab
    std::string command = userdata.asString();
    uuid_vec_t selected_uuids;
	getSelectedUUIDs(selected_uuids);
		
	// Perform the command (IM, profile, etc...) on the list using the general conversation container method
	LLFloaterIMContainer* floater_container = LLFloaterIMContainer::getInstance();
	// Note: By construction, those can only be participants so we can call doToParticipants() directly
	floater_container->doToParticipants(command, selected_uuids);
}

bool LLFloaterIMSessionTab::enableContextMenuItem(const LLSD& userdata)
{
	// Get the list of selected items in the tab
    std::string command = userdata.asString();
    uuid_vec_t selected_uuids;
	getSelectedUUIDs(selected_uuids);
	
	// Perform the item enable test on the list using the general conversation container method
	LLFloaterIMContainer* floater_container = LLFloaterIMContainer::getInstance();
	return floater_container->enableContextMenuItem(command, selected_uuids);
}

bool LLFloaterIMSessionTab::checkContextMenuItem(const LLSD& userdata)
{
	// Get the list of selected items in the tab
    std::string command = userdata.asString();
    uuid_vec_t selected_uuids;
	getSelectedUUIDs(selected_uuids);
	
	// Perform the item check on the list using the general conversation container method
	LLFloaterIMContainer* floater_container = LLFloaterIMContainer::getInstance();
	return floater_container->checkContextMenuItem(command, selected_uuids);
}

void LLFloaterIMSessionTab::getSelectedUUIDs(uuid_vec_t& selected_uuids)
{
    const std::set<LLFolderViewItem*> selected_items = mConversationsRoot->getSelectionList();
	
    std::set<LLFolderViewItem*>::const_iterator it = selected_items.begin();
    const std::set<LLFolderViewItem*>::const_iterator it_end = selected_items.end();
	
    for (; it != it_end; ++it)
    {
        LLConversationItem* conversation_item = static_cast<LLConversationItem *>((*it)->getViewModelItem());
        selected_uuids.push_back(conversation_item->getUUID());
    }
}

LLConversationItem* LLFloaterIMSessionTab::getCurSelectedViewModelItem()
{
	LLConversationItem *conversationItem = NULL;

	if(mConversationsRoot && 
        mConversationsRoot->getCurSelectedItem() && 
        mConversationsRoot->getCurSelectedItem()->getViewModelItem())
	{
		conversationItem = static_cast<LLConversationItem *>(mConversationsRoot->getCurSelectedItem()->getViewModelItem()) ;
	}

	return conversationItem;
}

void LLFloaterIMSessionTab::saveCollapsedState()
{
	LLFloaterIMSessionTab* conversp = LLFloaterIMSessionTab::getConversation(mSessionID);
	if(conversp->isNearbyChat())
	{
		gSavedPerAccountSettings.setBOOL("NearbyChatIsNotCollapsed", isMessagePaneExpanded());
	}
}

LLView* LLFloaterIMSessionTab::getChatHistory()
{
	return mChatHistory;
}

BOOL LLFloaterIMSessionTab::handleKeyHere(KEY key, MASK mask )
{
	BOOL handled = FALSE;

	if(mask == MASK_ALT)
	{
		LLFloaterIMContainer* floater_container = LLFloaterIMContainer::getInstance();
		if (KEY_RETURN == key && !isTornOff())
		{
			floater_container->expandConversation();
			handled = TRUE;
		}
		if ((KEY_UP == key) || (KEY_LEFT == key))
		{
			floater_container->selectNextorPreviousConversation(false);
			handled = TRUE;
		}
		if ((KEY_DOWN == key ) || (KEY_RIGHT == key))
		{
			floater_container->selectNextorPreviousConversation(true);
			handled = TRUE;
		}
	}
	return handled;
}

/** 
 * @file llconversationview.cpp
 * @brief Implementation of conversations list widgets and views
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

#include "llconversationview.h"

#include <boost/bind.hpp>
#include "llagentdata.h"
#include "llconversationmodel.h"
#include "llfloaterimsession.h"
#include "llfloaterimnearbychat.h"
#include "llfloaterimsessiontab.h"
#include "llfloaterimcontainer.h"
#include "llfloaterreg.h"
#include "llgroupiconctrl.h"
#include "lluictrlfactory.h"
#include "lltoolbarview.h"

//
// Implementation of conversations list session widgets
//
static LLDefaultChildRegistry::Register<LLConversationViewSession> r_conversation_view_session("conversation_view_session");

const LLColor4U DEFAULT_WHITE(255, 255, 255);

class LLNearbyVoiceClientStatusObserver : public LLVoiceClientStatusObserver
{
public:

	LLNearbyVoiceClientStatusObserver(LLConversationViewSession* conv)
	:	conversation(conv)
	{}

	virtual void onChange(EStatusType status, const std::string &channelURI, bool proximal)
	{
		conversation->showVoiceIndicator(conversation
			&& status != STATUS_JOINING
			&& status != STATUS_LEFT_CHANNEL
			&& LLVoiceClient::getInstance()->voiceEnabled()
			&& LLVoiceClient::getInstance()->isVoiceWorking());
	}

private:
	LLConversationViewSession* conversation;
};

LLConversationViewSession::Params::Params() :	
	container()
{}

LLConversationViewSession::LLConversationViewSession(const LLConversationViewSession::Params& p):
	LLFolderViewFolder(p),
	mContainer(p.container),
	mItemPanel(NULL),
	mCallIconLayoutPanel(NULL),
	mSessionTitle(NULL),
	mSpeakingIndicator(NULL),
	mVoiceClientObserver(NULL),
	mCollapsedMode(false),
    mHasArrow(true),
	mIsInActiveVoiceChannel(false),
	mFlashStateOn(false),
	mFlashStarted(false)
{
	mFlashTimer = new LLFlashTimer();
}

LLConversationViewSession::~LLConversationViewSession()
{
	mActiveVoiceChannelConnection.disconnect();

	if(LLVoiceClient::instanceExists() && mVoiceClientObserver)
	{
		LLVoiceClient::getInstance()->removeObserver(mVoiceClientObserver);
	}

	mFlashTimer->unset();
}

void LLConversationViewSession::setFlashState(bool flash_state)
{
	if (flash_state && !mFlashStateOn)
	{
		// flash chat toolbar button if scrolled out of sight (because flashing will not be visible)
		if (mContainer->isScrolledOutOfSight(this))
		{
			gToolBarView->flashCommand(LLCommandId("chat"), true);
		}
	}

	mFlashStateOn = flash_state;
	mFlashStarted = false;
	mFlashTimer->stopFlashing();
}

void LLConversationViewSession::setHighlightState(bool hihglight_state)
{
	mFlashStateOn = hihglight_state;
	mFlashStarted = true;
	mFlashTimer->stopFlashing();
}

void LLConversationViewSession::startFlashing()
{
	// Need to start flashing only when "Conversations" is opened or brought on top
	if (isInVisibleChain()
		&& mFlashStateOn
		&& !mFlashStarted
		&& ! LLFloaterReg::getTypedInstance<LLFloaterIMContainer>("im_container")->isMinimized() )
	{
		mFlashStarted = true;
		mFlashTimer->startFlashing();
	}
}

bool LLConversationViewSession::isHighlightAllowed()
{
	return mFlashStateOn || mIsSelected;
}

bool LLConversationViewSession::isHighlightActive()
{
	return (mFlashStateOn ? (mFlashTimer->isFlashingInProgress() ? mFlashTimer->isCurrentlyHighlighted() : true) : mIsCurSelection);
}

BOOL LLConversationViewSession::postBuild()
{
	LLFolderViewItem::postBuild();

	mItemPanel = LLUICtrlFactory::getInstance()->createFromFile<LLPanel>("panel_conversation_list_item.xml", NULL, LLPanel::child_registry_t::instance());
	addChild(mItemPanel);

	mCallIconLayoutPanel = mItemPanel->getChild<LLPanel>("call_icon_panel");
	mSessionTitle = mItemPanel->getChild<LLTextBox>("conversation_title");

	mActiveVoiceChannelConnection = LLVoiceChannel::setCurrentVoiceChannelChangedCallback(boost::bind(&LLConversationViewSession::onCurrentVoiceSessionChanged, this, _1));
	mSpeakingIndicator = getChild<LLOutputMonitorCtrl>("speaking_indicator");

	LLConversationItem* vmi = dynamic_cast<LLConversationItem*>(getViewModelItem());
	if (vmi)
	{
		switch(vmi->getType())
		{
		case LLConversationItem::CONV_PARTICIPANT:
		case LLConversationItem::CONV_SESSION_1_ON_1:
		{
			LLIMModel::LLIMSession* session=  LLIMModel::instance().findIMSession(vmi->getUUID());
			if (session)
			{
				LLAvatarIconCtrl* icon = mItemPanel->getChild<LLAvatarIconCtrl>("avatar_icon");
				icon->setVisible(true);
				icon->setValue(session->mOtherParticipantID);
				mSpeakingIndicator->setSpeakerId(gAgentID, session->mSessionID, true);
                mHasArrow = false;
			}
			break;
		}
		case LLConversationItem::CONV_SESSION_AD_HOC:
		{
			LLGroupIconCtrl* icon = mItemPanel->getChild<LLGroupIconCtrl>("group_icon");
			icon->setVisible(true);
			mSpeakingIndicator->setSpeakerId(gAgentID, vmi->getUUID(), true);
			break;
		}
		case LLConversationItem::CONV_SESSION_GROUP:
		{
			LLGroupIconCtrl* icon = mItemPanel->getChild<LLGroupIconCtrl>("group_icon");
			icon->setVisible(true);
			icon->setValue(vmi->getUUID());
			mSpeakingIndicator->setSpeakerId(gAgentID, vmi->getUUID(), true);
			break;
		}
		case LLConversationItem::CONV_SESSION_NEARBY:
		{
			LLIconCtrl* icon = mItemPanel->getChild<LLIconCtrl>("nearby_chat_icon");
			icon->setVisible(true);
			mSpeakingIndicator->setSpeakerId(gAgentID, LLUUID::null, true);
			mIsInActiveVoiceChannel = true;
			if(LLVoiceClient::instanceExists())
			{
				LLNearbyVoiceClientStatusObserver* mVoiceClientObserver = new LLNearbyVoiceClientStatusObserver(this);
				LLVoiceClient::getInstance()->addObserver(mVoiceClientObserver);
			}
			break;
		}
		default:
			break;
		}
	}

	refresh();

	return TRUE;
}

void LLConversationViewSession::draw()
{
	getViewModelItem()->update();

	const LLFolderViewItem::Params& default_params = LLUICtrlFactory::getDefaultParams<LLFolderViewItem>();
	const BOOL show_context = (getRoot() ? getRoot()->getShowSelectionContext() : FALSE);

	// Indicate that flash can start (moot operation if already started, done or not flashing)
	startFlashing();

	// draw highlight for selected items
	drawHighlight(show_context, true, sHighlightBgColor, sFlashBgColor, sFocusOutlineColor, sMouseOverColor);

	// Draw children if root folder, or any other folder that is open. Do not draw children when animating to closed state or you get rendering overlap.
	bool draw_children = getRoot() == static_cast<LLFolderViewFolder*>(this) || isOpen();

	// Todo/fix this: arrange hides children 'out of bonds', session 'slowly' adjusts container size, unhides children
	// this process repeats until children fit
	for (folders_t::iterator iter = mFolders.begin();
		iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		(*fit)->setVisible(draw_children);
	}
	for (items_t::iterator iter = mItems.begin();
		iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		(*iit)->setVisible(draw_children);
	}

	// we don't draw the open folder arrow in minimized mode
	if (mHasArrow && !mCollapsedMode)
	{
		// update the rotation angle of open folder arrow
		updateLabelRotation();
		drawOpenFolderArrow(default_params, sFgColor);
	}
	LLView::draw();
}

BOOL LLConversationViewSession::handleMouseDown( S32 x, S32 y, MASK mask )
{
	//Will try to select a child node and then itself (if a child was not selected)
    BOOL result = LLFolderViewFolder::handleMouseDown(x, y, mask);

    //This node (conversation) was selected and a child (participant) was not
    if(result && getRoot())
    {
		if(getRoot()->getCurSelectedItem() == this)
		{
			LLConversationItem* item = dynamic_cast<LLConversationItem *>(getViewModelItem());
			LLUUID session_id = item? item->getUUID() : LLUUID();

			LLFloaterIMContainer *im_container = LLFloaterReg::getTypedInstance<LLFloaterIMContainer>("im_container");
			if (im_container->isConversationsPaneCollapsed() && im_container->getSelectedSession() == session_id)
			{
				im_container->collapseMessagesPane(!im_container->isMessagesPaneCollapsed());
			}
			else
			{
				im_container->collapseMessagesPane(false);
			}
		}
		selectConversationItem();
    }

	return result;
}

BOOL LLConversationViewSession::handleMouseUp( S32 x, S32 y, MASK mask )
{
	BOOL result = LLFolderViewFolder::handleMouseUp(x, y, mask);

	LLFloater* volume_floater = LLFloaterReg::findInstance("floater_voice_volume");
	LLFloater* chat_volume_floater = LLFloaterReg::findInstance("chat_voice");
	if (result 
		&& getRoot() && (getRoot()->getCurSelectedItem() == this)
		&& !(volume_floater && volume_floater->isShown() && volume_floater->hasFocus())
		&& !(chat_volume_floater && chat_volume_floater->isShown() && chat_volume_floater->hasFocus()))
	{
		LLConversationItem* item = dynamic_cast<LLConversationItem *>(getViewModelItem());
		LLUUID session_id = item? item->getUUID() : LLUUID();
		LLFloaterIMSessionTab* session_floater = LLFloaterIMSessionTab::findConversation(session_id);
		if(session_floater && !session_floater->hasFocus())
		{
			session_floater->setFocus(true);
		}
    }

	return result;
}

BOOL LLConversationViewSession::handleRightMouseDown( S32 x, S32 y, MASK mask )
{
    BOOL result = LLFolderViewFolder::handleRightMouseDown(x, y, mask);

    if(result)
    {
		selectConversationItem();
    }

	return result;
}

void LLConversationViewSession::selectConversationItem()
{
	if(getRoot()->getCurSelectedItem() == this)
	{
		LLConversationItem* item = dynamic_cast<LLConversationItem *>(getViewModelItem());
		LLUUID session_id = item? item->getUUID() : LLUUID();

		LLFloaterIMContainer *im_container = LLFloaterReg::getTypedInstance<LLFloaterIMContainer>("im_container");
		im_container->flashConversationItemWidget(session_id,false);
		im_container->selectConversationPair(session_id, false);
	}
}

// virtual
S32 LLConversationViewSession::arrange(S32* width, S32* height)
{
    //LLFolderViewFolder::arrange computes value for getIndentation() function below
    S32 arranged = LLFolderViewFolder::arrange(width, height);

    S32 h_pad = mHasArrow ? getIndentation() + mArrowSize : getIndentation();
	
	LLRect rect(mCollapsedMode ? getLocalRect().mLeft : h_pad,
				getLocalRect().mTop,
				getLocalRect().mRight,
				getLocalRect().mTop - getItemHeight());
	mItemPanel->setShape(rect);

	return arranged;
}

// virtual
void LLConversationViewSession::toggleOpen()
{
	// conversations should not be opened while in minimized mode
	if (!mCollapsedMode)
	{
		LLFolderViewFolder::toggleOpen();

		// do item's selection when opened
		if (LLFolderViewFolder::isOpen())
		{
			getParentFolder()->setSelection(this, true);
		}
		mContainer->reSelectConversation();
	}
}

void LLConversationViewSession::toggleCollapsedMode(bool is_collapsed)
{
	mCollapsedMode = is_collapsed;

	// hide the layout stack which contains all item's child widgets
	// except for the icon which we display in minimized mode
	getChild<LLView>("conversation_item_stack")->setVisible(!mCollapsedMode);

    S32 h_pad = mHasArrow ? getIndentation() + mArrowSize : getIndentation();

	mItemPanel->translate(mCollapsedMode ? -h_pad : h_pad, 0);
}

void LLConversationViewSession::setVisibleIfDetached(BOOL visible)
{
	// Do this only if the conversation floater has been torn off (i.e. no multi floater host) and is not minimized
	// Note: minimized dockable floaters are brought to front hence unminimized when made visible and we don't want that here
	LLFloater* session_floater = getSessionFloater();
	if (session_floater && session_floater->isDetachedAndNotMinimized())
	{
		session_floater->setVisible(visible);
	}
}

LLFloater* LLConversationViewSession::getSessionFloater()
{
	LLFolderViewModelItem* item = mViewModelItem;
	LLUUID session_uuid = dynamic_cast<LLConversationItem*>(item)->getUUID();
	return LLFloaterIMSessionTab::getConversation(session_uuid);
}

LLConversationViewParticipant* LLConversationViewSession::findParticipant(const LLUUID& participant_id)
{
	// This is *not* a general tree parsing algorithm. We search only in the mItems list
	// assuming there is no mFolders which makes sense for sessions (sessions don't contain
	// sessions).
	LLConversationViewParticipant* participant = NULL;
	items_t::const_iterator iter;
	for (iter = getItemsBegin(); iter != getItemsEnd(); iter++)
	{
		participant = dynamic_cast<LLConversationViewParticipant*>(*iter);
		if (participant->hasSameValue(participant_id))
		{
			break;
		}
	}
	return (iter == getItemsEnd() ? NULL : participant);
}

void LLConversationViewSession::showVoiceIndicator(bool visible)
{
	mCallIconLayoutPanel->setVisible(visible && LLVoiceChannel::getCurrentVoiceChannel()->getSessionID().isNull());
	requestArrange();
}

void LLConversationViewSession::refresh()
{
	// Refresh the session view from its model data
	LLConversationItem* vmi = dynamic_cast<LLConversationItem*>(getViewModelItem());
	vmi->resetRefresh();
	
	if (mSessionTitle)
	{
		mSessionTitle->setText(vmi->getDisplayName());
	}

	// Update all speaking indicators
	LLSpeakingIndicatorManager::updateSpeakingIndicators();

	// we should show indicator for specified voice session only if this is current channel. EXT-5562.
	if (!mIsInActiveVoiceChannel)
	{
		if (mSpeakingIndicator)
		{
			mSpeakingIndicator->setVisible(false);
		}
		LLConversationViewParticipant* participant = NULL;
		items_t::const_iterator iter;
		for (iter = getItemsBegin(); iter != getItemsEnd(); iter++)
		{
			participant = dynamic_cast<LLConversationViewParticipant*>(*iter);
			if (participant)
			{
				participant->hideSpeakingIndicator();
			}
		}
	}
    
    if (mSpeakingIndicator)
    {
        mSpeakingIndicator->setShowParticipantsSpeaking(mIsInActiveVoiceChannel);
    }
	requestArrange();
	// Do the regular upstream refresh
	LLFolderViewFolder::refresh();
}

void LLConversationViewSession::onCurrentVoiceSessionChanged(const LLUUID& session_id)
{
	LLConversationItem* vmi = dynamic_cast<LLConversationItem*>(getViewModelItem());

	if (vmi)
	{
		mIsInActiveVoiceChannel = vmi->getUUID() == session_id;
		mCallIconLayoutPanel->setVisible(mIsInActiveVoiceChannel);
	}
}

//
// Implementation of conversations list participant (avatar) widgets
//

static LLDefaultChildRegistry::Register<LLConversationViewParticipant> r("conversation_view_participant");
bool LLConversationViewParticipant::sStaticInitialized = false;
S32 LLConversationViewParticipant::sChildrenWidths[LLConversationViewParticipant::ALIC_COUNT];

LLConversationViewParticipant::Params::Params() :	
container(),
participant_id(),
avatar_icon("avatar_icon"),
info_button("info_button"),
output_monitor("output_monitor")
{}

LLConversationViewParticipant::LLConversationViewParticipant( const LLConversationViewParticipant::Params& p ):
	LLFolderViewItem(p),
    mAvatarIcon(NULL),
    mInfoBtn(NULL),
    mSpeakingIndicator(NULL),
    mUUID(p.participant_id)
{
}

LLConversationViewParticipant::~LLConversationViewParticipant()
{
	mActiveVoiceChannelConnection.disconnect();
}

void LLConversationViewParticipant::initFromParams(const LLConversationViewParticipant::Params& params)
{	
    LLAvatarIconCtrl::Params avatar_icon_params(params.avatar_icon());
    applyXUILayout(avatar_icon_params, this);
    LLAvatarIconCtrl * avatarIcon = LLUICtrlFactory::create<LLAvatarIconCtrl>(avatar_icon_params);
    addChild(avatarIcon);	
    
	LLButton::Params info_button_params(params.info_button());
    applyXUILayout(info_button_params, this);
	LLButton * button = LLUICtrlFactory::create<LLButton>(info_button_params);
	addChild(button);	

    LLOutputMonitorCtrl::Params output_monitor_params(params.output_monitor());
    applyXUILayout(output_monitor_params, this);
    LLOutputMonitorCtrl * outputMonitor = LLUICtrlFactory::create<LLOutputMonitorCtrl>(output_monitor_params);
    addChild(outputMonitor);
}

BOOL LLConversationViewParticipant::postBuild()
{
    mAvatarIcon = getChild<LLAvatarIconCtrl>("avatar_icon");

	mInfoBtn = getChild<LLButton>("info_btn");
	mInfoBtn->setClickedCallback(boost::bind(&LLConversationViewParticipant::onInfoBtnClick, this));
	mInfoBtn->setVisible(false);

	mSpeakingIndicator = getChild<LLOutputMonitorCtrl>("speaking_indicator");

    if (!sStaticInitialized)
    {
        // Remember children widths including their padding from the next sibling,
        // so that we can hide and show them again later.
        initChildrenWidths(this);
        sStaticInitialized = true;
    }

    updateChildren();
	return LLFolderViewItem::postBuild();
}

void LLConversationViewParticipant::draw()
{
    static LLUIColor sFgColor = LLUIColorTable::instance().getColor("MenuItemEnabledColor", DEFAULT_WHITE);
	static LLUIColor sFgDisabledColor = LLUIColorTable::instance().getColor("MenuItemDisabledColor", DEFAULT_WHITE);
    static LLUIColor sHighlightFgColor = LLUIColorTable::instance().getColor("MenuItemHighlightFgColor", DEFAULT_WHITE);
    static LLUIColor sHighlightBgColor = LLUIColorTable::instance().getColor("MenuItemHighlightBgColor", DEFAULT_WHITE);
    static LLUIColor sFlashBgColor = LLUIColorTable::instance().getColor("MenuItemFlashBgColor", DEFAULT_WHITE);
    static LLUIColor sFocusOutlineColor = LLUIColorTable::instance().getColor("InventoryFocusOutlineColor", DEFAULT_WHITE);
    static LLUIColor sMouseOverColor = LLUIColorTable::instance().getColor("InventoryMouseOverColor", DEFAULT_WHITE);

    const BOOL show_context = (getRoot() ? getRoot()->getShowSelectionContext() : FALSE);

    const LLFontGL* font = getLabelFontForStyle(mLabelStyle);
    F32 right_x  = 0;

    F32 y = (F32)getRect().getHeight() - font->getLineHeight() - (F32)mTextPad;
    F32 text_left = (F32)getLabelXPos();
	
	LLColor4 color;

	LLLocalSpeakerMgr *speakerMgr = LLLocalSpeakerMgr::getInstance();

	if (speakerMgr && speakerMgr->isSpeakerToBeRemoved(mUUID))
	{
		color = sFgDisabledColor;
	}
	else
	{
		color = mIsSelected ? sHighlightFgColor : sFgColor;
	}

	LLConversationItemParticipant* participant_model = dynamic_cast<LLConversationItemParticipant*>(getViewModelItem());
	if (participant_model)
	{
		mSpeakingIndicator->setIsModeratorMuted(participant_model->isModeratorMuted());
	}

    drawHighlight(show_context, mIsSelected, sHighlightBgColor, sFlashBgColor, sFocusOutlineColor, sMouseOverColor);
    drawLabel(font, text_left, y, color, right_x);

    LLView::draw();
}

// virtual
S32 LLConversationViewParticipant::arrange(S32* width, S32* height)
{
    //Need to call arrange first since it computes value used in getIndentation()
    S32 arranged = LLFolderViewItem::arrange(width, height);

    //Adjusts the avatar icon based upon the indentation
    LLRect avatarRect(getIndentation(), 
                        mAvatarIcon->getRect().mTop,
                        getIndentation() + mAvatarIcon->getRect().getWidth(),
                        mAvatarIcon->getRect().mBottom);
    mAvatarIcon->setShape(avatarRect);

    //Since dimensions changed, adjust the children (info button, speaker indicator)
    updateChildren();

    return arranged;
}

// virtual
void LLConversationViewParticipant::refresh()
{
	// Refresh the participant view from its model data
	LLConversationItemParticipant* participant_model = dynamic_cast<LLConversationItemParticipant*>(getViewModelItem());
	participant_model->resetRefresh();
	
	// *TODO: We should also do something with vmi->isModerator() to echo that state in the UI somewhat
	mSpeakingIndicator->setIsModeratorMuted(participant_model->isModeratorMuted());
	
	// Do the regular upstream refresh
	LLFolderViewItem::refresh();
}

void LLConversationViewParticipant::addToFolder(LLFolderViewFolder* folder)
{
    // Add the item to the folder (conversation)
    LLFolderViewItem::addToFolder(folder);
	
    // Retrieve the folder (conversation) UUID, which is also the speaker session UUID
    LLConversationItem* vmi = getParentFolder() ? dynamic_cast<LLConversationItem*>(getParentFolder()->getViewModelItem()) : NULL;
    if (vmi)
    {
		addToSession(vmi->getUUID());
    }
}

void LLConversationViewParticipant::addToSession(const LLUUID& session_id)
{
	//Allows speaking icon image to be loaded based on mUUID
	mAvatarIcon->setValue(mUUID);
	
	//Allows the speaker indicator to be activated based on the user and conversation
	mSpeakingIndicator->setSpeakerId(mUUID, session_id); 
}

void LLConversationViewParticipant::onInfoBtnClick()
{
	LLFloaterReg::showInstance("inspect_avatar", LLSD().with("avatar_id", mUUID));
}

BOOL LLConversationViewParticipant::handleMouseDown( S32 x, S32 y, MASK mask )
{
	BOOL result = LLFolderViewItem::handleMouseDown(x, y, mask);

    if(result && getRoot())
    {
    	if(getRoot()->getCurSelectedItem() == this)
		{
    		LLConversationItem* vmi = getParentFolder() ? dynamic_cast<LLConversationItem*>(getParentFolder()->getViewModelItem()) : NULL;
    		LLUUID session_id = vmi? vmi->getUUID() : LLUUID();

    		LLFloaterIMContainer *im_container = LLFloaterReg::getTypedInstance<LLFloaterIMContainer>("im_container");
    		LLFloaterIMSessionTab* session_floater = LLFloaterIMSessionTab::findConversation(session_id);
			im_container->setSelectedSession(session_id);
			im_container->flashConversationItemWidget(session_id,false);
			im_container->selectFloater(session_floater);
			im_container->collapseMessagesPane(false);
		}
    }
    return result;
}

void LLConversationViewParticipant::onMouseEnter(S32 x, S32 y, MASK mask)
{
    mInfoBtn->setVisible(true);
    updateChildren();
    LLFolderViewItem::onMouseEnter(x, y, mask);
}

void LLConversationViewParticipant::onMouseLeave(S32 x, S32 y, MASK mask)
{
    mInfoBtn->setVisible(false);
    updateChildren();
    LLFolderViewItem::onMouseLeave(x, y, mask);
}

S32 LLConversationViewParticipant::getLabelXPos()
{
    return getIndentation() + mAvatarIcon->getRect().getWidth() + mIconPad;
}

// static
void LLConversationViewParticipant::initChildrenWidths(LLConversationViewParticipant* self)
{
    //speaking indicator width + padding
    S32 speaking_indicator_width = self->getRect().getWidth() - self->mSpeakingIndicator->getRect().mLeft;

    //info btn width + padding
    S32 info_btn_width = self->mSpeakingIndicator->getRect().mLeft - self->mInfoBtn->getRect().mLeft;

    S32 index = ALIC_COUNT;
    sChildrenWidths[--index] = info_btn_width;
    sChildrenWidths[--index] = speaking_indicator_width;
    llassert(index == 0);
}

void LLConversationViewParticipant::updateChildren()
{
    mLabelPaddingRight = DEFAULT_LABEL_PADDING_RIGHT;
    LLView* control;
    S32 ctrl_width;
    LLRect controlRect;

    //Cycles through controls starting from right to left
    for (S32 i = 0; i < ALIC_COUNT; ++i)
    {
        control = getItemChildView((EAvatarListItemChildIndex)i);

        // skip invisible views
        if (!control->getVisible()) continue;

        //Get current pos/dimensions
        controlRect = control->getRect();

        ctrl_width = sChildrenWidths[i]; // including space between current & left controls
        // accumulate the amount of space taken by the controls
        mLabelPaddingRight += ctrl_width;

        //Reposition visible controls in case adjacent controls to the right are hidden.
        controlRect.setLeftTopAndSize(
            getLocalRect().getWidth() - mLabelPaddingRight,
            controlRect.mTop,
            controlRect.getWidth(),
            controlRect.getHeight());

        //Sets the new position
        control->setShape(controlRect);
    }
}

LLView* LLConversationViewParticipant::getItemChildView(EAvatarListItemChildIndex child_view_index)
{
    LLView* child_view = NULL;

    switch (child_view_index)
    {
        case ALIC_SPEAKER_INDICATOR:
            child_view = mSpeakingIndicator;
            break;
        case ALIC_INFO_BUTTON:
            child_view = mInfoBtn;
            break;
        default:
            LL_WARNS("AvatarItemReshape") << "Unexpected child view index is passed: " << child_view_index << LL_ENDL;
            llassert(0);
            break;
            // leave child_view untouched
    }

    return child_view;
}

void LLConversationViewParticipant::hideSpeakingIndicator()
{
	mSpeakingIndicator->setVisible(false);
}

// EOF


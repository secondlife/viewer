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
		if (conversation
		   && status != STATUS_JOINING
		   && status != STATUS_LEFT_CHANNEL
		   && LLVoiceClient::getInstance()->voiceEnabled()
		   && LLVoiceClient::getInstance()->isVoiceWorking())
		{
			conversation->showVoiceIndicator();
		}
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
	mMinimizedMode(false)
{
}

LLConversationViewSession::~LLConversationViewSession()
{
	mActiveVoiceChannelConnection.disconnect();

	if(LLVoiceClient::instanceExists() && mVoiceClientObserver)
	{
		LLVoiceClient::getInstance()->removeObserver(mVoiceClientObserver);
	}
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

	// we don't draw the open folder arrow in minimized mode
	if (!mMinimizedMode)
	{
		// update the rotation angle of open folder arrow
		updateLabelRotation();

		drawOpenFolderArrow(default_params, sFgColor);
	}

	// draw highlight for selected items
	drawHighlight(show_context, true, sHighlightBgColor, sFocusOutlineColor, sMouseOverColor);

	// Draw children if root folder, or any other folder that is open. Do not draw children when animating to closed state or you get rendering overlap.
	bool draw_children = getRoot() == static_cast<LLFolderViewFolder*>(this) || isOpen();

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

	LLView::draw();
}

BOOL LLConversationViewSession::handleMouseDown( S32 x, S32 y, MASK mask )
{
	LLConversationItem* item = dynamic_cast<LLConversationItem *>(getViewModelItem());
    LLUUID session_id = item? item->getUUID() : LLUUID();
    BOOL result = LLFolderViewFolder::handleMouseDown(x, y, mask);
	(LLFloaterReg::getTypedInstance<LLFloaterIMContainer>("im_container"))->
    		selectConversationPair(session_id, false);

	return result;
}

// virtual
S32 LLConversationViewSession::arrange(S32* width, S32* height)
{
	S32 h_pad = getIndentation() + mArrowSize;
	LLRect rect(mMinimizedMode ? getLocalRect().mLeft : h_pad,
				getLocalRect().mTop,
				getLocalRect().mRight,
				getLocalRect().mTop - getItemHeight());
	mItemPanel->setShape(rect);

	return LLFolderViewFolder::arrange(width, height);
}

// virtual
void LLConversationViewSession::toggleOpen()
{
	// conversations should not be opened while in minimized mode
	if (!mMinimizedMode)
	{
		LLFolderViewFolder::toggleOpen();

		// do item's selection when opened
		if (LLFolderViewFolder::isOpen())
		{
			getParentFolder()->setSelection(this, true);
		}

	}
}

void LLConversationViewSession::toggleMinimizedMode(bool is_minimized)
{
	mMinimizedMode = is_minimized;

	// hide the layout stack which contains all item's child widgets
	// except for the icon which we display in minimized mode
	getChild<LLView>("conversation_item_stack")->setVisible(!mMinimizedMode);

	S32 h_pad = getIndentation() + mArrowSize;
	mItemPanel->translate(mMinimizedMode ? -h_pad : h_pad, 0);
}

void LLConversationViewSession::setVisibleIfDetached(BOOL visible)
{
	// Do this only if the conversation floater has been torn off (i.e. no multi floater host) and is not minimized
	// Note: minimized dockable floaters are brought to front hence unminimized when made visible and we don't want that here
	LLFolderViewModelItem* item = mViewModelItem;
	LLUUID session_uuid = dynamic_cast<LLConversationItem*>(item)->getUUID();
	LLFloater* session_floater = LLFloaterIMSessionTab::getConversation(session_uuid);
	
	if (session_floater && !session_floater->getHost() && !session_floater->isMinimized())
	{
		session_floater->setVisible(visible);
	}
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

void LLConversationViewSession::showVoiceIndicator()
{
	if (LLVoiceChannel::getCurrentVoiceChannel()->getSessionID().isNull())
	{
		mCallIconLayoutPanel->setVisible(true);
	}
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

	// Note: for the moment, all that needs to be done is done by LLFolderViewItem::refresh()
	
	// Do the regular upstream refresh
	LLFolderViewFolder::refresh();
}

void LLConversationViewSession::onCurrentVoiceSessionChanged(const LLUUID& session_id)
{
	LLConversationItem* vmi = dynamic_cast<LLConversationItem*>(getViewModelItem());

	if (vmi)
	{
		bool is_active = vmi->getUUID() == session_id;
		bool is_nearby = vmi->getType() == LLConversationItem::CONV_SESSION_NEARBY;

		if (is_nearby)
		{
			mSpeakingIndicator->setSpeakerId(is_active ? gAgentID : LLUUID::null);
		}

		mSpeakingIndicator->switchIndicator(is_active);
		mCallIconLayoutPanel->setVisible(is_active);
	}
}

void LLConversationViewSession::drawOpenFolderArrow(const LLFolderViewItem::Params& default_params, const LLUIColor& fg_color)
{
	LLConversationItem * itemp = dynamic_cast<LLConversationItem*>(getViewModelItem());
	if (itemp && itemp->getType() != LLConversationItem::CONV_SESSION_1_ON_1)
	{
		LLFolderViewFolder::drawOpenFolderArrow(default_params, fg_color);
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
    static LLUIColor sHighlightFgColor = LLUIColorTable::instance().getColor("MenuItemHighlightFgColor", DEFAULT_WHITE);
    static LLUIColor sHighlightBgColor = LLUIColorTable::instance().getColor("MenuItemHighlightBgColor", DEFAULT_WHITE);
    static LLUIColor sFocusOutlineColor = LLUIColorTable::instance().getColor("InventoryFocusOutlineColor", DEFAULT_WHITE);
    static LLUIColor sMouseOverColor = LLUIColorTable::instance().getColor("InventoryMouseOverColor", DEFAULT_WHITE);

    const BOOL show_context = (getRoot() ? getRoot()->getShowSelectionContext() : FALSE);

    const LLFontGL* font = getLabelFontForStyle(mLabelStyle);
    F32 right_x  = 0;

    F32 y = (F32)getRect().getHeight() - font->getLineHeight() - (F32)mTextPad;
    F32 text_left = (F32)getLabelXPos();
    LLColor4 color = mIsSelected ? sHighlightFgColor : sFgColor;

    drawHighlight(show_context, mIsSelected, sHighlightBgColor, sFocusOutlineColor, sMouseOverColor);
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

    return arranged;
}

void LLConversationViewParticipant::refresh()
{
	// Refresh the participant view from its model data
	LLConversationItemParticipant* vmi = dynamic_cast<LLConversationItemParticipant*>(getViewModelItem());
	vmi->resetRefresh();
	
	// *TODO: We should also do something with vmi->isModerator() to echo that state in the UI somewhat
	mSpeakingIndicator->setIsMuted(vmi->isMuted());
	
	// Do the regular upstream refresh
	LLFolderViewItem::refresh();
}

void LLConversationViewParticipant::addToFolder(LLFolderViewFolder* folder)
{
    //Add the item to the folder (conversation)
    LLFolderViewItem::addToFolder(folder);
	
    //Now retrieve the folder (conversation) UUID, which is the speaker session
    LLConversationItem* vmi = this->getParentFolder() ? dynamic_cast<LLConversationItem*>(this->getParentFolder()->getViewModelItem()) : NULL;
    if(vmi)
    {
        //Allows speaking icon image to be loaded based on mUUID
        mAvatarIcon->setValue(mUUID);

        //Allows the speaker indicator to be activated based on the user and conversation
        mSpeakingIndicator->setSpeakerId(mUUID, vmi->getUUID()); 
    }
}

void LLConversationViewParticipant::onInfoBtnClick()
{
	LLFloaterReg::showInstance("inspect_avatar", LLSD().with("avatar_id", mUUID));
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

BOOL LLConversationViewParticipant::handleMouseDown( S32 x, S32 y, MASK mask )
{
	LLConversationItem* item = NULL;
	LLConversationViewSession* session_widget =
			dynamic_cast<LLConversationViewSession *>(this->getParentFolder());
	if (session_widget)
	{
	    item = dynamic_cast<LLConversationItem*>(session_widget->getViewModelItem());
	}
    LLUUID session_id = item? item->getUUID() : LLUUID();
    BOOL result = LLFolderViewItem::handleMouseDown(x, y, mask);
    (LLFloaterReg::getTypedInstance<LLFloaterIMContainer>("im_container"))->
        		selectConversationPair(session_id, false);

	return result;
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

// EOF


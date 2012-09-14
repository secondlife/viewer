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
#include "llconversationmodel.h"
#include "llimconversation.h"
#include "llimfloatercontainer.h"
#include "llfloaterreg.h"
#include "lluictrlfactory.h"

//
// Implementation of conversations list session widgets
//



LLConversationViewSession::Params::Params() :	
	container()
{}

LLConversationViewSession::LLConversationViewSession( const LLConversationViewSession::Params& p ):
	LLFolderViewFolder(p),
	mContainer(p.container)
{
}

void LLConversationViewSession::selectItem()
{
	LLFolderViewItem::selectItem();
	
	LLConversationItem* item = dynamic_cast<LLConversationItem*>(mViewModelItem);
	LLFloater* session_floater = LLIMConversation::getConversation(item->getUUID());
	LLMultiFloater* host_floater = session_floater->getHost();
	
	if (host_floater == mContainer)
	{
		// Always expand the message pane if the panel is hosted by the container
		mContainer->collapseMessagesPane(false);
		// Switch to the conversation floater that is being selected
		mContainer->selectFloater(session_floater);
	}
	
	// Set the focus on the selected floater
	session_floater->setFocus(TRUE);
}

void LLConversationViewSession::setVisibleIfDetached(BOOL visible)
{
	// Do this only if the conversation floater has been torn off (i.e. no multi floater host) and is not minimized
	// Note: minimized dockable floaters are brought to front hence unminimized when made visible and we don't want that here
	LLConversationItem* item = dynamic_cast<LLConversationItem*>(mViewModelItem);
	LLFloater* session_floater = LLIMConversation::getConversation(item->getUUID());
	
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

void LLConversationViewSession::refresh()
{
	// Refresh the session view from its model data
	LLConversationItem* vmi = dynamic_cast<LLConversationItem*>(getViewModelItem());
	vmi->resetRefresh();
	
	// Note: for the moment, all that needs to be done is done by LLFolderViewItem::refresh()
	
	// Do the regular upstream refresh
	LLFolderViewFolder::refresh();
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
info_button("info_button"),
output_monitor("output_monitor")
{}

LLConversationViewParticipant::LLConversationViewParticipant( const LLConversationViewParticipant::Params& p ):
	LLFolderViewItem(p),
    mInfoBtn(NULL),
    mSpeakingIndicator(NULL),
    mUUID(p.participant_id)
{

}

void LLConversationViewParticipant::initFromParams(const LLConversationViewParticipant::Params& params)
{	
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

    computeLabelRightPadding();
	return LLFolderViewItem::postBuild();
}

void LLConversationViewParticipant::refresh()
{
	// Refresh the participant view from its model data
	LLConversationItem* vmi = dynamic_cast<LLConversationItem*>(getViewModelItem());
	vmi->resetRefresh();
	
	// Note: for the moment, all that needs to be done is done by LLFolderViewItem::refresh()
	
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
        mSpeakingIndicator->setSpeakerId(mUUID, 
        vmi->getUUID()); //set the session id
    }
}

void LLConversationViewParticipant::onInfoBtnClick()
{
	LLFloaterReg::showInstance("inspect_avatar", LLSD().with("avatar_id", mUUID));
}

void LLConversationViewParticipant::onMouseEnter(S32 x, S32 y, MASK mask)
{
    mInfoBtn->setVisible(true);
    computeLabelRightPadding();
    LLFolderViewItem::onMouseEnter(x, y, mask);
}

void LLConversationViewParticipant::onMouseLeave(S32 x, S32 y, MASK mask)
{
    mInfoBtn->setVisible(false);
    computeLabelRightPadding();
    LLFolderViewItem::onMouseEnter(x, y, mask);
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

void LLConversationViewParticipant::computeLabelRightPadding()
{
    mLabelPaddingRight = DEFAULT_TEXT_PADDING_RIGHT;
    LLView* control;
    S32 ctrl_width;

    for (S32 i = 0; i < ALIC_COUNT; ++i)
    {
        control = getItemChildView((EAvatarListItemChildIndex)i);

        // skip invisible views
        if (!control->getVisible()) continue;

        ctrl_width = sChildrenWidths[i]; // including space between current & left controls
        // accumulate the amount of space taken by the controls
        mLabelPaddingRight += ctrl_width;
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


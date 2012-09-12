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
#include "llconversationmodel.h"
#include "llimconversation.h"
#include "llimfloatercontainer.h"


#include "lluictrlfactory.h"
#include "llavatariconctrl.h"

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

LLConversationViewParticipant::Params::Params() :	
container(),
participant_id(),
info_button("info_button"),
output_monitor("output_monitor")
{}

LLConversationViewParticipant::LLConversationViewParticipant( const LLConversationViewParticipant::Params& p ):
	LLFolderViewItem(p),
    mUUID(p.participant_id)
{

}

void LLConversationViewParticipant::initFromParams(const LLConversationViewParticipant::Params& params)
{	
	LLButton::Params info_button_params(params.info_button());
	LLButton * button = LLUICtrlFactory::create<LLButton>(info_button_params);
	addChild(button);	

    LLOutputMonitorCtrl::Params output_monitor_params(params.output_monitor());
    LLOutputMonitorCtrl * outputMonitor = LLUICtrlFactory::create<LLOutputMonitorCtrl>(output_monitor_params);
    addChild(outputMonitor);
}

BOOL LLConversationViewParticipant::postBuild()
{
	mInfoBtn = getChild<LLButton>("info_btn");
	mInfoBtn->setClickedCallback(boost::bind(&LLConversationViewParticipant::onInfoBtnClick, this));
	
	
	LLFolderViewItem::postBuild();
	return TRUE;
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

void LLConversationViewParticipant::onInfoBtnClick()
{
	
	
}

LLButton* LLConversationViewParticipant::createProfileButton()
{
	
	LLButton::Params params;
	
	
	//<button
	params.follows.flags(FOLLOWS_RIGHT);
	//params.height="20";
	LLUIImage * someImage = LLUI::getUIImage("Web_Profile_Off");
	params.image_overlay = someImage;
	params.layout="topleft";
	params.left_pad=5;
	//params.right="-28";
	params.name="profile_btn";
	params.tab_stop="false";
	params.tool_tip="View profile";
	params.top_delta=-2;
	//params.width="20";
	///>	
	
	
	/*
	LLConversationViewParticipant::Params params;
	
	params.name = item->getDisplayName();
	//params.icon = bridge->getIcon();
	//params.icon_open = bridge->getOpenIcon();
	//params.creation_date = bridge->getCreationDate();
	params.root = mConversationsRoot;
	params.listener = item;
	params.rect = LLRect (0, 0, 0, 0);
	params.tool_tip = params.name;
	params.container = this;
	*/
	
	LLButton * button = LLUICtrlFactory::create<LLButton>(params);
	LLRect someRect;
	someRect.setOriginAndSize(30, 0, 20, 20);
	button->setShape(someRect);
	
	//button->follows= "right";
	//button->height = 20;
	//button->image_overlay="Web_Profile_Off";
	//button->right = -28;
	//button->width = 20;

	
	
	return button;
}


void LLConversationViewParticipant::draw()
{
    LLFolderViewItem::draw();
}

// EOF


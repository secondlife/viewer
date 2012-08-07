/** 
 * @file llconversationmodel.cpp
 * @brief Implementation of conversations list
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

#include "llconversationmodel.h"
#include "llimfloatercontainer.h"

// Conversation items
LLConversationItem::LLConversationItem(std::string name, const LLUUID& uuid, LLFloater* floaterp, LLIMFloaterContainer* containerp) :
	LLFolderViewModelItemCommon(containerp->getRootViewModel()),
	mName(name),
	mUUID(uuid),
    mFloater(floaterp),
    mContainer(containerp)
{
}

LLConversationItem::LLConversationItem(LLIMFloaterContainer* containerp) :
	LLFolderViewModelItemCommon(containerp->getRootViewModel()),
	mName(""),
	mUUID(),
	mFloater(NULL),
	mContainer(NULL)
{
}


// Virtual action callbacks
void LLConversationItem::selectItem(void)
{
	LLMultiFloater* host_floater = mFloater->getHost();
	if (host_floater == mContainer)
	{
		// Always expand the message pane if the panel is hosted by the container
		mContainer->collapseMessagesPane(false);
		// Switch to the conversation floater that is being selected
		mContainer->selectFloater(mFloater);
	}
	// Set the focus on the selected floater
	mFloater->setFocus(TRUE);    
}

void LLConversationItem::setVisibleIfDetached(BOOL visible)
{
	// Do this only if the conversation floater has been torn off (i.e. no multi floater host) and is not minimized
	// Note: minimized dockable floaters are brought to front hence unminimized when made visible and we don't want that here
	if (!mFloater->getHost() && !mFloater->isMinimized())
	{
		mFloater->setVisible(visible);    
	}
}

void LLConversationItem::performAction(LLInventoryModel* model, std::string action)
{
}

void LLConversationItem::openItem( void )
{
}

void LLConversationItem::closeItem( void )
{
}

void LLConversationItem::previewItem( void )
{
}

void LLConversationItem::showProperties(void)
{
}

bool LLConversationSort::operator()(const LLConversationItem* const& a, const LLConversationItem* const& b) const
{
	// We compare only by name for the moment
	// *TODO : Implement the sorting by date
	S32 compare = LLStringUtil::compareDict(a->getDisplayName(), b->getDisplayName());
	return (compare < 0);
}

// EOF

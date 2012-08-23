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
#include "llimconversation.h"
#include "llimfloatercontainer.h"

// Conversation items
LLConversationItem::LLConversationItem(std::string display_name, const LLUUID& uuid, LLFolderViewModelInterface& root_view_model) :
	LLFolderViewModelItemCommon(root_view_model),
	mName(display_name),
	mUUID(uuid)
{
}

LLConversationItem::LLConversationItem(LLFolderViewModelInterface& root_view_model) :
	LLFolderViewModelItemCommon(root_view_model),
	mName(""),
	mUUID()
{
}


// Virtual action callbacks
void LLConversationItem::setVisibleIfDetached(BOOL visible)
{
	// Do this only if the conversation floater has been torn off (i.e. no multi floater host) and is not minimized
	// Note: minimized dockable floaters are brought to front hence unminimized when made visible and we don't want that here
	LLFloater* session_floater = LLIMConversation::getConversation(mUUID);

	if (session_floater && !session_floater->getHost() && !session_floater->isMinimized())
	{
		session_floater->setVisible(visible);
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

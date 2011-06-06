/** 
 * @file llpanelmarketplaceinbox.cpp
 * @brief Panel for marketplace inbox
 *
* $LicenseInfo:firstyear=2011&license=viewerlgpl$
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

#include "llpanelmarketplaceinbox.h"

#include "llbutton.h"
#include "llinventorypanel.h"

static LLRegisterPanelClassWrapper<LLPanelMarketplaceInbox> t_panel_marketplace_inbox("panel_marketplace_inbox");

// protected
LLPanelMarketplaceInbox::LLPanelMarketplaceInbox()
:	LLPanel()
{
}

LLPanelMarketplaceInbox::~LLPanelMarketplaceInbox()
{
}

// virtual
BOOL LLPanelMarketplaceInbox::postBuild()
{
	mInventoryPanel = getChild<LLInventoryPanel>("inventory_inbox");

	mInventoryPanel->setSortOrder(LLInventoryFilter::SO_DATE);

	return TRUE;
}

BOOL LLPanelMarketplaceInbox::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop, EDragAndDropType cargo_type, void *cargo_data, EAcceptance *accept, std::string& tooltip_msg)
{
	*accept = ACCEPT_NO;
	return TRUE;
}

void LLPanelMarketplaceInbox::draw()
{
	LLInventoryModel* model = mInventoryPanel->getModel();

	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;

	model->getDirectDescendentsOf(model->findCategoryUUIDForType(LLFolderType::FT_INBOX, false, false), cats, items);

	S32 item_count = cats->size() + items->size();

	if (item_count)
	{
		LLStringUtil::format_map_t args;
		args["[NUM]"] =  llformat ("%d", item_count);
		getChild<LLButton>("inbox_btn")->setLabel(getString("InboxLabelWithArg", args));
	}
	else
	{
		getChild<LLButton>("inbox_btn")->setLabel(getString("InboxLabelNoArg"));
	}
		
	LLPanel::draw();
}

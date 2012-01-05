/** 
 * @file llpanelmarketplaceoutboxinventory.cpp
 * @brief LLOutboxInventoryPanel  class definition
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

#include "llpanelmarketplaceoutboxinventory.h"

#include "llfolderview.h"
#include "llfoldervieweventlistener.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llpanellandmarks.h"
#include "llplacesinventorybridge.h"
#include "lltrans.h"
#include "llviewerfoldertype.h"


//
// statics
//

static LLDefaultChildRegistry::Register<LLOutboxInventoryPanel> r1("outbox_inventory_panel");
static LLDefaultChildRegistry::Register<LLOutboxFolderViewFolder> r2("outbox_folder_view_folder");


//
// LLOutboxInventoryPanel Implementation
//

LLOutboxInventoryPanel::LLOutboxInventoryPanel(const LLOutboxInventoryPanel::Params& p)
	: LLInventoryPanel(p)
{
}

LLOutboxInventoryPanel::~LLOutboxInventoryPanel()
{
}

// virtual
void LLOutboxInventoryPanel::buildFolderView(const LLInventoryPanel::Params& params)
{
	// Determine the root folder in case specified, and
	// build the views starting with that folder.
	
	LLUUID root_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false, false);
	
	// leslie -- temporary HACK to work around sim not creating outbox with proper system folder type
	if (root_id.isNull())
	{
		std::string start_folder_name(params.start_folder());
		
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		
		gInventory.getDirectDescendentsOf(gInventory.getRootFolderID(), cats, items);
		
		if (cats)
		{
			for (LLInventoryModel::cat_array_t::const_iterator cat_it = cats->begin(); cat_it != cats->end(); ++cat_it)
			{
				LLInventoryCategory* cat = *cat_it;
				
				if (cat->getName() == start_folder_name)
				{
					root_id = cat->getUUID();
					break;
				}
			}
		}
		
		if (root_id == LLUUID::null)
		{
			llwarns << "No category found that matches outbox inventory panel start_folder: " << start_folder_name << llendl;
		}
	}
	// leslie -- end temporary HACK
	
	if (root_id == LLUUID::null)
	{
		llwarns << "Outbox inventory panel has no root folder!" << llendl;
		root_id = LLUUID::generateNewID();
	}
	
	LLInvFVBridge* new_listener = mInvFVBridgeBuilder->createBridge(LLAssetType::AT_CATEGORY,
																	LLAssetType::AT_CATEGORY,
																	LLInventoryType::IT_CATEGORY,
																	this,
																	NULL,
																	root_id);
	
	mFolderRoot = createFolderView(new_listener, params.use_label_suffix());
}

LLFolderViewFolder * LLOutboxInventoryPanel::createFolderViewFolder(LLInvFVBridge * bridge)
{
	LLOutboxFolderViewFolder::Params params;
	
	params.name = bridge->getDisplayName();
	params.icon = bridge->getIcon();
	params.icon_open = bridge->getOpenIcon();
	
	if (mShowItemLinkOverlays) // if false, then links show up just like normal items
	{
		params.icon_overlay = LLUI::getUIImage("Inv_Link");
	}
	
	params.root = mFolderRoot;
	params.listener = bridge;
	params.tool_tip = params.name;
	
	return LLUICtrlFactory::create<LLOutboxFolderViewFolder>(params);
}

LLFolderViewItem * LLOutboxInventoryPanel::createFolderViewItem(LLInvFVBridge * bridge)
{
	LLFolderViewItem::Params params;

	params.name = bridge->getDisplayName();
	params.icon = bridge->getIcon();
	params.icon_open = bridge->getOpenIcon();

	if (mShowItemLinkOverlays) // if false, then links show up just like normal items
	{
		params.icon_overlay = LLUI::getUIImage("Inv_Link");
	}

	params.creation_date = bridge->getCreationDate();
	params.root = mFolderRoot;
	params.listener = bridge;
	params.rect = LLRect (0, 0, 0, 0);
	params.tool_tip = params.name;

	return LLUICtrlFactory::create<LLOutboxFolderViewItem>(params);
}

//
// LLOutboxFolderViewFolder Implementation
//

LLOutboxFolderViewFolder::LLOutboxFolderViewFolder(const Params& p)
	: LLFolderViewFolder(p)
{
}

//
// LLOutboxFolderViewItem Implementation
//

LLOutboxFolderViewItem::LLOutboxFolderViewItem(const Params& p)
	: LLFolderViewItem(p)
{
}

BOOL LLOutboxFolderViewItem::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	return TRUE;
}

// eof

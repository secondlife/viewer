/** 
 * @file llpanelmarketplaceinboxinventory.cpp
 * @brief LLInboxInventoryPanel  class definition
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

#include "llpanelmarketplaceinboxinventory.h"

#include "llfolderview.h"
#include "llfoldervieweventlistener.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llpanellandmarks.h"
#include "llplacesinventorybridge.h"
#include "llviewerfoldertype.h"


//
// statics
//

static LLDefaultChildRegistry::Register<LLInboxInventoryPanel> r1("inbox_inventory_panel");
static LLDefaultChildRegistry::Register<LLInboxFolderViewFolder> r2("inbox_folder_view_folder");


//
// LLInboxInventoryPanel Implementation
//

LLInboxInventoryPanel::LLInboxInventoryPanel(const LLInboxInventoryPanel::Params& p)
	: LLInventoryPanel(p)
{
}

LLInboxInventoryPanel::~LLInboxInventoryPanel()
{
}

// virtual
void LLInboxInventoryPanel::buildFolderView(const LLInventoryPanel::Params& params)
{
	// Determine the root folder in case specified, and
	// build the views starting with that folder.
	
	LLUUID root_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX, false, false);
	
	// leslie -- temporary HACK to work around sim not creating inbox with proper system folder type
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
			llwarns << "No category found that matches inbox inventory panel start_folder: " << start_folder_name << llendl;
		}
	}
	// leslie -- end temporary HACK
	
	if (root_id == LLUUID::null)
	{
		llwarns << "Inbox inventory panel has no root folder!" << llendl;
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

LLFolderViewFolder * LLInboxInventoryPanel::createFolderViewFolder(LLInvFVBridge * bridge)
{
	LLInboxFolderViewFolder::Params params;
	
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
	
	return LLUICtrlFactory::create<LLInboxFolderViewFolder>(params);
}

LLFolderViewItem * LLInboxInventoryPanel::createFolderViewItem(LLInvFVBridge * bridge)
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

	return LLUICtrlFactory::create<LLInboxFolderViewItem>(params);
}

//
// LLInboxFolderViewFolder Implementation
//

LLInboxFolderViewFolder::LLInboxFolderViewFolder(const Params& p)
	: LLFolderViewFolder(p)
	, LLBadgeOwner(getHandle())
	, mFresh(false)
{
#if SUPPORTING_FRESH_ITEM_COUNT
	initBadgeParams(p.new_badge());
#endif
}

LLInboxFolderViewFolder::~LLInboxFolderViewFolder()
{
}

// virtual
void LLInboxFolderViewFolder::draw()
{
#if SUPPORTING_FRESH_ITEM_COUNT
	if (!badgeHasParent())
	{
		addBadgeToParentPanel();
	}
	
	setBadgeVisibility(mFresh);
#endif

	LLFolderViewFolder::draw();
}

BOOL LLInboxFolderViewFolder::addToFolder(LLFolderViewFolder* folder, LLFolderView* root)
{
	BOOL retval = LLFolderViewFolder::addToFolder(folder, root);

	// Only mark top-level inbox folders as fresh
	mFresh = (mParentFolder == mRoot);

	return retval;
}

void LLInboxFolderViewFolder::updateFlag() const
{
	const std::string& last_expansion = gSavedPerAccountSettings.getString("LastInventoryInboxExpansion");

	if (!last_expansion.empty())
	{
		LLDate saved_freshness_date = LLDate(last_expansion);

		mFresh = (mCreationDate > saved_freshness_date.secondsSinceEpoch());
	}
}

void LLInboxFolderViewFolder::selectItem()
{
	LLFolderViewFolder::selectItem();

	mFresh = false;
}

void LLInboxFolderViewFolder::toggleOpen()
{
	LLFolderViewFolder::toggleOpen();

	mFresh = false;
}

void LLInboxFolderViewFolder::setCreationDate(time_t creation_date_utc) const
{ 
	mCreationDate = creation_date_utc; 

	if (mFresh)
	{
		updateFlag();
	}
}

//
// LLInboxFolderViewItem Implementation
//

BOOL LLInboxFolderViewItem::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	return TRUE;
}

// eof

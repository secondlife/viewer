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
#include "llfolderviewitem.h"
#include "llfolderviewmodel.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llpanellandmarks.h"
#include "llplacesinventorybridge.h"
#include "llviewerfoldertype.h"
#include "llsdserialize.h"


#define DEBUGGING_FRESHNESS	0

const LLColor4U DEFAULT_WHITE(255, 255, 255);

const std::string NEW_INBOX_FILENAME("inbox_new_items.xml");

//
// statics
//

static LLDefaultChildRegistry::Register<LLInboxInventoryPanel> r1("inbox_inventory_panel");
static LLDefaultChildRegistry::Register<LLInboxFolderViewFolder> r2("inbox_folder_view_folder");
static LLDefaultChildRegistry::Register<LLInboxFolderViewItem> r3("inbox_folder_view_item");


//
// LLInboxInventoryPanel Implementation
//

LLInboxInventoryPanel::LLInboxInventoryPanel(const LLInboxInventoryPanel::Params& p)
:	LLInventoryPanel(p)
{
	LLInboxNewItemsStorage::getInstance()->load();
}

LLInboxInventoryPanel::~LLInboxInventoryPanel()
{}

void LLInboxInventoryPanel::initFromParams(const LLInventoryPanel::Params& params)
{
	LLInventoryPanel::initFromParams(params);
	getFilter().setFilterCategoryTypes(getFilter().getFilterCategoryTypes() | (1ULL << LLFolderType::FT_INBOX));
}

LLFolderViewFolder * LLInboxInventoryPanel::createFolderViewFolder(LLInvFVBridge * bridge, bool allow_drop)
{
	LLUIColor item_color = LLUIColorTable::instance().getColor("MenuItemEnabledColor", DEFAULT_WHITE);

	LLInboxFolderViewFolder::Params params;
	
	params.name = bridge->getDisplayName();
	params.root = mFolderRoot.get();
	params.listener = bridge;
	params.tool_tip = params.name;
	params.font_color = item_color;
	params.font_highlight_color = item_color;
    params.allow_drop = allow_drop;
	
	return LLUICtrlFactory::create<LLInboxFolderViewFolder>(params);
}

LLFolderViewItem * LLInboxInventoryPanel::createFolderViewItem(LLInvFVBridge * bridge)
{
	LLUIColor item_color = LLUIColorTable::instance().getColor("MenuItemEnabledColor", DEFAULT_WHITE);

	LLInboxFolderViewItem::Params params;

	params.name = bridge->getDisplayName();
	params.creation_date = bridge->getCreationDate();
	params.root = mFolderRoot.get();
	params.listener = bridge;
	params.rect = LLRect (0, 0, 0, 0);
	params.tool_tip = params.name;
	params.font_color = item_color;
	params.font_highlight_color = item_color;

	return LLUICtrlFactory::create<LLInboxFolderViewItem>(params);
}

//
// LLInboxFolderViewFolder Implementation
//

LLInboxFolderViewFolder::LLInboxFolderViewFolder(const Params& p)
:	LLFolderViewFolder(p),
	LLBadgeOwner(getHandle()),
	mFresh(false)
{
	initBadgeParams(p.new_badge());
}

void LLInboxFolderViewFolder::addItem(LLFolderViewItem* item)
{
    LLFolderViewFolder::addItem(item);

    if(item)
    {
        LLInvFVBridge* itemBridge = static_cast<LLInvFVBridge*>(item->getViewModelItem());
        LLFolderBridge * bridge = static_cast<LLFolderBridge *>(getViewModelItem());
        bridge->updateHierarchyCreationDate(itemBridge->getCreationDate());
    }

    // Compute freshness if our parent is the root folder for the inbox
    if ((mParentFolder == mRoot) && !mFresh)
    {
        computeFreshness();
    }
}

// virtual
void LLInboxFolderViewFolder::draw()
{
	if (!hasBadgeHolderParent())
	{
		addBadgeToParentHolder();
		setDrawBadgeAtTop(true);
	}

	setBadgeVisibility(mFresh);

	LLFolderViewFolder::draw();

	if (mFresh)
	{
		reshapeBadge(getRect());
	}

}

BOOL LLInboxFolderViewFolder::handleMouseDown( S32 x, S32 y, MASK mask )
{
	deFreshify();
	return LLFolderViewFolder::handleMouseDown(x, y, mask);
}

BOOL LLInboxFolderViewFolder::handleDoubleClick( S32 x, S32 y, MASK mask )
{
	deFreshify();
	return LLFolderViewFolder::handleDoubleClick(x, y, mask);
}

void LLInboxFolderViewFolder::selectItem()
{
	deFreshify();
	LLFolderViewFolder::selectItem();
}

void LLInboxFolderViewFolder::computeFreshness()
{
	LLFolderViewModelItemInventory* view_model = static_cast<LLFolderViewModelItemInventory*>(getViewModelItem());
	const U32 last_expansion_utc = gSavedPerAccountSettings.getU32("LastInventoryInboxActivity");

	if (last_expansion_utc > 0)
	{
		mFresh = (view_model->getCreationDate() > last_expansion_utc) || LLInboxNewItemsStorage::getInstance()->isItemFresh(view_model->getUUID());

#if DEBUGGING_FRESHNESS
		if (mFresh)
		{
			LL_INFOS() << "Item is fresh! -- creation " << mCreationDate << ", saved_freshness_date " << last_expansion_utc << LL_ENDL;
		}
#endif
	}
	else
	{
		mFresh = true;
	}

	if (mFresh)
	{
		LLInboxNewItemsStorage::getInstance()->addFreshItem(view_model->getUUID());
	}
}

void LLInboxFolderViewFolder::deFreshify()
{
	mFresh = false;

	gSavedPerAccountSettings.setU32("LastInventoryInboxActivity", time_corrected());
	LLInboxNewItemsStorage::getInstance()->removeItem(static_cast<LLFolderViewModelItemInventory*>(getViewModelItem())->getUUID());
}

//
// LLInboxFolderViewItem Implementation
//

LLInboxFolderViewItem::LLInboxFolderViewItem(const Params& p)
	: LLFolderViewItem(p)
	, LLBadgeOwner(getHandle())
	, mFresh(false)
{
	initBadgeParams(p.new_badge());
}

void LLInboxFolderViewItem::addToFolder(LLFolderViewFolder* folder)
{
	LLFolderViewItem::addToFolder(folder);

	// Compute freshness if our parent is the root folder for the inbox
	if (mParentFolder == mRoot)
	{
		computeFreshness();
	}
}

BOOL LLInboxFolderViewItem::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	deFreshify();
	
	return LLFolderViewItem::handleDoubleClick(x, y, mask);
}

// virtual
void LLInboxFolderViewItem::draw()
{
	if (!hasBadgeHolderParent())
	{
		addBadgeToParentHolder();
	}

	setBadgeVisibility(mFresh);

	LLFolderViewItem::draw();
}

void LLInboxFolderViewItem::selectItem()
{
	deFreshify();

	LLFolderViewItem::selectItem();
}

void LLInboxFolderViewItem::computeFreshness()
{
	const U32 last_expansion_utc = gSavedPerAccountSettings.getU32("LastInventoryInboxActivity");

	if (last_expansion_utc > 0)
	{
		mFresh = (static_cast<LLFolderViewModelItemInventory*>(getViewModelItem())->getCreationDate() > last_expansion_utc);

#if DEBUGGING_FRESHNESS
		if (mFresh)
		{
			LL_INFOS() << "Item is fresh! -- creation " << mCreationDate << ", saved_freshness_date " << last_expansion_utc << LL_ENDL;
		}
#endif
	}
	else
	{
		mFresh = true;
	}
}

void LLInboxFolderViewItem::deFreshify()
{
	mFresh = false;

	gSavedPerAccountSettings.setU32("LastInventoryInboxActivity", time_corrected());
}

LLInboxNewItemsStorage::LLInboxNewItemsStorage()
{
}

// static
void LLInboxNewItemsStorage::destroyClass()
{
	LLInboxNewItemsStorage::getInstance()->saveNewItemsIds();
}

void LLInboxNewItemsStorage::saveNewItemsIds()
{
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, NEW_INBOX_FILENAME);
	if (!filename.empty())
	{
		LLSD uuids_data;
		for (std::set<LLUUID>::const_iterator it = mNewItemsIDs.begin(); it != mNewItemsIDs.end(); it++)
		{
			uuids_data.append((*it));
		}

		llofstream file;
		file.open(filename.c_str());
		if ( file.is_open() )
		{
			LLSDSerialize::toPrettyXML(uuids_data, file);
			file.close();
		}
	}
}

void LLInboxNewItemsStorage::load()
{
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, NEW_INBOX_FILENAME);
	if (!filename.empty())
	{
		llifstream in_file;
		in_file.open(filename.c_str());

		LLSD uuids_data;
		if (in_file.is_open())
		{
			LLSDSerialize::fromXML(uuids_data, in_file);
			in_file.close();
			for (LLSD::array_iterator i = uuids_data.beginArray(); i != uuids_data.endArray(); ++i)
			{
				mNewItemsIDs.insert((*i).asUUID());
			}
		}
	}
}
// eof

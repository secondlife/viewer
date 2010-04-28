/** 
 * @file llcofwearables.cpp
 * @brief LLCOFWearables displayes wearables from the current outfit split into three lists (attachments, clothing and body parts)
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llcofwearables.h"

#include "llappearancemgr.h"
#include "llinventory.h"
#include "llinventoryitemslist.h"
#include "llinventoryfunctions.h"

static LLRegisterPanelClassWrapper<LLCOFWearables> t_cof_wearables("cof_wearables");

const LLSD REARRANGE = LLSD().with("rearrange", LLSD());


LLCOFWearables::LLCOFWearables() : LLPanel(),
	mAttachments(NULL),
	mClothing(NULL),
	mBodyParts(NULL),
	mLastSelectedList(NULL)
{
};


// virtual
BOOL LLCOFWearables::postBuild()
{
	mAttachments = getChild<LLFlatListView>("list_attachments");
	mClothing = getChild<LLFlatListView>("list_clothing");
	mBodyParts = getChild<LLFlatListView>("list_body_parts");


	//selection across different list/tabs is not supported
	mAttachments->setCommitCallback(boost::bind(&LLCOFWearables::onSelectionChange, this, mAttachments));
	mClothing->setCommitCallback(boost::bind(&LLCOFWearables::onSelectionChange, this, mClothing));
	mBodyParts->setCommitCallback(boost::bind(&LLCOFWearables::onSelectionChange, this, mBodyParts));
	
	mAttachments->setCommitOnSelectionChange(true);
	mClothing->setCommitOnSelectionChange(true);
	mBodyParts->setCommitOnSelectionChange(true);

	return LLPanel::postBuild();
}

void LLCOFWearables::onSelectionChange(LLFlatListView* selected_list)
{
	if (!selected_list) return;

	if (selected_list != mLastSelectedList)
	{
		if (selected_list != mAttachments) mAttachments->resetSelection(true);
		if (selected_list != mClothing) mClothing->resetSelection(true);
		if (selected_list != mBodyParts) mBodyParts->resetSelection(true);

		mLastSelectedList = selected_list;
	}

	onCommit();
}

void LLCOFWearables::refresh()
{
	clear();

	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	
	gInventory.collectDescendents(LLAppearanceMgr::getInstance()->getCOF(), cats, items, LLInventoryModel::EXCLUDE_TRASH);
	if (items.empty()) return;

	for (U32 i = 0; i < items.size(); ++i)
	{
		LLViewerInventoryItem* item = items.get(i);
		if (!item) continue;

		LLPanelInventoryListItem* item_panel = LLPanelInventoryListItem::createItemPanel(item);
		if (!item_panel) continue;

		switch (item->getType())
		{
			case LLAssetType::AT_OBJECT:
				mAttachments->addItem(item_panel, item->getUUID(), ADD_BOTTOM, false);
				break;

			case LLAssetType::AT_BODYPART:
				mBodyParts->addItem(item_panel, item->getUUID(), ADD_BOTTOM, false);
				break;

			case LLAssetType::AT_CLOTHING:
				mClothing->addItem(item_panel, item->getUUID(), ADD_BOTTOM, false);
				break;

			default: break;
		}
	}

	mAttachments->sort(); //*TODO by Name
	mAttachments->notify(REARRANGE); //notifying the parent about the list's size change (cause items were added with rearrange=false)
	
	mClothing->sort(); //*TODO by actual inventory item description
	mClothing->notify(REARRANGE);
	
	mBodyParts->sort(); //*TODO by name
	mBodyParts->notify(REARRANGE);
}


LLUUID LLCOFWearables::getSelectedUUID()
{
	if (!mLastSelectedList) return LLUUID::null;
	
	return mLastSelectedList->getSelectedUUID();
}

void LLCOFWearables::clear()
{
	mAttachments->clear();
	mClothing->clear();
	mBodyParts->clear();
}

//EOF

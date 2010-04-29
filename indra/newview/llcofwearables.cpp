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

#include "llwearableitemslist.h"
void LLCOFWearables::refresh()
{
	clear();

	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t cof_items;

	gInventory.collectDescendents(LLAppearanceMgr::getInstance()->getCOF(), cats, cof_items, LLInventoryModel::EXCLUDE_TRASH);

	populateAttachmentsAndBodypartsLists(cof_items);


	LLAppearanceMgr::wearables_by_type_t clothing_by_type(WT_COUNT);
	LLAppearanceMgr::getInstance()->divvyWearablesByType(cof_items, clothing_by_type);
	
	populateClothingList(clothing_by_type);
}


void LLCOFWearables::populateAttachmentsAndBodypartsLists(const LLInventoryModel::item_array_t& cof_items)
{
	for (U32 i = 0; i < cof_items.size(); ++i)
	{
		LLViewerInventoryItem* item = cof_items.get(i);
		if (!item) continue;

		const LLAssetType::EType item_type = item->getType();
		if (item_type == LLAssetType::AT_CLOTHING) continue;
		LLPanelInventoryListItemBase* item_panel = NULL;
		if (item_type == LLAssetType::AT_OBJECT)
		{
				item_panel = LLPanelInventoryListItemBase::create(item);
			mAttachments->addItem(item_panel, item->getUUID(), ADD_BOTTOM, false);
		}
		else if (item_type == LLAssetType::AT_BODYPART)
		{
			item_panel = LLPanelBodyPartsListItem::create(item);
			mBodyParts->addItem(item_panel, item->getUUID(), ADD_BOTTOM, false);
			addWearableTypeSeparator(mBodyParts);
		}
	}

	if (mAttachments->size())
	{
		mAttachments->sort(); //*TODO by Name
		mAttachments->notify(REARRANGE); //notifying the parent about the list's size change (cause items were added with rearrange=false)
	}

	if (mBodyParts->size())
	{
		mBodyParts->sort(); //*TODO by name
	}

	addListButtonBar(mBodyParts, "panel_bodyparts_list_button_bar.xml");
	mBodyParts->notify(REARRANGE);
}


void LLCOFWearables::populateClothingList(LLAppearanceMgr::wearables_by_type_t& clothing_by_type)
{
	llassert(clothing_by_type.size() == WT_COUNT);

	addListButtonBar(mClothing, "panel_clothing_list_button_bar.xml");

	for (U32 type = WT_SHIRT; type < WT_COUNT; ++type)
	{
		U32 size = clothing_by_type[type].size();
		if (!size) continue;

		LLAppearanceMgr::sortItemsByActualDescription(clothing_by_type[type]);

		for (U32 i = 0; i < size; i++)
		{
			LLViewerInventoryItem* item = clothing_by_type[type][i];

			LLPanelInventoryListItemBase* item_panel = LLPanelClothingListItem::create(item);
			if (!item_panel) continue;

			mClothing->addItem(item_panel, item->getUUID(), ADD_BOTTOM, false);
		}

		addWearableTypeSeparator(mClothing);
	}

	addClothingTypesDummies(clothing_by_type);

	mClothing->notify(REARRANGE);
}

void LLCOFWearables::addListButtonBar(LLFlatListView* list, std::string xml_filename)
{
	llassert(list);
	llassert(xml_filename.length());
	
	LLPanel::Params params;
	LLPanel* button_bar = LLUICtrlFactory::create<LLPanel>(params);
	LLUICtrlFactory::instance().buildPanel(button_bar, xml_filename);

	LLRect rc = button_bar->getRect();
	button_bar->reshape(list->getItemsRect().getWidth(), rc.getHeight());

	list->addItem(button_bar, LLUUID::null, ADD_TOP, false);
}

//adding dummy items for missing wearable types
void LLCOFWearables::addClothingTypesDummies(const LLAppearanceMgr::wearables_by_type_t& clothing_by_type)
{
	llassert(clothing_by_type.size() == WT_COUNT);
	
	for (U32 type = WT_SHIRT; type < WT_COUNT; type++)
	{
		U32 size = clothing_by_type[type].size();
		if (size) continue;

		//*TODO create dummy item panel
		
		//*TODO add dummy item panel -> mClothing->addItem(dummy_item_panel, item->getUUID(), ADD_BOTTOM, false);

		addWearableTypeSeparator(mClothing);
	}
}

void LLCOFWearables::addWearableTypeSeparator(LLFlatListView* list)
{
	llassert(list);
	
	static LLXMLNodePtr separator_xml_node = getXMLNode("panel_wearable_type_separator.xml");
	if (separator_xml_node->isNull()) return;

	LLPanel* separator = LLUICtrlFactory::defaultBuilder<LLPanel>(separator_xml_node, NULL, NULL);

	LLRect rc = separator->getRect();
	rc.setOriginAndSize(0, 0, list->getItemsRect().getWidth(), rc.getHeight());
	separator->setRect(rc);

	list->addItem(separator, LLUUID::null, ADD_BOTTOM, false);
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

LLXMLNodePtr LLCOFWearables::getXMLNode(std::string xml_filename)
{
	LLXMLNodePtr xmlNode = NULL;
	bool success = LLUICtrlFactory::getLayeredXMLNode(xml_filename, xmlNode);
	if (!success)
	{
		llwarning("Failed to read xml", 0);
		return NULL;
	}

	return xmlNode;
}

//EOF

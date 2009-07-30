/** 
 * @file llpanelteleporthistory.cpp
 * @brief Teleport history represented by a scrolling list
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "llfloaterreg.h"

#include "llfloaterworldmap.h"
#include "llpanelteleporthistory.h"
#include "llsidetray.h"
#include "llworldmap.h"

// Not yet implemented; need to remove buildPanel() from constructor when we switch
//static LLRegisterPanelClassWrapper<LLTeleportHistoryPanel> t_teleport_history("panel_teleport_history");

LLTeleportHistoryPanel::LLTeleportHistoryPanel()
	:	LLPanelPlacesTab(),
		mFilterSubString(LLStringUtil::null),
		mTeleportHistory(NULL),
		mHistoryItems(NULL)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_teleport_history.xml");
}

LLTeleportHistoryPanel::~LLTeleportHistoryPanel()
{
}

BOOL LLTeleportHistoryPanel::postBuild()
{
	mTeleportHistory = LLTeleportHistory::getInstance();
	if (mTeleportHistory)
	{
		mTeleportHistory->setHistoryChangedCallback(boost::bind(&LLTeleportHistoryPanel::showTeleportHistory, this));
	}

	mHistoryItems = getChild<LLScrollListCtrl>("history_items");
	if (mHistoryItems)
	{
		mHistoryItems->setDoubleClickCallback(onDoubleClickItem, this);
		mHistoryItems->setCommitOnSelectionChange(FALSE);
		mHistoryItems->setCommitCallback(boost::bind(&LLTeleportHistoryPanel::handleItemSelect, this, _2));
	}

	return TRUE;
}

// virtual
void LLTeleportHistoryPanel::onSearchEdit(const std::string& string)
{
	if (mFilterSubString != string)
	{
		mFilterSubString = string;
		showTeleportHistory();
	}
}

// virtual
void LLTeleportHistoryPanel::onShowOnMap()
{
	LLScrollListItem* itemp = mHistoryItems->getFirstSelected();
	if(!itemp)
		return;

	S32 index = itemp->getColumn(LIST_INDEX)->getValue().asInteger();

	const LLTeleportHistory::slurl_list_t& hist_items = mTeleportHistory->getItems();

	LLVector3d global_pos = hist_items[index].mGlobalPos;
	
	if (!global_pos.isExactlyZero())
	{
		LLFloaterWorldMap::getInstance()->trackLocation(global_pos);
		LLFloaterReg::showInstance("world_map", "center");
	}
}

// virtual
void LLTeleportHistoryPanel::onTeleport()
{
	LLScrollListItem* itemp = mHistoryItems->getFirstSelected();
	if(!itemp)
		return;

	S32 index = itemp->getColumn(LIST_INDEX)->getValue().asInteger();

	// teleport to existing item in history, so we don't add it again
	mTeleportHistory->goToItem(index);
}

/*
// virtual
void LLTeleportHistoryPanel::onCopySLURL()
{
	LLScrollListItem* itemp = mHistoryItems->getFirstSelected();
	if(!itemp)
		return;

	S32 index = itemp->getColumn(LIST_INDEX)->getValue().asInteger();

	const LLTeleportHistory::slurl_list_t& hist_items = mTeleportHistory->getItems();

	LLVector3d global_pos = hist_items[index].mGlobalPos;

	U64 new_region_handle = to_region_handle(global_pos);

	LLWorldMap::url_callback_t cb = boost::bind(
			&LLPanelPlacesTab::onRegionResponse, this,
			global_pos, _1, _2, _3, _4);

	LLWorldMap::getInstance()->sendHandleRegionRequest(new_region_handle, cb, std::string("unused"), false);
}
*/

// virtual
void LLTeleportHistoryPanel::updateVerbs()
{
	if (!isTabVisible()) 
		return;

	S32 index = 0;
	S32 cur_item = 0;

	LLScrollListItem* itemp = mHistoryItems->getFirstSelected();
	if (itemp)
	{
		index = itemp->getColumn(LIST_INDEX)->getValue().asInteger();
		cur_item = mTeleportHistory->getCurrentItemIndex();
	}

	mTeleportBtn->setEnabled(index != cur_item);
	mShowOnMapBtn->setEnabled(itemp != NULL);
}

void LLTeleportHistoryPanel::showTeleportHistory()
{
	const LLTeleportHistory::slurl_list_t& hist_items = mTeleportHistory->getItems();

	mHistoryItems->deleteAllItems();

	S32 cur_item = mTeleportHistory->getCurrentItemIndex();

	for (LLTeleportHistory::slurl_list_t::const_iterator iter = hist_items.begin();
				iter != hist_items.end(); ++iter)
	{
		std::string landmark_title = (*iter).mTitle;
		LLStringUtil::toUpper(landmark_title);

		std::string::size_type match_offset = mFilterSubString.size() ? landmark_title.find(mFilterSubString) : std::string::npos;
		bool passed = mFilterSubString.size() == 0 || match_offset != std::string::npos;
	
		if (!passed)
			continue;

		S32 index = iter - hist_items.begin();

		LLSD row;
		row["id"] = index;

		LLSD& icon_column = row["columns"][LIST_ICON];
		icon_column["column"] = "landmark_icon";
		icon_column["type"] = "icon";
		icon_column["value"] = "inv_item_landmark.tga";

		LLSD& region_column = row["columns"][LIST_ITEM_TITLE];
		region_column["column"] = "region";
		region_column["type"] = "text";
		region_column["value"] = (*iter).mTitle;

		LLSD& index_column = row["columns"][LIST_INDEX];
		index_column["column"] = "index";
		index_column["type"] = "text";
		index_column["value"] = index;

		mHistoryItems->addElement(row);

		if (cur_item == index)
		{
			LLScrollListItem* itemp = mHistoryItems->getItem(index);
			((LLScrollListText*)itemp->getColumn(LIST_ITEM_TITLE))->setFontStyle(LLFontGL::BOLD);
		}
	}

	updateVerbs();
}

void LLTeleportHistoryPanel::handleItemSelect(const LLSD& data)
{
	updateVerbs();
}

//static
void LLTeleportHistoryPanel::onDoubleClickItem(void* user_data)
{
	LLTeleportHistoryPanel* self = (LLTeleportHistoryPanel*)user_data;
	
	LLScrollListItem* itemp = self->mHistoryItems->getFirstSelected();
	if(!itemp)
		return;

	LLSD key;
	key["type"] = "teleport_history";
	key["id"] = itemp->getColumn(LIST_INDEX)->getValue().asInteger();

	LLSideTray::getInstance()->showPanel("panel_places", key);
}

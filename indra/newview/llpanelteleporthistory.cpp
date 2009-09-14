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
#include "llteleporthistorystorage.h"
#include "llaccordionctrl.h"
#include "llaccordionctrltab.h"

// Not yet implemented; need to remove buildPanel() from constructor when we switch
//static LLRegisterPanelClassWrapper<LLTeleportHistoryPanel> t_teleport_history("panel_teleport_history");

LLTeleportHistoryPanel::LLTeleportHistoryPanel()
	:	LLPanelPlacesTab(),
		mFilterSubString(LLStringUtil::null),
		mTeleportHistory(NULL),
		mHistoryAccordeon(NULL),
		mLastSelectedScrollList(NULL)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_teleport_history.xml");
}

LLTeleportHistoryPanel::~LLTeleportHistoryPanel()
{
}

BOOL LLTeleportHistoryPanel::postBuild()
{
	mTeleportHistory = LLTeleportHistoryStorage::getInstance();
	if (mTeleportHistory)
	{
		mTeleportHistory->setHistoryChangedCallback(boost::bind(&LLTeleportHistoryPanel::showTeleportHistory, this));
	}

	mHistoryAccordeon = getChild<LLAccordionCtrl>("history_accordion");

	if (mHistoryAccordeon)
	{
		
		for (child_list_const_iter_t iter = mHistoryAccordeon->beginChild(); iter != mHistoryAccordeon->endChild(); iter++)
		{
			if (dynamic_cast<LLAccordionCtrlTab*>(*iter))
			{
				LLAccordionCtrlTab* tab = (LLAccordionCtrlTab*)*iter;
				mItemContainers.put(tab);

				LLScrollListCtrl* sl = getScrollListFromTab(tab);
				if (sl)
				{
					sl->setDoubleClickCallback(onDoubleClickItem, this);
					sl->setCommitOnSelectionChange(FALSE);
					sl->setCommitCallback(boost::bind(&LLTeleportHistoryPanel::handleItemSelect, this, sl));
				}

			}
		}
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
	if (!mLastSelectedScrollList)
		return;

	LLScrollListItem* itemp = mLastSelectedScrollList->getFirstSelected();
	if(!itemp)
		return;

	S32 index = itemp->getColumn(LIST_INDEX)->getValue().asInteger();

	LLVector3d global_pos = mTeleportHistory->getItems()[mTeleportHistory->getItems().size() - 1 - index].mGlobalPos;
	
	if (!global_pos.isExactlyZero())
	{
		LLFloaterWorldMap::getInstance()->trackLocation(global_pos);
		LLFloaterReg::showInstance("world_map", "center");
	}
}

// virtual
void LLTeleportHistoryPanel::onTeleport()
{
	if (!mLastSelectedScrollList)
		return;

	LLScrollListItem* itemp = mLastSelectedScrollList->getFirstSelected();
	if(!itemp)
		return;

	S32 index = itemp->getColumn(LIST_INDEX)->getValue().asInteger();

	// teleport to existing item in history, so we don't add it again
	mTeleportHistory->goToItem(mTeleportHistory->getItems().size() - 1 - index);
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

	if (!mLastSelectedScrollList)
	{
		mTeleportBtn->setEnabled(false);
		mShowOnMapBtn->setEnabled(false);
		return;
	}

	LLScrollListItem* itemp = mLastSelectedScrollList->getFirstSelected();

	mTeleportBtn->setEnabled(NULL != itemp && 0 < itemp->getColumn(LIST_INDEX)->getValue().asInteger());
	mShowOnMapBtn->setEnabled(NULL != itemp);
}

void LLTeleportHistoryPanel::showTeleportHistory()
{
	if (!mHistoryAccordeon)
		return;

	const LLTeleportHistoryStorage::slurl_list_t& hist_items = mTeleportHistory->getItems();

	const U32 seconds_in_day = 24 * 60 * 60;
	LLDate curr_date =  LLDate::now();

	curr_date.secondsSinceEpoch(curr_date.secondsSinceEpoch() + seconds_in_day);

	S32 curr_tab = -1;
	S32 tabs_cnt = mItemContainers.size();
	S32 curr_year = 0, curr_month = 0, curr_day = 0;
	
	LLScrollListCtrl *curr_scroll_list = NULL;

	S32 index = 0;

	for (LLTeleportHistoryStorage::slurl_list_t::const_reverse_iterator iter = hist_items.rbegin();
	      iter != hist_items.rend(); ++iter)
	{
		std::string landmark_title = (*iter).mTitle;
		LLStringUtil::toUpper(landmark_title);

		std::string::size_type match_offset = mFilterSubString.size() ? landmark_title.find(mFilterSubString) : std::string::npos;
		bool passed = mFilterSubString.size() == 0 || match_offset != std::string::npos;
	
		if (!passed)
			continue;
		
		if (curr_tab < tabs_cnt - 1)
		{
			const LLDate &date = (*iter).mDate;

			S32 year, month, day;
			if (!date.split(&year, &month, &day))
			{
				llwarns << "Failed to split date: " << date << llendl;
				continue;
			}

			if (day != curr_day || month != curr_month || year != curr_year)
			{
				LLAccordionCtrlTab* tab = NULL;
				while (curr_tab < tabs_cnt - 1 && (day != curr_day || month != curr_month || year != curr_year))
				{
					curr_tab++;

					tab = mItemContainers.get(mItemContainers.size() - 1 - curr_tab);
					tab->setVisible(false);

					curr_date.secondsSinceEpoch(curr_date.secondsSinceEpoch() - seconds_in_day);
					curr_date.split(&curr_year, &curr_month, &curr_day);
				}				

				tab->setVisible(true);

				curr_scroll_list = getScrollListFromTab(tab);
				if (curr_scroll_list)
				{
					curr_scroll_list->deleteAllItems();
				}
			}
		}

		LLSD row;
		row["id"] = index;

		if (curr_scroll_list)
		{
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

			index++;

			curr_scroll_list->addElement(row);
		}
	}

	mHistoryAccordeon->arrange();

	updateVerbs();
}

void LLTeleportHistoryPanel::handleItemSelect(LLScrollListCtrl* sl)
{
	mLastSelectedScrollList = sl;
	updateVerbs();
}

//static
void LLTeleportHistoryPanel::onDoubleClickItem(void* user_data)
{
	/*LLTeleportHistoryPanel* self = (LLTeleportHistoryPanel*)user_data;
	
	LLScrollListItem* itemp = self->mHistoryItems->getFirstSelected();
	if(!itemp)
		return;

	LLSD key;
	key["type"] = "teleport_history";
	key["id"] = itemp->getColumn(LIST_INDEX)->getValue().asInteger();

	LLSideTray::getInstance()->showPanel("panel_places", key);*/
}

LLScrollListCtrl* LLTeleportHistoryPanel::getScrollListFromTab(LLAccordionCtrlTab *tab)
{
	for (child_list_const_iter_t iter = tab->beginChild(); iter != tab->endChild(); iter++)
	{
		if (dynamic_cast<LLScrollListCtrl*>(*iter))
		{
			return (LLScrollListCtrl*)*iter; // There should be one scroll list per tab.
		}
	}

	return NULL;
}

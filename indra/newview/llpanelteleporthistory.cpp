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
#include "llflatlistview.h"
#include "lltextbox.h"
#include "llviewermenu.h"
#include "llviewerinventory.h"
#include "lllandmarkactions.h"
#include "llclipboard.h"

class LLTeleportHistoryFlatItem : public LLPanel
{
public:
	LLTeleportHistoryFlatItem(S32 index, LLTeleportHistoryPanel::ContextMenu *context_menu, const std::string &region_name);
	virtual ~LLTeleportHistoryFlatItem() {};

	virtual BOOL postBuild();

	S32 getIndex() { return mIndex; }

	/*virtual*/ void setValue(const LLSD& value);

	void onMouseEnter(S32 x, S32 y, MASK mask);
	void onMouseLeave(S32 x, S32 y, MASK mask);
	virtual BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

	static void showPlaceInfoPanel(S32 index);
private:
	void onInfoBtnClick();

	LLButton* mInfoBtn;
	LLTeleportHistoryPanel::ContextMenu *mContextMenu;

	S32 mIndex;
	std::string mRegionName;
};

LLTeleportHistoryFlatItem::LLTeleportHistoryFlatItem(S32 index, LLTeleportHistoryPanel::ContextMenu *context_menu, const std::string &region_name)
:	LLPanel(),
	mIndex(index),
	mContextMenu(context_menu),
	mRegionName(region_name)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_teleport_history_item.xml");
}

//virtual
BOOL LLTeleportHistoryFlatItem::postBuild()
{
	LLTextBox *region = getChild<LLTextBox>("region");
	region->setValue(mRegionName);

	mInfoBtn = getChild<LLButton>("info_btn");
	mInfoBtn->setClickedCallback(boost::bind(&LLTeleportHistoryFlatItem::onInfoBtnClick, this));

	return true;
}

void LLTeleportHistoryFlatItem::setValue(const LLSD& value)
{
	if (!value.isMap()) return;;
	if (!value.has("selected")) return;
	childSetVisible("selected_icon", value["selected"]);
}

void LLTeleportHistoryFlatItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	childSetVisible("hovered_icon", true);
	mInfoBtn->setVisible(true);

	LLPanel::onMouseEnter(x, y, mask);
}

void LLTeleportHistoryFlatItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	childSetVisible("hovered_icon", false);
	mInfoBtn->setVisible(false);

	LLPanel::onMouseLeave(x, y, mask);
}

// virtual
BOOL LLTeleportHistoryFlatItem::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if (mContextMenu)
		mContextMenu->show(this, mIndex, x, y);

	return LLPanel::handleRightMouseDown(x, y, mask);
}

void LLTeleportHistoryFlatItem::showPlaceInfoPanel(S32 index)
{
	LLSD params;
	params["id"] = index;
	params["type"] = "teleport_history";

	LLSideTray::getInstance()->showPanel("panel_places", params);
}

void LLTeleportHistoryFlatItem::onInfoBtnClick()
{
	LLTeleportHistoryFlatItem::showPlaceInfoPanel(mIndex);
}

LLTeleportHistoryPanel::ContextMenu::ContextMenu() :
	mMenu(NULL)
{
}

void LLTeleportHistoryPanel::ContextMenu::show(LLView* spawning_view, S32 index, S32 x, S32 y)
{
	if (mMenu)
	{
		//preventing parent (menu holder) from deleting already "dead" context menus on exit
		LLView* parent = mMenu->getParent();
		if (parent)
		{
			parent->removeChild(mMenu);
			mMenu->setParent(NULL);
		}
		delete mMenu;
	}

	mIndex = index;
	mMenu = createMenu();

	LLViewerInventoryItem *landmark = LLLandmarkActions::findLandmarkForGlobalPos(
		LLTeleportHistoryStorage::getInstance()->getItems()[index].mGlobalPos);

	mMenu->setItemEnabled("Make Landmark", !landmark || landmark->getUUID().isNull());

	mMenu->show(x, y);
	LLMenuGL::showPopup(spawning_view, mMenu, x, y);
}

LLContextMenu* LLTeleportHistoryPanel::ContextMenu::createMenu()
{
	// set up the callbacks for all of the avatar menu items
	// (N.B. callbacks don't take const refs as mID is local scope)
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;

	registrar.add("TeleportHistory.Teleport",	boost::bind(&LLTeleportHistoryPanel::ContextMenu::onTeleport, this));
	registrar.add("TeleportHistory.MoreInformation",boost::bind(&LLTeleportHistoryPanel::ContextMenu::onInfo, this));
	registrar.add("TeleportHistory.Copy",		boost::bind(&LLTeleportHistoryPanel::ContextMenu::onCopy, this));
	registrar.add("TeleportHistory.MakeLandmark",	boost::bind(&LLTeleportHistoryPanel::ContextMenu::onMakeLandmark, this));

	// create the context menu from the XUI
	return LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>(
		"menu_teleport_history_item.xml", LLMenuGL::sMenuContainer, LLViewerMenuHolderGL::child_registry_t::instance());
}

void LLTeleportHistoryPanel::ContextMenu::onTeleport()
{
	LLTeleportHistoryStorage::getInstance()->goToItem(mIndex);
}

void LLTeleportHistoryPanel::ContextMenu::onInfo()
{
	LLTeleportHistoryFlatItem::showPlaceInfoPanel(mIndex);
}

//static
void LLTeleportHistoryPanel::ContextMenu::gotSLURLCallback(const std::string& slurl)
{
	gClipboard.copyFromString(utf8str_to_wstring(slurl));
}

void LLTeleportHistoryPanel::ContextMenu::onCopy()
{
	LLVector3d globalPos = LLTeleportHistoryStorage::getInstance()->getItems()[mIndex].mGlobalPos;
	LLLandmarkActions::getSLURLfromPosGlobal(globalPos,
		boost::bind(&LLTeleportHistoryPanel::ContextMenu::gotSLURLCallback, _1), false);
}

void LLTeleportHistoryPanel::ContextMenu::onMakeLandmark()
{
	//FIXME: it creates landmark for current agent positon, not for the global position of item of teleport history
	LLSideTray::getInstance()->showPanel("panel_places", LLSD().insert("type", "create_landmark"));
}

// Not yet implemented; need to remove buildPanel() from constructor when we switch
//static LLRegisterPanelClassWrapper<LLTeleportHistoryPanel> t_teleport_history("panel_teleport_history");

LLTeleportHistoryPanel::LLTeleportHistoryPanel()
	:	LLPanelPlacesTab(),
		mFilterSubString(LLStringUtil::null),
		mTeleportHistory(NULL),
		mHistoryAccordion(NULL),
		mAccordionTabMenu(NULL),
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

	mHistoryAccordion = getChild<LLAccordionCtrl>("history_accordion");

	if (mHistoryAccordion)
	{
		
		for (child_list_const_iter_t iter = mHistoryAccordion->beginChild(); iter != mHistoryAccordion->endChild(); iter++)
		{
			if (dynamic_cast<LLAccordionCtrlTab*>(*iter))
			{
				LLAccordionCtrlTab* tab = (LLAccordionCtrlTab*)*iter;
				tab->setRightMouseDownCallback(boost::bind(&LLTeleportHistoryPanel::onAccordionTabRightClick, this, _1, _2, _3, _4));

				mItemContainers.put(tab);

				LLFlatListView* fl = getFlatListViewFromTab(tab);
				if (fl)
				{
					fl->setCommitOnSelectionChange(true);
					fl->setDoubleClickCallback(boost::bind(&LLTeleportHistoryPanel::onDoubleClickItem, this));
					fl->setCommitCallback(boost::bind(&LLTeleportHistoryPanel::handleItemSelect, this, fl));
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

	LLTeleportHistoryFlatItem* itemp = dynamic_cast<LLTeleportHistoryFlatItem *> (mLastSelectedScrollList->getSelectedItem());

	if(!itemp)
		return;

	LLVector3d global_pos = mTeleportHistory->getItems()[itemp->getIndex()].mGlobalPos;
	
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

	LLTeleportHistoryFlatItem* itemp = dynamic_cast<LLTeleportHistoryFlatItem *> (mLastSelectedScrollList->getSelectedItem());
	if(!itemp)
		return;

	// teleport to existing item in history, so we don't add it again
	mTeleportHistory->goToItem(itemp->getIndex());
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

	LLTeleportHistoryFlatItem* itemp = dynamic_cast<LLTeleportHistoryFlatItem *> (mLastSelectedScrollList->getSelectedItem());

	mTeleportBtn->setEnabled(NULL != itemp && 0 < itemp->getIndex());
	mShowOnMapBtn->setEnabled(NULL != itemp);
}

void LLTeleportHistoryPanel::showTeleportHistory()
{
	if (!mHistoryAccordion)
		return;

	const LLTeleportHistoryStorage::slurl_list_t& hist_items = mTeleportHistory->getItems();

	const U32 seconds_in_day = 24 * 60 * 60;
	LLDate curr_date =  LLDate::now();

	S32 curr_tab = -1;
	S32 tabs_cnt = mItemContainers.size();
	S32 curr_year = 0, curr_month = 0, curr_day = 0;

	curr_date.split(&curr_year, &curr_month, &curr_day);
	curr_date.fromYMDHMS(curr_year, curr_month, curr_day); // Set hour, min, and sec to 0
	curr_date.secondsSinceEpoch(curr_date.secondsSinceEpoch() + seconds_in_day);
	
	LLFlatListView* curr_flat_view = NULL;

	S32 index = hist_items.size() - 1;

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

			if (date < curr_date)
			{
				LLAccordionCtrlTab* tab = NULL;
				while (curr_tab < tabs_cnt - 1 && date < curr_date)
				{
					curr_tab++;

					tab = mItemContainers.get(mItemContainers.size() - 1 - curr_tab);
					tab->setVisible(false);
					
					if (curr_tab <= tabs_cnt - 4)
					{
						curr_date.secondsSinceEpoch(curr_date.secondsSinceEpoch() - seconds_in_day);
					}
					else if (curr_tab == tabs_cnt - 3) // 6 day and older, low boundary is 1 month
					{
						curr_date =  LLDate::now();
						curr_date.split(&curr_year, &curr_month, &curr_day);
						curr_month--;
						if (0 == curr_month)
						{
							curr_month = 12;
							curr_year--;
						}
						curr_date.fromYMDHMS(curr_year, curr_month, curr_day);
					}
					else if (curr_tab == tabs_cnt - 2) // 1 month and older, low boundary is 6 months
					{
						curr_date =  LLDate::now();
						curr_date.split(&curr_year, &curr_month, &curr_day);
						if (curr_month > 6)
						{
							curr_month -= 6;
						}
						else
						{
							curr_month += 6;
							curr_year--;
						}
						curr_date.fromYMDHMS(curr_year, curr_month, curr_day);
						
					}
					else // 6 months and older
					{
						curr_date.secondsSinceEpoch(0);
					}
				}

				tab->setVisible(true);

				curr_flat_view = getFlatListViewFromTab(tab);
				if (curr_flat_view)
				{
					curr_flat_view->clear();
				}
			}
		}

		if (curr_flat_view)
		{			
			curr_flat_view->addItem(new LLTeleportHistoryFlatItem(index, &mContextMenu, (*iter).mTitle));
		}

		index--;
	}

	// Hide empty tabs from current to bottom
	for (curr_tab++; curr_tab < tabs_cnt; curr_tab++)
		mItemContainers.get(mItemContainers.size() - 1 - curr_tab)->setVisible(false);

	mHistoryAccordion->arrange();

	updateVerbs();
}

void LLTeleportHistoryPanel::handleItemSelect(LLFlatListView* selected)
{
	mLastSelectedScrollList = selected;

	S32 tabs_cnt = mItemContainers.size();

	for (S32 n = 0; n < tabs_cnt; n++)
	{
		LLAccordionCtrlTab* tab = mItemContainers.get(n);

		if (!tab->getVisible())
			continue;

		LLFlatListView *flv = getFlatListViewFromTab(tab);
		if (!flv)
			continue;

		if (flv == selected)
			continue;

		flv->resetSelection(true);
	}

	updateVerbs();
}

void LLTeleportHistoryPanel::onDoubleClickItem()
{
	// If item got doubleclick, then that item is already selected
	onTeleport();
}

void LLTeleportHistoryPanel::onAccordionTabRightClick(LLView *view, S32 x, S32 y, MASK mask)
{
	LLAccordionCtrlTab *tab = (LLAccordionCtrlTab *) view;

	// If click occurred below the header, don't show this menu
	if (y < tab->getRect().getHeight() - tab->getHeaderHeight() - tab->getPaddingBottom())
		return;

	if (mAccordionTabMenu)
	{
		//preventing parent (menu holder) from deleting already "dead" context menus on exit
		LLView* parent = mAccordionTabMenu->getParent();
		if (parent)
		{
			parent->removeChild(mAccordionTabMenu);
			mAccordionTabMenu->setParent(NULL);
		}
		delete mAccordionTabMenu;
	}

	// set up the callbacks for all of the avatar menu items
	// (N.B. callbacks don't take const refs as mID is local scope)
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;

	registrar.add("TeleportHistory.TabOpen",	boost::bind(&LLTeleportHistoryPanel::onAccordionTabOpen, this, tab));
	registrar.add("TeleportHistory.TabClose",	boost::bind(&LLTeleportHistoryPanel::onAccordionTabClose, this, tab));

	// create the context menu from the XUI
	mAccordionTabMenu = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>(
		"menu_teleport_history_tab.xml", LLMenuGL::sMenuContainer, LLViewerMenuHolderGL::child_registry_t::instance());

	mAccordionTabMenu->setItemVisible("TabOpen", !tab->isExpanded() ? true : false);
	mAccordionTabMenu->setItemVisible("TabClose", tab->isExpanded() ? true : false);

	mAccordionTabMenu->show(x, y);
	LLMenuGL::showPopup(tab, mAccordionTabMenu, x, y);
}

void LLTeleportHistoryPanel::onAccordionTabOpen(LLAccordionCtrlTab *tab)
{
	tab->setDisplayChildren(true);
	mHistoryAccordion->arrange();
}

void LLTeleportHistoryPanel::onAccordionTabClose(LLAccordionCtrlTab *tab)
{
	tab->setDisplayChildren(false);
	mHistoryAccordion->arrange();
}

LLFlatListView* LLTeleportHistoryPanel::getFlatListViewFromTab(LLAccordionCtrlTab *tab)
{
	for (child_list_const_iter_t iter = tab->beginChild(); iter != tab->endChild(); iter++)
	{
		if (dynamic_cast<LLFlatListView*>(*iter))
		{
			return (LLFlatListView*)*iter; // There should be one scroll list per tab.
		}
	}

	return NULL;
}


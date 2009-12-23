/** 
 * @file llpanelteleporthistory.cpp
 * @brief Teleport history represented by a scrolling list
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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
#include "lltextutil.h"

#include "llaccordionctrl.h"
#include "llaccordionctrltab.h"
#include "llflatlistview.h"
#include "llnotificationsutil.h"
#include "lltextbox.h"
#include "llviewermenu.h"
#include "llviewerinventory.h"
#include "lllandmarkactions.h"
#include "llclipboard.h"

// Maximum number of items that can be added to a list in one pass.
// Used to limit time spent for items list update per frame.
static const U32 ADD_LIMIT = 50;

static const std::string COLLAPSED_BY_USER = "collapsed_by_user";

class LLTeleportHistoryFlatItem : public LLPanel
{
public:
	LLTeleportHistoryFlatItem(S32 index, LLTeleportHistoryPanel::ContextMenu *context_menu, const std::string &region_name, const std::string &hl);
	virtual ~LLTeleportHistoryFlatItem();

	virtual BOOL postBuild();

	/*virtual*/ S32 notify(const LLSD& info);

	S32 getIndex() { return mIndex; }
	void setIndex(S32 index) { mIndex = index; }
	const std::string& getRegionName() { return mRegionName;}
	void setRegionName(const std::string& name);
	void setHighlightedText(const std::string& text);
	void updateTitle();

	/*virtual*/ void setValue(const LLSD& value);

	void onMouseEnter(S32 x, S32 y, MASK mask);
	void onMouseLeave(S32 x, S32 y, MASK mask);
	virtual BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

	static void showPlaceInfoPanel(S32 index);

	LLHandle<LLTeleportHistoryFlatItem> getItemHandle()	{ mItemHandle.bind(this); return mItemHandle; }

private:
	void onProfileBtnClick();

	LLButton* mProfileBtn;
	LLTextBox* mTitle;
	
	LLTeleportHistoryPanel::ContextMenu *mContextMenu;

	S32 mIndex;
	std::string mRegionName;
	std::string mHighlight;
	LLRootHandle<LLTeleportHistoryFlatItem> mItemHandle;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class LLTeleportHistoryFlatItemStorage: public LLSingleton<LLTeleportHistoryFlatItemStorage> {
protected:
	typedef std::vector< LLHandle<LLTeleportHistoryFlatItem> > flat_item_list_t;

public:
	LLTeleportHistoryFlatItem* getFlatItemForPersistentItem (
		LLTeleportHistoryPanel::ContextMenu *context_menu,
		const LLTeleportHistoryPersistentItem& persistent_item,
		const S32 cur_item_index,
		const std::string &hl);

	void removeItem(LLTeleportHistoryFlatItem* item);

	void purge();

private:

	flat_item_list_t mItems;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

LLTeleportHistoryFlatItem::LLTeleportHistoryFlatItem(S32 index, LLTeleportHistoryPanel::ContextMenu *context_menu, const std::string &region_name, const std::string &hl)
:	LLPanel(),
	mIndex(index),
	mContextMenu(context_menu),
	mRegionName(region_name),
	mHighlight(hl)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_teleport_history_item.xml");
}

LLTeleportHistoryFlatItem::~LLTeleportHistoryFlatItem()
{
}

//virtual
BOOL LLTeleportHistoryFlatItem::postBuild()
{
	mTitle = getChild<LLTextBox>("region");

	mProfileBtn = getChild<LLButton>("profile_btn");
        
	mProfileBtn->setClickedCallback(boost::bind(&LLTeleportHistoryFlatItem::onProfileBtnClick, this));

	updateTitle();

	return true;
}

S32 LLTeleportHistoryFlatItem::notify(const LLSD& info)
{
	if(info.has("detach"))
	{
		delete mMouseDownSignal;
		mMouseDownSignal = NULL;
		delete mRightMouseDownSignal;
		mRightMouseDownSignal = NULL;
		return 1;
	}
	return 0;
}

void LLTeleportHistoryFlatItem::setValue(const LLSD& value)
{
	if (!value.isMap()) return;;
	if (!value.has("selected")) return;
	childSetVisible("selected_icon", value["selected"]);
}

void LLTeleportHistoryFlatItem::setHighlightedText(const std::string& text)
{
	mHighlight = text;
}

void LLTeleportHistoryFlatItem::setRegionName(const std::string& name)
{
	mRegionName = name;
}

void LLTeleportHistoryFlatItem::updateTitle()
{
	LLTextUtil::textboxSetHighlightedVal(
		mTitle,
		LLStyle::Params(),
		mRegionName,
		mHighlight);
}

void LLTeleportHistoryFlatItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	childSetVisible("hovered_icon", true);
	mProfileBtn->setVisible(true);

	LLPanel::onMouseEnter(x, y, mask);
}

void LLTeleportHistoryFlatItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	childSetVisible("hovered_icon", false);
	mProfileBtn->setVisible(false);

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

void LLTeleportHistoryFlatItem::onProfileBtnClick()
{
	LLTeleportHistoryFlatItem::showPlaceInfoPanel(mIndex);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

LLTeleportHistoryFlatItem*
LLTeleportHistoryFlatItemStorage::getFlatItemForPersistentItem (
	LLTeleportHistoryPanel::ContextMenu *context_menu,
	const LLTeleportHistoryPersistentItem& persistent_item,
	const S32 cur_item_index,
	const std::string &hl)
{
	LLTeleportHistoryFlatItem* item = NULL;
	if ( cur_item_index < (S32) mItems.size() )
	{
		item = mItems[cur_item_index].get();
		if (item->getParent() == NULL)
		{
			item->setIndex(cur_item_index);
			item->setRegionName(persistent_item.mTitle);
			item->setHighlightedText(hl);
			item->setVisible(TRUE);
			item->updateTitle();
		}
		else
		{
			// Item already added to parent
			item = NULL;
		}
	}

	if ( !item )
	{
		item = new LLTeleportHistoryFlatItem(cur_item_index,
											 context_menu,
											 persistent_item.mTitle,
											 hl);
		mItems.push_back(item->getItemHandle());
	}

	return item;
}

void LLTeleportHistoryFlatItemStorage::removeItem(LLTeleportHistoryFlatItem* item)
{
	if (item)
	{
		flat_item_list_t::iterator item_iter = std::find(mItems.begin(),
														 mItems.end(),
														 item->getItemHandle());
		if (item_iter != mItems.end())
		{
			mItems.erase(item_iter);
		}
	}
}

void LLTeleportHistoryFlatItemStorage::purge()
{
	for ( flat_item_list_t::iterator
			  it = mItems.begin(),
			  it_end = mItems.end();
		  it != it_end; ++it )
	{
		LLHandle <LLTeleportHistoryFlatItem> item_handle = *it;
		if ( !item_handle.isDead() && item_handle.get()->getParent() == NULL )
		{
			item_handle.get()->die();
		}
	}
	mItems.clear();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

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
		}
		delete mMenu;
	}

	mIndex = index;
	mMenu = createMenu();

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
	registrar.add("TeleportHistory.CopyToClipboard",boost::bind(&LLTeleportHistoryPanel::ContextMenu::onCopyToClipboard, this));

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

void LLTeleportHistoryPanel::ContextMenu::onCopyToClipboard()
{
	LLVector3d globalPos = LLTeleportHistoryStorage::getInstance()->getItems()[mIndex].mGlobalPos;
	LLLandmarkActions::getSLURLfromPosGlobal(globalPos,
		boost::bind(&LLTeleportHistoryPanel::ContextMenu::gotSLURLCallback, _1));
}

// Not yet implemented; need to remove buildPanel() from constructor when we switch
//static LLRegisterPanelClassWrapper<LLTeleportHistoryPanel> t_teleport_history("panel_teleport_history");

LLTeleportHistoryPanel::LLTeleportHistoryPanel()
	:	LLPanelPlacesTab(),
		mDirty(true),
		mCurrentItem(0),
		mTeleportHistory(NULL),
		mHistoryAccordion(NULL),
		mAccordionTabMenu(NULL),
		mLastSelectedFlatlList(NULL),
		mLastSelectedItemIndex(-1)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_teleport_history.xml");
}

LLTeleportHistoryPanel::~LLTeleportHistoryPanel()
{
	LLTeleportHistoryFlatItemStorage::instance().purge();
	LLView::deleteViewByHandle(mGearMenuHandle);
}

BOOL LLTeleportHistoryPanel::postBuild()
{
	mTeleportHistory = LLTeleportHistoryStorage::getInstance();
	if (mTeleportHistory)
	{
		mTeleportHistory->setHistoryChangedCallback(boost::bind(&LLTeleportHistoryPanel::onTeleportHistoryChange, this, _1));
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
				tab->setDisplayChildren(false);
				tab->setDropDownStateChangedCallback(boost::bind(&LLTeleportHistoryPanel::onAccordionExpand, this, _1, _2));

				// All accordion tabs are collapsed initially
				setAccordionCollapsedByUser(tab, true);

				mItemContainers.put(tab);

				LLFlatListView* fl = getFlatListViewFromTab(tab);
				if (fl)
				{
					fl->setCommitOnSelectionChange(true);
					fl->setDoubleClickCallback(boost::bind(&LLTeleportHistoryPanel::onDoubleClickItem, this));
					fl->setCommitCallback(boost::bind(&LLTeleportHistoryPanel::handleItemSelect, this, fl));
					fl->setReturnCallback(boost::bind(&LLTeleportHistoryPanel::onReturnKeyPressed, this));
				}
			}
		}

		// Open first 2 accordion tabs
		if (mItemContainers.size() > 1)
		{
			LLAccordionCtrlTab* tab = mItemContainers.get(mItemContainers.size() - 1);
			tab->setDisplayChildren(true);
			setAccordionCollapsedByUser(tab, false);
		}

		if (mItemContainers.size() > 2)
		{
			LLAccordionCtrlTab* tab = mItemContainers.get(mItemContainers.size() - 2);
			tab->setDisplayChildren(true);
			setAccordionCollapsedByUser(tab, false);
		}
	}

	getChild<LLPanel>("bottom_panel")->childSetAction("gear_btn",boost::bind(&LLTeleportHistoryPanel::onGearButtonClicked, this));

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;

	registrar.add("TeleportHistory.ExpandAllFolders",  boost::bind(&LLTeleportHistoryPanel::onExpandAllFolders,  this));
	registrar.add("TeleportHistory.CollapseAllFolders",  boost::bind(&LLTeleportHistoryPanel::onCollapseAllFolders,  this));
	registrar.add("TeleportHistory.ClearTeleportHistory",  boost::bind(&LLTeleportHistoryPanel::onClearTeleportHistory,  this));

	LLMenuGL* gear_menu  = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_teleport_history_gear.xml",  gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if(gear_menu)
		mGearMenuHandle  = gear_menu->getHandle();

	return TRUE;
}

// virtual
void LLTeleportHistoryPanel::draw()
{
	if (mDirty)
		refresh();

	LLPanelPlacesTab::draw();
}

// virtual
void LLTeleportHistoryPanel::onSearchEdit(const std::string& string)
{
	if (sFilterSubString != string)
	{
		sFilterSubString = string;
		showTeleportHistory();
	}
}

// virtual
void LLTeleportHistoryPanel::onShowOnMap()
{
	if (!mLastSelectedFlatlList)
		return;

	LLTeleportHistoryFlatItem* itemp = dynamic_cast<LLTeleportHistoryFlatItem *> (mLastSelectedFlatlList->getSelectedItem());

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
	if (!mLastSelectedFlatlList)
		return;

	LLTeleportHistoryFlatItem* itemp = dynamic_cast<LLTeleportHistoryFlatItem *> (mLastSelectedFlatlList->getSelectedItem());
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

	LLWorldMapMessage::url_callback_t cb = boost::bind(
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

	if (!mLastSelectedFlatlList)
	{
		mTeleportBtn->setEnabled(false);
		mShowOnMapBtn->setEnabled(false);
		return;
	}

	LLTeleportHistoryFlatItem* itemp = dynamic_cast<LLTeleportHistoryFlatItem *> (mLastSelectedFlatlList->getSelectedItem());

	mTeleportBtn->setEnabled(NULL != itemp && itemp->getIndex() < (S32)mTeleportHistory->getItems().size() - 1);
	mShowOnMapBtn->setEnabled(NULL != itemp);
}

void LLTeleportHistoryPanel::getNextTab(const LLDate& item_date, S32& tab_idx, LLDate& tab_date)
{
	const U32 seconds_in_day = 24 * 60 * 60;

	S32 tabs_cnt = mItemContainers.size();
	S32 curr_year = 0, curr_month = 0, curr_day = 0;

	tab_date = LLDate::now();
	tab_date.split(&curr_year, &curr_month, &curr_day);
	tab_date.fromYMDHMS(curr_year, curr_month, curr_day); // Set hour, min, and sec to 0
	tab_date.secondsSinceEpoch(tab_date.secondsSinceEpoch() + seconds_in_day);

	tab_idx = -1;

	while (tab_idx < tabs_cnt - 1 && item_date < tab_date)
	{
		tab_idx++;

		if (tab_idx <= tabs_cnt - 4)
		{
			// All tabs, except last three, are tabs for one day, so just push tab_date back by one day
			tab_date.secondsSinceEpoch(tab_date.secondsSinceEpoch() - seconds_in_day);
		}
		else if (tab_idx == tabs_cnt - 3) // 6 day and older, low boundary is 1 month
		{
			tab_date =  LLDate::now();
			tab_date.split(&curr_year, &curr_month, &curr_day);
			curr_month--;
			if (0 == curr_month)
			{
				curr_month = 12;
				curr_year--;
			}
			tab_date.fromYMDHMS(curr_year, curr_month, curr_day);
		}
		else if (tab_idx == tabs_cnt - 2) // 1 month and older, low boundary is 6 months
		{
			tab_date =  LLDate::now();
			tab_date.split(&curr_year, &curr_month, &curr_day);
			if (curr_month > 6)
			{
				curr_month -= 6;
			}
			else
			{
				curr_month += 6;
				curr_year--;
			}
			tab_date.fromYMDHMS(curr_year, curr_month, curr_day);
		}
		else // 6 months and older
		{
			tab_date.secondsSinceEpoch(0);
		}
	}
}

// Called to add items, no more, than ADD_LIMIT at time
void LLTeleportHistoryPanel::refresh()
{
	if (!mHistoryAccordion)
	{
		mDirty = false;
		return;
	}

	const LLTeleportHistoryStorage::slurl_list_t& items = mTeleportHistory->getItems();

	// Setting tab_boundary_date to "now", so date from any item would be earlier, than boundary.
	// That leads to call to getNextTab to get right tab_idx in first pass
	LLDate tab_boundary_date =  LLDate::now();

	LLFlatListView* curr_flat_view = NULL;

	U32 added_items = 0;
	while (mCurrentItem >= 0)
	{
		// Filtering
		if (!sFilterSubString.empty())
		{
			std::string landmark_title(items[mCurrentItem].mTitle);
			LLStringUtil::toUpper(landmark_title);
			if( std::string::npos == landmark_title.find(sFilterSubString) )
			{
				mCurrentItem--;
				continue;
			}
		}

		// Checking whether date of item is earlier, than tab_boundary_date.
		// In that case, item should be added to another tab
		const LLDate &date = items[mCurrentItem].mDate;

		if (date < tab_boundary_date)
		{
			// Getting apropriate tab_idx for this and subsequent items,
			// tab_boundary_date would be earliest possible date for this tab
			S32 tab_idx = 0;
			getNextTab(date, tab_idx, tab_boundary_date);

			LLAccordionCtrlTab* tab = mItemContainers.get(mItemContainers.size() - 1 - tab_idx);
			tab->setVisible(true);

			// Expand all accordion tabs when filtering
			if(!sFilterSubString.empty())
			{
				tab->setDisplayChildren(true);
			}
			// Restore each tab's expand state when not filtering
			else
			{
				bool collapsed = isAccordionCollapsedByUser(tab);
				tab->setDisplayChildren(!collapsed);
			}

			curr_flat_view = getFlatListViewFromTab(tab);
		}

		if (curr_flat_view)
		{
			LLTeleportHistoryFlatItem* item =
				LLTeleportHistoryFlatItemStorage::instance()
				.getFlatItemForPersistentItem(&mContextMenu,
											  items[mCurrentItem],
											  mCurrentItem,
											  sFilterSubString);
			if ( !curr_flat_view->addItem(item, LLUUID::null, ADD_BOTTOM, false) )
				llerrs << "Couldn't add flat item to teleport history." << llendl;
			if (mLastSelectedItemIndex == mCurrentItem)
				curr_flat_view->selectItem(item, true);
		}

		mCurrentItem--;

		if (++added_items >= ADD_LIMIT)
			break;
	}

	for (S32 n = mItemContainers.size() - 1; n >= 0; --n)
	{
		LLAccordionCtrlTab* tab = mItemContainers.get(n);
		LLFlatListView* fv = getFlatListViewFromTab(tab);
		if (fv)
		{
			fv->notify(LLSD().with("rearrange", LLSD()));
		}
	}

	mHistoryAccordion->arrange();

	updateVerbs();

	if (mCurrentItem < 0)
		mDirty = false;
}

void LLTeleportHistoryPanel::onTeleportHistoryChange(S32 removed_index)
{
	mLastSelectedItemIndex = -1;

	if (-1 == removed_index)
		showTeleportHistory(); // recreate all items
	else
		replaceItem(removed_index); // replace removed item by most recent
}

void LLTeleportHistoryPanel::replaceItem(S32 removed_index)
{
	// Flat list for 'Today' (mItemContainers keeps accordion tabs in reverse order)
	LLFlatListView* fv = getFlatListViewFromTab(mItemContainers[mItemContainers.size() - 1]);

	// Empty flat list for 'Today' means that other flat lists are empty as well,
	// so all items from teleport history should be added.
	if (!fv || fv->size() == 0)
	{
		showTeleportHistory();
		return;
	}

	const LLTeleportHistoryStorage::slurl_list_t& history_items = mTeleportHistory->getItems();
	LLTeleportHistoryFlatItem* item = LLTeleportHistoryFlatItemStorage::instance()
		.getFlatItemForPersistentItem(&mContextMenu,
									  history_items[history_items.size() - 1], // Most recent item, it was added instead of removed
									  history_items.size(), // index will be decremented inside loop below
									  sFilterSubString);

	fv->addItem(item, LLUUID::null, ADD_TOP);

	// Index of each item, from last to removed item should be decremented
	// to point to the right item in LLTeleportHistoryStorage
	for (S32 tab_idx = mItemContainers.size() - 1; tab_idx >= 0; --tab_idx)
	{
		LLAccordionCtrlTab* tab = mItemContainers.get(tab_idx);
		if (!tab->getVisible())
			continue;

		fv = getFlatListViewFromTab(tab);
		if (!fv)
		{
			showTeleportHistory();
			return;
		}

		std::vector<LLPanel*> items;
		fv->getItems(items);

		S32 items_cnt = items.size();
		for (S32 n = 0; n < items_cnt; ++n)
		{
			LLTeleportHistoryFlatItem *item = (LLTeleportHistoryFlatItem*) items[n];

			if (item->getIndex() == removed_index)
			{
				LLTeleportHistoryFlatItemStorage::instance().removeItem(item);

				fv->removeItem(item);

				// If flat list becames empty, then accordion tab should be hidden
				if (fv->size() == 0)
					tab->setVisible(false);

				mHistoryAccordion->arrange();

				return; // No need to decrement idexes for the rest of items
			}

			item->setIndex(item->getIndex() - 1);
		}
	}
}

void LLTeleportHistoryPanel::showTeleportHistory()
{
	mDirty = true;

	// Starting to add items from last one, in reverse order,
	// since TeleportHistory keeps most recent item at the end
	mCurrentItem = mTeleportHistory->getItems().size() - 1;

	for (S32 n = mItemContainers.size() - 1; n >= 0; --n)
	{
		LLAccordionCtrlTab* tab = mItemContainers.get(n);
		tab->setVisible(false);

		LLFlatListView* fv = getFlatListViewFromTab(tab);
		if (fv)
		{
			// Detached panels are managed by LLTeleportHistoryFlatItemStorage
			std::vector<LLPanel*> detached_items;
			fv->detachItems(detached_items);
		}
	}
}

void LLTeleportHistoryPanel::handleItemSelect(LLFlatListView* selected)
{
	mLastSelectedFlatlList = selected;
	LLTeleportHistoryFlatItem* item = dynamic_cast<LLTeleportHistoryFlatItem *> (mLastSelectedFlatlList->getSelectedItem());
	if (item)
		mLastSelectedItemIndex = item->getIndex();

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

void LLTeleportHistoryPanel::onReturnKeyPressed()
{
	// Teleport to selected region as default action on return key pressed
	onTeleport();
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

void LLTeleportHistoryPanel::onExpandAllFolders()
{
	S32 tabs_cnt = mItemContainers.size();

	for (S32 n = 0; n < tabs_cnt; n++)
	{
		mItemContainers.get(n)->setDisplayChildren(true);
	}
	mHistoryAccordion->arrange();
}

void LLTeleportHistoryPanel::onCollapseAllFolders()
{
	S32 tabs_cnt = mItemContainers.size();

	for (S32 n = 0; n < tabs_cnt; n++)
	{
		mItemContainers.get(n)->setDisplayChildren(false);
	}
	mHistoryAccordion->arrange();
}

void LLTeleportHistoryPanel::onClearTeleportHistory()
{
	LLNotificationsUtil::add("ConfirmClearTeleportHistory", LLSD(), LLSD(), boost::bind(&LLTeleportHistoryPanel::onClearTeleportHistoryDialog, this, _1, _2));
}

bool LLTeleportHistoryPanel::onClearTeleportHistoryDialog(const LLSD& notification, const LLSD& response)
{

	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	if (0 == option)
	{
		LLTeleportHistoryStorage *th = LLTeleportHistoryStorage::getInstance();
		th->purgeItems();
		th->save();
	}

	return false;
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

void LLTeleportHistoryPanel::onGearButtonClicked()
{
	LLMenuGL* menu = (LLMenuGL*)mGearMenuHandle.get();
	if (!menu)
		return;

	// Shows the menu at the top of the button bar.

	// Calculate its coordinates.
	LLPanel* bottom_panel = getChild<LLPanel>("bottom_panel");
	menu->arrangeAndClear();
	S32 menu_height = menu->getRect().getHeight();
	S32 menu_x = -2; // *HACK: compensates HPAD in showPopup()
	S32 menu_y = bottom_panel->getRect().mTop + menu_height;

	// Actually show the menu.
	menu->buildDrawLabels();
	menu->updateParent(LLMenuGL::sMenuContainer);
	LLMenuGL::showPopup(this, menu, menu_x, menu_y);
}

void LLTeleportHistoryPanel::setAccordionCollapsedByUser(LLUICtrl* acc_tab, bool collapsed)
{
	LLSD param = acc_tab->getValue();
	param[COLLAPSED_BY_USER] = collapsed;
	acc_tab->setValue(param);
}

bool LLTeleportHistoryPanel::isAccordionCollapsedByUser(LLUICtrl* acc_tab)
{
	LLSD param = acc_tab->getValue();
	if(!param.has("acc_collapsed"))
	{
		return false;
	}
	return param[COLLAPSED_BY_USER].asBoolean();
}

void LLTeleportHistoryPanel::onAccordionExpand(LLUICtrl* ctrl, const LLSD& param)
{
	bool expanded = param.asBoolean();
	// Save accordion tab state to restore it in refresh()
	setAccordionCollapsedByUser(ctrl, !expanded);
}

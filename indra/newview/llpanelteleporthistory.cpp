/** 
 * @file llpanelteleporthistory.cpp
 * @brief Teleport history represented by a scrolling list
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

#include "llfloaterreg.h"
#include "llmenubutton.h"

#include "llfloaterworldmap.h"
#include "llpanelteleporthistory.h"
#include "llworldmap.h"
#include "llteleporthistorystorage.h"
#include "lltextutil.h"

#include "llaccordionctrl.h"
#include "llaccordionctrltab.h"
#include "llflatlistview.h"
#include "llfloatersidepanelcontainer.h"
#include "llnotificationsutil.h"
#include "lltextbox.h"
#include "lltoggleablemenu.h"
#include "llviewermenu.h"
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
	buildFromFile( "panel_teleport_history_item.xml");
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
	getChildView("selected_icon")->setVisible( value["selected"]);
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
	static LLUIColor sFgColor = LLUIColorTable::instance().getColor("MenuItemEnabledColor", LLColor4U(255, 255, 255));

	LLTextUtil::textboxSetHighlightedVal(
		mTitle,
		LLStyle::Params().color(sFgColor),
		mRegionName,
		mHighlight);
}

void LLTeleportHistoryFlatItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	getChildView("hovered_icon")->setVisible( true);
	mProfileBtn->setVisible(true);

	LLPanel::onMouseEnter(x, y, mask);
}

void LLTeleportHistoryFlatItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	getChildView("hovered_icon")->setVisible( false);
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

	LLFloaterSidePanelContainer::showPanel("places", params);
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
	mMenu(NULL), mIndex(0)
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
	confirmTeleport(mIndex);
}

void LLTeleportHistoryPanel::ContextMenu::onInfo()
{
	LLTeleportHistoryFlatItem::showPlaceInfoPanel(mIndex);
}

//static
void LLTeleportHistoryPanel::ContextMenu::gotSLURLCallback(const std::string& slurl)
{
	LLClipboard::instance().copyToClipboard(utf8str_to_wstring(slurl),0,slurl.size());
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
		mLastSelectedItemIndex(-1),
		mMenuGearButton(NULL)
{
	buildFromFile( "panel_teleport_history.xml");
}

LLTeleportHistoryPanel::~LLTeleportHistoryPanel()
{
	LLTeleportHistoryFlatItemStorage::instance().purge();
	if (mGearMenuHandle.get()) mGearMenuHandle.get()->die();
	mTeleportHistoryChangedConnection.disconnect();
}

BOOL LLTeleportHistoryPanel::postBuild()
{
	mTeleportHistory = LLTeleportHistoryStorage::getInstance();
	if (mTeleportHistory)
	{
		mTeleportHistoryChangedConnection = mTeleportHistory->setHistoryChangedCallback(boost::bind(&LLTeleportHistoryPanel::onTeleportHistoryChange, this, _1));
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

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;

	registrar.add("TeleportHistory.ExpandAllFolders",  boost::bind(&LLTeleportHistoryPanel::onExpandAllFolders,  this));
	registrar.add("TeleportHistory.CollapseAllFolders",  boost::bind(&LLTeleportHistoryPanel::onCollapseAllFolders,  this));
	registrar.add("TeleportHistory.ClearTeleportHistory",  boost::bind(&LLTeleportHistoryPanel::onClearTeleportHistory,  this));
	mEnableCallbackRegistrar.add("TeleportHistory.GearMenu.Enable", boost::bind(&LLTeleportHistoryPanel::isActionEnabled, this, _2));

	mMenuGearButton = getChild<LLMenuButton>("gear_btn");

	LLToggleableMenu* gear_menu  = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_teleport_history_gear.xml",  gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());;
	if(gear_menu)
	{
		mGearMenuHandle  = gear_menu->getHandle();
		mMenuGearButton->setMenu(gear_menu);
	}

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
	sFilterSubString = string;
	showTeleportHistory();
}

// virtual
bool LLTeleportHistoryPanel::isSingleItemSelected()
{
	return mLastSelectedFlatlList && mLastSelectedFlatlList->getSelectedItem();
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

//virtual
void LLTeleportHistoryPanel::onShowProfile()
{
	if (!mLastSelectedFlatlList)
		return;

	LLTeleportHistoryFlatItem* itemp = dynamic_cast<LLTeleportHistoryFlatItem *> (mLastSelectedFlatlList->getSelectedItem());

	if(!itemp)
		return;

	LLTeleportHistoryFlatItem::showPlaceInfoPanel(itemp->getIndex());
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
	confirmTeleport(itemp->getIndex());
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
		mShowProfile->setEnabled(false);
		mShowOnMapBtn->setEnabled(false);
		return;
	}

	LLTeleportHistoryFlatItem* itemp = dynamic_cast<LLTeleportHistoryFlatItem *> (mLastSelectedFlatlList->getSelectedItem());

	mTeleportBtn->setEnabled(NULL != itemp);
	mShowProfile->setEnabled(NULL != itemp);
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
	std::string filter_string = sFilterSubString;
	LLStringUtil::toUpper(filter_string);

	U32 added_items = 0;
	while (mCurrentItem >= 0)
	{
		// Filtering
		if (!filter_string.empty())
		{
			std::string landmark_title(items[mCurrentItem].mTitle);
			LLStringUtil::toUpper(landmark_title);
			if( std::string::npos == landmark_title.find(filter_string) )
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
			tab_idx = mItemContainers.size() - 1 - tab_idx;
			if (tab_idx >= 0)
			{
				LLAccordionCtrlTab* tab = mItemContainers.get(tab_idx);
				tab->setVisible(true);

				// Expand all accordion tabs when filtering
				if(!sFilterSubString.empty())
				{
					//store accordion tab state when filter is not empty
					tab->notifyChildren(LLSD().with("action","store_state"));
				
					tab->setDisplayChildren(true);
				}
				// Restore each tab's expand state when not filtering
				else
				{
					bool collapsed = isAccordionCollapsedByUser(tab);
					tab->setDisplayChildren(!collapsed);
			
					//restore accordion state after all those accodrion tabmanipulations
					tab->notifyChildren(LLSD().with("action","restore_state"));
				}

				curr_flat_view = getFlatListViewFromTab(tab);
			}
		}

		if (curr_flat_view)
		{
			LLTeleportHistoryFlatItem* item =
				LLTeleportHistoryFlatItemStorage::instance()
				.getFlatItemForPersistentItem(&mContextMenu,
											  items[mCurrentItem],
											  mCurrentItem,
											  filter_string);
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

	mHistoryAccordion->setFilterSubString(sFilterSubString);

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
	{
		replaceItem(removed_index); // replace removed item by most recent
		updateVerbs();
	}
}

void LLTeleportHistoryPanel::replaceItem(S32 removed_index)
{
	// Flat list for 'Today' (mItemContainers keeps accordion tabs in reverse order)
	LLFlatListView* fv = NULL;
	
	if (mItemContainers.size() > 0)
	{
		fv = getFlatListViewFromTab(mItemContainers[mItemContainers.size() - 1]);
	}

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
	if (!mTeleportHistory)
	{
		mTeleportHistory = LLTeleportHistoryStorage::getInstance();
	}

	mCurrentItem = mTeleportHistory->getItems().size() - 1;

	for (S32 n = mItemContainers.size() - 1; n >= 0; --n)
	{
		LLAccordionCtrlTab* tab = mItemContainers.get(n);
		if (tab)
		{
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
		// order does matter, call this first or teleport history will contain one record(current location)
		LLTeleportHistory::getInstance()->purgeItems();

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

bool LLTeleportHistoryPanel::isActionEnabled(const LLSD& userdata) const
{
	S32 tabs_cnt = mItemContainers.size();

	bool has_expanded_tabs = false;
	bool has_collapsed_tabs = false;

	for (S32 n = 0; n < tabs_cnt; n++)
	{
		LLAccordionCtrlTab* tab = mItemContainers.get(n);
		if (!tab->getVisible())
			continue;

		if (tab->getDisplayChildren())
		{
			has_expanded_tabs = true;
		}
		else
		{
			has_collapsed_tabs = true;
		}

		if (has_expanded_tabs && has_collapsed_tabs)
		{
			break;
		}
	}

	std::string command_name = userdata.asString();

	if (has_expanded_tabs && command_name == "collapse_all")
	{
		return true;
	}

	if (has_collapsed_tabs && command_name ==  "expand_all")
	{
		return true;
	}

	return false;
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
	if(!param.has(COLLAPSED_BY_USER))
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

	// Reset selection upon accordion being collapsed
	// to disable "Teleport" and "Map" buttons for hidden item.
	if (!expanded && mLastSelectedFlatlList)
	{
		mLastSelectedFlatlList->resetSelection();
	}
}

// static
void LLTeleportHistoryPanel::confirmTeleport(S32 hist_idx)
{
	LLSD args;
	args["HISTORY_ENTRY"] = LLTeleportHistoryStorage::getInstance()->getItems()[hist_idx].mTitle;
	LLNotificationsUtil::add("TeleportToHistoryEntry", args, LLSD(),
		boost::bind(&LLTeleportHistoryPanel::onTeleportConfirmation, _1, _2, hist_idx));
}

// Called when user reacts upon teleport confirmation dialog.
// static
bool LLTeleportHistoryPanel::onTeleportConfirmation(const LLSD& notification, const LLSD& response, S32 hist_idx)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	if (0 == option)
	{
		// Teleport to given history item.
		LLTeleportHistoryStorage::getInstance()->goToItem(hist_idx);
	}

	return false;
}

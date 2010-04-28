/** 
 * @file llavatarlist.h
 * @brief Generic avatar list
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

// common
#include "lltrans.h"
#include "llcommonutils.h"

#include "llavatarlist.h"
#include "llagentdata.h" // for comparator

// newview
#include "llavatariconctrl.h"
#include "llcallingcard.h" // for LLAvatarTracker
#include "llcachename.h"
#include "llrecentpeople.h"
#include "lltextutil.h"
#include "lluuid.h"
#include "llvoiceclient.h"
#include "llviewercontrol.h"	// for gSavedSettings

static LLDefaultChildRegistry::Register<LLAvatarList> r("avatar_list");

// Last interaction time update period.
static const F32 LIT_UPDATE_PERIOD = 5;

// Maximum number of avatars that can be added to a list in one pass.
// Used to limit time spent for avatar list update per frame.
static const unsigned ADD_LIMIT = 50;

bool LLAvatarList::contains(const LLUUID& id)
{
	const uuid_vec_t& ids = getIDs();
	return std::find(ids.begin(), ids.end(), id) != ids.end();
}

void LLAvatarList::toggleIcons()
{
	// Save the new value for new items to use.
	mShowIcons = !mShowIcons;
	gSavedSettings.setBOOL(mIconParamName, mShowIcons);
	
	// Show/hide icons for all existing items.
	std::vector<LLPanel*> items;
	getItems(items);
	for( std::vector<LLPanel*>::const_iterator it = items.begin(); it != items.end(); it++)
	{
		static_cast<LLAvatarListItem*>(*it)->setAvatarIconVisible(mShowIcons);
	}
}

void LLAvatarList::setSpeakingIndicatorsVisible(bool visible)
{
	// Save the new value for new items to use.
	mShowSpeakingIndicator = visible;
	
	// Show/hide icons for all existing items.
	std::vector<LLPanel*> items;
	getItems(items);
	for( std::vector<LLPanel*>::const_iterator it = items.begin(); it != items.end(); it++)
	{
		static_cast<LLAvatarListItem*>(*it)->showSpeakingIndicator(mShowSpeakingIndicator);
	}
}

static bool findInsensitive(std::string haystack, const std::string& needle_upper)
{
    LLStringUtil::toUpper(haystack);
    return haystack.find(needle_upper) != std::string::npos;
}


//comparators
static const LLAvatarItemNameComparator NAME_COMPARATOR;
static const LLFlatListView::ItemReverseComparator REVERSE_NAME_COMPARATOR(NAME_COMPARATOR);

LLAvatarList::Params::Params()
: ignore_online_status("ignore_online_status", false)
, show_last_interaction_time("show_last_interaction_time", false)
, show_info_btn("show_info_btn", true)
, show_profile_btn("show_profile_btn", true)
, show_speaking_indicator("show_speaking_indicator", true)
{
}

LLAvatarList::LLAvatarList(const Params& p)
:	LLFlatListViewEx(p)
, mIgnoreOnlineStatus(p.ignore_online_status)
, mShowLastInteractionTime(p.show_last_interaction_time)
, mContextMenu(NULL)
, mDirty(true) // to force initial update
, mLITUpdateTimer(NULL)
, mShowIcons(true)
, mShowInfoBtn(p.show_info_btn)
, mShowProfileBtn(p.show_profile_btn)
, mShowSpeakingIndicator(p.show_speaking_indicator)
{
	setCommitOnSelectionChange(true);

	// Set default sort order.
	setComparator(&NAME_COMPARATOR);

	if (mShowLastInteractionTime)
	{
		mLITUpdateTimer = new LLTimer();
		mLITUpdateTimer->setTimerExpirySec(0); // zero to force initial update
		mLITUpdateTimer->start();
	}
}

LLAvatarList::~LLAvatarList()
{
	delete mLITUpdateTimer;
}

void LLAvatarList::setShowIcons(std::string param_name)
{
	mIconParamName= param_name;
	mShowIcons = gSavedSettings.getBOOL(mIconParamName);
}

// virtual
void LLAvatarList::draw()
{
	// *NOTE dzaporozhan
	// Call refresh() after draw() to avoid flickering of avatar list items.

	LLFlatListViewEx::draw();

	if (mDirty)
		refresh();

	if (mShowLastInteractionTime && mLITUpdateTimer->hasExpired())
	{
		updateLastInteractionTimes();
		mLITUpdateTimer->setTimerExpirySec(LIT_UPDATE_PERIOD); // restart the timer
	}
}

//virtual
void LLAvatarList::clear()
{
	getIDs().clear();
	setDirty(true);
	LLFlatListViewEx::clear();
}

void LLAvatarList::setNameFilter(const std::string& filter)
{
	std::string filter_upper = filter;
	LLStringUtil::toUpper(filter_upper);
	if (mNameFilter != filter_upper)
	{
		mNameFilter = filter_upper;

		// update message for empty state here instead of refresh() to avoid blinking when switch
		// between tabs.
		updateNoItemsMessage(filter);
		setDirty();
	}
}

void LLAvatarList::sortByName()
{
	setComparator(&NAME_COMPARATOR);
	sort();
}

void LLAvatarList::setDirty(bool val /*= true*/, bool force_refresh /*= false*/)
{
	mDirty = val;
	if(mDirty && force_refresh)
	{
		refresh();
	}
}

void LLAvatarList::addAvalineItem(const LLUUID& item_id, const LLUUID& session_id, const std::string& item_name)
{
	LL_DEBUGS("Avaline") << "Adding avaline item into the list: " << item_name << "|" << item_id << ", session: " << session_id << LL_ENDL;
	LLAvalineListItem* item = new LLAvalineListItem;
	item->setAvatarId(item_id, session_id, true, false);
	item->setName(item_name);

	addItem(item, item_id);
	mIDs.push_back(item_id);
	sort();
}

//////////////////////////////////////////////////////////////////////////
// PROTECTED SECTION
//////////////////////////////////////////////////////////////////////////

void LLAvatarList::refresh()
{
	bool have_names			= TRUE;
	bool add_limit_exceeded	= false;
	bool modified			= false;
	bool have_filter		= !mNameFilter.empty();

	// Save selection.	
	uuid_vec_t selected_ids;
	getSelectedUUIDs(selected_ids);
	LLUUID current_id = getSelectedUUID();

	// Determine what to add and what to remove.
	uuid_vec_t added, removed;
	LLAvatarList::computeDifference(getIDs(), added, removed);

	// Handle added items.
	unsigned nadded = 0;
	for (uuid_vec_t::const_iterator it=added.begin(); it != added.end(); it++)
	{
		std::string name;
		const LLUUID& buddy_id = *it;
		have_names &= (bool)gCacheName->getFullName(buddy_id, name);
		if (!have_filter || findInsensitive(name, mNameFilter))
		{
			if (nadded >= ADD_LIMIT)
			{
				add_limit_exceeded = true;
				break;
			}
			else
			{
				addNewItem(buddy_id, name, LLAvatarTracker::instance().isBuddyOnline(buddy_id));
				modified = true;
				nadded++;
			}
		}
	}

	// Handle removed items.
	for (uuid_vec_t::const_iterator it=removed.begin(); it != removed.end(); it++)
	{
		removeItemByUUID(*it);
		modified = true;
	}

	// Handle filter.
	if (have_filter)
	{
		std::vector<LLSD> cur_values;
		getValues(cur_values);

		for (std::vector<LLSD>::const_iterator it=cur_values.begin(); it != cur_values.end(); it++)
		{
			std::string name;
			const LLUUID& buddy_id = it->asUUID();
			have_names &= (bool)gCacheName->getFullName(buddy_id, name);
			if (!findInsensitive(name, mNameFilter))
			{
				removeItemByUUID(buddy_id);
				modified = true;
			}
		}
	}

	// Changed item in place, need to request sort and update columns
	// because we might have changed data in a column on which the user
	// has already sorted. JC
	sort();

	// re-select items
	//	selectMultiple(selected_ids); // TODO: implement in LLFlatListView if need
	selectItemByUUID(current_id);

	// If the name filter is specified and the names are incomplete,
	// we need to re-update when the names are complete so that
	// the filter can be applied correctly.
	//
	// Otherwise, if we have no filter then no need to update again
	// because the items will update their names.
	bool dirty = add_limit_exceeded || (have_filter && !have_names);
	setDirty(dirty);

	// Refreshed all items.
	if(!dirty)
	{
		// Highlight items matching the filter.
		std::vector<LLPanel*> items;
		getItems(items);
		for( std::vector<LLPanel*>::const_iterator it = items.begin(); it != items.end(); it++)
		{
			static_cast<LLAvatarListItem*>(*it)->setHighlight(mNameFilter);
		}

		// Send refresh_complete signal.
		std::vector<LLSD> cur_values;
		getValues(cur_values);
		mRefreshCompleteSignal(this, LLSD((S32)cur_values.size()));
	}

	// Commit if we've added/removed items.
	if (modified)
		onCommit();
}

bool LLAvatarList::filterHasMatches()
{
	uuid_vec_t values = getIDs();

	for (uuid_vec_t::const_iterator it=values.begin(); it != values.end(); it++)
	{
		std::string name;
		const LLUUID& buddy_id = *it;
		BOOL have_name = gCacheName->getFullName(buddy_id, name);

		// If name has not been loaded yet we consider it as a match.
		// When the name will be loaded the filter will be applied again(in refresh()).

		if (have_name && !findInsensitive(name, mNameFilter))
		{
			continue;
		}

		return true;
	}
	return false;
}

boost::signals2::connection LLAvatarList::setRefreshCompleteCallback(const commit_signal_t::slot_type& cb)
{
	return mRefreshCompleteSignal.connect(cb);
}

boost::signals2::connection LLAvatarList::setItemDoubleClickCallback(const mouse_signal_t::slot_type& cb)
{
	return mItemDoubleClickSignal.connect(cb);
}

//virtual
S32 LLAvatarList::notifyParent(const LLSD& info)
{
	if (info.has("sort") && &NAME_COMPARATOR == mItemComparator)
	{
		sort();
		return 1;
	}
	return LLFlatListViewEx::notifyParent(info);
}

void LLAvatarList::addNewItem(const LLUUID& id, const std::string& name, BOOL is_online, EAddPosition pos)
{
	LLAvatarListItem* item = new LLAvatarListItem();
	item->setName(name);
	item->setAvatarId(id, mSessionID, mIgnoreOnlineStatus);
	item->setOnline(mIgnoreOnlineStatus ? true : is_online);
	item->showLastInteractionTime(mShowLastInteractionTime);

	item->setAvatarIconVisible(mShowIcons);
	item->setShowInfoBtn(mShowInfoBtn);
	item->setShowProfileBtn(mShowProfileBtn);
	item->showSpeakingIndicator(mShowSpeakingIndicator);

	item->setDoubleClickCallback(boost::bind(&LLAvatarList::onItemDoucleClicked, this, _1, _2, _3, _4));

	addItem(item, id, pos);
}

// virtual
BOOL LLAvatarList::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLUICtrl::handleRightMouseDown(x, y, mask);
	if ( mContextMenu )
	{
		uuid_vec_t selected_uuids;
		getSelectedUUIDs(selected_uuids);
		mContextMenu->show(this, selected_uuids, x, y);
	}
	return handled;
}

void LLAvatarList::computeDifference(
	const uuid_vec_t& vnew_unsorted,
	uuid_vec_t& vadded,
	uuid_vec_t& vremoved)
{
	uuid_vec_t vcur;

	// Convert LLSDs to LLUUIDs.
	{
		std::vector<LLSD> vcur_values;
		getValues(vcur_values);

		for (size_t i=0; i<vcur_values.size(); i++)
			vcur.push_back(vcur_values[i].asUUID());
	}

	LLCommonUtils::computeDifference(vnew_unsorted, vcur, vadded, vremoved);
}

// Refresh shown time of our last interaction with all listed avatars.
void LLAvatarList::updateLastInteractionTimes()
{
	S32 now = (S32) LLDate::now().secondsSinceEpoch();
	std::vector<LLPanel*> items;
	getItems(items);

	for( std::vector<LLPanel*>::const_iterator it = items.begin(); it != items.end(); it++)
	{
		// *TODO: error handling
		LLAvatarListItem* item = static_cast<LLAvatarListItem*>(*it);
		S32 secs_since = now - (S32) LLRecentPeople::instance().getDate(item->getAvatarId()).secondsSinceEpoch();
		if (secs_since >= 0)
			item->setLastInteractionTime(secs_since);
	}
}

void LLAvatarList::onItemDoucleClicked(LLUICtrl* ctrl, S32 x, S32 y, MASK mask)
{
	mItemDoubleClickSignal(ctrl, x, y, mask);
}

bool LLAvatarItemComparator::compare(const LLPanel* item1, const LLPanel* item2) const
{
	const LLAvatarListItem* avatar_item1 = dynamic_cast<const LLAvatarListItem*>(item1);
	const LLAvatarListItem* avatar_item2 = dynamic_cast<const LLAvatarListItem*>(item2);
	
	if (!avatar_item1 || !avatar_item2)
	{
		llerror("item1 and item2 cannot be null", 0);
		return true;
	}

	return doCompare(avatar_item1, avatar_item2);
}

bool LLAvatarItemNameComparator::doCompare(const LLAvatarListItem* avatar_item1, const LLAvatarListItem* avatar_item2) const
{
	std::string name1 = avatar_item1->getAvatarName();
	std::string name2 = avatar_item2->getAvatarName();

	LLStringUtil::toUpper(name1);
	LLStringUtil::toUpper(name2);

	return name1 < name2;
}
bool LLAvatarItemAgentOnTopComparator::doCompare(const LLAvatarListItem* avatar_item1, const LLAvatarListItem* avatar_item2) const
{
	//keep agent on top, if first is agent, 
	//then we need to return true to elevate this id, otherwise false.
	if(avatar_item1->getAvatarId() == gAgentID)
	{
		return true;
	}
	else if (avatar_item2->getAvatarId() == gAgentID)
	{
		return false;
	}
	return LLAvatarItemNameComparator::doCompare(avatar_item1,avatar_item2);
}

/************************************************************************/
/*             class LLAvalineListItem                                  */
/************************************************************************/
LLAvalineListItem::LLAvalineListItem(bool hide_number/* = true*/) : LLAvatarListItem(false)
, mIsHideNumber(hide_number)
{
	// should not use buildPanel from the base class to ensure LLAvalineListItem::postBuild is called.
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_avatar_list_item.xml");
}

BOOL LLAvalineListItem::postBuild()
{
	BOOL rv = LLAvatarListItem::postBuild();

	if (rv)
	{
		setOnline(true);
		showLastInteractionTime(false);
		setShowProfileBtn(false);
		setShowInfoBtn(false);
		mAvatarIcon->setValue("Avaline_Icon");
		mAvatarIcon->setToolTip(std::string(""));
	}
	return rv;
}

// to work correctly this method should be called AFTER setAvatarId for avaline callers with hidden phone number
void LLAvalineListItem::setName(const std::string& name)
{
	if (mIsHideNumber)
	{
		static U32 order = 0;
		typedef std::map<LLUUID, U32> avaline_callers_nums_t;
		static avaline_callers_nums_t mAvalineCallersNums;

		llassert(getAvatarId() != LLUUID::null);

		const LLUUID &uuid = getAvatarId();

		if (mAvalineCallersNums.find(uuid) == mAvalineCallersNums.end())
		{
			mAvalineCallersNums[uuid] = ++order;
			LL_DEBUGS("Avaline") << "Set name for new avaline caller: " << uuid << ", order: " << order << LL_ENDL;
		}
		LLStringUtil::format_map_t args;
		args["[ORDER]"] = llformat("%u", mAvalineCallersNums[uuid]);
		std::string hidden_name = LLTrans::getString("AvalineCaller", args);

		LL_DEBUGS("Avaline") << "Avaline caller: " << uuid << ", name: " << hidden_name << LL_ENDL;
		LLAvatarListItem::setName(hidden_name);
	}
	else
	{
		const std::string& formatted_phone = LLTextUtil::formatPhoneNumber(name);
		LLAvatarListItem::setName(formatted_phone);
	}
}

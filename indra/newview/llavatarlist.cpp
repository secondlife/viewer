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

#include "llavatarlist.h"

// newview
#include "llcallingcard.h" // for LLAvatarTracker
#include "llcachename.h"
#include "llvoiceclient.h"

static LLDefaultChildRegistry::Register<LLAvatarList> r("avatar_list");

static bool findInsensitive(std::string haystack, const std::string& needle_upper)
{
    LLStringUtil::toUpper(haystack);
    return haystack.find(needle_upper) != std::string::npos;
}


//comparators
static const LLAvatarItemNameComparator NAME_COMPARATOR;
static const LLFlatListView::ItemReverseComparator REVERSE_NAME_COMPARATOR(NAME_COMPARATOR);

LLAvatarList::Params::Params()
:
volume_column_width("volume_column_width", 0)
, online_go_first("online_go_first", true)
{
}



LLAvatarList::LLAvatarList(const Params& p)
:	LLFlatListView(p)
, mOnlineGoFirst(p.online_go_first)
, mContextMenu(NULL)
{
	setCommitOnSelectionChange(true);

	// Set default sort order.
	setComparator(&NAME_COMPARATOR);
}

void LLAvatarList::computeDifference(
	const std::vector<LLUUID>& vnew_unsorted,
	std::vector<LLUUID>& vadded,
	std::vector<LLUUID>& vremoved)
{
	std::vector<LLUUID> vcur;
	std::vector<LLUUID> vnew = vnew_unsorted;

	// Convert LLSDs to LLUUIDs.
	{
		std::vector<LLSD> vcur_values;
		getValues(vcur_values);

		for (size_t i=0; i<vcur_values.size(); i++)
			vcur.push_back(vcur_values[i].asUUID());
	}

	std::sort(vcur.begin(), vcur.end());
	std::sort(vnew.begin(), vnew.end());

	std::vector<LLUUID>::iterator it;
	size_t maxsize = llmax(vcur.size(), vnew.size());
	vadded.resize(maxsize);
	vremoved.resize(maxsize);

	// what to remove
	it = set_difference(vcur.begin(), vcur.end(), vnew.begin(), vnew.end(), vremoved.begin());
	vremoved.erase(it, vremoved.end());

	// what to add
	it = set_difference(vnew.begin(), vnew.end(), vcur.begin(), vcur.end(), vadded.begin());
	vadded.erase(it, vadded.end());
}

BOOL LLAvatarList::update(const std::vector<LLUUID>& all_buddies, const std::string& name_filter)
{
	BOOL have_names = TRUE;
	bool have_filter = name_filter != LLStringUtil::null;

	// Save selection.	
	std::vector<LLUUID> selected_ids;
	getSelectedUUIDs(selected_ids);
	LLUUID current_id = getSelectedUUID();

	// Determine what to add and what to remove.
	std::vector<LLUUID> added, removed;
	LLAvatarList::computeDifference(all_buddies, added, removed);

	// Handle added items.
	for (std::vector<LLUUID>::const_iterator it=added.begin(); it != added.end(); it++)
	{
		std::string name;
		const LLUUID& buddy_id = *it;
		have_names &= gCacheName->getFullName(buddy_id, name);
		if (!have_filter || findInsensitive(name, name_filter))
		addNewItem(buddy_id, name, LLAvatarTracker::instance().isBuddyOnline(buddy_id));
	}

	// Handle removed items.
	for (std::vector<LLUUID>::const_iterator it=removed.begin(); it != removed.end(); it++)
	{
		removeItemByUUID(*it);
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
			have_names &= gCacheName->getFullName(buddy_id, name);
			if (!findInsensitive(name, name_filter))
				removeItemByUUID(buddy_id);
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
	return !have_filter || have_names;
}

void LLAvatarList::sortByName()
{
	setComparator(&NAME_COMPARATOR);
	sort();
}

//////////////////////////////////////////////////////////////////////////
// PROTECTED SECTION
//////////////////////////////////////////////////////////////////////////
void LLAvatarList::addNewItem(const LLUUID& id, const std::string& name, BOOL is_bold, EAddPosition pos)
{
	LLAvatarListItem* item = new LLAvatarListItem();
	item->showStatus(false);
	item->showInfoBtn(true);
	item->showSpeakingIndicator(true);
	item->setName(name);
	item->setAvatarId(id);
	item->setContextMenu(mContextMenu);

	item->childSetVisible("info_btn", false);

	addItem(item, id, pos);
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

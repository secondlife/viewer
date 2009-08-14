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
#include "lloutputmonitorctrl.h"
#include "llvoiceclient.h"

static LLDefaultChildRegistry::Register<LLAvatarList> r("avatar_list");

LLAvatarList::Params::Params()
:
	volume_column_width("volume_column_width", 0)
	, online_go_first("online_go_first", true)
{
	draw_heading	= true;
	draw_stripes	= false;
	multi_select	= false;
	column_padding	= 0;
	search_column	= COL_NAME;
	sort_column		= COL_NAME;
}

LLAvatarList::LLAvatarList(const Params& p)
:	LLScrollListCtrl(p)
	, mHaveVolumeColumn(p.volume_column_width > 0)
	, mOnlineGoFirst(p.online_go_first)
{
	setCommitOnSelectionChange(TRUE); // there's no such param in LLScrollListCtrl::Params

    // "volume" column
    {
    	LLScrollListColumn::Params col_params;
    	col_params.name = "volume";
    	col_params.header.label = "Volume"; // *TODO: localize or remove the header
    	col_params.width.pixel_width = p.volume_column_width;
    	addColumn(col_params);
    }

    // "name" column
    {
    	LLScrollListColumn::Params col_params;
    	col_params.name = "name";
    	col_params.header.label = "Name"; // *TODO: localize or remove the header
    	col_params.width.dynamic_width = true;
    	addColumn(col_params);
    }

    // "online status" column
    {
    	LLScrollListColumn::Params col_params;
    	col_params.name = "online";
    	col_params.header.label = "Online"; // *TODO: localize or remove the header
    	col_params.width.pixel_width = 0; // invisible column
    	addColumn(col_params);
    }

    
    // invisible "id" column
    {
    	LLScrollListColumn::Params col_params;
    	col_params.name = "id";
    	col_params.header.label = "ID"; // *TODO: localize or remove the header
    	col_params.width.pixel_width = 0;
    	addColumn(col_params);
    }

	// Primary sort = online status, secondary sort = name
	// The corresponding parameters don't work because we create columns dynamically.
	sortByColumnIndex(COL_NAME, TRUE);
	if (mOnlineGoFirst)
		sortByColumnIndex(COL_ONLINE, FALSE);
	setSearchColumn(COL_NAME);
}

// virtual
void LLAvatarList::draw()
{
	LLScrollListCtrl::draw();
	if (mHaveVolumeColumn)
	{
		updateVolume();
	}
}

std::vector<LLUUID> LLAvatarList::getSelectedIDs()
{
	LLUUID selected_id;
	std::vector<LLUUID> avatar_ids;
	std::vector<LLScrollListItem*> selected = getAllSelected();
	for(std::vector<LLScrollListItem*>::iterator itr = selected.begin(); itr != selected.end(); ++itr)
	{
		avatar_ids.push_back((*itr)->getUUID());
	}
	return avatar_ids;
}

void LLAvatarList::addItem(const LLUUID& id, const std::string& name, BOOL is_bold, EAddPosition pos)
{
	std::string fullname;

	// Populate list item.
	LLSD element;
	element["id"] = id;

	// Update volume column (if we have one)
	{
		std::string icon = mHaveVolumeColumn ? getVolumeIcon(id) : "";
		LLSD& volume_column = element["columns"][COL_VOLUME];
		volume_column["column"] = "volume";
		volume_column["type"] = "icon";
		volume_column["value"] = icon;
	}

	LLSD& friend_column = element["columns"][COL_NAME];
	friend_column["column"] = "name";
	friend_column["value"] = name;

	LLSD& online_column = element["columns"][COL_ONLINE];
	online_column["column"] = "online";
	online_column["value"] = is_bold ? "1" : "0";

	LLScrollListItem* new_itemp = addElement(element, pos);

	// Indicate buddy online status.
	// (looks like parsing font parameters from LLSD is broken)
	if (is_bold)
	{
		LLScrollListText* name_textp = dynamic_cast<LLScrollListText*>(new_itemp->getColumn(COL_NAME));
		if (name_textp)
			name_textp->setFontStyle(LLFontGL::BOLD);
		else
		{
			llwarns << "Name column not found" << llendl;
		}
	}
}

static bool findInsensitive(std::string haystack, const std::string& needle_upper)
{
    LLStringUtil::toUpper(haystack);
    return haystack.find(needle_upper) != std::string::npos;
}

BOOL LLAvatarList::update(const std::vector<LLUUID>& all_buddies, const std::string& name_filter)
{
	BOOL have_names = TRUE;
	
	// Save selection.	
	std::vector<LLUUID> selected_ids = getSelectedIDs();
	LLUUID current_id = getCurrentID();
	S32 pos = getScrollPos();
	
	std::vector<LLUUID>::const_iterator buddy_it = all_buddies.begin();
	deleteAllItems();
	for(; buddy_it != all_buddies.end(); ++buddy_it)
	{
		std::string name;
		const LLUUID& buddy_id = *buddy_it;
		have_names &= gCacheName->getFullName(buddy_id, name);
		if (name_filter != LLStringUtil::null && !findInsensitive(name, name_filter))
			continue;
		addItem(buddy_id, name, LLAvatarTracker::instance().isBuddyOnline(buddy_id));
	}

	// Changed item in place, need to request sort and update columns
	// because we might have changed data in a column on which the user
	// has already sorted. JC
	sortItems();

	// re-select items
	selectMultiple(selected_ids);
	setCurrentByID(current_id);
#if 0
	// Restore selection.
	if(selected_ids.size() > 0)
	{
		// only non-null if friends was already found. This may fail,
		// but we don't really care here, because refreshUI() will
		// clean up the interface.
		for(std::vector<LLUUID>::iterator itr = selected_ids.begin(); itr != selected_ids.end(); ++itr)
		{
			setSelectedByValue(*itr, true);
		}
	}
#endif
	setScrollPos(pos);

	return have_names;
}

// static
std::string LLAvatarList::getVolumeIcon(const LLUUID& id)
{
	//
	// Determine icon appropriate for the current avatar volume.
	//
	// *TODO: remove this in favor of LLOutputMonitorCtrl
	// when ListView widget is implemented
	// which is capable of containing arbitrary widgets.
	//
	static LLOutputMonitorCtrl::Params default_monitor_params(LLUICtrlFactory::getDefaultParams<LLOutputMonitorCtrl>());
	bool		muted = gVoiceClient->getIsModeratorMuted(id) || gVoiceClient->getOnMuteList(id);
	F32			power = gVoiceClient->getCurrentPower(id);
	std::string	icon;

	if (muted)
	{
		icon = default_monitor_params.image_mute.name;
	}
	else if (power == 0.f)
	{
		icon = default_monitor_params.image_off.name;
	}
	else if (power < LLVoiceClient::OVERDRIVEN_POWER_LEVEL)
	{
		S32 icon_image_idx = llmin(2, llfloor((power / LLVoiceClient::OVERDRIVEN_POWER_LEVEL) * 3.f));
		switch(icon_image_idx)
		{
		default:
		case 0:
			icon = default_monitor_params.image_on.name;
			break;
		case 1:
			icon = default_monitor_params.image_level_1.name;
			break;
		case 2:
			icon = default_monitor_params.image_level_2.name;
			break;
		}
	}
	else
	{
		// overdriven
		icon = default_monitor_params.image_level_3.name;
	}

	return icon;
}

// Update volume column for all list rows.
void LLAvatarList::updateVolume()
{
	item_list& items = getItemList();

	for (item_list::iterator item_it = items.begin();
		item_it != items.end();
		++item_it)
	{
		LLScrollListItem* itemp = (*item_it);
		LLUUID speaker_id = itemp->getUUID();

		LLScrollListCell* icon_cell = itemp->getColumn(COL_VOLUME);
		if (icon_cell)
			icon_cell->setValue(getVolumeIcon(speaker_id));
	}
}

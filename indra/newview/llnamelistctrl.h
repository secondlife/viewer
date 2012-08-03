/**
 * @file llnamelistctrl.h
 * @brief A list of names, automatically refreshing from the name cache.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLNAMELISTCTRL_H
#define LL_LLNAMELISTCTRL_H

#include <set>

#include "llscrolllistctrl.h"

class LLAvatarName;

/**
 * LLNameListCtrl item
 *
 * We don't use LLScrollListItem to be able to override getUUID(), which is needed
 * because the name list item value is not simply an UUID but a map (uuid, is_group).
 */
class LLNameListItem : public LLScrollListItem, public LLHandleProvider<LLNameListItem>
{
public:
	LLUUID	getUUID() const		{ return getValue()["uuid"].asUUID(); }
protected:
	friend class LLNameListCtrl;

	LLNameListItem( const LLScrollListItem::Params& p )
	:	LLScrollListItem(p)
	{
	}
};


class LLNameListCtrl
:	public LLScrollListCtrl, public LLInstanceTracker<LLNameListCtrl>
{
public:
	typedef enum e_name_type
	{
		INDIVIDUAL,
		GROUP,
		SPECIAL
	} ENameType;

	// provide names for enums
	struct NameTypeNames : public LLInitParam::TypeValuesHelper<LLNameListCtrl::ENameType, NameTypeNames>
	{
		static void declareValues();
	};

	struct NameItem : public LLInitParam::Block<NameItem, LLScrollListItem::Params>
	{
		Optional<std::string>				name;
		Optional<ENameType, NameTypeNames>	target;

		NameItem()
		:	name("name"),
			target("target", INDIVIDUAL)
		{}
	};

	struct NameColumn : public LLInitParam::ChoiceBlock<NameColumn>
	{
		Alternative<S32>				column_index;
		Alternative<std::string>		column_name;
		NameColumn()
		:	column_name("name_column"),
			column_index("name_column_index", 0)
		{}
	};

	struct Params : public LLInitParam::Block<Params, LLScrollListCtrl::Params>
	{
		Optional<NameColumn>	name_column;
		Optional<bool>	allow_calling_card_drop;
		Optional<bool>			short_names;
		Params();
	};

protected:
	LLNameListCtrl(const Params&);
	friend class LLUICtrlFactory;
public:
	// Add a user to the list by name.  It will be added, the name
	// requested from the cache, and updated as necessary.
	void addNameItem(const LLUUID& agent_id, EAddPosition pos = ADD_BOTTOM,
					 BOOL enabled = TRUE, const std::string& suffix = LLStringUtil::null);
	void addNameItem(NameItem& item, EAddPosition pos = ADD_BOTTOM);

	/*virtual*/ LLScrollListItem* addElement(const LLSD& element, EAddPosition pos = ADD_BOTTOM, void* userdata = NULL);
	LLScrollListItem* addNameItemRow(const NameItem& value, EAddPosition pos = ADD_BOTTOM, const std::string& suffix = LLStringUtil::null);

	// Add a user to the list by name.  It will be added, the name
	// requested from the cache, and updated as necessary.
	void addGroupNameItem(const LLUUID& group_id, EAddPosition pos = ADD_BOTTOM,
						  BOOL enabled = TRUE);
	void addGroupNameItem(NameItem& item, EAddPosition pos = ADD_BOTTOM);


	void removeNameItem(const LLUUID& agent_id);

	// LLView interface
	/*virtual*/ BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask,
									  BOOL drop, EDragAndDropType cargo_type, void *cargo_data,
									  EAcceptance *accept,
									  std::string& tooltip_msg);
	/*virtual*/ BOOL handleToolTip(S32 x, S32 y, MASK mask);

	void setAllowCallingCardDrop(BOOL b) { mAllowCallingCardDrop = b; }

	void sortByName(BOOL ascending);

	/*virtual*/ void updateColumns();

	/*virtual*/ void	mouseOverHighlightNthItem( S32 index );
private:
	void showInspector(const LLUUID& avatar_id, bool is_group);
	void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name, LLHandle<LLNameListItem> item);

private:
	S32    			mNameColumnIndex;
	std::string		mNameColumn;
	BOOL			mAllowCallingCardDrop;
	bool			mShortNames;  // display name only, no SLID
};


#endif

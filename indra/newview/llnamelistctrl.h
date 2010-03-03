/** 
 * @file llnamelistctrl.h
 * @brief A list of names, automatically refreshing from the name cache.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#ifndef LL_LLNAMELISTCTRL_H
#define LL_LLNAMELISTCTRL_H

#include <set>

#include "llscrolllistctrl.h"


class LLNameListCtrl
:	public LLScrollListCtrl, protected LLInstanceTracker<LLNameListCtrl>
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

	struct NameColumn : public LLInitParam::Choice<NameColumn>
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
		Params();
	};

protected:
	LLNameListCtrl(const Params&);
	friend class LLUICtrlFactory;
public:
	// Add a user to the list by name.  It will be added, the name 
	// requested from the cache, and updated as necessary.
	void addNameItem(const LLUUID& agent_id, EAddPosition pos = ADD_BOTTOM,
					 BOOL enabled = TRUE, std::string& suffix = LLStringUtil::null);
	void addNameItem(NameItem& item, EAddPosition pos = ADD_BOTTOM);

	/*virtual*/ LLScrollListItem* addElement(const LLSD& element, EAddPosition pos = ADD_BOTTOM, void* userdata = NULL);
	LLScrollListItem* addNameItemRow(const NameItem& value, EAddPosition pos = ADD_BOTTOM, std::string& suffix = LLStringUtil::null);

	// Add a user to the list by name.  It will be added, the name 
	// requested from the cache, and updated as necessary.
	void addGroupNameItem(const LLUUID& group_id, EAddPosition pos = ADD_BOTTOM,
						  BOOL enabled = TRUE);
	void addGroupNameItem(NameItem& item, EAddPosition pos = ADD_BOTTOM);


	void removeNameItem(const LLUUID& agent_id);

	void refresh(const LLUUID& id, const std::string& first, const std::string& last, BOOL is_group);

	static void refreshAll(const LLUUID& id, const std::string& firstname,
						   const std::string& lastname, BOOL is_group);

	// LLView interface
	/*virtual*/ BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask,
									  BOOL drop, EDragAndDropType cargo_type, void *cargo_data,
									  EAcceptance *accept,
									  std::string& tooltip_msg);
	/*virtual*/ BOOL handleToolTip(S32 x, S32 y, MASK mask);

	void setAllowCallingCardDrop(BOOL b) { mAllowCallingCardDrop = b; }

	/*virtual*/ void updateColumns();

	/*virtual*/ void	mouseOverHighlightNthItem( S32 index );
private:
	void showInspector(const LLUUID& avatar_id, bool is_group);

private:
	S32    			mNameColumnIndex;
	std::string		mNameColumn;
	BOOL			mAllowCallingCardDrop;
};

/**
 * LLNameListCtrl item
 * 
 * We don't use LLScrollListItem to be able to override getUUID(), which is needed
 * because the name list item value is not simply an UUID but a map (uuid, is_group).
 */
class LLNameListItem : public LLScrollListItem
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

#endif

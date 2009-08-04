/** 
 * @file llnamelistctrl.cpp
 * @brief A list of names, automatically refreshed from name cache.
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

#include "llviewerprecompiledheaders.h"

#include <boost/tokenizer.hpp>

#include "llnamelistctrl.h"

#include "llcachename.h"
#include "llagent.h"
#include "llinventory.h"
#include "llscrolllistitem.h"
#include "llscrolllistcell.h"
#include "llscrolllistcolumn.h"
#include "llsdparam.h"

static LLDefaultChildRegistry::Register<LLNameListCtrl> r("name_list");

void LLNameListCtrl::NameTypeNames::declareValues()
{
	declare("INDIVIDUAL", LLNameListCtrl::INDIVIDUAL);
	declare("GROUP", LLNameListCtrl::GROUP);
	declare("SPECIAL", LLNameListCtrl::SPECIAL);
}

LLNameListCtrl::Params::Params()
:	name_column(""),
	allow_calling_card_drop("allow_calling_card_drop", false)
{
	name = "name_list";
}

LLNameListCtrl::LLNameListCtrl(const LLNameListCtrl::Params& p)
:	LLScrollListCtrl(p),
	mAllowCallingCardDrop(p.allow_calling_card_drop),
	mNameColumn(p.name_column.column_name),
	mNameColumnIndex(p.name_column.column_index)
{}

// public
void LLNameListCtrl::addNameItem(const LLUUID& agent_id, EAddPosition pos,
								 BOOL enabled, std::string& suffix)
{
	//llinfos << "LLNameListCtrl::addNameItem " << agent_id << llendl;

	std::string fullname;
	gCacheName->getFullName(agent_id, fullname);

	fullname.append(suffix);

	addStringUUIDItem(fullname, agent_id, pos, enabled);
}

// virtual, public
BOOL LLNameListCtrl::handleDragAndDrop( 
		S32 x, S32 y, MASK mask,
		BOOL drop,
		EDragAndDropType cargo_type, void *cargo_data, 
		EAcceptance *accept,
		std::string& tooltip_msg)
{
	if (!mAllowCallingCardDrop)
	{
		return FALSE;
	}

	BOOL handled = FALSE;

	if (cargo_type == DAD_CALLINGCARD)
	{
		if (drop)
		{
			LLInventoryItem* item = (LLInventoryItem *)cargo_data;
			addNameItem(item->getCreatorUUID());
		}

		*accept = ACCEPT_YES_MULTI;
	}
	else
	{
		*accept = ACCEPT_NO;
		if (tooltip_msg.empty())
		{
			if (!getToolTip().empty())
			{
				tooltip_msg = getToolTip();
			}
			else
			{
				// backwards compatable English tooltip (should be overridden in xml)
				tooltip_msg.assign("Drag a calling card here\nto add a resident.");
			}
		}
	}

	handled = TRUE;
	lldebugst(LLERR_USER_INPUT) << "dragAndDrop handled by LLNameListCtrl " << getName() << llendl;

	return handled;
}

// public
void LLNameListCtrl::addGroupNameItem(const LLUUID& group_id, EAddPosition pos,
									  BOOL enabled)
{
	NameItem item;
	item.value = group_id;
	item.enabled = enabled;
	item.target = GROUP;

	addNameItemRow(item, pos);
}

// public
void LLNameListCtrl::addGroupNameItem(LLNameListCtrl::NameItem& item, EAddPosition pos)
{
	item.target = GROUP;
	addNameItemRow(item, pos);
}

void LLNameListCtrl::addNameItem(LLNameListCtrl::NameItem& item, EAddPosition pos)
{
	item.target = INDIVIDUAL;
	addNameItemRow(item, pos);
}

LLScrollListItem* LLNameListCtrl::addElement(const LLSD& element, EAddPosition pos, void* userdata)
{
	LLNameListCtrl::NameItem item_params;
	LLParamSDParser::instance().readSD(element, item_params);
	item_params.userdata = userdata;
	return addNameItemRow(item_params, pos);
}


LLScrollListItem* LLNameListCtrl::addNameItemRow(const LLNameListCtrl::NameItem& name_item, EAddPosition pos)
{
	LLScrollListItem* item = LLScrollListCtrl::addRow(name_item, pos);
	if (!item) return NULL;

	// use supplied name by default
	std::string fullname = name_item.name;
	switch(name_item.target)
	{
	case GROUP:
		gCacheName->getGroupName(name_item.value().asUUID(), fullname);
		// fullname will be "nobody" if group not found
		break;
	case SPECIAL:
		// just use supplied name
		break;
	case INDIVIDUAL:
		{
			std::string name;
			if (gCacheName->getFullName(name_item.value().asUUID(), name))
			{
				fullname = name;
			}
			break;
		}
	default:
		break;
	}
	
	LLScrollListCell* cell = item->getColumn(mNameColumnIndex);
	if (cell)
	{
		cell->setValue(fullname);
	}

	dirtyColumns();

	// this column is resizable
	LLScrollListColumn* columnp = getColumn(mNameColumnIndex);
	if (columnp && columnp->mHeader)
	{
		columnp->mHeader->setHasResizableElement(TRUE);
	}

	return item;
}

// public
void LLNameListCtrl::removeNameItem(const LLUUID& agent_id)
{
	BOOL item_exists = selectByID( agent_id );
	if(item_exists)
	{
		S32 index = getItemIndex(getFirstSelected());
		if(index >= 0)
		{
			deleteSingleItem(index);
		}
	}
}

// public
void LLNameListCtrl::refresh(const LLUUID& id, const std::string& first, 
							 const std::string& last, BOOL is_group)
{
	//llinfos << "LLNameListCtrl::refresh " << id << " '" << first << " "
	//	<< last << "'" << llendl;

	std::string fullname;
	if (!is_group)
	{
		fullname = first + " " + last;
	}
	else
	{
		fullname = first;
	}

	// TODO: scan items for that ID, fix if necessary
	item_list::iterator iter;
	for (iter = getItemList().begin(); iter != getItemList().end(); iter++)
	{
		LLScrollListItem* item = *iter;
		if (item->getUUID() == id)
		{
			LLScrollListCell* cell = (LLScrollListCell*)item->getColumn(0);
			cell = item->getColumn(mNameColumnIndex);
			if (cell)
			{
				cell->setValue(fullname);
			}
		}
	}

	dirtyColumns();
}


// static
void LLNameListCtrl::refreshAll(const LLUUID& id, const std::string& first,
								const std::string& last, BOOL is_group)
{
	LLInstanceTracker<LLNameListCtrl>::instance_iter it;
	for (it = beginInstances(); it != endInstances(); ++it)
	{
		LLNameListCtrl* ctrl = *it;
		ctrl->refresh(id, first, last, is_group);
	}
}

void LLNameListCtrl::updateColumns()
{
	LLScrollListCtrl::updateColumns();

	if (!mNameColumn.empty())
	{
		LLScrollListColumn* name_column = getColumn(mNameColumn);
		if (name_column)
		{
			mNameColumnIndex = name_column->mIndex;
		}
	}
}

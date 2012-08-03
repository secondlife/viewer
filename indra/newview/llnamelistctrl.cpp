/** 
 * @file llnamelistctrl.cpp
 * @brief A list of names, automatically refreshed from name cache.
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

#include "llviewerprecompiledheaders.h"

#include "llnamelistctrl.h"

#include <boost/tokenizer.hpp>

#include "llavatarnamecache.h"
#include "llcachename.h"
#include "llfloaterreg.h"
#include "llinventory.h"
#include "llscrolllistitem.h"
#include "llscrolllistcell.h"
#include "llscrolllistcolumn.h"
#include "llsdparam.h"
#include "lltooltip.h"
#include "lltrans.h"

static LLDefaultChildRegistry::Register<LLNameListCtrl> r("name_list");

static const S32 info_icon_size = 16;

void LLNameListCtrl::NameTypeNames::declareValues()
{
	declare("INDIVIDUAL", LLNameListCtrl::INDIVIDUAL);
	declare("GROUP", LLNameListCtrl::GROUP);
	declare("SPECIAL", LLNameListCtrl::SPECIAL);
}

LLNameListCtrl::Params::Params()
:	name_column(""),
	allow_calling_card_drop("allow_calling_card_drop", false),
	short_names("short_names", false)
{
}

LLNameListCtrl::LLNameListCtrl(const LLNameListCtrl::Params& p)
:	LLScrollListCtrl(p),
	mNameColumnIndex(p.name_column.column_index),
	mNameColumn(p.name_column.column_name),
	mAllowCallingCardDrop(p.allow_calling_card_drop),
	mShortNames(p.short_names)
{}

// public
void LLNameListCtrl::addNameItem(const LLUUID& agent_id, EAddPosition pos,
								 BOOL enabled, const std::string& suffix)
{
	//llinfos << "LLNameListCtrl::addNameItem " << agent_id << llendl;

	NameItem item;
	item.value = agent_id;
	item.enabled = enabled;
	item.target = INDIVIDUAL;

	addNameItemRow(item, pos, suffix);
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

void LLNameListCtrl::showInspector(const LLUUID& avatar_id, bool is_group)
{
	if (is_group)
		LLFloaterReg::showInstance("inspect_group", LLSD().with("group_id", avatar_id));
	else
		LLFloaterReg::showInstance("inspect_avatar", LLSD().with("avatar_id", avatar_id));
}

void	LLNameListCtrl::mouseOverHighlightNthItem( S32 target_index )
{
	S32 cur_index = getHighlightedItemInx();
	if (cur_index != target_index)
	{
		bool is_mouse_over_name_cell = false;

		S32 mouse_x, mouse_y;
		LLUI::getMousePositionLocal(this, &mouse_x, &mouse_y);

		S32 column_index = getColumnIndexFromOffset(mouse_x);
		LLScrollListItem* hit_item = hitItem(mouse_x, mouse_y);
		if (hit_item && column_index == mNameColumnIndex)
		{
			// Get the name cell which is currently under the mouse pointer.
			LLScrollListCell* hit_cell = hit_item->getColumn(column_index);
			if (hit_cell)
			{
				is_mouse_over_name_cell = getCellRect(cur_index, column_index).pointInRect(mouse_x, mouse_y);
			}
		}

		// If the tool tip is visible and the mouse is over the currently highlighted item's name cell,
		// we should not reset the highlighted item index i.e. set mHighlightedItem = -1
		// and should not increase the width of the text inside the cell because it may
		// overlap the tool tip icon.
		if (LLToolTipMgr::getInstance()->toolTipVisible() && is_mouse_over_name_cell)
			return;

		if(0 <= cur_index && cur_index < (S32)getItemList().size())
		{
			LLScrollListItem* item = getItemList()[cur_index];
			if (item)
			{
				LLScrollListText* cell = dynamic_cast<LLScrollListText*>(item->getColumn(mNameColumnIndex));
				if (cell)
					cell->setTextWidth(cell->getTextWidth() + info_icon_size);
			}
			else
			{
				llwarns << "highlighted name list item is NULL" << llendl;
			}
		}
		if(target_index != -1)
		{
			LLScrollListItem* item = getItemList()[target_index];
			LLScrollListText* cell = dynamic_cast<LLScrollListText*>(item->getColumn(mNameColumnIndex));
			if (item)
			{
				if (cell)
					cell->setTextWidth(cell->getTextWidth() - info_icon_size);
			}
			else
			{
				llwarns << "target name item is NULL" << llendl;
			}
		}
	}

	LLScrollListCtrl::mouseOverHighlightNthItem(target_index);
}

//virtual
BOOL LLNameListCtrl::handleToolTip(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;
	S32 column_index = getColumnIndexFromOffset(x);
	LLScrollListItem* hit_item = hitItem(x, y);
	if (hit_item
		&& column_index == mNameColumnIndex)
	{
		// ...this is the column with the avatar name
		LLUUID avatar_id = hit_item->getUUID();
		if (avatar_id.notNull())
		{
			// ...valid avatar id

			LLScrollListCell* hit_cell = hit_item->getColumn(column_index);
			if (hit_cell)
			{
				S32 row_index = getItemIndex(hit_item);
				LLRect cell_rect = getCellRect(row_index, column_index);
				// Convert rect local to screen coordinates
				LLRect sticky_rect;
				localRectToScreen(cell_rect, &sticky_rect);

				// Spawn at right side of cell
				LLPointer<LLUIImage> icon = LLUI::getUIImage("Info_Small");
				LLCoordGL pos( sticky_rect.mRight - info_icon_size, sticky_rect.mTop - (sticky_rect.getHeight() - icon->getHeight())/2 );

				// Should we show a group or an avatar inspector?
				bool is_group = hit_item->getValue()["is_group"].asBoolean();

				LLToolTip::Params params;
				params.background_visible( false );
				params.click_callback( boost::bind(&LLNameListCtrl::showInspector, this, avatar_id, is_group) );
				params.delay_time(0.0f);		// spawn instantly on hover
				params.image( icon );
				params.message("");
				params.padding(0);
				params.pos(pos);
				params.sticky_rect(sticky_rect);

				LLToolTipMgr::getInstance()->show(params);
				handled = TRUE;
			}
		}
	}
	if (!handled)
	{
		handled = LLScrollListCtrl::handleToolTip(x, y, mask);
	}
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
	LLParamSDParser parser;
	parser.readSD(element, item_params);
	item_params.userdata = userdata;
	return addNameItemRow(item_params, pos);
}


LLScrollListItem* LLNameListCtrl::addNameItemRow(
	const LLNameListCtrl::NameItem& name_item,
	EAddPosition pos,
	const std::string& suffix)
{
	LLUUID id = name_item.value().asUUID();
	LLNameListItem* item = NULL;

	// Store item type so that we can invoke the proper inspector.
	// *TODO Vadim: Is there a more proper way of storing additional item data?
	{
		LLNameListCtrl::NameItem item_p(name_item);
		item_p.value = LLSD().with("uuid", id).with("is_group", name_item.target() == GROUP);
		item = new LLNameListItem(item_p);
		LLScrollListCtrl::addRow(item, item_p, pos);
	}

	if (!item) return NULL;

	// use supplied name by default
	std::string fullname = name_item.name;
	switch(name_item.target)
	{
	case GROUP:
		gCacheName->getGroupName(id, fullname);
		// fullname will be "nobody" if group not found
		break;
	case SPECIAL:
		// just use supplied name
		break;
	case INDIVIDUAL:
		{
			LLAvatarName av_name;
			if (id.isNull())
			{
				fullname = LLTrans::getString("AvatarNameNobody");
			}
			else if (LLAvatarNameCache::get(id, &av_name))
			{
				if (mShortNames)
					fullname = av_name.mDisplayName;
				else
					fullname = av_name.getCompleteName();
			}
			else
			{
				// ...schedule a callback
				LLAvatarNameCache::get(id,
					boost::bind(&LLNameListCtrl::onAvatarNameCache,
						this, _1, _2, item->getHandle()));
			}
			break;
		}
	default:
		break;
	}
	
	// Append optional suffix.
	if (!suffix.empty())
	{
		fullname.append(suffix);
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
	// Find the item specified with agent_id.
	S32 idx = -1;
	for (item_list::iterator it = getItemList().begin(); it != getItemList().end(); it++)
	{
		LLScrollListItem* item = *it;
		if (item->getUUID() == agent_id)
		{
			idx = getItemIndex(item);
			break;
		}
	}

	// Remove it.
	if (idx >= 0)
	{
		selectNthItem(idx); // not sure whether this is needed, taken from previous implementation
		deleteSingleItem(idx);
	}
}

void LLNameListCtrl::onAvatarNameCache(const LLUUID& agent_id,
									   const LLAvatarName& av_name,
									   LLHandle<LLNameListItem> item)
{
	std::string name;
	if (mShortNames)
		name = av_name.mDisplayName;
	else
		name = av_name.getCompleteName();

	LLNameListItem* list_item = item.get();
	if (list_item && list_item->getUUID() == agent_id)
	{
		LLScrollListCell* cell = list_item->getColumn(mNameColumnIndex);
		if (cell)
		{
			cell->setValue(name);
			setNeedsSort();
		}
	}

	dirtyColumns();
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

void LLNameListCtrl::sortByName(BOOL ascending)
{
	sortByColumnIndex(mNameColumnIndex,ascending);
}

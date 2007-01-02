/** 
 * @file llnamelistctrl.cpp
 * @brief A list of names, automatically refreshed from name cache.
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include <boost/tokenizer.hpp>

#include "llnamelistctrl.h"

#include "llcachename.h"
#include "llagent.h"
#include "llinventory.h"

// statics
std::set<LLNameListCtrl*> LLNameListCtrl::sInstances;

LLNameListCtrl::LLNameListCtrl(const LLString& name,
							   const LLRect& rect,
							   LLUICtrlCallback cb,
							   void* userdata,
							   BOOL allow_multiple_selection,
							   BOOL draw_border,
							   S32 name_column_index,
							   const LLString& tooltip)
:	LLScrollListCtrl(name, rect, cb, userdata, allow_multiple_selection,
					 draw_border),
	mNameColumnIndex(name_column_index),
	mAllowCallingCardDrop(FALSE)
{
	setToolTip(tooltip);
	LLNameListCtrl::sInstances.insert(this);
}


// virtual
LLNameListCtrl::~LLNameListCtrl()
{
	LLNameListCtrl::sInstances.erase(this);
}


// public
BOOL LLNameListCtrl::addNameItem(const LLUUID& agent_id, EAddPosition pos,
								 BOOL enabled, LLString& suffix)
{
	//llinfos << "LLNameListCtrl::addNameItem " << agent_id << llendl;

	char first[DB_FIRST_NAME_BUF_SIZE];
	char last[DB_LAST_NAME_BUF_SIZE];

	BOOL result = gCacheName->getName(agent_id, first, last);

	LLString fullname;
	fullname.assign(first);
	fullname.append(1, ' ');
	fullname.append(last);
	fullname.append(suffix);

	addStringUUIDItem(fullname, agent_id, pos, enabled);

	return result;
}

// virtual, public
BOOL LLNameListCtrl::handleDragAndDrop( 
		S32 x, S32 y, MASK mask,
		BOOL drop,
		EDragAndDropType cargo_type, void *cargo_data, 
		EAcceptance *accept,
		LLString& tooltip_msg)
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
	//llinfos << "LLNameListCtrl::addGroupNameItem " << group_id << llendl;
	char group_name[DB_GROUP_NAME_BUF_SIZE];
	gCacheName->getGroupName(group_id, group_name);
	addStringUUIDItem(group_name, group_id, pos, enabled);
}

// public
void LLNameListCtrl::addGroupNameItem(LLScrollListItem* item, EAddPosition pos)
					
{
	//llinfos << "LLNameListCtrl::addGroupNameItem " << item->getUUID() << llendl;

	char group_name[DB_GROUP_NAME_BUF_SIZE];
	gCacheName->getGroupName(item->getUUID(), group_name);

	LLScrollListCell* cell = (LLScrollListCell*)item->getColumn(mNameColumnIndex);
	((LLScrollListText*)cell)->setText( group_name );

	addItem(item, pos);
}

BOOL LLNameListCtrl::addNameItem(LLScrollListItem* item, EAddPosition pos)
{
	//llinfos << "LLNameListCtrl::addNameItem " << item->getUUID() << llendl;

	char first[DB_FIRST_NAME_BUF_SIZE];
	char last[DB_LAST_NAME_BUF_SIZE];

	BOOL result = gCacheName->getName(item->getUUID(), first, last);

	LLString fullname;
	fullname.assign(first);
	fullname.append(1, ' ');
	fullname.append(last);

	LLScrollListCell* cell = (LLScrollListCell*)item->getColumn(mNameColumnIndex);
	((LLScrollListText*)cell)->setText( fullname );

	addItem(item, pos);

	return result;
}

LLScrollListItem* LLNameListCtrl::addElement(const LLSD& value, EAddPosition pos, void* userdata)
{
	LLScrollListItem* item = LLScrollListCtrl::addElement(value, pos, userdata);

	char first[DB_FIRST_NAME_BUF_SIZE];
	char last[DB_LAST_NAME_BUF_SIZE];

	gCacheName->getName(item->getUUID(), first, last);

	LLString fullname;
	fullname.assign(first);
	fullname.append(1, ' ');
	fullname.append(last);

	LLScrollListCell* cell = (LLScrollListCell*)item->getColumn(mNameColumnIndex);
	((LLScrollListText*)cell)->setText( fullname );

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
void LLNameListCtrl::refresh(const LLUUID& id, const char* first, 
							 const char* last,
							 BOOL is_group)
{
	//llinfos << "LLNameListCtrl::refresh " << id << " '" << first << " "
	//	<< last << "'" << llendl;

	LLString fullname;
	fullname.assign(first);
	if (last[0] && !is_group)
	{
		fullname.append(1, ' ');
		fullname.append(last);
	}

	// TODO: scan items for that ID, fix if necessary
	item_list::iterator iter;
	for (iter = mItemList.begin(); iter != mItemList.end(); iter++)
	{
		LLScrollListItem* item = *iter;
		if (item->getUUID() == id)
		{
			LLScrollListCell* cell = (LLScrollListCell*)item->getColumn(0);
			cell = (LLScrollListCell*)item->getColumn(mNameColumnIndex);

			((LLScrollListText*)cell)->setText( fullname );
		}
	}
}


// static
void LLNameListCtrl::refreshAll(const LLUUID& id, const char* first,
								const char* last,
								BOOL is_group)
{
	std::set<LLNameListCtrl*>::iterator it;
	for (it = LLNameListCtrl::sInstances.begin();
		 it != LLNameListCtrl::sInstances.end();
		 ++it)
	{
		LLNameListCtrl* ctrl = *it;
		ctrl->refresh(id, first, last, is_group);
	}
}

// virtual
LLXMLNodePtr LLNameListCtrl::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLScrollListCtrl::getXML();

	node->createChild("allow_calling_card_drop", TRUE)->setBoolValue(mAllowCallingCardDrop);

	if (mNameColumnIndex != 0)
	{
		node->createChild("name_column_index", TRUE)->setIntValue(mNameColumnIndex);
	}

	// Don't save contents, probably filled by code

	return node;
}

LLView* LLNameListCtrl::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("name_list");
	node->getAttributeString("name", name);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	BOOL multi_select = FALSE;
	node->getAttributeBOOL("multi_select", multi_select);

	BOOL draw_border = TRUE;
	node->getAttributeBOOL("draw_border", draw_border);

	BOOL draw_heading = FALSE;
	node->getAttributeBOOL("draw_heading", draw_heading);

	BOOL collapse_empty_columns = FALSE;
	node->getAttributeBOOL("collapse_empty_columns", collapse_empty_columns);

	S32 name_column_index = 0;
	node->getAttributeS32("name_column_index", name_column_index);

	LLUICtrlCallback callback = NULL;

	LLNameListCtrl* name_list = new LLNameListCtrl(name,
				   rect,
				   callback,
				   NULL,
				   multi_select,
				   draw_border,
				   name_column_index);

	name_list->setDisplayHeading(draw_heading);
	if (node->hasAttribute("heading_height"))
	{
		S32 heading_height;
		node->getAttributeS32("heading_height", heading_height);
		name_list->setHeadingHeight(heading_height);
	}
	if (node->hasAttribute("heading_font"))
	{
		LLString heading_font("");
		node->getAttributeString("heading_font", heading_font);
		LLFontGL* gl_font = LLFontGL::fontFromName(heading_font.c_str());
		name_list->setHeadingFont(gl_font);
	}
	name_list->setCollapseEmptyColumns(collapse_empty_columns);

	BOOL allow_calling_card_drop = FALSE;
	if (node->getAttributeBOOL("allow_calling_card_drop", allow_calling_card_drop))
	{
		name_list->setAllowCallingCardDrop(allow_calling_card_drop);
	}

	name_list->setScrollListParameters(node);

	name_list->initFromXML(node, parent);

	LLSD columns;
	S32 index = 0;
	LLXMLNodePtr child;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		if (child->hasName("column"))
		{
			LLString labelname("");
			child->getAttributeString("label", labelname);

			LLString columnname(labelname);
			child->getAttributeString("name", columnname);

			if (child->hasAttribute("relwidth"))
			{
				F32 columnrelwidth = 0.f;
				child->getAttributeF32("relwidth", columnrelwidth);
				columns[index]["relwidth"] = columnrelwidth;
			}
			else
			{
				S32 columnwidth = -1;
				child->getAttributeS32("width", columnwidth);
				columns[index]["width"] = columnwidth;
			}

			columns[index]["name"] = columnname;
			columns[index]["label"] = labelname;
			index++;
		}
	}
	name_list->setColumnHeadings(columns);

	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		if (child->hasName("row"))
		{
			LLUUID id;
			child->getAttributeUUID("id", id);

			LLSD row;

			row["id"] = id;

			S32 column_idx = 0;
			LLXMLNodePtr row_child;
			for (row_child = node->getFirstChild(); row_child.notNull(); row_child = row_child->getNextSibling())
			{
				if (row_child->hasName("column"))
				{
					LLString value = row_child->getTextContents();

					LLString columnname("");
					row_child->getAttributeString("name", columnname);

					LLString font("");
					row_child->getAttributeString("font", font);

					LLString font_style("");
					row_child->getAttributeString("font-style", font_style);

					row["columns"][column_idx]["column"] = columnname;
					row["columns"][column_idx]["value"] = value;
					row["columns"][column_idx]["font"] = font;
					row["columns"][column_idx]["font-style"] = font_style;
					column_idx++;
				}
			}
			name_list->addElement(row);
		}
	}

	LLString contents = node->getTextContents();

	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep("\t\n");
	tokenizer tokens(contents, sep);
	tokenizer::iterator token_iter = tokens.begin();

	while(token_iter != tokens.end())
	{
		const char* line = token_iter->c_str();
		name_list->addSimpleItem(line);
		++token_iter;
	}

	return name_list;
}


/** 
 * @file llnamebox.cpp
 * @brief A text display widget
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llnamebox.h"

#include "llerror.h"
#include "llfontgl.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "lluuid.h"

#include "llcachename.h"
#include "llagent.h"

// statics
std::set<LLNameBox*> LLNameBox::sInstances;


LLNameBox::LLNameBox(const std::string& name, const LLRect& rect, const LLUUID& name_id, BOOL is_group, const LLFontGL* font, BOOL mouse_opaque)
:	LLTextBox(name, rect, "(retrieving)", font, mouse_opaque),
	mNameID(name_id)
{
	LLNameBox::sInstances.insert(this);
	if(!name_id.isNull())
	{
		setNameID(name_id, is_group);
	}
	else
	{
		setText("");
	}
}

LLNameBox::~LLNameBox()
{
	LLNameBox::sInstances.erase(this);
}

void LLNameBox::setNameID(const LLUUID& name_id, BOOL is_group)
{
	mNameID = name_id;

	char first[DB_FIRST_NAME_BUF_SIZE];
	char last[DB_LAST_NAME_BUF_SIZE];
	char group_name[DB_GROUP_NAME_BUF_SIZE];
	LLString name;

	if (!is_group)
	{
		gCacheName->getName(name_id, first, last);

		name.assign(first);
		name.append(1, ' ');
		name.append(last);
	}
	else
	{
		gCacheName->getGroupName(name_id, group_name);

		name.assign(group_name);
	}

	setText(name);
}

void LLNameBox::refresh(const LLUUID& id, const char* firstname,
						   const char* lastname, BOOL is_group)
{
	if (id == mNameID)
	{
		LLString name;

		name.assign(firstname);
		if (!is_group)
		{
			name.append(1, ' ');
			name.append(lastname);
		}

		setText(name);
	}
}

void LLNameBox::refreshAll(const LLUUID& id, const char* firstname,
						   const char* lastname, BOOL is_group)
{
	std::set<LLNameBox*>::iterator it;
	for (it = LLNameBox::sInstances.begin();
		 it != LLNameBox::sInstances.end();
		 ++it)
	{
		LLNameBox* box = *it;
		box->refresh(id, firstname, lastname, is_group);
	}
}

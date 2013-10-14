/** 
 * @file llnamebox.cpp
 * @brief A text display widget
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llnamebox.h"

#include "llerror.h"
#include "llfontgl.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "lluuid.h"

#include "llcachename.h"

// statics
std::set<LLNameBox*> LLNameBox::sInstances;

static LLDefaultChildRegistry::Register<LLNameBox> r("name_box");


LLNameBox::LLNameBox(const Params& p)
:	LLTextBox(p)
{
	mNameID = LLUUID::null;
	mLink = p.link;
	mParseHTML = mLink; // STORM-215
	mInitialValue = p.initial_value().asString();
	LLNameBox::sInstances.insert(this);
	setText(LLStringUtil::null);
}

LLNameBox::~LLNameBox()
{
	LLNameBox::sInstances.erase(this);
}

void LLNameBox::setNameID(const LLUUID& name_id, BOOL is_group)
{
	mNameID = name_id;

	std::string name;
	BOOL got_name = FALSE;

	if (!is_group)
	{
		got_name = gCacheName->getFullName(name_id, name);
	}
	else
	{
		got_name = gCacheName->getGroupName(name_id, name);
	}

	// Got the name already? Set it.
	// Otherwise it will be set later in refresh().
	if (got_name)
		setName(name, is_group);
	else
		setText(mInitialValue);
}

void LLNameBox::refresh(const LLUUID& id, const std::string& full_name, bool is_group)
{
	if (id == mNameID)
	{
		setName(full_name, is_group);
	}
}

void LLNameBox::refreshAll(const LLUUID& id, const std::string& full_name, bool is_group)
{
	std::set<LLNameBox*>::iterator it;
	for (it = LLNameBox::sInstances.begin();
		 it != LLNameBox::sInstances.end();
		 ++it)
	{
		LLNameBox* box = *it;
		box->refresh(id, full_name, is_group);
	}
}

void LLNameBox::setName(const std::string& name, BOOL is_group)
{
	if (mLink)
	{
		std::string url;

		if (is_group)
			url = "[secondlife:///app/group/" + mNameID.asString() + "/about " + name + "]";
		else
			url = "[secondlife:///app/agent/" + mNameID.asString() + "/about " + name + "]";

		setText(url);
	}
	else
	{
		setText(name);
	}
}

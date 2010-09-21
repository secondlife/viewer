/** 
 * @file llnamebox.cpp
 * @brief A text display widget
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

void LLNameBox::refresh(const LLUUID& id, const std::string& firstname,
						const std::string& lastname, BOOL is_group)
{
	if (id == mNameID)
	{
		std::string name;
		if (!is_group)
		{
			name = firstname + " " + lastname;
		}
		else
		{
			name = firstname;
		}
		setName(name, is_group);
	}
}

void LLNameBox::refreshAll(const LLUUID& id, const std::string& firstname,
						   const std::string& lastname, BOOL is_group)
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

void LLNameBox::setName(const std::string& name, BOOL is_group)
{
	if (mLink)
	{
		std::string url;

		if (is_group)
			url = "[secondlife:///app/group/" + LLURI::escape(name) + "/about " + name + "]";
		else
			url = "[secondlife:///app/agent/" + mNameID.asString() + "/about " + name + "]";

		setText(url);
	}
	else
	{
		setText(name);
	}
}

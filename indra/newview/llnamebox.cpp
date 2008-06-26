/** 
 * @file llnamebox.cpp
 * @brief A text display widget
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
#include "llagent.h"

// statics
std::set<LLNameBox*> LLNameBox::sInstances;


LLNameBox::LLNameBox(const std::string& name, const LLRect& rect, const LLUUID& name_id, BOOL is_group, const LLFontGL* font, BOOL mouse_opaque)
:	LLTextBox(name, rect, std::string("(retrieving)"), font, mouse_opaque),
	mNameID(name_id)
{
	LLNameBox::sInstances.insert(this);
	if(!name_id.isNull())
	{
		setNameID(name_id, is_group);
	}
	else
	{
		setText(LLStringUtil::null);
	}
}

LLNameBox::~LLNameBox()
{
	LLNameBox::sInstances.erase(this);
}

void LLNameBox::setNameID(const LLUUID& name_id, BOOL is_group)
{
	mNameID = name_id;

	std::string name;

	if (!is_group)
	{
		gCacheName->getFullName(name_id, name);
	}
	else
	{
		gCacheName->getGroupName(name_id, name);
	}

	setText(name);
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
		setText(name);
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

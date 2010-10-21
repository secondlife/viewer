/** 
 * @file llnameeditor.cpp
 * @brief Name Editor to refresh a name.
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
 
#include "llnameeditor.h"
#include "llcachename.h"

#include "llfontgl.h"

#include "lluuid.h"
#include "llrect.h"
#include "llstring.h"
#include "llui.h"

static LLDefaultChildRegistry::Register<LLNameEditor> r("name_editor");

// statics
std::set<LLNameEditor*> LLNameEditor::sInstances;

LLNameEditor::LLNameEditor(const LLNameEditor::Params& p)
:	LLLineEditor(p)
{
	LLNameEditor::sInstances.insert(this);

	if(!p.name_id().isNull())
	{
		setNameID(p.name_id, p.is_group);
	}
}

LLNameEditor::~LLNameEditor()
{
	LLNameEditor::sInstances.erase(this);
}

void LLNameEditor::setNameID(const LLUUID& name_id, BOOL is_group)
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

void LLNameEditor::refresh(const LLUUID& id, const std::string& full_name, bool is_group)
{
	if (id == mNameID)
	{
		setText(full_name);
	}
}

void LLNameEditor::refreshAll(const LLUUID& id, const std::string& full_name, bool is_group)
{
	std::set<LLNameEditor*>::iterator it;
	for (it = LLNameEditor::sInstances.begin();
		 it != LLNameEditor::sInstances.end();
		 ++it)
	{
		LLNameEditor* box = *it;
		box->refresh(id, full_name, is_group);
	}
}

void LLNameEditor::setValue( const LLSD& value )
{
	setNameID(value.asUUID(), FALSE);
}

LLSD LLNameEditor::getValue() const
{
	return LLSD(mNameID);
}


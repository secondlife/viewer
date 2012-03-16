/** 
 * @file llinventoryclipboard.cpp
 * @brief LLInventoryClipboard class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llinventoryclipboard.h"


LLInventoryClipboard LLInventoryClipboard::sInstance;

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------


///----------------------------------------------------------------------------
/// Class LLInventoryClipboard
///----------------------------------------------------------------------------

LLInventoryClipboard::LLInventoryClipboard()
: mCutMode(false)
{
}

LLInventoryClipboard::~LLInventoryClipboard()
{
	reset();
}

void LLInventoryClipboard::add(const LLUUID& object)
{
	mObjects.put(object);
}

// this stores a single inventory object
void LLInventoryClipboard::store(const LLUUID& object)
{
	reset();
	mObjects.put(object);
}

void LLInventoryClipboard::store(const LLDynamicArray<LLUUID>& inv_objects)
{
	reset();
	S32 count = inv_objects.count();
	for(S32 i = 0; i < count; i++)
	{
		mObjects.put(inv_objects[i]);
	}
}

void LLInventoryClipboard::cut(const LLUUID& object)
{
	if(!mCutMode && !mObjects.empty())
	{
		//looks like there are some stored items, reset clipboard state
		reset();
	}
	mCutMode = true;
	add(object);
}
void LLInventoryClipboard::retrieve(LLDynamicArray<LLUUID>& inv_objects) const
{
	inv_objects.reset();
	S32 count = mObjects.count();
	for(S32 i = 0; i < count; i++)
	{
		inv_objects.put(mObjects[i]);
	}
}

void LLInventoryClipboard::reset()
{
	mObjects.reset();
	mCutMode = false;
}

// returns true if the clipboard has something pasteable in it.
BOOL LLInventoryClipboard::hasContents() const
{
	return (mObjects.count() > 0);
}


///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------

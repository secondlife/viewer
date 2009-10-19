/** 
 * @file llinventoryclipboard.cpp
 * @brief LLInventoryClipboard class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

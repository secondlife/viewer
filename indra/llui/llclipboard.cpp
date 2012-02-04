/** 
 * @file llclipboard.cpp
 * @brief LLClipboard base class
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

#include "linden_common.h"

#include "llclipboard.h"

#include "llerror.h"
#include "llmath.h"
#include "llstring.h"
#include "llview.h"
#include "llwindow.h"

LLClipboard::LLClipboard()
: mCutMode(false)
{
}

LLClipboard::~LLClipboard()
{
	reset();
}

void LLClipboard::add(const LLUUID& object)
{
	mObjects.put(object);
}

void LLClipboard::store(const LLUUID& object)
{
	reset();
	mObjects.put(object);
}

void LLClipboard::store(const LLDynamicArray<LLUUID>& inv_objects)
{
	reset();
	S32 count = inv_objects.count();
	for(S32 i = 0; i < count; i++)
	{
		mObjects.put(inv_objects[i]);
	}
}

void LLClipboard::cut(const LLUUID& object)
{
	if(!mCutMode && !mObjects.empty())
	{
		//looks like there are some stored items, reset clipboard state
		reset();
	}
	mCutMode = true;
	add(object);
}
void LLClipboard::retrieve(LLDynamicArray<LLUUID>& inv_objects) const
{
	inv_objects.reset();
	S32 count = mObjects.count();
	for(S32 i = 0; i < count; i++)
	{
		inv_objects.put(mObjects[i]);
	}
}

void LLClipboard::reset()
{
	mObjects.reset();
	mCutMode = false;
}

// returns true if the clipboard has something pasteable in it.
BOOL LLClipboard::hasContents() const
{
	return (mObjects.count() > 0);
}


void LLClipboard::copyFromSubstring(const LLWString &src, S32 pos, S32 len, const LLUUID& source_id )
{
	reset();
	if (source_id.notNull())
	{
		store(source_id);
	}
	mString = src.substr(pos, len);
	llinfos << "Merov debug : copyFromSubstring, string = " << wstring_to_utf8str(mString) << ", uuid = " << (hasContents() ? mObjects[0] : LLUUID::null) << llendl;
	LLView::getWindow()->copyTextToClipboard( mString );
}

void LLClipboard::copyFromString(const LLWString &src, const LLUUID& source_id )
{
	reset();
	if (source_id.notNull())
	{
		store(source_id);
	}
	mString = src;
	llinfos << "Merov debug : copyFromString, string = " << wstring_to_utf8str(mString) << ", uuid = " << (hasContents() ? mObjects[0] : LLUUID::null) << llendl;
	LLView::getWindow()->copyTextToClipboard( mString );
}

const LLWString& LLClipboard::getPasteWString( LLUUID* source_id )
{
	if (hasContents())
	{
		LLWString temp_string;
		LLView::getWindow()->pasteTextFromClipboard(temp_string);

		if (temp_string != mString)
		{
			reset();
			mString = temp_string;
		}
	}
	else
	{
		LLView::getWindow()->pasteTextFromClipboard(mString);
	}

	if (source_id)
	{
		*source_id = (hasContents() ? mObjects[0] : LLUUID::null);
	}

	llinfos << "Merov debug : getPasteWString, string = " << wstring_to_utf8str(mString) << ", uuid = " << (hasContents() ? mObjects[0] : LLUUID::null) << llendl;

	return mString;
}


BOOL LLClipboard::canPasteString() const
{
	return LLView::getWindow()->isClipboardTextAvailable();
}


void LLClipboard::copyFromPrimarySubstring(const LLWString &src, S32 pos, S32 len, const LLUUID& source_id )
{
	reset();
	if (source_id.notNull())
	{
		store(source_id);
	}
	mString = src.substr(pos, len);
	LLView::getWindow()->copyTextToPrimary( mString );
}


const LLWString& LLClipboard::getPastePrimaryWString( LLUUID* source_id )
{
	if (hasContents())
	{
		LLWString temp_string;
		LLView::getWindow()->pasteTextFromPrimary(temp_string);

		if (temp_string != mString)
		{
			reset();
			mString = temp_string;
		}
	}
	else
	{
		LLView::getWindow()->pasteTextFromPrimary(mString);
	}

	if (source_id)
	{
		*source_id = (hasContents() ? mObjects[0] : LLUUID::null);
	}
	
	return mString;
}


BOOL LLClipboard::canPastePrimaryString() const
{
	return LLView::getWindow()->isPrimaryTextAvailable();
}

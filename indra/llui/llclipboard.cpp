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

// Global singleton
LLClipboard gClipboard;


LLClipboard::LLClipboard()
{
	mSourceItem = NULL;
}


LLClipboard::~LLClipboard()
{
}


void LLClipboard::copyFromSubstring(const LLWString &src, S32 pos, S32 len, const LLUUID& source_id )
{
	mSourceID = source_id;
	mString = src.substr(pos, len);
	LLView::getWindow()->copyTextToClipboard( mString );
}

void LLClipboard::copyFromString(const LLWString &src, const LLUUID& source_id )
{
	mSourceID = source_id;
	mString = src;
	LLView::getWindow()->copyTextToClipboard( mString );
}

const LLWString& LLClipboard::getPasteWString( LLUUID* source_id )
{
	if( mSourceID.notNull() )
	{
		LLWString temp_string;
		LLView::getWindow()->pasteTextFromClipboard(temp_string);

		if( temp_string != mString )
		{
			mSourceID.setNull();
			mString = temp_string;
		}
	}
	else
	{
		LLView::getWindow()->pasteTextFromClipboard(mString);
	}

	if( source_id )
	{
		*source_id = mSourceID;
	}

	return mString;
}


BOOL LLClipboard::canPasteString() const
{
	return LLView::getWindow()->isClipboardTextAvailable();
}


void LLClipboard::copyFromPrimarySubstring(const LLWString &src, S32 pos, S32 len, const LLUUID& source_id )
{
	mSourceID = source_id;
	mString = src.substr(pos, len);
	LLView::getWindow()->copyTextToPrimary( mString );
}


const LLWString& LLClipboard::getPastePrimaryWString( LLUUID* source_id )
{
	if( mSourceID.notNull() )
	{
		LLWString temp_string;
		LLView::getWindow()->pasteTextFromPrimary(temp_string);

		if( temp_string != mString )
		{
			mSourceID.setNull();
			mString = temp_string;
		}
	}
	else
	{
		LLView::getWindow()->pasteTextFromPrimary(mString);
	}

	if( source_id )
	{
		*source_id = mSourceID;
	}

	return mString;
}


BOOL LLClipboard::canPastePrimaryString() const
{
	return LLView::getWindow()->isPrimaryTextAvailable();
}

void LLClipboard::setSourceObject(const LLUUID& source_id, LLAssetType::EType type) 
{
	mSourceItem = new LLInventoryObject (source_id, LLUUID::null, type, "");
}

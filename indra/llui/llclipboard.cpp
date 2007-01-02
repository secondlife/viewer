/** 
 * @file llclipboard.cpp
 * @brief LLClipboard base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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


LLWString LLClipboard::getPasteWString( LLUUID* source_id )
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


BOOL LLClipboard::canPasteString()
{
	return LLView::getWindow()->isClipboardTextAvailable();
}

/** 
 * @file llurllineeditorctrl.cpp
 * @brief LLURLLineEditor base class
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

#include "llclipboard.h"
#include "lluictrlfactory.h"

#include "llurllineeditorctrl.h"

#include "llweb.h"
#include "llslurl.h"

//Constructor
LLURLLineEditor::LLURLLineEditor(const LLLineEditor::Params& p)
: LLLineEditor(p){

}

// copy selection to clipboard
void LLURLLineEditor::copy()
{
	if( canCopy() )
	{
		copyEscapedURLToClipboard();
	}
}

// cut selection to clipboard
void LLURLLineEditor::cut()
{
	if( canCut() )
	{
		// Prepare for possible rollback
		LLURLLineEditorRollback rollback( this );

		copyEscapedURLToClipboard();

		deleteSelection();

		// Validate new string and rollback the if needed.
		BOOL need_to_rollback = ( mPrevalidateFunc && !mPrevalidateFunc( mText.getWString() ) );
		if( need_to_rollback )
		{
			rollback.doRollback( this );
			LLUI::reportBadKeystroke();
		}
		else
		if( mKeystrokeCallback )
		{
			mKeystrokeCallback( this );
		}
	}
}
// Copies escaped URL to clipboard
void LLURLLineEditor::copyEscapedURLToClipboard()
{
	S32 left_pos = llmin( mSelectionStart, mSelectionEnd );
	S32 length = llabs( mSelectionStart - mSelectionEnd );

	const std::string unescaped_text = wstring_to_utf8str(mText.getWString().substr(left_pos, length));
	LLWString text_to_copy;
	// *HACK: Because LLSLURL is currently broken we cannot use it to check if unescaped_text is a valid SLURL (see EXT-8335).
	if (LLStringUtil::startsWith(unescaped_text, "http://")) // SLURL
		text_to_copy = utf8str_to_wstring(LLWeb::escapeURL(unescaped_text));
	else // human-readable location
		text_to_copy = utf8str_to_wstring(unescaped_text);
		
	LLClipboard::instance().copyToClipboard(text_to_copy, 0, text_to_copy.size());
}

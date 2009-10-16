/** 
 * @file llchatmsgbox.cpp
 * @author Martin Reddy
 * @brief chat history text box, able to show array of strings with separator
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "llchatmsgbox.h"
#include "llwindow.h"


static LLDefaultChildRegistry::Register<LLChatMsgBox> r("text_chat");

class ChatSeparator : public LLTextSegment
{
public:
	ChatSeparator(S32 start, S32 end)
	:	LLTextSegment(start, end),
		mEditor(NULL)
	{}

	/*virtual*/ void linkToDocument(class LLTextBase* editor)
	{
		mEditor = editor;
	}

	/*virtual*/ void unlinkFromDocument(class LLTextBase* editor)
	{
		mEditor = NULL;
	}

	/*virtual*/ S32 getWidth(S32 first_char, S32 num_chars) const
	{
		return mEditor->getDocumentView()->getRect().getWidth();
	}

	/*virtual*/ F32	draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect)
	{
		gl_line_2d(draw_rect.mLeft + 5, draw_rect.getCenterY(), draw_rect.mRight - 5, draw_rect.getCenterY(), LLColor4::grey);
		return draw_rect.getWidth();
	}

private:
	LLTextBase* mEditor;
};


LLChatMsgBox::Params::Params() :
	block_spacing("block_spacing", 10)
{
	line_spacing.pixels = 4;
}

LLChatMsgBox::LLChatMsgBox(const Params& p) :
	LLTextBox(p),
	mBlockSpacing(p.block_spacing)
{}

void LLChatMsgBox::addText( const LLStringExplicit& text )
{
	S32 length = getLength();
	// if there is existing text, add a separator
	if (length > 0)
	{
		// chat separator exists right before the null terminator
		insertSegment(new ChatSeparator(length - 1, length - 1));
	}
	// prepend newline only if there is some existing text
	appendText(text, length > 0);
}

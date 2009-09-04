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

LLChatMsgBox::Params::Params() :
	block_spacing("block_spacing", 10)
{
	line_spacing = 4;
}

LLChatMsgBox::LLChatMsgBox(const Params& p) :
	LLTextBox(p),
	mBlockSpacing(p.block_spacing)
{}

void LLChatMsgBox::addText( const LLStringExplicit& text )
{
	LLWString t = mText.getWString();
	if (! t.empty())
	{
		t += '\n';
	}
	t += getWrappedText(text);
	LLTextBox::setText(wstring_to_utf8str(t));
	mSeparatorOffset.push_back(getLength());
}

void LLChatMsgBox::setText(const LLStringExplicit& text)
{
	mSeparatorOffset.clear();
	mText.clear();
	addText(text);
}

void LLChatMsgBox::setValue(const LLSD& value )
{ 
	setText(value.asString());
}

S32 LLChatMsgBox::getTextPixelHeight()
{
	S32 num_blocks = mSeparatorOffset.size();
	S32 num_lines = getTextLinesNum();
	return (S32)(num_lines * mDefaultFont->getLineHeight() + \
				 (num_lines-1) * mLineSpacing + \
				 (num_blocks-1) * mBlockSpacing + \
				 2 * mLineSpacing);
}

S32 LLChatMsgBox::getTextLinesNum()
{
	S32 num_lines = getLineCount();
	if (num_lines < 1)
	{
		num_lines = 1;
	}
	
	return num_lines;
}

void LLChatMsgBox::drawText(S32 x, S32 y, const LLWString &text, const LLColor4 &color)
{
	S32 start = 0;
	S32 width = getRect().getWidth()-10;

	// iterate through each block of text that has been added
	y -= mLineSpacing;
	for (std::vector<S32>::iterator it = mSeparatorOffset.begin(); true ;)
	{
		// display the text for this block
		S32 num_chars = *it - start;
		LLWString text = mDisplayText.substr(start, num_chars);
		LLTextBox::drawText(x, y, text, color);
		
		// exit the loop if this is the last text block
		start += num_chars + 1;  // skip the newline
		if (++it == mSeparatorOffset.end())
		{
			break;
		}

		// output a separator line between blocks
		S32 num_lines = std::count(text.begin(), text.end(), '\n') + 1;
		y -= num_lines * (llfloor(mDefaultFont->getLineHeight()) + mLineSpacing);
		S32 sep_y = y - mBlockSpacing/2 + mLineSpacing/2;
		gl_line_2d(5, sep_y, width, sep_y, LLColor4::grey);
		y -= mBlockSpacing;
	}
}

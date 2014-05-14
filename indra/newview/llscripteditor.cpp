/**
 * @file llscripteditor.cpp
 * @author Cinder Roxley
 * @brief Text editor widget used for viewing and editing scripts
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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
#include "llscripteditor.h"

#include "llsyntaxid.h"
#include "lllocalcliprect.h"

const S32	UI_TEXTEDITOR_LINE_NUMBER_MARGIN = 32;
const S32	UI_TEXTEDITOR_LINE_NUMBER_DIGITS = 4;

static LLDefaultChildRegistry::Register<LLScriptEditor> r("script_editor");

LLScriptEditor::Params::Params()
:	show_line_numbers("show_line_numbers", true)
{}


LLScriptEditor::LLScriptEditor(const Params& p)
:	LLTextEditor(p)
,	mShowLineNumbers(p.show_line_numbers)
{
	if (mShowLineNumbers)
	{
		mHPad += UI_TEXTEDITOR_LINE_NUMBER_MARGIN;
		updateRects();
	}
}

void LLScriptEditor::draw()
{
	{
		// pad clipping rectangle so that cursor can draw at full width
		// when at left edge of mVisibleTextRect
		LLRect clip_rect(mVisibleTextRect);
		clip_rect.stretch(1);
		LLLocalClipRect clip(clip_rect);
	}
	
	LLTextBase::draw();
	drawLineNumbers();
	
    drawPreeditMarker();
	
	//RN: the decision was made to always show the orange border for keyboard focus but do not put an insertion caret
	// when in readonly mode
	mBorder->setKeyboardFocusHighlight( hasFocus() );// && !mReadOnly);
}

void LLScriptEditor::drawLineNumbers()
{
	LLGLSUIDefault gls_ui;
	LLRect scrolled_view_rect = getVisibleDocumentRect();
	LLRect content_rect = getVisibleTextRect();
	LLLocalClipRect clip(content_rect);
	S32 first_line = getFirstVisibleLine();
	S32 num_lines = getLineCount();
	if (first_line >= num_lines)
	{
		return;
	}
	
	S32 cursor_line = mLineInfoList[getLineNumFromDocIndex(mCursorPos)].mLineNum;
	
	if (mShowLineNumbers)
	{
		S32 left = 0;
		S32 top = getRect().getHeight();
		S32 bottom = 0;
		
		gl_rect_2d(left, top, UI_TEXTEDITOR_LINE_NUMBER_MARGIN, bottom, mReadOnlyBgColor.get() ); // line number area always read-only
		gl_rect_2d(UI_TEXTEDITOR_LINE_NUMBER_MARGIN, top, UI_TEXTEDITOR_LINE_NUMBER_MARGIN-1, bottom, LLColor4::grey3); // separator
		
		S32 last_line_num = -1;
		
		for (S32 cur_line = first_line; cur_line < num_lines; cur_line++)
		{
			line_info& line = mLineInfoList[cur_line];
			
			if ((line.mRect.mTop - scrolled_view_rect.mBottom) < mVisibleTextRect.mBottom)
			{
				break;
			}
			
			S32 line_bottom = line.mRect.mBottom - scrolled_view_rect.mBottom + mVisibleTextRect.mBottom;
			// draw the line numbers
			if(line.mLineNum != last_line_num && line.mRect.mTop <= scrolled_view_rect.mTop)
			{
				const LLFontGL *num_font = LLFontGL::getFontMonospace();
				const LLWString ltext = utf8str_to_wstring(llformat("%d", line.mLineNum ));
				BOOL is_cur_line = cursor_line == line.mLineNum;
				const U8 style = is_cur_line ? LLFontGL::BOLD : LLFontGL::NORMAL;
				const LLColor4 fg_color = is_cur_line ? mCursorColor : mReadOnlyFgColor;
				num_font->render(
								 ltext, // string to draw
								 0, // begin offset
								 UI_TEXTEDITOR_LINE_NUMBER_MARGIN - 2, // x
								 line_bottom, // y
								 fg_color,
								 LLFontGL::RIGHT, // horizontal alignment
								 LLFontGL::BOTTOM, // vertical alignment
								 style,
								 LLFontGL::NO_SHADOW,
								 S32_MAX, // max chars
								 UI_TEXTEDITOR_LINE_NUMBER_MARGIN - 2); // max pixels
				last_line_num = line.mLineNum;
			}
		}
	}
}

void LLScriptEditor::initKeywords()
{
	mKeywords.initialize(LLSyntaxIdLSL::getInstance()->getKeywordsXML());
}

LLTrace::BlockTimerStatHandle FTM_SYNTAX_HIGHLIGHTING("Syntax Highlighting");

void LLScriptEditor::loadKeywords()
{
	LL_RECORD_BLOCK_TIME(FTM_SYNTAX_HIGHLIGHTING);
	mKeywords.processTokens();
	
	segment_vec_t segment_list;
	mKeywords.findSegments(&segment_list, getWText(), mDefaultColor.get(), *this);
	
	mSegments.clear();
	segment_set_t::iterator insert_it = mSegments.begin();
	for (segment_vec_t::iterator list_it = segment_list.begin(); list_it != segment_list.end(); ++list_it)
	{
		insert_it = mSegments.insert(insert_it, *list_it);
	}
}

void LLScriptEditor::updateSegments()
{
	if (mReflowIndex < S32_MAX && mKeywords.isLoaded() && mParseOnTheFly)
	{
		LL_RECORD_BLOCK_TIME(FTM_SYNTAX_HIGHLIGHTING);
		// HACK:  No non-ascii keywords for now
		segment_vec_t segment_list;
		mKeywords.findSegments(&segment_list, getWText(), mDefaultColor.get(), *this);
		
		clearSegments();
		for (segment_vec_t::iterator list_it = segment_list.begin(); list_it != segment_list.end(); ++list_it)
		{
			insertSegment(*list_it);
		}
	}
	
	LLTextBase::updateSegments();
}

void LLScriptEditor::clearSegments()
{
	if (!mSegments.empty())
	{
		mSegments.clear();
	}
}

/**
 * @file llchatentry.cpp
 * @brief LLChatEntry implementation
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

#include "linden_common.h"
#include "llscrollcontainer.h"

#include "llchatentry.h"

static LLDefaultChildRegistry::Register<LLChatEntry> r("chat_editor");

LLChatEntry::Params::Params()
:	has_history("has_history", true),
 	is_expandable("is_expandable", false),
 	expand_lines_count("expand_lines_count", 1)
{}

LLChatEntry::LLChatEntry(const Params& p)
:	LLTextEditor(p),
 	mTextExpandedSignal(NULL),
 	mHasHistory(p.has_history),
 	mIsExpandable(p.is_expandable),
 	mExpandLinesCount(p.expand_lines_count),
 	mPrevLinesCount(0),
	mSingleLineMode(false),
	mPrevExpandedLineCount(S32_MAX)
{
	// Initialize current history line iterator
	mCurrentHistoryLine = mLineHistory.begin();

	mAutoIndent = false;
	keepSelectionOnReturn(true);
}

LLChatEntry::~LLChatEntry()
{
	delete mTextExpandedSignal;
}

void LLChatEntry::draw()
{
	if(mIsExpandable)
	{
		reflow();
		expandText();
	}
	LLTextEditor::draw();
}

void LLChatEntry::onCommit()
{
	updateHistory();
	LLTextEditor::onCommit();
}

boost::signals2::connection LLChatEntry::setTextExpandedCallback(const commit_signal_t::slot_type& cb)
{
	if (!mTextExpandedSignal)
	{
		mTextExpandedSignal = new commit_signal_t();
	}
	return mTextExpandedSignal->connect(cb);
}

void LLChatEntry::expandText()
{
	S32 line_count = mSingleLineMode ? 1 : mExpandLinesCount;

	int visible_lines_count = llabs(getVisibleLines(true).first - getVisibleLines(true).second);
	bool can_changed = getLineCount() <= line_count || line_count < mPrevExpandedLineCount ;
	mPrevExpandedLineCount = line_count;

	// true if pasted text has more lines than expand height limit and expand limit is not reached yet
	bool text_pasted = (getLineCount() > line_count) && (visible_lines_count < line_count);

	if (mIsExpandable && (can_changed || text_pasted || mSingleLineMode) && getLineCount() != mPrevLinesCount)
	{
		int lines_height = 0;
		if (text_pasted)
		{
			// text is pasted and now mLineInfoList.size() > mExpandLineCounts and mLineInfoList is not empty,
			// so lines_height is the sum of the last 'expanded_line_count' lines height
			lines_height = (mLineInfoList.end() - line_count)->mRect.mTop - mLineInfoList.back().mRect.mBottom;
		}
		else
		{
			lines_height = mLineInfoList.begin()->mRect.mTop - mLineInfoList.back().mRect.mBottom;
		}

		int height = mVPad * 2 +  lines_height;

		LLRect doc_rect = getRect();
		doc_rect.setOriginAndSize(doc_rect.mLeft, doc_rect.mBottom, doc_rect.getWidth(), height);
		setShape(doc_rect);

		mPrevLinesCount = getLineCount();

		if (mTextExpandedSignal)
		{
			(*mTextExpandedSignal)(this, LLSD() );
		}

		needsReflow();
	}
}

// line history support
void LLChatEntry::updateHistory()
{
	// On history enabled, remember committed line and
	// reset current history line number.
	// Be sure only to remember lines that are not empty and that are
	// different from the last on the list.
	if (mHasHistory && getLength())
	{
		// Add text to history, ignoring duplicates
		if (mLineHistory.empty() || getText() != mLineHistory.back())
		{
			mLineHistory.push_back(getText());
		}

		mCurrentHistoryLine = mLineHistory.end();
	}
}

void LLChatEntry::beforeValueChange()
{
    if(this->getLength() == 0 && !mLabel.empty())
    {
        this->clearSegments();
    }
}

void LLChatEntry::onValueChange(S32 start, S32 end)
{
    //Internally resetLabel() must meet a condition before it can reset the label
    resetLabel();
}

bool LLChatEntry::useLabel() const
{
    return !getLength() && !mLabel.empty();
}

void LLChatEntry::onFocusReceived()
{
	LLUICtrl::onFocusReceived();
	updateAllowingLanguageInput();
}

void LLChatEntry::onFocusLost()
{
	LLTextEditor::focusLostHelper();
	LLUICtrl::onFocusLost();
}

BOOL LLChatEntry::handleSpecialKey(const KEY key, const MASK mask)
{
	BOOL handled = FALSE;

	LLTextEditor::handleSpecialKey(key, mask);

	switch(key)
	{
	case KEY_RETURN:
		if (MASK_NONE == mask)
		{
			needsReflow();
		}
		break;

	case KEY_UP:
		if (mHasHistory && MASK_CONTROL == mask)
		{
			if (!mLineHistory.empty() && mCurrentHistoryLine > mLineHistory.begin())
			{
				setText(*(--mCurrentHistoryLine));
				endOfDoc();
			}
			else
			{
				LLUI::getInstance()->reportBadKeystroke();
			}
			handled = TRUE;
		}
		break;

	case KEY_DOWN:
		if (mHasHistory && MASK_CONTROL == mask)
		{
			if (!mLineHistory.empty() && mCurrentHistoryLine < (mLineHistory.end() - 1) )
			{
				setText(*(++mCurrentHistoryLine));
				endOfDoc();
			}
			else if (!mLineHistory.empty() && mCurrentHistoryLine == (mLineHistory.end() - 1) )
			{
				mCurrentHistoryLine++;
				std::string empty("");
				setText(empty);
				needsReflow();
				endOfDoc();
			}
			else
			{
				LLUI::getInstance()->reportBadKeystroke();
			}
			handled = TRUE;
		}
		break;

	default:
		break;
	}

	return handled;
}

void LLChatEntry::enableSingleLineMode(bool single_line_mode)
{
	if (mScroller)
	{
		mScroller->setSize(single_line_mode ? 0 : -1);
	}

	mSingleLineMode = single_line_mode;
	mPrevLinesCount = -1;
	setWordWrap(!single_line_mode);
}

/** 
 * @file lltexteditor.cpp
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

// Text editor widget to let users enter a a multi-line ASCII document.

#include "linden_common.h"

#define LLTEXTEDITOR_CPP
#include "lltexteditor.h"

#include "llfontfreetype.h" // for LLFontFreetype::FIRST_CHAR
#include "llfontgl.h"
#include "llgl.h"			// LLGLSUIDefault()
#include "lllocalcliprect.h"
#include "llrender.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llrect.h"
#include "llfocusmgr.h"
#include "lltimer.h"
#include "llmath.h"

#include "llclipboard.h"
#include "llscrollbar.h"
#include "llstl.h"
#include "llstring.h"
#include "llkeyboard.h"
#include "llkeywords.h"
#include "llundo.h"
#include "llviewborder.h"
#include "llcontrol.h"
#include "llwindow.h"
#include "lltextparser.h"
#include "llscrollcontainer.h"
#include "llspellcheck.h"
#include "llpanel.h"
#include "llurlregistry.h"
#include "lltooltip.h"
#include "llmenugl.h"

#include <queue>
#include "llcombobox.h"

// 
// Globals
//
static LLDefaultChildRegistry::Register<LLTextEditor> r("simple_text_editor");

// Compiler optimization, generate extern template
template class LLTextEditor* LLView::getChild<class LLTextEditor>(
	const std::string& name, BOOL recurse) const;

//
// Constants
//
const S32	UI_TEXTEDITOR_LINE_NUMBER_MARGIN = 32;
const S32	UI_TEXTEDITOR_LINE_NUMBER_DIGITS = 4;
const S32	SPACES_PER_TAB = 4;
const F32	SPELLCHECK_DELAY = 0.5f;	// delay between the last keypress and spell checking the word the cursor is on

///////////////////////////////////////////////////////////////////

class LLTextEditor::TextCmdInsert : public LLTextBase::TextCmd
{
public:
	TextCmdInsert(S32 pos, BOOL group_with_next, const LLWString &ws, LLTextSegmentPtr segment)
		: TextCmd(pos, group_with_next, segment), mWString(ws)
	{
	}
	virtual ~TextCmdInsert() {}
	virtual BOOL execute( LLTextBase* editor, S32* delta )
	{
		*delta = insert(editor, getPosition(), mWString );
		LLWStringUtil::truncate(mWString, *delta);
		//mWString = wstring_truncate(mWString, *delta);
		return (*delta != 0);
	}	
	virtual S32 undo( LLTextBase* editor )
	{
		remove(editor, getPosition(), mWString.length() );
		return getPosition();
	}
	virtual S32 redo( LLTextBase* editor )
	{
		insert(editor, getPosition(), mWString );
		return getPosition() + mWString.length();
	}

private:
	LLWString mWString;
};

///////////////////////////////////////////////////////////////////
class LLTextEditor::TextCmdAddChar : public LLTextBase::TextCmd
{
public:
	TextCmdAddChar( S32 pos, BOOL group_with_next, llwchar wc, LLTextSegmentPtr segment)
		: TextCmd(pos, group_with_next, segment), mWString(1, wc), mBlockExtensions(FALSE)
	{
	}
	virtual void blockExtensions()
	{
		mBlockExtensions = TRUE;
	}
	virtual BOOL canExtend(S32 pos) const
	{
		// cannot extend text with custom segments
		if (!mSegments.empty()) return FALSE;

		return !mBlockExtensions && (pos == getPosition() + (S32)mWString.length());
	}
	virtual BOOL execute( LLTextBase* editor, S32* delta )
	{
		*delta = insert(editor, getPosition(), mWString);
		LLWStringUtil::truncate(mWString, *delta);
		//mWString = wstring_truncate(mWString, *delta);
		return (*delta != 0);
	}
	virtual BOOL extendAndExecute( LLTextBase* editor, S32 pos, llwchar wc, S32* delta )	
	{ 
		LLWString ws;
		ws += wc;
		
		*delta = insert(editor, pos, ws);
		if( *delta > 0 )
		{
			mWString += wc;
		}
		return (*delta != 0);
	}
	virtual S32 undo( LLTextBase* editor )
	{
		remove(editor, getPosition(), mWString.length() );
		return getPosition();
	}
	virtual S32 redo( LLTextBase* editor )
	{
		insert(editor, getPosition(), mWString );
		return getPosition() + mWString.length();
	}

private:
	LLWString	mWString;
	BOOL		mBlockExtensions;

};

///////////////////////////////////////////////////////////////////

class LLTextEditor::TextCmdOverwriteChar : public LLTextBase::TextCmd
{
public:
	TextCmdOverwriteChar( S32 pos, BOOL group_with_next, llwchar wc)
		: TextCmd(pos, group_with_next), mChar(wc), mOldChar(0) {}

	virtual BOOL execute( LLTextBase* editor, S32* delta )
	{ 
		mOldChar = editor->getWText()[getPosition()];
		overwrite(editor, getPosition(), mChar);
		*delta = 0;
		return TRUE;
	}	
	virtual S32 undo( LLTextBase* editor )
	{
		overwrite(editor, getPosition(), mOldChar);
		return getPosition();
	}
	virtual S32 redo( LLTextBase* editor )
	{
		overwrite(editor, getPosition(), mChar);
		return getPosition()+1;
	}

private:
	llwchar		mChar;
	llwchar		mOldChar;
};

///////////////////////////////////////////////////////////////////

class LLTextEditor::TextCmdRemove : public LLTextBase::TextCmd
{
public:
	TextCmdRemove( S32 pos, BOOL group_with_next, S32 len, segment_vec_t& segments ) :
		TextCmd(pos, group_with_next), mLen(len)
	{
		std::swap(mSegments, segments);
	}
	virtual BOOL execute( LLTextBase* editor, S32* delta )
	{ 
		mWString = editor->getWText().substr(getPosition(), mLen);
		*delta = remove(editor, getPosition(), mLen );
		return (*delta != 0);
	}
	virtual S32 undo( LLTextBase* editor )
	{
		insert(editor, getPosition(), mWString);
		return getPosition() + mWString.length();
	}
	virtual S32 redo( LLTextBase* editor )
	{
		remove(editor, getPosition(), mLen );
		return getPosition();
	}
private:
	LLWString	mWString;
	S32				mLen;
};


///////////////////////////////////////////////////////////////////
LLTextEditor::Params::Params()
:	default_text("default_text"),
	prevalidate_callback("prevalidate_callback"),
	embedded_items("embedded_items", false),
	ignore_tab("ignore_tab", true),
	show_line_numbers("show_line_numbers", false),
	default_color("default_color"),
    commit_on_focus_lost("commit_on_focus_lost", false),
	show_context_menu("show_context_menu")
{
	addSynonym(prevalidate_callback, "text_type");
}

LLTextEditor::LLTextEditor(const LLTextEditor::Params& p) :
	LLTextBase(p),
	mBaseDocIsPristine(TRUE),
	mPristineCmd( NULL ),
	mLastCmd( NULL ),
	mDefaultColor(		p.default_color() ),
	mShowLineNumbers ( p.show_line_numbers ),
	mCommitOnFocusLost( p.commit_on_focus_lost),
	mAllowEmbeddedItems( p.embedded_items ),
	mMouseDownX(0),
	mMouseDownY(0),
	mTabsToNextField(p.ignore_tab),
	mPrevalidateFunc(p.prevalidate_callback()),
	mContextMenu(NULL),
	mShowContextMenu(p.show_context_menu)
{
	mSourceID.generate();

	//FIXME: use image?
	LLViewBorder::Params params;
	params.name = "text ed border";
	params.rect = getLocalRect();
	params.bevel_style = LLViewBorder::BEVEL_IN;
	params.border_thickness = 1;
	params.visible = p.border_visible;
	mBorder = LLUICtrlFactory::create<LLViewBorder> (params);
	addChild( mBorder );

	setText(p.default_text());

	if (mShowLineNumbers)
	{
		mHPad += UI_TEXTEDITOR_LINE_NUMBER_MARGIN;
		updateRects();
	}
	
	mParseOnTheFly = TRUE;
}

void LLTextEditor::initFromParams( const LLTextEditor::Params& p)
{
	LLTextBase::initFromParams(p);

	// HACK:  text editors always need to be enabled so that we can scroll
	LLView::setEnabled(true);

	if (p.commit_on_focus_lost.isProvided())
	{
		mCommitOnFocusLost = p.commit_on_focus_lost;
	}
	
	updateAllowingLanguageInput();
}

LLTextEditor::~LLTextEditor()
{
	gFocusMgr.releaseFocusIfNeeded( this ); // calls onCommit() while LLTextEditor still valid

	// Scrollbar is deleted by LLView
	std::for_each(mUndoStack.begin(), mUndoStack.end(), DeletePointer());

	// context menu is owned by menu holder, not us
	//delete mContextMenu;
}

////////////////////////////////////////////////////////////
// LLTextEditor
// Public methods

void LLTextEditor::setText(const LLStringExplicit &utf8str, const LLStyle::Params& input_params)
{
	// validate incoming text if necessary
	if (mPrevalidateFunc)
	{
		LLWString test_text = utf8str_to_wstring(utf8str);
		if (!mPrevalidateFunc(test_text))
		{
			// not valid text, nothing to do
			return;
		}
	}

	blockUndo();
	deselect();
	
	mParseOnTheFly = FALSE;
	LLTextBase::setText(utf8str, input_params);
	mParseOnTheFly = TRUE;

	resetDirty();
}

void LLTextEditor::selectNext(const std::string& search_text_in, BOOL case_insensitive, BOOL wrap)
{
	if (search_text_in.empty())
	{
		return;
	}

	LLWString text = getWText();
	LLWString search_text = utf8str_to_wstring(search_text_in);
	if (case_insensitive)
	{
		LLWStringUtil::toLower(text);
		LLWStringUtil::toLower(search_text);
	}
	
	if (mIsSelecting)
	{
		LLWString selected_text = text.substr(mSelectionEnd, mSelectionStart - mSelectionEnd);
		
		if (selected_text == search_text)
		{
			// We already have this word selected, we are searching for the next.
			setCursorPos(mCursorPos + search_text.size());
		}
	}
	
	S32 loc = text.find(search_text,mCursorPos);
	
	// If Maybe we wrapped, search again
	if (wrap && (-1 == loc))
	{	
		loc = text.find(search_text);
	}
	
	// If still -1, then search_text just isn't found.
    if (-1 == loc)
	{
		mIsSelecting = FALSE;
		mSelectionEnd = 0;
		mSelectionStart = 0;
		return;
	}

	setCursorPos(loc);
	
	mIsSelecting = TRUE;
	mSelectionEnd = mCursorPos;
	mSelectionStart = llmin((S32)getLength(), (S32)(mCursorPos + search_text.size()));
}

BOOL LLTextEditor::replaceText(const std::string& search_text_in, const std::string& replace_text,
							   BOOL case_insensitive, BOOL wrap)
{
	BOOL replaced = FALSE;

	if (search_text_in.empty())
	{
		return replaced;
	}

	LLWString search_text = utf8str_to_wstring(search_text_in);
	if (mIsSelecting)
	{
		LLWString text = getWText();
		LLWString selected_text = text.substr(mSelectionEnd, mSelectionStart - mSelectionEnd);

		if (case_insensitive)
		{
			LLWStringUtil::toLower(selected_text);
			LLWStringUtil::toLower(search_text);
		}

		if (selected_text == search_text)
		{
			insertText(replace_text);
			replaced = TRUE;
		}
	}

	selectNext(search_text_in, case_insensitive, wrap);
	return replaced;
}

void LLTextEditor::replaceTextAll(const std::string& search_text, const std::string& replace_text, BOOL case_insensitive)
{
	startOfDoc();
	selectNext(search_text, case_insensitive, FALSE);

	BOOL replaced = TRUE;
	while ( replaced )
	{
		replaced = replaceText(search_text,replace_text, case_insensitive, FALSE);
	}
}

S32 LLTextEditor::prevWordPos(S32 cursorPos) const
{
	LLWString wtext(getWText());
	while( (cursorPos > 0) && (wtext[cursorPos-1] == ' ') )
	{
		cursorPos--;
	}
	while( (cursorPos > 0) && LLWStringUtil::isPartOfWord( wtext[cursorPos-1] ) )
	{
		cursorPos--;
	}
	return cursorPos;
}

S32 LLTextEditor::nextWordPos(S32 cursorPos) const
{
	LLWString wtext(getWText());
	while( (cursorPos < getLength()) && LLWStringUtil::isPartOfWord( wtext[cursorPos] ) )
	{
		cursorPos++;
	} 
	while( (cursorPos < getLength()) && (wtext[cursorPos] == ' ') )
	{
		cursorPos++;
	}
	return cursorPos;
}

const LLTextSegmentPtr	LLTextEditor::getPreviousSegment() const
{
	static LLPointer<LLIndexSegment> index_segment = new LLIndexSegment;

	index_segment->setStart(mCursorPos);
	index_segment->setEnd(mCursorPos);

	// find segment index at character to left of cursor (or rightmost edge of selection)
	segment_set_t::const_iterator it = mSegments.lower_bound(index_segment);

	if (it != mSegments.end())
	{
		return *it;
	}
	else
	{
		return LLTextSegmentPtr();
	}
}

void LLTextEditor::getSelectedSegments(LLTextEditor::segment_vec_t& segments) const
{
	S32 left = hasSelection() ? llmin(mSelectionStart, mSelectionEnd) : mCursorPos;
	S32 right = hasSelection() ? llmax(mSelectionStart, mSelectionEnd) : mCursorPos;

	return getSegmentsInRange(segments, left, right, true);
}

void LLTextEditor::getSegmentsInRange(LLTextEditor::segment_vec_t& segments_out, S32 start, S32 end, bool include_partial) const
{
	segment_set_t::const_iterator first_it = getSegIterContaining(start);
	segment_set_t::const_iterator end_it = getSegIterContaining(end - 1);
	if (end_it != mSegments.end()) ++end_it;

	for (segment_set_t::const_iterator it = first_it; it != end_it; ++it)
	{
		LLTextSegmentPtr segment = *it;
		if (include_partial
			||	(segment->getStart() >= start
				&& segment->getEnd() <= end))
		{
			segments_out.push_back(segment);
		}
	}
}

BOOL LLTextEditor::selectionContainsLineBreaks()
{
	if (hasSelection())
	{
		S32 left = llmin(mSelectionStart, mSelectionEnd);
		S32 right = left + llabs(mSelectionStart - mSelectionEnd);

		LLWString wtext = getWText();
		for( S32 i = left; i < right; i++ )
		{
			if (wtext[i] == '\n')
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}


S32 LLTextEditor::indentLine( S32 pos, S32 spaces )
{
	// Assumes that pos is at the start of the line
	// spaces may be positive (indent) or negative (unindent).
	// Returns the actual number of characters added or removed.

	llassert(pos >= 0);
	llassert(pos <= getLength() );

	S32 delta_spaces = 0;

	if (spaces >= 0)
	{
		// Indent
		for(S32 i=0; i < spaces; i++)
		{
			delta_spaces += addChar(pos, ' ');
		}
	}
	else
	{
		// Unindent
		for(S32 i=0; i < -spaces; i++)
		{
			LLWString wtext = getWText();
			if (wtext[pos] == ' ')
			{
				delta_spaces += remove( pos, 1, FALSE );
			}
 		}
	}

	return delta_spaces;
}

void LLTextEditor::indentSelectedLines( S32 spaces )
{
	if( hasSelection() )
	{
		LLWString text = getWText();
		S32 left = llmin( mSelectionStart, mSelectionEnd );
		S32 right = left + llabs( mSelectionStart - mSelectionEnd );
		BOOL cursor_on_right = (mSelectionEnd > mSelectionStart);
		S32 cur = left;

		// Expand left to start of line
		while( (cur > 0) && (text[cur] != '\n') )
		{
			cur--;
		}
		left = cur;
		if( cur > 0 )
		{
			left++;
		}

		// Expand right to end of line
		if( text[right - 1] == '\n' )
		{
			right--;
		}
		else
		{
			while( (text[right] != '\n') && (right <= getLength() ) )
			{
				right++;
			}
		}

		// Disabling parsing on the fly to avoid updating text segments
		// until all indentation commands are executed.
		mParseOnTheFly = FALSE;

		// Find each start-of-line and indent it
		do
		{
			if( text[cur] == '\n' )
			{
				cur++;
			}

			S32 delta_spaces = indentLine( cur, spaces );
			if( delta_spaces > 0 )
			{
				cur += delta_spaces;
			}
			right += delta_spaces;

			text = getWText();

			// Find the next new line
			while( (cur < right) && (text[cur] != '\n') )
			{
				cur++;
			}
		}
		while( cur < right );

		mParseOnTheFly = TRUE;

		if( (right < getLength()) && (text[right] == '\n') )
		{
			right++;
		}

		// Set the selection and cursor
		if( cursor_on_right )
		{
			mSelectionStart = left;
			mSelectionEnd = right;
		}
		else
		{
			mSelectionStart = right;
			mSelectionEnd = left;
		}
		setCursorPos(mSelectionEnd);
	}
}

//virtual
BOOL LLTextEditor::canSelectAll() const
{
	return TRUE;
}

// virtual
void LLTextEditor::selectAll()
{
	mSelectionStart = getLength();
	mSelectionEnd = 0;
	setCursorPos(mSelectionEnd);
	updatePrimary();
}

BOOL LLTextEditor::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// set focus first, in case click callbacks want to change it
	// RN: do we really need to have a tab stop?
	if (hasTabStop())
	{
		setFocus( TRUE );
	}

	// Let scrollbar have first dibs
	handled = LLTextBase::handleMouseDown(x, y, mask);

	if( !handled )
	{
		if (!(mask & MASK_SHIFT))
		{
			deselect();
		}

		BOOL start_select = TRUE;
		if( start_select )
		{
			// If we're not scrolling (handled by child), then we're selecting
			if (mask & MASK_SHIFT)
			{
				S32 old_cursor_pos = mCursorPos;
				setCursorAtLocalPos( x, y, true );

				if (hasSelection())
				{
					mSelectionEnd = mCursorPos;
				}
				else
				{
					mSelectionStart = old_cursor_pos;
					mSelectionEnd = mCursorPos;
				}
				// assume we're starting a drag select
				mIsSelecting = TRUE;
			}
			else
			{
				setCursorAtLocalPos( x, y, true );
				startSelection();
			}
			gFocusMgr.setMouseCapture( this );
		}

		handled = TRUE;
	}

	// Delay cursor flashing
	resetCursorBlink();

	return handled;
}

BOOL LLTextEditor::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if (hasTabStop())
	{
		setFocus(TRUE);
	}
	// Prefer editor menu if it has selection. See EXT-6806.
	if (hasSelection() || !LLTextBase::handleRightMouseDown(x, y, mask))
	{
		if(getShowContextMenu())
		{
			showContextMenu(x, y);
		}
	}
	return TRUE;
}



BOOL LLTextEditor::handleMiddleMouseDown(S32 x, S32 y, MASK mask)
{
	if (hasTabStop())
	{
		setFocus(TRUE);
	}

	if (!LLTextBase::handleMouseDown(x, y, mask))
	{
		if( canPastePrimary() )
		{
			setCursorAtLocalPos( x, y, true );
			// does not rely on focus being set
			pastePrimary();
		}
	}
	return TRUE;
}


BOOL LLTextEditor::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;

	if(hasMouseCapture() )
	{
		if( mIsSelecting ) 
		{
			if(mScroller)
			{	
				mScroller->autoScroll(x, y);
			}
			S32 clamped_x = llclamp(x, mVisibleTextRect.mLeft, mVisibleTextRect.mRight);
			S32 clamped_y = llclamp(y, mVisibleTextRect.mBottom, mVisibleTextRect.mTop);
			setCursorAtLocalPos( clamped_x, clamped_y, true );
			mSelectionEnd = mCursorPos;
		}
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (active)" << llendl;		
		getWindow()->setCursor(UI_CURSOR_IBEAM);
		handled = TRUE;
	}

	if( !handled )
	{
		// Pass to children
		handled = LLTextBase::handleHover(x, y, mask);
	}

	if( handled )
	{
		// Delay cursor flashing
		resetCursorBlink();
	}

	if( !handled )
	{
		getWindow()->setCursor(UI_CURSOR_IBEAM);
		handled = TRUE;
	}

	return handled;
}


BOOL LLTextEditor::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// if I'm not currently selecting text
	if (!(hasSelection() && hasMouseCapture()))
	{
		// let text segments handle mouse event
		handled = LLTextBase::handleMouseUp(x, y, mask);
	}

	if( !handled )
	{
		if( mIsSelecting )
		{
			if(mScroller)
			{
				mScroller->autoScroll(x, y);
			}
			S32 clamped_x = llclamp(x, mVisibleTextRect.mLeft, mVisibleTextRect.mRight);
			S32 clamped_y = llclamp(y, mVisibleTextRect.mBottom, mVisibleTextRect.mTop);
			setCursorAtLocalPos( clamped_x, clamped_y, true );
			endSelection();
		}
		
		// take selection to 'primary' clipboard
		updatePrimary();

		handled = TRUE;
	}

	// Delay cursor flashing
	resetCursorBlink();

	if( hasMouseCapture()  )
	{
		gFocusMgr.setMouseCapture( NULL );
		
		handled = TRUE;
	}

	return handled;
}


BOOL LLTextEditor::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// let scrollbar and text segments have first dibs
	handled = LLTextBase::handleDoubleClick(x, y, mask);

	if( !handled )
	{
		setCursorAtLocalPos( x, y, false );
		deselect();

		LLWString text = getWText();
		
		if( LLWStringUtil::isPartOfWord( text[mCursorPos] ) )
		{
			// Select word the cursor is over
			while ((mCursorPos > 0) && LLWStringUtil::isPartOfWord(text[mCursorPos-1]))
			{
				if (!setCursorPos(mCursorPos - 1)) break;
			}
			startSelection();

			while ((mCursorPos < (S32)text.length()) && LLWStringUtil::isPartOfWord( text[mCursorPos] ) )
			{
				if (!setCursorPos(mCursorPos + 1)) break;
			}
		
			mSelectionEnd = mCursorPos;
		}
		else if ((mCursorPos < (S32)text.length()) && !iswspace( text[mCursorPos]) )
		{
			// Select the character the cursor is over
			startSelection();
			setCursorPos(mCursorPos + 1);
			mSelectionEnd = mCursorPos;
		}

		// We don't want handleMouseUp() to "finish" the selection (and thereby
		// set mSelectionEnd to where the mouse is), so we finish the selection here.
		mIsSelecting = FALSE;  

		// delay cursor flashing
		resetCursorBlink();

		// take selection to 'primary' clipboard
		updatePrimary();

		handled = TRUE;
	}

	return handled;
}


//----------------------------------------------------------------------------
// Returns change in number of characters in mText

S32 LLTextEditor::execute( TextCmd* cmd )
{
	S32 delta = 0;
	if( cmd->execute(this, &delta) )
	{
		// Delete top of undo stack
		undo_stack_t::iterator enditer = std::find(mUndoStack.begin(), mUndoStack.end(), mLastCmd);
		std::for_each(mUndoStack.begin(), enditer, DeletePointer());
		mUndoStack.erase(mUndoStack.begin(), enditer);
		// Push the new command is now on the top (front) of the undo stack.
		mUndoStack.push_front(cmd);
		mLastCmd = cmd;

		bool need_to_rollback = mPrevalidateFunc 
								&& !mPrevalidateFunc(getViewModel()->getDisplay());
		if (need_to_rollback)
		{
			// get rid of this last command and clean up undo stack
			undo();

			// remove any evidence of this command from redo history
			mUndoStack.pop_front();
			delete cmd;

			// failure, nothing changed
			delta = 0;
		}
	}
	else
	{
		// Operation failed, so don't put it on the undo stack.
		delete cmd;
	}

	return delta;
}

S32 LLTextEditor::insert(S32 pos, const LLWString &wstr, bool group_with_next_op, LLTextSegmentPtr segment)
{
	return execute( new TextCmdInsert( pos, group_with_next_op, wstr, segment ) );
}

S32 LLTextEditor::remove(S32 pos, S32 length, bool group_with_next_op)
{
	S32 end_pos = getEditableIndex(pos + length, true);

	segment_vec_t segments_to_remove;
	// store text segments
	getSegmentsInRange(segments_to_remove, pos, pos + length, false);

	return execute( new TextCmdRemove( pos, group_with_next_op, end_pos - pos, segments_to_remove ) );
}

S32 LLTextEditor::overwriteChar(S32 pos, llwchar wc)
{
	if ((S32)getLength() == pos)
	{
		return addChar(pos, wc);
	}
	else
	{
		return execute(new TextCmdOverwriteChar(pos, FALSE, wc));
	}
}

// Remove a single character from the text.  Tries to remove
// a pseudo-tab (up to for spaces in a row)
void LLTextEditor::removeCharOrTab()
{
	if( !getEnabled() )
	{
		return;
	}
	if( mCursorPos > 0 )
	{
		S32 chars_to_remove = 1;

		LLWString text = getWText();
		if (text[mCursorPos - 1] == ' ')
		{
			// Try to remove a "tab"
			S32 offset = getLineOffsetFromDocIndex(mCursorPos);
			if (offset > 0)
			{
				chars_to_remove = offset % SPACES_PER_TAB;
				if( chars_to_remove == 0 )
				{
					chars_to_remove = SPACES_PER_TAB;
				}

				for( S32 i = 0; i < chars_to_remove; i++ )
				{
					if (text[ mCursorPos - i - 1] != ' ')
					{
						// Fewer than a full tab's worth of spaces, so
						// just delete a single character.
						chars_to_remove = 1;
						break;
					}
				}
			}
		}
	
		for (S32 i = 0; i < chars_to_remove; i++)
		{
			setCursorPos(mCursorPos - 1);
			remove( mCursorPos, 1, FALSE );
		}
	}
	else
	{
		LLUI::reportBadKeystroke();
	}
}

// Remove a single character from the text
S32 LLTextEditor::removeChar(S32 pos)
{
	return remove( pos, 1, FALSE );
}

void LLTextEditor::removeChar()
{
	if (!getEnabled())
	{
		return;
	}
	if (mCursorPos > 0)
	{
		setCursorPos(mCursorPos - 1);
		removeChar(mCursorPos);
	}
	else
	{
		LLUI::reportBadKeystroke();
	}
}

// Add a single character to the text
S32 LLTextEditor::addChar(S32 pos, llwchar wc)
{
	if ( (wstring_utf8_length( getWText() ) + wchar_utf8_length( wc ))  > mMaxTextByteLength)
	{
		make_ui_sound("UISndBadKeystroke");
		return 0;
	}

	if (mLastCmd && mLastCmd->canExtend(pos))
	{
		S32 delta = 0;
		if (mPrevalidateFunc)
		{
			// get a copy of current text contents
			LLWString test_string(getViewModel()->getDisplay());

			// modify text contents as if this addChar succeeded
			llassert(pos <= (S32)test_string.size());
			test_string.insert(pos, 1, wc);
			if (!mPrevalidateFunc( test_string))
			{
				return 0;
			}
		}
		mLastCmd->extendAndExecute(this, pos, wc, &delta);

		return delta;
	}
	else
	{
		return execute(new TextCmdAddChar(pos, FALSE, wc, LLTextSegmentPtr()));
	}
}

void LLTextEditor::addChar(llwchar wc)
{
	if( !getEnabled() )
	{
		return;
	}
	if( hasSelection() )
	{
		deleteSelection(TRUE);
	}
	else if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode())
	{
		removeChar(mCursorPos);
	}

	setCursorPos(mCursorPos + addChar( mCursorPos, wc ));
}
void LLTextEditor::addLineBreakChar()
{
	if( !getEnabled() )
	{
		return;
	}
	if( hasSelection() )
	{
		deleteSelection(TRUE);
	}
	else if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode())
	{
		removeChar(mCursorPos);
	}

	LLStyleConstSP sp(new LLStyle(LLStyle::Params()));
	LLTextSegmentPtr segment = new LLLineBreakTextSegment(sp, mCursorPos);

	S32 pos = execute(new TextCmdAddChar(mCursorPos, FALSE, '\n', segment));
	
	setCursorPos(mCursorPos + pos);
}


BOOL LLTextEditor::handleSelectionKey(const KEY key, const MASK mask)
{
	BOOL handled = FALSE;

	if( mask & MASK_SHIFT )
	{
		handled = TRUE;
		
		switch( key )
		{
		case KEY_LEFT:
			if( 0 < mCursorPos )
			{
				startSelection();
				setCursorPos(mCursorPos - 1);
				if( mask & MASK_CONTROL )
				{
					setCursorPos(prevWordPos(mCursorPos));
				}
				mSelectionEnd = mCursorPos;
			}
			break;

		case KEY_RIGHT:
			if( mCursorPos < getLength() )
			{
				startSelection();
				setCursorPos(mCursorPos + 1);
				if( mask & MASK_CONTROL )
				{
					setCursorPos(nextWordPos(mCursorPos));
				}
				mSelectionEnd = mCursorPos;
			}
			break;

		case KEY_UP:
			startSelection();
			changeLine( -1 );
			mSelectionEnd = mCursorPos;
			break;

		case KEY_PAGE_UP:
			startSelection();
			changePage( -1 );
			mSelectionEnd = mCursorPos;
			break;

		case KEY_HOME:
			startSelection();
			if( mask & MASK_CONTROL )
			{
				setCursorPos(0);
			}
			else
			{
				startOfLine();
			}
			mSelectionEnd = mCursorPos;
			break;

		case KEY_DOWN:
			startSelection();
			changeLine( 1 );
			mSelectionEnd = mCursorPos;
			break;

		case KEY_PAGE_DOWN:
			startSelection();
			changePage( 1 );
			mSelectionEnd = mCursorPos;
			break;

		case KEY_END:
			startSelection();
			if( mask & MASK_CONTROL )
			{
				setCursorPos(getLength());
			}
			else
			{
				endOfLine();
			}
			mSelectionEnd = mCursorPos;
			break;

		default:
			handled = FALSE;
			break;
		}
	}

	if( handled )
	{
		// take selection to 'primary' clipboard
		updatePrimary();
	}
 
	return handled;
}

BOOL LLTextEditor::handleNavigationKey(const KEY key, const MASK mask)
{
	BOOL handled = FALSE;

	// Ignore capslock key
	if( MASK_NONE == mask )
	{
		handled = TRUE;
		switch( key )
		{
		case KEY_UP:
			changeLine( -1 );
			break;

		case KEY_PAGE_UP:
			changePage( -1 );
			break;

		case KEY_HOME:
			startOfLine();
			break;

		case KEY_DOWN:
			changeLine( 1 );
			deselect();
			break;

		case KEY_PAGE_DOWN:
			changePage( 1 );
			break;
 
		case KEY_END:
			endOfLine();
			break;

		case KEY_LEFT:
			if( hasSelection() )
			{
				setCursorPos(llmin( mSelectionStart, mSelectionEnd ));
			}
			else
			{
				if( 0 < mCursorPos )
				{
					setCursorPos(mCursorPos - 1);
				}
				else
				{
					LLUI::reportBadKeystroke();
				}
			}
			break;

		case KEY_RIGHT:
			if( hasSelection() )
			{
				setCursorPos(llmax( mSelectionStart, mSelectionEnd ));
			}
			else
			{
				if( mCursorPos < getLength() )
				{
					setCursorPos(mCursorPos + 1);
				}
				else
				{
					LLUI::reportBadKeystroke();
				}
			}	
			break;
			
		default:
			handled = FALSE;
			break;
		}
	}
	
	if (handled)
	{
		deselect();
	}
	
	return handled;
}

void LLTextEditor::deleteSelection(BOOL group_with_next_op )
{
	if( getEnabled() && hasSelection() )
	{
		S32 pos = llmin( mSelectionStart, mSelectionEnd );
		S32 length = llabs( mSelectionStart - mSelectionEnd );
	
		remove( pos, length, group_with_next_op );

		deselect();
		setCursorPos(pos);
	}
}

// virtual
BOOL LLTextEditor::canCut() const
{
	return !mReadOnly && hasSelection();
}

// cut selection to clipboard
void LLTextEditor::cut()
{
	if( !canCut() )
	{
		return;
	}
	S32 left_pos = llmin( mSelectionStart, mSelectionEnd );
	S32 length = llabs( mSelectionStart - mSelectionEnd );
	LLClipboard::instance().copyToClipboard( getWText(), left_pos, length);
	deleteSelection( FALSE );

	onKeyStroke();
}

BOOL LLTextEditor::canCopy() const
{
	return hasSelection();
}

// copy selection to clipboard
void LLTextEditor::copy()
{
	if( !canCopy() )
	{
		return;
	}
	S32 left_pos = llmin( mSelectionStart, mSelectionEnd );
	S32 length = llabs( mSelectionStart - mSelectionEnd );
	LLClipboard::instance().copyToClipboard(getWText(), left_pos, length);
}

BOOL LLTextEditor::canPaste() const
{
	return !mReadOnly && LLClipboard::instance().isTextAvailable();
}

// paste from clipboard
void LLTextEditor::paste()
{
	bool is_primary = false;
	pasteHelper(is_primary);
}

// paste from primary
void LLTextEditor::pastePrimary()
{
	bool is_primary = true;
	pasteHelper(is_primary);
}

// paste from primary (itsprimary==true) or clipboard (itsprimary==false)
void LLTextEditor::pasteHelper(bool is_primary)
{
	mParseOnTheFly = FALSE;
	bool can_paste_it;
	if (is_primary)
	{
		can_paste_it = canPastePrimary();
	}
	else
	{
		can_paste_it = canPaste();
	}

	if (!can_paste_it)
	{
		return;
	}

	LLWString paste;
	LLClipboard::instance().pasteFromClipboard(paste, is_primary);

	if (paste.empty())
	{
		return;
	}

	// Delete any selected characters (the paste replaces them)
	if( (!is_primary) && hasSelection() )
	{
		deleteSelection(TRUE);
	}

	// Clean up string (replace tabs and remove characters that our fonts don't support).
	LLWString clean_string(paste);
	LLWStringUtil::replaceTabsWithSpaces(clean_string, SPACES_PER_TAB);
	if( mAllowEmbeddedItems )
	{
		const llwchar LF = 10;
		S32 len = clean_string.length();
		for( S32 i = 0; i < len; i++ )
		{
			llwchar wc = clean_string[i];
			if( (wc < LLFontFreetype::FIRST_CHAR) && (wc != LF) )
			{
				clean_string[i] = LL_UNKNOWN_CHAR;
			}
			else if (wc >= FIRST_EMBEDDED_CHAR && wc <= LAST_EMBEDDED_CHAR)
			{
				clean_string[i] = pasteEmbeddedItem(wc);
			}
		}
	}

	// Insert the new text into the existing text.

	//paste text with linebreaks.
	std::basic_string<llwchar>::size_type start = 0;
	std::basic_string<llwchar>::size_type pos = clean_string.find('\n',start);
	
	while(pos!=-1)
	{
		if(pos!=start)
		{
			std::basic_string<llwchar> str = std::basic_string<llwchar>(clean_string,start,pos-start);
			setCursorPos(mCursorPos + insert(mCursorPos, str, FALSE, LLTextSegmentPtr()));
		}
		addLineBreakChar();
		
		start = pos+1;
		pos = clean_string.find('\n',start);
	}

	std::basic_string<llwchar> str = std::basic_string<llwchar>(clean_string,start,clean_string.length()-start);
	setCursorPos(mCursorPos + insert(mCursorPos, str, FALSE, LLTextSegmentPtr()));

	deselect();

	onKeyStroke();
	mParseOnTheFly = TRUE;
}



// copy selection to primary
void LLTextEditor::copyPrimary()
{
	if( !canCopy() )
	{
		return;
	}
	S32 left_pos = llmin( mSelectionStart, mSelectionEnd );
	S32 length = llabs( mSelectionStart - mSelectionEnd );
	LLClipboard::instance().copyToClipboard(getWText(), left_pos, length, true);
}

BOOL LLTextEditor::canPastePrimary() const
{
	return !mReadOnly && LLClipboard::instance().isTextAvailable(true);
}

void LLTextEditor::updatePrimary()
{
	if (canCopy())
	{
		copyPrimary();
	}
}

BOOL LLTextEditor::handleControlKey(const KEY key, const MASK mask)	
{
	BOOL handled = FALSE;

	if( mask & MASK_CONTROL )
	{
		handled = TRUE;

		switch( key )
		{
		case KEY_HOME:
			if( mask & MASK_SHIFT )
			{
				startSelection();
				setCursorPos(0);
				mSelectionEnd = mCursorPos;
			}
			else
			{
				// Ctrl-Home, Ctrl-Left, Ctrl-Right, Ctrl-Down
				// all move the cursor as if clicking, so should deselect.
				deselect();
				startOfDoc();
			}
			break;

		case KEY_END:
			{
				if( mask & MASK_SHIFT )
				{
					startSelection();
				}
				else
				{
					// Ctrl-Home, Ctrl-Left, Ctrl-Right, Ctrl-Down
					// all move the cursor as if clicking, so should deselect.
					deselect();
				}
				endOfDoc();
				if( mask & MASK_SHIFT )
				{
					mSelectionEnd = mCursorPos;
				}
				break;
			}

		case KEY_RIGHT:
			if( mCursorPos < getLength() )
			{
				// Ctrl-Home, Ctrl-Left, Ctrl-Right, Ctrl-Down
				// all move the cursor as if clicking, so should deselect.
				deselect();

				setCursorPos(nextWordPos(mCursorPos + 1));
			}
			break;


		case KEY_LEFT:
			if( mCursorPos > 0 )
			{
				// Ctrl-Home, Ctrl-Left, Ctrl-Right, Ctrl-Down
				// all move the cursor as if clicking, so should deselect.
				deselect();

				setCursorPos(prevWordPos(mCursorPos - 1));
			}
			break;

		default:
			handled = FALSE;
			break;
		}
	}

	if (handled)
	{
		updatePrimary();
	}

	return handled;
}


BOOL LLTextEditor::handleSpecialKey(const KEY key, const MASK mask)	
	{
	BOOL handled = TRUE;

	if (mReadOnly) return FALSE;

	switch( key )
	{
	case KEY_INSERT:
		if (mask == MASK_NONE)
		{
			gKeyboard->toggleInsertMode();
		}
		break;

	case KEY_BACKSPACE:
		if( hasSelection() )
		{
			deleteSelection(FALSE);
		}
		else
		if( 0 < mCursorPos )
		{
			removeCharOrTab();
		}
		else
		{
			LLUI::reportBadKeystroke();
		}
		break;


	case KEY_RETURN:
		if (mask == MASK_NONE)
		{
			if( hasSelection() )
			{
				deleteSelection(FALSE);
			}
			autoIndent(); // TODO: make this optional
		}
		else
		{
			handled = FALSE;
			break;
		}
		break;

	case KEY_TAB:
		if (mask & MASK_CONTROL)
		{
			handled = FALSE;
			break;
		}
		if( hasSelection() && selectionContainsLineBreaks() )
		{
			indentSelectedLines( (mask & MASK_SHIFT) ? -SPACES_PER_TAB : SPACES_PER_TAB );
		}
		else
		{
			if( hasSelection() )
			{
				deleteSelection(FALSE);
			}
			
			S32 offset = getLineOffsetFromDocIndex(mCursorPos);

			S32 spaces_needed = SPACES_PER_TAB - (offset % SPACES_PER_TAB);
			for( S32 i=0; i < spaces_needed; i++ )
			{
				addChar( ' ' );
			}
		}
		break;
		
	default:
		handled = FALSE;
		break;
	}

	if (handled)
	{
		onKeyStroke();
	}
	return handled;
}


void LLTextEditor::unindentLineBeforeCloseBrace()
{
	if( mCursorPos >= 1 )
	{
		LLWString text = getWText();
		if( ' ' == text[ mCursorPos - 1 ] )
		{
			removeCharOrTab();
		}
	}
}


BOOL LLTextEditor::handleKeyHere(KEY key, MASK mask )
{
	BOOL	handled = FALSE;

	// Special case for TAB.  If want to move to next field, report
	// not handled and let the parent take care of field movement.
	if (KEY_TAB == key && mTabsToNextField)
	{
		return FALSE;
	}
		
	if (mReadOnly && mScroller)
	{
		handled = (mScroller && mScroller->handleKeyHere( key, mask ))
				|| handleSelectionKey(key, mask)
				|| handleControlKey(key, mask);
		}
		else 
		{
		handled = handleNavigationKey( key, mask )
				|| handleSelectionKey(key, mask)
				|| handleControlKey(key, mask)
				|| handleSpecialKey(key, mask);
	}

	if( handled )
	{
		resetCursorBlink();
		needsScroll();
	}

	return handled;
}


BOOL LLTextEditor::handleUnicodeCharHere(llwchar uni_char)
{
	if ((uni_char < 0x20) || (uni_char == 0x7F)) // Control character or DEL
	{
		return FALSE;
	}

	BOOL	handled = FALSE;

	// Handle most keys only if the text editor is writeable.
	if( !mReadOnly )
	{
		if( '}' == uni_char )
		{
			unindentLineBeforeCloseBrace();
		}

		// TODO: KLW Add auto show of tool tip on (
		addChar( uni_char );

		// Keys that add characters temporarily hide the cursor
		getWindow()->hideCursorUntilMouseMove();

		handled = TRUE;
	}

	if( handled )
	{
		resetCursorBlink();

		// Most keystrokes will make the selection box go away, but not all will.
		deselect();

		onKeyStroke();
	}

	return handled;
}


// virtual
BOOL LLTextEditor::canDoDelete() const
{
	return !mReadOnly && ( hasSelection() || (mCursorPos < getLength()) );
}

void LLTextEditor::doDelete()
{
	if( !canDoDelete() )
	{
		return;
	}
	if( hasSelection() )
	{
		deleteSelection(FALSE);
	}
	else
	if( mCursorPos < getLength() )
	{	
		S32 i;
		S32 chars_to_remove = 1;
		LLWString text = getWText();
		if( (text[ mCursorPos ] == ' ') && (mCursorPos + SPACES_PER_TAB < getLength()) )
		{
			// Try to remove a full tab's worth of spaces
			S32 offset = getLineOffsetFromDocIndex(mCursorPos);
			chars_to_remove = SPACES_PER_TAB - (offset % SPACES_PER_TAB);
			if( chars_to_remove == 0 )
			{
				chars_to_remove = SPACES_PER_TAB;
			}

			for( i = 0; i < chars_to_remove; i++ )
			{
				if( text[mCursorPos + i] != ' ' )
				{
					chars_to_remove = 1;
					break;
				}
			}
		}

		for( i = 0; i < chars_to_remove; i++ )
		{
			setCursorPos(mCursorPos + 1);
			removeChar();
		}

	}

	onKeyStroke();
}

//----------------------------------------------------------------------------


void LLTextEditor::blockUndo()
{
	mBaseDocIsPristine = FALSE;
	mLastCmd = NULL;
	std::for_each(mUndoStack.begin(), mUndoStack.end(), DeletePointer());
	mUndoStack.clear();
}

// virtual
BOOL LLTextEditor::canUndo() const
{
	return !mReadOnly && mLastCmd != NULL;
}

void LLTextEditor::undo()
{
	if( !canUndo() )
	{
		return;
	}
	deselect();
	S32 pos = 0;
	do
	{
		pos = mLastCmd->undo(this);
		undo_stack_t::iterator iter = std::find(mUndoStack.begin(), mUndoStack.end(), mLastCmd);
		if (iter != mUndoStack.end())
			++iter;
		if (iter != mUndoStack.end())
			mLastCmd = *iter;
		else
			mLastCmd = NULL;

		} while( mLastCmd && mLastCmd->groupWithNext() );

		setCursorPos(pos);

	onKeyStroke();
}

BOOL LLTextEditor::canRedo() const
{
	return !mReadOnly && (mUndoStack.size() > 0) && (mLastCmd != mUndoStack.front());
}

void LLTextEditor::redo()
{
	if( !canRedo() )
	{
		return;
	}
	deselect();
	S32 pos = 0;
	do
	{
		if( !mLastCmd )
		{
			mLastCmd = mUndoStack.back();
		}
		else
		{
			undo_stack_t::iterator iter = std::find(mUndoStack.begin(), mUndoStack.end(), mLastCmd);
			if (iter != mUndoStack.begin())
				mLastCmd = *(--iter);
			else
				mLastCmd = NULL;
		}

			if( mLastCmd )
			{
				pos = mLastCmd->redo(this);
			}
		} while( 
			mLastCmd &&
			mLastCmd->groupWithNext() &&
			(mLastCmd != mUndoStack.front()) );
		
		setCursorPos(pos);

	onKeyStroke();
}

void LLTextEditor::onFocusReceived()
{
	LLTextBase::onFocusReceived();
	updateAllowingLanguageInput();
}

// virtual, from LLView
void LLTextEditor::onFocusLost()
{
	updateAllowingLanguageInput();

	// Route menu back to the default
 	if( gEditMenuHandler == this )
	{
		gEditMenuHandler = NULL;
	}

	if (mCommitOnFocusLost)
	{
		onCommit();
	}

	// Make sure cursor is shown again
	getWindow()->showCursorFromMouseMove();

	LLTextBase::onFocusLost();
}

void LLTextEditor::onCommit()
{
	setControlValue(getValue()); 
	LLTextBase::onCommit(); 
}

void LLTextEditor::setEnabled(BOOL enabled)
{
	// just treat enabled as read-only flag
	bool read_only = !enabled;
	if (read_only != mReadOnly)
	{
		//mReadOnly = read_only;
		LLTextBase::setReadOnly(read_only);
		updateSegments();
		updateAllowingLanguageInput();
	}
}

void LLTextEditor::showContextMenu(S32 x, S32 y)
{
	if (!mContextMenu)
	{
		mContextMenu = LLUICtrlFactory::instance().createFromFile<LLContextMenu>("menu_text_editor.xml", 
																				LLMenuGL::sMenuContainer, 
																				LLMenuHolderGL::child_registry_t::instance());
	}

	// Route menu to this class
	// previously this was done in ::handleRightMoseDown:
	//if(hasTabStop())
	// setFocus(TRUE)  - why? weird...
	// and then inside setFocus
	// ....
	//    gEditMenuHandler = this;
	// ....
	// but this didn't work in all cases and just weird...
    //why not here? 
	// (all this was done for EXT-4443)

	gEditMenuHandler = this;

	S32 screen_x, screen_y;
	localPointToScreen(x, y, &screen_x, &screen_y);

	setCursorAtLocalPos(x, y, false);
	if (hasSelection())
	{
		if ( (mCursorPos < llmin(mSelectionStart, mSelectionEnd)) || (mCursorPos > llmax(mSelectionStart, mSelectionEnd)) )
		{
			deselect();
		}
		else
		{
			setCursorPos(llmax(mSelectionStart, mSelectionEnd));
		}
	}

	bool use_spellcheck = getSpellCheck(), is_misspelled = false;
	if (use_spellcheck)
	{
		mSuggestionList.clear();

		// If the cursor is on a misspelled word, retrieve suggestions for it
		std::string misspelled_word = getMisspelledWord(mCursorPos);
		if ((is_misspelled = !misspelled_word.empty()) == true)
		{
			LLSpellChecker::instance().getSuggestions(misspelled_word, mSuggestionList);
		}
	}

	mContextMenu->setItemVisible("Suggestion Separator", (use_spellcheck) && (!mSuggestionList.empty()));
	mContextMenu->setItemVisible("Add to Dictionary", (use_spellcheck) && (is_misspelled));
	mContextMenu->setItemVisible("Add to Ignore", (use_spellcheck) && (is_misspelled));
	mContextMenu->setItemVisible("Spellcheck Separator", (use_spellcheck) && (is_misspelled));
	mContextMenu->show(screen_x, screen_y, this);
}


void LLTextEditor::drawPreeditMarker()
{
	static LLUICachedControl<F32> preedit_marker_brightness ("UIPreeditMarkerBrightness", 0);
	static LLUICachedControl<S32> preedit_marker_gap ("UIPreeditMarkerGap", 0);
	static LLUICachedControl<S32> preedit_marker_position ("UIPreeditMarkerPosition", 0);
	static LLUICachedControl<S32> preedit_marker_thickness ("UIPreeditMarkerThickness", 0);
	static LLUICachedControl<F32> preedit_standout_brightness ("UIPreeditStandoutBrightness", 0);
	static LLUICachedControl<S32> preedit_standout_gap ("UIPreeditStandoutGap", 0);
	static LLUICachedControl<S32> preedit_standout_position ("UIPreeditStandoutPosition", 0);
	static LLUICachedControl<S32> preedit_standout_thickness ("UIPreeditStandoutThickness", 0);

	if (!hasPreeditString())
	{
		return;
	}

    const LLWString textString(getWText());
	const llwchar *text = textString.c_str();
	const S32 text_len = getLength();
	const S32 num_lines = getLineCount();

	S32 cur_line = getFirstVisibleLine();
	if (cur_line >= num_lines)
	{
		return;
	}
		
	const S32 line_height = mDefaultFont->getLineHeight();

	S32 line_start = getLineStart(cur_line);
	S32 line_y = mVisibleTextRect.mTop - line_height;
	while((mVisibleTextRect.mBottom <= line_y) && (num_lines > cur_line))
	{
		S32 next_start = -1;
		S32 line_end = text_len;

		if ((cur_line + 1) < num_lines)
		{
			next_start = getLineStart(cur_line + 1);
			line_end = next_start;
		}
		if ( text[line_end-1] == '\n' )
		{
			--line_end;
		}

		// Does this line contain preedits?
		if (line_start >= mPreeditPositions.back())
		{
			// We have passed the preedits.
			break;
		}
		if (line_end > mPreeditPositions.front())
		{
			for (U32 i = 0; i < mPreeditStandouts.size(); i++)
			{
				S32 left = mPreeditPositions[i];
				S32 right = mPreeditPositions[i + 1];
				if (right <= line_start || left >= line_end)
				{
					continue;
				}

				S32 preedit_left = mVisibleTextRect.mLeft;
				if (left > line_start)
				{
					preedit_left += mDefaultFont->getWidth(text, line_start, left - line_start);
				}
				S32 preedit_right = mVisibleTextRect.mLeft;
				if (right < line_end)
				{
					preedit_right += mDefaultFont->getWidth(text, line_start, right - line_start);
				}
				else
				{
					preedit_right += mDefaultFont->getWidth(text, line_start, line_end - line_start);
				}

				if (mPreeditStandouts[i])
				{
					gl_rect_2d(preedit_left + preedit_standout_gap,
							line_y + preedit_standout_position,
							preedit_right - preedit_standout_gap - 1,
							line_y + preedit_standout_position - preedit_standout_thickness,
							(mCursorColor.get() * preedit_standout_brightness + mWriteableBgColor.get() * (1 - preedit_standout_brightness)).setAlpha(1.0f));
				}
				else
				{
					gl_rect_2d(preedit_left + preedit_marker_gap,
							line_y + preedit_marker_position,
							preedit_right - preedit_marker_gap - 1,
							line_y + preedit_marker_position - preedit_marker_thickness,
							(mCursorColor.get() * preedit_marker_brightness + mWriteableBgColor.get() * (1 - preedit_marker_brightness)).setAlpha(1.0f));
				}
			}
		}

		// move down one line
		line_y -= line_height;
		line_start = next_start;
		cur_line++;
	}
}


void LLTextEditor::drawLineNumbers()
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

void LLTextEditor::draw()
{
	{
		// pad clipping rectangle so that cursor can draw at full width
		// when at left edge of mVisibleTextRect
		LLRect clip_rect(mVisibleTextRect);
		clip_rect.stretch(1);
		LLLocalClipRect clip(clip_rect);
		drawPreeditMarker();
	}

	LLTextBase::draw();
	drawLineNumbers();

	//RN: the decision was made to always show the orange border for keyboard focus but do not put an insertion caret
	// when in readonly mode
	mBorder->setKeyboardFocusHighlight( hasFocus() );// && !mReadOnly);
}

// Start or stop the editor from accepting text-editing keystrokes
// see also LLLineEditor
void LLTextEditor::setFocus( BOOL new_state )
{
	BOOL old_state = hasFocus();

	// Don't change anything if the focus state didn't change
	if (new_state == old_state) return;

	// Notify early if we are losing focus.
	if (!new_state)
	{
		getWindow()->allowLanguageTextInput(this, FALSE);
	}

	LLTextBase::setFocus( new_state );

	if( new_state )
	{
		// Route menu to this class
		gEditMenuHandler = this;

		// Don't start the cursor flashing right away
		resetCursorBlink();
	}
	else
	{
		// Route menu back to the default
		if( gEditMenuHandler == this )
		{
			gEditMenuHandler = NULL;
		}

		endSelection();
	}
}

// public
void LLTextEditor::setCursorAndScrollToEnd()
{
	deselect();
	endOfDoc();
}

void LLTextEditor::getCurrentLineAndColumn( S32* line, S32* col, BOOL include_wordwrap ) 
{ 
	*line = getLineNumFromDocIndex(mCursorPos, include_wordwrap);
	*col = getLineOffsetFromDocIndex(mCursorPos, include_wordwrap);
}

void LLTextEditor::autoIndent()
{
	// Count the number of spaces in the current line
	S32 line = getLineNumFromDocIndex(mCursorPos, false);
	S32 line_start = getLineStart(line);
	S32 space_count = 0;
	S32 i;

	LLWString text = getWText();
	while( ' ' == text[line_start] )
	{
		space_count++;
		line_start++;
	}

	// If we're starting a braced section, indent one level.
	if( (mCursorPos > 0) && (text[mCursorPos -1] == '{') )
	{
		space_count += SPACES_PER_TAB;
	}

	// Insert that number of spaces on the new line

	//appendLineBreakSegment(LLStyle::Params());//addChar( '\n' );
	addLineBreakChar();

	for( i = 0; i < space_count; i++ )
	{
		addChar( ' ' );
	}
}

// Inserts new text at the cursor position
void LLTextEditor::insertText(const std::string &new_text)
{
	BOOL enabled = getEnabled();
	setEnabled( TRUE );

	// Delete any selected characters (the insertion replaces them)
	if( hasSelection() )
	{
		deleteSelection(TRUE);
	}

	setCursorPos(mCursorPos + insert( mCursorPos, utf8str_to_wstring(new_text), FALSE, LLTextSegmentPtr() ));
	
	setEnabled( enabled );
}

void LLTextEditor::insertText(LLWString &new_text)
{
	BOOL enabled = getEnabled();
	setEnabled( TRUE );

	// Delete any selected characters (the insertion replaces them)
	if( hasSelection() )
	{
		deleteSelection(TRUE);
	}

	setCursorPos(mCursorPos + insert( mCursorPos, new_text, FALSE, LLTextSegmentPtr() ));

	setEnabled( enabled );
}

void LLTextEditor::appendWidget(const LLInlineViewSegment::Params& params, const std::string& text, bool allow_undo)
{
	// Save old state
	S32 selection_start = mSelectionStart;
	S32 selection_end = mSelectionEnd;
	BOOL was_selecting = mIsSelecting;
	S32 cursor_pos = mCursorPos;
	S32 old_length = getLength();
	BOOL cursor_was_at_end = (mCursorPos == old_length);

	deselect();

	setCursorPos(old_length);

	LLWString widget_wide_text = utf8str_to_wstring(text);

	LLTextSegmentPtr segment = new LLInlineViewSegment(params, old_length, old_length + widget_wide_text.size());
	insert(getLength(), widget_wide_text, FALSE, segment);

	// Set the cursor and scroll position
	if( selection_start != selection_end )
	{
		mSelectionStart = selection_start;
		mSelectionEnd = selection_end;

		mIsSelecting = was_selecting;
		setCursorPos(cursor_pos);
	}
	else if( cursor_was_at_end )
	{
		setCursorPos(getLength());
	}
	else
	{
		setCursorPos(cursor_pos);
	}

	if (!allow_undo)
	{
		blockUndo();
	}
}

void LLTextEditor::removeTextFromEnd(S32 num_chars)
{
	if (num_chars <= 0) return;

	remove(getLength() - num_chars, num_chars, FALSE);

	S32 len = getLength();
	setCursorPos (llclamp(mCursorPos, 0, len));
	mSelectionStart = llclamp(mSelectionStart, 0, len);
	mSelectionEnd = llclamp(mSelectionEnd, 0, len);

	needsScroll();
}

//----------------------------------------------------------------------------

void LLTextEditor::makePristine()
{
	mPristineCmd = mLastCmd;
	mBaseDocIsPristine = !mLastCmd;

	// Create a clean partition in the undo stack.  We don't want a single command to extend from
	// the "pre-pristine" state to the "post-pristine" state.
	if( mLastCmd )
	{
		mLastCmd->blockExtensions();
	}
}

BOOL LLTextEditor::isPristine() const
{
	if( mPristineCmd )
	{
		return (mPristineCmd == mLastCmd);
	}
	else
	{
		// No undo stack, so check if the version before and commands were done was the original version
		return !mLastCmd && mBaseDocIsPristine;
	}
}

BOOL LLTextEditor::tryToRevertToPristineState()
{
	if( !isPristine() )
	{
		deselect();
		S32 i = 0;
		while( !isPristine() && canUndo() )
		{
			undo();
			i--;
		}

		while( !isPristine() && canRedo() )
		{
			redo();
			i++;
		}

		if( !isPristine() )
		{
			// failed, so go back to where we started
			while( i > 0 )
			{
				undo();
				i--;
			}
		}
	}

	return isPristine(); // TRUE => success
}


static LLFastTimer::DeclareTimer FTM_SYNTAX_HIGHLIGHTING("Syntax Highlighting");
void LLTextEditor::loadKeywords(const std::string& filename,
								const std::vector<std::string>& funcs,
								const std::vector<std::string>& tooltips,
								const LLColor3& color)
{
	LLFastTimer ft(FTM_SYNTAX_HIGHLIGHTING);
	if(mKeywords.loadFromFile(filename))
	{
		S32 count = llmin(funcs.size(), tooltips.size());
		for(S32 i = 0; i < count; i++)
		{
			std::string name = utf8str_trim(funcs[i]);
			mKeywords.addToken(LLKeywordToken::WORD, name, color, tooltips[i] );
		}
		segment_vec_t segment_list;
		mKeywords.findSegments(&segment_list, getWText(), mDefaultColor.get(), *this);

		mSegments.clear();
		segment_set_t::iterator insert_it = mSegments.begin();
		for (segment_vec_t::iterator list_it = segment_list.begin(); list_it != segment_list.end(); ++list_it)
		{
			insert_it = mSegments.insert(insert_it, *list_it);
		}
	}
}

void LLTextEditor::updateSegments()
{
	if (mReflowIndex < S32_MAX && mKeywords.isLoaded() && mParseOnTheFly)
	{
		LLFastTimer ft(FTM_SYNTAX_HIGHLIGHTING);
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

void LLTextEditor::updateLinkSegments()
{
	LLWString wtext = getWText();

	// update any segments that contain a link
	for (segment_set_t::iterator it = mSegments.begin(); it != mSegments.end(); ++it)
	{
		LLTextSegment *segment = *it;
		if (segment && segment->getStyle() && segment->getStyle()->isLink())
		{
			// if the link's label (what the user can edit) is a valid Url,
			// then update the link's HREF to be the same as the label text.
			// This lets users edit Urls in-place.
			LLStyleConstSP style = segment->getStyle();
			LLStyleSP new_style(new LLStyle(*style));
			LLWString url_label = wtext.substr(segment->getStart(), segment->getEnd()-segment->getStart());
			if (LLUrlRegistry::instance().hasUrl(url_label))
			{
				std::string new_url = wstring_to_utf8str(url_label);
				LLStringUtil::trim(new_url);
				new_style->setLinkHREF(new_url);
				LLStyleConstSP sp(new_style);
				segment->setStyle(sp);
			}
		}
	}
}



void LLTextEditor::onMouseCaptureLost()
{
	endSelection();
}

///////////////////////////////////////////////////////////////////
// Hack for Notecards

BOOL LLTextEditor::importBuffer(const char* buffer, S32 length )
{
	std::istringstream instream(buffer);
	
	// Version 1 format:
	//		Linden text version 1\n
	//		{\n
	//			<EmbeddedItemList chunk>
	//			Text length <bytes without \0>\n
	//			<text without \0> (text may contain ext_char_values)
	//		}\n

	char tbuf[MAX_STRING];	/* Flawfinder: ignore */
	
	S32 version = 0;
	instream.getline(tbuf, MAX_STRING);
	if( 1 != sscanf(tbuf, "Linden text version %d", &version) )
	{
		llwarns << "Invalid Linden text file header " << llendl;
		return FALSE;
	}

	if( 1 != version )
	{
		llwarns << "Invalid Linden text file version: " << version << llendl;
		return FALSE;
	}

	instream.getline(tbuf, MAX_STRING);
	if( 0 != sscanf(tbuf, "{") )
	{
		llwarns << "Invalid Linden text file format" << llendl;
		return FALSE;
	}

	S32 text_len = 0;
	instream.getline(tbuf, MAX_STRING);
	if( 1 != sscanf(tbuf, "Text length %d", &text_len) )
	{
		llwarns << "Invalid Linden text length field" << llendl;
		return FALSE;
	}

	if( text_len > mMaxTextByteLength )
	{
		llwarns << "Invalid Linden text length: " << text_len << llendl;
		return FALSE;
	}

	BOOL success = TRUE;

	char* text = new char[ text_len + 1];
	if (text == NULL)
	{
		llerrs << "Memory allocation failure." << llendl;			
		return FALSE;
	}
	instream.get(text, text_len + 1, '\0');
	text[text_len] = '\0';
	if( text_len != (S32)strlen(text) )/* Flawfinder: ignore */
	{
		llwarns << llformat("Invalid text length: %d != %d ",strlen(text),text_len) << llendl;/* Flawfinder: ignore */
		success = FALSE;
	}

	instream.getline(tbuf, MAX_STRING);
	if( success && (0 != sscanf(tbuf, "}")) )
	{
		llwarns << "Invalid Linden text file format: missing terminal }" << llendl;
		success = FALSE;
	}

	if( success )
	{
		// Actually set the text
		setText( LLStringExplicit(text) );
	}

	delete[] text;

	startOfDoc();
	deselect();

	return success;
}

BOOL LLTextEditor::exportBuffer(std::string &buffer )
{
	std::ostringstream outstream(buffer);
	
	outstream << "Linden text version 1\n";
	outstream << "{\n";

	outstream << llformat("Text length %d\n", getLength() );
	outstream << getText();
	outstream << "}\n";

	return TRUE;
}

void LLTextEditor::updateAllowingLanguageInput()
{
	LLWindow* window = getWindow();
	if (!window)
	{
		// test app, no window available
		return;	
	}
	if (hasFocus() && !mReadOnly)
	{
		window->allowLanguageTextInput(this, TRUE);
	}
	else
	{
		window->allowLanguageTextInput(this, FALSE);
	}
}

// Preedit is managed off the undo/redo command stack.

BOOL LLTextEditor::hasPreeditString() const
{
	return (mPreeditPositions.size() > 1);
}

void LLTextEditor::resetPreedit()
{
	if (hasPreeditString())
	{
		if (hasSelection())
		{
			llwarns << "Preedit and selection!" << llendl;
			deselect();
		}

		setCursorPos(mPreeditPositions.front());
		removeStringNoUndo(mCursorPos, mPreeditPositions.back() - mCursorPos);
		insertStringNoUndo(mCursorPos, mPreeditOverwrittenWString);

		mPreeditWString.clear();
		mPreeditOverwrittenWString.clear();
		mPreeditPositions.clear();

		// A call to updatePreedit should soon follow under a
		// normal course of operation, so we don't need to 
		// maintain internal variables such as line start 
		// positions now.
	}
}

void LLTextEditor::updatePreedit(const LLWString &preedit_string,
		const segment_lengths_t &preedit_segment_lengths, const standouts_t &preedit_standouts, S32 caret_position)
{
	// Just in case.
	if (mReadOnly)
	{
		return;
	}

	getWindow()->hideCursorUntilMouseMove();

	S32 insert_preedit_at = mCursorPos;

	mPreeditWString = preedit_string;
	mPreeditPositions.resize(preedit_segment_lengths.size() + 1);
	S32 position = insert_preedit_at;
	for (segment_lengths_t::size_type i = 0; i < preedit_segment_lengths.size(); i++)
	{
		mPreeditPositions[i] = position;
		position += preedit_segment_lengths[i];
	}
	mPreeditPositions.back() = position;

	if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode())
	{
		mPreeditOverwrittenWString = getWText().substr(insert_preedit_at, mPreeditWString.length());
		removeStringNoUndo(insert_preedit_at, mPreeditWString.length());
	}
	else
	{
		mPreeditOverwrittenWString.clear();
	}
	insertStringNoUndo(insert_preedit_at, mPreeditWString);

	mPreeditStandouts = preedit_standouts;

	setCursorPos(insert_preedit_at + caret_position);

	// Update of the preedit should be caused by some key strokes.
	resetCursorBlink();

	onKeyStroke();
}

BOOL LLTextEditor::getPreeditLocation(S32 query_offset, LLCoordGL *coord, LLRect *bounds, LLRect *control) const
{
	if (control)
	{
		LLRect control_rect_screen;
		localRectToScreen(mVisibleTextRect, &control_rect_screen);
		LLUI::screenRectToGL(control_rect_screen, control);
	}

	S32 preedit_left_position, preedit_right_position;
	if (hasPreeditString())
	{
		preedit_left_position = mPreeditPositions.front();
		preedit_right_position = mPreeditPositions.back();
	}
	else
	{
		preedit_left_position = preedit_right_position = mCursorPos;
	}

	const S32 query = (query_offset >= 0 ? preedit_left_position + query_offset : mCursorPos);
	if (query < preedit_left_position || query > preedit_right_position)
	{
		return FALSE;
	}

	const S32 first_visible_line = getFirstVisibleLine();
	if (query < getLineStart(first_visible_line))
	{
		return FALSE;
	}

	S32 current_line = first_visible_line;
	S32 current_line_start, current_line_end;
	for (;;)
	{
		current_line_start = getLineStart(current_line);
		current_line_end = getLineStart(current_line + 1);
		if (query >= current_line_start && query < current_line_end)
		{
			break;
		}
		if (current_line_start == current_line_end)
		{
			// We have reached on the last line.  The query position must be here.
			break;
		}
		current_line++;
	}

    const LLWString textString(getWText());
	const llwchar * const text = textString.c_str();
	const S32 line_height = mDefaultFont->getLineHeight();

	if (coord)
	{
		const S32 query_x = mVisibleTextRect.mLeft + mDefaultFont->getWidth(text, current_line_start, query - current_line_start);
		const S32 query_y = mVisibleTextRect.mTop - (current_line - first_visible_line) * line_height - line_height / 2;
		S32 query_screen_x, query_screen_y;
		localPointToScreen(query_x, query_y, &query_screen_x, &query_screen_y);
		LLUI::screenPointToGL(query_screen_x, query_screen_y, &coord->mX, &coord->mY);
	}

	if (bounds)
	{
		S32 preedit_left = mVisibleTextRect.mLeft;
		if (preedit_left_position > current_line_start)
		{
			preedit_left += mDefaultFont->getWidth(text, current_line_start, preedit_left_position - current_line_start);
		}

		S32 preedit_right = mVisibleTextRect.mLeft;
		if (preedit_right_position < current_line_end)
		{
			preedit_right += mDefaultFont->getWidth(text, current_line_start, preedit_right_position - current_line_start);
		}
		else
		{
			preedit_right += mDefaultFont->getWidth(text, current_line_start, current_line_end - current_line_start);
		}

		const S32 preedit_top = mVisibleTextRect.mTop - (current_line - first_visible_line) * line_height;
		const S32 preedit_bottom = preedit_top - line_height;

		const LLRect preedit_rect_local(preedit_left, preedit_top, preedit_right, preedit_bottom);
		LLRect preedit_rect_screen;
		localRectToScreen(preedit_rect_local, &preedit_rect_screen);
		LLUI::screenRectToGL(preedit_rect_screen, bounds);
	}

	return TRUE;
}

void LLTextEditor::getSelectionRange(S32 *position, S32 *length) const
{
	if (hasSelection())
	{
		*position = llmin(mSelectionStart, mSelectionEnd);
		*length = llabs(mSelectionStart - mSelectionEnd);
	}
	else
	{
		*position = mCursorPos;
		*length = 0;
	}
}

void LLTextEditor::getPreeditRange(S32 *position, S32 *length) const
{
	if (hasPreeditString())
	{
		*position = mPreeditPositions.front();
		*length = mPreeditPositions.back() - mPreeditPositions.front();
	}
	else
	{
		*position = mCursorPos;
		*length = 0;
	}
}

void LLTextEditor::markAsPreedit(S32 position, S32 length)
{
	deselect();
	setCursorPos(position);
	if (hasPreeditString())
	{
		llwarns << "markAsPreedit invoked when hasPreeditString is true." << llendl;
	}
	mPreeditWString = LLWString( getWText(), position, length );
	if (length > 0)
	{
		mPreeditPositions.resize(2);
		mPreeditPositions[0] = position;
		mPreeditPositions[1] = position + length;
		mPreeditStandouts.resize(1);
		mPreeditStandouts[0] = FALSE;
	}
	else
	{
		mPreeditPositions.clear();
		mPreeditStandouts.clear();
	}
	if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode())
	{
		mPreeditOverwrittenWString = mPreeditWString;
	}
	else
	{
		mPreeditOverwrittenWString.clear();
	}
}

S32 LLTextEditor::getPreeditFontSize() const
{
	return llround((F32)mDefaultFont->getLineHeight() * LLUI::getScaleFactor().mV[VY]);
}

BOOL LLTextEditor::isDirty() const
{
	if(mReadOnly)
	{
		return FALSE;
	}

	if( mPristineCmd )
	{
		return ( mPristineCmd == mLastCmd );
	}
	else
	{
		return ( NULL != mLastCmd );
	}
}

void LLTextEditor::setKeystrokeCallback(const keystroke_signal_t::slot_type& callback)
{
	mKeystrokeSignal.connect(callback);
}

void LLTextEditor::onKeyStroke()
{
	mKeystrokeSignal(this);

	mSpellCheckStart = mSpellCheckEnd = -1;
	mSpellCheckTimer.setTimerExpirySec(SPELLCHECK_DELAY);
}

//virtual
void LLTextEditor::clear()
{
	getViewModel()->setDisplay(LLWStringUtil::null);
	clearSegments();
}

bool LLTextEditor::canLoadOrSaveToFile()
{
	return !mReadOnly;
}

S32 LLTextEditor::spacesPerTab()
{
	return SPACES_PER_TAB;
}

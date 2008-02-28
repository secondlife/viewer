/** 
 * @file lllineeditor.cpp
 * @brief LLLineEditor base class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

// Text editor widget to let users enter a single line.

#include "linden_common.h"
 
#include "lllineeditor.h"

#include "audioengine.h"
#include "llmath.h"
#include "llfontgl.h"
#include "llgl.h"
#include "sound_ids.h"
#include "lltimer.h"

//#include "llclipboard.h"
#include "llcontrol.h"
#include "llbutton.h"
#include "llfocusmgr.h"
#include "llkeyboard.h"
#include "llrect.h"
#include "llresmgr.h"
#include "llstring.h"
#include "llwindow.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llclipboard.h"

//
// Imported globals
//

//
// Constants
//

const S32	UI_LINEEDITOR_CURSOR_THICKNESS = 2;
const S32	UI_LINEEDITOR_H_PAD = 2;
const S32	UI_LINEEDITOR_V_PAD = 1;
const F32	CURSOR_FLASH_DELAY = 1.0f;  // in seconds
const S32	SCROLL_INCREMENT_ADD = 0;	// make space for typing
const S32   SCROLL_INCREMENT_DEL = 4;	// make space for baskspacing
const F32   AUTO_SCROLL_TIME = 0.05f;
const F32	LABEL_HPAD = 5.f;

const F32	PREEDIT_MARKER_BRIGHTNESS = 0.4f;
const S32	PREEDIT_MARKER_GAP = 1;
const S32	PREEDIT_MARKER_POSITION = 2;
const S32	PREEDIT_MARKER_THICKNESS = 1;
const F32	PREEDIT_STANDOUT_BRIGHTNESS = 0.6f;
const S32	PREEDIT_STANDOUT_GAP = 1;
const S32	PREEDIT_STANDOUT_POSITION = 2;
const S32	PREEDIT_STANDOUT_THICKNESS = 2;

//
// Member functions
//
 
LLLineEditor::LLLineEditor(const LLString& name, const LLRect& rect,
						   const LLString& default_text, const LLFontGL* font,
						   S32 max_length_bytes,
						   void (*commit_callback)(LLUICtrl* caller, void* user_data ),
						   void (*keystroke_callback)(LLLineEditor* caller, void* user_data ),
						   void (*focus_lost_callback)(LLFocusableElement* caller, void* user_data ),
						   void* userdata,
						   LLLinePrevalidateFunc prevalidate_func,
						   LLViewBorder::EBevel border_bevel,
						   LLViewBorder::EStyle border_style,
						   S32 border_thickness)
	:
		LLUICtrl( name, rect, TRUE, commit_callback, userdata, FOLLOWS_TOP | FOLLOWS_LEFT ),
		mMaxLengthBytes(max_length_bytes),
		mCursorPos( 0 ),
		mScrollHPos( 0 ),
		mBorderLeft(0),
		mBorderRight(0),
		mCommitOnFocusLost( TRUE ),
		mRevertOnEsc( TRUE ),
		mKeystrokeCallback( keystroke_callback ),
		mIsSelecting( FALSE ),
		mSelectionStart( 0 ),
		mSelectionEnd( 0 ),
		mLastSelectionX(-1),
		mLastSelectionY(-1),
		mLastSelectionStart(-1),
		mLastSelectionEnd(-1),
		mPrevalidateFunc( prevalidate_func ),
		mCursorColor(		LLUI::sColorsGroup->getColor( "TextCursorColor" ) ),
		mFgColor(			LLUI::sColorsGroup->getColor( "TextFgColor" ) ),
		mReadOnlyFgColor(	LLUI::sColorsGroup->getColor( "TextFgReadOnlyColor" ) ),
		mTentativeFgColor(	LLUI::sColorsGroup->getColor( "TextFgTentativeColor" ) ),
		mWriteableBgColor(	LLUI::sColorsGroup->getColor( "TextBgWriteableColor" ) ),
		mReadOnlyBgColor(	LLUI::sColorsGroup->getColor( "TextBgReadOnlyColor" ) ),
		mFocusBgColor(		LLUI::sColorsGroup->getColor( "TextBgFocusColor" ) ),
		mBorderThickness( border_thickness ),
		mIgnoreArrowKeys( FALSE ),
		mIgnoreTab( TRUE ),
		mDrawAsterixes( FALSE ),
		mHandleEditKeysDirectly( FALSE ),
		mSelectAllonFocusReceived( FALSE ),
		mPassDelete(FALSE),
		mReadOnly(FALSE)
{
	llassert( max_length_bytes > 0 );

	// line history support:
	// - initialize line history list
	mLineHistory.insert( mLineHistory.end(), "" );
	// - disable line history by default
	mHaveHistory = FALSE;
	// - reset current history line pointer
	mCurrentHistoryLine = 0;

	if (font)
	{
		mGLFont = font;
	}
	else
	{
		mGLFont = LLFontGL::sSansSerifSmall;
	}

	setFocusLostCallback(focus_lost_callback);

	mMinHPixels = mBorderThickness + UI_LINEEDITOR_H_PAD + mBorderLeft;
	mMaxHPixels = getRect().getWidth() - mMinHPixels - mBorderThickness - mBorderRight;

	mScrollTimer.reset();

	setText(default_text);
	
	setCursor(mText.length());

	// Scalable UI somehow made these rectangles off-by-one.
	// I don't know why. JC
	LLRect border_rect(0, getRect().getHeight()-1, getRect().getWidth()-1, 0);
	mBorder = new LLViewBorder( "line ed border", border_rect, border_bevel, border_style, mBorderThickness );
	addChild( mBorder );
	mBorder->setFollows(FOLLOWS_LEFT|FOLLOWS_RIGHT|FOLLOWS_TOP|FOLLOWS_BOTTOM);
}


LLLineEditor::~LLLineEditor()
{
	mCommitOnFocusLost = FALSE;

	gFocusMgr.releaseFocusIfNeeded( this );

	if( gEditMenuHandler == this )
	{
		gEditMenuHandler = NULL;
	}
}


void LLLineEditor::onFocusReceived()
{
	LLUICtrl::onFocusReceived();
	updateAllowingLanguageInput();
}

void LLLineEditor::onFocusLost()
{
	// The call to updateAllowLanguageInput()
	// when loosing the keyboard focus *may*
	// indirectly invoke handleUnicodeCharHere(), 
	// so it must be called before onCommit.
	updateAllowingLanguageInput();

	if( mCommitOnFocusLost && mText.getString() != mPrevText) 
	{
		onCommit();
	}

	if( gEditMenuHandler == this )
	{
		gEditMenuHandler = NULL;
	}

	getWindow()->showCursorFromMouseMove();

	LLUICtrl::onFocusLost();
}

void LLLineEditor::onCommit()
{
	// put current line into the line history
	updateHistory();

	LLUICtrl::onCommit();
	selectAll();
}


// line history support
void LLLineEditor::updateHistory()
{
	// On history enabled line editors, remember committed line and
	// reset current history line number.
	// Be sure only to remember lines that are not empty and that are
	// different from the last on the list.
	if( mHaveHistory && mText.length() && ( mLineHistory.empty() || getText() != mLineHistory.back() ) )
	{
		// discard possible empty line at the end of the history
		// inserted by setText()
		if( !mLineHistory.back().length() )
		{
			mLineHistory.pop_back();
		}
		mLineHistory.insert( mLineHistory.end(), getText() );
		mCurrentHistoryLine = mLineHistory.size() - 1;
	}
}

void LLLineEditor::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLUICtrl::reshape(width, height, called_from_parent );

	mMaxHPixels = getRect().getWidth() - 2 * (mBorderThickness + UI_LINEEDITOR_H_PAD) + 1 - mBorderRight;
}

void LLLineEditor::setEnabled(BOOL enabled)
{
	mReadOnly = !enabled;
	setTabStop(!mReadOnly);
	updateAllowingLanguageInput();
}


void LLLineEditor::setMaxTextLength(S32 max_text_length)
{
	S32 max_len = llmax(0, max_text_length);
	mMaxLengthBytes = max_len;
} 

void LLLineEditor::setBorderWidth(S32 left, S32 right)
{
	mBorderLeft = llclamp(left, 0, getRect().getWidth());
	mBorderRight = llclamp(right, 0, getRect().getWidth());
	mMinHPixels = mBorderThickness + UI_LINEEDITOR_H_PAD + mBorderLeft;
	mMaxHPixels = getRect().getWidth() - mMinHPixels - mBorderThickness - mBorderRight;
}


void LLLineEditor::setText(const LLStringExplicit &new_text)
{
	// If new text is identical, don't copy and don't move insertion point
	if (mText.getString() == new_text)
	{
		return;
	}

	// Check to see if entire field is selected.
	S32 len = mText.length();
	BOOL all_selected = (len > 0)
		&& (( mSelectionStart == 0 && mSelectionEnd == len ) 
			|| ( mSelectionStart == len && mSelectionEnd == 0 ));

	// Do safe truncation so we don't split multi-byte characters
	// also consider entire string selected when mSelectAllonFocusReceived is set on an empty, focused line editor
	all_selected = all_selected || (len == 0 && hasFocus() && mSelectAllonFocusReceived);

	LLString truncated_utf8 = new_text;
	if (truncated_utf8.size() > (U32)mMaxLengthBytes)
	{	
		truncated_utf8 = utf8str_truncate(new_text, mMaxLengthBytes);
	}
	mText.assign(truncated_utf8);

	if (all_selected)
	{
		// ...keep whole thing selected
		selectAll();
	}
	else
	{
		// try to preserve insertion point, but deselect text
		deselect();
	}
	setCursor(llmin((S32)mText.length(), getCursor()));

	// Newly set text goes always in the last line of history.
	// Possible empty strings (as with chat line) will be deleted later.
	mLineHistory.insert( mLineHistory.end(), new_text );
	// Set current history line to end of history.
	mCurrentHistoryLine = mLineHistory.size() - 1;

	mPrevText = mText;
}


// Picks a new cursor position based on the actual screen size of text being drawn.
void LLLineEditor::setCursorAtLocalPos( S32 local_mouse_x )
{
	const llwchar* wtext = mText.getWString().c_str();
	LLWString asterix_text;
	if (mDrawAsterixes)
	{
		for (S32 i = 0; i < mText.length(); i++)
		{
			asterix_text += '*';
		}
		wtext = asterix_text.c_str();
	}

	S32 cursor_pos =
		mScrollHPos + 
		mGLFont->charFromPixelOffset(
			wtext, mScrollHPos,
			(F32)(local_mouse_x - mMinHPixels),
			(F32)(mMaxHPixels - mMinHPixels + 1)); // min-max range is inclusive
	setCursor(cursor_pos);
}

void LLLineEditor::setCursor( S32 pos )
{
	S32 old_cursor_pos = getCursor();
	mCursorPos = llclamp( pos, 0, mText.length());

	S32 pixels_after_scroll = findPixelNearestPos();
	if( pixels_after_scroll > mMaxHPixels )
	{
		S32 width_chars_to_left = mGLFont->getWidth(mText.getWString().c_str(), 0, mScrollHPos);
		S32 last_visible_char = mGLFont->maxDrawableChars(mText.getWString().c_str(), llmax(0.f, (F32)(mMaxHPixels - mMinHPixels + width_chars_to_left))); 
		S32 min_scroll = mGLFont->firstDrawableChar(mText.getWString().c_str(), (F32)(mMaxHPixels - mMinHPixels), mText.length(), getCursor());
		if (old_cursor_pos == last_visible_char)
		{
			mScrollHPos = llmin(mText.length(), llmax(min_scroll, mScrollHPos + SCROLL_INCREMENT_ADD));
		}
		else
		{
			mScrollHPos = min_scroll;
		}
	}
	else if (getCursor() < mScrollHPos)
	{
		if (old_cursor_pos == mScrollHPos)
		{
			mScrollHPos = llmax(0, llmin(getCursor(), mScrollHPos - SCROLL_INCREMENT_DEL));
		}
		else
		{
			mScrollHPos = getCursor();
		}
	}
}


void LLLineEditor::setCursorToEnd()
{
	setCursor(mText.length());
	deselect();
}

BOOL LLLineEditor::canDeselect() const
{
	return hasSelection();
}

void LLLineEditor::deselect()
{
	mSelectionStart = 0;
	mSelectionEnd = 0;
	mIsSelecting = FALSE;
}


void LLLineEditor::startSelection()
{
	mIsSelecting = TRUE;
	mSelectionStart = getCursor();
	mSelectionEnd = getCursor();
}

void LLLineEditor::endSelection()
{
	if( mIsSelecting )
	{
		mIsSelecting = FALSE;
		mSelectionEnd = getCursor();
	}
}

BOOL LLLineEditor::canSelectAll() const
{
	return TRUE;
}

void LLLineEditor::selectAll()
{
	mSelectionStart = mText.length();
	mSelectionEnd = 0;
	setCursor(mSelectionEnd);
	//mScrollHPos = 0;
	mIsSelecting = TRUE;
}


BOOL LLLineEditor::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	setFocus( TRUE );

	if (mSelectionEnd == 0 && mSelectionStart == mText.length())
	{
		// if everything is selected, handle this as a normal click to change insertion point
		handleMouseDown(x, y, mask);
	}
	else
	{
		const LLWString& wtext = mText.getWString();

		BOOL doSelectAll = TRUE;

		// Select the word we're on
		if( isPartOfWord( wtext[mCursorPos] ) )
		{
			S32 old_selection_start = mLastSelectionStart;
			S32 old_selection_end = mLastSelectionEnd;

			// Select word the cursor is over
			while ((mCursorPos > 0) && isPartOfWord( wtext[mCursorPos-1] ))
			{	// Find the start of the word
				mCursorPos--;
			}
			startSelection();	

			while ((mCursorPos < (S32)wtext.length()) && isPartOfWord( wtext[mCursorPos] ) )
			{	// Find the end of the word
				mCursorPos++;
			}
			mSelectionEnd = mCursorPos;

			// If nothing changed, then the word was already selected.  Select the whole line.
			doSelectAll = (old_selection_start == mSelectionStart) &&  
						  (old_selection_end   == mSelectionEnd);
		}
		
		if ( doSelectAll )
		{	// Select everything
			selectAll();
		}
	}

	// We don't want handleMouseUp() to "finish" the selection (and thereby
	// set mSelectionEnd to where the mouse is), so we finish the selection 
	// here.
	mIsSelecting = FALSE;  

	// delay cursor flashing
	mKeystrokeTimer.reset();

	return TRUE;
}

BOOL LLLineEditor::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (x < mBorderLeft || x > (getRect().getWidth() - mBorderRight))
	{
		return LLUICtrl::handleMouseDown(x, y, mask);
	}
	if (mSelectAllonFocusReceived
		&& gFocusMgr.getKeyboardFocus() != this)
	{
		setFocus( TRUE );
	}
	else
	{
		mLastSelectionStart = -1;
		mLastSelectionStart = -1;

		setFocus( TRUE );

		if (mask & MASK_SHIFT)
		{
			// Handle selection extension
			S32 old_cursor_pos = getCursor();
			setCursorAtLocalPos(x);

			if (hasSelection())
			{
				/* Mac-like behavior - extend selection towards the cursor
				if (getCursor() < mSelectionStart
					&& getCursor() < mSelectionEnd)
				{
					// ...left of selection
					mSelectionStart = llmax(mSelectionStart, mSelectionEnd);
					mSelectionEnd = getCursor();
				}
				else if (getCursor() > mSelectionStart
					&& getCursor() > mSelectionEnd)
				{
					// ...right of selection
					mSelectionStart = llmin(mSelectionStart, mSelectionEnd);
					mSelectionEnd = getCursor();
				}
				else
				{
					mSelectionEnd = getCursor();
				}
				*/
				// Windows behavior
				mSelectionEnd = getCursor();
			}
			else
			{
				mSelectionStart = old_cursor_pos;
				mSelectionEnd = getCursor();
			}
			// assume we're starting a drag select
			mIsSelecting = TRUE;
		}
		else
		{
			// Save selection for word/line selecting on double-click
			mLastSelectionStart = mSelectionStart;
			mLastSelectionEnd = mSelectionEnd;

			// Move cursor and deselect for regular click
			setCursorAtLocalPos( x );
			deselect();
			startSelection();
		}

		gFocusMgr.setMouseCapture( this );
	}

	// delay cursor flashing
	mKeystrokeTimer.reset();

	return TRUE;
}


BOOL LLLineEditor::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;
	if (!hasMouseCapture() && (x < mBorderLeft || x > (getRect().getWidth() - mBorderRight)))
	{
		return LLUICtrl::handleHover(x, y, mask);
	}

	if( getVisible() )
	{
		if( (hasMouseCapture()) && mIsSelecting )
		{
			if (x != mLastSelectionX || y != mLastSelectionY)
			{
				mLastSelectionX = x;
				mLastSelectionY = y;
			}
			// Scroll if mouse cursor outside of bounds
			if (mScrollTimer.hasExpired())
			{
				S32 increment = llround(mScrollTimer.getElapsedTimeF32() / AUTO_SCROLL_TIME);
				mScrollTimer.reset();
				mScrollTimer.setTimerExpirySec(AUTO_SCROLL_TIME);
				if( (x < mMinHPixels) && (mScrollHPos > 0 ) )
				{
					// Scroll to the left
					mScrollHPos = llclamp(mScrollHPos - increment, 0, mText.length());
				}
				else
				if( (x > mMaxHPixels) && (mCursorPos < (S32)mText.length()) )
				{
					// If scrolling one pixel would make a difference...
					S32 pixels_after_scrolling_one_char = findPixelNearestPos(1);
					if( pixels_after_scrolling_one_char >= mMaxHPixels )
					{
						// ...scroll to the right
						mScrollHPos = llclamp(mScrollHPos + increment, 0, mText.length());
					}
				}
			}

			setCursorAtLocalPos( x );
			mSelectionEnd = getCursor();

			// delay cursor flashing
			mKeystrokeTimer.reset();

			getWindow()->setCursor(UI_CURSOR_IBEAM);
			lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (active)" << llendl;		
			handled = TRUE;
		}

		if( !handled  )
		{
			getWindow()->setCursor(UI_CURSOR_IBEAM);
			lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (inactive)" << llendl;		
			handled = TRUE;
		}
	}

	return handled;
}


BOOL LLLineEditor::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	if( hasMouseCapture() )
	{
		gFocusMgr.setMouseCapture( NULL );
		handled = TRUE;
	}

	if (!handled && (x < mBorderLeft || x > (getRect().getWidth() - mBorderRight)))
	{
		return LLUICtrl::handleMouseUp(x, y, mask);
	}

	if( mIsSelecting )
	{
		setCursorAtLocalPos( x );
		mSelectionEnd = getCursor();

		handled = TRUE;
	}

	if( handled )
	{
		// delay cursor flashing
		mKeystrokeTimer.reset();
	}

	return handled;
}


// Remove a single character from the text
void LLLineEditor::removeChar()
{
	if( getCursor() > 0 )
	{
		mText.erase(getCursor() - 1, 1);

		setCursor(getCursor() - 1);
	}
	else
	{
		reportBadKeystroke();
	}
}


void LLLineEditor::addChar(const llwchar uni_char)
{
	llwchar new_c = uni_char;
	if (hasSelection())
	{
		deleteSelection();
	}
	else if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode())
	{
		mText.erase(getCursor(), 1);
	}

	S32 cur_bytes = mText.getString().size();
	S32 new_bytes = wchar_utf8_length(new_c);

	BOOL allow_char = TRUE;

	// Check byte length limit
	if ((new_bytes + cur_bytes) > mMaxLengthBytes)
	{
		allow_char = FALSE;
	}

	if (allow_char)
	{
		// Will we need to scroll?
		LLWString w_buf;
		w_buf.assign(1, new_c);

		mText.insert(getCursor(), w_buf);
		setCursor(getCursor() + 1);
	}
	else
	{
		reportBadKeystroke();
	}

	getWindow()->hideCursorUntilMouseMove();
}

// Extends the selection box to the new cursor position
void LLLineEditor::extendSelection( S32 new_cursor_pos )
{
	if( !mIsSelecting )
	{
		startSelection();
	}
	
	setCursor(new_cursor_pos);
	mSelectionEnd = getCursor();
}


void LLLineEditor::setSelection(S32 start, S32 end)
{
	S32 len = mText.length();

	mIsSelecting = TRUE;

	// JC, yes, this seems odd, but I think you have to presume a 
	// selection dragged from the end towards the start.
	mSelectionStart = llclamp(end, 0, len);
	mSelectionEnd = llclamp(start, 0, len);
	setCursor(start);
}

void LLLineEditor::setDrawAsterixes(BOOL b)
{
	mDrawAsterixes = b;
	updateAllowingLanguageInput();
}

S32 LLLineEditor::prevWordPos(S32 cursorPos) const
{
	const LLWString& wtext = mText.getWString();
	while( (cursorPos > 0) && (wtext[cursorPos-1] == ' ') )
	{
		cursorPos--;
	}
	while( (cursorPos > 0) && isPartOfWord( wtext[cursorPos-1] ) )
	{
		cursorPos--;
	}
	return cursorPos;
}

S32 LLLineEditor::nextWordPos(S32 cursorPos) const
{
	const LLWString& wtext = mText.getWString();
	while( (cursorPos < getLength()) && isPartOfWord( wtext[cursorPos] ) )
	{
		cursorPos++;
	} 
	while( (cursorPos < getLength()) && (wtext[cursorPos] == ' ') )
	{
		cursorPos++;
	}
	return cursorPos;
}


BOOL LLLineEditor::handleSelectionKey(KEY key, MASK mask)
{
	BOOL handled = FALSE;

	if( mask & MASK_SHIFT )
	{
		handled = TRUE;

		switch( key )
		{
		case KEY_LEFT:
			if( 0 < getCursor() )
			{
				S32 cursorPos = getCursor() - 1;
				if( mask & MASK_CONTROL )
				{
					cursorPos = prevWordPos(cursorPos);
				}
				extendSelection( cursorPos );
			}
			else
			{
				reportBadKeystroke();
			}
			break;

		case KEY_RIGHT:
			if( getCursor() < mText.length())
			{
				S32 cursorPos = getCursor() + 1;
				if( mask & MASK_CONTROL )
				{
					cursorPos = nextWordPos(cursorPos);
				}
				extendSelection( cursorPos );
			}
			else
			{
				reportBadKeystroke();
			}
			break;

		case KEY_PAGE_UP:
		case KEY_HOME:
			extendSelection( 0 );
			break;
		
		case KEY_PAGE_DOWN:
		case KEY_END:
			{
				S32 len = mText.length();
				if( len )
				{
					extendSelection( len );
				}
				break;
			}

		default:
			handled = FALSE;
			break;
		}
	}

	if (!handled && mHandleEditKeysDirectly)
	{
		if( (MASK_CONTROL & mask) && ('A' == key) )
		{
			if( canSelectAll() )
			{
				selectAll();
			}
			else
			{
				reportBadKeystroke();
			}
			handled = TRUE;
		}
	}


	return handled;
}

void LLLineEditor::deleteSelection()
{
	if( !mReadOnly && hasSelection() )
	{
		S32 left_pos = llmin( mSelectionStart, mSelectionEnd );
		S32 selection_length = abs( mSelectionStart - mSelectionEnd );

		mText.erase(left_pos, selection_length);
		deselect();
		setCursor(left_pos);
	}
}

BOOL LLLineEditor::canCut() const
{
	return !mReadOnly && !mDrawAsterixes && hasSelection();
}

// cut selection to clipboard
void LLLineEditor::cut()
{
	if( canCut() )
	{
		// Prepare for possible rollback
		LLLineEditorRollback rollback( this );


		S32 left_pos = llmin( mSelectionStart, mSelectionEnd );
		S32 length = abs( mSelectionStart - mSelectionEnd );
		gClipboard.copyFromSubstring( mText.getWString(), left_pos, length );
		deleteSelection();

		// Validate new string and rollback the if needed.
		BOOL need_to_rollback = ( mPrevalidateFunc && !mPrevalidateFunc( mText.getWString() ) );
		if( need_to_rollback )
		{
			rollback.doRollback( this );
			reportBadKeystroke();
		}
		else
		if( mKeystrokeCallback )
		{
			mKeystrokeCallback( this, mCallbackUserData );
		}
	}
}

BOOL LLLineEditor::canCopy() const
{
	return !mDrawAsterixes && hasSelection();
}


// copy selection to clipboard
void LLLineEditor::copy()
{
	if( canCopy() )
	{
		S32 left_pos = llmin( mSelectionStart, mSelectionEnd );
		S32 length = abs( mSelectionStart - mSelectionEnd );
		gClipboard.copyFromSubstring( mText.getWString(), left_pos, length );
	}
}

BOOL LLLineEditor::canPaste() const
{
	return !mReadOnly && gClipboard.canPasteString(); 
}


// paste from clipboard
void LLLineEditor::paste()
{
	if (canPaste())
	{
		LLWString paste = gClipboard.getPasteWString();
		if (!paste.empty())
		{
			// Prepare for possible rollback
			LLLineEditorRollback rollback(this);
			
			// Delete any selected characters
			if (hasSelection())
			{
				deleteSelection();
			}

			// Clean up string (replace tabs and returns and remove characters that our fonts don't support.)
			LLWString clean_string(paste);
			LLWString::replaceTabsWithSpaces(clean_string, 1);
			//clean_string = wstring_detabify(paste, 1);
			LLWString::replaceChar(clean_string, '\n', ' ');

			// Insert the string

			// Check to see that the size isn't going to be larger than the max number of bytes
			U32 available_bytes = mMaxLengthBytes - wstring_utf8_length(mText);

			if ( available_bytes < (U32) wstring_utf8_length(clean_string) )
			{	// Doesn't all fit
				llwchar current_symbol = clean_string[0];
				U32 wchars_that_fit = 0;
				U32 total_bytes = wchar_utf8_length(current_symbol);

				//loop over the "wide" characters (symbols)
				//and check to see how large (in bytes) each symbol is.
				while ( total_bytes <= available_bytes )
				{
					//while we still have available bytes
					//"accept" the current symbol and check the size
					//of the next one
					current_symbol = clean_string[++wchars_that_fit];
					total_bytes += wchar_utf8_length(current_symbol);
				}
				// Truncate the clean string at the limit of what will fit
				clean_string = clean_string.substr(0, wchars_that_fit);
				reportBadKeystroke();
			}

			mText.insert(getCursor(), clean_string);
			setCursor( getCursor() + (S32)clean_string.length() );
			deselect();

			// Validate new string and rollback the if needed.
			BOOL need_to_rollback = ( mPrevalidateFunc && !mPrevalidateFunc( mText.getWString() ) );
			if( need_to_rollback )
			{
				rollback.doRollback( this );
				reportBadKeystroke();
			}
			else
			if( mKeystrokeCallback )
			{
				mKeystrokeCallback( this, mCallbackUserData );
			}
		}
	}
}

	
BOOL LLLineEditor::handleSpecialKey(KEY key, MASK mask)	
{
	BOOL handled = FALSE;

	switch( key )
	{
	case KEY_INSERT:
		if (mask == MASK_NONE)
		{
			gKeyboard->toggleInsertMode();
		}

		handled = TRUE;
		break;

	case KEY_BACKSPACE:
		if (!mReadOnly)
		{
			//llinfos << "Handling backspace" << llendl;
			if( hasSelection() )
			{
				deleteSelection();
			}
			else
			if( 0 < getCursor() )
			{
				removeChar();
			}
			else
			{
				reportBadKeystroke();
			}
		}
		handled = TRUE;
		break;

	case KEY_PAGE_UP:
	case KEY_HOME:
		if (!mIgnoreArrowKeys)
		{
			setCursor(0);
			handled = TRUE;
		}
		break;

	case KEY_PAGE_DOWN:
	case KEY_END:
		if (!mIgnoreArrowKeys)
		{
			S32 len = mText.length();
			if( len )
			{
				setCursor(len);
			}
			handled = TRUE;
		}
		break;

	case KEY_LEFT:
		if (mIgnoreArrowKeys && mask == MASK_NONE)
			break;
		if ((mask & MASK_ALT) == 0)
		{
			if( hasSelection() )
			{
				setCursor(llmin( getCursor() - 1, mSelectionStart, mSelectionEnd ));
			}
			else
			if( 0 < getCursor() )
			{
				S32 cursorPos = getCursor() - 1;
				if( mask & MASK_CONTROL )
				{
					cursorPos = prevWordPos(cursorPos);
				}
				setCursor(cursorPos);
			}
			else
			{
				reportBadKeystroke();
			}
			handled = TRUE;
		}
		break;

	case KEY_RIGHT:
		if (mIgnoreArrowKeys && mask == MASK_NONE)
			break;
		if ((mask & MASK_ALT) == 0)
		{
			if (hasSelection())
			{
				setCursor(llmax(getCursor() + 1, mSelectionStart, mSelectionEnd));
			}
			else
			if (getCursor() < mText.length())
			{
				S32 cursorPos = getCursor() + 1;
				if( mask & MASK_CONTROL )
				{
					cursorPos = nextWordPos(cursorPos);
				}
				setCursor(cursorPos);
			}
			else
			{
				reportBadKeystroke();
			}
			handled = TRUE;
		}
		break;

	// handle ctrl-uparrow if we have a history enabled line editor.
	case KEY_UP:
		if( mHaveHistory && ( MASK_CONTROL == mask ) )
		{
			if( mCurrentHistoryLine > 0 )
			{
				mText.assign( mLineHistory[ --mCurrentHistoryLine ] );
				setCursor(llmin((S32)mText.length(), getCursor()));
			}
			else
			{
				reportBadKeystroke();
			}
			handled = TRUE;
		}
		break;

	// handle ctrl-downarrow if we have a history enabled line editor
	case KEY_DOWN:
		if( mHaveHistory  && ( MASK_CONTROL == mask ) )
		{
			if( !mLineHistory.empty() && mCurrentHistoryLine < mLineHistory.size() - 1 )
			{
				mText.assign( mLineHistory[ ++mCurrentHistoryLine ] );
				setCursor(llmin((S32)mText.length(), getCursor()));
			}
			else
			{
				reportBadKeystroke();
			}
			handled = TRUE;
		}
		break;

	case KEY_RETURN:
		// store sent line in history
		updateHistory();
		break;

	case KEY_ESCAPE:
	    if (mRevertOnEsc && mText.getString() != mPrevText)
		{
			setText(mPrevText);
			// Note, don't set handled, still want to loose focus (won't commit becase text is now unchanged)
		}
		break;
		
	default:
		break;
	}

	if( !handled && mHandleEditKeysDirectly )
	{
		// Standard edit keys (Ctrl-X, Delete, etc,) are handled here instead of routed by the menu system.
		if( KEY_DELETE == key )
		{
			if( canDoDelete() )
			{
				doDelete();
			}
			else
			{
				reportBadKeystroke();
			}
			handled = TRUE;
		}
		else
		if( MASK_CONTROL & mask )
		{
			if( 'C' == key )
			{
				if( canCopy() )
				{
					copy();
				}
				else
				{
					reportBadKeystroke();
				}
				handled = TRUE;
			}
			else
			if( 'V' == key )
			{
				if( canPaste() )
				{
					paste();
				}
				else
				{
					reportBadKeystroke();
				}
				handled = TRUE;
			}
			else
			if( 'X' == key )
			{
				if( canCut() )
				{
					cut();
				}
				else
				{
					reportBadKeystroke();
				}
				handled = TRUE;
			}
		}
	}
	return handled;
}


BOOL LLLineEditor::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent )
{
	BOOL	handled = FALSE;
	BOOL	selection_modified = FALSE;

	if ( (gFocusMgr.getKeyboardFocus() == this) && getVisible())
	{
		LLLineEditorRollback rollback( this );

		if( !handled )
		{
			handled = handleSelectionKey( key, mask );
			selection_modified = handled;
		}
		
		// Handle most keys only if the text editor is writeable.
		if ( !mReadOnly )
		{
			if( !handled )
			{
				handled = handleSpecialKey( key, mask );
			}
		}

		if( handled )
		{
			mKeystrokeTimer.reset();

			// Most keystrokes will make the selection box go away, but not all will.
			if( !selection_modified &&
				KEY_SHIFT != key &&
				KEY_CONTROL != key &&
				KEY_ALT != key &&
				KEY_CAPSLOCK )
			{
				deselect();
			}

			BOOL need_to_rollback = FALSE;

			// If read-only, don't allow changes
			need_to_rollback |= (mReadOnly && (mText.getString() == rollback.getText()));

			// Validate new string and rollback the keystroke if needed.
			need_to_rollback |= (mPrevalidateFunc && !mPrevalidateFunc(mText.getWString()));

			if (need_to_rollback)
			{
				rollback.doRollback(this);

				reportBadKeystroke();
			}

			// Notify owner if requested
			if (!need_to_rollback && handled)
			{
				if (mKeystrokeCallback)
				{
					mKeystrokeCallback(this, mCallbackUserData);
				}
			}
		}
	}

	return handled;
}


BOOL LLLineEditor::handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent)
{
	if ((uni_char < 0x20) || (uni_char == 0x7F)) // Control character or DEL
	{
		return FALSE;
	}

	BOOL	handled = FALSE;

	if ( (gFocusMgr.getKeyboardFocus() == this) && getVisible() && !mReadOnly)
	{
		handled = TRUE;

		LLLineEditorRollback rollback( this );

		addChar(uni_char);

		mKeystrokeTimer.reset();

		deselect();

		BOOL need_to_rollback = FALSE;

		// Validate new string and rollback the keystroke if needed.
		need_to_rollback |= ( mPrevalidateFunc && !mPrevalidateFunc( mText.getWString() ) );

		if( need_to_rollback )
		{
			rollback.doRollback( this );

			reportBadKeystroke();
		}

		// Notify owner if requested
		if( !need_to_rollback && handled )
		{
			if( mKeystrokeCallback )
			{
				// HACK! The only usage of this callback doesn't do anything with the character.
				// We'll have to do something about this if something ever changes! - Doug
				mKeystrokeCallback( this, mCallbackUserData );
			}
		}
	}
	return handled;
}


BOOL LLLineEditor::canDoDelete() const
{
	return ( !mReadOnly && (!mPassDelete || (hasSelection() || (getCursor() < mText.length()))) );
}

void LLLineEditor::doDelete()
{
	if (canDoDelete())
	{
		// Prepare for possible rollback
		LLLineEditorRollback rollback( this );

		if (hasSelection())
		{
			deleteSelection();
		}
		else if ( getCursor() < mText.length())
		{	
			setCursor(getCursor() + 1);
			removeChar();
		}

		// Validate new string and rollback the if needed.
		BOOL need_to_rollback = ( mPrevalidateFunc && !mPrevalidateFunc( mText.getWString() ) );
		if( need_to_rollback )
		{
			rollback.doRollback( this );
			reportBadKeystroke();
		}
		else
		{
			if( mKeystrokeCallback )
			{
				mKeystrokeCallback( this, mCallbackUserData );
			}
		}
	}
}


void LLLineEditor::draw()
{
	if( !getVisible() )
	{
		return;
	}

	S32 text_len = mText.length();

	LLString saved_text;
	if (mDrawAsterixes)
	{
		saved_text = mText.getString();
		LLString text;
		for (S32 i = 0; i < mText.length(); i++)
		{
			text += '*';
		}
		mText = text;
	}

	// draw rectangle for the background
	LLRect background( 0, getRect().getHeight(), getRect().getWidth(), 0 );
	background.stretch( -mBorderThickness );

	LLColor4 bg_color = mReadOnlyBgColor;

	// drawing solids requires texturing be disabled
	{
		LLGLSNoTexture no_texture;
		// draw background for text
		if( !mReadOnly )
		{
			if( gFocusMgr.getKeyboardFocus() == this )
			{
				bg_color = mFocusBgColor;
			}
			else
			{
				bg_color = mWriteableBgColor;
			}
		}
		gl_rect_2d(background, bg_color);
	}

	// draw text

	S32 cursor_bottom = background.mBottom + 1;
	S32 cursor_top = background.mTop - 1;

	LLColor4 text_color;
	if (!mReadOnly)
	{
		if (!getTentative())
		{
			text_color = mFgColor;
		}
		else
		{
			text_color = mTentativeFgColor;
		}
	}
	else
	{
		text_color = mReadOnlyFgColor;
	}
	LLColor4 label_color = mTentativeFgColor;

	if (hasPreeditString())
	{
		// Draw preedit markers.  This needs to be before drawing letters.
		for (U32 i = 0; i < mPreeditStandouts.size(); i++)
		{
			const S32 preedit_left = mPreeditPositions[i];
			const S32 preedit_right = mPreeditPositions[i + 1];
			if (preedit_right > mScrollHPos)
			{
				S32 preedit_pixels_left = findPixelNearestPos(llmax(preedit_left, mScrollHPos) - getCursor());
				S32 preedit_pixels_right = llmin(findPixelNearestPos(preedit_right - getCursor()), background.mRight);
				if (preedit_pixels_left >= background.mRight)
				{
					break;
				}
				if (mPreeditStandouts[i])
				{
					gl_rect_2d(preedit_pixels_left + PREEDIT_STANDOUT_GAP,
						background.mBottom + PREEDIT_STANDOUT_POSITION,
						preedit_pixels_right - PREEDIT_STANDOUT_GAP - 1,
						background.mBottom + PREEDIT_STANDOUT_POSITION - PREEDIT_STANDOUT_THICKNESS,
						(text_color * PREEDIT_STANDOUT_BRIGHTNESS + bg_color * (1 - PREEDIT_STANDOUT_BRIGHTNESS)).setAlpha(1.0f));
				}
				else
				{
					gl_rect_2d(preedit_pixels_left + PREEDIT_MARKER_GAP,
						background.mBottom + PREEDIT_MARKER_POSITION,
						preedit_pixels_right - PREEDIT_MARKER_GAP - 1,
						background.mBottom + PREEDIT_MARKER_POSITION - PREEDIT_MARKER_THICKNESS,
						(text_color * PREEDIT_MARKER_BRIGHTNESS + bg_color * (1 - PREEDIT_MARKER_BRIGHTNESS)).setAlpha(1.0f));
				}
			}
		}
	}

	S32 rendered_text = 0;
	F32 rendered_pixels_right = (F32)mMinHPixels;
	F32 text_bottom = (F32)background.mBottom + (F32)UI_LINEEDITOR_V_PAD;

	if( (gFocusMgr.getKeyboardFocus() == this) && hasSelection() )
	{
		S32 select_left;
		S32 select_right;
		if( mSelectionStart < getCursor() )
		{
			select_left = mSelectionStart;
			select_right = getCursor();
		}
		else
		{
			select_left = getCursor();
			select_right = mSelectionStart;
		}
		
		if( select_left > mScrollHPos )
		{
			// unselected, left side
			rendered_text = mGLFont->render( 
				mText, mScrollHPos,
				rendered_pixels_right, text_bottom,
				text_color,
				LLFontGL::LEFT, LLFontGL::BOTTOM,
				LLFontGL::NORMAL,
				select_left - mScrollHPos,
				mMaxHPixels - llround(rendered_pixels_right),
				&rendered_pixels_right);
		}
		
		if( (rendered_pixels_right < (F32)mMaxHPixels) && (rendered_text < text_len) )
		{
			LLColor4 color(1.f - bg_color.mV[0], 1.f - bg_color.mV[1], 1.f - bg_color.mV[2], 1.f);
			// selected middle
			S32 width = mGLFont->getWidth(mText.getWString().c_str(), mScrollHPos + rendered_text, select_right - mScrollHPos - rendered_text);
			width = llmin(width, mMaxHPixels - llround(rendered_pixels_right));
			gl_rect_2d(llround(rendered_pixels_right), cursor_top, llround(rendered_pixels_right)+width, cursor_bottom, color);

			rendered_text += mGLFont->render( 
				mText, mScrollHPos + rendered_text,
				rendered_pixels_right, text_bottom,
				LLColor4( 1.f - text_color.mV[0], 1.f - text_color.mV[1], 1.f - text_color.mV[2], 1 ),
				LLFontGL::LEFT, LLFontGL::BOTTOM,
				LLFontGL::NORMAL,
				select_right - mScrollHPos - rendered_text,
				mMaxHPixels - llround(rendered_pixels_right),
				&rendered_pixels_right);
		}

		if( (rendered_pixels_right < (F32)mMaxHPixels) && (rendered_text < text_len) )
		{
			// unselected, right side
			mGLFont->render( 
				mText, mScrollHPos + rendered_text,
				rendered_pixels_right, text_bottom,
				text_color,
				LLFontGL::LEFT, LLFontGL::BOTTOM,
				LLFontGL::NORMAL,
				S32_MAX,
				mMaxHPixels - llround(rendered_pixels_right),
				&rendered_pixels_right);
		}
	}
	else
	{
		mGLFont->render( 
			mText, mScrollHPos, 
			rendered_pixels_right, text_bottom,
			text_color,
			LLFontGL::LEFT, LLFontGL::BOTTOM,
			LLFontGL::NORMAL,
			S32_MAX,
			mMaxHPixels - llround(rendered_pixels_right),
			&rendered_pixels_right);
	}

	// If we're editing...
	if( gFocusMgr.getKeyboardFocus() == this)
	{
		// (Flash the cursor every half second)
		if (gShowTextEditCursor && !mReadOnly)
		{
			F32 elapsed = mKeystrokeTimer.getElapsedTimeF32();
			if( (elapsed < CURSOR_FLASH_DELAY ) || (S32(elapsed * 2) & 1) )
			{
				S32 cursor_left = findPixelNearestPos();
				cursor_left -= UI_LINEEDITOR_CURSOR_THICKNESS / 2;
				S32 cursor_right = cursor_left + UI_LINEEDITOR_CURSOR_THICKNESS;
				if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode() && !hasSelection())
				{
					const LLWString space(utf8str_to_wstring(LLString(" ")));
					S32 wswidth = mGLFont->getWidth(space.c_str());
					S32 width = mGLFont->getWidth(mText.getWString().c_str(), getCursor(), 1) + 1;
					cursor_right = cursor_left + llmax(wswidth, width);
				}
				// Use same color as text for the Cursor
				gl_rect_2d(cursor_left, cursor_top,
					cursor_right, cursor_bottom, text_color);
				if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode() && !hasSelection())
				{
					mGLFont->render(mText, getCursor(), (F32)(cursor_left + UI_LINEEDITOR_CURSOR_THICKNESS / 2), text_bottom, 
						LLColor4( 1.f - text_color.mV[0], 1.f - text_color.mV[1], 1.f - text_color.mV[2], 1 ),
						LLFontGL::LEFT, LLFontGL::BOTTOM,
						LLFontGL::NORMAL,
						1);
				}

				// Make sure the IME is in the right place
				S32 pixels_after_scroll = findPixelNearestPos();	// RCalculcate for IME position
				LLRect screen_pos = getScreenRect();
				LLCoordGL ime_pos( screen_pos.mLeft + pixels_after_scroll, screen_pos.mTop - UI_LINEEDITOR_V_PAD );

				ime_pos.mX = (S32) (ime_pos.mX * LLUI::sGLScaleFactor.mV[VX]);
				ime_pos.mY = (S32) (ime_pos.mY * LLUI::sGLScaleFactor.mV[VY]);
				getWindow()->setLanguageTextInput( ime_pos );
			}
		}

		// Draw children (border)
		//mBorder->setVisible(TRUE);
		mBorder->setKeyboardFocusHighlight( TRUE );
		LLView::draw();
		mBorder->setKeyboardFocusHighlight( FALSE );
		//mBorder->setVisible(FALSE);
	}
	else // does not have keyboard input
	{
		// draw label if no text provided
		if (0 == mText.length())
		{
			mGLFont->render(mLabel.getWString(), 0, 
							LABEL_HPAD, (F32)text_bottom,
							label_color,
							LLFontGL::LEFT, LLFontGL::BOTTOM,
							LLFontGL::NORMAL,
							S32_MAX,
							mMaxHPixels - llround(rendered_pixels_right),
							&rendered_pixels_right, FALSE);
		}
		// Draw children (border)
		LLView::draw();
	}

	if (mDrawAsterixes)
	{
		mText = saved_text;
	}
}


// Returns the local screen space X coordinate associated with the text cursor position.
S32 LLLineEditor::findPixelNearestPos(const S32 cursor_offset) const
{
	S32 dpos = getCursor() - mScrollHPos + cursor_offset;
	S32 result = mGLFont->getWidth(mText.getWString().c_str(), mScrollHPos, dpos) + mMinHPixels;
	return result;
}

void LLLineEditor::reportBadKeystroke()
{
	make_ui_sound("UISndBadKeystroke");
}

//virtual
void LLLineEditor::clear()
{
	mText.clear();
	setCursor(0);
}

//virtual
void LLLineEditor::onTabInto()
{
	selectAll();
}

//virtual
BOOL LLLineEditor::acceptsTextInput() const
{
	return TRUE;
}

// Start or stop the editor from accepting text-editing keystrokes
void LLLineEditor::setFocus( BOOL new_state )
{
	BOOL old_state = hasFocus();

	if (!new_state)
	{
		getWindow()->allowLanguageTextInput(this, FALSE);
	}


	// getting focus when we didn't have it before, and we want to select all
	if (!old_state && new_state && mSelectAllonFocusReceived)
	{
		selectAll();
		// We don't want handleMouseUp() to "finish" the selection (and thereby
		// set mSelectionEnd to where the mouse is), so we finish the selection 
		// here.
		mIsSelecting = FALSE;
	}

	if( new_state )
	{
		gEditMenuHandler = this;

		// Don't start the cursor flashing right away
		mKeystrokeTimer.reset();
	}
	else
	{
		// Not really needed, since loss of keyboard focus should take care of this,
		// but limited paranoia is ok.
		if( gEditMenuHandler == this )
		{
			gEditMenuHandler = NULL;
		}

		endSelection();
	}

	LLUICtrl::setFocus( new_state );

	if (new_state)
	{
		// Allow Language Text Input only when this LineEditor has
		// no prevalidate function attached.  This criterion works
		// fine on 1.15.0.2, since all prevalidate func reject any
		// non-ASCII characters.  I'm not sure on future versions,
		// however.
		getWindow()->allowLanguageTextInput(this, mPrevalidateFunc == NULL);
	}
}

//virtual 
void LLLineEditor::setRect(const LLRect& rect)
{
	LLUICtrl::setRect(rect);
	if (mBorder)
	{
		LLRect border_rect = mBorder->getRect();
		// Scalable UI somehow made these rectangles off-by-one.
		// I don't know why. JC
		border_rect.setOriginAndSize(border_rect.mLeft, border_rect.mBottom, 
				rect.getWidth()-1, rect.getHeight()-1);
		mBorder->setRect(border_rect);
	}
}

void LLLineEditor::setPrevalidate(BOOL (*func)(const LLWString &))
{
	mPrevalidateFunc = func;
	updateAllowingLanguageInput();
}

// Limits what characters can be used to [1234567890.-] with [-] only valid in the first position.
// Does NOT ensure that the string is a well-formed number--that's the job of post-validation--for
// the simple reasons that intermediate states may be invalid even if the final result is valid.
// 
// static
BOOL LLLineEditor::prevalidateFloat(const LLWString &str)
{
	LLLocale locale(LLLocale::USER_LOCALE);

	BOOL success = TRUE;
	LLWString trimmed = str;
	LLWString::trim(trimmed);
	S32 len = trimmed.length();
	if( 0 < len )
	{
		// May be a comma or period, depending on the locale
		llwchar decimal_point = (llwchar)gResMgr->getDecimalPoint();

		S32 i = 0;

		// First character can be a negative sign
		if( '-' == trimmed[0] )
		{
			i++;
		}

		for( ; i < len; i++ )
		{
			if( (decimal_point != trimmed[i] ) && !LLStringOps::isDigit( trimmed[i] ) )
			{
				success = FALSE;
				break;
			}
		}
	}		

	return success;
}

//static
BOOL LLLineEditor::isPartOfWord(llwchar c) { return (c == '_') || isalnum(c); }

// static
BOOL LLLineEditor::postvalidateFloat(const LLString &str)
{
	LLLocale locale(LLLocale::USER_LOCALE);

	BOOL success = TRUE;
	BOOL has_decimal = FALSE;
	BOOL has_digit = FALSE;

	LLWString trimmed = utf8str_to_wstring(str);
	LLWString::trim(trimmed);
	S32 len = trimmed.length();
	if( 0 < len )
	{
		S32 i = 0;

		// First character can be a negative sign
		if( '-' == trimmed[0] )
		{
			i++;
		}

		// May be a comma or period, depending on the locale
		llwchar decimal_point = (llwchar)gResMgr->getDecimalPoint();

		for( ; i < len; i++ )
		{
			if( decimal_point == trimmed[i] )
			{
				if( has_decimal )
				{
					// can't have two
					success = FALSE;
					break;
				}
				else
				{
					has_decimal = TRUE;
				}
			}
			else
			if( LLStringOps::isDigit( trimmed[i] ) )
			{
				has_digit = TRUE;
			}
			else
			{
				success = FALSE;
				break;
			}
		}
	}		

	// Gotta have at least one
	success = has_digit;

	return success;
}

// Limits what characters can be used to [1234567890-] with [-] only valid in the first position.
// Does NOT ensure that the string is a well-formed number--that's the job of post-validation--for
// the simple reasons that intermediate states may be invalid even if the final result is valid.
//
// static
BOOL LLLineEditor::prevalidateInt(const LLWString &str)
{
	LLLocale locale(LLLocale::USER_LOCALE);

	BOOL success = TRUE;
	LLWString trimmed = str;
	LLWString::trim(trimmed);
	S32 len = trimmed.length();
	if( 0 < len )
	{
		S32 i = 0;

		// First character can be a negative sign
		if( '-' == trimmed[0] )
		{
			i++;
		}

		for( ; i < len; i++ )
		{
			if( !LLStringOps::isDigit( trimmed[i] ) )
			{
				success = FALSE;
				break;
			}
		}
	}		

	return success;
}

// static
BOOL LLLineEditor::prevalidatePositiveS32(const LLWString &str)
{
	LLLocale locale(LLLocale::USER_LOCALE);

	LLWString trimmed = str;
	LLWString::trim(trimmed);
	S32 len = trimmed.length();
	BOOL success = TRUE;
	if(0 < len)
	{
		if(('-' == trimmed[0]) || ('0' == trimmed[0]))
		{
			success = FALSE;
		}
		S32 i = 0;
		while(success && (i < len))
		{
			if(!LLStringOps::isDigit(trimmed[i++]))
			{
				success = FALSE;
			}
		}
	}
	if (success)
	{
		S32 val = strtol(wstring_to_utf8str(trimmed).c_str(), NULL, 10);
		if (val <= 0)
		{
			success = FALSE;
		}
	}
	return success;
}

BOOL LLLineEditor::prevalidateNonNegativeS32(const LLWString &str)
{
	LLLocale locale(LLLocale::USER_LOCALE);

	LLWString trimmed = str;
	LLWString::trim(trimmed);
	S32 len = trimmed.length();
	BOOL success = TRUE;
	if(0 < len)
	{
		if('-' == trimmed[0])
		{
			success = FALSE;
		}
		S32 i = 0;
		while(success && (i < len))
		{
			if(!LLStringOps::isDigit(trimmed[i++]))
			{
				success = FALSE;
			}
		}
	}
	if (success)
	{
		S32 val = strtol(wstring_to_utf8str(trimmed).c_str(), NULL, 10);
		if (val < 0)
		{
			success = FALSE;
		}
	}
	return success;
}

BOOL LLLineEditor::prevalidateAlphaNum(const LLWString &str)
{
	LLLocale locale(LLLocale::USER_LOCALE);

	BOOL rv = TRUE;
	S32 len = str.length();
	if(len == 0) return rv;
	while(len--)
	{
		if( !isalnum(str[len]) )
		{
			rv = FALSE;
			break;
		}
	}
	return rv;
}

// static
BOOL LLLineEditor::prevalidateAlphaNumSpace(const LLWString &str)
{
	LLLocale locale(LLLocale::USER_LOCALE);

	BOOL rv = TRUE;
	S32 len = str.length();
	if(len == 0) return rv;
	while(len--)
	{
		if(!(isalnum(str[len]) || (' ' == str[len])))
		{
			rv = FALSE;
			break;
		}
	}
	return rv;
}

// static
BOOL LLLineEditor::prevalidatePrintableNotPipe(const LLWString &str)
{
	BOOL rv = TRUE;
	S32 len = str.length();
	if(len == 0) return rv;
	while(len--)
	{
		if('|' == str[len])
		{
			rv = FALSE;
			break;
		}
		if(!((' ' == str[len]) || isalnum(str[len]) || ispunct(str[len])))
		{
			rv = FALSE;
			break;
		}
	}
	return rv;
}


// static
BOOL LLLineEditor::prevalidatePrintableNoSpace(const LLWString &str)
{
	BOOL rv = TRUE;
	S32 len = str.length();
	if(len == 0) return rv;
	while(len--)
	{
		if(iswspace(str[len]))
		{
			rv = FALSE;
			break;
		}
		if( !(isalnum(str[len]) || ispunct(str[len]) ) )
		{
			rv = FALSE;
			break;
		}
	}
	return rv;
}

// static
BOOL LLLineEditor::prevalidateASCII(const LLWString &str)
{
	BOOL rv = TRUE;
	S32 len = str.length();
	while(len--)
	{
		if (str[len] < 0x20 || str[len] > 0x7f)
		{
			rv = FALSE;
			break;
		}
	}
	return rv;
}

void LLLineEditor::onMouseCaptureLost()
{
	endSelection();
}


void LLLineEditor::setSelectAllonFocusReceived(BOOL b)
{
	mSelectAllonFocusReceived = b;
}


void LLLineEditor::setKeystrokeCallback(void (*keystroke_callback)(LLLineEditor* caller, void* user_data))
{
	mKeystrokeCallback = keystroke_callback;
}

// virtual
LLXMLNodePtr LLLineEditor::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	node->createChild("max_length", TRUE)->setIntValue(mMaxLengthBytes);

	node->createChild("font", TRUE)->setStringValue(LLFontGL::nameFromFont(mGLFont));

	if (mBorder)
	{
		LLString bevel;
		switch(mBorder->getBevel())
		{
		default:
		case LLViewBorder::BEVEL_NONE:	bevel = "none"; break;
		case LLViewBorder::BEVEL_IN:	bevel = "in"; break;
		case LLViewBorder::BEVEL_OUT:	bevel = "out"; break;
		case LLViewBorder::BEVEL_BRIGHT:bevel = "bright"; break;
		}
		node->createChild("bevel_style", TRUE)->setStringValue(bevel);

		LLString style;
		switch(mBorder->getStyle())
		{
		default:
		case LLViewBorder::STYLE_LINE:		style = "line"; break;
		case LLViewBorder::STYLE_TEXTURE:	style = "texture"; break;
		}
		node->createChild("border_style", TRUE)->setStringValue(style);

		node->createChild("border_thickness", TRUE)->setIntValue(mBorder->getBorderWidth());
	}

	if (!mLabel.empty())
	{
		node->createChild("label", TRUE)->setStringValue(mLabel.getString());
	}

	node->createChild("select_all_on_focus_received", TRUE)->setBoolValue(mSelectAllonFocusReceived);

	node->createChild("handle_edit_keys_directly", TRUE)->setBoolValue(mHandleEditKeysDirectly );

	addColorXML(node, mCursorColor, "cursor_color", "TextCursorColor");
	addColorXML(node, mFgColor, "text_color", "TextFgColor");
	addColorXML(node, mReadOnlyFgColor, "text_readonly_color", "TextFgReadOnlyColor");
	addColorXML(node, mTentativeFgColor, "text_tentative_color", "TextFgTentativeColor");
	addColorXML(node, mReadOnlyBgColor, "bg_readonly_color", "TextBgReadOnlyColor");
	addColorXML(node, mWriteableBgColor, "bg_writeable_color", "TextBgWriteableColor");
	addColorXML(node, mFocusBgColor, "bg_focus_color", "TextBgFocusColor");

	node->createChild("select_on_focus", TRUE)->setBoolValue(mSelectAllonFocusReceived );

	return node;
}

// static
LLView* LLLineEditor::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("line_editor");
	node->getAttributeString("name", name);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	S32 max_text_length = 128;
	node->getAttributeS32("max_length", max_text_length);

	LLFontGL* font = LLView::selectFont(node);

	LLString text = node->getTextContents().substr(0, max_text_length - 1);

	LLViewBorder::EBevel bevel_style = LLViewBorder::BEVEL_IN;
	LLViewBorder::getBevelFromAttribute(node, bevel_style);
	
	LLViewBorder::EStyle border_style = LLViewBorder::STYLE_LINE;
	LLString border_string;
	node->getAttributeString("border_style", border_string);
	LLString::toLower(border_string);

	if (border_string == "texture")
	{
		border_style = LLViewBorder::STYLE_TEXTURE;
	}

	S32 border_thickness = 1;
	node->getAttributeS32("border_thickness", border_thickness);

	LLUICtrlCallback commit_callback = NULL;

	LLLineEditor* line_editor = new LLLineEditor(name,
								rect, 
								text, 
								font,
								max_text_length,
								commit_callback,
								NULL,
								NULL,
								NULL,
								NULL,
								bevel_style,
								border_style,
								border_thickness);

	LLString label;
	if(node->getAttributeString("label", label))
	{
		line_editor->setLabel(label);
	}
	BOOL select_all_on_focus_received = FALSE;
	if (node->getAttributeBOOL("select_all_on_focus_received", select_all_on_focus_received))
	{
		line_editor->setSelectAllonFocusReceived(select_all_on_focus_received);
	}
	BOOL handle_edit_keys_directly = FALSE;
	if (node->getAttributeBOOL("handle_edit_keys_directly", handle_edit_keys_directly))
	{
		line_editor->setHandleEditKeysDirectly(handle_edit_keys_directly);
	}
	BOOL commit_on_focus_lost = TRUE;
	if (node->getAttributeBOOL("commit_on_focus_lost", commit_on_focus_lost))
	{
		line_editor->setCommitOnFocusLost(commit_on_focus_lost);
	}
	
	line_editor->setColorParameters(node);
	
	if(node->hasAttribute("select_on_focus"))
	{
		BOOL selectall = FALSE;
		node->getAttributeBOOL("select_on_focus", selectall);
		line_editor->setSelectAllonFocusReceived(selectall);
	}

	LLString prevalidate;
	if(node->getAttributeString("prevalidate", prevalidate))
	{
		LLString::toLower(prevalidate);

		if ("ascii" == prevalidate)
		{
			line_editor->setPrevalidate( LLLineEditor::prevalidateASCII );
		}
		else if ("float" == prevalidate)
		{
			line_editor->setPrevalidate( LLLineEditor::prevalidateFloat );
		}
		else if ("int" == prevalidate)
		{
			line_editor->setPrevalidate( LLLineEditor::prevalidateInt );
		}
		else if ("positive_s32" == prevalidate)
		{
			line_editor->setPrevalidate( LLLineEditor::prevalidatePositiveS32 );
		}
		else if ("non_negative_s32" == prevalidate)
		{
			line_editor->setPrevalidate( LLLineEditor::prevalidateNonNegativeS32 );
		}
		else if ("alpha_num" == prevalidate)
		{
			line_editor->setPrevalidate( LLLineEditor::prevalidateAlphaNum );
		}
		else if ("alpha_num_space" == prevalidate)
		{
			line_editor->setPrevalidate( LLLineEditor::prevalidateAlphaNumSpace );
		}
		else if ("printable_not_pipe" == prevalidate)
		{
			line_editor->setPrevalidate( LLLineEditor::prevalidatePrintableNotPipe );
		}
		else if ("printable_no_space" == prevalidate)
		{
			line_editor->setPrevalidate( LLLineEditor::prevalidatePrintableNoSpace );
		}
	}
	
	line_editor->initFromXML(node, parent);
	
	return line_editor;
}

void LLLineEditor::setColorParameters(LLXMLNodePtr node)
{
	LLColor4 color;
	if (LLUICtrlFactory::getAttributeColor(node,"cursor_color", color)) 
	{
		setCursorColor(color);
	}
	if(node->hasAttribute("text_color"))
	{
		LLUICtrlFactory::getAttributeColor(node,"text_color", color);
		setFgColor(color);
	}
	if(node->hasAttribute("text_readonly_color"))
	{
		LLUICtrlFactory::getAttributeColor(node,"text_readonly_color", color);
		setReadOnlyFgColor(color);
	}
	if (LLUICtrlFactory::getAttributeColor(node,"text_tentative_color", color))
	{
		setTentativeFgColor(color);
	}
	if(node->hasAttribute("bg_readonly_color"))
	{
		LLUICtrlFactory::getAttributeColor(node,"bg_readonly_color", color);
		setReadOnlyBgColor(color);
	}
	if(node->hasAttribute("bg_writeable_color"))
	{
		LLUICtrlFactory::getAttributeColor(node,"bg_writeable_color", color);
		setWriteableBgColor(color);
	}
}

BOOL LLLineEditor::setTextArg( const LLString& key, const LLStringExplicit& text )
{
	mText.setArg(key, text);
	return TRUE;
}

BOOL LLLineEditor::setLabelArg( const LLString& key, const LLStringExplicit& text )
{
	mLabel.setArg(key, text);
	return TRUE;
}


void LLLineEditor::updateAllowingLanguageInput()
{
	// Allow Language Text Input only when this LineEditor has
	// no prevalidate function attached (as long as other criteria
	// common to LLTextEditor).  This criterion works
	// fine on 1.15.0.2, since all prevalidate func reject any
	// non-ASCII characters.  I'm not sure on future versions,
	// however...
	if (hasFocus() && !mReadOnly && !mDrawAsterixes && mPrevalidateFunc == NULL)
	{
		getWindow()->allowLanguageTextInput(this, TRUE);
	}
	else
	{
		getWindow()->allowLanguageTextInput(this, FALSE);
	}
}

BOOL LLLineEditor::hasPreeditString() const
{
	return (mPreeditPositions.size() > 1);
}

void LLLineEditor::resetPreedit()
{
	if (hasPreeditString())
	{
		if (hasSelection())
		{
			llwarns << "Preedit and selection!" << llendl;
			deselect();
		}

		const S32 preedit_pos = mPreeditPositions.front();
		mText.erase(preedit_pos, mPreeditPositions.back() - preedit_pos);
		mText.insert(preedit_pos, mPreeditOverwrittenWString);
		setCursor(preedit_pos);
		
		mPreeditWString.clear();
		mPreeditOverwrittenWString.clear();
		mPreeditPositions.clear();

		// Don't reset key stroke timer nor invoke keystroke callback,
		// because a call to updatePreedit should be follow soon in 
		// normal course of operation, and timer and callback will be 
		// maintained there.  Doing so here made an odd sound.  (VWR-3410) 
	}
}

void LLLineEditor::updatePreedit(const LLWString &preedit_string,
		const segment_lengths_t &preedit_segment_lengths, const standouts_t &preedit_standouts, S32 caret_position)
{
	// Just in case.
	if (mReadOnly)
	{
		return;
	}

	// Note that call to updatePreedit is always preceeded by resetPreedit,
	// so we have no existing selection/preedit.

	S32 insert_preedit_at = getCursor();

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
		mPreeditOverwrittenWString.assign( LLWString( mText, insert_preedit_at, mPreeditWString.length() ) );
		mText.erase(insert_preedit_at, mPreeditWString.length());
	}
	else
	{
		mPreeditOverwrittenWString.clear();
	}
	mText.insert(insert_preedit_at, mPreeditWString);

	mPreeditStandouts = preedit_standouts;

	setCursor(position);
	setCursor(mPreeditPositions.front() + caret_position);

	// Update of the preedit should be caused by some key strokes.
	mKeystrokeTimer.reset();
	if( mKeystrokeCallback )
	{
		mKeystrokeCallback( this, mCallbackUserData );
	}
}

BOOL LLLineEditor::getPreeditLocation(S32 query_offset, LLCoordGL *coord, LLRect *bounds, LLRect *control) const
{
	if (control)
	{
		LLRect control_rect_screen;
		localRectToScreen(getRect(), &control_rect_screen);
		LLUI::screenRectToGL(control_rect_screen, control);
	}

	S32 preedit_left_column, preedit_right_column;
	if (hasPreeditString())
	{
		preedit_left_column = mPreeditPositions.front();
		preedit_right_column = mPreeditPositions.back();
	}
	else
	{
		preedit_left_column = preedit_right_column = getCursor();
	}
	if (preedit_right_column < mScrollHPos)
	{
		// This should not occure...
		return FALSE;
	}

	const S32 query = (query_offset >= 0 ? preedit_left_column + query_offset : getCursor());
	if (query < mScrollHPos || query < preedit_left_column || query > preedit_right_column)
	{
		return FALSE;
	}

	if (coord)
	{
		S32 query_local = findPixelNearestPos(query - getCursor());
		S32 query_screen_x, query_screen_y;
		localPointToScreen(query_local, getRect().getHeight() / 2, &query_screen_x, &query_screen_y);
		LLUI::screenPointToGL(query_screen_x, query_screen_y, &coord->mX, &coord->mY);
	}

	if (bounds)
	{
		S32 preedit_left_local = findPixelNearestPos(llmax(preedit_left_column, mScrollHPos) - getCursor());
		S32 preedit_right_local = llmin(findPixelNearestPos(preedit_right_column - getCursor()), getRect().getWidth() - mBorderThickness);
		if (preedit_left_local > preedit_right_local)
		{
			// Is this condition possible?
			preedit_right_local = preedit_left_local;
		}

		LLRect preedit_rect_local(preedit_left_local, getRect().getHeight(), preedit_right_local, 0);
		LLRect preedit_rect_screen;
		localRectToScreen(preedit_rect_local, &preedit_rect_screen);
		LLUI::screenRectToGL(preedit_rect_screen, bounds);
	}

	return TRUE;
}

void LLLineEditor::getPreeditRange(S32 *position, S32 *length) const
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

void LLLineEditor::getSelectionRange(S32 *position, S32 *length) const
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

void LLLineEditor::markAsPreedit(S32 position, S32 length)
{
	deselect();
	setCursor(position);
	if (hasPreeditString())
	{
		llwarns << "markAsPreedit invoked when hasPreeditString is true." << llendl;
	}
	mPreeditWString.assign( LLWString( mText.getWString(), position, length ) );
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

S32 LLLineEditor::getPreeditFontSize() const
{
	return llround(mGLFont->getLineHeight() * LLUI::sGLScaleFactor.mV[VY]);
}


LLSearchEditor::LLSearchEditor(const LLString& name, 
		const LLRect& rect,
		S32 max_length_bytes,
		void (*search_callback)(const LLString& search_string, void* user_data),
		void* userdata)
	: 
		LLUICtrl(name, rect, TRUE, NULL, userdata),
		mSearchCallback(search_callback)
{
	LLRect search_edit_rect(0, getRect().getHeight(), getRect().getWidth(), 0);
	mSearchEdit = new LLLineEditor("search edit",
			search_edit_rect,
			LLString::null,
			NULL,
			max_length_bytes,
			NULL,
			onSearchEdit,
			NULL,
			this);

	mSearchEdit->setFollowsAll();
	mSearchEdit->setSelectAllonFocusReceived(TRUE);

	addChild(mSearchEdit);

	S32 btn_width = rect.getHeight(); // button is square, and as tall as search editor
	LLRect clear_btn_rect(rect.getWidth() - btn_width, rect.getHeight(), rect.getWidth(), 0);
	mClearSearchButton = new LLButton("clear search", 
								clear_btn_rect, 
								"closebox.tga",
								"UIImgBtnCloseInactiveUUID",
								LLString::null,
								onClearSearch,
								this,
								NULL,
								LLString::null);
	mClearSearchButton->setFollowsRight();
	mClearSearchButton->setFollowsTop();
	mClearSearchButton->setImageColor(LLUI::sColorsGroup->getColor("TextFgTentativeColor"));
	mClearSearchButton->setTabStop(FALSE);
	mSearchEdit->addChild(mClearSearchButton);

	mSearchEdit->setBorderWidth(0, btn_width);
}


void LLSearchEditor::draw()
{
	mClearSearchButton->setVisible(!mSearchEdit->getWText().empty());

	LLUICtrl::draw();
}


//static
void LLSearchEditor::onSearchEdit(LLLineEditor* caller, void* user_data )
{
	LLSearchEditor* search_editor = (LLSearchEditor*)user_data;
	if (search_editor->mSearchCallback)
	{
		search_editor->mSearchCallback(caller->getText(), search_editor->mCallbackUserData);
	}
}

//static
void LLSearchEditor::onClearSearch(void* user_data)
{
	LLSearchEditor* search_editor = (LLSearchEditor*)user_data;

	search_editor->setText(LLString::null);
	if (search_editor->mSearchCallback)
	{
		search_editor->mSearchCallback(LLString::null, search_editor->mCallbackUserData);
	}
}

// static
LLView* LLSearchEditor::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("search_editor");
	node->getAttributeString("name", name);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	S32 max_text_length = 128;
	node->getAttributeS32("max_length", max_text_length);

	LLString text = node->getValue().substr(0, max_text_length - 1);

	LLSearchEditor* search_editor = new LLSearchEditor(name,
								rect, 
								max_text_length,
								NULL, NULL);

	LLString label;
	if(node->getAttributeString("label", label))
	{
		search_editor->mSearchEdit->setLabel(label);
	}
	
	search_editor->setText(text);

	search_editor->initFromXML(node, parent);
	
	return search_editor;
}

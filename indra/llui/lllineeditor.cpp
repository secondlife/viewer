/** 
 * @file lllineeditor.cpp
 * @brief LLLineEditor base class
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

// Text editor widget to let users enter a single line.

#include "linden_common.h"
 
#define LLLINEEDITOR_CPP
#include "lllineeditor.h"

#include "lltexteditor.h"
#include "llmath.h"
#include "llfontgl.h"
#include "llgl.h"
#include "lltimer.h"

#include "llcalc.h"
//#include "llclipboard.h"
#include "llcontrol.h"
#include "llbutton.h"
#include "llfocusmgr.h"
#include "llkeyboard.h"
#include "llrect.h"
#include "llresmgr.h"
#include "llspellcheck.h"
#include "llstring.h"
#include "llwindow.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llclipboard.h"
#include "llmenugl.h"

//
// Imported globals
//

//
// Constants
//

const F32	CURSOR_FLASH_DELAY = 1.0f;  // in seconds
const S32	SCROLL_INCREMENT_ADD = 0;	// make space for typing
const S32   SCROLL_INCREMENT_DEL = 4;	// make space for baskspacing
const F32   AUTO_SCROLL_TIME = 0.05f;
const F32	TRIPLE_CLICK_INTERVAL = 0.3f;	// delay between double and triple click. *TODO: make this equal to the double click interval?
const F32	SPELLCHECK_DELAY = 0.5f;	// delay between the last keypress and spell checking the word the cursor is on

const std::string PASSWORD_ASTERISK( "\xE2\x80\xA2" ); // U+2022 BULLET

static LLDefaultChildRegistry::Register<LLLineEditor> r1("line_editor");

// Compiler optimization, generate extern template
template class LLLineEditor* LLView::getChild<class LLLineEditor>(
	const std::string& name, BOOL recurse) const;

//
// Member functions
//

LLLineEditor::Params::Params()
:	max_length(""),
    keystroke_callback("keystroke_callback"),
	prevalidate_callback("prevalidate_callback"),
	prevalidate_input_callback("prevalidate_input_callback"),
	background_image("background_image"),
	background_image_disabled("background_image_disabled"),
	background_image_focused("background_image_focused"),
	select_on_focus("select_on_focus", false),
	revert_on_esc("revert_on_esc", true),
	spellcheck("spellcheck", false),
	commit_on_focus_lost("commit_on_focus_lost", true),
	ignore_tab("ignore_tab", true),
	is_password("is_password", false),
	cursor_color("cursor_color"),
	text_color("text_color"),
	text_readonly_color("text_readonly_color"),
	text_tentative_color("text_tentative_color"),
	highlight_color("highlight_color"),
	preedit_bg_color("preedit_bg_color"),
	border(""),
	bg_visible("bg_visible"),
	text_pad_left("text_pad_left"),
	text_pad_right("text_pad_right"),
	default_text("default_text")
{
	changeDefault(mouse_opaque, true);
	addSynonym(select_on_focus, "select_all_on_focus_received");
	addSynonym(border, "border");
	addSynonym(label, "watermark_text");
	addSynonym(max_length.chars, "max_length");
}

LLLineEditor::LLLineEditor(const LLLineEditor::Params& p)
:	LLUICtrl(p),
	mMaxLengthBytes(p.max_length.bytes),
	mMaxLengthChars(p.max_length.chars),
	mCursorPos( 0 ),
	mScrollHPos( 0 ),
	mTextPadLeft(p.text_pad_left),
	mTextPadRight(p.text_pad_right),
	mTextLeftEdge(0),		// computed in updateTextPadding() below
	mTextRightEdge(0),		// computed in updateTextPadding() below
	mCommitOnFocusLost( p.commit_on_focus_lost ),
	mRevertOnEsc( p.revert_on_esc ),
	mKeystrokeCallback( p.keystroke_callback() ),
	mIsSelecting( FALSE ),
	mSelectionStart( 0 ),
	mSelectionEnd( 0 ),
	mLastSelectionX(-1),
	mLastSelectionY(-1),
	mLastSelectionStart(-1),
	mLastSelectionEnd(-1),
	mBorderThickness( 0 ),
	mIgnoreArrowKeys( FALSE ),
	mIgnoreTab( p.ignore_tab ),
	mDrawAsterixes( p.is_password ),
	mSpellCheck( p.spellcheck ),
	mSpellCheckStart(-1),
	mSpellCheckEnd(-1),
	mSelectAllonFocusReceived( p.select_on_focus ),
	mSelectAllonCommit( TRUE ),
	mPassDelete(FALSE),
	mReadOnly(FALSE),
	mBgImage( p.background_image ),
	mBgImageDisabled( p.background_image_disabled ),
	mBgImageFocused( p.background_image_focused ),
	mHaveHistory(FALSE),
	mReplaceNewlinesWithSpaces( TRUE ),
	mLabel(p.label),
	mCursorColor(p.cursor_color()),
	mFgColor(p.text_color()),
	mReadOnlyFgColor(p.text_readonly_color()),
	mTentativeFgColor(p.text_tentative_color()),
	mHighlightColor(p.highlight_color()),
	mPreeditBgColor(p.preedit_bg_color()),
	mGLFont(p.font),
	mContextMenuHandle(),
	mAutoreplaceCallback()
{
	llassert( mMaxLengthBytes > 0 );

	mScrollTimer.reset();
	mTripleClickTimer.reset();
	setText(p.default_text());

	// Initialize current history line iterator
	mCurrentHistoryLine = mLineHistory.begin();

	LLRect border_rect(getLocalRect());
	// adjust for gl line drawing glitch
	border_rect.mTop -= 1;
	border_rect.mRight -=1;
	LLViewBorder::Params border_p(p.border);
	border_p.rect = border_rect;
	border_p.follows.flags = FOLLOWS_ALL;
	border_p.bevel_style = LLViewBorder::BEVEL_IN;
	mBorder = LLUICtrlFactory::create<LLViewBorder>(border_p);
	addChild( mBorder );

	// clamp text padding to current editor size
	updateTextPadding();
	setCursor(mText.length());

	if (mSpellCheck)
	{
		LLSpellChecker::setSettingsChangeCallback(boost::bind(&LLLineEditor::onSpellCheckSettingsChange, this));
	}
	mSpellCheckTimer.reset();

	setPrevalidateInput(p.prevalidate_input_callback());
	setPrevalidate(p.prevalidate_callback());

	LLContextMenu* menu = LLUICtrlFactory::instance().createFromFile<LLContextMenu>
		("menu_text_editor.xml",
		 LLMenuGL::sMenuContainer,
		 LLMenuHolderGL::child_registry_t::instance());
	setContextMenu(menu);
}
 
LLLineEditor::~LLLineEditor()
{
	mCommitOnFocusLost = FALSE;

	// calls onCommit() while LLLineEditor still valid
	gFocusMgr.releaseFocusIfNeeded( this );
}

void LLLineEditor::onFocusReceived()
{
	gEditMenuHandler = this;
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

// virtual
void LLLineEditor::onCommit()
{
	// put current line into the line history
	updateHistory();

	setControlValue(getValue());
	LLUICtrl::onCommit();

	// Selection on commit needs to be turned off when evaluating maths
	// expressions, to allow indication of the error position
	if (mSelectAllonCommit) selectAll();
}

// Returns TRUE if user changed value at all
// virtual
BOOL LLLineEditor::isDirty() const
{
	return mText.getString() != mPrevText;
}

// Clear dirty state
// virtual
void LLLineEditor::resetDirty()
{
	mPrevText = mText.getString();
}		

// assumes UTF8 text
// virtual
void LLLineEditor::setValue(const LLSD& value )
{
	setText(value.asString());
}

//virtual
LLSD LLLineEditor::getValue() const
{
	return LLSD(getText());
}


// line history support
void LLLineEditor::updateHistory()
{
	// On history enabled line editors, remember committed line and
	// reset current history line number.
	// Be sure only to remember lines that are not empty and that are
	// different from the last on the list.
	if( mHaveHistory && getLength() )
	{
		if( !mLineHistory.empty() )
		{
			// When not empty, last line of history should always be blank.
			if( mLineHistory.back().empty() )
			{
				// discard the empty line
				mLineHistory.pop_back();
			}
			else
			{
				LL_WARNS("") << "Last line of history was not blank." << LL_ENDL;
			}
		}

		// Add text to history, ignoring duplicates
		if( mLineHistory.empty() || getText() != mLineHistory.back() )
		{
			mLineHistory.push_back( getText() );
		}

		// Restore the blank line and set mCurrentHistoryLine to point at it
		mLineHistory.push_back( "" );
		mCurrentHistoryLine = mLineHistory.end() - 1;
	}
}

void LLLineEditor::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLUICtrl::reshape(width, height, called_from_parent);
	updateTextPadding(); // For clamping side-effect.
	setCursor(mCursorPos); // For clamping side-effect.
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

void LLLineEditor::setMaxTextChars(S32 max_text_chars)
{
	S32 max_chars = llmax(0, max_text_chars);
	mMaxLengthChars = max_chars;
} 

void LLLineEditor::getTextPadding(S32 *left, S32 *right)
{
	*left = mTextPadLeft;
	*right = mTextPadRight;
}

void LLLineEditor::setTextPadding(S32 left, S32 right)
{
	mTextPadLeft = left;
	mTextPadRight = right;
	updateTextPadding();
}

void LLLineEditor::updateTextPadding()
{
	mTextLeftEdge = llclamp(mTextPadLeft, 0, getRect().getWidth());
	mTextRightEdge = getRect().getWidth() - llclamp(mTextPadRight, 0, getRect().getWidth());
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

	std::string truncated_utf8 = new_text;
	if (truncated_utf8.size() > (U32)mMaxLengthBytes)
	{	
		truncated_utf8 = utf8str_truncate(new_text, mMaxLengthBytes);
	}
	mText.assign(truncated_utf8);

	if (mMaxLengthChars)
	{
		LLWString truncated_wstring = utf8str_to_wstring(truncated_utf8);
		if (truncated_wstring.size() > (U32)mMaxLengthChars)
		{
			truncated_wstring = truncated_wstring.substr(0, mMaxLengthChars);
		}
		mText.assign(wstring_to_utf8str(truncated_wstring));
	}

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

	// Set current history line to end of history.
	if (mLineHistory.empty())
	{
		mCurrentHistoryLine = mLineHistory.end();
	}
	else
	{
		mCurrentHistoryLine = mLineHistory.end() - 1;
	}

	mPrevText = mText;
}


// Picks a new cursor position based on the actual screen size of text being drawn.
void LLLineEditor::setCursorAtLocalPos( S32 local_mouse_x )
{
	S32 cursor_pos = calcCursorPos(local_mouse_x);

	S32 left_pos = llmin( mSelectionStart, cursor_pos );
	S32 length = llabs( mSelectionStart - cursor_pos );
	const LLWString& substr = mText.getWString().substr(left_pos, length);

	if (mIsSelecting && !prevalidateInput(substr))
		return;

	setCursor(cursor_pos);
}

void LLLineEditor::setCursor( S32 pos )
{
	S32 old_cursor_pos = getCursor();
	mCursorPos = llclamp( pos, 0, mText.length());

	// position of end of next character after cursor
	S32 pixels_after_scroll = findPixelNearestPos();
	if( pixels_after_scroll > mTextRightEdge )
	{
		S32 width_chars_to_left = mGLFont->getWidth(mText.getWString().c_str(), 0, mScrollHPos);
		S32 last_visible_char = mGLFont->maxDrawableChars(mText.getWString().c_str(), llmax(0.f, (F32)(mTextRightEdge - mTextLeftEdge + width_chars_to_left))); 
		// character immediately to left of cursor should be last one visible (SCROLL_INCREMENT_ADD will scroll in more characters)
		// or first character if cursor is at beginning
		S32 new_last_visible_char = llmax(0, getCursor() - 1);
		S32 min_scroll = mGLFont->firstDrawableChar(mText.getWString().c_str(), (F32)(mTextRightEdge - mTextLeftEdge), mText.length(), new_last_visible_char);
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
	if (!prevalidateInput(mText.getWString()))
	{
		return;
	}

	mSelectionStart = mText.length();
	mSelectionEnd = 0;
	setCursor(mSelectionEnd);
	//mScrollHPos = 0;
	mIsSelecting = TRUE;
	updatePrimary();
}

bool LLLineEditor::getSpellCheck() const
{
	return (LLSpellChecker::getUseSpellCheck()) && (!mReadOnly) && (mSpellCheck);
}

const std::string& LLLineEditor::getSuggestion(U32 index) const
{
	return (index < mSuggestionList.size()) ? mSuggestionList[index] : LLStringUtil::null;
}

U32 LLLineEditor::getSuggestionCount() const
{
	return mSuggestionList.size();
}

void LLLineEditor::replaceWithSuggestion(U32 index)
{
	for (std::list<std::pair<U32, U32> >::const_iterator it = mMisspellRanges.begin(); it != mMisspellRanges.end(); ++it)
	{
		if ( (it->first <= (U32)mCursorPos) && (it->second >= (U32)mCursorPos) )
		{
			deselect();

			// Delete the misspelled word
			mText.erase(it->first, it->second - it->first);

			// Insert the suggestion in its place
			LLWString suggestion = utf8str_to_wstring(mSuggestionList[index]);
			mText.insert(it->first, suggestion);
			setCursor(it->first + (S32)suggestion.length());

			break;
		}
	}
	mSpellCheckStart = mSpellCheckEnd = -1;
}

void LLLineEditor::addToDictionary()
{
	if (canAddToDictionary())
	{
		LLSpellChecker::instance().addToCustomDictionary(getMisspelledWord(mCursorPos));
	}
}

bool LLLineEditor::canAddToDictionary() const
{
	return (getSpellCheck()) && (isMisspelledWord(mCursorPos));
}

void LLLineEditor::addToIgnore()
{
	if (canAddToIgnore())
	{
		LLSpellChecker::instance().addToIgnoreList(getMisspelledWord(mCursorPos));
	}
}

bool LLLineEditor::canAddToIgnore() const
{
	return (getSpellCheck()) && (isMisspelledWord(mCursorPos));
}

std::string LLLineEditor::getMisspelledWord(U32 pos) const
{
	for (std::list<std::pair<U32, U32> >::const_iterator it = mMisspellRanges.begin(); it != mMisspellRanges.end(); ++it)
	{
		if ( (it->first <= pos) && (it->second >= pos) )
		{
			return wstring_to_utf8str(mText.getWString().substr(it->first, it->second - it->first));
		}
	}
	return LLStringUtil::null;
}

bool LLLineEditor::isMisspelledWord(U32 pos) const
{
	for (std::list<std::pair<U32, U32> >::const_iterator it = mMisspellRanges.begin(); it != mMisspellRanges.end(); ++it)
	{
		if ( (it->first <= pos) && (it->second >= pos) )
		{
			return true;
		}
	}
	return false;
}

void LLLineEditor::onSpellCheckSettingsChange()
{
	// Recheck the spelling on every change
	mMisspellRanges.clear();
	mSpellCheckStart = mSpellCheckEnd = -1;
}

BOOL LLLineEditor::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	setFocus( TRUE );
	mTripleClickTimer.setTimerExpirySec(TRIPLE_CLICK_INTERVAL);

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
		if( LLWStringUtil::isPartOfWord( wtext[mCursorPos] ) )
		{
			S32 old_selection_start = mLastSelectionStart;
			S32 old_selection_end = mLastSelectionEnd;

			// Select word the cursor is over
			while ((mCursorPos > 0) && LLWStringUtil::isPartOfWord( wtext[mCursorPos-1] ))
			{	// Find the start of the word
				mCursorPos--;
			}
			startSelection();	

			while ((mCursorPos < (S32)wtext.length()) && LLWStringUtil::isPartOfWord( wtext[mCursorPos] ) )
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

	// take selection to 'primary' clipboard
	updatePrimary();

	return TRUE;
}

BOOL LLLineEditor::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// Check first whether the "clear search" button wants to deal with this.
	if(childrenHandleMouseDown(x, y, mask) != NULL) 
	{
		return TRUE;
	}
	
	if (!mSelectAllonFocusReceived
		|| gFocusMgr.getKeyboardFocus() == this)
	{
		mLastSelectionStart = -1;
		mLastSelectionStart = -1;

		if (mask & MASK_SHIFT)
		{
			// assume we're starting a drag select
			mIsSelecting = TRUE;

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
		}
		else
		{
			if (mTripleClickTimer.hasExpired())
			{
				// Save selection for word/line selecting on double-click
				mLastSelectionStart = mSelectionStart;
				mLastSelectionEnd = mSelectionEnd;

				// Move cursor and deselect for regular click
				setCursorAtLocalPos( x );
				deselect();
				startSelection();
			}
			else // handle triple click
			{
				selectAll();
				// We don't want handleMouseUp() to "finish" the selection (and thereby
				// set mSelectionEnd to where the mouse is), so we finish the selection 
				// here.
				mIsSelecting = FALSE;
			}
		}

		gFocusMgr.setMouseCapture( this );
	}

	setFocus(TRUE);

	// delay cursor flashing
	mKeystrokeTimer.reset();
	
	if (mMouseDownSignal)
		(*mMouseDownSignal)(this,x,y,mask);

	return TRUE;
}

BOOL LLLineEditor::handleMiddleMouseDown(S32 x, S32 y, MASK mask)
{
        // llinfos << "MiddleMouseDown" << llendl;
	setFocus( TRUE );
	if( canPastePrimary() )
	{
		setCursorAtLocalPos(x);
		pastePrimary();
	}
	return TRUE;
}

BOOL LLLineEditor::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	setFocus(TRUE);
	if (!LLUICtrl::handleRightMouseDown(x, y, mask))
	{
		showContextMenu(x, y);
	}
	return TRUE;
}

BOOL LLLineEditor::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;
	// Check first whether the "clear search" button wants to deal with this.
	if(!hasMouseCapture())
	{
		if(childrenHandleHover(x, y, mask) != NULL) 
		{
			return TRUE;
		}
	}

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
			if( (x < mTextLeftEdge) && (mScrollHPos > 0 ) )
			{
				// Scroll to the left
				mScrollHPos = llclamp(mScrollHPos - increment, 0, mText.length());
			}
			else
			if( (x > mTextRightEdge) && (mCursorPos < (S32)mText.length()) )
			{
				// If scrolling one pixel would make a difference...
				S32 pixels_after_scrolling_one_char = findPixelNearestPos(1);
				if( pixels_after_scrolling_one_char >= mTextRightEdge )
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

	// Check first whether the "clear search" button wants to deal with this.
	if(!handled && childrenHandleMouseUp(x, y, mask) != NULL) 
	{
		return TRUE;
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

		// take selection to 'primary' clipboard
		updatePrimary();
	}
	
	// We won't call LLUICtrl::handleMouseUp to avoid double calls of  childrenHandleMouseUp().Just invoke the signal manually.
	if (mMouseUpSignal)
		(*mMouseUpSignal)(this,x,y, mask);
	return handled;
}


// Remove a single character from the text
void LLLineEditor::removeChar()
{
	if( getCursor() > 0 )
	{
		if (!prevalidateInput(mText.getWString().substr(getCursor()-1, 1)))
			return;

		mText.erase(getCursor() - 1, 1);

		setCursor(getCursor() - 1);
	}
	else
	{
		LLUI::reportBadKeystroke();
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
		if (!prevalidateInput(mText.getWString().substr(getCursor(), 1)))
			return;

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
	else if (mMaxLengthChars)
	{
		S32 wide_chars = mText.getWString().size();
		if ((wide_chars + 1) > mMaxLengthChars)
		{
			allow_char = FALSE;
		}
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
		LLUI::reportBadKeystroke();
	}

	if (!mReadOnly && mAutoreplaceCallback != NULL)
	{
		// call callback
		mAutoreplaceCallback(mText, mCursorPos);
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
	
	S32 left_pos = llmin( mSelectionStart, new_cursor_pos );
	S32 selection_length = llabs( mSelectionStart - new_cursor_pos );
	const LLWString& selection = mText.getWString().substr(left_pos, selection_length);

	if (!prevalidateInput(selection))
		return;

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
	while( (cursorPos > 0) && LLWStringUtil::isPartOfWord( wtext[cursorPos-1] ) )
	{
		cursorPos--;
	}
	return cursorPos;
}

S32 LLLineEditor::nextWordPos(S32 cursorPos) const
{
	const LLWString& wtext = mText.getWString();
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
				LLUI::reportBadKeystroke();
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
				LLUI::reportBadKeystroke();
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

	if(handled)
	{
		// take selection to 'primary' clipboard
		updatePrimary();
	}
 
	return handled;
}

void LLLineEditor::deleteSelection()
{
	if( !mReadOnly && hasSelection() )
	{
		S32 left_pos, selection_length;
		getSelectionRange(&left_pos, &selection_length);
		const LLWString& selection = mText.getWString().substr(left_pos, selection_length);

		if (!prevalidateInput(selection))
			return;

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
		S32 left_pos, length;
		getSelectionRange(&left_pos, &length);
		const LLWString& selection = mText.getWString().substr(left_pos, length);

		if (!prevalidateInput(selection))
			return;

		// Prepare for possible rollback
		LLLineEditorRollback rollback( this );

		LLClipboard::instance().copyToClipboard( mText.getWString(), left_pos, length );
		deleteSelection();

		// Validate new string and rollback the if needed.
		BOOL need_to_rollback = ( mPrevalidateFunc && !mPrevalidateFunc( mText.getWString() ) );
		if( need_to_rollback )
		{
			rollback.doRollback( this );
			LLUI::reportBadKeystroke();
		}
		else
		{
			onKeystroke();
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
		S32 length = llabs( mSelectionStart - mSelectionEnd );
		LLClipboard::instance().copyToClipboard( mText.getWString(), left_pos, length );
	}
}

BOOL LLLineEditor::canPaste() const
{
	return !mReadOnly && LLClipboard::instance().isTextAvailable(); 
}

void LLLineEditor::paste()
{
	bool is_primary = false;
	pasteHelper(is_primary);
}

void LLLineEditor::pastePrimary()
{
	bool is_primary = true;
	pasteHelper(is_primary);
}

// paste from primary (is_primary==true) or clipboard (is_primary==false)
void LLLineEditor::pasteHelper(bool is_primary)
{
	bool can_paste_it;
	if (is_primary)
	{
		can_paste_it = canPastePrimary();
	}
	else
	{
		can_paste_it = canPaste();
	}

	if (can_paste_it)
	{
		LLWString paste;
		LLClipboard::instance().pasteFromClipboard(paste, is_primary);

		if (!paste.empty())
		{
			if (!prevalidateInput(paste))
				return;

			// Prepare for possible rollback
			LLLineEditorRollback rollback(this);
			
			// Delete any selected characters
			if ((!is_primary) && hasSelection())
			{
				deleteSelection();
			}

			// Clean up string (replace tabs and returns and remove characters that our fonts don't support.)
			LLWString clean_string(paste);
			LLWStringUtil::replaceTabsWithSpaces(clean_string, 1);
			//clean_string = wstring_detabify(paste, 1);
			LLWStringUtil::replaceChar(clean_string, '\n', mReplaceNewlinesWithSpaces ? ' ' : 182); // 182 == paragraph character

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
				LLUI::reportBadKeystroke();
			}

			if (mMaxLengthChars)
			{
				U32 available_chars = mMaxLengthChars - mText.getWString().size();
		
				if (available_chars < clean_string.size())
				{
					clean_string = clean_string.substr(0, available_chars);
				}

				LLUI::reportBadKeystroke();
			}

			mText.insert(getCursor(), clean_string);
			setCursor( getCursor() + (S32)clean_string.length() );
			deselect();

			// Validate new string and rollback the if needed.
			BOOL need_to_rollback = ( mPrevalidateFunc && !mPrevalidateFunc( mText.getWString() ) );
			if( need_to_rollback )
			{
				rollback.doRollback( this );
				LLUI::reportBadKeystroke();
			}
			else
			{
				onKeystroke();
			}
		}
	}
}

// copy selection to primary
void LLLineEditor::copyPrimary()
{
	if( canCopy() )
	{
		S32 left_pos = llmin( mSelectionStart, mSelectionEnd );
		S32 length = llabs( mSelectionStart - mSelectionEnd );
		LLClipboard::instance().copyToClipboard( mText.getWString(), left_pos, length, true);
	}
}

BOOL LLLineEditor::canPastePrimary() const
{
	return !mReadOnly && LLClipboard::instance().isTextAvailable(true); 
}

void LLLineEditor::updatePrimary()
{
	if(canCopy() )
	{
		copyPrimary();
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
				LLUI::reportBadKeystroke();
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
				LLUI::reportBadKeystroke();
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
				LLUI::reportBadKeystroke();
			}
			handled = TRUE;
		}
		break;

	// handle ctrl-uparrow if we have a history enabled line editor.
	case KEY_UP:
		if( mHaveHistory && ((mIgnoreArrowKeys == false) || ( MASK_CONTROL == mask )) )
		{
			if( mCurrentHistoryLine > mLineHistory.begin() )
			{
				mText.assign( *(--mCurrentHistoryLine) );
				setCursorToEnd();
			}
			else
			{
				LLUI::reportBadKeystroke();
			}
			handled = TRUE;
		}
		break;

	// handle [ctrl]-downarrow if we have a history enabled line editor
	case KEY_DOWN:
		if( mHaveHistory  && ((mIgnoreArrowKeys == false) || ( MASK_CONTROL == mask )) )
		{
			if( !mLineHistory.empty() && mCurrentHistoryLine < mLineHistory.end() - 1 )
			{
				mText.assign( *(++mCurrentHistoryLine) );
				setCursorToEnd();
			}
			else
			{
				LLUI::reportBadKeystroke();
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

	return handled;
}


BOOL LLLineEditor::handleKeyHere(KEY key, MASK mask )
{
	BOOL	handled = FALSE;
	BOOL	selection_modified = FALSE;

	if ( gFocusMgr.getKeyboardFocus() == this )
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

				LLUI::reportBadKeystroke();
			}

			// Notify owner if requested
			if (!need_to_rollback && handled)
			{
				onKeystroke();
				if ( (!selection_modified) && (KEY_BACKSPACE == key) )
				{
					mSpellCheckTimer.setTimerExpirySec(SPELLCHECK_DELAY);
				}
			}
		}
	}

	return handled;
}


BOOL LLLineEditor::handleUnicodeCharHere(llwchar uni_char)
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

		{
			LLWString u_char;
			u_char.assign(1, uni_char);
			if (!prevalidateInput(u_char))
				return handled;
		}

		addChar(uni_char);

		mKeystrokeTimer.reset();

		deselect();

		BOOL need_to_rollback = FALSE;

		// Validate new string and rollback the keystroke if needed.
		need_to_rollback |= ( mPrevalidateFunc && !mPrevalidateFunc( mText.getWString() ) );

		if( need_to_rollback )
		{
			rollback.doRollback( this );

			LLUI::reportBadKeystroke();
		}

		// Notify owner if requested
		if( !need_to_rollback && handled )
		{
			// HACK! The only usage of this callback doesn't do anything with the character.
			// We'll have to do something about this if something ever changes! - Doug
			onKeystroke();

			mSpellCheckTimer.setTimerExpirySec(SPELLCHECK_DELAY);
		}
	}
	return handled;
}


BOOL LLLineEditor::canDoDelete() const
{
	return ( !mReadOnly && mText.length() > 0 && (!mPassDelete || (hasSelection() || (getCursor() < mText.length()))) );
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
			const LLWString& text_to_delete = mText.getWString().substr(getCursor(), 1);

			if (!prevalidateInput(text_to_delete))
			{
				onKeystroke();
				return;
			}
			setCursor(getCursor() + 1);
			removeChar();
		}

		// Validate new string and rollback the if needed.
		BOOL need_to_rollback = ( mPrevalidateFunc && !mPrevalidateFunc( mText.getWString() ) );
		if( need_to_rollback )
		{
			rollback.doRollback( this );
			LLUI::reportBadKeystroke();
		}
		else
		{
			onKeystroke();

			mSpellCheckTimer.setTimerExpirySec(SPELLCHECK_DELAY);
		}
	}
}


void LLLineEditor::drawBackground()
{
	bool has_focus = hasFocus();
	LLUIImage* image;
	if ( mReadOnly )
	{
		image = mBgImageDisabled;
	}
	else if ( has_focus )
	{
		image = mBgImageFocused;
	}
	else
	{
		image = mBgImage;
	}

	if (!image) return;
	
	F32 alpha = getCurrentTransparency();

	// optionally draw programmatic border
	if (has_focus)
	{
		LLColor4 tmp_color = gFocusMgr.getFocusColor();
		tmp_color.setAlpha(alpha);
		image->drawBorder(0, 0, getRect().getWidth(), getRect().getHeight(),
						  tmp_color,
						  gFocusMgr.getFocusFlashWidth());
	}
	LLColor4 tmp_color = UI_VERTEX_COLOR;
	tmp_color.setAlpha(alpha);
	image->draw(getLocalRect(), tmp_color);
}

void LLLineEditor::draw()
{
	F32 alpha = getDrawContext().mAlpha;
	S32 text_len = mText.length();
	static LLUICachedControl<S32> lineeditor_cursor_thickness ("UILineEditorCursorThickness", 0);
	static LLUICachedControl<F32> preedit_marker_brightness ("UIPreeditMarkerBrightness", 0);
	static LLUICachedControl<S32> preedit_marker_gap ("UIPreeditMarkerGap", 0);
	static LLUICachedControl<S32> preedit_marker_position ("UIPreeditMarkerPosition", 0);
	static LLUICachedControl<S32> preedit_marker_thickness ("UIPreeditMarkerThickness", 0);
	static LLUICachedControl<F32> preedit_standout_brightness ("UIPreeditStandoutBrightness", 0);
	static LLUICachedControl<S32> preedit_standout_gap ("UIPreeditStandoutGap", 0);
	static LLUICachedControl<S32> preedit_standout_position ("UIPreeditStandoutPosition", 0);
	static LLUICachedControl<S32> preedit_standout_thickness ("UIPreeditStandoutThickness", 0);

	std::string saved_text;
	if (mDrawAsterixes)
	{
		saved_text = mText.getString();
		std::string text;
		for (S32 i = 0; i < mText.length(); i++)
		{
			text += PASSWORD_ASTERISK;
		}
		mText = text;
	}

	// draw rectangle for the background
	LLRect background( 0, getRect().getHeight(), getRect().getWidth(), 0 );
	background.stretch( -mBorderThickness );

	S32 lineeditor_v_pad = (background.getHeight() - mGLFont->getLineHeight()) / 2;
	if (mSpellCheck)
	{
		lineeditor_v_pad += 1;
	}

	drawBackground();
	
	// draw text

	// With viewer-2 art files, input region is 2 pixels up
	S32 cursor_bottom = background.mBottom + 2;
	S32 cursor_top = background.mTop - 1;

	LLColor4 text_color;
	if (!mReadOnly)
	{
		if (!getTentative())
		{
			text_color = mFgColor.get();
		}
		else
		{
			text_color = mTentativeFgColor.get();
		}
	}
	else
	{
		text_color = mReadOnlyFgColor.get();
	}
	text_color.setAlpha(alpha);
	LLColor4 label_color = mTentativeFgColor.get();
	label_color.setAlpha(alpha);
	
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
					gl_rect_2d(preedit_pixels_left + preedit_standout_gap,
						background.mBottom + preedit_standout_position,
						preedit_pixels_right - preedit_standout_gap - 1,
						background.mBottom + preedit_standout_position - preedit_standout_thickness,
						(text_color * preedit_standout_brightness 
						 + mPreeditBgColor * (1 - preedit_standout_brightness)).setAlpha(alpha/*1.0f*/));
				}
				else
				{
					gl_rect_2d(preedit_pixels_left + preedit_marker_gap,
						background.mBottom + preedit_marker_position,
						preedit_pixels_right - preedit_marker_gap - 1,
						background.mBottom + preedit_marker_position - preedit_marker_thickness,
						(text_color * preedit_marker_brightness
						 + mPreeditBgColor * (1 - preedit_marker_brightness)).setAlpha(alpha/*1.0f*/));
				}
			}
		}
	}

	S32 rendered_text = 0;
	F32 rendered_pixels_right = (F32)mTextLeftEdge;
	F32 text_bottom = (F32)background.mBottom + (F32)lineeditor_v_pad;

	if( (gFocusMgr.getKeyboardFocus() == this) && hasSelection() )
	{
		S32 select_left;
		S32 select_right;
		if (mSelectionStart < mSelectionEnd)
		{
			select_left = mSelectionStart;
			select_right = mSelectionEnd;
		}
		else
		{
			select_left = mSelectionEnd;
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
				0,
				LLFontGL::NO_SHADOW,
				select_left - mScrollHPos,
				mTextRightEdge - llround(rendered_pixels_right),
				&rendered_pixels_right);
		}
		
		if( (rendered_pixels_right < (F32)mTextRightEdge) && (rendered_text < text_len) )
		{
			LLColor4 color = mHighlightColor;
			color.setAlpha(alpha);
			// selected middle
			S32 width = mGLFont->getWidth(mText.getWString().c_str(), mScrollHPos + rendered_text, select_right - mScrollHPos - rendered_text);
			width = llmin(width, mTextRightEdge - llround(rendered_pixels_right));
			gl_rect_2d(llround(rendered_pixels_right), cursor_top, llround(rendered_pixels_right)+width, cursor_bottom, color);

			LLColor4 tmp_color( 1.f - text_color.mV[0], 1.f - text_color.mV[1], 1.f - text_color.mV[2], alpha );
			rendered_text += mGLFont->render( 
				mText, mScrollHPos + rendered_text,
				rendered_pixels_right, text_bottom,
				tmp_color,
				LLFontGL::LEFT, LLFontGL::BOTTOM,
				0,
				LLFontGL::NO_SHADOW,
				select_right - mScrollHPos - rendered_text,
				mTextRightEdge - llround(rendered_pixels_right),
				&rendered_pixels_right);
		}

		if( (rendered_pixels_right < (F32)mTextRightEdge) && (rendered_text < text_len) )
		{
			// unselected, right side
			rendered_text += mGLFont->render( 
				mText, mScrollHPos + rendered_text,
				rendered_pixels_right, text_bottom,
				text_color,
				LLFontGL::LEFT, LLFontGL::BOTTOM,
				0,
				LLFontGL::NO_SHADOW,
				S32_MAX,
				mTextRightEdge - llround(rendered_pixels_right),
				&rendered_pixels_right);
		}
	}
	else
	{
		rendered_text = mGLFont->render( 
			mText, mScrollHPos, 
			rendered_pixels_right, text_bottom,
			text_color,
			LLFontGL::LEFT, LLFontGL::BOTTOM,
			0,
			LLFontGL::NO_SHADOW,
			S32_MAX,
			mTextRightEdge - llround(rendered_pixels_right),
			&rendered_pixels_right);
	}
#if 1 // for when we're ready for image art.
	mBorder->setVisible(FALSE); // no more programmatic art.
#endif

	if ( (getSpellCheck()) && (mText.length() > 2) )
	{
		// Calculate start and end indices for the first and last visible word
		U32 start = prevWordPos(mScrollHPos), end = nextWordPos(mScrollHPos + rendered_text);

		if ( (mSpellCheckStart != start) || (mSpellCheckEnd != end) )
		{
			const LLWString& text = mText.getWString().substr(start, end);

			// Find the start of the first word
			U32 word_start = 0, word_end = 0;
			while ( (word_start < text.length()) && (!LLStringOps::isAlpha(text[word_start])) )
			{
				word_start++;
			}

			// Iterate over all words in the text block and check them one by one
			mMisspellRanges.clear();
			while (word_start < text.length())
			{
				// Find the end of the current word (special case handling for "'" when it's used as a contraction)
				word_end = word_start + 1;
				while ( (word_end < text.length()) && 
					    ((LLWStringUtil::isPartOfWord(text[word_end])) ||
						 ((L'\'' == text[word_end]) && (word_end + 1 < text.length()) &&
						  (LLStringOps::isAlnum(text[word_end - 1])) && (LLStringOps::isAlnum(text[word_end + 1])))) )
				{
					word_end++;
				}
				if (word_end > text.length())
				{
					break;
				}

				// Don't process words shorter than 3 characters
				std::string word = wstring_to_utf8str(text.substr(word_start, word_end - word_start));
				if ( (word.length() >= 3) && (!LLSpellChecker::instance().checkSpelling(word)) )
				{
					mMisspellRanges.push_back(std::pair<U32, U32>(start + word_start, start + word_end));
				}

				// Find the start of the next word
				word_start = word_end + 1;
				while ( (word_start < text.length()) && (!LLWStringUtil::isPartOfWord(text[word_start])) )
				{
					word_start++;
				}
			}

			mSpellCheckStart = start;
			mSpellCheckEnd = end;
		}

		// Draw squiggly lines under any (visible) misspelled words
		for (std::list<std::pair<U32, U32> >::const_iterator it = mMisspellRanges.begin(); it != mMisspellRanges.end(); ++it)
		{
			// Skip over words that aren't (partially) visible
			if ( ((it->first < start) && (it->second < start)) || (it->first > end) )
			{
				continue;
			}

			// Skip the current word if the user is still busy editing it
			if ( (!mSpellCheckTimer.hasExpired()) && (it->first <= (U32)mCursorPos) && (it->second >= (U32)mCursorPos) )
			{
 				continue;
			}

			S32 pxWidth = getRect().getWidth();
			S32 pxStart = findPixelNearestPos(it->first - getCursor());
			if (pxStart > pxWidth)
			{
				continue;
			}
			S32 pxEnd = findPixelNearestPos(it->second - getCursor());
			if (pxEnd > pxWidth)
			{
				pxEnd = pxWidth;
			}

			S32 pxBottom = (S32)(text_bottom + mGLFont->getDescenderHeight());

			gGL.color4ub(255, 0, 0, 200);
			while (pxStart + 1 < pxEnd)
			{
				gl_line_2d(pxStart, pxBottom, pxStart + 2, pxBottom - 2);
				if (pxStart + 3 < pxEnd)
				{
					gl_line_2d(pxStart + 2, pxBottom - 3, pxStart + 4, pxBottom - 1);
				}
				pxStart += 4;
			}
		}
	}

	// If we're editing...
	if( hasFocus())
	{
		//mBorder->setVisible(TRUE); // ok, programmer art just this once.
		// (Flash the cursor every half second)
		if (!mReadOnly && gFocusMgr.getAppHasFocus())
		{
			F32 elapsed = mKeystrokeTimer.getElapsedTimeF32();
			if( (elapsed < CURSOR_FLASH_DELAY ) || (S32(elapsed * 2) & 1) )
			{
				S32 cursor_left = findPixelNearestPos();
				cursor_left -= lineeditor_cursor_thickness / 2;
				S32 cursor_right = cursor_left + lineeditor_cursor_thickness;
				if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode() && !hasSelection())
				{
					const LLWString space(utf8str_to_wstring(std::string(" ")));
					S32 wswidth = mGLFont->getWidth(space.c_str());
					S32 width = mGLFont->getWidth(mText.getWString().c_str(), getCursor(), 1) + 1;
					cursor_right = cursor_left + llmax(wswidth, width);
				}
				// Use same color as text for the Cursor
				gl_rect_2d(cursor_left, cursor_top,
					cursor_right, cursor_bottom, text_color);
				if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode() && !hasSelection())
				{
					LLColor4 tmp_color( 1.f - text_color.mV[0], 1.f - text_color.mV[1], 1.f - text_color.mV[2], alpha );
					mGLFont->render(mText, getCursor(), (F32)(cursor_left + lineeditor_cursor_thickness / 2), text_bottom, 
						tmp_color,
						LLFontGL::LEFT, LLFontGL::BOTTOM,
						0,
						LLFontGL::NO_SHADOW,
						1);
				}

				// Make sure the IME is in the right place
				S32 pixels_after_scroll = findPixelNearestPos();	// RCalculcate for IME position
				LLRect screen_pos = calcScreenRect();
				LLCoordGL ime_pos( screen_pos.mLeft + pixels_after_scroll, screen_pos.mTop - lineeditor_v_pad );

				ime_pos.mX = (S32) (ime_pos.mX * LLUI::sGLScaleFactor.mV[VX]);
				ime_pos.mY = (S32) (ime_pos.mY * LLUI::sGLScaleFactor.mV[VY]);
				getWindow()->setLanguageTextInput( ime_pos );
			}
		}

		//draw label if no text is provided
		//but we should draw it in a different color
		//to give indication that it is not text you typed in
		if (0 == mText.length() && mReadOnly)
		{
			mGLFont->render(mLabel.getWString(), 0,
							mTextLeftEdge, (F32)text_bottom,
							label_color,
							LLFontGL::LEFT,
							LLFontGL::BOTTOM,
							0,
							LLFontGL::NO_SHADOW,
							S32_MAX,
							mTextRightEdge - llround(rendered_pixels_right),
							&rendered_pixels_right, FALSE);
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
							mTextLeftEdge, (F32)text_bottom,
							label_color,
							LLFontGL::LEFT,
							LLFontGL::BOTTOM,
							0,
							LLFontGL::NO_SHADOW,
							S32_MAX,
							mTextRightEdge - llround(rendered_pixels_right),
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
	S32 result = mGLFont->getWidth(mText.getWString().c_str(), mScrollHPos, dpos) + mTextLeftEdge;
	return result;
}

S32 LLLineEditor::calcCursorPos(S32 mouse_x)
{
	const llwchar* wtext = mText.getWString().c_str();
	LLWString asterix_text;
	if (mDrawAsterixes)
	{
		for (S32 i = 0; i < mText.length(); i++)
		{
			asterix_text += utf8str_to_wstring(PASSWORD_ASTERISK);
		}
		wtext = asterix_text.c_str();
	}

	S32 cur_pos = mScrollHPos +
			mGLFont->charFromPixelOffset(
				wtext, mScrollHPos,
				(F32)(mouse_x - mTextLeftEdge),
				(F32)(mTextRightEdge - mTextLeftEdge + 1)); // min-max range is inclusive

	return cur_pos;
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

void LLLineEditor::setPrevalidate(LLTextValidate::validate_func_t func)
{
	mPrevalidateFunc = func;
	updateAllowingLanguageInput();
}

void LLLineEditor::setPrevalidateInput(LLTextValidate::validate_func_t func)
{
	mPrevalidateInputFunc = func;
	updateAllowingLanguageInput();
}

bool LLLineEditor::prevalidateInput(const LLWString& wstr)
{
	if (mPrevalidateInputFunc && !mPrevalidateInputFunc(wstr))
	{
		return false;
	}

	return true;
}

// static
BOOL LLLineEditor::postvalidateFloat(const std::string &str)
{
	LLLocale locale(LLLocale::USER_LOCALE);

	BOOL success = TRUE;
	BOOL has_decimal = FALSE;
	BOOL has_digit = FALSE;

	LLWString trimmed = utf8str_to_wstring(str);
	LLWStringUtil::trim(trimmed);
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
		llwchar decimal_point = (llwchar)LLResMgr::getInstance()->getDecimalPoint();

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

BOOL LLLineEditor::evaluateFloat()
{
	bool success;
	F32 result = 0.f;
	std::string expr = getText();
	LLStringUtil::toUpper(expr);

	success = LLCalc::getInstance()->evalString(expr, result);

	if (!success)
	{
		// Move the cursor to near the error on failure
		setCursor(LLCalc::getInstance()->getLastErrorPos());
		// *TODO: Translated error message indicating the type of error? Select error text?
	}
	else
	{
		// Replace the expression with the result
		std::string result_str = llformat("%f",result);
		setText(result_str);
		selectAll();
	}

	return success;
}

void LLLineEditor::onMouseCaptureLost()
{
	endSelection();
}


void LLLineEditor::setSelectAllonFocusReceived(BOOL b)
{
	mSelectAllonFocusReceived = b;
}

void LLLineEditor::onKeystroke()
{
	if (mKeystrokeCallback)
	{
		mKeystrokeCallback(this);
	}

	mSpellCheckStart = mSpellCheckEnd = -1;
}

void LLLineEditor::setKeystrokeCallback(callback_t callback, void* user_data)
{
	mKeystrokeCallback = boost::bind(callback, _1, user_data);
}


BOOL LLLineEditor::setTextArg( const std::string& key, const LLStringExplicit& text )
{
	mText.setArg(key, text);
	return TRUE;
}

BOOL LLLineEditor::setLabelArg( const std::string& key, const LLStringExplicit& text )
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
	LLWindow* window = getWindow();
	if (!window)
	{
		// test app, no window available
		return;	
	}
	if (hasFocus() && !mReadOnly && !mDrawAsterixes && mPrevalidateFunc == NULL)
	{
		window->allowLanguageTextInput(this, TRUE);
	}
	else
	{
		window->allowLanguageTextInput(this, FALSE);
	}
}

BOOL LLLineEditor::hasPreeditString() const
{
	return (mPreeditPositions.size() > 1);
}

void LLLineEditor::resetPreedit()
{
	if (hasSelection())
	{
		if (hasPreeditString())
		{
			llwarns << "Preedit and selection!" << llendl;
			deselect();
		}
		else
		{
			deleteSelection();
		}
	}
	if (hasPreeditString())
	{
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
	onKeystroke();

	mSpellCheckTimer.setTimerExpirySec(SPELLCHECK_DELAY);
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

void LLLineEditor::setReplaceNewlinesWithSpaces(BOOL replace)
{
	mReplaceNewlinesWithSpaces = replace;
}

LLWString LLLineEditor::getConvertedText() const
{
	LLWString text = getWText();
	LLWStringUtil::trim(text);
	if (!mReplaceNewlinesWithSpaces)
	{
		LLWStringUtil::replaceChar(text,182,'\n'); // Convert paragraph symbols back into newlines.
	}
	return text;
}

void LLLineEditor::showContextMenu(S32 x, S32 y)
{
	LLContextMenu* menu = static_cast<LLContextMenu*>(mContextMenuHandle.get());

	if (menu)
	{
		gEditMenuHandler = this;

		S32 screen_x, screen_y;
		localPointToScreen(x, y, &screen_x, &screen_y);

		setCursorAtLocalPos(x);
		if (hasSelection())
		{
			if ( (mCursorPos < llmin(mSelectionStart, mSelectionEnd)) || (mCursorPos > llmax(mSelectionStart, mSelectionEnd)) )
			{
				deselect();
			}
			else
			{
				setCursor(llmax(mSelectionStart, mSelectionEnd));
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

		menu->setItemVisible("Suggestion Separator", (use_spellcheck) && (!mSuggestionList.empty()));
		menu->setItemVisible("Add to Dictionary", (use_spellcheck) && (is_misspelled));
		menu->setItemVisible("Add to Ignore", (use_spellcheck) && (is_misspelled));
		menu->setItemVisible("Spellcheck Separator", (use_spellcheck) && (is_misspelled));
		menu->show(screen_x, screen_y, this);
	}
}

void LLLineEditor::setContextMenu(LLContextMenu* new_context_menu)
{
	if (new_context_menu)
		mContextMenuHandle = new_context_menu->getHandle();
	else
		mContextMenuHandle.markDead();
}

void LLLineEditor::setFont(const LLFontGL* font)
{
	mGLFont = font;
}

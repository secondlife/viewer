/** 
 * @file lltexteditor.cpp
 * @brief LLTextEditor base class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

// Text editor widget to let users enter a a multi-line ASCII document.

#include "linden_common.h"

#include "lltexteditor.h"

#include "llfontgl.h"
#include "llrender.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llrect.h"
#include "llfocusmgr.h"
#include "lltimer.h"
#include "llmath.h"

#include "audioengine.h"
#include "llclipboard.h"
#include "llscrollbar.h"
#include "llstl.h"
#include "llstring.h"
#include "llkeyboard.h"
#include "llkeywords.h"
#include "llundo.h"
#include "llviewborder.h"
#include "llcontrol.h"
#include "llimagegl.h"
#include "llwindow.h"
#include "lltextparser.h"
#include <queue>

// 
// Globals
//
static LLDefaultWidgetRegistry::Register<LLTextEditor> r("simple_text_editor");

//
// Constants
//
const S32	UI_TEXTEDITOR_LINE_NUMBER_MARGIN = 32;
const S32	UI_TEXTEDITOR_LINE_NUMBER_DIGITS = 4;
const F32	CURSOR_FLASH_DELAY = 1.0f;  // in seconds
const S32	CURSOR_THICKNESS = 2;
const S32	SPACES_PER_TAB = 4;


LLUIColor LLTextEditor::mLinkColor = LLColor4::blue;
void (* LLTextEditor::mURLcallback)(const std::string&)   = NULL;
bool (* LLTextEditor::mSecondlifeURLcallback)(const std::string&)   = NULL;
bool (* LLTextEditor::mSecondlifeURLcallbackRightClick)(const std::string&)   = NULL;


///////////////////////////////////////////////////////////////////

class LLTextEditor::LLTextCmdInsert : public LLTextEditor::LLTextCmd
{
public:
	LLTextCmdInsert(S32 pos, BOOL group_with_next, const LLWString &ws)
		: LLTextCmd(pos, group_with_next), mWString(ws)
	{
	}
	virtual ~LLTextCmdInsert() {}
	virtual BOOL execute( LLTextEditor* editor, S32* delta )
	{
		*delta = insert(editor, getPosition(), mWString );
		LLWStringUtil::truncate(mWString, *delta);
		//mWString = wstring_truncate(mWString, *delta);
		return (*delta != 0);
	}	
	virtual S32 undo( LLTextEditor* editor )
	{
		remove(editor, getPosition(), mWString.length() );
		return getPosition();
	}
	virtual S32 redo( LLTextEditor* editor )
	{
		insert(editor, getPosition(), mWString );
		return getPosition() + mWString.length();
	}

private:
	LLWString mWString;
};

///////////////////////////////////////////////////////////////////
class LLTextEditor::LLTextCmdAddChar : public LLTextEditor::LLTextCmd
{
public:
	LLTextCmdAddChar( S32 pos, BOOL group_with_next, llwchar wc)
		: LLTextCmd(pos, group_with_next), mWString(1, wc), mBlockExtensions(FALSE)
	{
	}
	virtual void blockExtensions()
	{
		mBlockExtensions = TRUE;
	}
	virtual BOOL canExtend(S32 pos) const
	{
		return !mBlockExtensions && (pos == getPosition() + (S32)mWString.length());
	}
	virtual BOOL execute( LLTextEditor* editor, S32* delta )
	{
		*delta = insert(editor, getPosition(), mWString);
		LLWStringUtil::truncate(mWString, *delta);
		//mWString = wstring_truncate(mWString, *delta);
		return (*delta != 0);
	}
	virtual BOOL extendAndExecute( LLTextEditor* editor, S32 pos, llwchar wc, S32* delta )	
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
	virtual S32 undo( LLTextEditor* editor )
	{
		remove(editor, getPosition(), mWString.length() );
		return getPosition();
	}
	virtual S32 redo( LLTextEditor* editor )
	{
		insert(editor, getPosition(), mWString );
		return getPosition() + mWString.length();
	}

private:
	LLWString	mWString;
	BOOL		mBlockExtensions;

};

///////////////////////////////////////////////////////////////////

class LLTextEditor::LLTextCmdOverwriteChar : public LLTextEditor::LLTextCmd
{
public:
	LLTextCmdOverwriteChar( S32 pos, BOOL group_with_next, llwchar wc)
		: LLTextCmd(pos, group_with_next), mChar(wc), mOldChar(0) {}

	virtual BOOL execute( LLTextEditor* editor, S32* delta )
	{ 
		mOldChar = editor->getWChar(getPosition());
		overwrite(editor, getPosition(), mChar);
		*delta = 0;
		return TRUE;
	}	
	virtual S32 undo( LLTextEditor* editor )
	{
		overwrite(editor, getPosition(), mOldChar);
		return getPosition();
	}
	virtual S32 redo( LLTextEditor* editor )
	{
		overwrite(editor, getPosition(), mChar);
		return getPosition()+1;
	}

private:
	llwchar		mChar;
	llwchar		mOldChar;
};

///////////////////////////////////////////////////////////////////

class LLTextEditor::LLTextCmdRemove : public LLTextEditor::LLTextCmd
{
public:
	LLTextCmdRemove( S32 pos, BOOL group_with_next, S32 len ) :
		LLTextCmd(pos, group_with_next), mLen(len)
	{
	}
	virtual BOOL execute( LLTextEditor* editor, S32* delta )
	{ 
		mWString = editor->getWSubString(getPosition(), mLen);
		*delta = remove(editor, getPosition(), mLen );
		return (*delta != 0);
	}
	virtual S32 undo( LLTextEditor* editor )
	{
		insert(editor, getPosition(), mWString );
		return getPosition() + mWString.length();
	}
	virtual S32 redo( LLTextEditor* editor )
	{
		remove(editor, getPosition(), mLen );
		return getPosition();
	}
private:
	LLWString	mWString;
	S32				mLen;
};


///////////////////////////////////////////////////////////////////
LLTextEditor::LLTextEditor(const LLTextEditor::Params& p)
	:	LLUICtrl(p, LLTextViewModelPtr(new LLTextViewModel)),
	mMaxTextByteLength( p.max_text_length ),
	mBaseDocIsPristine(TRUE),
	mPristineCmd( NULL ),
	mLastCmd( NULL ),
	mCursorPos( 0 ),
	mIsSelecting( FALSE ),
	mSelectionStart( 0 ),
	mSelectionEnd( 0 ),
	mScrolledToBottom( TRUE ),
	mOnScrollEndCallback( NULL ),
	mOnScrollEndData( NULL ),
	mCursorColor(		p.cursor_color() ),
	mFgColor(			p.text_color() ),
	mDefaultColor(		p.default_color() ),
	mReadOnlyFgColor(	p.text_readonly_color() ),
	mWriteableBgColor(	p.bg_writeable_color() ),
	mReadOnlyBgColor(	p.bg_readonly_color() ),
	mFocusBgColor(		p.bg_focus_color() ),
	mReadOnly(p.read_only),
	mWordWrap( p.word_wrap ),
	mShowLineNumbers ( FALSE ),
	mCommitOnFocusLost( FALSE ),
	mHideScrollbarForShortDocs( FALSE ),
	mTakesNonScrollClicks( p.takes_non_scroll_clicks ),
	mTrackBottom( p.track_bottom ),
	mAllowEmbeddedItems( p.allow_embedded_items ),
	mAcceptCallingCardNames(FALSE),
	mHandleEditKeysDirectly( FALSE ),
	mMouseDownX(0),
	mMouseDownY(0),
	mLastSelectionX(-1),
	mReflowNeeded(FALSE),
	mScrollNeeded(FALSE),
	mLastSelectionY(-1),
	mTabsToNextField(p.ignore_tab),
	mGLFont(p.font),
	mGLFontStyle(LLFontGL::getStyleFromString(p.font.style))
{
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);

	mSourceID.generate();

	// reset desired x cursor position
	mDesiredXPixel = -1;

	updateTextRect();

	S32 line_height = llround( mGLFont->getLineHeight() );
	S32 page_size = mTextRect.getHeight() / line_height;

	// Init the scrollbar
	LLRect scroll_rect;
	scroll_rect.setOriginAndSize( 
		getRect().getWidth() - scrollbar_size,
		1,
		scrollbar_size,
		getRect().getHeight() - 1);
	S32 lines_in_doc = getLineCount();
	LLScrollbar::Params sbparams;
	sbparams.name("Scrollbar");
	sbparams.rect(scroll_rect);
	sbparams.orientation(LLScrollbar::VERTICAL);
	sbparams.doc_size(lines_in_doc);
	sbparams.doc_pos(0);
	sbparams.page_size(page_size);
	sbparams.follows.flags(FOLLOWS_RIGHT | FOLLOWS_TOP | FOLLOWS_BOTTOM);
	mScrollbar = LLUICtrlFactory::create<LLScrollbar> (sbparams);
	mScrollbar->setOnScrollEndCallback(mOnScrollEndCallback, mOnScrollEndData);
	addChild(mScrollbar);

	static LLUICachedControl<S32> text_editor_border ("UITextEditorBorder", 0);
	LLViewBorder::Params params;
	params.name("text ed border");
	params.rect(getLocalRect());
	params.bevel_type(LLViewBorder::BEVEL_IN);
	params.border_thickness(text_editor_border);
	mBorder = LLUICtrlFactory::create<LLViewBorder> (params);
	addChild( mBorder );
	mBorder->setVisible(!p.hide_border);

	appendText(p.default_text, FALSE, FALSE);

	setHideScrollbarForShortDocs(p.hide_scrollbar);

	mParseHTML=FALSE;
	mHTML.clear();
}

void LLTextEditor::initFromParams( const LLTextEditor::Params& p)
{
	resetDirty();		// Update saved text state
	LLUICtrl::initFromParams(p);
	// HACK: work around enabled == readonly design bug -- RN
	// setEnabled will modify our read only status, so do this after
	// LLUICtrl::initFromParams
	if (p.read_only.isProvided())
	{
		mReadOnly = p.read_only;
		updateSegments();
		updateAllowingLanguageInput();
	}
	// HACK:  text editors always need to be enabled so that we can scroll
	LLView::setEnabled(true);
}

LLTextEditor::~LLTextEditor()
{
	gFocusMgr.releaseFocusIfNeeded( this ); // calls onCommit()

	// Route menu back to the default
	if( gEditMenuHandler == this )
	{
		gEditMenuHandler = NULL;
	}

	// Scrollbar is deleted by LLView
	mHoverSegment = NULL;
	std::for_each(mSegments.begin(), mSegments.end(), DeletePointer());

	std::for_each(mUndoStack.begin(), mUndoStack.end(), DeletePointer());
}

LLTextViewModel* LLTextEditor::getViewModel() const
{
	return (LLTextViewModel*)mViewModel.get();
}

void LLTextEditor::setTrackColor( const LLColor4& color )
{ 
	mScrollbar->setTrackColor(color); 
}

void LLTextEditor::setThumbColor( const LLColor4& color ) 
{ 
	mScrollbar->setThumbColor(color); 
}

void LLTextEditor::updateLineStartList(S32 startpos)
{
	updateSegments();
	
	bindEmbeddedChars(mGLFont);

	S32 seg_num = mSegments.size();
	S32 seg_idx = 0;
	S32 seg_offset = 0;

	if (!mLineStartList.empty())
	{
		getSegmentAndOffset(startpos, &seg_idx, &seg_offset);
		line_info t(seg_idx, seg_offset);
		line_list_t::iterator iter = std::upper_bound(mLineStartList.begin(), mLineStartList.end(), t, line_info_compare());
		if (iter != mLineStartList.begin()) --iter;
		seg_idx = iter->mSegment;
		seg_offset = iter->mOffset;
		mLineStartList.erase(iter, mLineStartList.end());
	}

    LLWString text(getWText());
	while( seg_idx < seg_num )
	{
		mLineStartList.push_back(line_info(seg_idx,seg_offset));
		BOOL line_ended = FALSE;
		S32 start_x = mShowLineNumbers ? UI_TEXTEDITOR_LINE_NUMBER_MARGIN : 0;
		S32 line_width = start_x;
		while(!line_ended && seg_idx < seg_num)
		{
			LLTextSegment* segment = mSegments[seg_idx];
			S32 start_idx = segment->getStart() + seg_offset;
			S32 end_idx = start_idx;
			while (end_idx < segment->getEnd() && text[end_idx] != '\n')
			{
				end_idx++;
			}
			if (start_idx == end_idx)
			{
				if (end_idx >= segment->getEnd())
				{
					// empty segment
					seg_idx++;
					seg_offset = 0;
				}
				else
				{
					// empty line
					line_ended = TRUE;
					seg_offset++;
				}
			}
			else
			{ 
				const llwchar* str = text.c_str() + start_idx;
				S32 drawn = mGLFont->maxDrawableChars(str, (F32)abs(mTextRect.getWidth()) - line_width,
													  end_idx - start_idx, mWordWrap, mAllowEmbeddedItems );
				if( 0 == drawn && line_width == start_x)
				{
					// If at the beginning of a line, draw at least one character, even if it doesn't all fit.
					drawn = 1;
				}
				seg_offset += drawn;
				line_width += mGLFont->getWidth(str, 0, drawn, mAllowEmbeddedItems);
				end_idx = segment->getStart() + seg_offset;
				if (end_idx < segment->getEnd())
				{
					line_ended = TRUE;
					if (text[end_idx] == '\n')
					{
						seg_offset++; // skip newline
					}
				}
				else
				{
					// finished with segment
					seg_idx++;
					seg_offset = 0;
				}
			}
		}
	}
	
	unbindEmbeddedChars(mGLFont);

	mScrollbar->setDocSize( getLineCount() );

	if (mHideScrollbarForShortDocs)
	{
		BOOL short_doc = (mScrollbar->getDocSize() <= mScrollbar->getPageSize());
		mScrollbar->setVisible(!short_doc);
	}

	// if scrolled to bottom, stay at bottom
	// unless user is selecting text
	// do this after updating page size
	if (mScrolledToBottom && mTrackBottom && !hasMouseCapture())
	{
		endOfDoc();
	}
}

////////////////////////////////////////////////////////////
// LLTextEditor
// Public methods

BOOL LLTextEditor::truncate()
{
	BOOL did_truncate = FALSE;

	// First rough check - if we're less than 1/4th the size, we're OK
	if (getLength() >= S32(mMaxTextByteLength / 4))
	{	
		// Have to check actual byte size
        LLWString text(getWText());
		S32 utf8_byte_size = wstring_utf8_length(text);
		if ( utf8_byte_size > mMaxTextByteLength )
		{
			// Truncate safely in UTF-8
			std::string temp_utf8_text = wstring_to_utf8str(text);
			temp_utf8_text = utf8str_truncate( temp_utf8_text, mMaxTextByteLength );
			getViewModel()->setDisplay(utf8str_to_wstring( temp_utf8_text ));
			did_truncate = TRUE;
		}
	}

	return did_truncate;
}

void LLTextEditor::setText(const LLStringExplicit &utf8str)
{
	// LLStringUtil::removeCRLF(utf8str);
	mViewModel->setValue(utf8str_removeCRLF(utf8str));

	truncate();
	blockUndo();

	setCursorPos(0);
	deselect();

	needsReflow();

	resetDirty();
}

void LLTextEditor::setWText(const LLWString &wtext)
{
	getViewModel()->setDisplay(wtext);

	truncate();
	blockUndo();

	setCursorPos(0);
	deselect();

	needsReflow();

	resetDirty();
}

// virtual
void LLTextEditor::setValue(const LLSD& value)
{
	setText(value.asString());
}

std::string LLTextEditor::getText() const
{
	if (mAllowEmbeddedItems)
	{
		llwarns << "getText() called on text with embedded items (not supported)" << llendl;
	}
	return mViewModel->getValue().asString();
}

void LLTextEditor::setWordWrap(BOOL b)
{
	mWordWrap = b; 

	setCursorPos(0);
	deselect();
	
	needsReflow();
}


void LLTextEditor::setBorderVisible(BOOL b)
{
	mBorder->setVisible(b);
}

BOOL LLTextEditor::isBorderVisible() const 
{
	return mBorder->getVisible();
}

void LLTextEditor::setHideScrollbarForShortDocs(BOOL b)
{
	mHideScrollbarForShortDocs = b;

	if (mHideScrollbarForShortDocs)
	{
		BOOL short_doc = (mScrollbar->getDocSize() <= mScrollbar->getPageSize());
		mScrollbar->setVisible(!short_doc);
	}
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
			mCursorPos += search_text.size();
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
	S32 cur_pos = mScrollbar->getDocPos();

	setCursorPos(0);
	selectNext(search_text, case_insensitive, FALSE);

	BOOL replaced = TRUE;
	while ( replaced )
	{
		replaced = replaceText(search_text,replace_text, case_insensitive, FALSE);
	}

	mScrollbar->setDocPos(cur_pos);
}

// Picks a new cursor position based on the screen size of text being drawn.
void LLTextEditor::setCursorAtLocalPos( S32 local_x, S32 local_y, BOOL round )
{
	setCursorPos(getCursorPosFromLocalCoord(local_x, local_y, round));
}

S32 LLTextEditor::prevWordPos(S32 cursorPos) const
{
	LLWString wtext(getWText());
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

S32 LLTextEditor::nextWordPos(S32 cursorPos) const
{
	LLWString wtext(getWText());
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

S32 LLTextEditor::getLineStart( S32 line ) const
{
	S32 num_lines = getLineCount();
	if (num_lines == 0)
    {
		return 0;
    }

	line = llclamp(line, 0, num_lines-1);
	S32 segidx = mLineStartList[line].mSegment;
	S32 segoffset = mLineStartList[line].mOffset;
	LLTextSegment* seg = mSegments[segidx];
	S32 res = seg->getStart() + segoffset;
	if (res > seg->getEnd()) llerrs << "wtf" << llendl;
	return res;
}

// Given an offset into text (pos), find the corresponding line (from the start of the doc) and an offset into the line.
void LLTextEditor::getLineAndOffset( S32 startpos, S32* linep, S32* offsetp ) const
{
	if (mLineStartList.empty())
	{
		*linep = 0;
		*offsetp = startpos;
	}
	else
	{
		S32 seg_idx, seg_offset;
		getSegmentAndOffset( startpos, &seg_idx, &seg_offset );

		line_info tline(seg_idx, seg_offset);
		line_list_t::const_iterator iter = std::upper_bound(mLineStartList.begin(), mLineStartList.end(), tline, line_info_compare());
		if (iter != mLineStartList.begin()) --iter;
		*linep = iter - mLineStartList.begin();
		S32 line_start = mSegments[iter->mSegment]->getStart() + iter->mOffset;
		*offsetp = startpos - line_start;
	}
}

void LLTextEditor::getSegmentAndOffset( S32 startpos, S32* segidxp, S32* offsetp ) const
{
	if (mSegments.empty())
	{
		*segidxp = -1;
		*offsetp = startpos;
	}
	
	LLTextSegment tseg(startpos);
	segment_list_t::const_iterator seg_iter;
	seg_iter = std::upper_bound(mSegments.begin(), mSegments.end(), &tseg, LLTextSegment::compare());
	if (seg_iter != mSegments.begin()) --seg_iter;
	*segidxp = seg_iter - mSegments.begin();
	*offsetp = startpos - (*seg_iter)->getStart();
}

const LLTextSegment*	LLTextEditor::getPreviousSegment() const
{
	// find segment index at character to left of cursor (or rightmost edge of selection)
	S32 idx = llmax(0, getSegmentIdxAtOffset(mCursorPos) - 1);
	return idx >= 0 ? mSegments[idx] : NULL;
}

void LLTextEditor::getSelectedSegments(std::vector<const LLTextSegment*>& segments) const
{
	S32 left = hasSelection() ? llmin(mSelectionStart, mSelectionEnd) : mCursorPos;
	S32 right = hasSelection() ? llmax(mSelectionStart, mSelectionEnd) : mCursorPos;
	S32 first_idx = llmax(0, getSegmentIdxAtOffset(left));
	S32 last_idx = llmax(0, first_idx, getSegmentIdxAtOffset(right));

	for (S32 idx = first_idx; idx <= last_idx; ++idx)
	{
		segments.push_back(mSegments[idx]);
	}
}

S32 LLTextEditor::getCursorPosFromLocalCoord( S32 local_x, S32 local_y, BOOL round ) const
{
	if(mShowLineNumbers)
	{
		local_x -= UI_TEXTEDITOR_LINE_NUMBER_MARGIN;
	}

	// If round is true, if the position is on the right half of a character, the cursor
	// will be put to its right.  If round is false, the cursor will always be put to the
	// character's left.

	// Figure out which line we're nearest to.
	S32 total_lines = getLineCount();
	S32 line_height = llround( mGLFont->getLineHeight() );
	S32 max_visible_lines = mTextRect.getHeight() / line_height;
	S32 scroll_lines = mScrollbar->getDocPos();
	S32 visible_lines = llmin( total_lines - scroll_lines, max_visible_lines );			// Lines currently visible 

	//S32 line = S32( 0.5f + ((mTextRect.mTop - local_y) / mGLFont->getLineHeight()) );
	S32 line = (mTextRect.mTop - 1 - local_y) / line_height;
	if (line >= total_lines)
	{
		return getLength(); // past the end
	}
	
	line = llclamp( line, 0, visible_lines ) + scroll_lines;

	S32 line_start = getLineStart(line);
	S32 next_start = getLineStart(line+1);
	S32	line_end = (next_start != line_start) ? next_start - 1 : getLength();

	if(line_start == -1)
	{
		return 0;
	}
	else
	{
		S32 line_len = line_end - line_start;
		S32 pos;
        LLWString text(getWText());

		if (mAllowEmbeddedItems)
		{
			// Figure out which character we're nearest to.
			bindEmbeddedChars(mGLFont);
			pos = mGLFont->charFromPixelOffset(text.c_str(), line_start,
											   (F32)(local_x - mTextRect.mLeft),
											   (F32)(mTextRect.getWidth()),
											   line_len,
											   round, TRUE);
			unbindEmbeddedChars(mGLFont);
		}
		else
		{
			pos = mGLFont->charFromPixelOffset(text.c_str(), line_start,
											   (F32)(local_x - mTextRect.mLeft),
											   (F32)mTextRect.getWidth(),
											   line_len,
											   round);
		}

		return line_start + pos;
	}
}

void LLTextEditor::setCursor(S32 row, S32 column)
{
    LLWString text(getWText());
	const llwchar* doc = text.c_str();
	const char CR = 10;
	while(row--)
	{
		while (CR != *doc++);
	}
	doc += column;
	setCursorPos(doc - text.c_str());
}

void LLTextEditor::setCursorPos(S32 offset)
{
	mCursorPos = llclamp(offset, 0, (S32)getLength());
	needsScroll();
	// reset desired x cursor position
	mDesiredXPixel = -1;
}

// virtual
BOOL LLTextEditor::canDeselect() const
{
	return hasSelection(); 
}


void LLTextEditor::deselect()
{
	mSelectionStart = 0;
	mSelectionEnd = 0;
	mIsSelecting = FALSE;
}


void LLTextEditor::startSelection()
{
	if( !mIsSelecting )
	{
		mIsSelecting = TRUE;
		mSelectionStart = mCursorPos;
		mSelectionEnd = mCursorPos;
	}
}

void LLTextEditor::endSelection()
{
	if( mIsSelecting )
	{
		mIsSelecting = FALSE;
		mSelectionEnd = mCursorPos;
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

			//text = mWText;

			// Find the next new line
			while( (cur < right) && (text[cur] != '\n') )
			{
				cur++;
			}
		}
		while( cur < right );

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
		mCursorPos = mSelectionEnd;
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
	mCursorPos = mSelectionEnd;
}


BOOL LLTextEditor::handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect_screen)
{
	for ( child_list_const_iter_t child_it = getChildList()->begin();
		  child_it != getChildList()->end(); ++child_it)
	{
		LLView* viewp = *child_it;
		S32 local_x = x - viewp->getRect().mLeft;
		S32 local_y = y - viewp->getRect().mBottom;
		if( viewp->handleToolTip(local_x, local_y, msg, sticky_rect_screen ) )
		{
			return TRUE;
		}
	}

	if( mSegments.empty() )
	{
		return TRUE;
	}

	const LLTextSegment* cur_segment = getSegmentAtLocalPos( x, y );
	if( cur_segment )
	{
		BOOL has_tool_tip = FALSE;
		has_tool_tip = cur_segment->getToolTip( msg );

		if( has_tool_tip )
		{
			// Just use a slop area around the cursor
			// Convert rect local to screen coordinates
			S32 SLOP = 8;
			localPointToScreen( 
				x - SLOP, y - SLOP, 
				&(sticky_rect_screen->mLeft), &(sticky_rect_screen->mBottom) );
			sticky_rect_screen->mRight = sticky_rect_screen->mLeft + 2 * SLOP;
			sticky_rect_screen->mTop = sticky_rect_screen->mBottom + 2 * SLOP;
		}
	}
	return TRUE;
}

BOOL LLTextEditor::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	// Pretend the mouse is over the scrollbar
	return mScrollbar->handleScrollWheel( 0, 0, clicks );
}

BOOL LLTextEditor::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// Let scrollbar have first dibs
	handled = LLView::childrenHandleMouseDown(x, y, mask) != NULL;

	if( !handled && mTakesNonScrollClicks)
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
				setCursorAtLocalPos( x, y, TRUE );

				if (hasSelection())
				{
					/* Mac-like behavior - extend selection towards the cursor
					if (mCursorPos < mSelectionStart
						&& mCursorPos < mSelectionEnd)
					{
						// ...left of selection
						mSelectionStart = llmax(mSelectionStart, mSelectionEnd);
						mSelectionEnd = mCursorPos;
					}
					else if (mCursorPos > mSelectionStart
						&& mCursorPos > mSelectionEnd)
					{
						// ...right of selection
						mSelectionStart = llmin(mSelectionStart, mSelectionEnd);
						mSelectionEnd = mCursorPos;
					}
					else
					{
						mSelectionEnd = mCursorPos;
					}
					*/
					// Windows behavior
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
				setCursorAtLocalPos( x, y, TRUE );
				startSelection();
			}
			gFocusMgr.setMouseCapture( this );
		}

		handled = TRUE;
	}

	if (hasTabStop())
	{
		setFocus( TRUE );
		handled = TRUE;
	}

	// Delay cursor flashing
	resetKeystrokeTimer();

	return handled;
}


BOOL LLTextEditor::handleMiddleMouseDown(S32 x, S32 y, MASK mask)
{
	setFocus( TRUE );
	if( canPastePrimary() )
	{
		setCursorAtLocalPos( x, y, TRUE );
		pastePrimary();
	}
	return TRUE;
}


BOOL LLTextEditor::handleHover(S32 x, S32 y, MASK mask)
{
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);
	BOOL handled = FALSE;

	mHoverSegment = NULL;
	if(hasMouseCapture() )
	{
		if( mIsSelecting ) 
		{
			if (x != mLastSelectionX || y != mLastSelectionY)
			{
				mLastSelectionX = x;
				mLastSelectionY = y;
			}

			if( y > mTextRect.mTop )
			{
				mScrollbar->setDocPos( mScrollbar->getDocPos() - 1 );
			}
			else
			if( y < mTextRect.mBottom )
			{
				mScrollbar->setDocPos( mScrollbar->getDocPos() + 1 );
			}

			setCursorAtLocalPos( x, y, TRUE );
			mSelectionEnd = mCursorPos;
		}

		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (active)" << llendl;		
		getWindow()->setCursor(UI_CURSOR_IBEAM);
		handled = TRUE;
	}

	if( !handled )
	{
		// Pass to children
		handled = LLView::childrenHandleHover(x, y, mask) != NULL;
	}

	if( handled )
	{
		// Delay cursor flashing
		resetKeystrokeTimer();
	}

	// Opaque
	if( !handled && mTakesNonScrollClicks)
	{
		// Check to see if we're over an HTML-style link
		if( !mSegments.empty() )
		{
			const LLTextSegment* cur_segment = getSegmentAtLocalPos( x, y );
			if( cur_segment )
			{
				if(cur_segment->getStyle()->isLink())
				{
					lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (over link, inactive)" << llendl;		
					getWindow()->setCursor(UI_CURSOR_HAND);
					handled = TRUE;
				}
				else
				if(cur_segment->getStyle()->getIsEmbeddedItem())
				{
					lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (over embedded item, inactive)" << llendl;		
					getWindow()->setCursor(UI_CURSOR_HAND);
					//getWindow()->setCursor(UI_CURSOR_ARROW);
					handled = TRUE;
				}
				mHoverSegment = cur_segment;
			}
		}

		if( !handled )
		{
			lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (inactive)" << llendl;		
			if (!mScrollbar->getVisible() || x < getRect().getWidth() - scrollbar_size)
			{
				getWindow()->setCursor(UI_CURSOR_IBEAM);
			}
			else
			{
				getWindow()->setCursor(UI_CURSOR_ARROW);
			}
			handled = TRUE;
		}
	}

	if (mOnScrollEndCallback && mOnScrollEndData && (mScrollbar->getDocPos() == mScrollbar->getDocPosMax()))
	{
		mOnScrollEndCallback(mOnScrollEndData);
	}
	return handled;
}


BOOL LLTextEditor::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// let scrollbar have first dibs
	handled = LLView::childrenHandleMouseUp(x, y, mask) != NULL;

	if( !handled && mTakesNonScrollClicks)
	{
		if( mIsSelecting )
		{
			// Finish selection
			if( y > mTextRect.mTop )
			{
				mScrollbar->setDocPos( mScrollbar->getDocPos() - 1 );
			}
			else
			if( y < mTextRect.mBottom )
			{
				mScrollbar->setDocPos( mScrollbar->getDocPos() + 1 );
			}
			
			setCursorAtLocalPos( x, y, TRUE );
			endSelection();
		}
		
		if( !hasSelection() )
		{
			handleMouseUpOverSegment( x, y, mask );
		}

		// take selection to 'primary' clipboard
		updatePrimary();

		handled = TRUE;
	}

	// Delay cursor flashing
	resetKeystrokeTimer();

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

	// let scrollbar have first dibs
	handled = LLView::childrenHandleDoubleClick(x, y, mask) != NULL;

	if( !handled && mTakesNonScrollClicks)
	{
		setCursorAtLocalPos( x, y, FALSE );
		deselect();

		LLWString text = getWText();
		
		if( isPartOfWord( text[mCursorPos] ) )
		{
			// Select word the cursor is over
			while ((mCursorPos > 0) && isPartOfWord(text[mCursorPos-1]))
			{
				mCursorPos--;
			}
			startSelection();

			while ((mCursorPos < (S32)text.length()) && isPartOfWord( text[mCursorPos] ) )
			{
				mCursorPos++;
			}
		
			mSelectionEnd = mCursorPos;
		}
		else if ((mCursorPos < (S32)text.length()) && !iswspace( text[mCursorPos]) )
		{
			// Select the character the cursor is over
			startSelection();
			mCursorPos++;
			mSelectionEnd = mCursorPos;
		}

		// We don't want handleMouseUp() to "finish" the selection (and thereby
		// set mSelectionEnd to where the mouse is), so we finish the selection here.
		mIsSelecting = FALSE;  

		// delay cursor flashing
		resetKeystrokeTimer();

		// take selection to 'primary' clipboard
		updatePrimary();

		handled = TRUE;
	}

	return handled;
}


// Allow calling cards to be dropped onto text fields.  Append the name and
// a carriage return.
// virtual
BOOL LLTextEditor::handleDragAndDrop(S32 x, S32 y, MASK mask,
					  BOOL drop, EDragAndDropType cargo_type, void *cargo_data,
					  EAcceptance *accept,
					  std::string& tooltip_msg)
{
	*accept = ACCEPT_NO;

	return TRUE;
}

//----------------------------------------------------------------------------
// Returns change in number of characters in mText

S32 LLTextEditor::execute( LLTextCmd* cmd )
{
	S32 delta = 0;
	if( cmd->execute(this, &delta) )
	{
		// Delete top of undo stack
		undo_stack_t::iterator enditer = std::find(mUndoStack.begin(), mUndoStack.end(), mLastCmd);
		if (enditer != mUndoStack.begin())
		{
			--enditer;
			std::for_each(mUndoStack.begin(), enditer, DeletePointer());
			mUndoStack.erase(mUndoStack.begin(), enditer);
		}
		// Push the new command is now on the top (front) of the undo stack.
		mUndoStack.push_front(cmd);
		mLastCmd = cmd;
	}
	else
	{
		// Operation failed, so don't put it on the undo stack.
		delete cmd;
	}

	return delta;
}

S32 LLTextEditor::insert(const S32 pos, const LLWString &wstr, const BOOL group_with_next_op)
{
	return execute( new LLTextCmdInsert( pos, group_with_next_op, wstr ) );
}

S32 LLTextEditor::remove(const S32 pos, const S32 length, const BOOL group_with_next_op)
{
	return execute( new LLTextCmdRemove( pos, group_with_next_op, length ) );
}

S32 LLTextEditor::append(const LLWString &wstr, const BOOL group_with_next_op)
{
	return insert(getLength(), wstr, group_with_next_op);
}

S32 LLTextEditor::overwriteChar(S32 pos, llwchar wc)
{
	if ((S32)getLength() == pos)
	{
		return addChar(pos, wc);
	}
	else
	{
		return execute(new LLTextCmdOverwriteChar(pos, FALSE, wc));
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
			S32 line, offset;
			getLineAndOffset(mCursorPos, &line, &offset);
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
		reportBadKeystroke();
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
		reportBadKeystroke();
	}
}

// Add a single character to the text
S32 LLTextEditor::addChar(S32 pos, llwchar wc)
{
	if ( (wstring_utf8_length( getWText() ) + wchar_utf8_length( wc ))  >= mMaxTextByteLength)
	{
		make_ui_sound("UISndBadKeystroke");
		return 0;
	}

	if (mLastCmd && mLastCmd->canExtend(pos))
	{
		S32 delta = 0;
		mLastCmd->extendAndExecute(this, pos, wc, &delta);
		return delta;
	}
	else
	{
		return execute(new LLTextCmdAddChar(pos, FALSE, wc));
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
				mCursorPos--;
				if( mask & MASK_CONTROL )
				{
					mCursorPos = prevWordPos(mCursorPos);
				}
				mSelectionEnd = mCursorPos;
			}
			break;

		case KEY_RIGHT:
			if( mCursorPos < getLength() )
			{
				startSelection();
				mCursorPos++;
				if( mask & MASK_CONTROL )
				{
					mCursorPos = nextWordPos(mCursorPos);
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
				mCursorPos = 0;
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
				mCursorPos = getLength();
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

	if( !handled && mHandleEditKeysDirectly )
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
			if (mReadOnly)
			{
				mScrollbar->setDocPos(mScrollbar->getDocPos() - 1);
			}
			else
			{
				changeLine( -1 );
			}
			break;

		case KEY_PAGE_UP:
			changePage( -1 );
			break;

		case KEY_HOME:
			if (mReadOnly)
			{
				mScrollbar->setDocPos(0);
			}
			else
			{
				startOfLine();
			}
			break;

		case KEY_DOWN:
			if (mReadOnly)
			{
				mScrollbar->setDocPos(mScrollbar->getDocPos() + 1);
			}
			else
			{
				changeLine( 1 );
			}
			break;

		case KEY_PAGE_DOWN:
			changePage( 1 );
			break;
 
		case KEY_END:
			if (mReadOnly)
			{
				mScrollbar->setDocPos(mScrollbar->getDocPosMax());
			}
			else
			{
				endOfLine();
			}
			break;

		case KEY_LEFT:
			if (mReadOnly)
			{
				break;
			}
			if( hasSelection() )
			{
				setCursorPos(llmin( mCursorPos - 1, mSelectionStart, mSelectionEnd ));
			}
			else
			{
				if( 0 < mCursorPos )
				{
					setCursorPos(mCursorPos - 1);
				}
				else
				{
					reportBadKeystroke();
				}
			}
			break;

		case KEY_RIGHT:
			if (mReadOnly)
			{
				break;
			}
			if( hasSelection() )
			{
				setCursorPos(llmax( mCursorPos + 1, mSelectionStart, mSelectionEnd ));
			}
			else
			{
				if( mCursorPos < getLength() )
				{
					setCursorPos(mCursorPos + 1);
				}
				else
				{
					reportBadKeystroke();
				}
			}	
			break;
			
		default:
			handled = FALSE;
			break;
		}
	}
	
	if (mOnScrollEndCallback && mOnScrollEndData && (mScrollbar->getDocPos() == mScrollbar->getDocPosMax()))
	{
		mOnScrollEndCallback(mOnScrollEndData);
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
	gClipboard.copyFromSubstring( getWText(), left_pos, length, mSourceID );
	deleteSelection( FALSE );

	needsReflow();
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
	gClipboard.copyFromSubstring(getWText(), left_pos, length, mSourceID);
}

BOOL LLTextEditor::canPaste() const
{
	return !mReadOnly && gClipboard.canPasteString();
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

	LLUUID source_id;
	LLWString paste;
	if (is_primary)
	{
		paste = gClipboard.getPastePrimaryWString(&source_id);
	}
	else 
	{
		paste = gClipboard.getPasteWString(&source_id);
	}

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
			if( (wc < LLFont::FIRST_CHAR) && (wc != LF) )
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
	setCursorPos(mCursorPos + insert(mCursorPos, clean_string, FALSE));
	deselect();

	needsReflow();
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
	gClipboard.copyFromPrimarySubstring(getWText(), left_pos, length, mSourceID);
}

BOOL LLTextEditor::canPastePrimary() const
{
	return !mReadOnly && gClipboard.canPastePrimaryString();
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
				mCursorPos = 0;
				mSelectionEnd = mCursorPos;
			}
			else
			{
				// Ctrl-Home, Ctrl-Left, Ctrl-Right, Ctrl-Down
				// all move the cursor as if clicking, so should deselect.
				deselect();
				setCursorPos(0);
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

BOOL LLTextEditor::handleEditKey(const KEY key, const MASK mask)
{
	BOOL handled = FALSE;

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

	return handled;
}

	
BOOL LLTextEditor::handleSpecialKey(const KEY key, const MASK mask, BOOL* return_key_hit)	
{
	*return_key_hit = FALSE;
	BOOL handled = TRUE;

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
			reportBadKeystroke();
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
			
			S32 line, offset;
			getLineAndOffset( mCursorPos, &line, &offset );

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
	BOOL	selection_modified = FALSE;
	BOOL	return_key_hit = FALSE;
	BOOL	text_may_have_changed = TRUE;

	if ( gFocusMgr.getKeyboardFocus() == this )
	{
		// Special case for TAB.  If want to move to next field, report
		// not handled and let the parent take care of field movement.
		if (KEY_TAB == key && mTabsToNextField)
		{
			return FALSE;
		}

		handled = handleNavigationKey( key, mask );
		if( handled )
		{
			text_may_have_changed = FALSE;
		}
			
		if( !handled )
		{
			handled = handleSelectionKey( key, mask );
			if( handled )
			{
				selection_modified = TRUE;
			}
		}
	
		if( !handled )
		{
			handled = handleControlKey( key, mask );
			if( handled )
			{
				selection_modified = TRUE;
			}
		}

		if( !handled && mHandleEditKeysDirectly )
		{
			handled = handleEditKey( key, mask );
			if( handled )
			{
				selection_modified = TRUE;
				text_may_have_changed = TRUE;
			}
		}

		// Handle most keys only if the text editor is writeable.
		if( !mReadOnly )
		{
			if( !handled )
			{
				handled = handleSpecialKey( key, mask, &return_key_hit );
				if( handled )
				{
					selection_modified = TRUE;
					text_may_have_changed = TRUE;
				}
			}

		}

		if( handled )
		{
			resetKeystrokeTimer();

			// Most keystrokes will make the selection box go away, but not all will.
			if( !selection_modified &&
				KEY_SHIFT != key &&
				KEY_CONTROL != key &&
				KEY_ALT != key &&
				KEY_CAPSLOCK )
			{
				deselect();
			}

			if(text_may_have_changed)
			{
				needsReflow();
			}
			needsScroll();
		}
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

	if ( gFocusMgr.getKeyboardFocus() == this )
	{
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
			resetKeystrokeTimer();

			// Most keystrokes will make the selection box go away, but not all will.
			deselect();

			needsReflow();
		}
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
			S32 line, offset;
			getLineAndOffset( mCursorPos, &line, &offset );
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

	needsReflow();
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

	needsReflow();
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

	needsReflow();
}

void LLTextEditor::onFocusReceived()
{
	LLUICtrl::onFocusReceived();
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

	LLUICtrl::onFocusLost();
}

void LLTextEditor::setEnabled(BOOL enabled)
{
	// just treat enabled as read-only flag
	BOOL read_only = !enabled;
	if (read_only != mReadOnly)
	{
		mReadOnly = read_only;
		updateSegments();
		updateAllowingLanguageInput();
	}
}

void LLTextEditor::drawBackground()
{
	S32 left = 0;
	S32 top = getRect().getHeight();
	S32 right = getRect().getWidth();
	S32 bottom = 0;

	LLColor4 bg_color = mReadOnly ? mReadOnlyBgColor.get()
		: gFocusMgr.getKeyboardFocus() == this ? mFocusBgColor.get() : mWriteableBgColor.get();
	if( mShowLineNumbers ) {
		gl_rect_2d(left, top, UI_TEXTEDITOR_LINE_NUMBER_MARGIN, bottom, mReadOnlyBgColor.get() ); // line number area always read-only
		gl_rect_2d(UI_TEXTEDITOR_LINE_NUMBER_MARGIN, top, right, bottom, bg_color); // body text area to the right of line numbers
		gl_rect_2d(UI_TEXTEDITOR_LINE_NUMBER_MARGIN, top, UI_TEXTEDITOR_LINE_NUMBER_MARGIN-1, bottom, LLColor4::grey3); // separator
	} else {
		gl_rect_2d(left, top, right, bottom, bg_color); // body text area
	}

	LLView::draw();
}

// Draws the black box behind the selected text
void LLTextEditor::drawSelectionBackground()
{
	// Draw selection even if we don't have keyboard focus for search/replace
	if( hasSelection() )
	{
		LLWString text = getWText();
		const S32 text_len = getLength();
		std::queue<S32> line_endings;

		S32 line_height = llround( mGLFont->getLineHeight() );

		S32 selection_left		= llmin( mSelectionStart, mSelectionEnd );
		S32 selection_right		= llmax( mSelectionStart, mSelectionEnd );
		S32 selection_left_x	= mTextRect.mLeft;
		S32 selection_left_y	= mTextRect.mTop - line_height;
		S32 selection_right_x	= mTextRect.mRight;
		S32 selection_right_y	= mTextRect.mBottom;

		BOOL selection_left_visible = FALSE;
		BOOL selection_right_visible = FALSE;

		// Skip through the lines we aren't drawing.
		S32 cur_line = mScrollbar->getDocPos();

		S32 left_line_num = cur_line;
		S32 num_lines = getLineCount();
		S32 right_line_num = num_lines - 1;

		S32 line_start = -1;
		if (cur_line >= num_lines)
		{
			return;
		}

		line_start = getLineStart(cur_line);

		S32 left_visible_pos	= line_start;
		S32 right_visible_pos	= line_start;

		S32 text_y = mTextRect.mTop - line_height;

		// Find the coordinates of the selected area
		while((cur_line < num_lines))
		{
			S32 next_line = -1;
			S32 line_end = text_len;
			
			if ((cur_line + 1) < num_lines)
			{
				next_line = getLineStart(cur_line + 1);
				line_end = next_line;

				line_end = ( (line_end - line_start)==0 || text[next_line-1] == '\n' || text[next_line-1] == '\0' || text[next_line-1] == ' ' || text[next_line-1] == '\t'  ) ? next_line-1 : next_line;
			}

			const llwchar* line = text.c_str() + line_start;

			if( line_start <= selection_left && selection_left <= line_end )
			{
				left_line_num = cur_line;
				selection_left_visible = TRUE;
				selection_left_x = mTextRect.mLeft + mGLFont->getWidth(line, 0, selection_left - line_start, mAllowEmbeddedItems);
				selection_left_y = text_y;
			}
			if( line_start <= selection_right && selection_right <= line_end )
			{
				right_line_num = cur_line;
				selection_right_visible = TRUE;
				selection_right_x = mTextRect.mLeft + mGLFont->getWidth(line, 0, selection_right - line_start, mAllowEmbeddedItems);
				if (selection_right == line_end)
				{
					// add empty space for "newline"
					//selection_right_x += mGLFont->getWidth("n");
				}
				selection_right_y = text_y;
			}
			
			// if selection spans end of current line...
			if (selection_left <= line_end && line_end < selection_right && selection_left != selection_right)
			{
				// extend selection slightly beyond end of line
				// to indicate selection of newline character (use "n" character to determine width)
				const LLWString nstr(utf8str_to_wstring(std::string("n")));
				line_endings.push(mTextRect.mLeft + mGLFont->getWidth(line, 0, line_end - line_start, mAllowEmbeddedItems) + mGLFont->getWidth(nstr.c_str()));
			}
			
			// move down one line
			text_y -= line_height;

			right_visible_pos = line_end;
			line_start = next_line;
			cur_line++;

			if (selection_right_visible)
			{
				break;
			}
		}
		
		// Draw the selection box (we're using a box instead of reversing the colors on the selected text).
		BOOL selection_visible = (left_visible_pos <= selection_right) && (selection_left <= right_visible_pos);
		if( selection_visible )
		{
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			const LLColor4& color = mReadOnly ? mReadOnlyBgColor.get() : mWriteableBgColor.get();
			F32 alpha = hasFocus() ? 1.f : 0.5f;
			gGL.color4f( 1.f - color.mV[0], 1.f - color.mV[1], 1.f - color.mV[2], alpha );
			S32 margin_offset = mShowLineNumbers ? UI_TEXTEDITOR_LINE_NUMBER_MARGIN : 0;

			if( selection_left_y == selection_right_y )
			{
				// Draw from selection start to selection end
				gl_rect_2d( selection_left_x + margin_offset, selection_left_y + line_height + 1,
					selection_right_x + margin_offset, selection_right_y);
			}
			else
			{
				// Draw from selection start to the end of the first line
				if( mTextRect.mRight == selection_left_x )
				{
					selection_left_x -= CURSOR_THICKNESS;
				}
				
				S32 line_end = line_endings.front();
				line_endings.pop();
				gl_rect_2d( selection_left_x + margin_offset, selection_left_y + line_height + 1,
					line_end + margin_offset, selection_left_y );

				S32 line_num = left_line_num + 1;
				while(line_endings.size())
				{
					S32 vert_offset = -(line_num - left_line_num) * line_height;
					// Draw the block between the two lines
					gl_rect_2d( mTextRect.mLeft + margin_offset, selection_left_y + vert_offset + line_height + 1,
						line_endings.front() + margin_offset, selection_left_y + vert_offset);
					line_endings.pop();
					line_num++;
				}

				// Draw from the start of the last line to selection end
				if( mTextRect.mLeft == selection_right_x )
				{
					selection_right_x += CURSOR_THICKNESS;
				}
				gl_rect_2d( mTextRect.mLeft + margin_offset, selection_right_y + line_height + 1,
					selection_right_x + margin_offset, selection_right_y );
			}
		}
	}
}

void LLTextEditor::drawCursor()
{
	if( gFocusMgr.getKeyboardFocus() == this
		&& gShowTextEditCursor && !mReadOnly)
	{
		LLWString text = getWText();
		const S32 text_len = getLength();

		// Skip through the lines we aren't drawing.
		S32 cur_pos = mScrollbar->getDocPos();

		S32 num_lines = getLineCount();
		if (cur_pos >= num_lines)
		{
			return;
		}
		S32 line_start = getLineStart(cur_pos);

		F32 line_height = mGLFont->getLineHeight();
		F32 text_y = (F32)(mTextRect.mTop) - line_height;

		F32 cursor_left = 0.f; 
		F32 next_char_left = 0.f;
		F32 cursor_bottom = 0.f;
		BOOL cursor_visible = FALSE;

		S32 line_end = 0;
		// Determine if the cursor is visible and if so what its coordinates are.
		while( (mTextRect.mBottom <= llround(text_y)) && (cur_pos < num_lines))
		{
			line_end = text_len + 1;
			S32 next_line = -1;

			if ((cur_pos + 1) < num_lines)
			{
				next_line = getLineStart(cur_pos + 1);
				line_end = next_line - 1;
			}

			const llwchar* line = text.c_str() + line_start;

			// Find the cursor and selection bounds
			if( line_start <= mCursorPos && mCursorPos <= line_end )
			{
				cursor_visible = TRUE;
				next_char_left = (F32)mTextRect.mLeft + mGLFont->getWidthF32(line, 0, mCursorPos - line_start, mAllowEmbeddedItems );
				cursor_left = next_char_left - 1.f;
				cursor_bottom = text_y;
				break;
			}

			// move down one line
			text_y -= line_height;
			line_start = next_line;
			cur_pos++;
		}

		if(mShowLineNumbers)
		{
			cursor_left += UI_TEXTEDITOR_LINE_NUMBER_MARGIN;
		}

		// Draw the cursor
		if( cursor_visible )
		{
			// (Flash the cursor every half second starting a fixed time after the last keystroke)
			F32 elapsed = mKeystrokeTimer.getElapsedTimeF32();
			if( (elapsed < CURSOR_FLASH_DELAY ) || (S32(elapsed * 2) & 1) )
			{
				F32 cursor_top = cursor_bottom + line_height + 1.f;
				F32 cursor_right = cursor_left + (F32)CURSOR_THICKNESS;
				if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode() && !hasSelection())
				{
					cursor_left += CURSOR_THICKNESS;
					const LLWString space(utf8str_to_wstring(std::string(" ")));
					F32 spacew = mGLFont->getWidthF32(space.c_str());
					if (mCursorPos == line_end)
					{
						cursor_right = cursor_left + spacew;
					}
					else
					{
						F32 width = mGLFont->getWidthF32(text.c_str(), mCursorPos, 1, mAllowEmbeddedItems);
						cursor_right = cursor_left + llmax(spacew, width);
					}
				}
				
				gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

				gGL.color4fv( mCursorColor.get().mV );
				
				gl_rect_2d(llfloor(cursor_left), llfloor(cursor_top),
					llfloor(cursor_right), llfloor(cursor_bottom));

				if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode() && !hasSelection() && text[mCursorPos] != '\n')
				{
					const LLTextSegment* segmentp = getSegmentAtOffset(mCursorPos);
					LLColor4 text_color;
					if (segmentp)
					{
						text_color = segmentp->getColor();
					}
					else if (mReadOnly)
					{
						text_color = mReadOnlyFgColor.get();
					}
					else
					{
						text_color = mFgColor.get();
					}
					mGLFont->render(text, mCursorPos, next_char_left, cursor_bottom + line_height, 
						LLColor4(1.f - text_color.mV[VRED], 1.f - text_color.mV[VGREEN], 1.f - text_color.mV[VBLUE], 1.f),
						LLFontGL::LEFT, LLFontGL::TOP,
						LLFontGL::NORMAL,
						LLFontGL::NO_SHADOW,
						1);
				}

				// Make sure the IME is in the right place
				LLRect screen_pos = calcScreenRect();
				LLCoordGL ime_pos( screen_pos.mLeft + llfloor(cursor_left), screen_pos.mBottom + llfloor(cursor_top) );

				ime_pos.mX = (S32) (ime_pos.mX * LLUI::sGLScaleFactor.mV[VX]);
				ime_pos.mY = (S32) (ime_pos.mY * LLUI::sGLScaleFactor.mV[VY]);
				getWindow()->setLanguageTextInput( ime_pos );
			}
		}
	}
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

	S32 cur_line = mScrollbar->getDocPos();
	if (cur_line >= num_lines)
	{
		return;
	}
		
	const S32 line_height = llround( mGLFont->getLineHeight() );

	S32 line_start = getLineStart(cur_line);
	S32 line_y = mTextRect.mTop - line_height;
	while((mTextRect.mBottom <= line_y) && (num_lines > cur_line))
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

				S32 preedit_left = mTextRect.mLeft;
				if (left > line_start)
				{
					preedit_left += mGLFont->getWidth(text, line_start, left - line_start, mAllowEmbeddedItems);
				}
				S32 preedit_right = mTextRect.mLeft;
				if (right < line_end)
				{
					preedit_right += mGLFont->getWidth(text, line_start, right - line_start, mAllowEmbeddedItems);
				}
				else
				{
					preedit_right += mGLFont->getWidth(text, line_start, line_end - line_start, mAllowEmbeddedItems);
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


void LLTextEditor::drawText()
{
	LLWString text = getWText();
	const S32 text_len = getLength();
	if( text_len <= 0 )
	{
		return;
	}
	S32 selection_left = -1;
	S32 selection_right = -1;
	// Draw selection even if we don't have keyboard focus for search/replace
	if( hasSelection())
	{
		selection_left = llmin( mSelectionStart, mSelectionEnd );
		selection_right = llmax( mSelectionStart, mSelectionEnd );
	}

	LLGLSUIDefault gls_ui;

	S32 cur_line = mScrollbar->getDocPos();
	S32 num_lines = getLineCount();
	if (cur_line >= num_lines)
	{
		return;
	}
	
	S32 line_start = getLineStart(cur_line);
	LLTextSegment t(line_start);
	segment_list_t::iterator seg_iter;
	seg_iter = std::upper_bound(mSegments.begin(), mSegments.end(), &t, LLTextSegment::compare());
	if (seg_iter == mSegments.end() || (*seg_iter)->getStart() > line_start) --seg_iter;
	LLTextSegment* cur_segment = *seg_iter;
	
	S32 line_height = llround( mGLFont->getLineHeight() );
	F32 text_y = (F32)(mTextRect.mTop - line_height);
	while((mTextRect.mBottom <= text_y) && (cur_line < num_lines))
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
		
		F32 text_x = (F32)mTextRect.mLeft;

		S32 seg_start = line_start;
		while( seg_start < line_end )
		{
			while( cur_segment->getEnd() <= seg_start )
			{
				seg_iter++;
				if (seg_iter == mSegments.end())
				{
					llwarns << "Ran off the segmentation end!" << llendl;
					return;
				}
				cur_segment = *seg_iter;
			}
			
			// Draw a segment within the line
			S32 clipped_end	=	llmin( line_end, cur_segment->getEnd() );
			S32 clipped_len =	clipped_end - seg_start;
			if( clipped_len > 0 )
			{
				LLStyleSP style = cur_segment->getStyle();
				if ( style->isImage() && (cur_segment->getStart() >= seg_start) && (cur_segment->getStart() <= clipped_end))
				{
					S32 style_image_height = style->mImageHeight;
					S32 style_image_width = style->mImageWidth;
					LLUIImagePtr image = style->getImage();
					image->draw(llround(text_x), llround(text_y)+line_height-style_image_height, 
						style_image_width, style_image_height);
				}

				if (cur_segment == mHoverSegment && style->getIsEmbeddedItem())
				{
					style->mUnderline = TRUE;
				}

				S32 left_pos = llmin( mSelectionStart, mSelectionEnd );
				
				if ( (mParseHTML) && (left_pos > seg_start) && (left_pos < clipped_end) &&  mIsSelecting && (mSelectionStart == mSelectionEnd) )
				{
					mHTML = style->getLinkHREF();
				}

				drawClippedSegment( text, seg_start, clipped_end, text_x, text_y, selection_left, selection_right, style, &text_x );

				// Note: text_x is incremented by drawClippedSegment()
				seg_start += clipped_len;
			}
		}

			// move down one line
			text_y -= (F32)line_height;

		line_start = next_start;
		cur_line++;
	}
}


// Draws a single text segment, reversing the color for selection if needed.
void LLTextEditor::drawClippedSegment(const LLWString &text, S32 seg_start, S32 seg_end, F32 x, F32 y, S32 selection_left, S32 selection_right, const LLStyleSP& style, F32* right_x )
{
	if (!style->isVisible())
	{
		return;
	}

	const LLFontGL* font = mGLFont;

	LLColor4 color = style->getColor();

	if ( style->getFontString()[0] )
	{
		font = style->getFont();
	}

	U8 font_flags = LLFontGL::NORMAL;
	
	if (style->mBold)
	{
		font_flags |= LLFontGL::BOLD;
	}
	if (style->mItalic)
	{
		font_flags |= LLFontGL::ITALIC;
	}
	if (style->mUnderline)
	{
		font_flags |= LLFontGL::UNDERLINE;
	}

	if (style->getIsEmbeddedItem())
	{
		static LLUIColor text_embedded_item_readonly_color = LLUIColorTable::instance().getColor("TextEmbeddedItemReadOnlyColor");
		static LLUIColor text_embedded_item_color = LLUIColorTable::instance().getColor("TextEmbeddedItemColor");
		if (mReadOnly)
		{
			color = text_embedded_item_readonly_color;
		}
		else
		{
			color = text_embedded_item_color;
		}
	}

	F32 y_top = y + (F32)llround(font->getLineHeight());

  	if( selection_left > seg_start )
	{
		// Draw normally
		S32 start = seg_start;
		S32 end = llmin( selection_left, seg_end );
		S32 length =  end - start;
		font->render(text, start, x, y_top, color, LLFontGL::LEFT, LLFontGL::TOP, mGLFontStyle, LLFontGL::NO_SHADOW, length, S32_MAX, right_x, mAllowEmbeddedItems);
	}
	x = *right_x;
	
	if( (selection_left < seg_end) && (selection_right > seg_start) )
	{
		// Draw reversed
		S32 start = llmax( selection_left, seg_start );
		S32 end = llmin( selection_right, seg_end );
		S32 length = end - start;

		font->render(text, start, x, y_top,
					 LLColor4( 1.f - color.mV[0], 1.f - color.mV[1], 1.f - color.mV[2], 1.f ),
					 LLFontGL::LEFT, LLFontGL::TOP, mGLFontStyle, LLFontGL::NO_SHADOW, length, S32_MAX, right_x, mAllowEmbeddedItems);
	}
	x = *right_x;
	if( selection_right < seg_end )
	{
		// Draw normally
		S32 start = llmax( selection_right, seg_start );
		S32 end = seg_end;
		S32 length = end - start;
		font->render(text, start, x, y_top, color, LLFontGL::LEFT, LLFontGL::TOP, mGLFontStyle, LLFontGL::NO_SHADOW, length, S32_MAX, right_x, mAllowEmbeddedItems);
	}
 }


void LLTextEditor::draw()
{
	// do on-demand reflow 
	if (mReflowNeeded)
	{
		updateLineStartList();
		mReflowNeeded = FALSE;
	}

	// then update scroll position, as cursor may have moved
	if (mScrollNeeded)
	{
		updateScrollFromCursor();
		mScrollNeeded = FALSE; 
	}

	{
		static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);
		LLLocalClipRect clip(LLRect(0, getRect().getHeight(), getRect().getWidth() - (mScrollbar->getVisible() ? scrollbar_size : 0), 0));

		bindEmbeddedChars( mGLFont );

		drawBackground();
		drawSelectionBackground();
		drawPreeditMarker();
		drawText();
		drawCursor();

		unbindEmbeddedChars( mGLFont );

		//RN: the decision was made to always show the orange border for keyboard focus but do not put an insertion caret
		// when in readonly mode
		mBorder->setKeyboardFocusHighlight( gFocusMgr.getKeyboardFocus() == this);// && !mReadOnly);
	}
	
	LLView::draw();  // Draw children (scrollbar and border)

	// remember if we are supposed to be at the bottom of the buffer
	mScrolledToBottom = isScrolledToBottom();
}


void LLTextEditor::onTabInto()
{
	// selecting all on tabInto causes users to hit tab twice and replace their text with a tab character
	// theoretically, one could selectAll if mTabsToNextField is true, but we couldn't think of a use case
	// where you'd want to select all anyway
	// preserve insertion point when returning to the editor
	//selectAll();
}

// virtual
void LLTextEditor::clear()
{
	setText(LLStringUtil::null);
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

	LLUICtrl::setFocus( new_state );

	if( new_state )
	{
		// Route menu to this class
		gEditMenuHandler = this;

		// Don't start the cursor flashing right away
		resetKeystrokeTimer();
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

// virtual
BOOL LLTextEditor::acceptsTextInput() const
{
	return !mReadOnly;
}

// Given a line (from the start of the doc) and an offset into the line, find the offset (pos) into text.
S32 LLTextEditor::getPos( S32 line, S32 offset )
{
	S32 line_start = getLineStart(line);
	S32 next_start = getLineStart(line+1);
	if (next_start == line_start)
	{
		next_start = getLength() + 1;
	}
	S32 line_length = next_start - line_start - 1;
	line_length = llmax(line_length, 0);
	return line_start + llmin( offset, line_length );
}


void LLTextEditor::changePage( S32 delta )
{
	S32 line, offset;
	getLineAndOffset( mCursorPos, &line, &offset );

	// get desired x position to remember previous position
	S32 desired_x_pixel = mDesiredXPixel;

	// allow one line overlap
	S32 page_size = mScrollbar->getPageSize() - 1;
	if( delta == -1 )
	{
		line = llmax( line - page_size, 0);
		setCursorPos(getPos( line, offset ));
		mScrollbar->setDocPos( mScrollbar->getDocPos() - page_size );
	}
	else
	if( delta == 1 )
	{
		setCursorPos(getPos( line + page_size, offset ));
		mScrollbar->setDocPos( mScrollbar->getDocPos() + page_size );
	}

	// put desired position into remember-buffer after setCursorPos()
	mDesiredXPixel = desired_x_pixel;

	if (mOnScrollEndCallback && mOnScrollEndData && (mScrollbar->getDocPos() == mScrollbar->getDocPosMax()))
	{
		mOnScrollEndCallback(mOnScrollEndData);
	}
}

void LLTextEditor::changeLine( S32 delta )
{
	bindEmbeddedChars(mGLFont);

	S32 line, offset;
	getLineAndOffset( mCursorPos, &line, &offset );

	S32  line_start = getLineStart(line);

	// set desired x position to remembered previous position
	S32 desired_x_pixel = mDesiredXPixel;
	// if remembered position was reset (thus -1), calculate new one here
	if( desired_x_pixel == -1 )
	{
        LLWString text(getWText());
		desired_x_pixel = mGLFont->getWidth(text.c_str(), line_start, offset, mAllowEmbeddedItems );
	}

	S32 new_line = 0;
	if( (delta < 0) && (line > 0 ) )
	{
		new_line = line - 1;
	}
	else
	if( (delta > 0) && (line < (getLineCount() - 1)) )
	{
		new_line = line + 1;
	}
	else
	{
		unbindEmbeddedChars(mGLFont);
		return;
	}

	S32 num_lines = getLineCount();
	S32 new_line_start = getLineStart(new_line);
	S32 new_line_end = getLength();
	if (new_line + 1 < num_lines)
	{
		new_line_end = getLineStart(new_line + 1) - 1;
	}

	S32 new_line_len = new_line_end - new_line_start;

	S32 new_offset;
    LLWString text(getWText());
	new_offset = mGLFont->charFromPixelOffset(text.c_str(), new_line_start,
											  (F32)desired_x_pixel,
											  (F32)mTextRect.getWidth(),
											  new_line_len,
											  mAllowEmbeddedItems);

	setCursorPos (getPos( new_line, new_offset ));

	// put desired position into remember-buffer after setCursorPos()
	mDesiredXPixel = desired_x_pixel;
	unbindEmbeddedChars(mGLFont);
}

BOOL LLTextEditor::isScrolledToTop() 
{ 
	return mScrollbar->isAtBeginning(); 
}

BOOL LLTextEditor::isScrolledToBottom() 
{ 
	return mScrollbar->isAtEnd(); 
}


void LLTextEditor::startOfLine()
{
	S32 line, offset;
	getLineAndOffset( mCursorPos, &line, &offset );
	setCursorPos(mCursorPos - offset);
}


// public
void LLTextEditor::setCursorAndScrollToEnd()
{
	deselect();
	endOfDoc();
	needsScroll();
}

void LLTextEditor::getLineAndColumnForPosition( S32 position, S32* line, S32* col, BOOL include_wordwrap )
{
	if( include_wordwrap )
	{
		getLineAndOffset( mCursorPos, line, col );
	}
	else
	{
		LLWString text = getWText();
		S32 line_count = 0;
		S32 line_start = 0;
		S32 i;
		for( i = 0; text[i] && (i < position); i++ )
		{
			if( '\n' == text[i] )
			{
				line_start = i + 1;
				line_count++;
			}
		}
		*line = line_count;
		*col = i - line_start;
	}
}

void LLTextEditor::getCurrentLineAndColumn( S32* line, S32* col, BOOL include_wordwrap ) 
{ 
	getLineAndColumnForPosition(mCursorPos, line, col, include_wordwrap); 
}

S32 LLTextEditor::getCurrentLine()
{
	return getLineForPosition(mCursorPos);
}

S32 LLTextEditor::getLineForPosition(S32 position)
{
	S32 line, col;
	getLineAndColumnForPosition(position, &line, &col, FALSE);
	return line;
}


void LLTextEditor::endOfLine()
{
	S32 line, offset;
	getLineAndOffset( mCursorPos, &line, &offset );
	S32 num_lines = getLineCount();
	if (line + 1 >= num_lines)
	{
		setCursorPos(getLength());
	}
	else
	{
		setCursorPos( getLineStart(line + 1) - 1 );
	}
}

void LLTextEditor::endOfDoc()
{
	mScrollbar->setDocPos(mScrollbar->getDocPosMax());
	mScrolledToBottom = true;

	S32 len = getLength();
	if( len )
	{
		setCursorPos(len);
	}
	if (mOnScrollEndCallback && mOnScrollEndData && (mScrollbar->getDocPos() == mScrollbar->getDocPosMax()))
	{
		mOnScrollEndCallback(mOnScrollEndData);
	}
}

// Sets the scrollbar from the cursor position
void LLTextEditor::updateScrollFromCursor()
{
	mScrollbar->setDocSize( getLineCount() );

	if (mReadOnly)
	{
		// no cursor in read only mode
		return;
	}

	S32 line, offset;
	getLineAndOffset( mCursorPos, &line, &offset ); 

	S32 page_size = mScrollbar->getPageSize();

	if( line < mScrollbar->getDocPos() )
	{
		// scroll so that the cursor is at the top of the page
		mScrollbar->setDocPos( line );
	}
	else if( line >= mScrollbar->getDocPos() + page_size - 1 )
	{
		S32 new_pos = 0;
		if( line < mScrollbar->getDocSize() - 1 )
		{
			// scroll so that the cursor is one line above the bottom of the page,
			new_pos = line - page_size + 1;
		}
		else
		{
			// if there is less than a page of text remaining, scroll so that the cursor is at the bottom
			new_pos = mScrollbar->getDocPosMax();
		}
		mScrollbar->setDocPos( new_pos );
	}

	// Check if we've scrolled to bottom for callback if asked for callback
	if (mOnScrollEndCallback && mOnScrollEndData && (mScrollbar->getDocPos() == mScrollbar->getDocPosMax()))
	{
		mOnScrollEndCallback(mOnScrollEndData);
	}
}

void LLTextEditor::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLView::reshape( width, height, called_from_parent );

	// do this first after reshape, because other things depend on
	// up-to-date mTextRect
	updateTextRect();
	
	needsReflow();

	// propagate shape information to scrollbar
	mScrollbar->setDocSize( getLineCount() );

	S32 line_height = llround( mGLFont->getLineHeight() );
	S32 page_lines = mTextRect.getHeight() / line_height;
	mScrollbar->setPageSize( page_lines );
}

void LLTextEditor::autoIndent()
{
	// Count the number of spaces in the current line
	S32 line, offset;
	getLineAndOffset( mCursorPos, &line, &offset );
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
	addChar( '\n' );
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

	setCursorPos(mCursorPos + insert( mCursorPos, utf8str_to_wstring(new_text), FALSE ));
	
	needsReflow();

	setEnabled( enabled );
}


void LLTextEditor::appendColoredText(const std::string &new_text, 
									 bool allow_undo, 
									 bool prepend_newline,
									 const LLColor4 &color,
									 const std::string& font_name)
{
	LLColor4 lcolor=color;
	if (mParseHighlights)
	{
		LLTextParser* highlight = LLTextParser::getInstance();
		highlight->parseFullLineHighlights(new_text, &lcolor);
	}
	
	LLStyleSP style(new LLStyle);
	style->setVisible(true);
	style->setColor(lcolor);
	style->setFontName(font_name);
	appendStyledText(new_text, allow_undo, prepend_newline, style);
}

void LLTextEditor::appendStyledText(const std::string &new_text, 
									 bool allow_undo, 
									 bool prepend_newline,
									 LLStyleSP stylep)
{
	S32 part = (S32)LLTextParser::WHOLE;
	if(mParseHTML)
	{

		S32 start=0,end=0;
		std::string text = new_text;
		while ( findHTML(text, &start, &end) )
		{
			LLStyleSP html(new LLStyle);
			html->setVisible(true);
			html->setColor(mLinkColor);
			if (stylep)
			{
				html->setFontName(stylep->getFontString());
			}
			html->mUnderline = TRUE;

			if (start > 0)
			{
				if (part == (S32)LLTextParser::WHOLE ||
					part == (S32)LLTextParser::START)
				{
					part = (S32)LLTextParser::START;
				}
				else
				{
					part = (S32)LLTextParser::MIDDLE;
				}
				std::string subtext=text.substr(0,start);
				appendHighlightedText(subtext,allow_undo, prepend_newline, part, stylep); 
			}
			
			html->setLinkHREF(text.substr(start,end-start));
			appendText(text.substr(start, end-start),allow_undo, prepend_newline, html);
			if (end < (S32)text.length()) 
			{
				text = text.substr(end,text.length() - end);
				end=0;
				part=(S32)LLTextParser::END;
			}
			else
			{
				break;
			}
		}
		if (part != (S32)LLTextParser::WHOLE) part=(S32)LLTextParser::END;
		if (end < (S32)text.length()) appendHighlightedText(text,allow_undo, prepend_newline, part, stylep);		
	}
	else
	{
		appendHighlightedText(new_text, allow_undo, prepend_newline, part, stylep);
	}
}

void LLTextEditor::appendHighlightedText(const std::string &new_text, 
										 bool allow_undo, 
										 bool prepend_newline,
										 S32  highlight_part,
										 LLStyleSP stylep)
{
	if (mParseHighlights) 
	{
		LLTextParser* highlight = LLTextParser::getInstance();
		
		if (highlight && stylep)
		{
			LLSD pieces = highlight->parsePartialLineHighlights(new_text, stylep->getColor(), highlight_part);
			bool lprepend=prepend_newline;
			for (S32 i=0;i<pieces.size();i++)
			{
				LLSD color_llsd = pieces[i]["color"];
				LLColor4 lcolor;
				lcolor.setValue(color_llsd);
				LLStyleSP lstylep(new LLStyle(*stylep));
				lstylep->setColor(lcolor);
				if (i != 0 && (pieces.size() > 1) ) lprepend=FALSE;
				appendText((std::string)pieces[i]["text"], allow_undo, lprepend, lstylep);
			}
			return;
		}
	}
	appendText(new_text, allow_undo, prepend_newline, stylep);
}

// Appends new text to end of document
void LLTextEditor::appendText(const std::string &new_text, bool allow_undo, bool prepend_newline,
							  const LLStyleSP stylep)
{
	// Save old state
	BOOL was_scrolled_to_bottom = (mScrollbar->getDocPos() == mScrollbar->getDocPosMax());
	S32 selection_start = mSelectionStart;
	S32 selection_end = mSelectionEnd;
	BOOL was_selecting = mIsSelecting;
	S32 cursor_pos = mCursorPos;
	S32 old_length = getLength();
	BOOL cursor_was_at_end = (mCursorPos == old_length);

	deselect();

	setCursorPos(old_length);

	// Add carriage return if not first line
	if (getLength() != 0
		&& prepend_newline)
	{
		std::string final_text = "\n";
		final_text += new_text;
		append(utf8str_to_wstring(final_text), TRUE);
	}
	else
	{
		append(utf8str_to_wstring(new_text), TRUE );
	}

	if (stylep)
	{
		S32 segment_start = old_length;
		S32 segment_end = getLength();
		LLTextSegment* segment = new LLTextSegment(stylep, segment_start, segment_end );
		mSegments.push_back(segment);
	}
	
	needsReflow();
	
	// Set the cursor and scroll position
	// Maintain the scroll position unless the scroll was at the end of the doc (in which 
	// case, move it to the new end of the doc) or unless the user was doing actively selecting
	if( was_scrolled_to_bottom && !was_selecting )
	{
		if( selection_start != selection_end )
		{
			// maintain an existing non-active selection
			mSelectionStart = selection_start;
			mSelectionEnd = selection_end;
		}	
		endOfDoc();
	}
	else if( selection_start != selection_end )
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

	if( !allow_undo )
	{
		blockUndo();
	}
}

void LLTextEditor::removeTextFromEnd(S32 num_chars)
{
	if (num_chars <= 0) return;

	remove(getLength() - num_chars, num_chars, FALSE);

	S32 len = getLength();
	mCursorPos = llclamp(mCursorPos, 0, len);
	mSelectionStart = llclamp(mSelectionStart, 0, len);
	mSelectionEnd = llclamp(mSelectionEnd, 0, len);

	pruneSegments();
	
	// pruneSegments will invalidate mLineStartList.
	updateLineStartList();
	needsScroll();
}

///////////////////////////////////////////////////////////////////
// Returns change in number of characters in mWText

S32 LLTextEditor::insertStringNoUndo(const S32 pos, const LLWString &wstr)
{
    LLWString text(getWText());
	S32 old_len = text.length();		// length() returns character length
	S32 insert_len = wstr.length();

	text.insert(pos, wstr);
    getViewModel()->setDisplay(text);

	if ( truncate() )
	{
		// The user's not getting everything he's hoping for
		make_ui_sound("UISndBadKeystroke");
		insert_len = getLength() - old_len;
	}

	return insert_len;
}

S32 LLTextEditor::removeStringNoUndo(S32 pos, S32 length)
{
    LLWString text(getWText());
	text.erase(pos, length);
    getViewModel()->setDisplay(text);
	return -length;	// This will be wrong if someone calls removeStringNoUndo with an excessive length
}

S32 LLTextEditor::overwriteCharNoUndo(S32 pos, llwchar wc)
{
	if (pos > (S32)getLength())
	{
		return 0;
	}
    LLWString text(getWText());
	text[pos] = wc;
    getViewModel()->setDisplay(text);
	return 1;
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

		needsReflow();
	}

	return isPristine(); // TRUE => success
}


void LLTextEditor::updateTextRect()
{
	static LLUICachedControl<S32> texteditor_border ("UITextEditorBorder", 0);
	static LLUICachedControl<S32> texteditor_h_pad ("UITextEditorHPad", 0);
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);
	static LLUICachedControl<S32> texteditor_vpad_top ("UITextEditorVPadTop", 0);

	mTextRect.setOriginAndSize( 
		texteditor_border + texteditor_h_pad,
		texteditor_border, 
		getRect().getWidth() - scrollbar_size - 2 * (texteditor_border + texteditor_h_pad),
		getRect().getHeight() - 2 * texteditor_border - texteditor_vpad_top );
}

void LLTextEditor::loadKeywords(const std::string& filename,
								const std::vector<std::string>& funcs,
								const std::vector<std::string>& tooltips,
								const LLColor3& color)
{
	if(mKeywords.loadFromFile(filename))
	{
		S32 count = llmin(funcs.size(), tooltips.size());
		for(S32 i = 0; i < count; i++)
		{
			std::string name = utf8str_trim(funcs[i]);
			mKeywords.addToken(LLKeywordToken::WORD, name, color, tooltips[i] );
		}

		mKeywords.findSegments( &mSegments, getWText(), mDefaultColor.get() );

		llassert( mSegments.front()->getStart() == 0 );
		llassert( mSegments.back()->getEnd() == getLength() );
	}
}

void LLTextEditor::updateSegments()
{
	if (mKeywords.isLoaded())
	{
		// HACK:  No non-ascii keywords for now
		mKeywords.findSegments(&mSegments, getWText(), mDefaultColor.get());
	}
	else if (mAllowEmbeddedItems)
	{
		findEmbeddedItemSegments();
	}

	// Make sure we have at least one segment
	if (mSegments.size() == 1 && mSegments[0]->getIsDefault())
	{
		delete mSegments[0];
		mSegments.clear(); // create default segment
	}
	if (mSegments.empty())
	{
		LLColor4 text_color = ( mReadOnly ? mReadOnlyFgColor.get() : mFgColor.get() );
		LLTextSegment* default_segment = new LLTextSegment( text_color, 0, getLength() );
		default_segment->setIsDefault(TRUE);
		mSegments.push_back(default_segment);
	}
}

// Only effective if text was removed from the end of the editor
// *NOTE: Using this will invalidate references to mSegments from mLineStartList.
void LLTextEditor::pruneSegments()
{
	S32 len = getLength();
	// Find and update the first valid segment
	segment_list_t::iterator iter = mSegments.end();
	while(iter != mSegments.begin())
	{
		--iter;
		LLTextSegment* seg = *iter;
		if (seg->getStart() < len)
		{
			// valid segment
			if (seg->getEnd() > len)
			{
				seg->setEnd(len);
			}
			break; // done
		}			
	}
	if (iter != mSegments.end())
	{
		// erase invalid segments
		++iter;
		std::for_each(iter, mSegments.end(), DeletePointer());
		mSegments.erase(iter, mSegments.end());
	}
	else
	{
		llwarns << "Tried to erase end of empty LLTextEditor" << llendl;
	}
}

void LLTextEditor::findEmbeddedItemSegments()
{
	mHoverSegment = NULL;
	std::for_each(mSegments.begin(), mSegments.end(), DeletePointer());
	mSegments.clear();

	BOOL found_embedded_items = FALSE;
	LLWString text = getWText();
	S32 idx = 0;
	while( text[idx] )
	{
		if( text[idx] >= FIRST_EMBEDDED_CHAR && text[idx] <= LAST_EMBEDDED_CHAR )
 		{
			found_embedded_items = TRUE;
			break;
		}
		++idx;
	}

	if( !found_embedded_items )
	{
		return;
	}

	S32 text_len = text.length();

	BOOL in_text = FALSE;

	LLColor4 text_color = ( mReadOnly ? mReadOnlyFgColor.get() : mFgColor.get()  );

	if( idx > 0 )
	{
		mSegments.push_back( new LLTextSegment( text_color, 0, text_len ) ); // text
		in_text = TRUE;
	}

	LLStyleSP embedded_style(new LLStyle);
	embedded_style->setIsEmbeddedItem( TRUE );

	// Start with i just after the first embedded item
	while ( text[idx] )
	{
		if( text[idx] >= FIRST_EMBEDDED_CHAR && text[idx] <= LAST_EMBEDDED_CHAR )
		{
			if( in_text )
			{
				mSegments.back()->setEnd( idx );
			}
			mSegments.push_back( new LLTextSegment( embedded_style, idx, idx + 1 ) );  // item
			in_text = FALSE;
		}
		else
		if( !in_text )
		{
			mSegments.push_back( new LLTextSegment( text_color, idx, text_len ) );  // text
			in_text = TRUE;
		}
		++idx;
	}
}

BOOL LLTextEditor::handleMouseUpOverSegment(S32 x, S32 y, MASK mask)
{
	if ( hasMouseCapture() )
	{
		// This mouse up was part of a click.
		// Regardless of where the cursor is, see if we recently touched a link
		// and launch it if we did.
		if (mParseHTML && mHTML.length() > 0)
		{
				//Special handling for slurls
			if ( (mSecondlifeURLcallback!=NULL) && !(*mSecondlifeURLcallback)(mHTML) )
			{
				if (mURLcallback!=NULL) (*mURLcallback)(mHTML);
			}
			mHTML.clear();
		}
	}

	return FALSE;
}


// Finds the text segment (if any) at the give local screen position
const LLTextSegment* LLTextEditor::getSegmentAtLocalPos( S32 x, S32 y ) const
{
	// Find the cursor position at the requested local screen position
	S32 offset = getCursorPosFromLocalCoord( x, y, FALSE );
	S32 idx = getSegmentIdxAtOffset(offset);
	return idx >= 0 ? mSegments[idx] : NULL;
}

const LLTextSegment* LLTextEditor::getSegmentAtOffset(S32 offset) const
{
	S32 idx = getSegmentIdxAtOffset(offset);
	return idx >= 0 ? mSegments[idx] : NULL;
}

S32 LLTextEditor::getSegmentIdxAtOffset(S32 offset) const
{
	if (mSegments.empty() || offset < 0 || offset >= getLength())
	{
		return -1;
	}
	else
	{
		S32 segidx, segoff;
		getSegmentAndOffset(offset, &segidx, &segoff);
		return segidx;
	}
}

void LLTextEditor::onMouseCaptureLost()
{
	endSelection();
}

void LLTextEditor::setOnScrollEndCallback(void (*callback)(void*), void* userdata)
{
	mOnScrollEndCallback = callback;
	mOnScrollEndData = userdata;
	mScrollbar->setOnScrollEndCallback(callback, userdata);
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

	setCursorPos(0);
	deselect();

	needsReflow();
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

//////////////////////////////////////////////////////////////////////////
// LLTextSegment

LLTextSegment::LLTextSegment(S32 start) :
	mStart(start),
	mEnd(0),
	mToken(NULL),
	mIsDefault(FALSE)
{
} 
LLTextSegment::LLTextSegment( const LLStyleSP& style, S32 start, S32 end ) :
	mStyle( style ),
	mStart( start),
	mEnd( end ),
	mToken(NULL),
	mIsDefault(FALSE)
{
}
LLTextSegment::LLTextSegment( const LLColor4& color, S32 start, S32 end, BOOL is_visible) :
	mStyle(new LLStyle(is_visible,color,LLStringUtil::null)),
	mStart( start),
	mEnd( end ),
	mToken(NULL),
	mIsDefault(FALSE)
{
}
LLTextSegment::LLTextSegment( const LLColor4& color, S32 start, S32 end ) :
	mStyle(new LLStyle(TRUE, color,LLStringUtil::null )),
	mStart( start),
	mEnd( end ),
	mToken(NULL),
	mIsDefault(FALSE)
{
}
LLTextSegment::LLTextSegment( const LLColor3& color, S32 start, S32 end ) :
	mStyle(new LLStyle(TRUE, color,LLStringUtil::null )),
	mStart( start),
	mEnd( end ),
	mToken(NULL),
	mIsDefault(FALSE)
{
}

BOOL LLTextSegment::getToolTip(std::string& msg) const
{
	if (mToken && !mToken->getToolTip().empty())
	{
		const LLWString& wmsg = mToken->getToolTip();
		msg = wstring_to_utf8str(wmsg);
		return TRUE;
	}
	return FALSE;
}



void LLTextSegment::dump() const
{
	llinfos << "Segment [" << 
//			mColor.mV[VX] << ", " <<
//			mColor.mV[VY] << ", " <<
//			mColor.mV[VZ] << "]\t[" <<
		mStart << ", " <<
		getEnd() << "]" <<
		llendl;

}

///////////////////////////////////////////////////////////////////
// Refactoring note: We may eventually want to replace this with boost::regex or 
// boost::tokenizer capabilities since we've already fixed at least two JIRAs
// concerning logic issues associated with this function.
S32 LLTextEditor::findHTMLToken(const std::string &line, S32 pos, BOOL reverse) const
{
	std::string openers=" \t\n('\"[{<>";
	std::string closers=" \t\n)'\"]}><;";

	if (reverse)
	{
		for (int index=pos; index >= 0; index--)
		{
			char c = line[index];
			S32 m2 = openers.find(c);
			if (m2 >= 0)
			{
				return index+1;
			}
		}
		return 0; // index is -1, don't want to return that. 
	} 
	else
	{
		// adjust the search slightly, to allow matching parenthesis inside the URL
		S32 paren_count = 0;
		for (int index=pos; index<(S32)line.length(); index++)
		{
			char c = line[index];

			if (c == '(')
			{
				paren_count++;
			}
			else if (c == ')')
			{
				if (paren_count <= 0)
				{
					return index;
				}
				else
				{
					paren_count--;
				}
			}
			else
			{
				S32 m2 = closers.find(c);
				if (m2 >= 0)
				{
					return index;
				}
			}
		} 
		return line.length();
	}		
}

BOOL LLTextEditor::findHTML(const std::string &line, S32 *begin, S32 *end) const
{
	  
	S32 m1,m2,m3;
	BOOL matched = FALSE;
	
	m1=line.find("://",*end);
	
	if (m1 >= 0) //Easy match.
	{
		*begin = findHTMLToken(line, m1, TRUE);
		*end   = findHTMLToken(line, m1, FALSE);
		
		//Load_url only handles http and https so don't hilite ftp, smb, etc.
		m2 = line.substr(*begin,(m1 - *begin)).find("http");
		m3 = line.substr(*begin,(m1 - *begin)).find("secondlife");
	
		std::string badneighbors=".,<>?';\"][}{=-+_)(*&^%$#@!~`\t\r\n\\";
	
		if (m2 >= 0 || m3>=0)
		{
			S32 bn = badneighbors.find(line.substr(m1+3,1));
			
			if (bn < 0)
			{
				matched = TRUE;
			}
		}
	}
/*	matches things like secondlife.com (no http://) needs a whitelist to really be effective.
	else	//Harder match.
	{
		m1 = line.find(".",*end);
		
		if (m1 >= 0)
		{
			*end   = findHTMLToken(line, m1, FALSE);
			*begin = findHTMLToken(line, m1, TRUE);
			
			m1 = line.rfind(".",*end);

			if ( ( *end - m1 ) > 2 && m1 > *begin)
			{
				std::string badneighbors=".,<>/?';\"][}{=-+_)(*&^%$#@!~`";
				m2 = badneighbors.find(line.substr(m1+1,1));
				m3 = badneighbors.find(line.substr(m1-1,1));
				if (m3<0 && m2<0)
				{
					matched = TRUE;
				}
			}
		}
	}
	*/
	
	if (matched)
	{
		S32 strpos, strpos2;

		std::string url     = line.substr(*begin,*end - *begin);
		std::string slurlID = "slurl.com/secondlife/";
		strpos = url.find(slurlID);
		
		if (strpos < 0)
		{
			slurlID="secondlife://";
			strpos = url.find(slurlID);
		}
	
		if (strpos < 0)
		{
			slurlID="sl://";
			strpos = url.find(slurlID);
		}
	
		if (strpos >= 0) 
		{
			strpos+=slurlID.length();
			
			while ( ( strpos2=url.find("/",strpos) ) == -1 ) 
			{
				if ((*end+2) >= (S32)line.length() || line.substr(*end,1) != " " )
				{
					matched=FALSE;
					break;
				}
				
				strpos = (*end + 1) - *begin;
								
				*end = findHTMLToken(line,(*begin + strpos),FALSE);
				url = line.substr(*begin,*end - *begin);
			}
		}

	}
	
	if (!matched)
	{
		*begin=*end=0;
	}
	return matched;
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

		mCursorPos = mPreeditPositions.front();
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
		mPreeditOverwrittenWString = getWSubString(insert_preedit_at, mPreeditWString.length());
		removeStringNoUndo(insert_preedit_at, mPreeditWString.length());
	}
	else
	{
		mPreeditOverwrittenWString.clear();
	}
	insertStringNoUndo(insert_preedit_at, mPreeditWString);

	mPreeditStandouts = preedit_standouts;

	needsReflow();
	setCursorPos(insert_preedit_at + caret_position);

	// Update of the preedit should be caused by some key strokes.
	mKeystrokeTimer.reset();
}

BOOL LLTextEditor::getPreeditLocation(S32 query_offset, LLCoordGL *coord, LLRect *bounds, LLRect *control) const
{
	if (control)
	{
		LLRect control_rect_screen;
		localRectToScreen(mTextRect, &control_rect_screen);
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

	const S32 first_visible_line = mScrollbar->getDocPos();
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
	const S32 line_height = llround(mGLFont->getLineHeight());

	if (coord)
	{
		const S32 query_x = mTextRect.mLeft + mGLFont->getWidth(text, current_line_start, query - current_line_start, mAllowEmbeddedItems);
		const S32 query_y = mTextRect.mTop - (current_line - first_visible_line) * line_height - line_height / 2;
		S32 query_screen_x, query_screen_y;
		localPointToScreen(query_x, query_y, &query_screen_x, &query_screen_y);
		LLUI::screenPointToGL(query_screen_x, query_screen_y, &coord->mX, &coord->mY);
	}

	if (bounds)
	{
		S32 preedit_left = mTextRect.mLeft;
		if (preedit_left_position > current_line_start)
		{
			preedit_left += mGLFont->getWidth(text, current_line_start, preedit_left_position - current_line_start, mAllowEmbeddedItems);
		}

		S32 preedit_right = mTextRect.mLeft;
		if (preedit_right_position < current_line_end)
		{
			preedit_right += mGLFont->getWidth(text, current_line_start, preedit_right_position - current_line_start, mAllowEmbeddedItems);
		}
		else
		{
			preedit_right += mGLFont->getWidth(text, current_line_start, current_line_end - current_line_start, mAllowEmbeddedItems);
		}

		const S32 preedit_top = mTextRect.mTop - (current_line - first_visible_line) * line_height;
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
	return llround(mGLFont->getLineHeight() * LLUI::sGLScaleFactor.mV[VY]);
}

LLWString       LLTextEditor::getWText() const
{
    return getViewModel()->getDisplay();
}

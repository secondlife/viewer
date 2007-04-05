/** 
 * @file lltexteditor.cpp
 * @brief LLTextEditor base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// Text editor widget to let users enter a a multi-line ASCII document.

#include "linden_common.h"

#include "lltexteditor.h"

#include "llfontgl.h"
#include "llgl.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llrect.h"
#include "llfocusmgr.h"
#include "sound_ids.h"
#include "lltimer.h"
#include "llmath.h"

#include "audioengine.h"
#include "llclipboard.h"
#include "llscrollbar.h"
#include "llstl.h"
#include "llkeyboard.h"
#include "llkeywords.h"
#include "llundo.h"
#include "llviewborder.h"
#include "llcontrol.h"
#include "llimagegl.h"
#include "llwindow.h"
#include "llglheaders.h"
#include <queue>

// 
// Globals
//

BOOL gDebugTextEditorTips = FALSE;

//
// Constants
//

const S32	UI_TEXTEDITOR_BUFFER_BLOCK_SIZE = 512;

const S32	UI_TEXTEDITOR_BORDER = 1;
const S32	UI_TEXTEDITOR_H_PAD = 4;
const S32	UI_TEXTEDITOR_V_PAD_TOP = 4;
const F32	CURSOR_FLASH_DELAY = 1.0f;  // in seconds
const S32	CURSOR_THICKNESS = 2;
const S32	SPACES_PER_TAB = 4;

LLColor4 LLTextEditor::mLinkColor = LLColor4::blue;
void (* LLTextEditor::mURLcallback)(const char*)              = NULL;
BOOL (* LLTextEditor::mSecondlifeURLcallback)(LLString)   = NULL;

///////////////////////////////////////////////////////////////////
//virtuals
BOOL LLTextCmd::canExtend(S32 pos)
{
	return FALSE;
}

void LLTextCmd::blockExtensions()
{
}

BOOL LLTextCmd::extendAndExecute( LLTextEditor* editor, S32 pos, llwchar c, S32* delta )
{
	llassert(0);
	return 0;
}

BOOL LLTextCmd::hasExtCharValue( llwchar value )
{
	return FALSE;
}

// Utility funcs
S32 LLTextCmd::insert(LLTextEditor* editor, S32 pos, const LLWString &utf8str)
{
	return editor->insertStringNoUndo( pos, utf8str );
}
S32 LLTextCmd::remove(LLTextEditor* editor, S32 pos, S32 length)
{
	return editor->removeStringNoUndo( pos, length );
}
S32 LLTextCmd::overwrite(LLTextEditor* editor, S32 pos, llwchar wc)
{
	return editor->overwriteCharNoUndo(pos, wc);
}

///////////////////////////////////////////////////////////////////

class LLTextCmdInsert : public LLTextCmd 
{
public:
	LLTextCmdInsert(S32 pos, BOOL group_with_next, const LLWString &ws)
		: LLTextCmd(pos, group_with_next), mString(ws)
	{
	}
	virtual BOOL execute( LLTextEditor* editor, S32* delta )
	{
		*delta = insert(editor, mPos, mString );
		LLWString::truncate(mString, *delta);
		//mString = wstring_truncate(mString, *delta);
		return (*delta != 0);
	}	
	virtual S32 undo( LLTextEditor* editor )
	{
		remove(editor, mPos, mString.length() );
		return mPos;
	}
	virtual S32 redo( LLTextEditor* editor )
	{
		insert(editor, mPos, mString );
		return mPos + mString.length();
	}

private:
	LLWString mString;
};

///////////////////////////////////////////////////////////////////

class LLTextCmdAddChar : public LLTextCmd
{
public:
	LLTextCmdAddChar( S32 pos, BOOL group_with_next, llwchar wc)
		: LLTextCmd(pos, group_with_next), mString(1, wc), mBlockExtensions(FALSE)
	{
	}
	virtual void blockExtensions()
	{
		mBlockExtensions = TRUE;
	}
	virtual BOOL canExtend(S32 pos)
	{
		return !mBlockExtensions && (pos == mPos + (S32)mString.length());
	}
	virtual BOOL execute( LLTextEditor* editor, S32* delta )
	{
		*delta = insert(editor, mPos, mString);
		LLWString::truncate(mString, *delta);
		//mString = wstring_truncate(mString, *delta);
		return (*delta != 0);
	}
	virtual BOOL extendAndExecute( LLTextEditor* editor, S32 pos, llwchar wc, S32* delta )	
	{ 
		LLWString ws;
		ws += wc;
		
		*delta = insert(editor, pos, ws);
		if( *delta > 0 )
		{
			mString += wc;
		}
		return (*delta != 0);
	}
	virtual S32 undo( LLTextEditor* editor )
	{
		remove(editor, mPos, mString.length() );
		return mPos;
	}
	virtual S32 redo( LLTextEditor* editor )
	{
		insert(editor, mPos, mString );
		return mPos + mString.length();
	}

private:
	LLWString	mString;
	BOOL		mBlockExtensions;

};

///////////////////////////////////////////////////////////////////

class LLTextCmdOverwriteChar : public LLTextCmd
{
public:
	LLTextCmdOverwriteChar( S32 pos, BOOL group_with_next, llwchar wc)
		: LLTextCmd(pos, group_with_next), mChar(wc), mOldChar(0) {}

	virtual BOOL execute( LLTextEditor* editor, S32* delta )
	{ 
		mOldChar = editor->getWChar(mPos);
		overwrite(editor, mPos, mChar);
		*delta = 0;
		return TRUE;
	}	
	virtual S32 undo( LLTextEditor* editor )
	{
		overwrite(editor, mPos, mOldChar);
		return mPos;
	}
	virtual S32 redo( LLTextEditor* editor )
	{
		overwrite(editor, mPos, mChar);
		return mPos+1;
	}

private:
	llwchar		mChar;
	llwchar		mOldChar;
};

///////////////////////////////////////////////////////////////////

class LLTextCmdRemove : public LLTextCmd
{
public:
	LLTextCmdRemove( S32 pos, BOOL group_with_next, S32 len ) :
		LLTextCmd(pos, group_with_next), mLen(len)
	{
	}
	virtual BOOL execute( LLTextEditor* editor, S32* delta )
	{ 
		mString = editor->getWSubString(mPos, mLen);
		*delta = remove(editor, mPos, mLen );
		return (*delta != 0);
	}
	virtual S32 undo( LLTextEditor* editor )
	{
		insert(editor, mPos, mString );
		return mPos + mString.length();
	}
	virtual S32 redo( LLTextEditor* editor )
	{
		remove(editor, mPos, mLen );
		return mPos;
	}
private:
	LLWString	mString;
	S32				mLen;
};

///////////////////////////////////////////////////////////////////

//
// Member functions
//

LLTextEditor::LLTextEditor(	
	const LLString& name, 
	const LLRect& rect, 
	S32 max_length, 
	const LLString &default_text, 
	const LLFontGL* font,
	BOOL allow_embedded_items)
	:	
	LLUICtrl( name, rect, TRUE, NULL, NULL, FOLLOWS_TOP | FOLLOWS_LEFT ),
	mTextIsUpToDate(TRUE),
	mMaxTextLength( max_length ),
	mBaseDocIsPristine(TRUE),
	mPristineCmd( NULL ),
	mLastCmd( NULL ),
	mCursorPos( 0 ),
	mIsSelecting( FALSE ),
	mSelectionStart( 0 ),
	mSelectionEnd( 0 ),
	mOnScrollEndCallback( NULL ),
	mOnScrollEndData( NULL ),
	mCursorColor(		LLUI::sColorsGroup->getColor( "TextCursorColor" ) ),
	mFgColor(			LLUI::sColorsGroup->getColor( "TextFgColor" ) ),
	mReadOnlyFgColor(	LLUI::sColorsGroup->getColor( "TextFgReadOnlyColor" ) ),
	mWriteableBgColor(	LLUI::sColorsGroup->getColor( "TextBgWriteableColor" ) ),
	mReadOnlyBgColor(	LLUI::sColorsGroup->getColor( "TextBgReadOnlyColor" ) ),
	mFocusBgColor(		LLUI::sColorsGroup->getColor( "TextBgFocusColor" ) ),
	mReadOnly(FALSE),
	mWordWrap( FALSE ),
	mTabToNextField( TRUE ),
	mCommitOnFocusLost( FALSE ),
	mTakesFocus( TRUE ),
	mHideScrollbarForShortDocs( FALSE ),
	mTakesNonScrollClicks( TRUE ),
	mAllowEmbeddedItems( allow_embedded_items ),
	mAcceptCallingCardNames(FALSE),
	mHandleEditKeysDirectly( FALSE ),
	mMouseDownX(0),
	mMouseDownY(0),
	mLastSelectionX(-1),
	mLastSelectionY(-1)
{
	mSourceID.generate();

	if (font)
	{
		mGLFont = font;
	}
	else
	{
		mGLFont = LLFontGL::sSansSerif;
	}

	updateTextRect();

	S32 line_height = llround( mGLFont->getLineHeight() );
	S32 page_size = mTextRect.getHeight() / line_height;

	// Init the scrollbar
	LLRect scroll_rect;
	scroll_rect.setOriginAndSize( 
		mRect.getWidth() - UI_TEXTEDITOR_BORDER - SCROLLBAR_SIZE,
		UI_TEXTEDITOR_BORDER,
		SCROLLBAR_SIZE,
		mRect.getHeight() - 2 * UI_TEXTEDITOR_BORDER );
	S32 lines_in_doc = getLineCount();
	mScrollbar = new LLScrollbar( "Scrollbar", scroll_rect,
		LLScrollbar::VERTICAL,
		lines_in_doc,						
		0,						
		page_size,
		NULL, this );
	mScrollbar->setFollowsRight();
	mScrollbar->setFollowsTop();
	mScrollbar->setFollowsBottom();
	mScrollbar->setEnabled( TRUE );
	mScrollbar->setVisible( TRUE );
	mScrollbar->setOnScrollEndCallback(mOnScrollEndCallback, mOnScrollEndData);
	addChild(mScrollbar);

	mBorder = new LLViewBorder( "text ed border", LLRect(0, mRect.getHeight(), mRect.getWidth(), 0), LLViewBorder::BEVEL_IN, LLViewBorder::STYLE_LINE, UI_TEXTEDITOR_BORDER );
	addChild( mBorder );

	setText(default_text);
	
	mParseHTML=FALSE;
	mHTML="";
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

//virtual
LLString LLTextEditor::getWidgetTag() const
{
	return LL_TEXT_EDITOR_TAG;
}

void LLTextEditor::setTrackColor( const LLColor4& color )
{ 
	mScrollbar->setTrackColor(color); 
}

void LLTextEditor::setThumbColor( const LLColor4& color ) 
{ 
	mScrollbar->setThumbColor(color); 
}

void LLTextEditor::setHighlightColor( const LLColor4& color ) 
{ 
	mScrollbar->setHighlightColor(color); 
}

void LLTextEditor::setShadowColor( const LLColor4& color ) 
{ 
	mScrollbar->setShadowColor(color); 
}

void LLTextEditor::updateLineStartList(S32 startpos)
{
	updateSegments();
	
	bindEmbeddedChars( mGLFont );

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
	
	while( seg_idx < seg_num )
	{
		mLineStartList.push_back(line_info(seg_idx,seg_offset));
		BOOL line_ended = FALSE;
		S32 line_width = 0;
		while(!line_ended && seg_idx < seg_num)
		{
			LLTextSegment* segment = mSegments[seg_idx];
			S32 start_idx = segment->getStart() + seg_offset;
			S32 end_idx = start_idx;
			while (end_idx < segment->getEnd() && mWText[end_idx] != '\n')
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
				const llwchar* str = mWText.c_str() + start_idx;
				S32 drawn = mGLFont->maxDrawableChars(str, (F32)mTextRect.getWidth() - line_width,
													  end_idx - start_idx, mWordWrap, mAllowEmbeddedItems );
				if( 0 == drawn && line_width == 0)
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
					if (mWText[end_idx] == '\n')
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

}

////////////////////////////////////////////////////////////
// LLTextEditor
// Public methods

//static
BOOL LLTextEditor::isPartOfWord(llwchar c) { return (c == '_') || isalnum(c); }



void LLTextEditor::truncate()
{
	if (mWText.size() > (size_t)mMaxTextLength)
	{
		LLWString::truncate(mWText, mMaxTextLength);
		mTextIsUpToDate = FALSE;
	}
}

void LLTextEditor::setText(const LLString &utf8str)
{
	mUTF8Text = utf8str;
	mWText = utf8str_to_wstring(utf8str);
	mTextIsUpToDate = TRUE;

	truncate();
	blockUndo();

	setCursorPos(0);
	deselect();

	updateLineStartList();
	updateScrollFromCursor();
}

void LLTextEditor::setWText(const LLWString &wtext)
{
	mWText = wtext;
	mUTF8Text.clear();
	mTextIsUpToDate = FALSE;

	truncate();
	blockUndo();

	setCursorPos(0);
	deselect();

	updateLineStartList();
	updateScrollFromCursor();
}

void LLTextEditor::setValue(const LLSD& value)
{
	setText(value.asString());
}

const LLString& LLTextEditor::getText() const
{
	if (!mTextIsUpToDate)
	{
		if (mAllowEmbeddedItems)
		{
			llwarns << "getText() called on text with embedded items (not supported)" << llendl;
		}
		mUTF8Text = wstring_to_utf8str(mWText);
		mTextIsUpToDate = TRUE;
	}
	return mUTF8Text;
}

LLSD LLTextEditor::getValue() const
{
	return LLSD(getText());
}

void LLTextEditor::setWordWrap(BOOL b)
{
	mWordWrap = b; 

	setCursorPos(0);
	deselect();
	
	updateLineStartList();
	updateScrollFromCursor();
}


void LLTextEditor::setBorderVisible(BOOL b)
{
	mBorder->setVisible(b);
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

void LLTextEditor::selectNext(const LLString& search_text_in, BOOL case_insensitive, BOOL wrap)
{
	if (search_text_in.empty())
	{
		return;
	}

	LLWString text = getWText();
	LLWString search_text = utf8str_to_wstring(search_text_in);
	if (case_insensitive)
	{
		LLWString::toLower(text);
		LLWString::toLower(search_text);
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

BOOL LLTextEditor::replaceText(const LLString& search_text_in, const LLString& replace_text,
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
			LLWString::toLower(selected_text);
			LLWString::toLower(search_text);
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

void LLTextEditor::replaceTextAll(const LLString& search_text, const LLString& replace_text, BOOL case_insensitive)
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

void LLTextEditor::setTakesNonScrollClicks(BOOL b)
{
	mTakesNonScrollClicks = b;
}


// Picks a new cursor position based on the screen size of text being drawn.
void LLTextEditor::setCursorAtLocalPos( S32 local_x, S32 local_y, BOOL round )
{
	setCursorPos(getCursorPosFromLocalCoord(local_x, local_y, round));
}

S32 LLTextEditor::prevWordPos(S32 cursorPos) const
{
	const LLWString& wtext = mWText;
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
	const LLWString& wtext = mWText;
	while( (cursorPos < getLength()) && isPartOfWord( wtext[cursorPos+1] ) )
	{
		cursorPos++;
	} 
	while( (cursorPos < getLength()) && (wtext[cursorPos+1] == ' ') )
	{
		cursorPos++;
	}
	return cursorPos;
}

S32 LLTextEditor::getLineCount()
{
	return mLineStartList.size();
}

S32 LLTextEditor::getLineStart( S32 line )
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
void LLTextEditor::getLineAndOffset( S32 startpos, S32* linep, S32* offsetp )
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
		line_list_t::iterator iter = std::upper_bound(mLineStartList.begin(), mLineStartList.end(), tline, line_info_compare());
		if (iter != mLineStartList.begin()) --iter;
		*linep = iter - mLineStartList.begin();
		S32 line_start = mSegments[iter->mSegment]->getStart() + iter->mOffset;
		*offsetp = startpos - line_start;
	}
}

void LLTextEditor::getSegmentAndOffset( S32 startpos, S32* segidxp, S32* offsetp )
{
	if (mSegments.empty())
	{
		*segidxp = -1;
		*offsetp = startpos;
	}
	
	LLTextSegment tseg(startpos);
	segment_list_t::iterator seg_iter;
	seg_iter = std::upper_bound(mSegments.begin(), mSegments.end(), &tseg, LLTextSegment::compare());
	if (seg_iter != mSegments.begin()) --seg_iter;
	*segidxp = seg_iter - mSegments.begin();
	*offsetp = startpos - (*seg_iter)->getStart();
}

const LLWString& LLTextEditor::getWText() const
{
	return mWText;
}

S32 LLTextEditor::getLength() const
{
	return mWText.length();
}

llwchar	LLTextEditor::getWChar(S32 pos)
{
	return mWText[pos];
}

LLWString LLTextEditor::getWSubString(S32 pos, S32 len)
{
	return mWText.substr(pos, len);
}

LLTextSegment*	LLTextEditor::getCurrentSegment()
{
	return getSegmentAtOffset(mCursorPos);
}

LLTextSegment*	LLTextEditor::getPreviousSegment()
{
	// find segment index at character to left of cursor (or rightmost edge of selection)
	S32 idx = llmax(0, getSegmentIdxAtOffset(mCursorPos) - 1);
	return idx >= 0 ? mSegments[idx] : NULL;
}

void LLTextEditor::getSelectedSegments(std::vector<LLTextSegment*>& segments)
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

S32 LLTextEditor::getCursorPosFromLocalCoord( S32 local_x, S32 local_y, BOOL round )
{
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

		if (mAllowEmbeddedItems)
		{
			// Figure out which character we're nearest to.
			bindEmbeddedChars(mGLFont);
			pos = mGLFont->charFromPixelOffset(mWText.c_str(), line_start,
											   (F32)(local_x - mTextRect.mLeft),
											   (F32)(mTextRect.getWidth()),
											   line_len,
											   round, TRUE);
			unbindEmbeddedChars(mGLFont);
		}
		else
		{
			pos = mGLFont->charFromPixelOffset(mWText.c_str(), line_start,
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
	const llwchar* doc = mWText.c_str();
	const char CR = 10;
	while(row--)
	{
		while (CR != *doc++);
	}
	doc += column;
	setCursorPos(doc - mWText.c_str());
	updateScrollFromCursor();
}

void LLTextEditor::setCursorPos(S32 offset)
{
	mCursorPos = llclamp(offset, 0, (S32)getLength());
	updateScrollFromCursor();
}


BOOL LLTextEditor::canDeselect()
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
	if (mParseHTML && mHTML.length() > 0)
	{
			//Special handling for slurls
		if ( (mSecondlifeURLcallback!=NULL) && !(*mSecondlifeURLcallback)(mHTML) )
		{
			if (mURLcallback!=NULL) (*mURLcallback)(mHTML.c_str());

			//load_url(url.c_str());
		}
		mHTML="";
	}
}

BOOL LLTextEditor::selectionContainsLineBreaks()
{
	if (hasSelection())
	{
		S32 left = llmin(mSelectionStart, mSelectionEnd);
		S32 right = left + abs(mSelectionStart - mSelectionEnd);

		const LLWString &wtext = mWText;
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
			const LLWString &wtext = mWText;
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
		const LLWString &text = mWText;
		S32 left = llmin( mSelectionStart, mSelectionEnd );
		S32 right = left + abs( mSelectionStart - mSelectionEnd );
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


BOOL LLTextEditor::canSelectAll()
{
	return TRUE;
}

void LLTextEditor::selectAll()
{
	mSelectionStart = getLength();
	mSelectionEnd = 0;
	mCursorPos = mSelectionEnd;
}


BOOL LLTextEditor::handleToolTip(S32 x, S32 y, LLString& msg, LLRect* sticky_rect_screen)
{
	if (pointInView(x, y) && getVisible())
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

		LLTextSegment* cur_segment = getSegmentAtLocalPos( x, y );
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
	return FALSE;
}

BOOL LLTextEditor::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	// Pretend the mouse is over the scrollbar
	if (getVisible())
	{
		return mScrollbar->handleScrollWheel( 0, 0, clicks );
	}
	else
	{
		return FALSE;
	}
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

	if (mTakesFocus)
	{
		setFocus( TRUE );
		handled = TRUE;
	}

	// Delay cursor flashing
	mKeystrokeTimer.reset();

	return handled;
}


BOOL LLTextEditor::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;

	mHoverSegment = NULL;
	if( getVisible() )
	{
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
				
				updateScrollFromCursor();
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
			mKeystrokeTimer.reset();
		}
	
		// Opaque
		if( !handled && mTakesNonScrollClicks)
		{
			// Check to see if we're over an HTML-style link
			if( !mSegments.empty() )
			{
				LLTextSegment* cur_segment = getSegmentAtLocalPos( x, y );
				if( cur_segment )
				{
					if(cur_segment->getStyle().isLink())
					{
						lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (over link, inactive)" << llendl;		
						getWindow()->setCursor(UI_CURSOR_HAND);
						handled = TRUE;
					}
					else
					if(cur_segment->getStyle().getIsEmbeddedItem())
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
				if (!mScrollbar->getVisible() || x < mRect.getWidth() - SCROLLBAR_SIZE)
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

			updateScrollFromCursor();
		}
		
		if( !hasSelection() )
		{
			handleMouseUpOverSegment( x, y, mask );
		}

		handled = TRUE;
	}

	// Delay cursor flashing
	mKeystrokeTimer.reset();

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
		if (mTakesFocus)
		{
			setFocus( TRUE );
		}
		
		setCursorAtLocalPos( x, y, FALSE );
		deselect();

		const LLWString &text = mWText;
		
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
		mKeystrokeTimer.reset();

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
					  LLString& tooltip_msg)
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
	return insert(mWText.length(), wstr, group_with_next_op);
}

S32 LLTextEditor::overwriteChar(S32 pos, llwchar wc)
{
	if ((S32)mWText.length() == pos)
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
	if( getEnabled() )
	{
		if( mCursorPos > 0 )
		{
			S32 chars_to_remove = 1;

			const LLWString &text = mWText;
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
}

// Remove a single character from the text
S32 LLTextEditor::removeChar(S32 pos)
{
	return remove( pos, 1, FALSE );
}

void LLTextEditor::removeChar()
{
	if (getEnabled())
	{
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
}

// Add a single character to the text
S32 LLTextEditor::addChar(S32 pos, llwchar wc)
{
	if ((S32)mWText.length() == mMaxTextLength)
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
	if( getEnabled() )
	{
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
		S32 length = abs( mSelectionStart - mSelectionEnd );
	
		remove( pos, length, group_with_next_op );

		deselect();
		setCursorPos(pos);
	}
}

BOOL LLTextEditor::canCut()
{
	return !mReadOnly && hasSelection();
}

// cut selection to clipboard
void LLTextEditor::cut()
{
	if( canCut() )
	{
		S32 left_pos = llmin( mSelectionStart, mSelectionEnd );
		S32 length = abs( mSelectionStart - mSelectionEnd );
		gClipboard.copyFromSubstring( mWText, left_pos, length, mSourceID );
		deleteSelection( FALSE );

		updateLineStartList();
		updateScrollFromCursor();
	}
}

BOOL LLTextEditor::canCopy()
{
	return hasSelection();
}


// copy selection to clipboard
void LLTextEditor::copy()
{
	if( canCopy() )
	{
		S32 left_pos = llmin( mSelectionStart, mSelectionEnd );
		S32 length = abs( mSelectionStart - mSelectionEnd );
		gClipboard.copyFromSubstring(mWText, left_pos, length, mSourceID);
	}
}

BOOL LLTextEditor::canPaste()
{
	return !mReadOnly && gClipboard.canPasteString();
}


// paste from clipboard
void LLTextEditor::paste()
{
	if (canPaste())
	{
		LLUUID source_id;
		LLWString paste = gClipboard.getPasteWString(&source_id);
		if (!paste.empty())
		{
			// Delete any selected characters (the paste replaces them)
			if( hasSelection() )
			{
				deleteSelection(TRUE);
			}

			// Clean up string (replace tabs and remove characters that our fonts don't support).
			LLWString clean_string(paste);
			LLWString::replaceTabsWithSpaces(clean_string, SPACES_PER_TAB);
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

			updateLineStartList();
			updateScrollFromCursor();
		}
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
		const LLWString &text = mWText;
		if( ' ' == text[ mCursorPos - 1 ] )
		{
			removeCharOrTab();
		}
	}
}


BOOL LLTextEditor::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent )
{
	BOOL	handled = FALSE;
	BOOL	selection_modified = FALSE;
	BOOL	return_key_hit = FALSE;
	BOOL	text_may_have_changed = TRUE;

	if ( (gFocusMgr.getKeyboardFocus() == this) && getVisible())
	{
		// Special case for TAB.  If want to move to next field, report
		// not handled and let the parent take care of field movement.
		if (KEY_TAB == key && mTabToNextField)
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

			if(text_may_have_changed)
			{
				updateLineStartList();
			}
			updateScrollFromCursor();
		}
	}

	return handled;
}


BOOL LLTextEditor::handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent)
{
	if ((uni_char < 0x20) || (uni_char == 0x7F)) // Control character or DEL
	{
		return FALSE;
	}

	BOOL	handled = FALSE;

	if ( (gFocusMgr.getKeyboardFocus() == this) && getVisible())
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
			mKeystrokeTimer.reset();

			// Most keystrokes will make the selection box go away, but not all will.
			deselect();

			updateLineStartList();
			updateScrollFromCursor();
		}
	}

	return handled;
}



BOOL LLTextEditor::canDoDelete()
{
	return !mReadOnly && ( hasSelection() || (mCursorPos < getLength()) );
}

void LLTextEditor::doDelete()
{
	if( canDoDelete() )
	{
		if( hasSelection() )
		{
			deleteSelection(FALSE);
		}
		else
		if( mCursorPos < getLength() )
		{	
			S32 i;
			S32 chars_to_remove = 1;
			const LLWString &text = mWText;
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

		updateLineStartList();
		updateScrollFromCursor();		
	}
}

//----------------------------------------------------------------------------


void LLTextEditor::blockUndo()
{
	mBaseDocIsPristine = FALSE;
	mLastCmd = NULL;
	std::for_each(mUndoStack.begin(), mUndoStack.end(), DeletePointer());
	mUndoStack.clear();
}


BOOL LLTextEditor::canUndo()
{
	return !mReadOnly && mLastCmd != NULL;
}

void LLTextEditor::undo()
{
	if( canUndo() )
	{
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

		updateLineStartList();
		updateScrollFromCursor();
	}
}

BOOL LLTextEditor::canRedo()
{
	return !mReadOnly && (mUndoStack.size() > 0) && (mLastCmd != mUndoStack.front());
}

void LLTextEditor::redo()
{
	if( canRedo() )
	{
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

		updateLineStartList();
		updateScrollFromCursor();
	}
}


// virtual, from LLView
void LLTextEditor::onFocusLost()
{
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
	}
}

void LLTextEditor::drawBackground()
{
	S32 left = 0;
	S32 top = mRect.getHeight();
	S32 right = mRect.getWidth();
	S32 bottom = 0;

	LLColor4 bg_color = mReadOnlyBgColor;

	if( !mReadOnly )
	{
		if (gFocusMgr.getKeyboardFocus() == this)
		{
			bg_color = mFocusBgColor;
		}
		else
		{
			bg_color = mWriteableBgColor;
		}
	}
	gl_rect_2d(left, top, right, bottom, bg_color);

	LLView::draw();
}

// Draws the black box behind the selected text
void LLTextEditor::drawSelectionBackground()
{
	// Draw selection even if we don't have keyboard focus for search/replace
	if( hasSelection() )
	{
		const LLWString &text = mWText;
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
				const LLWString nstr(utf8str_to_wstring(LLString("n")));
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
			LLGLSNoTexture no_texture;
			const LLColor4& color = mReadOnly ? mReadOnlyBgColor : mWriteableBgColor;
			glColor3f( 1.f - color.mV[0], 1.f - color.mV[1], 1.f - color.mV[2] );

			if( selection_left_y == selection_right_y )
			{
				// Draw from selection start to selection end
				gl_rect_2d( selection_left_x, selection_left_y + line_height + 1,
					selection_right_x, selection_right_y);
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
				gl_rect_2d( selection_left_x, selection_left_y + line_height + 1,
					line_end, selection_left_y );

				S32 line_num = left_line_num + 1;
				while(line_endings.size())
				{
					S32 vert_offset = -(line_num - left_line_num) * line_height;
					// Draw the block between the two lines
					gl_rect_2d( mTextRect.mLeft, selection_left_y + vert_offset + line_height + 1,
						line_endings.front(), selection_left_y + vert_offset);
					line_endings.pop();
					line_num++;
				}

				// Draw from the start of the last line to selection end
				if( mTextRect.mLeft == selection_right_x )
				{
					selection_right_x += CURSOR_THICKNESS;
				}
				gl_rect_2d( mTextRect.mLeft, selection_right_y + line_height + 1,
					selection_right_x, selection_right_y );
			}
		}
	}
}

void LLTextEditor::drawCursor()
{
	if( gFocusMgr.getKeyboardFocus() == this
		&& gShowTextEditCursor && !mReadOnly)
	{
		const LLWString &text = mWText;
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
					const LLWString space(utf8str_to_wstring(LLString(" ")));
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
				
				LLGLSNoTexture no_texture;

				glColor4fv( mCursorColor.mV );
				
				gl_rect_2d(llfloor(cursor_left), llfloor(cursor_top),
					llfloor(cursor_right), llfloor(cursor_bottom));

				if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode() && !hasSelection() && text[mCursorPos] != '\n')
				{
					LLTextSegment* segmentp = getSegmentAtOffset(mCursorPos);
					LLColor4 text_color;
					if (segmentp)
					{
						text_color = segmentp->getColor();
					}
					else if (mReadOnly)
					{
						text_color = mReadOnlyFgColor;
					}
					else
					{
						text_color = mFgColor;
					}
					LLGLSTexture texture;
					mGLFont->render(text, mCursorPos, next_char_left, cursor_bottom + line_height, 
						LLColor4(1.f - text_color.mV[VRED], 1.f - text_color.mV[VGREEN], 1.f - text_color.mV[VBLUE], 1.f),
						LLFontGL::LEFT, LLFontGL::TOP,
						LLFontGL::NORMAL,
						1);
				}


			}
		}
	}
}


void LLTextEditor::drawText()
{
	const LLWString &text = mWText;
	const S32 text_len = getLength();

	if( text_len > 0 )
	{
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
					LLStyle style = cur_segment->getStyle();
					if ( style.isImage() && (cur_segment->getStart() >= seg_start) && (cur_segment->getStart() <= clipped_end))
					{
						LLImageGL *image = style.getImage();

						gl_draw_scaled_image( llround(text_x), llround(text_y)+line_height-style.mImageHeight, style.mImageWidth, style.mImageHeight, image, LLColor4::white );
						
					}

					if (cur_segment == mHoverSegment && style.getIsEmbeddedItem())
					{
						style.mUnderline = TRUE;
					}

					S32 left_pos = llmin( mSelectionStart, mSelectionEnd );
					
					if ( (mParseHTML) && (left_pos > seg_start) && (left_pos < clipped_end) &&  mIsSelecting && (mSelectionStart == mSelectionEnd) )
					{
						mHTML = style.getLinkHREF();
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
}

// Draws a single text segment, reversing the color for selection if needed.
void LLTextEditor::drawClippedSegment(const LLWString &text, S32 seg_start, S32 seg_end, F32 x, F32 y, S32 selection_left, S32 selection_right, const LLStyle& style, F32* right_x )
{
	const LLFontGL* font = mGLFont;

	LLColor4 color;

	if (!style.isVisible())
	{
		return;
	}

	color = style.getColor();

	if ( style.getFontString()[0] )
	{
		font = gResMgr->getRes(style.getFontID());
	}

	U8 font_flags = LLFontGL::NORMAL;
	
	if (style.mBold)
	{
		font_flags |= LLFontGL::BOLD;
	}
	if (style.mItalic)
	{
		font_flags |= LLFontGL::ITALIC;
	}
	if (style.mUnderline)
	{
		font_flags |= LLFontGL::UNDERLINE;
	}

	if (style.getIsEmbeddedItem())
	{
		if (mReadOnly)
		{
			color = LLUI::sColorsGroup->getColor("TextEmbeddedItemReadOnlyColor");
		}
		else
		{
			color = LLUI::sColorsGroup->getColor("TextEmbeddedItemColor");
		}
	}

	F32 y_top = y + (F32)llround(font->getLineHeight());

  	if( selection_left > seg_start )
	{
		// Draw normally
		S32 start = seg_start;
		S32 end = llmin( selection_left, seg_end );
		S32 length =  end - start;
		font->render(text, start, x, y_top, color, LLFontGL::LEFT, LLFontGL::TOP, font_flags, length, S32_MAX, right_x, mAllowEmbeddedItems);
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
					 LLFontGL::LEFT, LLFontGL::TOP, font_flags, length, S32_MAX, right_x, mAllowEmbeddedItems);
	}
	x = *right_x;
	if( selection_right < seg_end )
	{
		// Draw normally
		S32 start = llmax( selection_right, seg_start );
		S32 end = seg_end;
		S32 length = end - start;
		font->render(text, start, x, y_top, color, LLFontGL::LEFT, LLFontGL::TOP, font_flags, length, S32_MAX, right_x, mAllowEmbeddedItems);
	}
 }


void LLTextEditor::draw()
{
	if( getVisible() )
	{
		{
			LLGLEnable scissor_test(GL_SCISSOR_TEST);
			LLUI::setScissorRegionLocal(LLRect(0, mRect.getHeight(), mRect.getWidth() - (mScrollbar->getVisible() ? SCROLLBAR_SIZE : 0), 0));

			bindEmbeddedChars( mGLFont );

			drawBackground();
			drawSelectionBackground();
			drawText();
			drawCursor();

			unbindEmbeddedChars( mGLFont );

			//RN: the decision was made to always show the orange border for keyboard focus but do not put an insertion caret
			// when in readonly mode
			mBorder->setKeyboardFocusHighlight( gFocusMgr.getKeyboardFocus() == this);// && !mReadOnly);
		}
		LLView::draw();  // Draw children (scrollbar and border)
	}
}

void LLTextEditor::reportBadKeystroke()
{
	make_ui_sound("UISndBadKeystroke");
}


void LLTextEditor::onTabInto()
{
	// selecting all on tabInto causes users to hit tab twice and replace their text with a tab character
	// theoretically, one could selectAll if mTabToNextField is true, but we couldn't think of a use case
	// where you'd want to select all anyway
	// preserve insertion point when returning to the editor
	//selectAll();
}

void LLTextEditor::clear()
{
	setText("");
}

// Start or stop the editor from accepting text-editing keystrokes
// see also LLLineEditor
void LLTextEditor::setFocus( BOOL new_state )
{
	BOOL old_state = hasFocus();

	// Don't change anything if the focus state didn't change
	if (new_state == old_state) return;

	LLUICtrl::setFocus( new_state );

	if( new_state )
	{
		// Route menu to this class
		gEditMenuHandler = this;

		// Don't start the cursor flashing right away
		mKeystrokeTimer.reset();
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
	if (mOnScrollEndCallback && mOnScrollEndData && (mScrollbar->getDocPos() == mScrollbar->getDocPosMax()))
	{
		mOnScrollEndCallback(mOnScrollEndData);
	}
}

void LLTextEditor::changeLine( S32 delta )
{
	bindEmbeddedChars( mGLFont );

	S32 line, offset;
	getLineAndOffset( mCursorPos, &line, &offset );

	S32  line_start = getLineStart(line);

	S32 desired_x_pixel;
	
	desired_x_pixel = mGLFont->getWidth(mWText.c_str(), line_start, offset, mAllowEmbeddedItems );

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
		unbindEmbeddedChars( mGLFont );
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
	new_offset = mGLFont->charFromPixelOffset(mWText.c_str(), new_line_start,
											  (F32)desired_x_pixel,
											  (F32)mTextRect.getWidth(),
											  new_line_len,
											  mAllowEmbeddedItems);

	setCursorPos (getPos( new_line, new_offset ));
	unbindEmbeddedChars( mGLFont );
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
	updateScrollFromCursor();
}


void LLTextEditor::getCurrentLineAndColumn( S32* line, S32* col, BOOL include_wordwrap )
{
	if( include_wordwrap )
	{
		getLineAndOffset( mCursorPos, line, col );
	}
	else
	{
		const LLWString &text = mWText;
		S32 line_count = 0;
		S32 line_start = 0;
		S32 i;
		for( i = 0; text[i] && (i < mCursorPos); i++ )
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
	mScrollbar->setDocPos( mScrollbar->getDocPosMax() );
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

	updateTextRect();

	S32 line_height = llround( mGLFont->getLineHeight() );
	S32 page_lines = mTextRect.getHeight() / line_height;
	mScrollbar->setPageSize( page_lines );

	updateLineStartList();
}

void LLTextEditor::autoIndent()
{
	// Count the number of spaces in the current line
	S32 line, offset;
	getLineAndOffset( mCursorPos, &line, &offset );
	S32 line_start = getLineStart(line);
	S32 space_count = 0;
	S32 i;

	const LLWString &text = mWText;
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
void LLTextEditor::insertText(const LLString &new_text)
{
	BOOL enabled = getEnabled();
	setEnabled( TRUE );

	// Delete any selected characters (the insertion replaces them)
	if( hasSelection() )
	{
		deleteSelection(TRUE);
	}

	setCursorPos(mCursorPos + insert( mCursorPos, utf8str_to_wstring(new_text), FALSE ));
	
	updateLineStartList();
	updateScrollFromCursor();

	setEnabled( enabled );
}


void LLTextEditor::appendColoredText(const LLString &new_text, 
									 bool allow_undo, 
									 bool prepend_newline,
									 const LLColor4 &color,
									 const LLString& font_name)
{
	LLStyle style;
	style.setVisible(true);
	style.setColor(color);
	style.setFontName(font_name);
	if(mParseHTML)
	{

		S32 start=0,end=0;
		LLString text = new_text;
		while ( findHTML(text, &start, &end) )
		{
			LLStyle html;
			html.setVisible(true);
			html.setColor(mLinkColor);
			html.setFontName(font_name);
			html.mUnderline = TRUE;

			if (start > 0) appendText(text.substr(0,start),allow_undo, prepend_newline, &style);
			html.setLinkHREF(text.substr(start,end-start));
			appendText(text.substr(start, end-start),allow_undo, prepend_newline, &html);
			if (end < (S32)text.length()) 
			{
				text = text.substr(end,text.length() - end);
				end=0;
			}
			else
			{
				break;
			}
		}
		if (end < (S32)text.length()) appendText(text,allow_undo, prepend_newline, &style);		
	}
	else
	{
		appendText(new_text, allow_undo, prepend_newline, &style);
	}
}

void LLTextEditor::appendStyledText(const LLString &new_text, 
									 bool allow_undo, 
									 bool prepend_newline,
									 const LLStyle &style)
{
	appendText(new_text, allow_undo, prepend_newline, &style);
}

// Appends new text to end of document
void LLTextEditor::appendText(const LLString &new_text, bool allow_undo, bool prepend_newline,
							  const LLStyle* segment_style)
{
	// Save old state
	BOOL was_scrolled_to_bottom = (mScrollbar->getDocPos() == mScrollbar->getDocPosMax());
	S32 selection_start = mSelectionStart;
	S32 selection_end = mSelectionEnd;
	S32 cursor_pos = mCursorPos;
	S32 old_length = getLength();
	BOOL cursor_was_at_end = (mCursorPos == old_length);

	deselect();

	setCursorPos(old_length);

	// Add carriage return if not first line
	if (getLength() != 0
		&& prepend_newline)
	{
		LLString final_text = "\n";
		final_text += new_text;
		append(utf8str_to_wstring(final_text), TRUE);
	}
	else
	{
		append(utf8str_to_wstring(new_text), TRUE );
	}

	if (segment_style)
	{
		S32 segment_start = old_length;
		S32 segment_end = getLength();
		LLTextSegment* segment = new LLTextSegment(*segment_style, segment_start, segment_end );
		mSegments.push_back(segment);
	}
	
	updateLineStartList(old_length);
	
	// Set the cursor and scroll position
	// Maintain the scroll position unless the scroll was at the end of the doc 
	//	  (in which case, move it to the new end of the doc)
	if( was_scrolled_to_bottom )
	{
		endOfDoc();
	}
	else if( selection_start != selection_end )
	{
		mSelectionStart = selection_start;

		mSelectionEnd = selection_end;
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
	updateLineStartList();
}

///////////////////////////////////////////////////////////////////
// Returns change in number of characters in mWText

S32 LLTextEditor::insertStringNoUndo(const S32 pos, const LLWString &wstr)
{
	S32 len = mWText.length();
	S32 s_len = wstr.length();
	S32 new_len = len + s_len;
	if( new_len > mMaxTextLength )
	{
		new_len = mMaxTextLength;

		// The user's not getting everything he's hoping for
		make_ui_sound("UISndBadKeystroke");
	}

	mWText.insert(pos, wstr);
	mTextIsUpToDate = FALSE;
	truncate();

	return new_len - len;
}

S32 LLTextEditor::removeStringNoUndo(S32 pos, S32 length)
{
	mWText.erase(pos, length);
	mTextIsUpToDate = FALSE;
	return -length;	// This will be wrong if someone calls removeStringNoUndo with an excessive length
}

S32 LLTextEditor::overwriteCharNoUndo(S32 pos, llwchar wc)
{
	if (pos > (S32)mWText.length())
	{
		return 0;
	}
	mWText[pos] = wc;
	mTextIsUpToDate = FALSE;
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

		updateLineStartList();
		updateScrollFromCursor();
	}

	return isPristine(); // TRUE => success
}



void LLTextEditor::updateTextRect()
{
	mTextRect.setOriginAndSize( 
		UI_TEXTEDITOR_BORDER + UI_TEXTEDITOR_H_PAD,
		UI_TEXTEDITOR_BORDER, 
		mRect.getWidth() - SCROLLBAR_SIZE - 2 * (UI_TEXTEDITOR_BORDER + UI_TEXTEDITOR_H_PAD),
		mRect.getHeight() - 2 * UI_TEXTEDITOR_BORDER - UI_TEXTEDITOR_V_PAD_TOP );
}

void LLTextEditor::loadKeywords(const LLString& filename,
								const LLDynamicArray<const char*>& funcs,
								const LLDynamicArray<const char*>& tooltips,
								const LLColor3& color)
{
	if(mKeywords.loadFromFile(filename))
	{
		S32 count = funcs.count();
		LLString name;
		for(S32 i = 0; i < count; i++)
		{
			name = funcs.get(i);
			name = utf8str_trim(name);
			mKeywords.addToken(LLKeywordToken::WORD, name.c_str(), color, tooltips.get(i) );
		}

		mKeywords.findSegments( &mSegments, mWText );

		llassert( mSegments.front()->getStart() == 0 );
		llassert( mSegments.back()->getEnd() == getLength() );
	}
}

void LLTextEditor::updateSegments()
{
	if (mKeywords.isLoaded())
	{
		// HACK:  No non-ascii keywords for now
		mKeywords.findSegments(&mSegments, mWText);
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
		LLColor4& text_color = ( mReadOnly ? mReadOnlyFgColor : mFgColor );
		LLTextSegment* default_segment = new LLTextSegment( text_color, 0, mWText.length() );
		default_segment->setIsDefault(TRUE);
		mSegments.push_back(default_segment);
	}
}

// Only effective if text was removed from the end of the editor
void LLTextEditor::pruneSegments()
{
	S32 len = mWText.length();
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
	// erase invalid segments
	++iter;
	std::for_each(iter, mSegments.end(), DeletePointer());
	mSegments.erase(iter, mSegments.end());
}

void LLTextEditor::findEmbeddedItemSegments()
{
	mHoverSegment = NULL;
	std::for_each(mSegments.begin(), mSegments.end(), DeletePointer());
	mSegments.clear();

	BOOL found_embedded_items = FALSE;
	const LLWString &text = mWText;
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

	LLColor4& text_color = ( mReadOnly ? mReadOnlyFgColor : mFgColor  );

	if( idx > 0 )
	{
		mSegments.push_back( new LLTextSegment( text_color, 0, text_len ) ); // text
		in_text = TRUE;
	}

	LLStyle embedded_style;
	embedded_style.setIsEmbeddedItem( TRUE );

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
	return FALSE;
}

llwchar LLTextEditor::pasteEmbeddedItem(llwchar ext_char)
{
	return ext_char;
}

void LLTextEditor::bindEmbeddedChars(const LLFontGL* font)
{
}

void LLTextEditor::unbindEmbeddedChars(const LLFontGL* font)
{
}

// Finds the text segment (if any) at the give local screen position
LLTextSegment* LLTextEditor::getSegmentAtLocalPos( S32 x, S32 y )
{
	// Find the cursor position at the requested local screen position
	S32 offset = getCursorPosFromLocalCoord( x, y, FALSE );
	S32 idx = getSegmentIdxAtOffset(offset);
	return idx >= 0 ? mSegments[idx] : NULL;
}

LLTextSegment* LLTextEditor::getSegmentAtOffset(S32 offset)
{
	S32 idx = getSegmentIdxAtOffset(offset);
	return idx >= 0 ? mSegments[idx] : NULL;
}

S32 LLTextEditor::getSegmentIdxAtOffset(S32 offset)
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

BOOL LLTextEditor::importBuffer(const LLString& buffer )
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

	if( text_len > mMaxTextLength )
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
		setText( text );
	}

	delete[] text;

	setCursorPos(0);
	deselect();

	updateLineStartList();
	updateScrollFromCursor();

	return success;
}

BOOL LLTextEditor::exportBuffer(LLString &buffer )
{
	std::ostringstream outstream(buffer);
	
	outstream << "Linden text version 1\n";
	outstream << "{\n";

	outstream << llformat("Text length %d\n", mWText.length() );
	outstream << getText();
	outstream << "}\n";

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
// LLTextSegment

LLTextSegment::LLTextSegment(S32 start) : mStart(start)
{
} 
LLTextSegment::LLTextSegment( const LLStyle& style, S32 start, S32 end ) :
	mStyle( style ),
	mStart( start),
	mEnd( end ),
	mToken(NULL),
	mIsDefault(FALSE)
{
}
LLTextSegment::LLTextSegment(
	const LLColor4& color, S32 start, S32 end, BOOL is_visible) :
	mStyle( is_visible, color,"" ),
	mStart( start),
	mEnd( end ),
	mToken(NULL),
	mIsDefault(FALSE)
{
}
LLTextSegment::LLTextSegment( const LLColor4& color, S32 start, S32 end ) :
	mStyle( TRUE, color,"" ),
	mStart( start),
	mEnd( end ),
	mToken(NULL),
	mIsDefault(FALSE)
{
}
LLTextSegment::LLTextSegment( const LLColor3& color, S32 start, S32 end ) :
	mStyle( TRUE, color,"" ),
	mStart( start),
	mEnd( end ),
	mToken(NULL),
	mIsDefault(FALSE)
{
}

BOOL LLTextSegment::getToolTip(LLString& msg)
{
	if (mToken && !mToken->getToolTip().empty())
	{
		const LLWString& wmsg = mToken->getToolTip();
		msg = wstring_to_utf8str(wmsg);
		return TRUE;
	}
	return FALSE;
}



void LLTextSegment::dump()
{
	llinfos << "Segment [" << 
//			mColor.mV[VX] << ", " <<
//			mColor.mV[VY] << ", " <<
//			mColor.mV[VZ] << "]\t[" <<
		mStart << ", " <<
		getEnd() << "]" <<
		llendl;

}

// virtual
LLXMLNodePtr LLTextEditor::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	// Attributes

	node->createChild("max_length", TRUE)->setIntValue(getMaxLength());

	node->createChild("embedded_items", TRUE)->setBoolValue(mAllowEmbeddedItems);

	node->createChild("font", TRUE)->setStringValue(LLFontGL::nameFromFont(mGLFont));

	node->createChild("word_wrap", TRUE)->setBoolValue(mWordWrap);

	node->createChild("hide_scrollbar", TRUE)->setBoolValue(mHideScrollbarForShortDocs);

	addColorXML(node, mCursorColor, "cursor_color", "TextCursorColor");
	addColorXML(node, mFgColor, "text_color", "TextFgColor");
	addColorXML(node, mReadOnlyFgColor, "text_readonly_color", "TextFgReadOnlyColor");
	addColorXML(node, mReadOnlyBgColor, "bg_readonly_color", "TextBgReadOnlyColor");
	addColorXML(node, mWriteableBgColor, "bg_writeable_color", "TextBgWriteableColor");
	addColorXML(node, mFocusBgColor, "bg_focus_color", "TextBgFocusColor");

	// Contents
 	node->setStringValue(getText());

	return node;
}

// static
LLView* LLTextEditor::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("text_editor");
	node->getAttributeString("name", name);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	U32 max_text_length = 255;
	node->getAttributeU32("max_length", max_text_length);

	BOOL allow_embedded_items;
	node->getAttributeBOOL("embedded_items", allow_embedded_items);

	LLFontGL* font = LLView::selectFont(node);

	LLString text = node->getTextContents().substr(0, max_text_length - 1);

	LLTextEditor* text_editor = new LLTextEditor(name, 
								rect,
								max_text_length,
								text,
								font,
								allow_embedded_items);

	text_editor->setTextEditorParameters(node);

	BOOL hide_scrollbar = FALSE;
	node->getAttributeBOOL("hide_scrollbar",hide_scrollbar);
	text_editor->setHideScrollbarForShortDocs(hide_scrollbar);

	text_editor->initFromXML(node, parent);

	return text_editor;
}

void LLTextEditor::setTextEditorParameters(LLXMLNodePtr node)
{
	BOOL word_wrap = FALSE;
	node->getAttributeBOOL("word_wrap", word_wrap);
	setWordWrap(word_wrap);

	LLColor4 color;
	if (LLUICtrlFactory::getAttributeColor(node,"cursor_color", color)) 
	{
		setCursorColor(color);
	}
	if(LLUICtrlFactory::getAttributeColor(node,"text_color", color))
	{
		setFgColor(color);
	}
	if(LLUICtrlFactory::getAttributeColor(node,"text_readonly_color", color))
	{
		setReadOnlyFgColor(color);
	}
	if(LLUICtrlFactory::getAttributeColor(node,"bg_readonly_color", color))
	{
		setReadOnlyBgColor(color);
	}
	if(LLUICtrlFactory::getAttributeColor(node,"bg_writeable_color", color))
	{
		setWriteableBgColor(color);
	}
}

///////////////////////////////////////////////////////////////////
S32 LLTextEditor::findHTMLToken(const LLString &line, S32 pos, BOOL reverse)
{
	LLString openers=" \t('\"[{<>";
	LLString closers=" \t)'\"]}><;";

	S32 m2;
	S32 retval;
	
	if (reverse)
	{
		
		for (retval=pos; retval>0; retval--)
		{
			m2 = openers.find(line.substr(retval,1));
			if (m2 >= 0)
			{
				retval++;
				break;
			}
		}
	} 
	else
	{
		
		for (retval=pos; retval<(S32)line.length(); retval++)
		{
			m2 = closers.find(line.substr(retval,1));
			if (m2 >= 0)
			{
				break;
			}
		} 
	}		
	
	return retval;
}

BOOL LLTextEditor::findHTML(const LLString &line, S32 *begin, S32 *end)
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
	
		LLString badneighbors=".,<>/?';\"][}{=-+_)(*&^%$#@!~`\t\r\n\\";
	
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
				LLString badneighbors=".,<>/?';\"][}{=-+_)(*&^%$#@!~`";
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

		LLString url     = line.substr(*begin,*end - *begin);
		LLString slurlID = "slurl.com/secondlife/";
		strpos = url.find(slurlID);
		
		if (strpos < 0)
		{
			slurlID="secondlife://";
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

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

#include "llfontfreetype.h" // for LLFontFreetype::FIRST_CHAR
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
#include "llwindow.h"
#include "lltextparser.h"
#include "llscrollcontainer.h"
#include "llpanel.h"
#include <queue>
#include "llcombobox.h"

// 
// Globals
//
static LLDefaultChildRegistry::Register<LLTextEditor> r("simple_text_editor");

//
// Constants
//
const S32	UI_TEXTEDITOR_LINE_NUMBER_MARGIN = 32;
const S32	UI_TEXTEDITOR_LINE_NUMBER_DIGITS = 4;
const F32	CURSOR_FLASH_DELAY = 1.0f;  // in seconds
const S32	CURSOR_THICKNESS = 2;
const S32	SPACES_PER_TAB = 4;


void (* LLTextEditor::sURLcallback)(const std::string&)   = NULL;
bool (* LLTextEditor::sSecondlifeURLcallback)(const std::string&)   = NULL;
bool (* LLTextEditor::sSecondlifeURLcallbackRightClick)(const std::string&)   = NULL;

// helper functors
struct LLTextEditor::compare_bottom
{
	bool operator()(const S32& a, const LLTextEditor::line_info& b) const
	{
		return a > b.mBottom; // bottom of a is higher than bottom of b
	}

	bool operator()(const LLTextEditor::line_info& a, const S32& b) const
	{
		return a.mBottom > b; // bottom of a is higher than bottom of b
	}
};

// helper functors
struct LLTextEditor::compare_top
{
	bool operator()(const S32& a, const LLTextEditor::line_info& b) const
	{
		return a > b.mTop; // top of a is higher than top of b
	}

	bool operator()(const LLTextEditor::line_info& a, const S32& b) const
	{
		return a.mTop > b; // top of a is higher than top of b
	}
};

struct LLTextEditor::line_end_compare
{
	bool operator()(const S32& pos, const LLTextEditor::line_info& info) const
	{
		return (pos < info.mDocIndexEnd);
	}

	bool operator()(const LLTextEditor::line_info& info, const S32& pos) const
	{
		return (info.mDocIndexEnd < pos);
	}

};

//
// DocumentPanel
//

class DocumentPanel : public LLPanel
{
public:
	DocumentPanel(const Params&);
};

DocumentPanel::DocumentPanel(const Params& p)
: LLPanel(p)
{}

///////////////////////////////////////////////////////////////////

class LLTextEditor::LLTextCmdInsert : public LLTextEditor::LLTextCmd
{
public:
	LLTextCmdInsert(S32 pos, BOOL group_with_next, const LLWString &ws, LLTextSegmentPtr segment)
		: LLTextCmd(pos, group_with_next, segment), mWString(ws)
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
	LLTextCmdAddChar( S32 pos, BOOL group_with_next, llwchar wc, LLTextSegmentPtr segment)
		: LLTextCmd(pos, group_with_next, segment), mWString(1, wc), mBlockExtensions(FALSE)
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
	LLTextCmdRemove( S32 pos, BOOL group_with_next, S32 len, segment_vec_t& segments ) :
		LLTextCmd(pos, group_with_next), mLen(len)
	{
		std::swap(mSegments, segments);
	}
	virtual BOOL execute( LLTextEditor* editor, S32* delta )
	{ 
		mWString = editor->getWSubString(getPosition(), mLen);
		*delta = remove(editor, getPosition(), mLen );
		return (*delta != 0);
	}
	virtual S32 undo( LLTextEditor* editor )
	{
		insert(editor, getPosition(), mWString);
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
LLTextEditor::Params::Params()
:	default_text("default_text"),
	max_text_length("max_length", 255),
	read_only("read_only", false),
	embedded_items("embedded_items", false),
	hide_scrollbar("hide_scrollbar"),
	hide_border("hide_border", false),
	word_wrap("word_wrap", false),
	ignore_tab("ignore_tab", true),
	track_bottom("track_bottom", false),
	handle_edit_keys_directly("handle_edit_keys_directly", false),
	show_line_numbers("show_line_numbers", false),
	cursor_color("cursor_color"),
	default_color("default_color"),
	text_color("text_color"),
	text_readonly_color("text_readonly_color"),
	bg_readonly_color("bg_readonly_color"),
	bg_writeable_color("bg_writeable_color"),
	bg_focus_color("bg_focus_color"),
	link_color("link_color"),
    commit_on_focus_lost("commit_on_focus_lost", false),
	length("length"),		// ignored
	type("type"),			// ignored
	is_unicode("is_unicode")// ignored
{}

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
	mOnScrollEndData( NULL ),
	mCursorColor(		p.cursor_color() ),
	mFgColor(			p.text_color() ),
	mDefaultColor(		p.default_color() ),
	mReadOnlyFgColor(	p.text_readonly_color() ),
	mWriteableBgColor(	p.bg_writeable_color() ),
	mReadOnlyBgColor(	p.bg_readonly_color() ),
	mFocusBgColor(		p.bg_focus_color() ),
	mLinkColor(			p.link_color() ),
	mReadOnly(p.read_only),
	mWordWrap( p.word_wrap ),
	mShowLineNumbers ( p.show_line_numbers ),
	mCommitOnFocusLost( p.commit_on_focus_lost),
	mTrackBottom( p.track_bottom ),
	mAllowEmbeddedItems( p.embedded_items ),
	mHandleEditKeysDirectly( p.handle_edit_keys_directly ),
	mMouseDownX(0),
	mMouseDownY(0),
	mLastSelectionX(-1),
	mReflowNeeded(FALSE),
	mScrollNeeded(FALSE),
	mLastSelectionY(-1),
	mParseHTML(FALSE),
	mParseHighlights(FALSE),
	mTabsToNextField(p.ignore_tab),
	mDefaultFont(p.font),
	mScrollIndex(-1)
{
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);

	mSourceID.generate();

	// reset desired x cursor position
	mDesiredXPixel = -1;

	LLScrollContainer::Params scroll_params;
	scroll_params.name = "text scroller";
	scroll_params.rect = getLocalRect();
	scroll_params.follows.flags = FOLLOWS_ALL;
	scroll_params.is_opaque = false;
	scroll_params.mouse_opaque = false;
	scroll_params.min_auto_scroll_rate = 200;
	scroll_params.max_auto_scroll_rate = 800;
	mScroller = LLUICtrlFactory::create<LLScrollContainer>(scroll_params);
	addChild(mScroller);

	LLPanel::Params panel_params;
	panel_params.name = "text_contents";
	panel_params.rect =  LLRect(0, 500, 500, 0);
	panel_params.background_visible = true;
	panel_params.background_opaque = true;
	panel_params.mouse_opaque = false;

	mDocumentPanel = LLUICtrlFactory::create<DocumentPanel>(panel_params);
	mScroller->addChild(mDocumentPanel);

	updateTextRect();

	static LLUICachedControl<S32> text_editor_border ("UITextEditorBorder", 0);
	LLViewBorder::Params params;
	params.name = "text ed border";
	params.rect = getLocalRect();
	params.bevel_style = LLViewBorder::BEVEL_IN;
	params.border_thickness = text_editor_border;
	mBorder = LLUICtrlFactory::create<LLViewBorder> (params);
	addChild( mBorder );
	mBorder->setVisible(!p.hide_border);

	createDefaultSegment();

	appendText(p.default_text, FALSE, FALSE);

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
	}
	
	if (p.commit_on_focus_lost.isProvided())
	{
		mCommitOnFocusLost = p.commit_on_focus_lost;
	}
	
	updateSegments();
	updateAllowingLanguageInput();

	// HACK:  text editors always need to be enabled so that we can scroll
	LLView::setEnabled(true);
}

LLTextEditor::~LLTextEditor()
{
	gFocusMgr.releaseFocusIfNeeded( this ); // calls onCommit() while LLTextEditor still valid

	// Route menu back to the default
	if( gEditMenuHandler == this )
	{
		gEditMenuHandler = NULL;
	}

	// Scrollbar is deleted by LLView
	mHoverSegment = NULL;
	std::for_each(mUndoStack.begin(), mUndoStack.end(), DeletePointer());
}

LLTextViewModel* LLTextEditor::getViewModel() const
{
	return (LLTextViewModel*)mViewModel.get();
}

static LLFastTimer::DeclareTimer FTM_TEXT_REFLOW ("Text Reflow");
void LLTextEditor::reflow(S32 start_index)
{
	if (!mReflowNeeded) return;

	LLFastTimer ft(FTM_TEXT_REFLOW);
	static LLUICachedControl<S32> texteditor_vpad_top ("UITextEditorVPadTop", 0);

	updateSegments();

	while(mReflowNeeded)
	{
		bool scrolled_to_bottom = mScroller->isAtBottom();
		mReflowNeeded = FALSE;

		LLRect old_cursor_rect = getLocalRectFromDocIndex(mCursorPos);
		bool follow_selection = mTextRect.overlaps(old_cursor_rect); // cursor is visible
		S32 first_line = getFirstVisibleLine();
		// if scroll anchor not on first line, update it to first character of first line
		if (!mLineInfoList.empty()
			&&	(mScrollIndex <  mLineInfoList[first_line].mDocIndexStart
				||	mScrollIndex >= mLineInfoList[first_line].mDocIndexEnd))
		{
			mScrollIndex = mLineInfoList[first_line].mDocIndexStart;
		}
		LLRect first_char_rect = getLocalRectFromDocIndex(mScrollIndex);
		//first_char_rect.intersectWith(mTextRect);

		S32 cur_top = -texteditor_vpad_top;
		
		if (getLength())
		{
			segment_set_t::iterator seg_iter = mSegments.begin();
			S32 seg_offset = 0;
			S32 line_start_index = 0;
			S32 text_width = mTextRect.getWidth();  // optionally reserve room for margin
			S32 remaining_pixels = text_width;
			LLWString text(getWText());
			S32 line_count = 0;

			// find and erase line info structs starting at start_index and going to end of document
			if (!mLineInfoList.empty())
			{
				// find first element whose end comes after start_index
				line_list_t::iterator iter = std::upper_bound(mLineInfoList.begin(), mLineInfoList.end(), start_index, line_end_compare());
				line_start_index = iter->mDocIndexStart;
				line_count = iter->mLineNum;
				getSegmentAndOffset(iter->mDocIndexStart, &seg_iter, &seg_offset);
				mLineInfoList.erase(iter, mLineInfoList.end());
			}

			// reserve enough space for line numbers
			S32 line_height = mShowLineNumbers ? (S32)(LLFontGL::getFontMonospace()->getLineHeight()) : 0;

			while(seg_iter != mSegments.end())
			{
				LLTextSegmentPtr segment = *seg_iter;

				// track maximum height of any segment on this line
				line_height = llmax(line_height, segment->getMaxHeight());
				S32 cur_index = segment->getStart() + seg_offset;
				// find run of text from this segment that we can display on one line
				S32 end_index = cur_index;
				while(end_index < segment->getEnd() && text[end_index] != '\n')
				{
					++end_index;
				}

				// ask segment how many character fit in remaining space
				S32 max_characters = end_index - cur_index;
				S32 character_count = segment->getNumChars(llmax(0, remaining_pixels), seg_offset, cur_index - line_start_index, max_characters);

				seg_offset += character_count;

				S32 last_segment_char_on_line = segment->getStart() + seg_offset;

				// if we didn't finish the current segment...
				if (last_segment_char_on_line < segment->getEnd())
				{
					// set up index for next line
					// ...skip newline, we don't want to draw
					S32 next_line_count = line_count;
					if (text[last_segment_char_on_line] == '\n')
					{
						seg_offset++;
						last_segment_char_on_line++;
						next_line_count++;
					}

					// add line info and keep going
					mLineInfoList.push_back(line_info(line_start_index, last_segment_char_on_line, cur_top, cur_top - line_height, line_count));

					line_start_index = segment->getStart() + seg_offset;
					cur_top -= line_height;
					remaining_pixels = text_width;
					line_height = 0;
					line_count = next_line_count;
				}
				// ...just consumed last segment..
				else if (++segment_set_t::iterator(seg_iter) == mSegments.end())
				{
					mLineInfoList.push_back(line_info(line_start_index, last_segment_char_on_line, cur_top, cur_top - line_height, line_count));
					cur_top -= line_height;
					break;
				}
				// finished a segment and there are segments remaining on this line
				else
				{
					// subtract pixels used and increment segment
					remaining_pixels -= segment->getWidth(seg_offset, character_count);
					++seg_iter;
					seg_offset = 0;
				}
			}
		}

		// change mDocumentPanel document size to accomodate reflowed text
		LLRect document_rect;
		document_rect.setOriginAndSize(1, 1, 
									mScroller->getContentWindowRect().getWidth(), 
									llmax(mScroller->getContentWindowRect().getHeight(), -cur_top));
		mDocumentPanel->setShape(document_rect);

		// after making document big enough to hold all the text, move the text to fit in the document
		if (!mLineInfoList.empty())
		{
			S32 delta_pos = mDocumentPanel->getRect().getHeight() - mLineInfoList.begin()->mTop - texteditor_vpad_top;
			// move line segments to fit new document rect
			for (line_list_t::iterator it = mLineInfoList.begin(); it != mLineInfoList.end(); ++it)
			{
				it->mTop += delta_pos;
				it->mBottom += delta_pos;
			}
		}

		// calculate visible region for diplaying text
		updateTextRect();

		for (segment_set_t::iterator segment_it = mSegments.begin();
			segment_it != mSegments.end();
			++segment_it)
		{
			LLTextSegmentPtr segmentp = *segment_it;
			segmentp->updateLayout(*this);

		}

		// apply scroll constraints after reflowing text
		if (!hasMouseCapture())
		{
			LLRect visible_content_rect = mScroller->getVisibleContentRect();
			if (scrolled_to_bottom && mTrackBottom)
			{
				// keep bottom of text buffer visible
				endOfDoc();
			}
			else if (hasSelection() && follow_selection)
			{
				// keep cursor in same vertical position on screen when selecting text
				LLRect new_cursor_rect_doc = getLocalRectFromDocIndex(mCursorPos);
				new_cursor_rect_doc.translate(visible_content_rect.mLeft, visible_content_rect.mBottom);
				mScroller->scrollToShowRect(new_cursor_rect_doc, old_cursor_rect);
				//llassert_always(getLocalRectFromDocIndex(mCursorPos).mBottom == old_cursor_rect.mBottom);
			}
			else
			{
				// keep first line of text visible
				LLRect new_first_char_rect = getLocalRectFromDocIndex(mScrollIndex);
				new_first_char_rect.translate(visible_content_rect.mLeft, visible_content_rect.mBottom);
				mScroller->scrollToShowRect(new_first_char_rect, first_char_rect);
				//llassert_always(getLocalRectFromDocIndex(mScrollIndex).mBottom == first_char_rect.mBottom);
			}
		}
	}

	// reset desired x cursor position
	updateCursorXPos();
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

void LLTextEditor::clearSegments()
{
	mHoverSegment = NULL;
	mSegments.clear();
}

void LLTextEditor::setText(const LLStringExplicit &utf8str)
{
	clearSegments();

	// LLStringUtil::removeCRLF(utf8str);
	getViewModel()->setValue(utf8str_removeCRLF(utf8str));

	truncate();
	blockUndo();

	createDefaultSegment();

	startOfDoc();
	deselect();

	needsReflow();

	resetDirty();

	onValueChange(0, getLength());
}

void LLTextEditor::setWText(const LLWString &wtext)
{
	clearSegments();

	getViewModel()->setDisplay(wtext);

	truncate();
	blockUndo();

	createDefaultSegment();

	startOfDoc();
	deselect();

	needsReflow();

	resetDirty();

	onValueChange(0, getLength());
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
	return getViewModel()->getValue().asString();
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

// Picks a new cursor position based on the screen size of text being drawn.
void LLTextEditor::setCursorAtLocalPos( S32 local_x, S32 local_y, bool round, bool keep_cursor_offset )
{
	setCursorPos(getDocIndexFromLocalCoord(local_x, local_y, round), keep_cursor_offset);
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

S32 LLTextEditor::getLineStart( S32 line ) const
{
	S32 num_lines = getLineCount();
	if (num_lines == 0)
    {
		return 0;
    }

	line = llclamp(line, 0, num_lines-1);
	return mLineInfoList[line].mDocIndexStart;
}

S32 LLTextEditor::getLineHeight( S32 line ) const
{
	S32 num_lines = getLineCount();
	if (num_lines == 0)
    {
		return 0;
    }

	line = llclamp(line, 0, num_lines-1);
	return mLineInfoList[line].mTop - mLineInfoList[line].mBottom;
}

// Given an offset into text (pos), find the corresponding line (from the start of the doc) and an offset into the line.
void LLTextEditor::getLineAndOffset( S32 startpos, S32* linep, S32* offsetp, bool include_wordwrap) const
{
	if (mLineInfoList.empty())
	{
		*linep = 0;
		*offsetp = startpos;
	}
	else
	{
		line_list_t::const_iterator iter = std::upper_bound(mLineInfoList.begin(), mLineInfoList.end(), startpos, line_end_compare());
		if (include_wordwrap)
		{
			*linep = iter - mLineInfoList.begin();
		}
		else
		{
			if (iter == mLineInfoList.end())
			{
				*linep = mLineInfoList.back().mLineNum;
			}
			else
			{
				*linep = iter->mLineNum;
			}
		}
		*offsetp = startpos - iter->mDocIndexStart;
	}
}

void LLTextEditor::getSegmentAndOffset( S32 startpos, segment_set_t::const_iterator* seg_iter, S32* offsetp ) const
{
	*seg_iter = getSegIterContaining(startpos);
	if (*seg_iter == mSegments.end())
	{
		*offsetp = 0;
	}
	else
	{
		*offsetp = startpos - (**seg_iter)->getStart();
	}
}

void LLTextEditor::getSegmentAndOffset( S32 startpos, segment_set_t::iterator* seg_iter, S32* offsetp )
{
	*seg_iter = getSegIterContaining(startpos);
	if (*seg_iter == mSegments.end())
	{
		*offsetp = 0;
	}
	else
	{
		*offsetp = startpos - (**seg_iter)->getStart();
	}
}

const LLTextSegmentPtr	LLTextEditor::getPreviousSegment() const
{
	// find segment index at character to left of cursor (or rightmost edge of selection)
	segment_set_t::const_iterator it = mSegments.lower_bound(new LLIndexSegment(mCursorPos));

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

// If round is true, if the position is on the right half of a character, the cursor
// will be put to its right.  If round is false, the cursor will always be put to the
// character's left.

S32 LLTextEditor::getDocIndexFromLocalCoord( S32 local_x, S32 local_y, BOOL round ) const
{
	// Figure out which line we're nearest to.
	LLRect visible_region = mScroller->getVisibleContentRect();

	// binary search for line that starts before local_y
	line_list_t::const_iterator line_iter = std::lower_bound(mLineInfoList.begin(), mLineInfoList.end(), local_y - mTextRect.mBottom + visible_region.mBottom, compare_bottom());

	if (line_iter == mLineInfoList.end())
	{
		return getLength(); // past the end
	}
	
	S32 pos = getLength();
	S32 start_x = mTextRect.mLeft;

	segment_set_t::iterator line_seg_iter;
	S32 line_seg_offset;
	for(getSegmentAndOffset(line_iter->mDocIndexStart, &line_seg_iter, &line_seg_offset);
		line_seg_iter != mSegments.end(); 
		++line_seg_iter, line_seg_offset = 0)
	{
		const LLTextSegmentPtr segmentp = *line_seg_iter;

		S32 segment_line_start = segmentp->getStart() + line_seg_offset;
		S32 segment_line_length = llmin(segmentp->getEnd(), line_iter->mDocIndexEnd - 1) - segment_line_start;
		S32 text_width = segmentp->getWidth(line_seg_offset, segment_line_length);
		if (local_x < start_x + text_width						// cursor to left of right edge of text
			|| segmentp->getEnd() >= line_iter->mDocIndexEnd - 1)	// or this segment wraps to next line
		{
			// Figure out which character we're nearest to.
			S32 offset;
			if (!segmentp->canEdit())
			{
				S32 segment_width = segmentp->getWidth(0, segmentp->getEnd() - segmentp->getStart());
				if (round && local_x - start_x > segment_width / 2)
				{
					offset = segment_line_length;
				}
				else
				{
					offset = 0;
				}
			}
			else
			{
				offset = segmentp->getOffset(local_x - start_x, line_seg_offset, segment_line_length, round);
			}
			pos = segment_line_start + offset;
			break;
		}
		start_x += text_width;
	}

	return pos;
}

LLRect LLTextEditor::getLocalRectFromDocIndex(S32 pos) const
{
	LLRect local_rect(mTextRect);
	local_rect.mBottom = local_rect.mTop - (S32)(mDefaultFont->getLineHeight());
	if (mLineInfoList.empty()) 
	{ 
		return local_rect;
	}

	// clamp pos to valid values
	pos = llclamp(pos, 0, mLineInfoList.back().mDocIndexEnd - 1);


	// find line that contains cursor
	line_list_t::const_iterator line_iter = std::upper_bound(mLineInfoList.begin(), mLineInfoList.end(), pos, line_end_compare());

	LLRect scrolled_view_rect = mScroller->getVisibleContentRect();
	local_rect.mLeft = mTextRect.mLeft - scrolled_view_rect.mLeft; 
	local_rect.mBottom = mTextRect.mBottom + (line_iter->mBottom - scrolled_view_rect.mBottom);
	local_rect.mTop = mTextRect.mBottom + (line_iter->mTop - scrolled_view_rect.mBottom);

	segment_set_t::iterator line_seg_iter;
	S32 line_seg_offset;
	segment_set_t::iterator cursor_seg_iter;
	S32 cursor_seg_offset;
	getSegmentAndOffset(line_iter->mDocIndexStart, &line_seg_iter, &line_seg_offset);
	getSegmentAndOffset(pos, &cursor_seg_iter, &cursor_seg_offset);

	while(line_seg_iter != mSegments.end())
	{
		const LLTextSegmentPtr segmentp = *line_seg_iter;

		if (line_seg_iter == cursor_seg_iter)
		{
			// cursor advanced to right based on difference in offset of cursor to start of line
			local_rect.mLeft += segmentp->getWidth(line_seg_offset, cursor_seg_offset - line_seg_offset);

			break;
		}
		else
		{
			// add remainder of current text segment to cursor position
			local_rect.mLeft += segmentp->getWidth(line_seg_offset, (segmentp->getEnd() - segmentp->getStart()) - line_seg_offset);
			// offset will be 0 for all segments after the first
			line_seg_offset = 0;
			// go to next text segment on this line
			++line_seg_iter;
		}
	}

	local_rect.mRight = local_rect.mLeft; 

	return local_rect;
}

void LLTextEditor::addDocumentChild(LLView* view) 
{ 
	mDocumentPanel->addChild(view); 
}

void LLTextEditor::removeDocumentChild(LLView* view) 
{ 
	mDocumentPanel->removeChild(view); 
}

bool LLTextEditor::setCursor(S32 row, S32 column)
{
	if (0 <= row && row < (S32)mLineInfoList.size())
	{
		S32 doc_pos = mLineInfoList[row].mDocIndexStart;
		column = llclamp(column, 0, mLineInfoList[row].mDocIndexEnd - mLineInfoList[row].mDocIndexStart - 1);
		doc_pos += column;
		updateCursorXPos();

		return setCursorPos(doc_pos);
	}
	return false;
}

bool LLTextEditor::setCursorPos(S32 cursor_pos, bool keep_cursor_offset)
{
	S32 new_cursor_pos = cursor_pos;
	if (new_cursor_pos != mCursorPos)
	{
		new_cursor_pos = getEditableIndex(new_cursor_pos, new_cursor_pos >= mCursorPos);
	}

	mCursorPos = llclamp(new_cursor_pos, 0, (S32)getLength());
	needsScroll();
	if (!keep_cursor_offset)
		updateCursorXPos();
	// did we get requested position?
	return new_cursor_pos == cursor_pos;
}

void LLTextEditor::updateCursorXPos()
{
	// reset desired x cursor position
	mDesiredXPixel = getLocalRectFromDocIndex(mCursorPos).mLeft;
}

// constraint cursor to editable segments of document
S32 LLTextEditor::getEditableIndex(S32 index, bool increasing_direction)
{
	//// always allow editable position at end of doc
	//if (index == getLength())
	//{
	//	return index;
	//}

	segment_set_t::iterator segment_iter;
	S32 offset;
	getSegmentAndOffset(index, &segment_iter, &offset);

	LLTextSegmentPtr segmentp = *segment_iter;

	if (segmentp->canEdit()) 
	{
		return segmentp->getStart() + offset;			
	}
	else if (segmentp->getStart() < index && index < segmentp->getEnd())
	{
		// bias towards document end
		if (increasing_direction)
		{
			return segmentp->getEnd();
		}
		// bias towards document start
		else
		{
			return segmentp->getStart();
		}
	}
	else
	{
		return index;
	}
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

			text = getWText();

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

	const LLTextSegmentPtr cur_segment = getSegmentAtLocalPos( x, y );
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

BOOL LLTextEditor::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// Let scrollbar have first dibs
	handled = LLView::childrenHandleMouseDown(x, y, mask) != NULL;

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
	BOOL	handled = FALSE;
	handled = childrenHandleMiddleMouseDown(x, y, mask) != NULL;

	if (!handled)
	{
		setFocus( TRUE );
		if( canPastePrimary() )
		{
			setCursorAtLocalPos( x, y, true );
			pastePrimary();
		}
	}
	return TRUE;
}


BOOL LLTextEditor::handleHover(S32 x, S32 y, MASK mask)
{
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);
	BOOL handled = FALSE;

	if (mHoverSegment) 
	{
		mHoverSegment->setHasMouseHover(false);
	}
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

			mScroller->autoScroll(x, y);

			S32 clamped_x = llclamp(x, mTextRect.mLeft, mTextRect.mRight);
			S32 clamped_y = llclamp(y, mTextRect.mBottom, mTextRect.mTop);
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
		handled = LLView::childrenHandleHover(x, y, mask) != NULL;
	}

	if( handled )
	{
		// Delay cursor flashing
		resetKeystrokeTimer();
	}

	// Opaque
	if( !handled )
	{
		// Check to see if we're over an HTML-style link
		LLTextSegmentPtr cur_segment = getSegmentAtLocalPos( x, y );
		if( cur_segment )
		{
			if(cur_segment->getStyle()->isLink())
			{
				lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (over link, inactive)" << llendl;		
				getWindow()->setCursor(UI_CURSOR_HAND);
				handled = TRUE;
			}
			//else
			//if(cur_segment->getStyle()->getIsEmbeddedItem())
			//{
			//	lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (over embedded item, inactive)" << llendl;		
			//	getWindow()->setCursor(UI_CURSOR_HAND);
			//	//getWindow()->setCursor(UI_CURSOR_ARROW);
			//	handled = TRUE;
			//}
			if (mHoverSegment) 
			{
				mHoverSegment->setHasMouseHover(false);
			}
			cur_segment->setHasMouseHover(true);
			mHoverSegment = cur_segment;
			mHTML = mHoverSegment->getStyle()->getLinkHREF();
		}

		if( !handled )
		{
			lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (inactive)" << llendl;		
			getWindow()->setCursor(UI_CURSOR_IBEAM);
			handled = TRUE;
		}
	}

	return handled;
}


BOOL LLTextEditor::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// let scrollbar have first dibs
	handled = LLView::childrenHandleMouseUp(x, y, mask) != NULL;

	if( !handled )
	{
		if( mIsSelecting )
		{
			mScroller->autoScroll(x, y);
			S32 clamped_x = llclamp(x, mTextRect.mLeft, mTextRect.mRight);
			S32 clamped_y = llclamp(y, mTextRect.mBottom, mTextRect.mTop);
			setCursorAtLocalPos( clamped_x, clamped_y, true );
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

S32 LLTextEditor::insert(S32 pos, const LLWString &wstr, bool group_with_next_op, LLTextSegmentPtr segment)
{
	return execute( new LLTextCmdInsert( pos, group_with_next_op, wstr, segment ) );
}

S32 LLTextEditor::remove(S32 pos, S32 length, bool group_with_next_op)
{
	S32 end_pos = getEditableIndex(pos + length, true);

	segment_vec_t segments_to_remove;
	// store text segments
	getSegmentsInRange(segments_to_remove, pos, pos + length, false);

	return execute( new LLTextCmdRemove( pos, group_with_next_op, end_pos - pos, segments_to_remove ) );
}

S32 LLTextEditor::append(const LLWString &wstr, bool group_with_next_op, LLTextSegmentPtr segment)
{
	return insert(getLength(), wstr, group_with_next_op, segment);
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
		return execute(new LLTextCmdAddChar(pos, FALSE, wc, LLTextSegmentPtr()));
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
	setCursorPos(mCursorPos + insert(mCursorPos, clean_string, FALSE, LLTextSegmentPtr()));
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

	// Special case for TAB.  If want to move to next field, report
	// not handled and let the parent take care of field movement.
	if (KEY_TAB == key && mTabsToNextField)
	{
		return FALSE;
	}
	/*
	if (KEY_F10 == key)
	{
		LLComboBox::Params cp;
		cp.name = "combo box";
		cp.label = "my combo";
		cp.rect.width = 100;
		cp.rect.height = 20;
		cp.items.add().label = "item 1";
		cp.items.add().label = "item 2";
		cp.items.add().label = "item 3";
		
		appendWidget(LLUICtrlFactory::create<LLComboBox>(cp), "combo", true, false);
	}
	if (KEY_F11 == key)
	{
		LLButton::Params bp;
		bp.name = "text button";
		bp.label = "Click me";
		bp.rect.width = 100;
		bp.rect.height = 20;

		appendWidget(LLUICtrlFactory::create<LLButton>(bp), "button", true, false);
	}
	*/
	if (mReadOnly)
	{
		handled = mScroller->handleKeyHere( key, mask );
	}
	else
	{
		// handle navigation keys ourself
		handled = handleNavigationKey( key, mask );
	}


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
		resetKeystrokeTimer();

		// Most keystrokes will make the selection box go away, but not all will.
		deselect();

		needsReflow();
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

void LLTextEditor::onCommit()
{
	setControlValue(getValue()); 
	LLUICtrl::onCommit(); 
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
	S32 bottom = 0;

	LLColor4 bg_color = mReadOnly ? mReadOnlyBgColor.get()
		: hasFocus() ? mFocusBgColor.get() : mWriteableBgColor.get();
	if( mShowLineNumbers ) {
		gl_rect_2d(left, top, UI_TEXTEDITOR_LINE_NUMBER_MARGIN, bottom, mReadOnlyBgColor.get() ); // line number area always read-only
		gl_rect_2d(UI_TEXTEDITOR_LINE_NUMBER_MARGIN, top, UI_TEXTEDITOR_LINE_NUMBER_MARGIN-1, bottom, LLColor4::grey3); // separator
	} 
}

// Draws the black box behind the selected text
void LLTextEditor::drawSelectionBackground()
{
	// Draw selection even if we don't have keyboard focus for search/replace
	if( hasSelection() && !mLineInfoList.empty())
	{
		LLWString text = getWText();
		std::vector<LLRect> selection_rects;

		S32 selection_left		= llmin( mSelectionStart, mSelectionEnd );
		S32 selection_right		= llmax( mSelectionStart, mSelectionEnd );
		LLRect selection_rect = mTextRect;

		// Skip through the lines we aren't drawing.
		LLRect content_display_rect = mScroller->getVisibleContentRect();

		// binary search for line that starts before top of visible buffer
		line_list_t::const_iterator line_iter = std::lower_bound(mLineInfoList.begin(), mLineInfoList.end(), content_display_rect.mTop, compare_bottom());
		line_list_t::const_iterator end_iter = std::lower_bound(mLineInfoList.begin(), mLineInfoList.end(), content_display_rect.mBottom, compare_top());

		bool done = false;

		// Find the coordinates of the selected area
		for (;line_iter != end_iter && !done; ++line_iter)
		{
			// is selection visible on this line?
			if (line_iter->mDocIndexEnd > selection_left && line_iter->mDocIndexStart < selection_right)
			{
				segment_set_t::iterator segment_iter;
				S32 segment_offset;
				getSegmentAndOffset(line_iter->mDocIndexStart, &segment_iter, &segment_offset);
				
				LLRect selection_rect;
				selection_rect.mLeft = 0;
				selection_rect.mRight = 0;
				selection_rect.mBottom = line_iter->mBottom;
				selection_rect.mTop = line_iter->mTop;
					
				for(;segment_iter != mSegments.end(); ++segment_iter, segment_offset = 0)
				{
					LLTextSegmentPtr segmentp = *segment_iter;

					S32 segment_line_start = segmentp->getStart() + segment_offset;
					S32 segment_line_end = llmin(segmentp->getEnd(), line_iter->mDocIndexEnd);

					// if selection after beginning of segment
					if(selection_left >= segment_line_start)
					{
						S32 num_chars = llmin(selection_left, segment_line_end) - segment_line_start;
						selection_rect.mLeft += segmentp->getWidth(segment_offset, num_chars);
					}

					// if selection spans end of current segment...
					if (selection_right > segment_line_end)
					{
						// extend selection slightly beyond end of line
						// to indicate selection of newline character (use "n" character to determine width)
						selection_rect.mRight += segmentp->getWidth(segment_offset, segment_line_end - segment_line_start);
					}
					// else if selection ends on current segment...
					else
					{
						S32 num_chars = selection_right - segment_line_start;
						selection_rect.mRight += segmentp->getWidth(segment_offset, num_chars);

						break;
					}
				}
				selection_rects.push_back(selection_rect);
			}
		}
		
		// Draw the selection box (we're using a box instead of reversing the colors on the selected text).
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		const LLColor4& color = mReadOnly ? mReadOnlyBgColor.get() : mWriteableBgColor.get();
		F32 alpha = hasFocus() ? 0.7f : 0.3f;
		gGL.color4f( 1.f - color.mV[0], 1.f - color.mV[1], 1.f - color.mV[2], alpha );

		for (std::vector<LLRect>::iterator rect_it = selection_rects.begin();
			rect_it != selection_rects.end();
			++rect_it)
		{
			LLRect selection_rect = *rect_it;
			selection_rect.translate(mTextRect.mLeft - content_display_rect.mLeft, mTextRect.mBottom - content_display_rect.mBottom);
			gl_rect_2d(selection_rect);
		}
	}
}

void LLTextEditor::drawCursor()
{
	if( hasFocus()
		&& gFocusMgr.getAppHasFocus()
		&& !mReadOnly)
	{
		LLWString wtext = getWText();
		const llwchar* text = wtext.c_str();

		LLRect cursor_rect = getLocalRectFromDocIndex(mCursorPos);
		cursor_rect.translate(-1, 0);
		segment_set_t::iterator seg_it = getSegIterContaining(mCursorPos);

		// take style from last segment
		LLTextSegmentPtr segmentp;

		if (seg_it != mSegments.end())
		{
			segmentp = *seg_it;
		}
		else
		{
			//segmentp = mSegments.back();
			return;
		}

		// Draw the cursor
		// (Flash the cursor every half second starting a fixed time after the last keystroke)
		F32 elapsed = mKeystrokeTimer.getElapsedTimeF32();
		if( (elapsed < CURSOR_FLASH_DELAY ) || (S32(elapsed * 2) & 1) )
		{

			if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode() && !hasSelection())
			{
				S32 width = llmax(CURSOR_THICKNESS, segmentp->getWidth(mCursorPos - segmentp->getStart(), 1));
				cursor_rect.mRight = cursor_rect.mLeft + width;
			}
			else
			{
				cursor_rect.mRight = cursor_rect.mLeft + CURSOR_THICKNESS;
			}
			
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

			gGL.color4fv( mCursorColor.get().mV );
			
			gl_rect_2d(cursor_rect);

			if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode() && !hasSelection() && text[mCursorPos] != '\n')
			{
				LLColor4 text_color;
				const LLFontGL* fontp;
				if (segmentp)
				{
					text_color = segmentp->getColor();
					fontp = segmentp->getStyle()->getFont();
				}
				else if (mReadOnly)
				{
					text_color = mReadOnlyFgColor.get();
					fontp = mDefaultFont;
				}
				else
				{
					text_color = mFgColor.get();
					fontp = mDefaultFont;
				}
				fontp->render(text, mCursorPos, cursor_rect.mLeft, cursor_rect.mBottom, 
					LLColor4(1.f - text_color.mV[VRED], 1.f - text_color.mV[VGREEN], 1.f - text_color.mV[VBLUE], 1.f),
					LLFontGL::LEFT, LLFontGL::BOTTOM,
					LLFontGL::NORMAL,
					LLFontGL::NO_SHADOW,
					1);
			}

			// Make sure the IME is in the right place
			LLRect screen_pos = calcScreenRect();
			LLCoordGL ime_pos( screen_pos.mLeft + llfloor(cursor_rect.mLeft), screen_pos.mBottom + llfloor(cursor_rect.mTop) );

			ime_pos.mX = (S32) (ime_pos.mX * LLUI::sGLScaleFactor.mV[VX]);
			ime_pos.mY = (S32) (ime_pos.mY * LLUI::sGLScaleFactor.mV[VY]);
			getWindow()->setLanguageTextInput( ime_pos );
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

	S32 cur_line = getFirstVisibleLine();
	if (cur_line >= num_lines)
	{
		return;
	}
		
	const S32 line_height = llround( mDefaultFont->getLineHeight() );

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
					preedit_left += mDefaultFont->getWidth(text, line_start, left - line_start);
				}
				S32 preedit_right = mTextRect.mLeft;
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
	LLRect scrolled_view_rect = mScroller->getVisibleContentRect();
	LLRect content_rect = mScroller->getContentWindowRect();
	S32 first_line = getFirstVisibleLine();
	S32 num_lines = getLineCount();
	if (first_line >= num_lines)
	{
		return;
	}
	
	S32 line_start = getLineStart(first_line);
	// find first text segment that spans top of visible portion of text buffer
	segment_set_t::iterator seg_iter = getSegIterContaining(line_start);
	if (seg_iter == mSegments.end()) 
	{
		return;
	}

	LLTextSegmentPtr cur_segment = *seg_iter;

	for (S32 cur_line = first_line; cur_line < num_lines; cur_line++)
	{
		line_info& line = mLineInfoList[cur_line];

		if ((line.mTop - scrolled_view_rect.mBottom) < mTextRect.mBottom) 
		{
			break;
		}

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

		LLRect text_rect(mTextRect.mLeft - scrolled_view_rect.mLeft,
						line.mTop - scrolled_view_rect.mBottom + mTextRect.mBottom,
						mTextRect.getWidth() - scrolled_view_rect.mLeft,
						line.mBottom - scrolled_view_rect.mBottom + mTextRect.mBottom);

		// draw a single line of text
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
			
			S32 clipped_end	=	llmin( line_end, cur_segment->getEnd() )  - cur_segment->getStart();
			text_rect.mLeft = (S32)(cur_segment->draw(seg_start - cur_segment->getStart(), clipped_end, selection_left, selection_right, text_rect));

			seg_start = clipped_end + cur_segment->getStart();
		}

		line_start = next_start;
	}
}

void LLTextEditor::drawLineNumbers()
{
	LLGLSUIDefault gls_ui;

	LLRect scrolled_view_rect = mScroller->getVisibleContentRect();
	LLRect content_rect = mScroller->getContentWindowRect();
	LLLocalClipRect clip(content_rect);
	S32 first_line = getFirstVisibleLine();
	S32 num_lines = getLineCount();
	if (first_line >= num_lines)
	{
		return;
	}
	
	S32 cursor_line = getCurrentLine();

	if (mShowLineNumbers)
	{
		S32 last_line_num = -1;

		for (S32 cur_line = first_line; cur_line < num_lines; cur_line++)
		{
			line_info& line = mLineInfoList[cur_line];

			if ((line.mTop - scrolled_view_rect.mBottom) < mTextRect.mBottom) 
			{
				break;
			}

			S32 line_bottom = line.mBottom - scrolled_view_rect.mBottom + mTextRect.mBottom;
			// draw the line numbers
			if(line.mLineNum != last_line_num && line.mTop <= scrolled_view_rect.mTop) 
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
	// reflow if needed, on demand
	reflow();

	// then update scroll position, as cursor may have moved
	updateScrollFromCursor();

	LLColor4 bg_color = mReadOnly 
						? mReadOnlyBgColor.get()
						: hasFocus() 
							? mFocusBgColor.get() 
							: mWriteableBgColor.get();

	mDocumentPanel->setBackgroundColor(bg_color);

	drawChildren();
	drawBackground(); //overlays scrolling panel bg
	drawLineNumbers();

	{
		LLLocalClipRect clip(mTextRect);
		drawSelectionBackground();
		drawPreeditMarker();
		drawText();
		drawCursor();
	}

	//RN: the decision was made to always show the orange border for keyboard focus but do not put an insertion caret
	// when in readonly mode
	mBorder->setKeyboardFocusHighlight( hasFocus() );// && !mReadOnly);
}


S32	LLTextEditor::getFirstVisibleLine() const
{
	LLRect visible_region = mScroller->getVisibleContentRect();

	// binary search for line that starts before top of visible buffer
	line_list_t::const_iterator iter = std::lower_bound(mLineInfoList.begin(), mLineInfoList.end(), visible_region.mTop, compare_bottom());

	return iter - mLineInfoList.begin();
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
	const S32 PIXEL_OVERLAP_ON_PAGE_CHANGE = 10;
	if (delta == 0) return;

	//RN: use pixel heights
	S32 line, offset;
	getLineAndOffset( mCursorPos, &line, &offset );

	LLRect cursor_rect = getLocalRectFromDocIndex(mCursorPos);

	if( delta == -1 )
	{
		mScroller->pageUp(PIXEL_OVERLAP_ON_PAGE_CHANGE);
	}
	else
	if( delta == 1 )
	{
		mScroller->pageDown(PIXEL_OVERLAP_ON_PAGE_CHANGE);
	}

	if (getLocalRectFromDocIndex(mCursorPos) == cursor_rect)
	{
		// cursor didn't change apparent position, so move to top or bottom of document, respectively
		if (delta < 0)
		{
			startOfDoc();
		}
		else
		{
			endOfDoc();
		}
	}
	else
	{
		setCursorAtLocalPos(cursor_rect.getCenterX(), cursor_rect.getCenterY(), true, false);
	}
}

void LLTextEditor::changeLine( S32 delta )
{
	S32 line, offset;
	getLineAndOffset( mCursorPos, &line, &offset );

	S32 new_line = line;
	if( (delta < 0) && (line > 0 ) )
	{
		new_line = line - 1;
	}
	else if( (delta > 0) && (line < (getLineCount() - 1)) )
	{
		new_line = line + 1;
	}

	LLRect visible_region = mScroller->getVisibleContentRect();

	S32 new_cursor_pos = getDocIndexFromLocalCoord(mDesiredXPixel, mLineInfoList[new_line].mBottom + mTextRect.mBottom - visible_region.mBottom, TRUE);
	setCursorPos(new_cursor_pos, true);
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
}

void LLTextEditor::getLineAndColumnForPosition( S32 position, S32* line, S32* col, BOOL include_wordwrap )
{
	getLineAndOffset( mCursorPos, line, col, include_wordwrap );
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

void LLTextEditor::startOfDoc()
{
	setCursorPos(0);
}

void LLTextEditor::endOfDoc()
{
	setCursorPos(getLength());
}

// Sets the scrollbar from the cursor position
void LLTextEditor::updateScrollFromCursor()
{
	if (mReadOnly)
	{
		// no cursor in read only mode
		return;
	}

	if (!mScrollNeeded)
	{
		return;
	}
	mScrollNeeded = FALSE; 

	S32 line, offset;
	getLineAndOffset( mCursorPos, &line, &offset ); 

	// scroll so that the cursor is at the top of the page
	LLRect scroller_doc_window = mScroller->getVisibleContentRect();
	LLRect cursor_rect_doc = getLocalRectFromDocIndex(mCursorPos);
	cursor_rect_doc.translate(scroller_doc_window.mLeft, scroller_doc_window.mBottom);
	mScroller->scrollToShowRect(cursor_rect_doc, LLRect(0, scroller_doc_window.getHeight() - 5, scroller_doc_window.getWidth(), 5));
}

void LLTextEditor::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLView::reshape( width, height, called_from_parent );

	// do this first after reshape, because other things depend on
	// up-to-date mTextRect
	updateTextRect();
	
	needsReflow();
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

	setCursorPos(mCursorPos + insert( mCursorPos, utf8str_to_wstring(new_text), FALSE, LLTextSegmentPtr() ));
	
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
	
	LLStyle::Params style_params;
	style_params.color = lcolor;
	if (font_name.empty())
	{
		style_params.font = mDefaultFont;
	}
	else
	{
		style_params.font.name = font_name;
	}
	appendStyledText(new_text, allow_undo, prepend_newline, style_params);
}

void LLTextEditor::appendStyledText(const std::string &new_text, 
									 bool allow_undo, 
									 bool prepend_newline,
									 const LLStyle::Params& style_params)
{
	S32 part = (S32)LLTextParser::WHOLE;
	if(mParseHTML)
	{

		S32 start=0,end=0;
		std::string text = new_text;
		while ( findHTML(text, &start, &end) )
		{
			LLStyle::Params link_params = style_params;
			link_params.color = mLinkColor;
			link_params.font.style = "UNDERLINE";
			link_params.link_href = text.substr(start,end-start);

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
				appendHighlightedText(subtext,allow_undo, prepend_newline, part, style_params); 
			}
			
			appendText(text.substr(start, end-start),allow_undo, prepend_newline, link_params);
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
		if (end < (S32)text.length()) appendHighlightedText(text,allow_undo, prepend_newline, part, style_params);		
	}
	else
	{
		appendHighlightedText(new_text, allow_undo, prepend_newline, part, style_params);
	}
}

void LLTextEditor::appendHighlightedText(const std::string &new_text, 
										 bool allow_undo, 
										 bool prepend_newline,
										 S32  highlight_part,
										 const LLStyle::Params& style_params)
{
	if (mParseHighlights) 
	{
		LLTextParser* highlight = LLTextParser::getInstance();
		
		if (highlight && !style_params.isDefault())
		{
			LLStyle::Params highlight_params = style_params;

			LLSD pieces = highlight->parsePartialLineHighlights(new_text, highlight_params.color(), highlight_part);
			bool lprepend=prepend_newline;
			for (S32 i=0;i<pieces.size();i++)
			{
				LLSD color_llsd = pieces[i]["color"];
				LLColor4 lcolor;
				lcolor.setValue(color_llsd);
				highlight_params.color = lcolor;
				if (i != 0 && (pieces.size() > 1) ) lprepend=FALSE;
				appendText((std::string)pieces[i]["text"], allow_undo, lprepend, highlight_params);
			}
			return;
		}
	}
	appendText(new_text, allow_undo, prepend_newline, style_params);
}

// Appends new text to end of document
void LLTextEditor::appendText(const std::string &new_text, bool allow_undo, bool prepend_newline,
							  const LLStyle::Params& stylep)
{
	if (new_text.empty()) return;

	// Save old state
	S32 selection_start = mSelectionStart;
	S32 selection_end = mSelectionEnd;
	BOOL was_selecting = mIsSelecting;
	S32 cursor_pos = mCursorPos;
	S32 old_length = getLength();
	BOOL cursor_was_at_end = (mCursorPos == old_length);

	deselect();

	setCursorPos(old_length);

	LLWString wide_text;

	// Add carriage return if not first line
	if (getLength() != 0
		&& prepend_newline)
	{
		wide_text = utf8str_to_wstring(std::string("\n") + new_text);
	}
	else
	{
		wide_text = utf8str_to_wstring(new_text);
	}

	LLTextSegmentPtr segmentp;
	if (!stylep.isDefault())
	{
		S32 segment_start = old_length;
		S32 segment_end = old_length + wide_text.size();
		segmentp = new LLNormalTextSegment(new LLStyle(stylep), segment_start, segment_end, *this );
	}

	append(wide_text, TRUE, segmentp);
	
	needsReflow();
	
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

	if( !allow_undo )
	{
		blockUndo();
	}
}


void LLTextEditor::appendWidget(LLView* widget, const std::string &widget_text, bool allow_undo, bool prepend_newline)
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

	LLWString widget_wide_text;

	// Add carriage return if not first line
	if (getLength() != 0
		&& prepend_newline)
	{
		widget_wide_text = utf8str_to_wstring(std::string("\n") + widget_text);
	}
	else
	{
		widget_wide_text = utf8str_to_wstring(widget_text);
	}

	LLTextSegmentPtr segment = new LLInlineViewSegment(widget, old_length, old_length + widget_text.size());
	append(widget_wide_text, FALSE, segment);

	needsReflow();
	
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
	setCursorPos (llclamp(mCursorPos, 0, len));
	mSelectionStart = llclamp(mSelectionStart, 0, len);
	mSelectionEnd = llclamp(mSelectionEnd, 0, len);

	reflow();
	needsScroll();
}

///////////////////////////////////////////////////////////////////
// Returns change in number of characters in mWText

S32 LLTextEditor::insertStringNoUndo(S32 pos, const LLWString &wstr, LLTextEditor::segment_vec_t* segments )
{
	LLWString text(getWText());
	S32 old_len = text.length();		// length() returns character length
	S32 insert_len = wstr.length();

	pos = getEditableIndex(pos, true);

	segment_set_t::iterator seg_iter = getSegIterContaining(pos);

	LLTextSegmentPtr default_segment;

	LLTextSegmentPtr segmentp;
	if (seg_iter != mSegments.end())
	{
		segmentp = *seg_iter;
	}
	else
	{
		//segmentp = mSegments.back();
		return pos;
	}

	if (segmentp->canEdit())
	{
		segmentp->setEnd(segmentp->getEnd() + insert_len);
		if (seg_iter != mSegments.end())
		{
			++seg_iter;
		}
	}
	else
	{
		// create default editable segment to hold new text
		default_segment = new LLNormalTextSegment( getDefaultStyle(), pos, pos + insert_len, *this);
	}

	// shift remaining segments to right
	for(;seg_iter != mSegments.end(); ++seg_iter)
	{
		LLTextSegmentPtr segmentp = *seg_iter;
		segmentp->setStart(segmentp->getStart() + insert_len);
		segmentp->setEnd(segmentp->getEnd() + insert_len);
	}

	// insert new segments
	if (segments)
	{
		if (default_segment.notNull())
		{
			// potentially overwritten by segments passed in
			insertSegment(default_segment);
		}
		for (segment_vec_t::iterator seg_iter = segments->begin();
			seg_iter != segments->end();
			++seg_iter)
		{
			LLTextSegment* segmentp = *seg_iter;
			insertSegment(segmentp);
		}
	}

	text.insert(pos, wstr);
    getViewModel()->setDisplay(text);

	if ( truncate() )
	{
		// The user's not getting everything he's hoping for
		make_ui_sound("UISndBadKeystroke");
		insert_len = getLength() - old_len;
	}

	onValueChange(pos, pos + insert_len);

	return insert_len;
}

S32 LLTextEditor::removeStringNoUndo(S32 pos, S32 length)
{
    LLWString text(getWText());
	segment_set_t::iterator seg_iter = getSegIterContaining(pos);
	while(seg_iter != mSegments.end())
	{
		LLTextSegmentPtr segmentp = *seg_iter;
		S32 end = pos + length;
		if (segmentp->getStart() < pos)
		{
			// deleting from middle of segment
			if (segmentp->getEnd() > end)
			{
				segmentp->setEnd(segmentp->getEnd() - length);
			}
			// truncating segment
			else
			{
				segmentp->setEnd(pos);
			}
		}
		else if (segmentp->getStart() < end)
		{
			// deleting entire segment
			if (segmentp->getEnd() <= end)
			{
				// remove segment
				segmentp->unlinkFromDocument(this);
				segment_set_t::iterator seg_to_erase(seg_iter++);
				mSegments.erase(seg_to_erase);
				continue;
			}
			// deleting head of segment
			else
			{
				segmentp->setStart(pos);
				segmentp->setEnd(segmentp->getEnd() - length);
			}
		}
		else
		{
			// shifting segments backward to fill deleted portion
			segmentp->setStart(segmentp->getStart() - length);
			segmentp->setEnd(segmentp->getEnd() - length);
		}
		++seg_iter;
	}

	text.erase(pos, length);
    getViewModel()->setDisplay(text);

	// recreate default segment in case we erased everything
	createDefaultSegment();

	onValueChange(pos, pos);

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

	onValueChange(pos, pos + 1);

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
	
	LLRect old_text_rect = mTextRect;
	mTextRect = mScroller->getContentWindowRect();
	mTextRect.stretch(texteditor_border * -1);
	mTextRect.mLeft += texteditor_h_pad;
	mTextRect.mLeft += mShowLineNumbers ? UI_TEXTEDITOR_LINE_NUMBER_MARGIN : 0;
	if (mTextRect != old_text_rect)
	{
		needsReflow();
	}
}

LLFastTimer::DeclareTimer FTM_TEXT_EDITOR_LOAD_KEYWORD("Text Editor Load Keywords");
void LLTextEditor::loadKeywords(const std::string& filename,
								const std::vector<std::string>& funcs,
								const std::vector<std::string>& tooltips,
								const LLColor3& color)
{
	LLFastTimer ft(FTM_TEXT_EDITOR_LOAD_KEYWORD);
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

void LLTextEditor::createDefaultSegment()
{
	// ensures that there is always at least one segment
	if (mSegments.empty())
	{
		LLTextSegmentPtr default_segment = new LLNormalTextSegment( getDefaultStyle(), 0, getLength() + 1, *this);
		mSegments.insert(default_segment);
		default_segment->linkToDocument(this);
	}
}

LLStyleSP	LLTextEditor::getDefaultStyle()
{
	LLColor4 text_color = ( mReadOnly ? mReadOnlyFgColor.get() : mFgColor.get() );
	return LLStyleSP(new LLStyle(LLStyle::Params().color(text_color).font(mDefaultFont)));
}

LLFastTimer::DeclareTimer FTM_UPDATE_TEXT_SEGMENTS("Update Text Segments");
void LLTextEditor::updateSegments()
{
	LLFastTimer ft(FTM_UPDATE_TEXT_SEGMENTS);
	if (mKeywords.isLoaded())
	{
		// HACK:  No non-ascii keywords for now
		segment_vec_t segment_list;
		mKeywords.findSegments(&segment_list, getWText(), mDefaultColor.get(), *this);

		mSegments.clear();
		segment_set_t::iterator insert_it = mSegments.begin();
		for (segment_vec_t::iterator list_it = segment_list.begin(); list_it != segment_list.end(); ++list_it)
		{
			insert_it = mSegments.insert(insert_it, *list_it);
		}
	}

	createDefaultSegment();

}

void LLTextEditor::insertSegment(LLTextSegmentPtr segment_to_insert)
{
	if (segment_to_insert.isNull()) 
	{
		return;
	}

	segment_set_t::iterator cur_seg_iter = getSegIterContaining(segment_to_insert->getStart());

	if (cur_seg_iter == mSegments.end())
	{
		mSegments.insert(segment_to_insert);
		segment_to_insert->linkToDocument(this);
	}
	else
	{
		LLTextSegmentPtr cur_segmentp = *cur_seg_iter;
		if (cur_segmentp->getStart() < segment_to_insert->getStart())
		{
			S32 old_segment_end = cur_segmentp->getEnd();
			// split old at start point for new segment
			cur_segmentp->setEnd(segment_to_insert->getStart());
			// advance to next segment
			++cur_seg_iter;
			// insert remainder of old segment
			LLTextSegmentPtr remainder_segment = new LLNormalTextSegment( cur_segmentp->getStyle(), segment_to_insert->getStart(), old_segment_end, *this);
			cur_seg_iter = mSegments.insert(cur_seg_iter, remainder_segment);
			remainder_segment->linkToDocument(this);
			// insert new segment before remainder of old segment
			cur_seg_iter = mSegments.insert(cur_seg_iter, segment_to_insert);

			segment_to_insert->linkToDocument(this);
			// move to "remanider" segment and start truncation there
			++cur_seg_iter;
		}
		else
		{
			cur_seg_iter = mSegments.insert(cur_seg_iter, segment_to_insert);
			++cur_seg_iter;
			segment_to_insert->linkToDocument(this);
		}

		// now delete/truncate remaining segments as necessary
		while(cur_seg_iter != mSegments.end())
		{
			cur_segmentp = *cur_seg_iter;
			if (cur_segmentp->getEnd() <= segment_to_insert->getEnd())
			{
				cur_segmentp->unlinkFromDocument(this);
				segment_set_t::iterator seg_to_erase(cur_seg_iter++);
				mSegments.erase(seg_to_erase);
			}
			else
			{
				cur_segmentp->setStart(segment_to_insert->getEnd());
				break;
			}
		}
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
			if ( (sSecondlifeURLcallback!=NULL) && !(*sSecondlifeURLcallback)(mHTML) )
			{
				if (sURLcallback!=NULL) (*sURLcallback)(mHTML);
			}
			mHTML.clear();
		}
	}

	return FALSE;
}


// Finds the text segment (if any) at the give local screen position
LLTextSegmentPtr LLTextEditor::getSegmentAtLocalPos( S32 x, S32 y )
{
	// Find the cursor position at the requested local screen position
	S32 offset = getDocIndexFromLocalCoord( x, y, FALSE );
	segment_set_t::iterator seg_iter = getSegIterContaining(offset);
	if (seg_iter != mSegments.end())
	{
		return *seg_iter;
	}
	else
	{
		return LLTextSegmentPtr();
	}
}

LLTextEditor::segment_set_t::iterator LLTextEditor::getSegIterContaining(S32 index)
{
	segment_set_t::iterator it = mSegments.upper_bound(new LLIndexSegment(index));
	return it;
}

LLTextEditor::segment_set_t::const_iterator LLTextEditor::getSegIterContaining(S32 index) const
{
	LLTextEditor::segment_set_t::const_iterator it =  mSegments.upper_bound(new LLIndexSegment(index));
	return it;
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
	const S32 line_height = llround(mDefaultFont->getLineHeight());

	if (coord)
	{
		const S32 query_x = mTextRect.mLeft + mDefaultFont->getWidth(text, current_line_start, query - current_line_start);
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
			preedit_left += mDefaultFont->getWidth(text, current_line_start, preedit_left_position - current_line_start);
		}

		S32 preedit_right = mTextRect.mLeft;
		if (preedit_right_position < current_line_end)
		{
			preedit_right += mDefaultFont->getWidth(text, current_line_start, preedit_right_position - current_line_start);
		}
		else
		{
			preedit_right += mDefaultFont->getWidth(text, current_line_start, current_line_end - current_line_start);
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
	return llround(mDefaultFont->getLineHeight() * LLUI::sGLScaleFactor.mV[VY]);
}

LLWString       LLTextEditor::getWText() const
{
    return getViewModel()->getDisplay();
}

void	LLTextEditor::onValueChange(S32 start, S32 end)
{
}

//
// LLTextSegment
//

LLTextSegment::~LLTextSegment()
{}

S32	LLTextSegment::getWidth(S32 first_char, S32 num_chars) const { return 0; }
S32	LLTextSegment::getOffset(S32 segment_local_x_coord, S32 start_offset, S32 num_chars, bool round) const { return 0; }
S32	LLTextSegment::getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const { return 0; }
void LLTextSegment::updateLayout(const LLTextEditor& editor) {}
F32	LLTextSegment::draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect) { return draw_rect.mLeft; }
S32	LLTextSegment::getMaxHeight() const { return 0; }
bool LLTextSegment::canEdit() const { return false; }
void LLTextSegment::unlinkFromDocument(LLTextEditor*) {}
void LLTextSegment::linkToDocument(LLTextEditor*) {}
void LLTextSegment::setHasMouseHover(bool hover) {}
const LLColor4& LLTextSegment::getColor() const { return LLColor4::white; }
void LLTextSegment::setColor(const LLColor4 &color) {}
const LLStyleSP LLTextSegment::getStyle() const {static LLStyleSP sp(new LLStyle()); return sp; }
void LLTextSegment::setStyle(const LLStyleSP &style) {}
void LLTextSegment::setToken( LLKeywordToken* token ) {}
LLKeywordToken*	LLTextSegment::getToken() const { return NULL; }
BOOL LLTextSegment::getToolTip( std::string& msg ) const { return FALSE; }
void LLTextSegment::dump() const {}


//
// LLNormalTextSegment
//

LLNormalTextSegment::LLNormalTextSegment( const LLStyleSP& style, S32 start, S32 end, LLTextEditor& editor ) 
:	LLTextSegment(start, end),
	mStyle( style ),
	mToken(NULL),
	mHasMouseHover(false),
	mEditor(editor)
{
	mMaxHeight = llceil(mStyle->getFont()->getLineHeight());
}

LLNormalTextSegment::LLNormalTextSegment( const LLColor4& color, S32 start, S32 end, LLTextEditor& editor, BOOL is_visible) 
:	LLTextSegment(start, end),
	mToken(NULL),
	mHasMouseHover(false),
	mEditor(editor)
{
	mStyle = new LLStyle(LLStyle::Params().visible(is_visible).color(color));

	mMaxHeight = llceil(mStyle->getFont()->getLineHeight());
}

F32 LLNormalTextSegment::draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect)
{
	if( end - start > 0 )
	{
		if ( mStyle->isImage() && (start >= 0) && (end <= mEnd - mStart))
		{
			S32 style_image_height = mStyle->mImageHeight;
			S32 style_image_width = mStyle->mImageWidth;
			LLUIImagePtr image = mStyle->getImage();
			image->draw(draw_rect.mLeft, draw_rect.mTop-style_image_height, 
				style_image_width, style_image_height);
		}

		return drawClippedSegment( getStart() + start, getStart() + end, selection_start, selection_end, draw_rect.mLeft, draw_rect.mBottom);
	}
	return draw_rect.mLeft;
}

// Draws a single text segment, reversing the color for selection if needed.
F32 LLNormalTextSegment::drawClippedSegment(S32 seg_start, S32 seg_end, S32 selection_start, S32 selection_end, F32 x, F32 y)
{
	const LLWString &text = mEditor.getWText();

	F32 right_x = x;
	if (!mStyle->isVisible())
	{
		return right_x;
	}

	const LLFontGL* font = mStyle->getFont();

	LLColor4 color = mStyle->getColor();

	font = mStyle->getFont();

  	if( selection_start > seg_start )
	{
		// Draw normally
		S32 start = seg_start;
		S32 end = llmin( selection_start, seg_end );
		S32 length =  end - start;
		font->render(text, start, x, y, color, LLFontGL::LEFT, LLFontGL::BOTTOM, 0, LLFontGL::NO_SHADOW, length, S32_MAX, &right_x, mEditor.allowsEmbeddedItems());
	}
	x = right_x;
	
	if( (selection_start < seg_end) && (selection_end > seg_start) )
	{
		// Draw reversed
		S32 start = llmax( selection_start, seg_start );
		S32 end = llmin( selection_end, seg_end );
		S32 length = end - start;

		font->render(text, start, x, y,
					 LLColor4( 1.f - color.mV[0], 1.f - color.mV[1], 1.f - color.mV[2], 1.f ),
					 LLFontGL::LEFT, LLFontGL::BOTTOM, 0, LLFontGL::NO_SHADOW, length, S32_MAX, &right_x, mEditor.allowsEmbeddedItems());
	}
	x = right_x;
	if( selection_end < seg_end )
	{
		// Draw normally
		S32 start = llmax( selection_end, seg_start );
		S32 end = seg_end;
		S32 length = end - start;
		font->render(text, start, x, y, color, LLFontGL::LEFT, LLFontGL::BOTTOM, 0, LLFontGL::NO_SHADOW, length, S32_MAX, &right_x, mEditor.allowsEmbeddedItems());
	}
	return right_x;
}

S32	LLNormalTextSegment::getMaxHeight() const	
{ 
	return mMaxHeight; 
}

BOOL LLNormalTextSegment::getToolTip(std::string& msg) const
{
	if (mToken && !mToken->getToolTip().empty())
	{
		const LLWString& wmsg = mToken->getToolTip();
		msg = wstring_to_utf8str(wmsg);
		return TRUE;
	}
	return FALSE;
}


S32	LLNormalTextSegment::getWidth(S32 first_char, S32 num_chars) const
{
	LLWString text = mEditor.getWText();
	return mStyle->getFont()->getWidth(text.c_str(), mStart + first_char, num_chars);
}

S32	LLNormalTextSegment::getOffset(S32 segment_local_x_coord, S32 start_offset, S32 num_chars, bool round) const
{
	LLWString text = mEditor.getWText();
	return mStyle->getFont()->charFromPixelOffset(text.c_str(), mStart + start_offset,
											   (F32)segment_local_x_coord,
											   F32_MAX,
											   num_chars,
											   round);
}

S32	LLNormalTextSegment::getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const
{
	LLWString text = mEditor.getWText();
	S32 num_chars = mStyle->getFont()->maxDrawableChars(text.c_str() + segment_offset + mStart, 
												(F32)num_pixels,
												max_chars, 
												mEditor.getWordWrap());

	if (num_chars == 0 
		&& line_offset == 0 
		&& max_chars > 0)
	{
		// If at the beginning of a line, and a single character won't fit, draw it anyway
		num_chars = 1;
	}
	if (mStart + segment_offset + num_chars == mEditor.getLength())
	{
		// include terminating NULL
		num_chars++;
	}
	return num_chars;
}

void LLNormalTextSegment::dump() const
{
	llinfos << "Segment [" << 
//			mColor.mV[VX] << ", " <<
//			mColor.mV[VY] << ", " <<
//			mColor.mV[VZ] << "]\t[" <<
		mStart << ", " <<
		getEnd() << "]" <<
		llendl;
}

//
// LLInlineViewSegment
//

LLInlineViewSegment::LLInlineViewSegment(LLView* view, S32 start, S32 end)
:	LLTextSegment(start, end),
	mView(view)
{
} 

LLInlineViewSegment::~LLInlineViewSegment()
{
	mView->die();
}

S32	LLInlineViewSegment::getWidth(S32 first_char, S32 num_chars) const
{
	if (first_char == 0 && num_chars == 0) 
	{
		return 0;
	}
	else
	{
		return mView->getRect().getWidth();
	}
}

S32	LLInlineViewSegment::getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const
{
	if (line_offset != 0 && num_pixels < mView->getRect().getWidth()) 
	{
		return 0;
	}
	else
	{
		return mEnd - mStart;
	}
}

void LLInlineViewSegment::updateLayout(const LLTextEditor& editor)
{
	LLRect start_rect = editor.getLocalRectFromDocIndex(mStart);
	LLRect doc_rect = editor.getDocumentPanel()->getRect();
	mView->setOrigin(doc_rect.mLeft + start_rect.mLeft, doc_rect.mBottom + start_rect.mBottom);
}

F32	LLInlineViewSegment::draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect)
{
	return (F32)(draw_rect.mLeft + mView->getRect().getWidth());
}

S32	LLInlineViewSegment::getMaxHeight() const
{
	return mView->getRect().getHeight();
}

void LLInlineViewSegment::unlinkFromDocument(LLTextEditor* editor)
{
	editor->removeDocumentChild(mView);
}

void LLInlineViewSegment::linkToDocument(LLTextEditor* editor)
{
	editor->addDocumentChild(mView);
}

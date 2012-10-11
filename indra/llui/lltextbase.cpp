/** 
 * @file lltextbase.cpp
 * @author Martin Reddy
 * @brief The base class of text box/editor, providing Url handling support
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2009-2010, Linden Research, Inc.
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

#include "lltextbase.h"

#include "lllocalcliprect.h"
#include "llmenugl.h"
#include "llscrollcontainer.h"
#include "llspellcheck.h"
#include "llstl.h"
#include "lltextparser.h"
#include "lltextutil.h"
#include "lltooltip.h"
#include "lluictrl.h"
#include "llurlaction.h"
#include "llurlregistry.h"
#include "llview.h"
#include "llwindow.h"
#include <boost/bind.hpp>

const F32	CURSOR_FLASH_DELAY = 1.0f;  // in seconds
const S32	CURSOR_THICKNESS = 2;

LLTextBase::line_info::line_info(S32 index_start, S32 index_end, LLRect rect, S32 line_num) 
:	mDocIndexStart(index_start), 
	mDocIndexEnd(index_end),
	mRect(rect),
	mLineNum(line_num)
{}

bool LLTextBase::compare_segment_end::operator()(const LLTextSegmentPtr& a, const LLTextSegmentPtr& b) const
{
	// sort empty spans (e.g. 11-11) after previous non-empty spans (e.g. 5-11)
	if (a->getEnd() == b->getEnd())
	{
		return a->getStart() < b->getStart();
	}
	else
	{
		return a->getEnd() < b->getEnd();
	}
}


// helper functors
struct LLTextBase::compare_bottom
{
	bool operator()(const S32& a, const LLTextBase::line_info& b) const
	{
		return a > b.mRect.mBottom; // bottom of a is higher than bottom of b
	}

	bool operator()(const LLTextBase::line_info& a, const S32& b) const
	{
		return a.mRect.mBottom > b; // bottom of a is higher than bottom of b
	}

	bool operator()(const LLTextBase::line_info& a, const LLTextBase::line_info& b) const
	{
		return a.mRect.mBottom > b.mRect.mBottom; // bottom of a is higher than bottom of b
	}

};

// helper functors
struct LLTextBase::compare_top
{
	bool operator()(const S32& a, const LLTextBase::line_info& b) const
	{
		return a > b.mRect.mTop; // top of a is higher than top of b
	}

	bool operator()(const LLTextBase::line_info& a, const S32& b) const
	{
		return a.mRect.mTop > b; // top of a is higher than top of b
	}

	bool operator()(const LLTextBase::line_info& a, const LLTextBase::line_info& b) const
	{
		return a.mRect.mTop > b.mRect.mTop; // top of a is higher than top of b
	}
};

struct LLTextBase::line_end_compare
{
	bool operator()(const S32& pos, const LLTextBase::line_info& info) const
	{
		return (pos < info.mDocIndexEnd);
	}

	bool operator()(const LLTextBase::line_info& info, const S32& pos) const
	{
		return (info.mDocIndexEnd < pos);
	}

	bool operator()(const LLTextBase::line_info& a, const LLTextBase::line_info& b) const
	{
		return (a.mDocIndexEnd < b.mDocIndexEnd);
	}

};

//////////////////////////////////////////////////////////////////////////
//
// LLTextBase
//

// register LLTextBase::Params under name "textbase"
static LLWidgetNameRegistry::StaticRegistrar sRegisterTextBaseParams(&typeid(LLTextBase::Params), "textbase");

LLTextBase::LineSpacingParams::LineSpacingParams()
:	multiple("multiple", 1.f),
	pixels("pixels", 0)
{
}


LLTextBase::Params::Params()
:	cursor_color("cursor_color"),
	text_color("text_color"),
	text_readonly_color("text_readonly_color"),
	bg_visible("bg_visible", false),
	border_visible("border_visible", false),
	bg_readonly_color("bg_readonly_color"),
	bg_writeable_color("bg_writeable_color"),
	bg_focus_color("bg_focus_color"),
	text_selected_color("text_selected_color"),
	bg_selected_color("bg_selected_color"),
	allow_scroll("allow_scroll", true),
	plain_text("plain_text",false),
	track_end("track_end", false),
	read_only("read_only", false),
	spellcheck("spellcheck", false),
	v_pad("v_pad", 0),
	h_pad("h_pad", 0),
	clip("clip", true),
	clip_partial("clip_partial", true),
	line_spacing("line_spacing"),
	max_text_length("max_length", 255),
	font_shadow("font_shadow"),
	wrap("wrap"),
	use_ellipses("use_ellipses", false),
	parse_urls("parse_urls", false),
	parse_highlights("parse_highlights", false)
{
	addSynonym(track_end, "track_bottom");
	addSynonym(wrap, "word_wrap");
	addSynonym(parse_urls, "allow_html");
}


LLTextBase::LLTextBase(const LLTextBase::Params &p) 
:	LLUICtrl(p, LLTextViewModelPtr(new LLTextViewModel)),
	mURLClickSignal(NULL),
	mMaxTextByteLength( p.max_text_length ),
	mDefaultFont(p.font),
	mFontShadow(p.font_shadow),
	mPopupMenu(NULL),
	mReadOnly(p.read_only),
	mSpellCheck(p.spellcheck),
	mSpellCheckStart(-1),
	mSpellCheckEnd(-1),
	mCursorColor(p.cursor_color),
	mFgColor(p.text_color),
	mBorderVisible( p.border_visible ),
	mReadOnlyFgColor(p.text_readonly_color),
	mWriteableBgColor(p.bg_writeable_color),
	mReadOnlyBgColor(p.bg_readonly_color),
	mFocusBgColor(p.bg_focus_color),
	mTextSelectedColor(p.text_selected_color),
	mSelectedBGColor(p.bg_selected_color),
	mReflowIndex(S32_MAX),
	mCursorPos( 0 ),
	mScrollNeeded(FALSE),
	mDesiredXPixel(-1),
	mHPad(p.h_pad),
	mVPad(p.v_pad),
	mHAlign(p.font_halign),
	mVAlign(p.font_valign),
	mLineSpacingMult(p.line_spacing.multiple),
	mLineSpacingPixels(p.line_spacing.pixels),
	mClip(p.clip),
	mClipPartial(p.clip_partial && !p.allow_scroll),
	mTrackEnd( p.track_end ),
	mScrollIndex(-1),
	mSelectionStart( 0 ),
	mSelectionEnd( 0 ),
	mIsSelecting( FALSE ),
	mPlainText ( p.plain_text ),
	mWordWrap(p.wrap),
	mUseEllipses( p.use_ellipses ),
	mParseHTML(p.parse_urls),
	mParseHighlights(p.parse_highlights),
	mBGVisible(p.bg_visible),
	mScroller(NULL),
	mStyleDirty(true)
{
	if(p.allow_scroll)
	{
		LLScrollContainer::Params scroll_params;
		scroll_params.name = "text scroller";
		scroll_params.rect = getLocalRect();
		scroll_params.follows.flags = FOLLOWS_ALL;
		scroll_params.is_opaque = false;
		scroll_params.mouse_opaque = false;
		scroll_params.min_auto_scroll_rate = 200;
		scroll_params.max_auto_scroll_rate = 800;
		scroll_params.border_visible = p.border_visible;
		mScroller = LLUICtrlFactory::create<LLScrollContainer>(scroll_params);
		addChild(mScroller);
	}

	LLView::Params view_params;
	view_params.name = "text_contents";
	view_params.rect =  LLRect(0, 500, 500, 0);
	view_params.mouse_opaque = false;

	mDocumentView = LLUICtrlFactory::create<LLView>(view_params);
	if (mScroller)
	{
		mScroller->addChild(mDocumentView);
	}
	else
	{
		addChild(mDocumentView);
	}

	if (mSpellCheck)
	{
		LLSpellChecker::setSettingsChangeCallback(boost::bind(&LLTextBase::onSpellCheckSettingsChange, this));
	}
	mSpellCheckTimer.reset();

	createDefaultSegment();

	updateRects();
}

LLTextBase::~LLTextBase()
{
	mSegments.clear();
	delete mURLClickSignal;
}

void LLTextBase::initFromParams(const LLTextBase::Params& p)
{
	LLUICtrl::initFromParams(p);
	resetDirty();		// Update saved text state
	updateSegments();

	// HACK: work around enabled == readonly design bug -- RN
	// setEnabled will modify our read only status, so do this after
	// LLTextBase::initFromParams
	if (p.read_only.isProvided())
	{
		mReadOnly = p.read_only;
	}
}

bool LLTextBase::truncate()
{
	BOOL did_truncate = FALSE;

	// First rough check - if we're less than 1/4th the size, we're OK
	if (getLength() >= S32(mMaxTextByteLength / 4))
	{	
		// Have to check actual byte size
		S32 utf8_byte_size = 0;
		LLSD value = getViewModel()->getValue();
		if (value.type() == LLSD::TypeString)
		{
			// save a copy for strings.
			utf8_byte_size = value.size();
		}
		else
		{
			// non string LLSDs need explicit conversion to string
			utf8_byte_size = value.asString().size();
		}

		if ( utf8_byte_size > mMaxTextByteLength )
		{
			// Truncate safely in UTF-8
			std::string temp_utf8_text = value.asString();
			temp_utf8_text = utf8str_truncate( temp_utf8_text, mMaxTextByteLength );
			LLWString text = utf8str_to_wstring( temp_utf8_text );
			// remove extra bit of current string, to preserve formatting, etc.
			removeStringNoUndo(text.size(), getWText().size() - text.size());
			did_truncate = TRUE;
		}
	}

	return did_truncate;
}

const LLStyle::Params& LLTextBase::getDefaultStyleParams()
{
	//FIXME: convert mDefaultStyle to a flyweight http://www.boost.org/doc/libs/1_40_0/libs/flyweight/doc/index.html
	//and eliminate color member values
	if (mStyleDirty)
	{
		  mDefaultStyle
				  .color(LLUIColor(&mFgColor))						// pass linked color instead of copy of mFGColor
				  .readonly_color(LLUIColor(&mReadOnlyFgColor))
				  .selected_color(LLUIColor(&mTextSelectedColor))
				  .font(mDefaultFont)
				  .drop_shadow(mFontShadow);
		  mStyleDirty = false;
	}
	return mDefaultStyle;
}

void LLTextBase::onValueChange(S32 start, S32 end)
{
}


// Draws the black box behind the selected text
void LLTextBase::drawSelectionBackground()
{
	// Draw selection even if we don't have keyboard focus for search/replace
	if( hasSelection() && !mLineInfoList.empty())
	{
		std::vector<LLRect> selection_rects;

		S32 selection_left		= llmin( mSelectionStart, mSelectionEnd );
		S32 selection_right		= llmax( mSelectionStart, mSelectionEnd );

		// Skip through the lines we aren't drawing.
		LLRect content_display_rect = getVisibleDocumentRect();

		// binary search for line that starts before top of visible buffer
		line_list_t::const_iterator line_iter = std::lower_bound(mLineInfoList.begin(), mLineInfoList.end(), content_display_rect.mTop, compare_bottom());
		line_list_t::const_iterator end_iter = std::upper_bound(mLineInfoList.begin(), mLineInfoList.end(), content_display_rect.mBottom, compare_top());

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
				selection_rect.mLeft = line_iter->mRect.mLeft;
				selection_rect.mRight = line_iter->mRect.mLeft;
				selection_rect.mBottom = line_iter->mRect.mBottom;
				selection_rect.mTop = line_iter->mRect.mTop;
					
				for(;segment_iter != mSegments.end(); ++segment_iter, segment_offset = 0)
				{
					LLTextSegmentPtr segmentp = *segment_iter;

					S32 segment_line_start = segmentp->getStart() + segment_offset;
					S32 segment_line_end = llmin(segmentp->getEnd(), line_iter->mDocIndexEnd);

					if (segment_line_start > segment_line_end) break;

					S32 segment_width = 0;
					S32 segment_height = 0;

					// if selection after beginning of segment
					if(selection_left >= segment_line_start)
					{
						S32 num_chars = llmin(selection_left, segment_line_end) - segment_line_start;
						segmentp->getDimensions(segment_offset, num_chars, segment_width, segment_height);
						selection_rect.mLeft += segment_width;
					}

					// if selection_right == segment_line_end then that means we are the first character of the next segment
					// or first character of the next line, in either case we want to add the length of the current segment
					// to the selection rectangle and continue.
					// if selection right > segment_line_end then selection spans end of current segment...
					if (selection_right >= segment_line_end)
					{
						// extend selection slightly beyond end of line
						// to indicate selection of newline character (use "n" character to determine width)
						S32 num_chars = segment_line_end - segment_line_start;
						segmentp->getDimensions(segment_offset, num_chars, segment_width, segment_height);
						selection_rect.mRight += segment_width;
					}
					// else if selection ends on current segment...
					else
					{
						S32 num_chars = selection_right - segment_line_start;
						segmentp->getDimensions(segment_offset, num_chars, segment_width, segment_height);
						selection_rect.mRight += segment_width;

						break;
					}
				}
				selection_rects.push_back(selection_rect);
			}
		}
		
		// Draw the selection box (we're using a box instead of reversing the colors on the selected text).
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		const LLColor4& color = mSelectedBGColor;
		F32 alpha = hasFocus() ? 0.7f : 0.3f;
		alpha *= getDrawContext().mAlpha;
		LLColor4 selection_color(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE], alpha);

		for (std::vector<LLRect>::iterator rect_it = selection_rects.begin();
			rect_it != selection_rects.end();
			++rect_it)
		{
			LLRect selection_rect = *rect_it;
			selection_rect.translate(mVisibleTextRect.mLeft - content_display_rect.mLeft, mVisibleTextRect.mBottom - content_display_rect.mBottom);
			gl_rect_2d(selection_rect, selection_color);
		}
	}
}

void LLTextBase::drawCursor()
{
	F32 alpha = getDrawContext().mAlpha;

	if( hasFocus()
		&& gFocusMgr.getAppHasFocus()
		&& !mReadOnly)
	{
		const LLWString &wtext = getWText();
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
			return;
		}

		// Draw the cursor
		// (Flash the cursor every half second starting a fixed time after the last keystroke)
		F32 elapsed = mCursorBlinkTimer.getElapsedTimeF32();
		if( (elapsed < CURSOR_FLASH_DELAY ) || (S32(elapsed * 2) & 1) )
		{

			if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode() && !hasSelection())
			{
				S32 segment_width = 0;
				S32 segment_height = 0;
				segmentp->getDimensions(mCursorPos - segmentp->getStart(), 1, segment_width, segment_height);
				S32 width = llmax(CURSOR_THICKNESS, segment_width);
				cursor_rect.mRight = cursor_rect.mLeft + width;
			}
			else
			{
				cursor_rect.mRight = cursor_rect.mLeft + CURSOR_THICKNESS;
			}
			
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

			LLColor4 cursor_color = mCursorColor.get() % alpha;
			gGL.color4fv( cursor_color.mV );
			
			gl_rect_2d(cursor_rect);

			if (LL_KIM_OVERWRITE == gKeyboard->getInsertMode() && !hasSelection() && text[mCursorPos] != '\n')
			{
				LLColor4 text_color;
				const LLFontGL* fontp;
				text_color = segmentp->getColor();
				fontp = segmentp->getStyle()->getFont();
				fontp->render(text, mCursorPos, cursor_rect, 
					LLColor4(1.f - text_color.mV[VRED], 1.f - text_color.mV[VGREEN], 1.f - text_color.mV[VBLUE], alpha),
					LLFontGL::LEFT, mVAlign,
					LLFontGL::NORMAL,
					LLFontGL::NO_SHADOW,
					1);
			}

			// Make sure the IME is in the right place
			LLRect screen_pos = calcScreenRect();
			LLCoordGL ime_pos( screen_pos.mLeft + llfloor(cursor_rect.mLeft), screen_pos.mBottom + llfloor(cursor_rect.mTop) );

			ime_pos.mX = (S32) (ime_pos.mX * LLUI::getScaleFactor().mV[VX]);
			ime_pos.mY = (S32) (ime_pos.mY * LLUI::getScaleFactor().mV[VY]);
			getWindow()->setLanguageTextInput( ime_pos );
		}
	}
}

void LLTextBase::drawText()
{
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

	std::pair<S32, S32> line_range = getVisibleLines(mClipPartial);
	S32 first_line = line_range.first;
	S32 last_line = line_range.second;
	if (first_line >= last_line)
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

	// Perform spell check if needed
	if ( (getSpellCheck()) && (getWText().length() > 2) )
	{
		// Calculate start and end indices for the spell checking range
		S32 start = line_start, end = getLineEnd(last_line);

		if ( (mSpellCheckStart != start) || (mSpellCheckEnd != end) )
		{
			const LLWString& wstrText = getWText(); 
			mMisspellRanges.clear();

			segment_set_t::const_iterator seg_it = getSegIterContaining(start);
			while (mSegments.end() != seg_it)
			{
				LLTextSegmentPtr text_segment = *seg_it;
				if ( (text_segment.isNull()) || (text_segment->getStart() >= end) )
				{
					break;
				}

				if (!text_segment->canEdit())
				{
					++seg_it;
					continue;
				}

				// Combine adjoining text segments into one
				U32 seg_start = text_segment->getStart(), seg_end = llmin(text_segment->getEnd(), end);
				while (mSegments.end() != ++seg_it)
				{
					text_segment = *seg_it;
					if ( (text_segment.isNull()) || (!text_segment->canEdit()) || (text_segment->getStart() >= end) )
					{
						break;
					}
					seg_end = llmin(text_segment->getEnd(), end);
				}

				// Find the start of the first word
				U32 word_start = seg_start, word_end = -1;
				while ( (word_start < wstrText.length()) && (!LLStringOps::isAlpha(wstrText[word_start])) )
				{
					word_start++;
				}

				// Iterate over all words in the text block and check them one by one
				while (word_start < seg_end)
				{
					// Find the end of the current word (special case handling for "'" when it's used as a contraction)
					word_end = word_start + 1;
					while ( (word_end < seg_end) && 
							((LLWStringUtil::isPartOfWord(wstrText[word_end])) ||
								((L'\'' == wstrText[word_end]) && 
								(LLStringOps::isAlnum(wstrText[word_end - 1])) && (LLStringOps::isAlnum(wstrText[word_end + 1])))) )
					{
						word_end++;
					}
					if (word_end > seg_end)
					{
						break;
					}

					// Don't process words shorter than 3 characters
					std::string word = wstring_to_utf8str(wstrText.substr(word_start, word_end - word_start));
					if ( (word.length() >= 3) && (!LLSpellChecker::instance().checkSpelling(word)) )
					{
						mMisspellRanges.push_back(std::pair<U32, U32>(word_start, word_end));
					}

					// Find the start of the next word
					word_start = word_end + 1;
					while ( (word_start < seg_end) && (!LLWStringUtil::isPartOfWord(wstrText[word_start])) )
					{
						word_start++;
					}
				}
			}

			mSpellCheckStart = start;
			mSpellCheckEnd = end;
		}
	}

	LLTextSegmentPtr cur_segment = *seg_iter;

	std::list<std::pair<U32, U32> >::const_iterator misspell_it = std::lower_bound(mMisspellRanges.begin(), mMisspellRanges.end(), std::pair<U32, U32>(line_start, 0));
	for (S32 cur_line = first_line; cur_line < last_line; cur_line++)
	{
		S32 next_line = cur_line + 1;
		line_info& line = mLineInfoList[cur_line];

		S32 next_start = -1;
		S32 line_end = text_len;

		if (next_line < getLineCount())
		{
			next_start = getLineStart(next_line);
			line_end = next_start;
		}

		LLRect text_rect(line.mRect);
		text_rect.mRight = mDocumentView->getRect().getWidth(); // clamp right edge to document extents
		text_rect.translate(mDocumentView->getRect().mLeft, mDocumentView->getRect().mBottom); // adjust by scroll position

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
			
			S32 seg_end = llmin(line_end, cur_segment->getEnd());
			S32 clipped_end	= seg_end - cur_segment->getStart();

			if (mUseEllipses								// using ellipses
				&& clipped_end == line_end					// last segment on line
				&& next_line == last_line					// this is the last visible line
				&& last_line < (S32)mLineInfoList.size())	// and there is more text to display
			{
				// more lines of text to go, but we can't fit them
				// so shrink text rect to force ellipses
				text_rect.mRight -= 2;
			}

			// Draw squiggly lines under any visible misspelled words
			while ( (mMisspellRanges.end() != misspell_it) && (misspell_it->first < seg_end) && (misspell_it->second > seg_start) )
			{
				// Skip the current word if the user is still busy editing it
				if ( (!mSpellCheckTimer.hasExpired()) && (misspell_it->first <= (U32)mCursorPos) && (misspell_it->second >= (U32)mCursorPos) )
				{
					++misspell_it;
 					continue;
				}

				U32 misspell_start = llmax<U32>(misspell_it->first, seg_start), misspell_end = llmin<U32>(misspell_it->second, seg_end);
				S32 squiggle_start = 0, squiggle_end = 0, pony = 0;
				cur_segment->getDimensions(seg_start - cur_segment->getStart(), misspell_start - seg_start, squiggle_start, pony);
				cur_segment->getDimensions(misspell_start - cur_segment->getStart(), misspell_end - misspell_start, squiggle_end, pony);
				squiggle_start += text_rect.mLeft;

				pony = (squiggle_end + 3) / 6;
				squiggle_start += squiggle_end / 2 - pony * 3;
				squiggle_end = squiggle_start + pony * 6;

				S32 squiggle_bottom = text_rect.mBottom + (S32)cur_segment->getStyle()->getFont()->getDescenderHeight();

				gGL.color4ub(255, 0, 0, 200);
				while (squiggle_start + 1 < squiggle_end)
				{
					gl_line_2d(squiggle_start, squiggle_bottom, squiggle_start + 2, squiggle_bottom - 2);
					if (squiggle_start + 3 < squiggle_end)
					{
						gl_line_2d(squiggle_start + 2, squiggle_bottom - 3, squiggle_start + 4, squiggle_bottom - 1);
					}
					squiggle_start += 4;
				}

				if (misspell_it->second > seg_end)
				{
					break;
				}
				++misspell_it;
			}

			text_rect.mLeft = (S32)(cur_segment->draw(seg_start - cur_segment->getStart(), clipped_end, selection_left, selection_right, text_rect));

			seg_start = clipped_end + cur_segment->getStart();
		}

		line_start = next_start;
	}
}

///////////////////////////////////////////////////////////////////
// Returns change in number of characters in mWText

S32 LLTextBase::insertStringNoUndo(S32 pos, const LLWString &wstr, LLTextBase::segment_vec_t* segments )
{
	S32 old_len = getLength();		// length() returns character length
	S32 insert_len = wstr.length();

	pos = getEditableIndex(pos, true);

	segment_set_t::iterator seg_iter = getEditableSegIterContaining(pos);

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
		LLStyleConstSP sp(new LLStyle(getDefaultStyleParams()));
		default_segment = new LLNormalTextSegment( sp, pos, pos + insert_len, *this);
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

	getViewModel()->getEditableDisplay().insert(pos, wstr);

	if ( truncate() )
	{
		insert_len = getLength() - old_len;
	}

	onValueChange(pos, pos + insert_len);
	needsReflow(pos);

	return insert_len;
}

S32 LLTextBase::removeStringNoUndo(S32 pos, S32 length)
{
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

	getViewModel()->getEditableDisplay().erase(pos, length);

	// recreate default segment in case we erased everything
	createDefaultSegment();

	onValueChange(pos, pos);
	needsReflow(pos);

	return -length;	// This will be wrong if someone calls removeStringNoUndo with an excessive length
}

S32 LLTextBase::overwriteCharNoUndo(S32 pos, llwchar wc)
{
	if (pos > (S32)getLength())
	{
		return 0;
	}
	getViewModel()->getEditableDisplay()[pos] = wc;

	onValueChange(pos, pos + 1);
	needsReflow(pos);

	return 1;
}


void LLTextBase::createDefaultSegment()
{
	// ensures that there is always at least one segment
	if (mSegments.empty())
	{
		LLStyleConstSP sp(new LLStyle(getDefaultStyleParams()));
		LLTextSegmentPtr default_segment = new LLNormalTextSegment( sp, 0, getLength() + 1, *this);
		mSegments.insert(default_segment);
		default_segment->linkToDocument(this);
	}
}

void LLTextBase::insertSegment(LLTextSegmentPtr segment_to_insert)
{
	if (segment_to_insert.isNull()) 
	{
		return;
	}

	segment_set_t::iterator cur_seg_iter = getSegIterContaining(segment_to_insert->getStart());
	S32 reflow_start_index = 0;

	if (cur_seg_iter == mSegments.end())
	{
		mSegments.insert(segment_to_insert);
		segment_to_insert->linkToDocument(this);
		reflow_start_index = segment_to_insert->getStart();
	}
	else
	{
		LLTextSegmentPtr cur_segmentp = *cur_seg_iter;
		reflow_start_index = cur_segmentp->getStart();
		if (cur_segmentp->getStart() < segment_to_insert->getStart())
		{
			S32 old_segment_end = cur_segmentp->getEnd();
			// split old at start point for new segment
			cur_segmentp->setEnd(segment_to_insert->getStart());
			// advance to next segment
			// insert remainder of old segment
			LLStyleConstSP sp = cur_segmentp->getStyle();
			LLTextSegmentPtr remainder_segment = new LLNormalTextSegment( sp, segment_to_insert->getStart(), old_segment_end, *this);
			mSegments.insert(cur_seg_iter, remainder_segment);
			remainder_segment->linkToDocument(this);
			// insert new segment before remainder of old segment
			mSegments.insert(cur_seg_iter, segment_to_insert);

			segment_to_insert->linkToDocument(this);
			// at this point, there will be two overlapping segments owning the text
			// associated with the incoming segment
		}
		else
		{
			mSegments.insert(cur_seg_iter, segment_to_insert);
			segment_to_insert->linkToDocument(this);
		}

		// now delete/truncate remaining segments as necessary
		// cur_seg_iter points to segment before incoming segment
		while(cur_seg_iter != mSegments.end())
		{
			cur_segmentp = *cur_seg_iter;
			if (cur_segmentp == segment_to_insert) 
			{
				++cur_seg_iter;
				continue;
			}

			if (cur_segmentp->getStart() >= segment_to_insert->getStart())
			{
				if(cur_segmentp->getEnd() <= segment_to_insert->getEnd())
				{
					cur_segmentp->unlinkFromDocument(this);
					// grab copy of iterator to erase, and bump it
					segment_set_t::iterator seg_to_erase(cur_seg_iter++);
					mSegments.erase(seg_to_erase);
					continue;
				}
				else
				{
					// last overlapping segment, clip to end of incoming segment
					// and stop traversal
					cur_segmentp->setStart(segment_to_insert->getEnd());
					break;
				}
			}
			++cur_seg_iter;
		}
	}

	// layout potentially changed
	needsReflow(reflow_start_index);
}

BOOL LLTextBase::handleMouseDown(S32 x, S32 y, MASK mask)
{
	LLTextSegmentPtr cur_segment = getSegmentAtLocalPos(x, y);
	if (cur_segment && cur_segment->handleMouseDown(x, y, mask))
	{
		return TRUE;
	}

	return LLUICtrl::handleMouseDown(x, y, mask);
}

BOOL LLTextBase::handleMouseUp(S32 x, S32 y, MASK mask)
{
	LLTextSegmentPtr cur_segment = getSegmentAtLocalPos(x, y);
	if (cur_segment && cur_segment->handleMouseUp(x, y, mask))
	{
		// Did we just click on a link?
		if (mURLClickSignal
			&& cur_segment->getStyle()
			&& cur_segment->getStyle()->isLink())
		{
			// *TODO: send URL here?
			(*mURLClickSignal)(this, LLSD() );
		}
		return TRUE;
	}

	return LLUICtrl::handleMouseUp(x, y, mask);
}

BOOL LLTextBase::handleMiddleMouseDown(S32 x, S32 y, MASK mask)
{
	LLTextSegmentPtr cur_segment = getSegmentAtLocalPos(x, y);
	if (cur_segment && cur_segment->handleMiddleMouseDown(x, y, mask))
	{
		return TRUE;
	}

	return LLUICtrl::handleMiddleMouseDown(x, y, mask);
}

BOOL LLTextBase::handleMiddleMouseUp(S32 x, S32 y, MASK mask)
{
	LLTextSegmentPtr cur_segment = getSegmentAtLocalPos(x, y);
	if (cur_segment && cur_segment->handleMiddleMouseUp(x, y, mask))
	{
		return TRUE;
	}

	return LLUICtrl::handleMiddleMouseUp(x, y, mask);
}

BOOL LLTextBase::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	LLTextSegmentPtr cur_segment = getSegmentAtLocalPos(x, y);
	if (cur_segment && cur_segment->handleRightMouseDown(x, y, mask))
	{
		return TRUE;
	}

	return LLUICtrl::handleRightMouseDown(x, y, mask);
}

BOOL LLTextBase::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	LLTextSegmentPtr cur_segment = getSegmentAtLocalPos(x, y);
	if (cur_segment && cur_segment->handleRightMouseUp(x, y, mask))
	{
		return TRUE;
	}

	return LLUICtrl::handleRightMouseUp(x, y, mask);
}

BOOL LLTextBase::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	LLTextSegmentPtr cur_segment = getSegmentAtLocalPos(x, y);
	if (cur_segment && cur_segment->handleDoubleClick(x, y, mask))
	{
		return TRUE;
	}

	return LLUICtrl::handleDoubleClick(x, y, mask);
}

BOOL LLTextBase::handleHover(S32 x, S32 y, MASK mask)
{
	LLTextSegmentPtr cur_segment = getSegmentAtLocalPos(x, y);
	if (cur_segment && cur_segment->handleHover(x, y, mask))
	{
		return TRUE;
	}

	return LLUICtrl::handleHover(x, y, mask);
}

BOOL LLTextBase::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	LLTextSegmentPtr cur_segment = getSegmentAtLocalPos(x, y);
	if (cur_segment && cur_segment->handleScrollWheel(x, y, clicks))
	{
		return TRUE;
	}

	return LLUICtrl::handleScrollWheel(x, y, clicks);
}

BOOL LLTextBase::handleToolTip(S32 x, S32 y, MASK mask)
{
	LLTextSegmentPtr cur_segment = getSegmentAtLocalPos(x, y);
	if (cur_segment && cur_segment->handleToolTip(x, y, mask))
	{
		return TRUE;
	}

	return LLUICtrl::handleToolTip(x, y, mask);
}


void LLTextBase::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	if (width != getRect().getWidth() || height != getRect().getHeight())
	{
		bool scrolled_to_bottom = mScroller ? mScroller->isAtBottom() : false;

		LLUICtrl::reshape( width, height, called_from_parent );

		if (mScroller && scrolled_to_bottom && mTrackEnd)
		{
			// keep bottom of text buffer visible
			// do this here as well as in reflow to handle case
			// where shrinking from top, which causes buffer to temporarily 
			// not be scrolled to the bottom, since the scroll index
			// specified the _top_ of the visible document region
			mScroller->goToBottom();
		}

		// do this first after reshape, because other things depend on
		// up-to-date mVisibleTextRect
		updateRects();
		
		needsReflow();
	}
}

void LLTextBase::draw()
{
	// reflow if needed, on demand
	reflow();

	// then update scroll position, as cursor may have moved
	if (!mReadOnly)
	{
		updateScrollFromCursor();
	}

	LLRect text_rect;
	if (mScroller)
	{
		mScroller->localRectToOtherView(mScroller->getContentWindowRect(), &text_rect, this);
	}
	else
	{
		LLRect visible_lines_rect;
		std::pair<S32, S32> line_range = getVisibleLines(mClipPartial);
		for (S32 i = line_range.first; i < line_range.second; i++)
		{
			if (visible_lines_rect.isEmpty())
			{
				visible_lines_rect = mLineInfoList[i].mRect;
			}
			else
			{
				visible_lines_rect.unionWith(mLineInfoList[i].mRect);
			}
		}
		text_rect = visible_lines_rect;
		text_rect.translate(mDocumentView->getRect().mLeft, mDocumentView->getRect().mBottom);
	}

	if (mBGVisible)
	{
		F32 alpha = getCurrentTransparency();
		// clip background rect against extents, if we support scrolling
		LLRect bg_rect = mVisibleTextRect;
		if (mScroller)
		{
			bg_rect.intersectWith(text_rect);
		}
		LLColor4 bg_color = mReadOnly 
							? mReadOnlyBgColor.get()
							: hasFocus() 
								? mFocusBgColor.get() 
								: mWriteableBgColor.get();
		gl_rect_2d(text_rect, bg_color % alpha, TRUE);
	}

	bool should_clip = mClip || mScroller != NULL;
	{ LLLocalClipRect clip(text_rect, should_clip);
 
		// draw document view
		if (mScroller)
		{
			drawChild(mScroller);
		}
		else
		{
			drawChild(mDocumentView);
		}
 
		drawSelectionBackground();
		drawText();
		drawCursor();
	}
 
	mDocumentView->setVisible(FALSE);
	LLUICtrl::draw();
	mDocumentView->setVisible(TRUE);
}


//virtual
void LLTextBase::setColor( const LLColor4& c )
{
	mFgColor = c;
	mStyleDirty = true;
}

//virtual 
void LLTextBase::setReadOnlyColor(const LLColor4 &c)
{
	mReadOnlyFgColor = c;
	mStyleDirty = true;
}

//virtual
void LLTextBase::handleVisibilityChange( BOOL new_visibility )
{
	if(!new_visibility && mPopupMenu)
	{
		mPopupMenu->hide();
	}
	LLUICtrl::handleVisibilityChange(new_visibility);
}

//virtual
void LLTextBase::setValue(const LLSD& value )
{
	setText(value.asString());
}

//virtual
BOOL LLTextBase::canDeselect() const 
{ 
	return hasSelection(); 
}


//virtual
void LLTextBase::deselect()
{
	mSelectionStart = 0;
	mSelectionEnd = 0;
	mIsSelecting = FALSE;
}

bool LLTextBase::getSpellCheck() const
{
	return (LLSpellChecker::getUseSpellCheck()) && (!mReadOnly) && (mSpellCheck);
}

const std::string& LLTextBase::getSuggestion(U32 index) const
{
	return (index < mSuggestionList.size()) ? mSuggestionList[index] : LLStringUtil::null;
}

U32 LLTextBase::getSuggestionCount() const
{
	return mSuggestionList.size();
}

void LLTextBase::replaceWithSuggestion(U32 index)
{
	for (std::list<std::pair<U32, U32> >::const_iterator it = mMisspellRanges.begin(); it != mMisspellRanges.end(); ++it)
	{
		if ( (it->first <= (U32)mCursorPos) && (it->second >= (U32)mCursorPos) )
		{
			deselect();

			// Delete the misspelled word
			removeStringNoUndo(it->first, it->second - it->first);

			// Insert the suggestion in its place
			LLWString suggestion = utf8str_to_wstring(mSuggestionList[index]);
			insertStringNoUndo(it->first, utf8str_to_wstring(mSuggestionList[index]));
			setCursorPos(it->first + (S32)suggestion.length());

			break;
		}
	}
	mSpellCheckStart = mSpellCheckEnd = -1;
}

void LLTextBase::addToDictionary()
{
	if (canAddToDictionary())
	{
		LLSpellChecker::instance().addToCustomDictionary(getMisspelledWord(mCursorPos));
	}
}

bool LLTextBase::canAddToDictionary() const
{
	return (getSpellCheck()) && (isMisspelledWord(mCursorPos));
}

void LLTextBase::addToIgnore()
{
	if (canAddToIgnore())
	{
		LLSpellChecker::instance().addToIgnoreList(getMisspelledWord(mCursorPos));
	}
}

bool LLTextBase::canAddToIgnore() const
{
	return (getSpellCheck()) && (isMisspelledWord(mCursorPos));
}

std::string LLTextBase::getMisspelledWord(U32 pos) const
{
	for (std::list<std::pair<U32, U32> >::const_iterator it = mMisspellRanges.begin(); it != mMisspellRanges.end(); ++it)
	{
		if ( (it->first <= pos) && (it->second >= pos) )
		{
			return wstring_to_utf8str(getWText().substr(it->first, it->second - it->first));
		}
	}
	return LLStringUtil::null;
}

bool LLTextBase::isMisspelledWord(U32 pos) const
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

void LLTextBase::onSpellCheckSettingsChange()
{
	// Recheck the spelling on every change
	mMisspellRanges.clear();
	mSpellCheckStart = mSpellCheckEnd = -1;
}

// Sets the scrollbar from the cursor position
void LLTextBase::updateScrollFromCursor()
{
	// Update scroll position even in read-only mode (when there's no cursor displayed)
	// because startOfDoc()/endOfDoc() modify cursor position. See EXT-736.

	if (!mScrollNeeded || !mScroller)
	{
		return;
	}
	mScrollNeeded = FALSE; 

	// scroll so that the cursor is at the top of the page
	LLRect scroller_doc_window = getVisibleDocumentRect();
	LLRect cursor_rect_doc = getDocRectFromDocIndex(mCursorPos);
	mScroller->scrollToShowRect(cursor_rect_doc, LLRect(0, scroller_doc_window.getHeight() - 5, scroller_doc_window.getWidth(), 5));
}

S32 LLTextBase::getLeftOffset(S32 width)
{
	switch (mHAlign)
	{
	case LLFontGL::LEFT:
		return mHPad;
	case LLFontGL::HCENTER:
		return mHPad + llmax(0, (mVisibleTextRect.getWidth() - width - mHPad) / 2);
	case LLFontGL::RIGHT:
		return mVisibleTextRect.getWidth() - width;
	default:
		return mHPad;
	}
}


static LLFastTimer::DeclareTimer FTM_TEXT_REFLOW ("Text Reflow");
void LLTextBase::reflow()
{
	LLFastTimer ft(FTM_TEXT_REFLOW);

	updateSegments();

	if (mReflowIndex == S32_MAX)
	{
		return;
	}

	bool scrolled_to_bottom = mScroller ? mScroller->isAtBottom() : false;

	LLRect cursor_rect = getLocalRectFromDocIndex(mCursorPos);
	bool follow_selection = getLocalRect().overlaps(cursor_rect); // cursor is (potentially) visible

	// store in top-left relative coordinates to avoid issues with horizontal scrollbar appearing and disappearing
	cursor_rect.mTop = mVisibleTextRect.mTop - cursor_rect.mTop;
	cursor_rect.mBottom = mVisibleTextRect.mTop - cursor_rect.mBottom;

	S32 first_line = getFirstVisibleLine();

	// if scroll anchor not on first line, update it to first character of first line
	if ((first_line < mLineInfoList.size())
		&&	(mScrollIndex <  mLineInfoList[first_line].mDocIndexStart
			||	mScrollIndex >= mLineInfoList[first_line].mDocIndexEnd))
	{
		mScrollIndex = mLineInfoList[first_line].mDocIndexStart;
	}
	LLRect first_char_rect = getLocalRectFromDocIndex(mScrollIndex);
	// store in top-left relative coordinates to avoid issues with horizontal scrollbar appearing and disappearing
	first_char_rect.mTop = mVisibleTextRect.mTop - first_char_rect.mTop;
	first_char_rect.mBottom = mVisibleTextRect.mTop - first_char_rect.mBottom;

	S32 reflow_count = 0;
	while(mReflowIndex < S32_MAX)
	{
		// we can get into an infinite loop if the document height does not monotonically increase
		// with decreasing width (embedded ui elements with alternate layouts).  In that case, 
		// we want to stop reflowing after 2 iterations.  We use 2, since we need to handle the case
		// of introducing a vertical scrollbar causing a reflow with less width.  We should also always
		// use an even number of iterations to avoid user visible oscillation of the layout
		if(++reflow_count > 2)
		{
			lldebugs << "Breaking out of reflow due to possible infinite loop in " << getName() << llendl;
			break;
		}
	
		S32 start_index = mReflowIndex;
		mReflowIndex = S32_MAX;

		// shrink document to minimum size (visible portion of text widget)
		// to force inlined widgets with follows set to shrink
		if (mWordWrap)
		{
			mDocumentView->reshape(mVisibleTextRect.getWidth(), mDocumentView->getRect().getHeight());
		}

		S32 cur_top = 0;

		segment_set_t::iterator seg_iter = mSegments.begin();
		S32 seg_offset = 0;
		S32 line_start_index = 0;
		const S32 text_available_width = mVisibleTextRect.getWidth() - mHPad;  // reserve room for margin
		S32 remaining_pixels = text_available_width;
		S32 line_count = 0;

		// find and erase line info structs starting at start_index and going to end of document
		if (!mLineInfoList.empty())
		{
			// find first element whose end comes after start_index
			line_list_t::iterator iter = std::upper_bound(mLineInfoList.begin(), mLineInfoList.end(), start_index, line_end_compare());
			line_start_index = iter->mDocIndexStart;
			line_count = iter->mLineNum;
			cur_top = iter->mRect.mTop;
			getSegmentAndOffset(iter->mDocIndexStart, &seg_iter, &seg_offset);
			mLineInfoList.erase(iter, mLineInfoList.end());
		}

		S32 line_height = 0;

		while(seg_iter != mSegments.end())
		{
			LLTextSegmentPtr segment = *seg_iter;

			// track maximum height of any segment on this line
			S32 cur_index = segment->getStart() + seg_offset;

			// ask segment how many character fit in remaining space
			S32 character_count = segment->getNumChars(getWordWrap() ? llmax(0, remaining_pixels) : S32_MAX,
														seg_offset, 
														cur_index - line_start_index, 
														S32_MAX);

			S32 segment_width, segment_height;
			bool force_newline = segment->getDimensions(seg_offset, character_count, segment_width, segment_height);
			// grow line height as necessary based on reported height of this segment
			line_height = llmax(line_height, segment_height);
			remaining_pixels -= segment_width;

			seg_offset += character_count;

			S32 last_segment_char_on_line = segment->getStart() + seg_offset;

			S32 text_actual_width = text_available_width - remaining_pixels;
			S32 text_left = getLeftOffset(text_actual_width);
			LLRect line_rect(text_left, 
							cur_top, 
							text_left + text_actual_width, 
							cur_top - line_height);

			// if we didn't finish the current segment...
			if (last_segment_char_on_line < segment->getEnd())
			{
				// add line info and keep going
				mLineInfoList.push_back(line_info(
											line_start_index, 
											last_segment_char_on_line, 
											line_rect, 
											line_count));

				line_start_index = segment->getStart() + seg_offset;
				cur_top -= llround((F32)line_height * mLineSpacingMult) + mLineSpacingPixels;
				remaining_pixels = text_available_width;
				line_height = 0;
			}
			// ...just consumed last segment..
			else if (++segment_set_t::iterator(seg_iter) == mSegments.end())
			{
				mLineInfoList.push_back(line_info(
											line_start_index, 
											last_segment_char_on_line, 
											line_rect, 
											line_count));
				cur_top -= llround((F32)line_height * mLineSpacingMult) + mLineSpacingPixels;
				break;
			}
			// ...or finished a segment and there are segments remaining on this line
			else
			{
				// subtract pixels used and increment segment
				if (force_newline)
				{
					mLineInfoList.push_back(line_info(
												line_start_index, 
												last_segment_char_on_line, 
												line_rect, 
												line_count));
					line_start_index = segment->getStart() + seg_offset;
					cur_top -= llround((F32)line_height * mLineSpacingMult) + mLineSpacingPixels;
					line_height = 0;
					remaining_pixels = text_available_width;
				}
				++seg_iter;
				seg_offset = 0;
			}
			if (force_newline) 
			{
				line_count++;
			}
		}

		// calculate visible region for diplaying text
		updateRects();

		for (segment_set_t::iterator segment_it = mSegments.begin();
			segment_it != mSegments.end();
			++segment_it)
		{
			LLTextSegmentPtr segmentp = *segment_it;
			segmentp->updateLayout(*this);

		}
	}

	// apply scroll constraints after reflowing text
	if (!hasMouseCapture() && mScroller)
	{
		if (scrolled_to_bottom && mTrackEnd)
		{
			// keep bottom of text buffer visible
			endOfDoc();
		}
		else if (hasSelection() && follow_selection)
		{
			// keep cursor in same vertical position on screen when selecting text
			LLRect new_cursor_rect_doc = getDocRectFromDocIndex(mCursorPos);
			LLRect old_cursor_rect = cursor_rect;
			old_cursor_rect.mTop = mVisibleTextRect.mTop - cursor_rect.mTop;
			old_cursor_rect.mBottom = mVisibleTextRect.mTop - cursor_rect.mBottom;

			mScroller->scrollToShowRect(new_cursor_rect_doc, old_cursor_rect);
		}
		else
		{
			// keep first line of text visible
			LLRect new_first_char_rect = getDocRectFromDocIndex(mScrollIndex);

			// pass in desired rect in the coordinate frame of the document viewport
			LLRect old_first_char_rect = first_char_rect;
			old_first_char_rect.mTop = mVisibleTextRect.mTop - first_char_rect.mTop;
			old_first_char_rect.mBottom = mVisibleTextRect.mTop - first_char_rect.mBottom;

			mScroller->scrollToShowRect(new_first_char_rect, old_first_char_rect);
		}
	}

	// reset desired x cursor position
	updateCursorXPos();
}

LLRect LLTextBase::getTextBoundingRect()
{
	reflow();
	return mTextBoundingRect;
}


void LLTextBase::clearSegments()
{
	mSegments.clear();
	createDefaultSegment();
}

S32 LLTextBase::getLineStart( S32 line ) const
{
	S32 num_lines = getLineCount();
	if (num_lines == 0)
	{
		return 0;
	}

	line = llclamp(line, 0, num_lines-1);
	return mLineInfoList[line].mDocIndexStart;
}

S32 LLTextBase::getLineEnd( S32 line ) const
{
	S32 num_lines = getLineCount();
	if (num_lines == 0)
	{
		return 0;
	}

	line = llclamp(line, 0, num_lines-1);
	return mLineInfoList[line].mDocIndexEnd;
}



S32 LLTextBase::getLineNumFromDocIndex( S32 doc_index, bool include_wordwrap) const
{
	if (mLineInfoList.empty())
	{
		return 0;
	}
	else
	{
		line_list_t::const_iterator iter = std::upper_bound(mLineInfoList.begin(), mLineInfoList.end(), doc_index, line_end_compare());
		if (include_wordwrap)
		{
			return iter - mLineInfoList.begin();
		}
		else
		{
			if (iter == mLineInfoList.end())
			{
				return mLineInfoList.back().mLineNum;
			}
			else
			{
				return iter->mLineNum;
			}
		}
	}
}

// Given an offset into text (pos), find the corresponding line (from the start of the doc) and an offset into the line.
S32 LLTextBase::getLineOffsetFromDocIndex( S32 startpos, bool include_wordwrap) const
{
	if (mLineInfoList.empty())
	{
		return startpos;
	}
	else
	{
		line_list_t::const_iterator iter = std::upper_bound(mLineInfoList.begin(), mLineInfoList.end(), startpos, line_end_compare());
		return startpos - iter->mDocIndexStart;
	}
}

S32	LLTextBase::getFirstVisibleLine() const
{
	LLRect visible_region = getVisibleDocumentRect();

	// binary search for line that starts before top of visible buffer
	line_list_t::const_iterator iter = std::lower_bound(mLineInfoList.begin(), mLineInfoList.end(), visible_region.mTop, compare_bottom());

	return iter - mLineInfoList.begin();
}

std::pair<S32, S32>	LLTextBase::getVisibleLines(bool require_fully_visible) 
{
	LLRect visible_region = getVisibleDocumentRect();
	line_list_t::const_iterator first_iter;
	line_list_t::const_iterator last_iter;

	// make sure we have an up-to-date mLineInfoList
	reflow();

	if (require_fully_visible)
	{
		first_iter = std::lower_bound(mLineInfoList.begin(), mLineInfoList.end(), visible_region.mTop, compare_top());
		last_iter = std::upper_bound(mLineInfoList.begin(), mLineInfoList.end(), visible_region.mBottom, compare_bottom());
	}
	else
	{
		first_iter = std::upper_bound(mLineInfoList.begin(), mLineInfoList.end(), visible_region.mTop, compare_bottom());
		last_iter = std::lower_bound(mLineInfoList.begin(), mLineInfoList.end(), visible_region.mBottom, compare_top());
	}
	return std::pair<S32, S32>(first_iter - mLineInfoList.begin(), last_iter - mLineInfoList.begin());
}



LLTextViewModel* LLTextBase::getViewModel() const
{
	return (LLTextViewModel*)mViewModel.get();
}

void LLTextBase::addDocumentChild(LLView* view) 
{ 
	mDocumentView->addChild(view); 
}

void LLTextBase::removeDocumentChild(LLView* view) 
{ 
	mDocumentView->removeChild(view); 
}


static LLFastTimer::DeclareTimer FTM_UPDATE_TEXT_SEGMENTS("Update Text Segments");
void LLTextBase::updateSegments()
{
	LLFastTimer ft(FTM_UPDATE_TEXT_SEGMENTS);
	createDefaultSegment();
}

void LLTextBase::getSegmentAndOffset( S32 startpos, segment_set_t::const_iterator* seg_iter, S32* offsetp ) const
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

void LLTextBase::getSegmentAndOffset( S32 startpos, segment_set_t::iterator* seg_iter, S32* offsetp )
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

LLTextBase::segment_set_t::iterator LLTextBase::getEditableSegIterContaining(S32 index)
{
	segment_set_t::iterator it = getSegIterContaining(index);
	segment_set_t::iterator orig_it = it;

	if (it == mSegments.end()) return it;

	if (!(*it)->canEdit() 
		&& index == (*it)->getStart() 
		&& it != mSegments.begin())
	{
		it--;
		if ((*it)->canEdit())
		{
			return it;
		}
	}
	return orig_it;
}

LLTextBase::segment_set_t::const_iterator LLTextBase::getEditableSegIterContaining(S32 index) const
{
	segment_set_t::const_iterator it = getSegIterContaining(index);
	segment_set_t::const_iterator orig_it = it;
	if (it == mSegments.end()) return it;

	if (!(*it)->canEdit() 
		&& index == (*it)->getStart() 
		&& it != mSegments.begin())
	{
		it--;
		if ((*it)->canEdit())
		{
			return it;
		}
	}
	return orig_it;
}

LLTextBase::segment_set_t::iterator LLTextBase::getSegIterContaining(S32 index)
{

	static LLPointer<LLIndexSegment> index_segment = new LLIndexSegment();

	if (index > getLength()) { return mSegments.end(); }

	// when there are no segments, we return the end iterator, which must be checked by caller
	if (mSegments.size() <= 1) { return mSegments.begin(); }

	//FIXME: avoid operator new somehow (without running into refcount problems)
	index_segment->setStart(index);
	index_segment->setEnd(index);
	segment_set_t::iterator it = mSegments.upper_bound(index_segment);
	return it;
}

LLTextBase::segment_set_t::const_iterator LLTextBase::getSegIterContaining(S32 index) const
{
	static LLPointer<LLIndexSegment> index_segment = new LLIndexSegment();

	if (index > getLength()) { return mSegments.end(); }

	// when there are no segments, we return the end iterator, which must be checked by caller
	if (mSegments.size() <= 1) { return mSegments.begin(); }

	index_segment->setStart(index);
	index_segment->setEnd(index);
	LLTextBase::segment_set_t::const_iterator it =  mSegments.upper_bound(index_segment);
	return it;
}

// Finds the text segment (if any) at the give local screen position
LLTextSegmentPtr LLTextBase::getSegmentAtLocalPos( S32 x, S32 y, bool hit_past_end_of_line)
{
	// Find the cursor position at the requested local screen position
	S32 offset = getDocIndexFromLocalCoord( x, y, FALSE, hit_past_end_of_line);
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

void LLTextBase::createUrlContextMenu(S32 x, S32 y, const std::string &in_url)
{
	// work out the XUI menu file to use for this url
	LLUrlMatch match;
	std::string url = in_url;
	if (! LLUrlRegistry::instance().findUrl(url, match))
	{
		return;
	}
	
	std::string xui_file = match.getMenuName();
	if (xui_file.empty())
	{
		return;
	}

	// set up the callbacks for all of the potential menu items, N.B. we
	// don't use const ref strings in callbacks in case url goes out of scope
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	registrar.add("Url.Open", boost::bind(&LLUrlAction::openURL, url));
	registrar.add("Url.OpenInternal", boost::bind(&LLUrlAction::openURLInternal, url));
	registrar.add("Url.OpenExternal", boost::bind(&LLUrlAction::openURLExternal, url));
	registrar.add("Url.Execute", boost::bind(&LLUrlAction::executeSLURL, url));
	registrar.add("Url.Teleport", boost::bind(&LLUrlAction::teleportToLocation, url));
	registrar.add("Url.ShowProfile", boost::bind(&LLUrlAction::showProfile, url));
	registrar.add("Url.ShowOnMap", boost::bind(&LLUrlAction::showLocationOnMap, url));
	registrar.add("Url.CopyLabel", boost::bind(&LLUrlAction::copyLabelToClipboard, url));
	registrar.add("Url.CopyUrl", boost::bind(&LLUrlAction::copyURLToClipboard, url));

	// create and return the context menu from the XUI file
	delete mPopupMenu;
	mPopupMenu = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>(xui_file, LLMenuGL::sMenuContainer,
																		 LLMenuHolderGL::child_registry_t::instance());	
	if (mPopupMenu)
	{
		mPopupMenu->show(x, y);
		LLMenuGL::showPopup(this, mPopupMenu, x, y);
	}
}

void LLTextBase::setText(const LLStringExplicit &utf8str, const LLStyle::Params& input_params)
{
	// clear out the existing text and segments
	getViewModel()->setDisplay(LLWStringUtil::null);

	clearSegments();
//	createDefaultSegment();

	deselect();

	// append the new text (supports Url linking)
	std::string text(utf8str);
	LLStringUtil::removeCRLF(text);

	// appendText modifies mCursorPos...
	appendText(text, false, input_params);
	// ...so move cursor to top after appending text
	if (!mTrackEnd)
	{
		startOfDoc();
	}

	onValueChange(0, getLength());
}

//virtual
std::string LLTextBase::getText() const
{
	return getViewModel()->getValue().asString();
}

// IDEVO - icons can be UI image names or UUID sent from
// server with avatar display name
static LLUIImagePtr image_from_icon_name(const std::string& icon_name)
{
	if (LLUUID::validate(icon_name))
	{
		return LLUI::getUIImageByID( LLUUID(icon_name) );
	}
	else
	{
		return LLUI::getUIImage(icon_name);
	}
}

static LLFastTimer::DeclareTimer FTM_PARSE_HTML("Parse HTML");

void LLTextBase::appendTextImpl(const std::string &new_text, const LLStyle::Params& input_params)
{
	LLStyle::Params style_params(input_params);
	style_params.fillFrom(getDefaultStyleParams());

	S32 part = (S32)LLTextParser::WHOLE;
	if (mParseHTML && !style_params.is_link) // Don't search for URLs inside a link segment (STORM-358).
	{
		LLFastTimer _(FTM_PARSE_HTML);
		S32 start=0,end=0;
		LLUrlMatch match;
		std::string text = new_text;
		while ( LLUrlRegistry::instance().findUrl(text, match,
				boost::bind(&LLTextBase::replaceUrl, this, _1, _2, _3)) )
		{
			start = match.getStart();
			end = match.getEnd()+1;

			LLStyle::Params link_params(style_params);
			link_params.overwriteFrom(match.getStyle());

			// output the text before the Url
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
				appendAndHighlightText(subtext, part, style_params); 
			}
			// output the styled Url
			appendAndHighlightTextImpl(match.getLabel(), part, link_params, match.underlineOnHoverOnly());
			
			// set the tooltip for the Url label
			if (! match.getTooltip().empty())
			{
				segment_set_t::iterator it = getSegIterContaining(getLength()-1);
				if (it != mSegments.end())
					{
						LLTextSegmentPtr segment = *it;
						segment->setToolTip(match.getTooltip());
					}
			}

			LLTextUtil::processUrlMatch(&match,this);

			// move on to the rest of the text after the Url
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
		if (part != (S32)LLTextParser::WHOLE) 
			part=(S32)LLTextParser::END;
		if (end < (S32)text.length()) 
			appendAndHighlightText(text, part, style_params);		
	}
	else
	{
		appendAndHighlightText(new_text, part, style_params);
	}
}

static LLFastTimer::DeclareTimer FTM_APPEND_TEXT("Append Text");

void LLTextBase::appendText(const std::string &new_text, bool prepend_newline, const LLStyle::Params& input_params)
{
	LLFastTimer _(FTM_APPEND_TEXT);
	if (new_text.empty()) 
		return;

	if(prepend_newline)
		appendLineBreakSegment(input_params);
	appendTextImpl(new_text,input_params);
}

void LLTextBase::needsReflow(S32 index)
{
	lldebugs << "reflow on object " << (void*)this << " index = " << mReflowIndex << ", new index = " << index << llendl;
	mReflowIndex = llmin(mReflowIndex, index);
}

void LLTextBase::appendLineBreakSegment(const LLStyle::Params& style_params)
{
	segment_vec_t segments;
	LLStyleConstSP sp(new LLStyle(style_params));
	segments.push_back(new LLLineBreakTextSegment(sp, getLength()));

	insertStringNoUndo(getLength(), utf8str_to_wstring("\n"), &segments);
}

void LLTextBase::appendImageSegment(const LLStyle::Params& style_params)
{
	if(getPlainText())
	{
		return;
	}
	segment_vec_t segments;
	LLStyleConstSP sp(new LLStyle(style_params));
	segments.push_back(new LLImageTextSegment(sp, getLength(),*this));

	insertStringNoUndo(getLength(), utf8str_to_wstring(" "), &segments);
}

void LLTextBase::appendWidget(const LLInlineViewSegment::Params& params, const std::string& text, bool allow_undo)
{
	segment_vec_t segments;
	LLWString widget_wide_text = utf8str_to_wstring(text);
	segments.push_back(new LLInlineViewSegment(params, getLength(), getLength() + widget_wide_text.size()));

	insertStringNoUndo(getLength(), widget_wide_text, &segments);
}

void LLTextBase::appendAndHighlightTextImpl(const std::string &new_text, S32 highlight_part, const LLStyle::Params& style_params, bool underline_on_hover_only)
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

	if (mParseHighlights)
	{
		LLStyle::Params highlight_params(style_params);

		LLSD pieces = LLTextParser::instance().parsePartialLineHighlights(new_text, highlight_params.color(), (LLTextParser::EHighlightPosition)highlight_part);
		for (S32 i = 0; i < pieces.size(); i++)
		{
			LLSD color_llsd = pieces[i]["color"];
			LLColor4 lcolor;
			lcolor.setValue(color_llsd);
			highlight_params.color = lcolor;

			LLWString wide_text;
			wide_text = utf8str_to_wstring(pieces[i]["text"].asString());

			S32 cur_length = getLength();
			LLStyleConstSP sp(new LLStyle(highlight_params));
			LLTextSegmentPtr segmentp;
			if(underline_on_hover_only)
			{
				highlight_params.font.style("NORMAL");
				LLStyleConstSP normal_sp(new LLStyle(highlight_params));
				segmentp = new LLOnHoverChangeableTextSegment(sp, normal_sp, cur_length, cur_length + wide_text.size(), *this);
			}
			else
			{
				segmentp = new LLNormalTextSegment(sp, cur_length, cur_length + wide_text.size(), *this);
			}
			segment_vec_t segments;
			segments.push_back(segmentp);
			insertStringNoUndo(cur_length, wide_text, &segments);
		}
	}
	else
	{
		LLWString wide_text;
		wide_text = utf8str_to_wstring(new_text);

		segment_vec_t segments;
		S32 segment_start = old_length;
		S32 segment_end = old_length + wide_text.size();
		LLStyleConstSP sp(new LLStyle(style_params));
		if (underline_on_hover_only)
		{
			LLStyle::Params normal_style_params(style_params);
			normal_style_params.font.style("NORMAL");
			LLStyleConstSP normal_sp(new LLStyle(normal_style_params));
			segments.push_back(new LLOnHoverChangeableTextSegment(sp, normal_sp, segment_start, segment_end, *this ));
		}
		else
		{
		segments.push_back(new LLNormalTextSegment(sp, segment_start, segment_end, *this ));
		}

		insertStringNoUndo(getLength(), wide_text, &segments);
	}

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
}

void LLTextBase::appendAndHighlightText(const std::string &new_text, S32 highlight_part, const LLStyle::Params& style_params, bool underline_on_hover_only)
{
	if (new_text.empty()) return; 

	std::string::size_type start = 0;
	std::string::size_type pos = new_text.find("\n",start);
	
	while(pos!=-1)
	{
		if(pos!=start)
		{
			std::string str = std::string(new_text,start,pos-start);
			appendAndHighlightTextImpl(str,highlight_part, style_params, underline_on_hover_only);
		}
		appendLineBreakSegment(style_params);
		start = pos+1;
		pos = new_text.find("\n",start);
	}

	std::string str = std::string(new_text,start,new_text.length()-start);
	appendAndHighlightTextImpl(str,highlight_part, style_params, underline_on_hover_only);
}


void LLTextBase::replaceUrl(const std::string &url,
							const std::string &label,
							const std::string &icon)
{
	// get the full (wide) text for the editor so we can change it
	LLWString text = getWText();
	LLWString wlabel = utf8str_to_wstring(label);
	bool modified = false;
	S32 seg_start = 0;

	// iterate through each segment looking for ones styled as links
	segment_set_t::iterator it;
	for (it = mSegments.begin(); it != mSegments.end(); ++it)
	{
		LLTextSegment *seg = *it;
		LLStyleConstSP style = seg->getStyle();

		// update segment start/end length in case we replaced text earlier
		S32 seg_length = seg->getEnd() - seg->getStart();
		seg->setStart(seg_start);
		seg->setEnd(seg_start + seg_length);

		// if we find a link with our Url, then replace the label
		if (style->getLinkHREF() == url)
		{
			S32 start = seg->getStart();
			S32 end = seg->getEnd();
			text = text.substr(0, start) + wlabel + text.substr(end, text.size() - end + 1);
			seg->setEnd(start + wlabel.size());
			modified = true;
		}

		// Icon might be updated when more avatar or group info
		// becomes available
		if (style->isImage() && style->getLinkHREF() == url)
		{
			LLUIImagePtr image = image_from_icon_name( icon );
			if (image)
			{
				LLStyle::Params icon_params;
				icon_params.image = image;
				LLStyleConstSP new_style(new LLStyle(icon_params));
				seg->setStyle(new_style);
				modified = true;
			}
		}

		// work out the character offset for the next segment
		seg_start = seg->getEnd();
	}

	// update the editor with the new (wide) text string
	if (modified)
	{
		getViewModel()->setDisplay(text);
		deselect();
		setCursorPos(mCursorPos);
		needsReflow();
	}
}


void LLTextBase::setWText(const LLWString& text)
{
	setText(wstring_to_utf8str(text));
}

const LLWString& LLTextBase::getWText() const
{
	return getViewModel()->getDisplay();
}

// If round is true, if the position is on the right half of a character, the cursor
// will be put to its right.  If round is false, the cursor will always be put to the
// character's left.

S32 LLTextBase::getDocIndexFromLocalCoord( S32 local_x, S32 local_y, BOOL round, bool hit_past_end_of_line) const
{
	// Figure out which line we're nearest to.
	LLRect doc_rect = mDocumentView->getRect();

	S32 doc_y = local_y - doc_rect.mBottom;
	
	// binary search for line that starts before local_y
	line_list_t::const_iterator line_iter = std::lower_bound(mLineInfoList.begin(), mLineInfoList.end(), doc_y, compare_bottom());

	if (line_iter == mLineInfoList.end())
	{
		return getLength(); // past the end
	}
	
	S32 pos = getLength();
	S32 start_x = line_iter->mRect.mLeft + doc_rect.mLeft;

	segment_set_t::iterator line_seg_iter;
	S32 line_seg_offset;
	for(getSegmentAndOffset(line_iter->mDocIndexStart, &line_seg_iter, &line_seg_offset);
		line_seg_iter != mSegments.end(); 
		++line_seg_iter, line_seg_offset = 0)
	{
		const LLTextSegmentPtr segmentp = *line_seg_iter;

		S32 segment_line_start = segmentp->getStart() + line_seg_offset;
		S32 segment_line_length = llmin(segmentp->getEnd(), line_iter->mDocIndexEnd) - segment_line_start;
		S32 text_width, text_height;
		bool newline = segmentp->getDimensions(line_seg_offset, segment_line_length, text_width, text_height);

		if(newline)
		{
			pos = segment_line_start + segmentp->getOffset(local_x - start_x, line_seg_offset, segment_line_length, round);
			break;
		}

		// if we've reached a line of text *below* the mouse cursor, doc index is first character on that line
		if (hit_past_end_of_line && doc_y > line_iter->mRect.mTop)
		{
			pos = segment_line_start;
			break;
		}
		if (local_x < start_x + text_width)			// cursor to left of right edge of text
		{
			// Figure out which character we're nearest to.
			S32 offset;
			if (!segmentp->canEdit())
			{
				S32 segment_width, segment_height;
				segmentp->getDimensions(0, segmentp->getEnd() - segmentp->getStart(), segment_width, segment_height);
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
		else if (hit_past_end_of_line && segmentp->getEnd() >= line_iter->mDocIndexEnd)
		{
			if (getLineNumFromDocIndex(line_iter->mDocIndexEnd - 1) == line_iter->mLineNum)
			{
				// if segment wraps to the next line we should step one char back
				// to compensate for the space char between words
				// which is removed due to wrapping
				pos = llclamp(line_iter->mDocIndexEnd - 1, 0, getLength());
			}
			else
			{
				pos = llclamp(line_iter->mDocIndexEnd, 0, getLength());
			}
			break;
		}
		start_x += text_width;
	}

	return pos;
}

// returns rectangle of insertion caret 
// in document coordinate frame from given index into text
LLRect LLTextBase::getDocRectFromDocIndex(S32 pos) const
{
	if (mLineInfoList.empty()) 
	{ 
		return LLRect();
	}

	LLRect doc_rect;

	// clamp pos to valid values
	pos = llclamp(pos, 0, mLineInfoList.back().mDocIndexEnd - 1);

	// find line that contains cursor
	line_list_t::const_iterator line_iter = std::upper_bound(mLineInfoList.begin(), mLineInfoList.end(), pos, line_end_compare());

	doc_rect.mLeft = line_iter->mRect.mLeft; 
	doc_rect.mBottom = line_iter->mRect.mBottom;
	doc_rect.mTop = line_iter->mRect.mTop;

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
			S32 segment_width, segment_height;
			segmentp->getDimensions(line_seg_offset, cursor_seg_offset - line_seg_offset, segment_width, segment_height);
			doc_rect.mLeft += segment_width;

			break;
		}
		else
		{
			// add remainder of current text segment to cursor position
			S32 segment_width, segment_height;
			segmentp->getDimensions(line_seg_offset, (segmentp->getEnd() - segmentp->getStart()) - line_seg_offset, segment_width, segment_height);
			doc_rect.mLeft += segment_width;
			// offset will be 0 for all segments after the first
			line_seg_offset = 0;
			// go to next text segment on this line
			++line_seg_iter;
		}
	}

	// set rect to 0 width
	doc_rect.mRight = doc_rect.mLeft; 

	return doc_rect;
}

LLRect LLTextBase::getLocalRectFromDocIndex(S32 pos) const
{
	LLRect content_window_rect = mScroller ? mScroller->getContentWindowRect() : getLocalRect();
	if (mBorderVisible)
	{
		content_window_rect.stretch(-1);
	}

	LLRect local_rect;

	if (mLineInfoList.empty()) 
	{ 
		// return default height rect in upper left
		local_rect = content_window_rect;
		local_rect.mBottom = local_rect.mTop - mDefaultFont->getLineHeight();
		return local_rect;
	}

	// get the rect in document coordinates
	LLRect doc_rect = getDocRectFromDocIndex(pos);

	// compensate for scrolled, inset view of doc
	LLRect scrolled_view_rect = getVisibleDocumentRect();
	local_rect = doc_rect;
	local_rect.translate(content_window_rect.mLeft - scrolled_view_rect.mLeft, 
						content_window_rect.mBottom - scrolled_view_rect.mBottom);

	return local_rect;
}

void LLTextBase::updateCursorXPos()
{
	// reset desired x cursor position
	mDesiredXPixel = getLocalRectFromDocIndex(mCursorPos).mLeft;
}


void LLTextBase::startOfLine()
{
	S32 offset = getLineOffsetFromDocIndex(mCursorPos);
	setCursorPos(mCursorPos - offset);
}

void LLTextBase::endOfLine()
{
	S32 line = getLineNumFromDocIndex(mCursorPos);
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

void LLTextBase::startOfDoc()
{
	setCursorPos(0);
	if (mScroller)
	{
		mScroller->goToTop();
	}
}

void LLTextBase::endOfDoc()
{
	setCursorPos(getLength());
	if (mScroller)
	{
		mScroller->goToBottom();
	}
}

void LLTextBase::changePage( S32 delta )
{
	const S32 PIXEL_OVERLAP_ON_PAGE_CHANGE = 10;
	if (delta == 0 || !mScroller) return;

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

// Picks a new cursor position based on the screen size of text being drawn.
void LLTextBase::setCursorAtLocalPos( S32 local_x, S32 local_y, bool round, bool keep_cursor_offset )
{
	setCursorPos(getDocIndexFromLocalCoord(local_x, local_y, round), keep_cursor_offset);
}


void LLTextBase::changeLine( S32 delta )
{
	S32 line = getLineNumFromDocIndex(mCursorPos);

	S32 new_line = line;
	if( (delta < 0) && (line > 0 ) )
	{
		new_line = line - 1;
	}
	else if( (delta > 0) && (line < (getLineCount() - 1)) )
	{
		new_line = line + 1;
	}

	LLRect visible_region = getVisibleDocumentRect();

	S32 new_cursor_pos = getDocIndexFromLocalCoord(mDesiredXPixel, mLineInfoList[new_line].mRect.mBottom + mVisibleTextRect.mBottom - visible_region.mBottom, TRUE);
	setCursorPos(new_cursor_pos, true);
}

bool LLTextBase::scrolledToStart()
{
	return mScroller->isAtTop();
}

bool LLTextBase::scrolledToEnd()
{
	return mScroller->isAtBottom();
}

bool LLTextBase::setCursor(S32 row, S32 column)
{
	if (row < 0 || column < 0) return false;

	S32 n_lines = mLineInfoList.size();
	for (S32 line = row; line < n_lines; ++line)
	{
		const line_info& li = mLineInfoList[line];

		if (li.mLineNum < row)
		{
			continue;
		}
		else if (li.mLineNum > row)
		{
			break; // invalid column specified
		}

		// Found the given row.
		S32 line_length = li.mDocIndexEnd - li.mDocIndexStart;;
		if (column >= line_length)
		{
			column -= line_length;
			continue;
		}

		// Found the given column.
		updateCursorXPos();
		S32 doc_pos = li.mDocIndexStart + column;
		return setCursorPos(doc_pos);
	}

	return false; // invalid row or column specified
}


bool LLTextBase::setCursorPos(S32 cursor_pos, bool keep_cursor_offset)
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

// constraint cursor to editable segments of document
S32 LLTextBase::getEditableIndex(S32 index, bool increasing_direction)
{
	segment_set_t::iterator segment_iter;
	S32 offset;
	getSegmentAndOffset(index, &segment_iter, &offset);
	if (segment_iter == mSegments.end())
	{
		return 0;
	}

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

void LLTextBase::updateRects()
{
	LLRect old_text_rect = mVisibleTextRect;
	mVisibleTextRect = mScroller ? mScroller->getContentWindowRect() : getLocalRect();

	if (mLineInfoList.empty()) 
	{
		mTextBoundingRect = LLRect(0, mVPad, mHPad, 0);
	}
	else
	{
		mTextBoundingRect = mLineInfoList.begin()->mRect;
		for (line_list_t::const_iterator line_iter = ++mLineInfoList.begin();
			line_iter != mLineInfoList.end();
			++line_iter)
		{
			mTextBoundingRect.unionWith(line_iter->mRect);
		}

		mTextBoundingRect.mTop += mVPad;

		S32 delta_pos = 0;
		
		switch(mVAlign)
		{
		case LLFontGL::TOP:
			delta_pos = llmax(mVisibleTextRect.getHeight() - mTextBoundingRect.mTop, -mTextBoundingRect.mBottom);
			break;
		case LLFontGL::VCENTER:
			delta_pos = (llmax(mVisibleTextRect.getHeight() - mTextBoundingRect.mTop, -mTextBoundingRect.mBottom) + (mVisibleTextRect.mBottom - mTextBoundingRect.mBottom)) / 2;
			break;
		case LLFontGL::BOTTOM:
			delta_pos = mVisibleTextRect.mBottom - mTextBoundingRect.mBottom;
			break;
		case LLFontGL::BASELINE:
			// do nothing
			break;
		}
		// move line segments to fit new document rect
		for (line_list_t::iterator it = mLineInfoList.begin(); it != mLineInfoList.end(); ++it)
		{
			it->mRect.translate(0, delta_pos);
		}
		mTextBoundingRect.translate(0, delta_pos);
	}

	// update document container dimensions according to text contents
	LLRect doc_rect;
	// use old mVisibleTextRect constraint document to width of viewable region
	doc_rect.mBottom = llmin(mVisibleTextRect.mBottom,  mTextBoundingRect.mBottom);
	doc_rect.mLeft = 0;

	// allow horizontal scrolling?
	// if so, use entire width of text contents
	// otherwise, stop at width of mVisibleTextRect
	//FIXME: consider use of getWordWrap() instead
	doc_rect.mRight = mScroller 
		? llmax(mVisibleTextRect.getWidth(), mTextBoundingRect.mRight)
		: mVisibleTextRect.getWidth();
	doc_rect.mTop = llmax(mVisibleTextRect.mTop, mTextBoundingRect.mTop);

	if (!mScroller)
	{
		// push doc rect to top of text widget
		switch(mVAlign)
		{
		case LLFontGL::TOP:
			doc_rect.translate(0, mVisibleTextRect.getHeight() - doc_rect.mTop);
			break;
		case LLFontGL::VCENTER:
			doc_rect.translate(0, (mVisibleTextRect.getHeight() - doc_rect.mTop) / 2);
		case LLFontGL::BOTTOM:
		default:
			break;
		}
	}

	mDocumentView->setShape(doc_rect);

	//update mVisibleTextRect *after* mDocumentView has been resized
	// so that scrollbars are added if document needs to scroll
	// since mVisibleTextRect does not include scrollbars
	mVisibleTextRect = mScroller ? mScroller->getContentWindowRect() : getLocalRect();
	//FIXME: replace border with image?
	if (mBorderVisible)
	{
		mVisibleTextRect.stretch(-1);
	}
	if (mVisibleTextRect != old_text_rect)
	{
		needsReflow();
	}

	// update document container again, using new mVisibleTextRect (that has scrollbars enabled as needed)
	doc_rect.mBottom = llmin(mVisibleTextRect.mBottom,  mTextBoundingRect.mBottom);
	doc_rect.mLeft = 0;
	doc_rect.mRight = mScroller 
		? llmax(mVisibleTextRect.getWidth(), mTextBoundingRect.mRight)
		: mVisibleTextRect.getWidth();
	doc_rect.mTop = llmax(mVisibleTextRect.mTop, mTextBoundingRect.mTop);
	if (!mScroller)
	{
		// push doc rect to top of text widget
		switch(mVAlign)
		{
		case LLFontGL::TOP:
			doc_rect.translate(0, mVisibleTextRect.getHeight() - doc_rect.mTop);
			break;
		case LLFontGL::VCENTER:
			doc_rect.translate(0, (mVisibleTextRect.getHeight() - doc_rect.mTop) / 2);
		case LLFontGL::BOTTOM:
		default:
			break;
		}
	}
	mDocumentView->setShape(doc_rect);
}


void LLTextBase::startSelection()
{
	if( !mIsSelecting )
	{
		mIsSelecting = TRUE;
		mSelectionStart = mCursorPos;
		mSelectionEnd = mCursorPos;
	}
}

void LLTextBase::endSelection()
{
	if( mIsSelecting )
	{
		mIsSelecting = FALSE;
		mSelectionEnd = mCursorPos;
	}
}

// get portion of document that is visible in text editor
LLRect LLTextBase::getVisibleDocumentRect() const
{
	if (mScroller)
	{
		return mScroller->getVisibleContentRect();
	}
	else if (mClip)
	{
		LLRect visible_text_rect = getVisibleTextRect();
		LLRect doc_rect = mDocumentView->getRect();
		visible_text_rect.translate(-doc_rect.mLeft, -doc_rect.mBottom);

		// reject partially visible lines
		LLRect visible_lines_rect;
		for (line_list_t::const_iterator it = mLineInfoList.begin(), end_it = mLineInfoList.end();
			it != end_it;
			++it)
		{
			bool line_visible = mClipPartial ? visible_text_rect.contains(it->mRect) : visible_text_rect.overlaps(it->mRect);
			if (line_visible)
			{
				if (visible_lines_rect.isEmpty())
				{
					visible_lines_rect = it->mRect;
				}
				else
				{
					visible_lines_rect.unionWith(it->mRect);
				}
			}
		}
		return visible_lines_rect;
	}
	else
	{	// entire document rect is visible
		// but offset according to height of widget
	
		LLRect doc_rect = mDocumentView->getLocalRect();
		doc_rect.mLeft -= mDocumentView->getRect().mLeft;
		// adjust for height of text above widget baseline
		doc_rect.mBottom = doc_rect.getHeight() - mVisibleTextRect.getHeight();
		return doc_rect;
	}
}

boost::signals2::connection LLTextBase::setURLClickedCallback(const commit_signal_t::slot_type& cb)
{
	if (!mURLClickSignal)
	{
		mURLClickSignal = new commit_signal_t();
	}
	return mURLClickSignal->connect(cb);
}

//
// LLTextSegment
//

LLTextSegment::~LLTextSegment()
{}

bool LLTextSegment::getDimensions(S32 first_char, S32 num_chars, S32& width, S32& height) const { width = 0; height = 0; return false;}
S32	LLTextSegment::getOffset(S32 segment_local_x_coord, S32 start_offset, S32 num_chars, bool round) const { return 0; }
S32	LLTextSegment::getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const { return 0; }
void LLTextSegment::updateLayout(const LLTextBase& editor) {}
F32	LLTextSegment::draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect) { return draw_rect.mLeft; }
bool LLTextSegment::canEdit() const { return false; }
void LLTextSegment::unlinkFromDocument(LLTextBase*) {}
void LLTextSegment::linkToDocument(LLTextBase*) {}
const LLColor4& LLTextSegment::getColor() const { return LLColor4::white; }
//void LLTextSegment::setColor(const LLColor4 &color) {}
LLStyleConstSP LLTextSegment::getStyle() const {static LLStyleConstSP sp(new LLStyle()); return sp; }
void LLTextSegment::setStyle(LLStyleConstSP style) {}
void LLTextSegment::setToken( LLKeywordToken* token ) {}
LLKeywordToken*	LLTextSegment::getToken() const { return NULL; }
void LLTextSegment::setToolTip( const std::string &msg ) {}
void LLTextSegment::dump() const {}
BOOL LLTextSegment::handleMouseDown(S32 x, S32 y, MASK mask) { return FALSE; }
BOOL LLTextSegment::handleMouseUp(S32 x, S32 y, MASK mask) { return FALSE; }
BOOL LLTextSegment::handleMiddleMouseDown(S32 x, S32 y, MASK mask) { return FALSE; }
BOOL LLTextSegment::handleMiddleMouseUp(S32 x, S32 y, MASK mask) { return FALSE; }
BOOL LLTextSegment::handleRightMouseDown(S32 x, S32 y, MASK mask) { return FALSE; }
BOOL LLTextSegment::handleRightMouseUp(S32 x, S32 y, MASK mask) { return FALSE; }
BOOL LLTextSegment::handleDoubleClick(S32 x, S32 y, MASK mask) { return FALSE; }
BOOL LLTextSegment::handleHover(S32 x, S32 y, MASK mask) { return FALSE; }
BOOL LLTextSegment::handleScrollWheel(S32 x, S32 y, S32 clicks) { return FALSE; }
BOOL LLTextSegment::handleToolTip(S32 x, S32 y, MASK mask) { return FALSE; }
const std::string&	LLTextSegment::getName() const 
{
	return LLStringUtil::null;
}
void LLTextSegment::onMouseCaptureLost() {}
void LLTextSegment::screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const {}
void LLTextSegment::localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const {}
BOOL LLTextSegment::hasMouseCapture() { return FALSE; }

//
// LLNormalTextSegment
//

LLNormalTextSegment::LLNormalTextSegment( LLStyleConstSP style, S32 start, S32 end, LLTextBase& editor ) 
:	LLTextSegment(start, end),
	mStyle( style ),
	mToken(NULL),
	mEditor(editor)
{
	mFontHeight = mStyle->getFont()->getLineHeight();

	LLUIImagePtr image = mStyle->getImage();
	if (image.notNull())
	{
		mImageLoadedConnection = image->addLoadedCallback(boost::bind(&LLTextBase::needsReflow, &mEditor, start));
	}
}

LLNormalTextSegment::LLNormalTextSegment( const LLColor4& color, S32 start, S32 end, LLTextBase& editor, BOOL is_visible) 
:	LLTextSegment(start, end),
	mToken(NULL),
	mEditor(editor)
{
	mStyle = new LLStyle(LLStyle::Params().visible(is_visible).color(color));

	mFontHeight = mStyle->getFont()->getLineHeight();
}

LLNormalTextSegment::~LLNormalTextSegment()
{
	mImageLoadedConnection.disconnect();
}


F32 LLNormalTextSegment::draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect)
{
	if( end - start > 0 )
	{
		return drawClippedSegment( getStart() + start, getStart() + end, selection_start, selection_end, draw_rect);
	}
	return draw_rect.mLeft;
}

// Draws a single text segment, reversing the color for selection if needed.
F32 LLNormalTextSegment::drawClippedSegment(S32 seg_start, S32 seg_end, S32 selection_start, S32 selection_end, LLRect rect)
{
	F32 alpha = LLViewDrawContext::getCurrentContext().mAlpha;

	const LLWString &text = mEditor.getWText();

	F32 right_x = rect.mLeft;
	if (!mStyle->isVisible())
	{
		return right_x;
	}

	const LLFontGL* font = mStyle->getFont();

	LLColor4 color = (mEditor.getReadOnly() ? mStyle->getReadOnlyColor() : mStyle->getColor())  % alpha;

	if( selection_start > seg_start )
	{
		// Draw normally
		S32 start = seg_start;
		S32 end = llmin( selection_start, seg_end );
		S32 length =  end - start;
		font->render(text, start, 
				 rect, 
				 color, 
				 LLFontGL::LEFT, mEditor.mVAlign, 
				 LLFontGL::NORMAL, 
				 mStyle->getShadowType(), 
				 length,
				 &right_x, 
				 mEditor.getUseEllipses());
	}
	rect.mLeft = (S32)ceil(right_x);
	
	if( (selection_start < seg_end) && (selection_end > seg_start) )
	{
		// Draw reversed
		S32 start = llmax( selection_start, seg_start );
		S32 end = llmin( selection_end, seg_end );
		S32 length = end - start;

		font->render(text, start, 
				 rect,
				 mStyle->getSelectedColor().get(),
				 LLFontGL::LEFT, mEditor.mVAlign, 
				 LLFontGL::NORMAL, 
				 LLFontGL::NO_SHADOW, 
				 length,
				 &right_x, 
				 mEditor.getUseEllipses());
	}
	rect.mLeft = (S32)ceil(right_x);
	if( selection_end < seg_end )
	{
		// Draw normally
		S32 start = llmax( selection_end, seg_start );
		S32 end = seg_end;
		S32 length = end - start;
		font->render(text, start, 
				 rect, 
				 color, 
				 LLFontGL::LEFT, mEditor.mVAlign, 
				 LLFontGL::NORMAL, 
				 mStyle->getShadowType(), 
				 length,
				 &right_x, 
				 mEditor.getUseEllipses());
	}
	return right_x;
}

BOOL LLNormalTextSegment::handleHover(S32 x, S32 y, MASK mask)
{
	if (getStyle() && getStyle()->isLink())
	{
		// Only process the click if it's actually in this segment, not to the right of the end-of-line.
		if(mEditor.getSegmentAtLocalPos(x, y, false) == this)
		{
			LLUI::getWindow()->setCursor(UI_CURSOR_HAND);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL LLNormalTextSegment::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if (getStyle() && getStyle()->isLink())
	{
		// Only process the click if it's actually in this segment, not to the right of the end-of-line.
		if(mEditor.getSegmentAtLocalPos(x, y, false) == this)
		{
			mEditor.createUrlContextMenu(x, y, getStyle()->getLinkHREF());
			return TRUE;
		}
	}
	return FALSE;
}

BOOL LLNormalTextSegment::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (getStyle() && getStyle()->isLink())
	{
		// Only process the click if it's actually in this segment, not to the right of the end-of-line.
		if(mEditor.getSegmentAtLocalPos(x, y, false) == this)
		{
			// eat mouse down event on hyperlinks, so we get the mouse up
			return TRUE;
		}
	}

	return FALSE;
}

BOOL LLNormalTextSegment::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if (getStyle() && getStyle()->isLink())
	{
		// Only process the click if it's actually in this segment, not to the right of the end-of-line.
		if(mEditor.getSegmentAtLocalPos(x, y, false) == this)
		{
			LLUrlAction::clickAction(getStyle()->getLinkHREF());
			return TRUE;
		}
	}

	return FALSE;
}

BOOL LLNormalTextSegment::handleToolTip(S32 x, S32 y, MASK mask)
{
	std::string msg;
	// do we have a tooltip for a loaded keyword (for script editor)?
	if (mToken && !mToken->getToolTip().empty())
	{
		const LLWString& wmsg = mToken->getToolTip();
		LLToolTipMgr::instance().show(wstring_to_utf8str(wmsg));
		return TRUE;
	}
	// or do we have an explicitly set tooltip (e.g., for Urls)
	if (!mTooltip.empty())
	{
		LLToolTipMgr::instance().show(mTooltip);
		return TRUE;
	}

	return FALSE;
}

void LLNormalTextSegment::setToolTip(const std::string& tooltip)
{
	// we cannot replace a keyword tooltip that's loaded from a file
	if (mToken)
	{
		llwarns << "LLTextSegment::setToolTip: cannot replace keyword tooltip." << llendl;
		return;
	}
	mTooltip = tooltip;
}

bool LLNormalTextSegment::getDimensions(S32 first_char, S32 num_chars, S32& width, S32& height) const
{
	height = 0;
	width = 0;
	if (num_chars > 0)
	{
		height = mFontHeight;
		const LLWString &text = mEditor.getWText();
		// if last character is a newline, then return true, forcing line break
		width = mStyle->getFont()->getWidth(text.c_str(), mStart + first_char, num_chars);
	}
	return false;
}

S32	LLNormalTextSegment::getOffset(S32 segment_local_x_coord, S32 start_offset, S32 num_chars, bool round) const
{
	const LLWString &text = mEditor.getWText();
	return mStyle->getFont()->charFromPixelOffset(text.c_str(), mStart + start_offset,
											   (F32)segment_local_x_coord,
											   F32_MAX,
											   num_chars,
											   round);
}

S32	LLNormalTextSegment::getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const
{
	const LLWString &text = mEditor.getWText();

	LLUIImagePtr image = mStyle->getImage();
	if( image.notNull())
	{
		num_pixels = llmax(0, num_pixels - image->getWidth());
	}

	S32 last_char = mEnd;

	// set max characters to length of segment, or to first newline
	max_chars = llmin(max_chars, last_char - (mStart + segment_offset));

	// if no character yet displayed on this line, don't require word wrapping since
	// we can just move to the next line, otherwise insist on it so we make forward progress
	LLFontGL::EWordWrapStyle word_wrap_style = (line_offset == 0) 
		? LLFontGL::WORD_BOUNDARY_IF_POSSIBLE 
		: LLFontGL::ONLY_WORD_BOUNDARIES;
	S32 num_chars = mStyle->getFont()->maxDrawableChars(text.c_str() + segment_offset + mStart, 
												(F32)num_pixels,
												max_chars, 
												word_wrap_style);

	if (num_chars == 0 
		&& line_offset == 0 
		&& max_chars > 0)
	{
		// If at the beginning of a line, and a single character won't fit, draw it anyway
		num_chars = 1;
	}

	// include *either* the EOF or newline character in this run of text
	// but not both
	S32 last_char_in_run = mStart + segment_offset + num_chars;
	// check length first to avoid indexing off end of string
	if (last_char_in_run < mEnd 
		&& (last_char_in_run >= mEditor.getLength() ))
	{
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
// LLOnHoverChangeableTextSegment
//

LLOnHoverChangeableTextSegment::LLOnHoverChangeableTextSegment( LLStyleConstSP style, LLStyleConstSP normal_style, S32 start, S32 end, LLTextBase& editor ):
	  LLNormalTextSegment(normal_style, start, end, editor),
	  mHoveredStyle(style),
	  mNormalStyle(normal_style){}

/*virtual*/ 
F32 LLOnHoverChangeableTextSegment::draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect)
{
	F32 result = LLNormalTextSegment::draw(start, end, selection_start, selection_end, draw_rect);
	if (end == mEnd - mStart)
	{
		mStyle = mNormalStyle;
	}
	return result;
}

/*virtual*/
BOOL LLOnHoverChangeableTextSegment::handleHover(S32 x, S32 y, MASK mask)
{
	mStyle = mHoveredStyle;
	return LLNormalTextSegment::handleHover(x, y, mask);
}


//
// LLInlineViewSegment
//

LLInlineViewSegment::LLInlineViewSegment(const Params& p, S32 start, S32 end)
:	LLTextSegment(start, end),
	mView(p.view),
	mForceNewLine(p.force_newline),
	mLeftPad(p.left_pad),
	mRightPad(p.right_pad),
	mTopPad(p.top_pad),
	mBottomPad(p.bottom_pad)
{
} 

LLInlineViewSegment::~LLInlineViewSegment()
{
	mView->die();
}

bool LLInlineViewSegment::getDimensions(S32 first_char, S32 num_chars, S32& width, S32& height) const
{
	if (first_char == 0 && num_chars == 0) 
	{
		// we didn't fit on a line, the widget will fall on the next line
		// so dimensions here are 0
		width = 0;
		height = 0;
	}
	else
	{
		width = mLeftPad + mRightPad + mView->getRect().getWidth();
		height = mBottomPad + mTopPad + mView->getRect().getHeight();
	}

	return false;
}

S32	LLInlineViewSegment::getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const
{
	// if putting a widget anywhere but at the beginning of a line
	// and the widget doesn't fit or mForceNewLine is true
	// then return 0 chars for that line, and all characters for the next
	if (line_offset != 0 
		&& (mForceNewLine || num_pixels < mView->getRect().getWidth())) 
	{
		return 0;
	}
	else
	{
		return mEnd - mStart;
	}
}

void LLInlineViewSegment::updateLayout(const LLTextBase& editor)
{
	LLRect start_rect = editor.getDocRectFromDocIndex(mStart);
	mView->setOrigin(start_rect.mLeft + mLeftPad, start_rect.mBottom + mBottomPad);
}

F32	LLInlineViewSegment::draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect)
{
	// return padded width of widget
	// widget is actually drawn during mDocumentView's draw()
	return (F32)(draw_rect.mLeft + mView->getRect().getWidth() + mLeftPad + mRightPad);
}

void LLInlineViewSegment::unlinkFromDocument(LLTextBase* editor)
{
	editor->removeDocumentChild(mView);
}

void LLInlineViewSegment::linkToDocument(LLTextBase* editor)
{
	editor->addDocumentChild(mView);
}

LLLineBreakTextSegment::LLLineBreakTextSegment(S32 pos):LLTextSegment(pos,pos+1)
{
	LLStyleSP s( new LLStyle(LLStyle::Params().visible(true)));

	mFontHeight = s->getFont()->getLineHeight();
}
LLLineBreakTextSegment::LLLineBreakTextSegment(LLStyleConstSP style,S32 pos):LLTextSegment(pos,pos+1)
{
	mFontHeight = style->getFont()->getLineHeight();
}
LLLineBreakTextSegment::~LLLineBreakTextSegment()
{
}
bool LLLineBreakTextSegment::getDimensions(S32 first_char, S32 num_chars, S32& width, S32& height) const
{
	width = 0;
	height = mFontHeight;

	return true;
}
S32	LLLineBreakTextSegment::getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const
{
	return 1;
}
F32	LLLineBreakTextSegment::draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect)
{
	return  draw_rect.mLeft;
}

LLImageTextSegment::LLImageTextSegment(LLStyleConstSP style,S32 pos,class LLTextBase& editor)
:	LLTextSegment(pos,pos+1),
	mStyle( style ),
	mEditor(editor)
{
}

LLImageTextSegment::~LLImageTextSegment()
{
}

static const S32 IMAGE_HPAD = 3;

bool LLImageTextSegment::getDimensions(S32 first_char, S32 num_chars, S32& width, S32& height) const
{
	width = 0;
	height = mStyle->getFont()->getLineHeight();

	LLUIImagePtr image = mStyle->getImage();
	if( num_chars>0 && image.notNull())
	{
		width += image->getWidth() + IMAGE_HPAD;
		height = llmax(height, image->getHeight() + IMAGE_HPAD );
	}
	return false;
}

S32	 LLImageTextSegment::getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const
{
	LLUIImagePtr image = mStyle->getImage();
	
	if (image.isNull())
	{
		return 1;
	}

	S32 image_width = image->getWidth();
	if(line_offset == 0 || num_pixels>image_width + IMAGE_HPAD)
	{
		return 1;
	}

	return 0;
}

F32	LLImageTextSegment::draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect)
{
	if ( (start >= 0) && (end <= mEnd - mStart))
	{
		LLColor4 color = LLColor4::white % mEditor.getDrawContext().mAlpha;
		LLUIImagePtr image = mStyle->getImage();
		if (image.notNull())
		{
			S32 style_image_height = image->getHeight();
			S32 style_image_width = image->getWidth();
			// Text is drawn from the top of the draw_rect downward
			
			S32 text_center = draw_rect.mTop - (draw_rect.getHeight() / 2);
			// Align image to center of draw rect
			S32 image_bottom = text_center - (style_image_height / 2);
			image->draw(draw_rect.mLeft, image_bottom, 
				style_image_width, style_image_height, color);
			
			const S32 IMAGE_HPAD = 3;
			return draw_rect.mLeft + style_image_width + IMAGE_HPAD;
		}
	}
	return 0.0;
}


/** 
 * @file lltextbox.cpp
 * @brief A text display widget
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

#include "linden_common.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "llfocusmgr.h"
#include "llwindow.h"
#include "llurlregistry.h"
#include "llstyle.h"

static LLDefaultChildRegistry::Register<LLTextBox> r("text");

LLTextBox::Params::Params()
:	text_color("text_color"),
	length("length"),
	type("type"),
	border_visible("border_visible", false),
	border_drop_shadow_visible("border_drop_shadow_visible", false),
	bg_visible("bg_visible", false),
	use_ellipses("use_ellipses"),
	word_wrap("word_wrap", false),
	drop_shadow_visible("drop_shadow_visible"),
	disabled_color("disabled_color"),
	background_color("background_color"),
	v_pad("v_pad", 0),
	h_pad("h_pad", 0),
	line_spacing("line_spacing", 0),
	text("text"),
	font_shadow("font_shadow", LLFontGL::NO_SHADOW)
{}

LLTextBox::LLTextBox(const LLTextBox::Params& p)
:	LLUICtrl(p),
	LLTextBase(p),
	mBackgroundVisible( p.bg_visible ),
	mBorderVisible( p.border_visible ),
	mShadowType( p.font_shadow ),
	mBorderDropShadowVisible( p.border_drop_shadow_visible ),
	mUseEllipses( p.use_ellipses ),
	mHPad(p.h_pad),
	mVPad(p.v_pad),
	mVAlign( LLFontGL::TOP ),
	mClickedCallback(NULL),
	mTextColor(p.text_color()),
	mDisabledColor(p.disabled_color()),
	mBackgroundColor(p.background_color()),
	mHAlign(p.font_halign),
	mLineSpacing(p.line_spacing),
	mDidWordWrap(FALSE)
{
	mWordWrap = p.word_wrap;
	setText( p.text() );
}

BOOL LLTextBox::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// HACK: Only do this if there actually is something to click, so that
	// overly large text boxes in the older UI won't start eating clicks.
	if (isClickable())
	{
		handled = TRUE;

		// Route future Mouse messages here preemptively.  (Release on mouse up.)
		gFocusMgr.setMouseCapture( this );
		
		if (getSoundFlags() & MOUSE_DOWN)
		{
			make_ui_sound("UISndClick");
		}
	}

	return handled;
}

BOOL LLTextBox::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// We only handle the click if the click both started and ended within us

	// HACK: Only do this if there actually is something to click, so that
	// overly large text boxes in the older UI won't start eating clicks.
	if (isClickable() && hasMouseCapture())
	{
		handled = TRUE;

		// Release the mouse
		gFocusMgr.setMouseCapture( NULL );

		if (getSoundFlags() & MOUSE_UP)
		{
			make_ui_sound("UISndClickRelease");
		}

		// handle clicks on Urls in the textbox first
		if (! handleMouseUpOverUrl(x, y))
		{
			// DO THIS AT THE VERY END to allow the button to be destroyed
			// as a result of being clicked.  If mouseup in the widget,
			// it's been clicked
			if (mClickedCallback && ! handled)
			{
				mClickedCallback();
			}
		}
	}

	return handled;
}

BOOL LLTextBox::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	// pop up a context menu for any Url under the cursor
	return handleRightMouseDownOverUrl(this, x, y);
}

BOOL LLTextBox::handleHover(S32 x, S32 y, MASK mask)
{
	// Check to see if we're over an HTML-style link
	if (handleHoverOverUrl(x, y))
	{
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << llendl;		
		getWindow()->setCursor(UI_CURSOR_HAND);
		return TRUE;
	}

	return LLView::handleHover(x,y,mask);
}

BOOL LLTextBox::handleToolTip(S32 x, S32 y, std::string& msg, LLRect& sticky_rect_screen)
{
	return handleToolTipForUrl(this, x, y, msg, sticky_rect_screen);
}

void LLTextBox::setText(const LLStringExplicit& text)
{
	if(mWordWrap && !mDidWordWrap)
	{
		setWrappedText(text);
	}
	else
	{
		mText.assign(text);
		updateDisplayTextAndSegments();
	}
}

void LLTextBox::setLineLengths()
{
	mLineLengthList.clear();
	
	std::string::size_type  cur = 0;
	std::string::size_type  len = mDisplayText.size();

	while (cur < len) 
	{
		std::string::size_type end = mDisplayText.find('\n', cur);
		std::string::size_type runLen;
		
		if (end == std::string::npos)
		{
			runLen = len - cur;
			cur = len;
		}
		else
		{
			runLen = end - cur;
			cur = end + 1; // skip the new line character
		}

		mLineLengthList.push_back( (S32)runLen );
	}
}

LLWString LLTextBox::wrapText(const LLWString &wtext, S32 &hoffset, S32 &line_num, F32 max_width)
{
	LLWString final_wtext;

	LLWString::size_type cur = 0;
	LLWString::size_type len = wtext.size();
	while (cur < len)
	{
		LLWString::size_type end = wtext.find('\n', cur);
		if (end == LLWString::npos)
		{
			end = len;
		}
		
		bool charsRemaining = true;
		LLWString::size_type runLen = end - cur;
		if (runLen > 0)
		{
			// work out how many chars can fit onto the current line
			LLWString run(wtext, cur, runLen);
			LLWString::size_type useLen =
				mDefaultFont->maxDrawableChars(run.c_str(), max_width-hoffset, runLen, TRUE);
			charsRemaining = (cur + useLen < len);

			// try to break lines on word boundaries
			if (useLen < run.size())
			{
				LLWString::size_type prev_use_len = useLen;
				while (useLen > 0 && ! isspace(run[useLen-1]) && ! ispunct(run[useLen-1]))
				{
					--useLen;
				}
				if (useLen == 0)
				{
					useLen = prev_use_len;
				}
			}

			// add the chars that could fit onto one line to our result
			final_wtext.append(wtext, cur, useLen);
			cur += useLen;
			hoffset += mDefaultFont->getWidth(run.substr(0, useLen).c_str());

			// abort if not enough room to add any more characters
			if (useLen == 0)
			{
				break;
			}
		}

		if (charsRemaining)
		{
			if (wtext[cur] == '\n')
			{
				cur += 1;
			}
			final_wtext += '\n';
			hoffset = 0;
			line_num += 1;
		}
	}

	return final_wtext;
}

void LLTextBox::setWrappedText(const LLStringExplicit& in_text, F32 max_width)
{
	mDidWordWrap = TRUE;
	setText(wstring_to_utf8str(getWrappedText(in_text, max_width)));
}

LLWString LLTextBox::getWrappedText(const LLStringExplicit& in_text, F32 max_width)
{
	//
	// we don't want to wrap Urls otherwise we won't be able to detect their
	// presence for hyperlinking. So we look for all Urls, and then word wrap
	// the text before and after, but never break a Url in the middle. We
	// also need to consider that the Url will be displayed as a label (not
	// necessary the actual Url string).
	//

	if (max_width < 0.0f)
	{
		max_width = (F32)getRect().getWidth();
	}

	LLWString wtext = utf8str_to_wstring(in_text);
	LLWString final_wtext;
	S32 line_num = 1;
	S32 hoffset = 0;

	// find the next Url in the text string
	LLUrlMatch match;
	while ( LLUrlRegistry::instance().findUrl(wtext, match))
	{
		S32 start = match.getStart();
		S32 end = match.getEnd() + 1;

		// perform word wrap on the text before the Url
		final_wtext += wrapText(wtext.substr(0, start), hoffset, line_num, max_width);

		// add the Url (but compute width based on its label)
		S32 label_width = mDefaultFont->getWidth(match.getLabel());
		if (hoffset > 0 && hoffset + label_width > max_width)
		{
			final_wtext += '\n';
			line_num++;
			hoffset = 0;
		}
		final_wtext += wtext.substr(start, end-start);
		hoffset += label_width;
		if (hoffset > max_width)
		{
			final_wtext += '\n';
			line_num++;
			hoffset = 0;
			// eat any leading whitespace on the next line
			while (isspace(wtext[end]) && end < (S32)wtext.size())
			{
				end++;
			}
		}

		// move on to the rest of the text after the Url
		wtext = wtext.substr(end, wtext.size() - end + 1);
	}

	final_wtext += wrapText(wtext, hoffset, line_num, max_width);
	return final_wtext;
}

S32 LLTextBox::getTextPixelWidth()
{
	S32 max_line_width = 0;
	if( mLineLengthList.size() > 0 )
	{
		S32 cur_pos = 0;
		for (std::vector<S32>::iterator iter = mLineLengthList.begin();
			iter != mLineLengthList.end(); ++iter)
		{
			S32 line_length = *iter;
			S32 line_width = mDefaultFont->getWidth( mDisplayText.c_str(), cur_pos, line_length );
			if( line_width > max_line_width )
			{
				max_line_width = line_width;
			}
			cur_pos += line_length+1;
		}
	}
	else
	{
		max_line_width = mDefaultFont->getWidth(mDisplayText.c_str());
	}
	return max_line_width;
}

S32 LLTextBox::getTextPixelHeight()
{
	S32 num_lines = mLineLengthList.size();
	if( num_lines < 1 )
	{
		num_lines = 1;
	}
	return (S32)(num_lines * mDefaultFont->getLineHeight());
}

void LLTextBox::setValue(const LLSD& value )
{ 
	mDidWordWrap = FALSE;
	setText(value.asString());
}

BOOL LLTextBox::setTextArg( const std::string& key, const LLStringExplicit& text )
{
	mText.setArg(key, text);
	updateDisplayTextAndSegments();
	return TRUE;
}

void LLTextBox::draw()
{
	F32 alpha = getDrawContext().mAlpha;

	if (mBorderVisible)
	{
		gl_rect_2d_offset_local(getLocalRect(), 2, FALSE);
	}

	if( mBorderDropShadowVisible )
	{
		static LLUIColor color_drop_shadow = LLUIColorTable::instance().getColor("ColorDropShadow");
		static LLUICachedControl<S32> drop_shadow_tooltip ("DropShadowTooltip", 0);
		gl_drop_shadow(0, getRect().getHeight(), getRect().getWidth(), 0,
			color_drop_shadow % alpha, drop_shadow_tooltip);
	}

	if (mBackgroundVisible)
	{
		LLRect r( 0, getRect().getHeight(), getRect().getWidth(), 0 );
		gl_rect_2d( r, mBackgroundColor.get() % alpha );
	}

	S32 text_x = 0;
	switch( mHAlign )
	{
	case LLFontGL::LEFT:	
		text_x = mHPad;						
		break;
	case LLFontGL::HCENTER:
		text_x = getRect().getWidth() / 2;
		break;
	case LLFontGL::RIGHT:
		text_x = getRect().getWidth() - mHPad;
		break;
	}

	S32 text_y = getRect().getHeight() - mVPad;

	if ( getEnabled() )
	{
		drawText( text_x, text_y, mDisplayText, mTextColor.get() );
	}
	else
	{
		drawText( text_x, text_y, mDisplayText, mDisabledColor.get() );
	}

	if (sDebugRects)
	{
		drawDebugRect();
	}

	//// *HACK: also draw debug rectangles around currently-being-edited LLView, and any elements that are being highlighted by GUI preview code (see LLFloaterUIPreview)
	//std::set<LLView*>::iterator iter = std::find(sPreviewHighlightedElements.begin(), sPreviewHighlightedElements.end(), this);
	//if ((sEditingUI && this == sEditingUIView) || (iter != sPreviewHighlightedElements.end() && sDrawPreviewHighlights))
	//{
	//	drawDebugRect();
	//}
}

void LLTextBox::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	// reparse line lengths (don't need to recalculate the display text)
	setLineLengths();
	LLView::reshape(width, height, called_from_parent);
}

void LLTextBox::drawText( S32 x, S32 y, const LLWString &text, const LLColor4& color )
{
	F32 alpha = getDrawContext().mAlpha;
	if (mSegments.size() > 1)
	{
		// we have Urls (or other multi-styled segments)
		drawTextSegments(x, y, text);
	}
	else if( mLineLengthList.empty() )
	{
		// simple case of 1 line of text in one style
		mDefaultFont->render(text, 0, (F32)x, (F32)y, color % alpha,
							 mHAlign, mVAlign, 
							 0,
							 mShadowType,
							 S32_MAX, getRect().getWidth(), NULL, mUseEllipses);
	}
	else
	{
		// simple case of multiple lines of text, all in the same style
		S32 cur_pos = 0;
		for (std::vector<S32>::iterator iter = mLineLengthList.begin();
			iter != mLineLengthList.end(); ++iter)
		{
			S32 line_length = *iter;
			mDefaultFont->render(text, cur_pos, (F32)x, (F32)y, color % alpha,
								 mHAlign, mVAlign,
								 0,
								 mShadowType,
								 line_length, getRect().getWidth(), NULL, mUseEllipses );
			cur_pos += line_length + 1;
			S32 line_height = llfloor(mDefaultFont->getLineHeight()) + mLineSpacing; 
			y -= line_height;
			if(y < line_height)
				break;
		}
	}
}

void LLTextBox::reshapeToFitText()
{
	// wrap remaining lines that did not fit on call to setWrappedText()
	setLineLengths();

	S32 width = getTextPixelWidth();
	S32 height = getTextPixelHeight();
	reshape( width + 2 * mHPad, height + 2 * mVPad );
}

S32 LLTextBox::getDocIndexFromLocalCoord( S32 local_x, S32 local_y, BOOL round ) const
{
	// Returns the character offset for the character under the local (x, y) coordinate.
	// When round is true, if the position is on the right half of a character, the cursor
	// will be put to its right.  If round is false, the cursor will always be put to the
	// character's left.

	LLRect rect = getLocalRect();
	rect.mLeft += mHPad;
	rect.mRight -= mHPad;
	rect.mTop += mVPad;
	rect.mBottom -= mVPad;

	// Figure out which line we're nearest to.
	S32 total_lines = getLineCount();
	S32 line_height = llround( mDefaultFont->getLineHeight() ) + mLineSpacing;
	S32 line = (rect.mTop - 1 - local_y) / line_height;
	if (line >= total_lines)
	{
		return getLength(); // past the end
	}

	line = llclamp( line, 0, total_lines );
	S32 line_start = getLineStart(line);
	S32 next_start = getLineStart(line+1);
	S32	line_end = (next_start != line_start) ? next_start - 1 : getLength();
	if (line_start == -1)
	{
		return 0;
	}

	S32 line_len = line_end - line_start;
	S32 pos = mDefaultFont->charFromPixelOffset(mDisplayText.c_str(), line_start,
												(F32)(local_x - rect.mLeft),
												(F32)rect.getWidth(),
												line_len, round);

	return line_start + pos;
}

S32 LLTextBox::getLineStart( S32 line ) const
{
	line = llclamp(line, 0, getLineCount()-1);

	S32 result = 0;
	for (int i = 0; i < line; i++)
	{
		result += mLineLengthList[i] + 1 /* add newline */;
	}

	return result;
}

void LLTextBox::updateDisplayTextAndSegments()
{
	// remove any previous segment list
	clearSegments();

	// if URL parsing is turned off, then not much to bo
	if (! mParseHTML)
	{
		mDisplayText = mText.getWString();
		setLineLengths();
		return;
	}

	// create unique text segments for Urls
	mDisplayText.clear();
	S32 end = 0;
	LLUrlMatch match;
	LLWString text = mText.getWString();
		
	// find the next Url in the text string
	while ( LLUrlRegistry::instance().findUrl(text, match,
											  boost::bind(&LLTextBox::onUrlLabelUpdated, this, _1, _2)) )
	{
		// work out the char offset for the start/end of the url
		S32 seg_start = mDisplayText.size();
		S32 start = seg_start + match.getStart();
		end = start + match.getLabel().size();

		// create a segment for the text before the Url
		mSegments.insert(new LLNormalTextSegment(new LLStyle(), seg_start, start, *this));
		mDisplayText += text.substr(0, match.getStart());

		// create a segment for the Url text
		LLStyleSP html(new LLStyle);
		html->setVisible(true);
		html->setColor(mLinkColor);
		html->mUnderline = TRUE;
		html->setLinkHREF(match.getUrl());

		LLNormalTextSegment *html_seg = new LLNormalTextSegment(html, start, end, *this); 
		html_seg->setToolTip(match.getTooltip());

		mSegments.insert(html_seg);
		mDisplayText += utf8str_to_wstring(match.getLabel());

		// move on to the rest of the text after the Url
		text = text.substr(match.getEnd()+1, text.size() - match.getEnd());
	}

	// output a segment for the remaining text
	if (text.size() > 0)
	{
		mSegments.insert(new LLNormalTextSegment(new LLStyle(), end, end + text.size(), *this));
		mDisplayText += text;
	}

	// strip whitespace from the end of the text
	while (mDisplayText.size() > 0 && isspace(mDisplayText[mDisplayText.size()-1]))
	{
		mDisplayText = mDisplayText.substr(0, mDisplayText.size() - 1);

		segment_set_t::iterator it = getSegIterContaining(mDisplayText.size());
		if (it != mSegments.end())
		{
			LLTextSegmentPtr seg = *it;
			seg->setEnd(seg->getEnd()-1);
		}
	}

	// we may have changed the line lengths, so recalculate them
	setLineLengths();
}

void LLTextBox::onUrlLabelUpdated(const std::string &url, const std::string &label)
{
	if (mDidWordWrap)
	{
		// re-word wrap as the url label lengths may have changed
		setWrappedText(mText.getString());
	}
	else
	{
		// or just update the display text with the latest Url labels
		updateDisplayTextAndSegments();
	}
}

bool LLTextBox::isClickable() const
{
	// return true if we have been given a click callback
	if (mClickedCallback)
	{
		return true;
	}

	// also return true if we have a clickable Url in the text
	segment_set_t::const_iterator it;
	for (it = mSegments.begin(); it != mSegments.end(); ++it)
	{
		LLTextSegmentPtr segmentp = *it;
		if (segmentp)
		{
			const LLStyleSP style = segmentp->getStyle();
			if (style && style->isLink())
			{
				return true;
			}
		}
	}

	// otherwise there is nothing clickable here
	return false;
}

void LLTextBox::drawTextSegments(S32 init_x, S32 init_y, const LLWString &text)
{
	F32 alpha = getDrawContext().mAlpha;

	const S32 text_len = text.length();
	if (text_len <= 0)
	{
		return;
	}

	S32 cur_line = 0;
	S32 num_lines = getLineCount();
	S32 line_start = getLineStart(cur_line);
	S32 line_height = llround( mDefaultFont->getLineHeight() ) + mLineSpacing;
	F32 text_y = (F32) init_y;
	segment_set_t::iterator cur_seg = mSegments.begin();

	// render a line of text at a time
	const LLRect textRect = getLocalRect();
	while((textRect.mBottom <= text_y) && (cur_line < num_lines))
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
		
		// render all segments on this line
		F32 text_x = init_x;
		S32 seg_start = line_start;
		while (seg_start < line_end && cur_seg != mSegments.end())
		{
			// move to the next segment (or continue the previous one)
			LLTextSegment *cur_segment = *cur_seg;
			while (cur_segment->getEnd() <= seg_start)
			{
				if (++cur_seg == mSegments.end())
				{
					return;
				}
				cur_segment = *cur_seg;
			}

			// Draw a segment within the line
			S32 clipped_end	= llmin( line_end, cur_segment->getEnd() );
			S32 clipped_len = clipped_end - seg_start;
			if( clipped_len > 0 )
			{
				LLStyleSP style = cur_segment->getStyle();
				if (style && style->isVisible())
				{
					// work out the color for the segment
					LLColor4 color ;
					if (getEnabled())
					{
						color = style->isLink() ? mLinkColor.get() : mTextColor.get();
					}
					else
					{
						color = mDisabledColor.get();
					}
					color = color % alpha;

					// render a single line worth for this segment
					mDefaultFont->render(text, seg_start, text_x, text_y, color,
										 mHAlign, mVAlign, 0, mShadowType, clipped_len,
										 textRect.getWidth(), &text_x, mUseEllipses);
				}

				seg_start += clipped_len;
			}
		}

		// move down one line
		text_y -= (F32)line_height;
		line_start = next_start;
		cur_line++;
	}
}

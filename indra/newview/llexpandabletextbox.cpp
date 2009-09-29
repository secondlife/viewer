/** 
 * @file llexpandabletextbox.cpp
 * @brief LLExpandableTextBox and related class implementations
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
#include "llexpandabletextbox.h"

#include "llscrollcontainer.h"

static LLDefaultChildRegistry::Register<LLExpandableTextBox> t1("expandable_text");

LLExpandableTextBox::LLTextBoxEx::Params::Params()
: expand_textbox("expand_textbox")
{
}

LLExpandableTextBox::LLTextBoxEx::LLTextBoxEx(const Params& p)
: LLTextBox(p)
{
	setIsChrome(TRUE);

	LLTextBox::Params params = p.expand_textbox;
	mExpandTextBox = LLUICtrlFactory::create<LLTextBox>(params);
	addChild(mExpandTextBox);

	LLRect rc = getLocalRect();
	rc.mRight -= getHPad();
	rc.mLeft = rc.mRight - mExpandTextBox->getTextPixelWidth();
	rc.mTop = mExpandTextBox->getTextPixelHeight();
	mExpandTextBox->setRect(rc);
}

BOOL LLExpandableTextBox::LLTextBoxEx::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL ret = LLTextBox::handleMouseUp(x, y, mask);

	if(mExpandTextBox->getRect().pointInRect(x, y))
	{
		onCommit();
	}

	return ret;
}

void LLExpandableTextBox::LLTextBoxEx::draw()
{
	// draw text box
	LLTextBox::draw();
	// force text box to draw children
	LLUICtrl::draw();
}

void LLExpandableTextBox::LLTextBoxEx::drawText( S32 x, S32 y, const LLWString &text, const LLColor4& color )
{
	// *NOTE:dzaporozhan:
	// Copy/paste from LLTextBox::drawText in order to modify last 
	// line width if needed and who "More" link
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

		mExpandTextBox->setVisible(FALSE);
	}
	else
	{
		// simple case of multiple lines of text, all in the same style
		S32 cur_pos = 0;
		for (std::vector<S32>::iterator iter = mLineLengthList.begin();
			iter != mLineLengthList.end(); ++iter)
		{
			S32 line_length = *iter;
			S32 line_height = llfloor(mDefaultFont->getLineHeight()) + mLineSpacing;
			S32 max_pixels = getRect().getWidth();

			if(iter + 1 != mLineLengthList.end() 
				&& y - line_height < line_height)
			{
				max_pixels = getCropTextWidth();
			}

			mDefaultFont->render(text, cur_pos, (F32)x, (F32)y, color % alpha,
				mHAlign, mVAlign,
				0,
				mShadowType,
				line_length, max_pixels, NULL, mUseEllipses );

			cur_pos += line_length + 1;

			y -= line_height;
			if(y < line_height)
			{
				if( mLineLengthList.end() != iter + 1 )
				{
					showExpandText(y);
				}
				else
				{
					hideExpandText();
				}
				break;
			}
		}
	}
}

void LLExpandableTextBox::LLTextBoxEx::showExpandText(S32 y)
{
	LLRect rc = mExpandTextBox->getRect();
	rc.mTop = y + mExpandTextBox->getTextPixelHeight();
	rc.mBottom = y;
	mExpandTextBox->setRect(rc);
	mExpandTextBox->setVisible(TRUE);
}

void LLExpandableTextBox::LLTextBoxEx::hideExpandText() 
{ 
	mExpandTextBox->setVisible(FALSE); 
}

S32 LLExpandableTextBox::LLTextBoxEx::getCropTextWidth() 
{ 
	return mExpandTextBox->getRect().mLeft - getHPad() * 2; 
}

void LLExpandableTextBox::LLTextBoxEx::drawTextSegments(S32 init_x, S32 init_y, const LLWString &text)
{
	// *NOTE:dzaporozhan:
	// Copy/paste from LLTextBox::drawTextSegments in order to modify last 
	// line width if needed and who "More" link
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

					S32 max_pixels = textRect.getWidth();

					if(cur_line + 1 < num_lines
						&& text_y - line_height < line_height)
					{
						max_pixels = getCropTextWidth();
					}

					// render a single line worth for this segment
					mDefaultFont->render(text, seg_start, text_x, text_y, color,
						mHAlign, mVAlign, 0, mShadowType, clipped_len,
						max_pixels, &text_x, mUseEllipses);
				}

				seg_start += clipped_len;
			}
		}

		// move down one line
		text_y -= (F32)line_height;
		line_start = next_start;
		cur_line++;
		if(text_y < line_height)
		{
			if( cur_line < num_lines )
			{
				showExpandText((S32)text_y);
			}
			else
			{
				hideExpandText();
			}
			break;
		}
	}
}

S32 LLExpandableTextBox::LLTextBoxEx::getVerticalTextDelta()
{
	S32 text_height = getTextPixelHeight();
	S32 textbox_height = getRect().getHeight();

	return text_height - textbox_height;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLExpandableTextBox::Params::Params()
: textbox("textbox")
, scroll("scroll")
, max_height("max_height", 0)
, bg_visible("bg_visible", false)
, expanded_bg_visible("expanded_bg_visible", true)
, bg_color("bg_color", LLColor4::black)
, expanded_bg_color("expanded_bg_color", LLColor4::black)
{
}

LLExpandableTextBox::LLExpandableTextBox(const Params& p)
: LLUICtrl(p)
, mMaxHeight(p.max_height)
, mBGVisible(p.bg_visible)
, mExpandedBGVisible(p.expanded_bg_visible)
, mBGColor(p.bg_color)
, mExpandedBGColor(p.expanded_bg_color)
, mExpanded(false)
{
	LLRect rc = getLocalRect();

	LLScrollContainer::Params scroll_params = p.scroll;
	scroll_params.rect(rc);
	mScroll = LLUICtrlFactory::create<LLScrollContainer>(scroll_params);
	addChild(mScroll);

	LLTextBoxEx::Params textbox_params = p.textbox;
	textbox_params.rect(rc);
	mTextBox = LLUICtrlFactory::create<LLTextBoxEx>(textbox_params);
	mScroll->addChild(mTextBox);

	updateTextBoxRect();

	mTextBox->setCommitCallback(boost::bind(&LLExpandableTextBox::onExpandClicked, this));
}

void LLExpandableTextBox::draw()
{
	if(mBGVisible && !mExpanded)
	{
		gl_rect_2d(getLocalRect(), mBGColor.get(), TRUE);
	}
	if(mExpandedBGVisible && mExpanded)
	{
		gl_rect_2d(getLocalRect(), mExpandedBGColor.get(), TRUE);
	}

	collapseIfPosChanged();

	LLUICtrl::draw();
}

void LLExpandableTextBox::collapseIfPosChanged()
{
	if(mExpanded)
	{
		LLView* parentp = getParent();
		LLRect parent_rect = parentp->getRect();
		parentp->localRectToOtherView(parent_rect, &parent_rect, getRootView());

		if(parent_rect.mLeft != mParentRect.mLeft 
			|| parent_rect.mTop != mParentRect.mTop)
		{
			collapseTextBox();
		}
	}
}

void LLExpandableTextBox::onExpandClicked()
{
	expandTextBox();
}

void LLExpandableTextBox::updateTextBoxRect()
{
	LLRect rc = getLocalRect();

	rc.mLeft += mScroll->getBorderWidth();
	rc.mRight -= mScroll->getBorderWidth();
	rc.mTop -= mScroll->getBorderWidth();
	rc.mBottom += mScroll->getBorderWidth();

	mTextBox->reshape(rc.getWidth(), rc.getHeight());
	mTextBox->setRect(rc);
}

S32 LLExpandableTextBox::recalculateTextDelta(S32 text_delta)
{
	LLRect expanded_rect = getLocalRect();
	LLView* root_view = getRootView();
	LLRect window_rect = root_view->getRect();

	LLRect expanded_screen_rect;
	localRectToOtherView(expanded_rect, &expanded_screen_rect, root_view);

	// don't allow expanded text box bottom go off screen
	if(expanded_screen_rect.mBottom - text_delta < window_rect.mBottom)
	{
		text_delta = expanded_screen_rect.mBottom - window_rect.mBottom;
	}
	// show scroll bar if max_height is valid 
	// and expanded size is greater that max_height
	else if(mMaxHeight > 0 && expanded_rect.getHeight() + text_delta > mMaxHeight)
	{
		text_delta = mMaxHeight - expanded_rect.getHeight();
	}

	return text_delta;
}

void LLExpandableTextBox::expandTextBox()
{
	S32 text_delta = mTextBox->getVerticalTextDelta();
	text_delta += mTextBox->getVPad() * 2 + mScroll->getBorderWidth() * 2;
	// no need to expand
	if(text_delta <= 0)
	{
		return;
	}

	saveCollapsedState();

	LLRect expanded_rect = getLocalRect();
	LLRect expanded_screen_rect;

	S32 updated_text_delta = recalculateTextDelta(text_delta);
	// actual expand
	expanded_rect.mBottom -= updated_text_delta;

	LLRect text_box_rect = mTextBox->getRect();

	// check if we need to show scrollbar
	if(text_delta != updated_text_delta)
	{
		static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);

		// disable horizontal scrollbar
		text_box_rect.mRight -= scrollbar_size;
		// text box size has changed - redo text wrap
		mTextBox->setWrappedText(mText, text_box_rect.getWidth());
		// recalculate text delta since text wrap changed text height
		text_delta = mTextBox->getVerticalTextDelta() + mTextBox->getVPad() * 2;
	}

	// expand text
	text_box_rect.mBottom -= text_delta;
	mTextBox->reshape(text_box_rect.getWidth(), text_box_rect.getHeight());
	mTextBox->setRect(text_box_rect);

	// expand text box
	localRectToOtherView(expanded_rect, &expanded_screen_rect, getParent());
	reshape(expanded_screen_rect.getWidth(), expanded_screen_rect.getHeight(), FALSE);
	setRect(expanded_screen_rect);

	setFocus(TRUE);
	// this lets us receive top_lost event(needed to collapse text box)
	// it also draws text box above all other ui elements
	gFocusMgr.setTopCtrl(this);

	mExpanded = true;
}

void LLExpandableTextBox::collapseTextBox()
{
	if(!mExpanded)
	{
		return;
	}

	mExpanded = false;

	reshape(mCollapsedRect.getWidth(), mCollapsedRect.getHeight(), FALSE);
	setRect(mCollapsedRect);

	updateTextBoxRect();

	mTextBox->setWrappedText(mText);
	if(gFocusMgr.getTopCtrl() == this)
	{
		gFocusMgr.setTopCtrl(NULL);
	}
}

void LLExpandableTextBox::onFocusLost()
{
	collapseTextBox();

	LLUICtrl::onFocusLost();
}

void LLExpandableTextBox::onTopLost()
{
	collapseTextBox();

	LLUICtrl::onTopLost();
}

void LLExpandableTextBox::setValue(const LLSD& value)
{
	collapseTextBox();

	mText = value.asString();
	mTextBox->setValue(value);
}

void LLExpandableTextBox::setText(const std::string& str)
{
	collapseTextBox();

	mText = str;
	mTextBox->setText(str);
}

void LLExpandableTextBox::saveCollapsedState()
{
	mCollapsedRect = getRect();

	mParentRect = getParent()->getRect();
	// convert parent rect to screen coordinates, 
	// this will allow to track parent's position change
	getParent()->localRectToOtherView(mParentRect, &mParentRect, getRootView());
}

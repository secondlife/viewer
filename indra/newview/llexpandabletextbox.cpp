/** 
 * @file llexpandabletextbox.cpp
 * @brief LLExpandableTextBox and related class implementations
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include "llexpandabletextbox.h"

#include "llscrollcontainer.h"
#include "lltrans.h"
#include "llwindow.h"
#include "llviewerwindow.h"

static LLDefaultChildRegistry::Register<LLExpandableTextBox> t1("expandable_text");

class LLExpanderSegment : public LLTextSegment
{
public:
	LLExpanderSegment(const LLStyleSP& style, S32 start, S32 end, const std::string& more_text, LLTextBase& editor )
	:	LLTextSegment(start, end),
		mEditor(editor),
		mStyle(style),
		mExpanderLabel(more_text)
	{}

	/*virtual*/ bool	getDimensions(S32 first_char, S32 num_chars, S32& width, S32& height) const 
	{
		// more label always spans width of text box
		if (num_chars == 0)
		{
			width = 0; 
			height = 0;
		}
		else
		{
			width = mEditor.getDocumentView()->getRect().getWidth() - mEditor.getHPad(); 
			height = mStyle->getFont()->getLineHeight();
		}
		return true;
	}
	/*virtual*/ S32		getOffset(S32 segment_local_x_coord, S32 start_offset, S32 num_chars, bool round) const 
	{ 
		return start_offset;
	}
	/*virtual*/ S32		getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const 
	{ 
		// require full line to ourselves
		if (line_offset == 0) 
		{
			// print all our text
			return getEnd() - getStart(); 
		}
		else
		{
			// wait for next line
			return 0;
		}
	}
	/*virtual*/ F32		draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect)
	{
		F32 right_x;
		mStyle->getFont()->renderUTF8(mExpanderLabel, start, 
									draw_rect.mRight, draw_rect.mTop, 
									mStyle->getColor(), 
									LLFontGL::RIGHT, LLFontGL::TOP, 
									0, 
									mStyle->getShadowType(), 
									end - start, draw_rect.getWidth(), 
									&right_x, 
									mEditor.getUseEllipses());
		return right_x;
	}
	/*virtual*/ bool	canEdit() const { return false; }
	// eat handleMouseDown event so we get the mouseup event
	/*virtual*/ BOOL	handleMouseDown(S32 x, S32 y, MASK mask) { return TRUE; }
	/*virtual*/ BOOL	handleMouseUp(S32 x, S32 y, MASK mask) { mEditor.onCommit(); return TRUE; }
	/*virtual*/ BOOL	handleHover(S32 x, S32 y, MASK mask) 
	{
		LLUI::getWindow()->setCursor(UI_CURSOR_HAND);
		return TRUE; 
	}
private:
	LLTextBase& mEditor;
	LLStyleSP	mStyle;
	std::string	mExpanderLabel;
};

LLExpandableTextBox::LLTextBoxEx::Params::Params()
{
}

LLExpandableTextBox::LLTextBoxEx::LLTextBoxEx(const Params& p)
:	LLTextEditor(p),
	mExpanderLabel(p.label.isProvided() ? p.label : LLTrans::getString("More")),
	mExpanderVisible(false)
{
	setIsChrome(TRUE);

}

void LLExpandableTextBox::LLTextBoxEx::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLTextEditor::reshape(width, height, called_from_parent);

	hideOrShowExpandTextAsNeeded();
}

void LLExpandableTextBox::LLTextBoxEx::setText(const LLStringExplicit& text,const LLStyle::Params& input_params)
{
	// LLTextBox::setText will obliterate the expander segment, so make sure
	// we generate it again by clearing mExpanderVisible
	mExpanderVisible = false;
	LLTextEditor::setText(text, input_params);

	hideOrShowExpandTextAsNeeded();
}


void LLExpandableTextBox::LLTextBoxEx::showExpandText()
{
	if (!mExpanderVisible)
	{
		// make sure we're scrolled to top when collapsing
		if (mScroller)
		{
			mScroller->goToTop();
		}
		// get fully visible lines
		std::pair<S32, S32> visible_lines = getVisibleLines(true);
		S32 last_line = visible_lines.second - 1;

		LLStyle::Params expander_style(getDefaultStyleParams());
		expander_style.font.style = "UNDERLINE";
		expander_style.color = LLUIColorTable::instance().getColor("HTMLLinkColor");
		LLExpanderSegment* expanderp = new LLExpanderSegment(new LLStyle(expander_style), getLineStart(last_line), getLength() + 1, mExpanderLabel, *this);
		insertSegment(expanderp);
		mExpanderVisible = true;
	}

}

//NOTE: obliterates existing styles (including hyperlinks)
void LLExpandableTextBox::LLTextBoxEx::hideExpandText() 
{ 
	if (mExpanderVisible)
	{
		// this will overwrite the expander segment and all text styling with a single style
		LLStyleConstSP sp(new LLStyle(getDefaultStyleParams()));
		LLNormalTextSegment* segmentp = new LLNormalTextSegment(sp, 0, getLength() + 1, *this);
		insertSegment(segmentp);
		
		mExpanderVisible = false;
	}
}

S32 LLExpandableTextBox::LLTextBoxEx::getVerticalTextDelta()
{
	S32 text_height = getTextPixelHeight();
	S32 textbox_height = getRect().getHeight();

	return text_height - textbox_height;
}

S32 LLExpandableTextBox::LLTextBoxEx::getTextPixelHeight()
{
	return getTextBoundingRect().getHeight();
}

void LLExpandableTextBox::LLTextBoxEx::hideOrShowExpandTextAsNeeded()
{
	// Restore the text box contents to calculate the text height properly,
	// otherwise if a part of the text is hidden under "More" link
	// getTextPixelHeight() returns only the height of currently visible text
	// including the "More" link. See STORM-250.
	hideExpandText();

	// Show the expander a.k.a. "More" link if we need it, depending on text
	// contents height. If not, keep it hidden.
	if (getTextPixelHeight() > getRect().getHeight())
	{
		showExpandText();
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLExpandableTextBox::Params::Params()
:	textbox("textbox"),
	scroll("scroll"),
	max_height("max_height", 0),
	bg_visible("bg_visible", false),
	expanded_bg_visible("expanded_bg_visible", true),
	bg_color("bg_color", LLColor4::black),
	expanded_bg_color("expanded_bg_color", LLColor4::black)
{
}

LLExpandableTextBox::LLExpandableTextBox(const Params& p)
:	LLUICtrl(p),
	mMaxHeight(p.max_height),
	mBGVisible(p.bg_visible),
	mExpandedBGVisible(p.expanded_bg_visible),
	mBGColor(p.bg_color),
	mExpandedBGColor(p.expanded_bg_color),
	mExpanded(false)
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
	// hide "more" link, and show full text contents
	mTextBox->hideExpandText();

	// *HACK dz
	// hideExpandText brakes text styles (replaces hyper-links with plain text), see ticket EXT-3290
	// Set text again to make text box re-apply styles.
	// *TODO Find proper solution to fix this issue.
	// Maybe add removeSegment to LLTextBase
	mTextBox->setTextBase(mText);

	S32 text_delta = mTextBox->getVerticalTextDelta();
	text_delta += mTextBox->getVPad() * 2;
	text_delta += mScroll->getBorderWidth() * 2;
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
		// Should be handled automatically in reshape() below. JC
		//mTextBox->setWrappedText(mText, text_box_rect.getWidth());

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
	gViewerWindow->addPopup(this);

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

	gViewerWindow->removePopup(this);
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

void LLExpandableTextBox::updateTextShape()
{
	// I guess this should be done on every reshape(),
	// but adding this code to reshape() currently triggers bug VWR-26455,
	// which makes the text virtually unreadable.
	llassert(!mExpanded);
	updateTextBoxRect();
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

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
#include "llwindow.h"

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
			height = llceil(mStyle->getFont()->getLineHeight());
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
:	more_label("more_label")
{
}

LLExpandableTextBox::LLTextBoxEx::LLTextBoxEx(const Params& p)
:	LLTextEditor(p),
	mExpanderLabel(p.more_label),
	mExpanderVisible(false)
{
	setIsChrome(TRUE);

}

void LLExpandableTextBox::LLTextBoxEx::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	hideExpandText();
	LLTextEditor::reshape(width, height, called_from_parent);

	if (getTextPixelHeight() > getRect().getHeight())
	{
		showExpandText();
	}
}

void LLExpandableTextBox::LLTextBoxEx::setText(const LLStringExplicit& text,const LLStyle::Params& input_params)
{
	// LLTextBox::setText will obliterate the expander segment, so make sure
	// we generate it again by clearing mExpanderVisible
	mExpanderVisible = false;
	LLTextEditor::setText(text, input_params);

	// text contents have changed, segments are cleared out
	// so hide the expander and determine if we need it
	//mExpanderVisible = false;
	if (getTextPixelHeight() > getRect().getHeight())
	{
		showExpandText();
	}
	else
	{
		hideExpandText();
	}
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

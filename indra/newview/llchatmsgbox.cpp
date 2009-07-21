/** 
 * @file llchatmsgbox.cpp
 * @brief chat history text box, able to show array of strings with separator
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

#include "llchatmsgbox.h"
#include "llwindow.h"
#include "llfocusmgr.h"

static LLDefaultChildRegistry::Register<LLChatMsgBox> r("text_chat");

LLChatMsgBox::Params::Params()
:	text_color("text_color"),
	highlight_on_hover("hover", false),
	border_visible("border_visible", false),
	border_drop_shadow_visible("border_drop_shadow_visible", false),
	bg_visible("bg_visible", false),
	use_ellipses("use_ellipses"),
	word_wrap("word_wrap", false),
	hover_color("hover_color"),
	disabled_color("disabled_color"),
	background_color("background_color"),
	border_color("border_color"),
	v_pad("v_pad", 0),
	h_pad("h_pad", 0),
	line_spacing("line_spacing", 0),
	text("text"),
	font_shadow("font_shadow", LLFontGL::NO_SHADOW)
{}

LLChatMsgBox::LLChatMsgBox(const LLChatMsgBox::Params& p)
:	LLUICtrl(p),
    mFontGL(p.font),
	mHoverActive( p.highlight_on_hover ),
	mHasHover( FALSE ),
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
	mBorderColor(p.border_color()),
	mHoverColor(p.hover_color()),
	mHAlign(p.font_halign),
	mLineSpacing(p.line_spacing),
	mWordWrap( p.word_wrap ),
	mFontStyle(LLFontGL::getStyleFromString(p.font.style))
{
	setText( p.text() );
}

BOOL LLChatMsgBox::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// HACK: Only do this if there actually is a click callback, so that
	// overly large text boxes in the older UI won't start eating clicks.
	if (mClickedCallback)
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

BOOL LLChatMsgBox::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// We only handle the click if the click both started and ended within us

	// HACK: Only do this if there actually is a click callback, so that
	// overly large text boxes in the older UI won't start eating clicks.
	if (mClickedCallback
		&& hasMouseCapture())
	{
		handled = TRUE;

		// Release the mouse
		gFocusMgr.setMouseCapture( NULL );

		if (getSoundFlags() & MOUSE_UP)
		{
			make_ui_sound("UISndClickRelease");
		}

		// DO THIS AT THE VERY END to allow the button to be destroyed as a result of being clicked.
		// If mouseup in the widget, it's been clicked
		if (mClickedCallback)
		{
			mClickedCallback();
		}
	}

	return handled;
}

BOOL LLChatMsgBox::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLView::handleHover(x,y,mask);
	if(mHoverActive)
	{
		mHasHover = TRUE; // This should be set every frame during a hover.
		getWindow()->setCursor(UI_CURSOR_ARROW);
	}

	return (handled || mHasHover);
}

void	LLChatMsgBox::addText( const LLStringExplicit& text )
{
	boost::shared_ptr<text_block> t(new text_block());
	t->text = wrapText(text);
	setLineLengths(*t);
	mTextStrings.push_back(t);
}

void LLChatMsgBox::setText(const LLStringExplicit& text)
{
	mTextStrings.clear();

	addText(text);

}

void LLChatMsgBox::resetLineLengths()
{
	for(std::vector< boost::shared_ptr<text_block> >::iterator it = mTextStrings.begin();
			it!=mTextStrings.end();++it)
	{
		boost::shared_ptr<text_block> tblock = *it;
		setLineLengths(*tblock);
	}
}

void LLChatMsgBox::setLineLengths(text_block& t)
{
	t.lines.clear();
	
	std::string::size_type  cur = 0;
	std::string::size_type  len = t.text.length();

	while (cur < len) 
	{
		std::string::size_type end = t.text.getWString().find('\n', cur);
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

		t.lines.push_back( (S32)runLen );
	}
}

std::string LLChatMsgBox::wrapText(const LLStringExplicit& in_text, F32 max_width)
{
	if (max_width < 0.0f)
	{
		max_width = (F32)getRect().getWidth();
	}

	LLWString wtext = utf8str_to_wstring(in_text);
	LLWString final_wtext;

	LLWString::size_type  cur = 0;;
	LLWString::size_type  len = wtext.size();
	while (cur < len)
	{
		LLWString::size_type end = wtext.find('\n', cur);
		if (end == LLWString::npos)
		{
			end = len;
		}
		
		LLWString::size_type runLen = end - cur;
		if (runLen > 0)
		{
			LLWString run(wtext, cur, runLen);
			LLWString::size_type useLen =
				mFontGL->maxDrawableChars(run.c_str(), max_width, runLen, TRUE);

			final_wtext.append(wtext, cur, useLen);
			cur += useLen;
			// not enough room to add any more characters
			if (useLen == 0) break;
		}

		if (cur < len)
		{
			if (wtext[cur] == '\n')
				cur += 1;
			else
				final_wtext += '\n';
		}
	}
	
	std::string final_text = wstring_to_utf8str(final_wtext);
	return final_text;
}

S32	LLChatMsgBox::getTextLinesNum()
{
	S32 num_lines = 0;
	for(std::vector< boost::shared_ptr<text_block> >::iterator it = mTextStrings.begin();
			it!=mTextStrings.end();++it)
	{
		boost::shared_ptr<text_block> tblock = *it;
		num_lines+=tblock->lines.size();
	}

	if( num_lines < 1 )
	{
		num_lines = 1;
	}

	return num_lines;
}

S32 LLChatMsgBox::getTextPixelHeight()
{
	S32 num_lines = getTextLinesNum();
	return (S32)(num_lines * mFontGL->getLineHeight() + (num_lines-1)*4);
}

void LLChatMsgBox::setValue(const LLSD& value )
{ 
	setText(value.asString());
}


void LLChatMsgBox::draw()
{
	if (mBorderVisible)
	{
		gl_rect_2d_offset_local(getLocalRect(), 2, FALSE);
	}

	if( mBorderDropShadowVisible )
	{
		static LLUICachedControl<LLColor4> color_drop_shadow ("ColorDropShadow", *(new LLColor4));
		static LLUICachedControl<S32> drop_shadow_tooltip ("DropShadowTooltip", 0);
		gl_drop_shadow(0, getRect().getHeight(), getRect().getWidth(), 0,
			color_drop_shadow, drop_shadow_tooltip);
	}

	if (mBackgroundVisible)
	{
		LLRect r( 0, getRect().getHeight(), getRect().getWidth(), 0 );
		gl_rect_2d( r, mBackgroundColor.get() );
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
		if(mHasHover)
		{
			drawText( text_x, text_y, mHoverColor.get() );
		}
		else
		{
			drawText( text_x, text_y, mTextColor.get() );
		}				
	}
	else
	{
		drawText( text_x, text_y, mDisabledColor.get() );
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

	mHasHover = FALSE; // This is reset every frame.
}

void LLChatMsgBox::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	// reparse line lengths
	LLView::reshape(width, height, called_from_parent);
	resetLineLengths();
}

void LLChatMsgBox::drawText( S32 x, S32 y, const LLColor4& color )
{
	S32 width = getRect().getWidth()-10;
	
	for(std::vector< boost::shared_ptr<text_block> >::iterator it = mTextStrings.begin();
			it!=mTextStrings.end();++it)
	{
		boost::shared_ptr<text_block> tblock = *it;
		
		S32 cur_pos = 0;
		for (std::vector<S32>::iterator iter = tblock->lines.begin();
			iter != tblock->lines.end(); ++iter)
		{
			S32 line_length = *iter;
			mFontGL->render(tblock->text, cur_pos, (F32)x, (F32)y, color,
							mHAlign, mVAlign,
							mFontStyle,
							mShadowType,
							line_length, getRect().getWidth(), NULL, TRUE, mUseEllipses );
			cur_pos += line_length + 1;
			y -= llfloor(mFontGL->getLineHeight()) + mLineSpacing;

		}
		std::vector< boost::shared_ptr<text_block> >::iterator next = it;
		++next;
		if(next == mTextStrings.end())
			break;
		//separator
		gl_line_2d(5,y-2,width,y-2,LLColor4::grey);
		y-=4;
	}
}


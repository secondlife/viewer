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

#define LLTEXTBOX_CPP
#include "lltextbox.h"

#include "lluictrlfactory.h"
#include "llfocusmgr.h"
#include "llwindow.h"
#include "llurlregistry.h"
#include "llstyle.h"

static LLDefaultChildRegistry::Register<LLTextBox> r("text");

// Compiler optimization, generate extern template
template class LLTextBox* LLView::getChild<class LLTextBox>(
	const std::string& name, BOOL recurse) const;

LLTextBox::LLTextBox(const LLTextBox::Params& p)
:	LLTextBase(p),
	mClickedCallback(NULL)
{}

LLTextBox::~LLTextBox()
{}

BOOL LLTextBox::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL	handled = LLTextBase::handleMouseDown(x, y, mask);

	if (getSoundFlags() & MOUSE_DOWN)
	{
		make_ui_sound("UISndClick");
	}

	if (!handled && mClickedCallback)
	{
		// Route future Mouse messages here preemptively.  (Release on mouse up.)
		gFocusMgr.setMouseCapture( this );

		handled = TRUE;
	}

	return handled;
}

BOOL LLTextBox::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	if (getSoundFlags() & MOUSE_UP)
	{
		make_ui_sound("UISndClickRelease");
	}

	// We only handle the click if the click both started and ended within us
	if (hasMouseCapture())
	{
		// Release the mouse
		gFocusMgr.setMouseCapture( NULL );

		// DO THIS AT THE VERY END to allow the button  to be destroyed
		// as a result of being clicked.  If mouseup in the widget,
		// it's been clicked
		if (mClickedCallback && !handled)
		{
			mClickedCallback();
			handled = TRUE;
		}
	}
	else
	{
		handled = LLTextBase::handleMouseUp(x, y, mask);
	}

	return handled;
}

BOOL LLTextBox::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLTextBase::handleHover(x, y, mask);
	if (!handled && mClickedCallback)
	{
		// Clickable text boxes change the cursor to a hand
		LLUI::getWindow()->setCursor(UI_CURSOR_HAND);
		return TRUE;
	}
	return handled;
}

void LLTextBox::setText(const LLStringExplicit& text , const LLStyle::Params& input_params )
{
	// does string argument insertion
	mText.assign(text);
	
	LLTextBase::setText(mText.getString(), input_params );
}

void LLTextBox::setClickedCallback( boost::function<void (void*)> cb, void* userdata /*= NULL */ )
{
	mClickedCallback = boost::bind(cb, userdata);
}

S32 LLTextBox::getTextPixelWidth()
{
	return getTextBoundingRect().getWidth();
}

S32 LLTextBox::getTextPixelHeight()
{
	return getTextBoundingRect().getHeight();
}


LLSD LLTextBox::getValue() const
{
	return LLSD(getText());
}

BOOL LLTextBox::setTextArg( const std::string& key, const LLStringExplicit& text )
{
	mText.setArg(key, text);
	LLTextBase::setText(mText.getString());

	return TRUE;
}


void LLTextBox::reshapeToFitText()
{
	reflow();

	S32 width = getTextPixelWidth();
	S32 height = getTextPixelHeight();
	reshape( width + 2 * mHPad, height + 2 * mVPad, FALSE );
}


void LLTextBox::onUrlLabelUpdated(const std::string &url, const std::string &label)
{
	needsReflow();
}


/** 
 * @file lltextbox.cpp
 * @brief A text display widget
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

void LLTextBox::setEnabled(BOOL enabled)
{
	// just treat enabled as read-only flag
	bool read_only = !enabled;
	if (read_only != mReadOnly)
	{
		LLTextBase::setReadOnly(read_only);
		updateSegments();
	}
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
	return getViewModel()->getValue();
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


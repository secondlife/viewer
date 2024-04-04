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
	const std::string& name, bool recurse) const;

LLTextBox::LLTextBox(const LLTextBox::Params& p)
:	LLTextBase(p),
	mClickedCallback(NULL),
	mShowCursorHand(true)
{
	mSkipTripleClick = true;
}

LLTextBox::~LLTextBox()
{}

bool LLTextBox::handleMouseDown(S32 x, S32 y, MASK mask)
{
	bool	handled = LLTextBase::handleMouseDown(x, y, mask);

	if (getSoundFlags() & MOUSE_DOWN)
	{
		make_ui_sound("UISndClick");
	}

	if (!handled && mClickedCallback)
	{
		handled = true;
	}

	if (handled)
	{
		// Route future Mouse messages here preemptively.  (Release on mouse up.)
		gFocusMgr.setMouseCapture( this );
	}

	return handled;
}

bool LLTextBox::handleMouseUp(S32 x, S32 y, MASK mask)
{
	bool	handled = LLTextBase::handleMouseUp(x, y, mask);

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
			handled = true;
		}
	}

	return handled;
}

bool LLTextBox::handleHover(S32 x, S32 y, MASK mask)
{
	bool handled = LLTextBase::handleHover(x, y, mask);
	if (!handled && mClickedCallback && mShowCursorHand)
	{
		// Clickable text boxes change the cursor to a hand
		LLUI::getInstance()->getWindow()->setCursor(UI_CURSOR_HAND);
		return true;
	}
	return handled;
}

void LLTextBox::setEnabled(bool enabled)
{
	// just treat enabled as read-only flag
	bool read_only = !enabled;
	if (read_only != mReadOnly)
	{
		LLTextBase::setReadOnly(read_only);
		updateSegments();
	}
	LLTextBase::setEnabled(enabled);
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

bool LLTextBox::setTextArg( const std::string& key, const LLStringExplicit& text )
{
	mText.setArg(key, text);
	LLTextBase::setText(mText.getString());

	return true;
}


void LLTextBox::reshapeToFitText(bool called_from_parent)
{
	reflow();

	S32 width = getTextPixelWidth();
	S32 height = getTextPixelHeight();
    //consider investigating reflow() to find missing width pixel (see SL-17045 changes)
	reshape( width + 2 * mHPad + 1, height + 2 * mVPad, called_from_parent );
}


void LLTextBox::onUrlLabelUpdated(const std::string &url, const std::string &label)
{
	needsReflow();
}


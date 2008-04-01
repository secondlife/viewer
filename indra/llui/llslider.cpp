/** 
 * @file llslider.cpp
 * @brief LLSlider base class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llslider.h"
#include "llui.h"

#include "llgl.h"
#include "llwindow.h"
#include "llfocusmgr.h"
#include "llkeyboard.h"			// for the MASK constants
#include "llcontrol.h"
#include "llimagegl.h"

static LLRegisterWidget<LLSlider> r1("slider_bar");
static LLRegisterWidget<LLSlider> r2("volume_slider");


LLSlider::LLSlider( 
	const LLString& name,
	const LLRect& rect,
	void (*on_commit_callback)(LLUICtrl* ctrl, void* userdata),
	void* callback_userdata,
	F32 initial_value,
	F32 min_value,
	F32 max_value,
	F32 increment,
	BOOL volume,
	const LLString& control_name)
	:
	LLUICtrl( name, rect, TRUE,	on_commit_callback, callback_userdata, 
		FOLLOWS_LEFT | FOLLOWS_TOP),
	mValue( initial_value ),
	mInitialValue( initial_value ),
	mMinValue( min_value ),
	mMaxValue( max_value ),
	mIncrement( increment ),
	mVolumeSlider( volume ),
	mMouseOffset( 0 ),
	mTrackColor(		LLUI::sColorsGroup->getColor( "SliderTrackColor" ) ),
	mThumbOutlineColor(	LLUI::sColorsGroup->getColor( "SliderThumbOutlineColor" ) ),
	mThumbCenterColor(	LLUI::sColorsGroup->getColor( "SliderThumbCenterColor" ) ),
	mMouseDownCallback( NULL ),
	mMouseUpCallback( NULL )
{
	mThumbImage = LLUI::sImageProvider->getUIImage("icn_slide-thumb_dark.tga");
	mTrackImage = LLUI::sImageProvider->getUIImage("icn_slide-groove_dark.tga");
	mTrackHighlightImage = LLUI::sImageProvider->getUIImage("icn_slide-highlight.tga");

	// properly handle setting the starting thumb rect
	// do it this way to handle both the operating-on-settings
	// and standalone ways of using this
	setControlName(control_name, NULL);
	setValue(getValueF32());

	updateThumbRect();
	mDragStartThumbRect = mThumbRect;
}


void LLSlider::setValue(F32 value, BOOL from_event)
{
	value = llclamp( value, mMinValue, mMaxValue );

	// Round to nearest increment (bias towards rounding down)
	value -= mMinValue;
	value += mIncrement/2.0001f;
	value -= fmod(value, mIncrement);
	value += mMinValue;

	if (!from_event && mValue != value)
	{
		setControlValue(value);
	}

	mValue = value;
	updateThumbRect();
}

void LLSlider::updateThumbRect()
{
	F32 t = (mValue - mMinValue) / (mMaxValue - mMinValue);

	S32 thumb_width = mThumbImage->getWidth();
	S32 thumb_height = mThumbImage->getHeight();
	S32 left_edge = (thumb_width / 2);
	S32 right_edge = getRect().getWidth() - (thumb_width / 2);

	S32 x = left_edge + S32( t * (right_edge - left_edge) );
	mThumbRect.mLeft = x - (thumb_width / 2);
	mThumbRect.mRight = mThumbRect.mLeft + thumb_width;
	mThumbRect.mBottom = getLocalRect().getCenterY() - (thumb_height / 2);
	mThumbRect.mTop = mThumbRect.mBottom + thumb_height;
}


void LLSlider::setValueAndCommit(F32 value)
{
	F32 old_value = mValue;
	setValue(value);

	if (mValue != old_value)
	{
		onCommit();
	}
}


BOOL LLSlider::handleHover(S32 x, S32 y, MASK mask)
{
	if( hasMouseCapture() )
	{
		S32 thumb_half_width = mThumbImage->getWidth()/2;
		S32 left_edge = thumb_half_width;
		S32 right_edge = getRect().getWidth() - (thumb_half_width);

		x += mMouseOffset;
		x = llclamp( x, left_edge, right_edge );

		F32 t = F32(x - left_edge) / (right_edge - left_edge);
		setValueAndCommit(t * (mMaxValue - mMinValue) + mMinValue );

		getWindow()->setCursor(UI_CURSOR_ARROW);
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (active)" << llendl;		
	}
	else
	{
		getWindow()->setCursor(UI_CURSOR_ARROW);
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (inactive)" << llendl;		
	}
	return TRUE;
}

BOOL LLSlider::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;

	if( hasMouseCapture() )
	{
		gFocusMgr.setMouseCapture( NULL );

		if( mMouseUpCallback )
		{
			mMouseUpCallback( this, mCallbackUserData );
		}
		handled = TRUE;
		make_ui_sound("UISndClickRelease");
	}
	else
	{
		handled = TRUE;
	}

	return handled;
}

BOOL LLSlider::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// only do sticky-focus on non-chrome widgets
	if (!getIsChrome())
	{
		setFocus(TRUE);
	}
	if( mMouseDownCallback )
	{
		mMouseDownCallback( this, mCallbackUserData );
	}

	if (MASK_CONTROL & mask) // if CTRL is modifying
	{
		setValueAndCommit(mInitialValue);
	}
	else
	{
		// Find the offset of the actual mouse location from the center of the thumb.
		if (mThumbRect.pointInRect(x,y))
		{
			mMouseOffset = (mThumbRect.mLeft + mThumbImage->getWidth()/2) - x;
		}
		else
		{
			mMouseOffset = 0;
		}

		// Start dragging the thumb
		// No handler needed for focus lost since this class has no state that depends on it.
		gFocusMgr.setMouseCapture( this );  
		mDragStartThumbRect = mThumbRect;				
	}
	make_ui_sound("UISndClick");

	return TRUE;
}

BOOL LLSlider::handleKeyHere(KEY key, MASK mask)
{
	BOOL handled = FALSE;
	switch(key)
	{
	case KEY_UP:
	case KEY_DOWN:
		// eat up and down keys to be consistent
		handled = TRUE;
		break;
	case KEY_LEFT:
		setValueAndCommit(getValueF32() - getIncrement());
		handled = TRUE;
		break;
	case KEY_RIGHT:
		setValueAndCommit(getValueF32() + getIncrement());
		handled = TRUE;
		break;
	default:
		break;
	}
	return handled;
}

void LLSlider::draw()
{
	// since thumb image might still be decoding, need thumb to accomodate image size
	updateThumbRect();

	// Draw background and thumb.

	// drawing solids requires texturing be disabled
	LLGLSNoTexture no_texture;

	F32 opacity = getEnabled() ? 1.f : 0.3f;
	LLColor4 center_color = (mThumbCenterColor % opacity);
	LLColor4 track_color = (mTrackColor % opacity);

	// Track
	LLRect track_rect(mThumbImage->getWidth() / 2, 
						getLocalRect().getCenterY() + (mTrackImage->getHeight() / 2), 
						getRect().getWidth() - mThumbImage->getWidth() / 2, 
						getLocalRect().getCenterY() - (mTrackImage->getHeight() / 2) );
	LLRect highlight_rect(track_rect.mLeft, track_rect.mTop, mThumbRect.getCenterX(), track_rect.mBottom);
	mTrackImage->draw(track_rect);
	mTrackHighlightImage->draw(highlight_rect);

	// Thumb
	if( hasMouseCapture() )
	{
		// Show ghost where thumb was before dragging began.
		mThumbImage->draw(mDragStartThumbRect, mThumbCenterColor % 0.3f);
	}
	if (hasFocus())
	{
		// Draw focus highlighting.
		mThumbImage->drawBorder(mThumbRect, gFocusMgr.getFocusColor(), gFocusMgr.getFocusFlashWidth());
	}
	// Fill in the thumb.
	mThumbImage->draw(mThumbRect, hasMouseCapture() ? mThumbOutlineColor : center_color);

	LLUICtrl::draw();
}

// virtual
LLXMLNodePtr LLSlider::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	node->createChild("initial_val", TRUE)->setFloatValue(getInitialValue());
	node->createChild("min_val", TRUE)->setFloatValue(getMinValue());
	node->createChild("max_val", TRUE)->setFloatValue(getMaxValue());
	node->createChild("increment", TRUE)->setFloatValue(getIncrement());
	node->createChild("volume", TRUE)->setBoolValue(mVolumeSlider);

	return node;
}


//static
LLView* LLSlider::fromXML(LLXMLNodePtr node, LLView *parent, class LLUICtrlFactory *factory)
{
	LLString name("slider_bar");
	node->getAttributeString("name", name);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	F32 initial_value = 0.f;
	node->getAttributeF32("initial_val", initial_value);

	F32 min_value = 0.f;
	node->getAttributeF32("min_val", min_value);

	F32 max_value = 1.f; 
	node->getAttributeF32("max_val", max_value);

	F32 increment = 0.1f;
	node->getAttributeF32("increment", increment);

	BOOL volume = node->hasName("volume_slider") ? TRUE : FALSE;
	node->getAttributeBOOL("volume", volume);

	LLSlider* slider = new LLSlider(name,
							rect,
							NULL,
							NULL,
							initial_value,
							min_value,
							max_value,
							increment,
							volume);

	slider->initFromXML(node, parent);

	return slider;
}

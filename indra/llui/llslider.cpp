/** 
 * @file llslider.cpp
 * @brief LLSlider base class
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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

const S32 THUMB_WIDTH = 8;
const S32 TRACK_HEIGHT = 6;

LLSlider::LLSlider( 
	const LLString& name,
	const LLRect& rect,
	void (*on_commit_callback)(LLUICtrl* ctrl, void* userdata),
	void* callback_userdata,
	F32 initial_value,
	F32 min_value,
	F32 max_value,
	F32 increment,
	const LLString& control_name)
	:
	LLUICtrl( name, rect, TRUE,	on_commit_callback, callback_userdata, 
		FOLLOWS_LEFT | FOLLOWS_TOP),
	mValue( initial_value ),
	mInitialValue( initial_value ),
	mMinValue( min_value ),
	mMaxValue( max_value ),
	mIncrement( increment ),
	mMouseOffset( 0 ),
	mDragStartThumbRect( 0, mRect.getHeight(), THUMB_WIDTH, 0 ),
	mThumbRect( 0, mRect.getHeight(), THUMB_WIDTH, 0 ),
	mTrackColor(		LLUI::sColorsGroup->getColor( "SliderTrackColor" ) ),
	mThumbOutlineColor(	LLUI::sColorsGroup->getColor( "SliderThumbOutlineColor" ) ),
	mThumbCenterColor(	LLUI::sColorsGroup->getColor( "SliderThumbCenterColor" ) ),
	mDisabledThumbColor(LLUI::sColorsGroup->getColor( "SliderDisabledThumbColor" ) ),
	mMouseDownCallback( NULL ),
	mMouseUpCallback( NULL )
{
	// prperly handle setting the starting thumb rect
	// do it this way to handle both the operating-on-settings
	// and standalone ways of using this
	setControlName(control_name, NULL);
	setValue(getValueF32());
}

EWidgetType LLSlider::getWidgetType() const
{
	return WIDGET_TYPE_SLIDER_BAR;
}

LLString LLSlider::getWidgetTag() const
{
	return LL_SLIDER_TAG;
}

void LLSlider::setValue(F32 value, BOOL from_event)
{
	value = llclamp( value, mMinValue, mMaxValue );

	// Round to nearest increment (bias towards rounding down)
	value -= mMinValue;
	value += mIncrement/2.0001f;
	value -= fmod(value, mIncrement);
	mValue = mMinValue + value;

	if (!from_event)
	{
		setControlValue(mValue);
	}
	
	F32 t = (mValue - mMinValue) / (mMaxValue - mMinValue);

	S32 left_edge = THUMB_WIDTH/2;
	S32 right_edge = mRect.getWidth() - (THUMB_WIDTH/2);

	S32 x = left_edge + S32( t * (right_edge - left_edge) );
	mThumbRect.mLeft = x - (THUMB_WIDTH/2);
	mThumbRect.mRight = x + (THUMB_WIDTH/2);
}

F32 LLSlider::getValueF32() const
{
	return mValue;
}

BOOL LLSlider::handleHover(S32 x, S32 y, MASK mask)
{
	if( hasMouseCapture() )
	{
		S32 left_edge = THUMB_WIDTH/2;
		S32 right_edge = mRect.getWidth() - (THUMB_WIDTH/2);

		x += mMouseOffset;
		x = llclamp( x, left_edge, right_edge );

		F32 t = F32(x - left_edge) / (right_edge - left_edge);
		setValue(t * (mMaxValue - mMinValue) + mMinValue );
		onCommit();

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
		setValue(mInitialValue);
		onCommit();
	}
	else
	{
		// Find the offset of the actual mouse location from the center of the thumb.
		if (mThumbRect.pointInRect(x,y))
		{
			mMouseOffset = (mThumbRect.mLeft + THUMB_WIDTH/2) - x;
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

BOOL	LLSlider::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent)
{
	BOOL handled = FALSE;
	if( getVisible() && mEnabled && !called_from_parent )
	{
		switch(key)
		{
		case KEY_UP:
		case KEY_DOWN:
			// eat up and down keys to be consistent
			handled = TRUE;
			break;
		case KEY_LEFT:
			setValue(getValueF32() - getIncrement());
			onCommit();
			handled = TRUE;
			break;
		case KEY_RIGHT:
			setValue(getValueF32() + getIncrement());
			onCommit();
			handled = TRUE;
			break;
		default:
			break;
		}
	}
	return handled;
}

void LLSlider::draw()
{
	if( getVisible() )
	{
		// Draw background and thumb.

		// drawing solids requires texturing be disabled
		LLGLSNoTexture no_texture;

		LLRect rect(mDragStartThumbRect);

		F32 opacity = mEnabled ? 1.f : 0.3f;

		// Track

		LLUUID thumb_image_id;
		thumb_image_id.set(LLUI::sAssetsGroup->getString("rounded_square.tga"));
		LLImageGL* thumb_imagep = LLUI::sImageProvider->getUIImageByID(thumb_image_id);

		S32 height_offset = (mRect.getHeight() - TRACK_HEIGHT) / 2;
		LLRect track_rect(0, mRect.getHeight() - height_offset, mRect.getWidth(), height_offset );

		track_rect.stretch(-1);
		gl_draw_scaled_image_with_border(track_rect.mLeft, track_rect.mBottom, 16, 16, track_rect.getWidth(), track_rect.getHeight(),
			thumb_imagep, mTrackColor % opacity);
		//gl_rect_2d( track_rect, mThumbOutlineColor % opacity );

		if (!thumb_imagep)
		{
			gl_rect_2d(mThumbRect, mThumbCenterColor, TRUE);
			if (hasMouseCapture())
			{
				gl_rect_2d(mDragStartThumbRect, mThumbCenterColor % opacity, FALSE);
			}
		}
		else if( hasMouseCapture() )
		{
			gl_draw_scaled_image_with_border(mDragStartThumbRect.mLeft, mDragStartThumbRect.mBottom, 16, 16, mDragStartThumbRect.getWidth(), mDragStartThumbRect.getHeight(), 
				thumb_imagep, mThumbCenterColor % 0.3f, TRUE);

			if (hasFocus())
			{
				F32 lerp_amt = gFocusMgr.getFocusFlashAmt();
				LLRect highlight_rect = mThumbRect;
				highlight_rect.stretch(llround(lerp(1.f, 3.f, lerp_amt)));
				gl_draw_scaled_image_with_border(highlight_rect.mLeft, highlight_rect.mBottom, 16, 16, highlight_rect.getWidth(), highlight_rect.getHeight(),
					thumb_imagep, gFocusMgr.getFocusColor());
			}


			gl_draw_scaled_image_with_border(mThumbRect.mLeft, mThumbRect.mBottom, 16, 16, mThumbRect.getWidth(), mThumbRect.getHeight(), 
				thumb_imagep, mThumbOutlineColor, TRUE);

			//// Start Thumb
			//gl_rect_2d( mDragStartThumbRect, mThumbOutlineColor % 0.3f );
			//rect.stretch(-1);
			//gl_rect_2d( rect, mThumbCenterColor % 0.3f );

			//// Thumb
			//gl_rect_2d( mThumbRect, mThumbOutlineColor );
		}
		else
		{ 
			if (hasFocus())
			{
				F32 lerp_amt = gFocusMgr.getFocusFlashAmt();
				LLRect highlight_rect = mThumbRect;
				highlight_rect.stretch(llround(lerp(1.f, 3.f, lerp_amt)));
				gl_draw_scaled_image_with_border(highlight_rect.mLeft, highlight_rect.mBottom, 16, 16, highlight_rect.getWidth(), highlight_rect.getHeight(),
					thumb_imagep, gFocusMgr.getFocusColor());
			}

			gl_draw_scaled_image_with_border(mThumbRect.mLeft, mThumbRect.mBottom, 16, 16, mThumbRect.getWidth(), mThumbRect.getHeight(), 
				thumb_imagep, mThumbCenterColor % opacity, TRUE);
			//rect = mThumbRect;

			//gl_rect_2d( mThumbRect, mThumbOutlineColor % opacity );
			//  
			//rect.stretch(-1);

			//// Thumb
			//gl_rect_2d( rect, mThumbCenterColor % opacity );

		}

		LLUICtrl::draw();
	}
}

// virtual
LLXMLNodePtr LLSlider::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	node->createChild("initial_val", TRUE)->setFloatValue(getInitialValue());
	node->createChild("min_val", TRUE)->setFloatValue(getMinValue());
	node->createChild("max_val", TRUE)->setFloatValue(getMaxValue());
	node->createChild("increment", TRUE)->setFloatValue(getIncrement());

	return node;
}


//static
LLView* LLSlider::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
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


	LLSlider* slider = new LLSlider(name,
							rect,
							NULL,
							NULL,
							initial_value,
							min_value,
							max_value,
							increment);

	slider->initFromXML(node, parent);

	return slider;
}

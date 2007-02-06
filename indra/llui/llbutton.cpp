/** 
 * @file llbutton.cpp
 * @brief LLButton base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llbutton.h"

// Linden library includes
#include "v4color.h"
#include "llstring.h"

// Project includes
#include "llkeyboard.h"
#include "llgl.h"
#include "llui.h"
#include "lluiconstants.h"
//#include "llcallbacklist.h"
#include "llresmgr.h"
#include "llcriticaldamp.h"
#include "llglheaders.h"
#include "llfocusmgr.h"
#include "llwindow.h"

// globals loaded from settings.xml
S32	LLBUTTON_ORIG_H_PAD	= 6; // Pre-zoomable UI
S32	LLBUTTON_H_PAD	= 0;
S32	LLBUTTON_V_PAD	= 0;
S32 BTN_HEIGHT_SMALL= 0;
S32 BTN_HEIGHT		= 0;

S32 BTN_GRID		= 12;
S32 BORDER_SIZE = 1;

// static
LLFrameTimer	LLButton::sFlashingTimer;

LLButton::LLButton(	const LLString& name, const LLRect& rect, const LLString& control_name, void (*click_callback)(void*), void *callback_data)
:	LLUICtrl(name, rect, TRUE, NULL, NULL),
	mClickedCallback( click_callback ),
	mMouseDownCallback( NULL ),
	mMouseUpCallback( NULL ),
	mHeldDownCallback( NULL ),
	mGLFont( NULL ),
	mHeldDownDelay( 0.5f ),			// seconds until held-down callback is called
	mImageUnselected( NULL ),
	mImageSelected( NULL ),
	mImageHoverSelected( NULL ),
	mImageHoverUnselected( NULL ),
	mImageDisabled( NULL ),
	mImageDisabledSelected( NULL ),
	mToggleState( FALSE ),
	mScaleImage( TRUE ),
	mDropShadowedText( TRUE ),
	mBorderEnabled( FALSE ),
	mFlashing( FALSE ),
	mHAlign( LLFontGL::HCENTER ),
	mLeftHPad( LLBUTTON_H_PAD ),
	mRightHPad( LLBUTTON_H_PAD ),
	mFixedWidth( 16 ),
	mFixedHeight( 16 ),
	mHoverGlowStrength(0.15f),
	mCurGlowStrength(0.f),
	mNeedsHighlight(FALSE),
	mCommitOnReturn(TRUE),
	mImagep( NULL )
{
	mUnselectedLabel = name;
	mSelectedLabel = name;

	setImageUnselected("button_enabled_32x128.tga");
	setImageSelected("button_enabled_selected_32x128.tga");
	setImageDisabled("button_disabled_32x128.tga");
	setImageDisabledSelected("button_disabled_32x128.tga");

	mImageColor = LLUI::sColorsGroup->getColor( "ButtonImageColor" );
	mDisabledImageColor = LLUI::sColorsGroup->getColor( "ButtonImageColor" );

	init(click_callback, callback_data, NULL, control_name);
}


LLButton::LLButton(const LLString& name, const LLRect& rect, 
				   const LLString &unselected_image_name, 
				   const LLString &selected_image_name, 
				   const LLString& control_name,
				   void (*click_callback)(void*),
				   void *callback_data,
				   const LLFontGL *font,
				   const LLString& unselected_label, 
				   const LLString& selected_label )
:	LLUICtrl(name, rect, TRUE, NULL, NULL),
	mClickedCallback( click_callback ),
	mMouseDownCallback( NULL ),
	mMouseUpCallback( NULL ),
	mHeldDownCallback( NULL ),
	mGLFont( NULL ),
	mHeldDownDelay( 0.5f ),			// seconds until held-down callback is called
	mImageUnselected( NULL ),
	mImageSelected( NULL ),
	mImageHoverSelected( NULL ),
	mImageHoverUnselected( NULL ),
	mImageDisabled( NULL ),
	mImageDisabledSelected( NULL ),
	mToggleState( FALSE ),
	mScaleImage( TRUE ),
	mDropShadowedText( TRUE ),
	mBorderEnabled( FALSE ),
	mFlashing( FALSE ),
	mHAlign( LLFontGL::HCENTER ),
	mLeftHPad( LLBUTTON_H_PAD ), 
	mRightHPad( LLBUTTON_H_PAD ),
	mFixedWidth( 16 ),
	mFixedHeight( 16 ),
	mHoverGlowStrength(0.25f),
	mCurGlowStrength(0.f),
	mNeedsHighlight(FALSE),
	mCommitOnReturn(TRUE),
	mImagep( NULL )
{
	mUnselectedLabel = unselected_label;
	mSelectedLabel = selected_label;

	// by default, disabled color is same as enabled
	mImageColor = LLUI::sColorsGroup->getColor( "ButtonImageColor" );
	mDisabledImageColor = LLUI::sColorsGroup->getColor( "ButtonImageColor" );

	if( unselected_image_name != "" )
	{
		setImageUnselected(unselected_image_name);
		setImageDisabled(unselected_image_name);
		
		mDisabledImageColor.mV[VALPHA] = 0.5f;
		mImageDisabled = mImageUnselected;
		mDisabledImageColor.mV[VALPHA] = 0.5f;
		// user-specified image - don't use fixed borders unless requested
		mFixedWidth = 0;
		mFixedHeight = 0;
		mScaleImage = FALSE;
	}
	else
	{
		setImageUnselected("button_enabled_32x128.tga");
		setImageDisabled("button_disabled_32x128.tga");
	}

	if( selected_image_name != "" )
	{
		setImageSelected(selected_image_name);
		setImageDisabledSelected(selected_image_name);

		mDisabledImageColor.mV[VALPHA] = 0.5f;
		// user-specified image - don't use fixed borders unless requested
		mFixedWidth = 0;
		mFixedHeight = 0;
		mScaleImage = FALSE;
	}
	else
	{
		setImageSelected("button_enabled_selected_32x128.tga");
		setImageDisabledSelected("button_disabled_32x128.tga");
	}

	init(click_callback, callback_data, font, control_name);
}

void LLButton::init(void (*click_callback)(void*), void *callback_data, const LLFontGL* font, const LLString& control_name)
{
	mGLFont = ( font ? font : LLFontGL::sSansSerif);

	// Hack to make sure there is space for at least one character
	if (mRect.getWidth() - (mRightHPad + mLeftHPad) < mGLFont->getWidth(" "))
	{
		// Use old defaults
		mLeftHPad = LLBUTTON_ORIG_H_PAD;
		mRightHPad = LLBUTTON_ORIG_H_PAD;
	}
	
	mCallbackUserData = callback_data;
	mMouseDownTimer.stop();

	setControlName(control_name, NULL);

	mUnselectedLabelColor = (			LLUI::sColorsGroup->getColor( "ButtonLabelColor" ) );
	mSelectedLabelColor = (			LLUI::sColorsGroup->getColor( "ButtonLabelSelectedColor" ) );
	mDisabledLabelColor = (			LLUI::sColorsGroup->getColor( "ButtonLabelDisabledColor" ) );
	mDisabledSelectedLabelColor = (	LLUI::sColorsGroup->getColor( "ButtonLabelSelectedDisabledColor" ) );
	mHighlightColor = (				LLUI::sColorsGroup->getColor( "ButtonUnselectedFgColor" ) );
	mUnselectedBgColor = (				LLUI::sColorsGroup->getColor( "ButtonUnselectedBgColor" ) );
	mSelectedBgColor = (				LLUI::sColorsGroup->getColor( "ButtonSelectedBgColor" ) );
}

LLButton::~LLButton()
{
 	if( this == gFocusMgr.getMouseCapture() )
	{
		gFocusMgr.setMouseCapture( NULL, NULL );
	}
}

// virtual
EWidgetType LLButton::getWidgetType() const
{
	return WIDGET_TYPE_BUTTON;
}

// virtual
LLString LLButton::getWidgetTag() const
{
	return LL_BUTTON_TAG;
}

// HACK: Committing a button is the same as instantly clicking it.
// virtual
void LLButton::onCommit()
{
	// WARNING: Sometimes clicking a button destroys the floater or
	// panel containing it.  Therefore we need to call mClickedCallback
	// LAST, otherwise this becomes deleted memory.
	LLUICtrl::onCommit();

	if (mMouseDownCallback)
	{
		(*mMouseDownCallback)(mCallbackUserData);
	}
	
	if (mMouseUpCallback)
	{
		(*mMouseUpCallback)(mCallbackUserData);
	}

	if (mSoundFlags & MOUSE_DOWN)
	{
		make_ui_sound("UISndClick");
	}

	if (mSoundFlags & MOUSE_UP)
	{
		make_ui_sound("UISndClickRelease");
	}

	if (mClickedCallback)
	{
		(*mClickedCallback)( mCallbackUserData );
	}
}

BOOL LLButton::handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent)
{
	BOOL handled = FALSE;
	if( getVisible() && mEnabled && !called_from_parent && ' ' == uni_char && !gKeyboard->getKeyRepeated(' '))
	{
		if (mClickedCallback)
		{
			(*mClickedCallback)( mCallbackUserData );
		}
		handled = TRUE;
	}
	return handled;	
}

BOOL LLButton::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent )
{
	BOOL handled = FALSE;
	if( getVisible() && mEnabled && !called_from_parent )
	{
		if( mCommitOnReturn && KEY_RETURN == key && mask == MASK_NONE && !gKeyboard->getKeyRepeated(key))
		{
			if (mClickedCallback)
			{
				(*mClickedCallback)( mCallbackUserData );
			}
			handled = TRUE;
		}
	}
	return handled;
}


BOOL LLButton::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// Route future Mouse messages here preemptively.  (Release on mouse up.)
	gFocusMgr.setMouseCapture( this, &LLButton::onMouseCaptureLost );

	if (hasTabStop() && !getIsChrome())
	{
		setFocus(TRUE);
	}

	if (mMouseDownCallback)
	{
		(*mMouseDownCallback)(mCallbackUserData);
	}

	mMouseDownTimer.start();
	
	if (mSoundFlags & MOUSE_DOWN)
	{
		make_ui_sound("UISndClick");
	}

	return TRUE;
}


BOOL LLButton::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// We only handle the click if the click both started and ended within us
	if( this == gFocusMgr.getMouseCapture() )
	{
		// Regardless of where mouseup occurs, handle callback
		if (mMouseUpCallback)
		{
			(*mMouseUpCallback)(mCallbackUserData);
		}

		mMouseDownTimer.stop();

		// Always release the mouse
		gFocusMgr.setMouseCapture( NULL, NULL );

		// DO THIS AT THE VERY END to allow the button to be destroyed as a result of being clicked.
		// If mouseup in the widget, it's been clicked
		if (pointInView(x, y))
		{
			if (mSoundFlags & MOUSE_UP)
			{
				make_ui_sound("UISndClickRelease");
			}

			if (mClickedCallback)
			{
				(*mClickedCallback)( mCallbackUserData );
			}
		}
	}

	return TRUE;
}


BOOL LLButton::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	LLMouseHandler* other_captor = gFocusMgr.getMouseCapture();
	mNeedsHighlight = other_captor == NULL || 
				other_captor == this ||
				// this following bit is to support modal dialogs
				(other_captor->isView() && hasAncestor((LLView*)other_captor));

	if (mMouseDownTimer.getStarted() && NULL != mHeldDownCallback)
	{
		F32 elapsed = mMouseDownTimer.getElapsedTimeF32();
 		if( mHeldDownDelay < elapsed )
		{
			mHeldDownCallback( mCallbackUserData );		
		}
	}

	// We only handle the click if the click both started and ended within us
	if( this == gFocusMgr.getMouseCapture() )
	{
		handled = TRUE;
	}
	else if( getVisible() )
	{
		// Opaque
		handled = TRUE;
	}

	if( handled )
	{
		getWindow()->setCursor(UI_CURSOR_ARROW);
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << llendl;		
	}

	return handled;
}


// virtual
void LLButton::draw()
{
	if( getVisible() ) 
	{
		BOOL flash = FALSE;
		if( mFlashing )
		{
			F32 elapsed = LLButton::sFlashingTimer.getElapsedTimeF32();
			flash = S32(elapsed * 2) & 1;
		}

		BOOL pressed_by_keyboard = FALSE;
		if (hasFocus())
		{
			pressed_by_keyboard = gKeyboard->getKeyDown(' ') || (mCommitOnReturn && gKeyboard->getKeyDown(KEY_RETURN));
		}

		// Unselected image assignments
		S32 local_mouse_x;
		S32 local_mouse_y;
		LLCoordWindow cursor_pos_window;
		getWindow()->getCursorPosition(&cursor_pos_window);
		LLCoordGL cursor_pos_gl;
		getWindow()->convertCoords(cursor_pos_window, &cursor_pos_gl);
		cursor_pos_gl.mX = llround((F32)cursor_pos_gl.mX / LLUI::sGLScaleFactor.mV[VX]);
		cursor_pos_gl.mY = llround((F32)cursor_pos_gl.mY / LLUI::sGLScaleFactor.mV[VY]);
		screenPointToLocal(cursor_pos_gl.mX, cursor_pos_gl.mY, &local_mouse_x, &local_mouse_y);

		BOOL pressed = pressed_by_keyboard || (this == gFocusMgr.getMouseCapture() && pointInView(local_mouse_x, local_mouse_y));

		BOOL display_state = FALSE;
		if( pressed )
		{
			mImagep = mImageSelected;
			// show the resulting state after releasing the mouse button while it is down
			display_state = mToggleState ? FALSE : TRUE;
		}
		else
		{
			display_state = mToggleState || flash;
		}
		
		BOOL use_glow_effect = FALSE;
		if ( mNeedsHighlight )
		{
			if (display_state)
			{
				if (mImageHoverSelected)
				{
					mImagep = mImageHoverSelected;
				}
				else
				{
					mImagep = mImageSelected;
					use_glow_effect = TRUE;
				}
			}
			else
			{
				if (mImageHoverUnselected)
				{
					mImagep = mImageHoverUnselected;
				}
				else
				{
					mImagep = mImageUnselected;
					use_glow_effect = TRUE;
				}
			}
		}
		else if ( display_state )
		{
			mImagep = mImageSelected;
		}
		else
		{
			mImagep = mImageUnselected;
		}

		// Override if more data is available
		// HACK: Use gray checked state to mean either:
		//   enabled and tentative
		// or
		//   disabled but checked
		if (!mImageDisabledSelected.isNull() && ( (mEnabled && mTentative) || (!mEnabled && display_state ) ) )
		{
			mImagep = mImageDisabledSelected;
		}
		else if (!mImageDisabled.isNull() && !mEnabled && !display_state)
		{
			mImagep = mImageDisabled;
		}

		if (mNeedsHighlight && !mImagep)
		{
			use_glow_effect = TRUE;
		}

		// Figure out appropriate color for the text
		LLColor4 label_color;

		if ( mEnabled )
		{
			if ( !display_state )
			{
				label_color = mUnselectedLabelColor;
			}
			else
			{
				label_color = mSelectedLabelColor;
			}
		}
		else
		{
			if ( !display_state )
			{
				label_color = mDisabledLabelColor;
			}
			else
			{
				label_color = mDisabledSelectedLabelColor;
			}
		}

		// Unselected label assignments
		LLWString label;

		if( display_state )
		{
			if( mEnabled || mDisabledSelectedLabel.empty() )
			{
				label = mSelectedLabel;
			}
			else
			{
				label = mDisabledSelectedLabel;
			}
		}
		else
		{
			if( mEnabled || mDisabledLabel.empty() )
			{
				label = mUnselectedLabel;
			}
			else
			{
				label = mDisabledLabel;
			}
		}
		
		// draw default button border
		if (mEnabled && mBorderEnabled && gFocusMgr.getAppHasFocus()) // because we're the default button in a panel
		{
			drawBorder(LLUI::sColorsGroup->getColor( "ButtonBorderColor" ), BORDER_SIZE);
		}

		// overlay with keyboard focus border
		if (hasFocus())
		{
			F32 lerp_amt = gFocusMgr.getFocusFlashAmt();
			drawBorder(gFocusMgr.getFocusColor(), llround(lerp(1.f, 3.f, lerp_amt)));
		}
		
		if (use_glow_effect)
		{
			mCurGlowStrength = lerp(mCurGlowStrength, mHoverGlowStrength, LLCriticalDamp::getInterpolant(0.05f));
		}
		else
		{
			mCurGlowStrength = lerp(mCurGlowStrength, 0.f, LLCriticalDamp::getInterpolant(0.05f));
		}

		// Draw button image, if available.
		// Otherwise draw basic rectangular button.
		if( mImagep.notNull() && !mScaleImage)
		{
			gl_draw_image( 0, 0, mImagep, mEnabled ? mImageColor : mDisabledImageColor );
			if (mCurGlowStrength > 0.01f)
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				gl_draw_scaled_image_with_border(0, 0, 0, 0, mImagep->getWidth(), mImagep->getHeight(), mImagep, LLColor4(1.f, 1.f, 1.f, mCurGlowStrength), TRUE);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
		}
		else
		if ( mImagep.notNull() && mScaleImage)
		{
			gl_draw_scaled_image_with_border(0, 0, mFixedWidth, mFixedHeight, mRect.getWidth(), mRect.getHeight(), 
											mImagep, mEnabled ? mImageColor : mDisabledImageColor  );
			if (mCurGlowStrength > 0.01f)
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				gl_draw_scaled_image_with_border(0, 0, mFixedWidth, mFixedHeight, mRect.getWidth(), mRect.getHeight(), 
											mImagep, LLColor4(1.f, 1.f, 1.f, mCurGlowStrength), TRUE);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
		}
		else
		{
			// no image
			llwarns << "No image for button " << mName << llendl;
			// draw it in pink so we can find it
			gl_rect_2d(0, mRect.getHeight(), mRect.getWidth(), 0, LLColor4::pink1, FALSE);
		}

		// Draw label
		if( !label.empty() )
		{
			S32 drawable_width = mRect.getWidth() - mLeftHPad - mRightHPad;

			LLWString::trim(label);

			S32 x;
			switch( mHAlign )
			{
			case LLFontGL::RIGHT:
				x = mRect.getWidth() - mRightHPad;
				break;
			case LLFontGL::HCENTER:
				x = mRect.getWidth() / 2;
				break;
			case LLFontGL::LEFT:
			default:
				x = mLeftHPad;
				break;
			}

			S32 y_offset = 2 + (mRect.getHeight() - 20)/2;
		
			if (pressed || display_state)
			{
				y_offset--;
				x++;
			}

			mGLFont->render(label, 0, (F32)x, (F32)(LLBUTTON_V_PAD + y_offset), 
				label_color,
				mHAlign, LLFontGL::BOTTOM,
				mDropShadowedText ? LLFontGL::DROP_SHADOW : LLFontGL::NORMAL,
				U32_MAX, drawable_width,
				NULL, FALSE, FALSE);
		}

		if (sDebugRects	
			|| (LLView::sEditingUI && this == LLView::sEditingUIView))
		{
			drawDebugRect();
		}
	}
	// reset hover status for next frame
	mNeedsHighlight = FALSE;
}

void LLButton::drawBorder(const LLColor4& color, S32 size)
{
	S32 left = -size;
	S32 top = mRect.getHeight() + size;
	S32 right = mRect.getWidth() + size;
	S32 bottom = -size;

	if (mImagep.isNull())
	{
		gl_rect_2d(left, top, right, bottom, color, FALSE);
		return;
	}

	if (mScaleImage)
	{
		gl_draw_scaled_image_with_border(left, bottom, mFixedWidth, mFixedHeight, right-left, top-bottom, 
								mImagep, color, TRUE );
	}
	else
	{
		gl_draw_scaled_image_with_border(left, bottom, 0, 0, mImagep->getWidth() + size * 2, 
								mImagep->getHeight() + size * 2, mImagep, color, TRUE );
	}
}

void LLButton::setClickedCallback(void (*cb)(void*), void* userdata)
{
	mClickedCallback = cb;
	if (userdata)
	{
		mCallbackUserData = userdata;
	}
}


void LLButton::setToggleState(BOOL b)
{
	if( b != mToggleState )
	{
		mToggleState = b;
		LLValueChangedEvent *evt = new LLValueChangedEvent(this, mToggleState);
		fireEvent(evt, "");
	}
}

void LLButton::setValue(const LLSD& value )
{
	mToggleState = value.asBoolean();
}

LLSD LLButton::getValue() const
{
	return mToggleState;
}

void LLButton::setLabel( const LLString& label )
{
	setLabelUnselected(label);
	setLabelSelected(label);
}

//virtual
BOOL LLButton::setLabelArg( const LLString& key, const LLString& text )
{
	mUnselectedLabel.setArg(key, text);
	mSelectedLabel.setArg(key, text);
	return TRUE;
}

void LLButton::setLabelUnselected( const LLString& label )
{
	mUnselectedLabel = label;
}

void LLButton::setLabelSelected( const LLString& label )
{
	mSelectedLabel = label;
}

void LLButton::setDisabledLabel( const LLString& label )
{
	mDisabledLabel = label;
}

void LLButton::setDisabledSelectedLabel( const LLString& label )
{
	mDisabledSelectedLabel = label;
}

void LLButton::setImageUnselectedID( const LLUUID &image_id )
{	
	mImageUnselectedName = "";
	mImageUnselected = LLUI::sImageProvider->getUIImageByID(image_id);
}

void LLButton::setImages( const LLString &image_name, const LLString &selected_name )
{
	setImageUnselected(image_name);
	setImageSelected(selected_name);

}

void LLButton::setImageSelectedID( const LLUUID &image_id )
{
	mImageSelectedName = "";
	mImageSelected = LLUI::sImageProvider->getUIImageByID(image_id);
}

void LLButton::setImageColor(const LLColor4& c)		
{ 
	mImageColor = c; 
}


void LLButton::setImageDisabledID( const LLUUID &image_id )
{
	mImageDisabledName = "";
	mImageDisabled = LLUI::sImageProvider->getUIImageByID(image_id);
	mDisabledImageColor = mImageColor;
	mDisabledImageColor.mV[VALPHA] *= 0.5f;
}

void LLButton::setImageDisabledSelectedID( const LLUUID &image_id )
{	
	mImageDisabledSelectedName = "";
	mImageDisabledSelected = LLUI::sImageProvider->getUIImageByID(image_id);
	mDisabledImageColor = mImageColor;
	mDisabledImageColor.mV[VALPHA] *= 0.5f;
}

void LLButton::setDisabledImages( const LLString &image_name, const LLString &selected_name, const LLColor4& c )
{
	setImageDisabled(image_name);
	setImageDisabledSelected(selected_name);
	mDisabledImageColor = c;
}


void LLButton::setImageHoverSelectedID( const LLUUID& image_id )
{
	mImageHoverSelectedName = "";
	mImageHoverSelected = LLUI::sImageProvider->getUIImageByID(image_id);
}

void LLButton::setDisabledImages( const LLString &image_name, const LLString &selected_name)
{
	LLColor4 clr = mImageColor;
	clr.mV[VALPHA] *= .5f;
	setDisabledImages( image_name, selected_name, clr );
}

void LLButton::setImageHoverUnselectedID( const LLUUID& image_id )
{
	mImageHoverUnselectedName = "";
	mImageHoverUnselected = LLUI::sImageProvider->getUIImageByID(image_id);
}

void LLButton::setHoverImages( const LLString& image_name, const LLString& selected_name )
{
	setImageHoverUnselected(image_name);
	setImageHoverSelected(selected_name);
}

// static
void LLButton::onMouseCaptureLost( LLMouseHandler* old_captor )
{
	LLButton* self = (LLButton*) old_captor;
	self->mMouseDownTimer.stop();
}

//-------------------------------------------------------------------------
// LLSquareButton
//-------------------------------------------------------------------------
LLSquareButton::LLSquareButton(const LLString& name, const LLRect& rect, 
		const LLString& label,
		const LLFontGL *font,
		const LLString& control_name,	
		void (*click_callback)(void*),
		void *callback_data,
		const LLString& selected_label )
:	LLButton(name, rect, "","", 
			control_name, 
			click_callback, callback_data,
			font,
			label, 
			(selected_label.empty() ? label : selected_label) )
{ 
	setImageUnselected("square_btn_32x128.tga");
	//	mImageUnselected = LLUI::sImageProvider->getUIImageByID(LLUUID(LLUI::sAssetsGroup->getString("square_btn_32x128.tga")));
	setImageSelected("square_btn_selected_32x128.tga");
	//	mImageSelectedImage = LLUI::sImageProvider->getUIImageByID(LLUUID(LLUI::sAssetsGroup->getString("square_btn_selected_32x128.tga")));
	setImageDisabled("square_btn_32x128.tga");
	//mDisabledImage = LLUI::sImageProvider->getUIImageByID(LLUUID(LLUI::sAssetsGroup->getString("square_btn_32x128.tga")));
	setImageDisabledSelected("square_btn_selected_32x128.tga");
	//mDisabledSelectedImage = LLUI::sImageProvider->getUIImageByID(LLUUID(LLUI::sAssetsGroup->getString("square_btn_selected_32x128.tga")));
	mImageColor = LLUI::sColorsGroup->getColor("ButtonColor");
}

//-------------------------------------------------------------------------
// Utilities
//-------------------------------------------------------------------------
S32 round_up(S32 grid, S32 value)
{
	S32 mod = value % grid;

	if (mod > 0)
	{
		// not even multiple
		return value + (grid - mod);
	}
	else
	{
		return value;
	}
}

void			LLButton::setImageUnselected(const LLString &image_name)
{	
	setImageUnselectedID(LLUI::findAssetUUIDByName(image_name));
	mImageUnselectedName = image_name;
}

void			LLButton::setImageSelected(const LLString &image_name)
{	
	setImageSelectedID(LLUI::findAssetUUIDByName(image_name));
	mImageSelectedName = image_name;
}

void			LLButton::setImageHoverSelected(const LLString &image_name)
{
	setImageHoverSelectedID(LLUI::findAssetUUIDByName(image_name));
	mImageHoverSelectedName = image_name;
}

void			LLButton::setImageHoverUnselected(const LLString &image_name)
{
	setImageHoverUnselectedID(LLUI::findAssetUUIDByName(image_name));
	mImageHoverUnselectedName = image_name;
}

void			LLButton::setImageDisabled(const LLString &image_name)
{
	setImageDisabledID(LLUI::findAssetUUIDByName(image_name));
	mImageDisabledName = image_name;
}

void			LLButton::setImageDisabledSelected(const LLString &image_name)
{
	setImageDisabledSelectedID(LLUI::findAssetUUIDByName(image_name));
	mImageDisabledSelectedName = image_name;
}

void LLButton::addImageAttributeToXML(LLXMLNodePtr node, 
									  const LLString& image_name,
									  const LLUUID&	image_id,
									  const LLString& xml_tag_name) const
{
	if( !image_name.empty() )
	{
		node->createChild(xml_tag_name, TRUE)->setStringValue(image_name);
	}
	else if( image_id != LLUUID::null )
	{
		node->createChild(xml_tag_name + "_id", TRUE)->setUUIDValue(image_id);
	}
}

// virtual
LLXMLNodePtr LLButton::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	node->createChild("label", TRUE)->setStringValue(getLabelUnselected());
	node->createChild("label_selected", TRUE)->setStringValue(getLabelSelected());
	node->createChild("font", TRUE)->setStringValue(LLFontGL::nameFromFont(mGLFont));
	node->createChild("halign", TRUE)->setStringValue(LLFontGL::nameFromHAlign(mHAlign));
	node->createChild("border_width", TRUE)->setIntValue(mFixedWidth);
	node->createChild("border_height", TRUE)->setIntValue(mFixedHeight);

	addImageAttributeToXML(node,mImageUnselectedName,mImageUnselectedID,"image_unselected");
	addImageAttributeToXML(node,mImageSelectedName,mImageSelectedID,"image_selected");
	addImageAttributeToXML(node,mImageHoverSelectedName,mImageHoverSelectedID,"image_hover_selected");
	addImageAttributeToXML(node,mImageHoverUnselectedName,mImageHoverUnselectedID,"image_hover_unselected");
	addImageAttributeToXML(node,mImageDisabledName,mImageDisabledID,"image_disabled");
	addImageAttributeToXML(node,mImageDisabledSelectedName,mImageDisabledSelectedID,"image_disabled_selected");

	node->createChild("scale_image", TRUE)->setBoolValue(mScaleImage);

	return node;
}

// static
LLView* LLButton::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("button");
	node->getAttributeString("name", name);

	LLString label = name;
	node->getAttributeString("label", label);

	LLString label_selected = label;
	node->getAttributeString("label_selected", label_selected);

	LLFontGL* font = selectFont(node);

	LLString	image_unselected;
	if (node->hasAttribute("image_unselected")) node->getAttributeString("image_unselected",image_unselected);
	
	LLString	image_selected;
	if (node->hasAttribute("image_selected")) node->getAttributeString("image_selected",image_selected);
	
	LLString	image_hover_selected;
	if (node->hasAttribute("image_hover_selected")) node->getAttributeString("image_hover_selected",image_hover_selected);
	
	LLString	image_hover_unselected;
	if (node->hasAttribute("image_hover_unselected")) node->getAttributeString("image_hover_unselected",image_hover_unselected);
	
	LLString	image_disabled_selected;
	if (node->hasAttribute("image_disabled_selected")) node->getAttributeString("image_disabled_selected",image_disabled_selected);
	
	LLString	image_disabled;
	if (node->hasAttribute("image_disabled")) node->getAttributeString("image_disabled",image_disabled);

	LLButton *button = new LLButton(name, 
			LLRect(),
			image_unselected,
			image_selected,
			"", 
			NULL, 
			parent,
			font,
			label,
			label_selected);

	node->getAttributeS32("border_width", button->mFixedWidth);
	node->getAttributeS32("border_height", button->mFixedHeight);

	if(image_hover_selected != LLString::null) button->setImageHoverSelected(image_hover_selected);
	
	if(image_hover_unselected != LLString::null) button->setImageHoverUnselected(image_hover_unselected);
	
	if(image_disabled_selected != LLString::null) button->setImageDisabledSelected(image_disabled_selected );
	
	if(image_disabled != LLString::null) button->setImageDisabled(image_disabled);
	

	if (node->hasAttribute("halign"))
	{
		LLFontGL::HAlign halign = selectFontHAlign(node);
		button->setHAlign(halign);
	}

	if (node->hasAttribute("scale_image"))
	{
		BOOL	needsScale = FALSE;
		node->getAttributeBOOL("scale_image",needsScale);
		button->setScaleImage( needsScale );
	}

	if(label.empty())
	{
		button->setLabelUnselected(node->getTextContents());
	}
	if (label_selected.empty())
	{
		button->setLabelSelected(node->getTextContents());
	}

	button->initFromXML(node, parent);
	
	return button;
}


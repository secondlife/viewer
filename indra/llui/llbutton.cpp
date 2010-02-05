/** 
 * @file llbutton.cpp
 * @brief LLButton base class
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

#define LLBUTTON_CPP
#include "llbutton.h"

// Linden library includes
#include "v4color.h"
#include "llstring.h"

// Project includes
#include "llkeyboard.h"
#include "llui.h"
#include "lluiconstants.h"
#include "llresmgr.h"
#include "llcriticaldamp.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llfocusmgr.h"
#include "llwindow.h"
#include "llnotificationsutil.h"
#include "llrender.h"
#include "lluictrlfactory.h"
#include "llhelp.h"
#include "lldockablefloater.h"

static LLDefaultChildRegistry::Register<LLButton> r("button");

// Compiler optimization, generate extern template
template class LLButton* LLView::getChild<class LLButton>(
	const std::string& name, BOOL recurse) const;

// globals loaded from settings.xml
S32	LLBUTTON_H_PAD	= 0;
S32 BTN_HEIGHT_SMALL= 0;
S32 BTN_HEIGHT		= 0;

LLButton::Params::Params()
:	label_selected("label_selected"),				// requires is_toggle true
	label_shadow("label_shadow", true),
	auto_resize("auto_resize", false),
	use_ellipses("use_ellipses", false),
	image_unselected("image_unselected"),
	image_selected("image_selected"),
	image_hover_selected("image_hover_selected"),
	image_hover_unselected("image_hover_unselected"),
	image_disabled_selected("image_disabled_selected"),
	image_disabled("image_disabled"),
	image_pressed("image_pressed"),
	image_pressed_selected("image_pressed_selected"),
	image_overlay("image_overlay"),
	image_overlay_alignment("image_overlay_alignment", std::string("center")),
	image_left_pad("image_left_pad"),
	image_right_pad("image_right_pad"),
	image_top_pad("image_top_pad"),
	image_bottom_pad("image_bottom_pad"),
	label_color("label_color"),
	label_color_selected("label_color_selected"),	// requires is_toggle true
	label_color_disabled("label_color_disabled"),
	label_color_disabled_selected("label_color_disabled_selected"),
	highlight_color("highlight_color"),
	image_color("image_color"),
	image_color_disabled("image_color_disabled"),
	image_overlay_color("image_overlay_color", LLColor4::white),
	flash_color("flash_color"),
	pad_right("pad_right", LLUI::sSettingGroups["config"]->getS32("ButtonHPad")),
	pad_left("pad_left", LLUI::sSettingGroups["config"]->getS32("ButtonHPad")),
	pad_bottom("pad_bottom"),
	click_callback("click_callback"),
	mouse_down_callback("mouse_down_callback"),
	mouse_up_callback("mouse_up_callback"),
	mouse_held_callback("mouse_held_callback"),
	is_toggle("is_toggle", false),
	scale_image("scale_image", true),
	hover_glow_amount("hover_glow_amount"),
	commit_on_return("commit_on_return", true)
{
	addSynonym(is_toggle, "toggle");
	held_down_delay.seconds = 0.5f;
	initial_value.set(LLSD(false), false);
}


LLButton::LLButton(const LLButton::Params& p)
:	LLUICtrl(p),
	mMouseDownFrame(0),
	mMouseHeldDownCount(0),
	mBorderEnabled( FALSE ),
	mFlashing( FALSE ),
	mCurGlowStrength(0.f),
	mNeedsHighlight(FALSE),
	mUnselectedLabel(p.label()),
	mSelectedLabel(p.label_selected()),
	mGLFont(p.font),
	mHeldDownDelay(p.held_down_delay.seconds),			// seconds until held-down callback is called
	mHeldDownFrameDelay(p.held_down_delay.frames),
	mImageUnselected(p.image_unselected),
	mImageSelected(p.image_selected),
	mImageDisabled(p.image_disabled),
	mImageDisabledSelected(p.image_disabled_selected),
	mImagePressed(p.image_pressed),
	mImagePressedSelected(p.image_pressed_selected),
	mImageHoverSelected(p.image_hover_selected),
	mImageHoverUnselected(p.image_hover_unselected),
	mUnselectedLabelColor(p.label_color()),
	mSelectedLabelColor(p.label_color_selected()),
	mDisabledLabelColor(p.label_color_disabled()),
	mDisabledSelectedLabelColor(p.label_color_disabled_selected()),
	mHighlightColor(p.highlight_color()),
	mImageColor(p.image_color()),
	mFlashBgColor(p.flash_color()),
	mDisabledImageColor(p.image_color_disabled()),
	mImageOverlay(p.image_overlay()),
	mImageOverlayColor(p.image_overlay_color()),
	mImageOverlayAlignment(LLFontGL::hAlignFromName(p.image_overlay_alignment)),
	mImageOverlayLeftPad(p.image_left_pad),
	mImageOverlayRightPad(p.image_right_pad),
	mImageOverlayTopPad(p.image_top_pad),
	mImageOverlayBottomPad(p.image_bottom_pad),
	mIsToggle(p.is_toggle),
	mScaleImage(p.scale_image),
	mDropShadowedText(p.label_shadow),
	mAutoResize(p.auto_resize),
	mUseEllipses( p.use_ellipses ),
	mHAlign(p.font_halign),
	mLeftHPad(p.pad_left),
	mRightHPad(p.pad_right),
	mBottomVPad(p.pad_bottom),
	mHoverGlowStrength(p.hover_glow_amount),
	mCommitOnReturn(p.commit_on_return),
	mFadeWhenDisabled(FALSE),
	mForcePressedState(false),
	mLastDrawCharsCount(0),
	mMouseDownSignal(NULL),
	mMouseUpSignal(NULL),
	mHeldDownSignal(NULL)

{
	static LLUICachedControl<S32> llbutton_orig_h_pad ("UIButtonOrigHPad", 0);
	static Params default_params(LLUICtrlFactory::getDefaultParams<LLButton>());

	if (!p.label_selected.isProvided())
	{
		mSelectedLabel = mUnselectedLabel;
	}

	// Hack to make sure there is space for at least one character
	if (getRect().getWidth() - (mRightHPad + mLeftHPad) < mGLFont->getWidth(std::string(" ")))
	{
		// Use old defaults
		mLeftHPad = llbutton_orig_h_pad;
		mRightHPad = llbutton_orig_h_pad;
	}
	
	mMouseDownTimer.stop();

	// if custom unselected button image provided...
	if (p.image_unselected != default_params.image_unselected)
	{
		//...fade it out for disabled image by default...
		if (p.image_disabled() == default_params.image_disabled() )
		{
			mImageDisabled = p.image_unselected;
			mFadeWhenDisabled = TRUE;
		}

		if (p.image_pressed_selected == default_params.image_pressed_selected)
		{
			mImagePressedSelected = mImageUnselected;
		}
	}

	// if custom selected button image provided...
	if (p.image_selected != default_params.image_selected)
	{
		//...fade it out for disabled image by default...
		if (p.image_disabled_selected() == default_params.image_disabled_selected())
		{
			mImageDisabledSelected = p.image_selected;
			mFadeWhenDisabled = TRUE;
		}

		if (p.image_pressed == default_params.image_pressed)
		{
			mImagePressed = mImageSelected;
		}
	}

	if (!p.image_pressed.isProvided())
	{
		mImagePressed = mImageSelected;
	}

	if (!p.image_pressed_selected.isProvided())
	{
		mImagePressedSelected = mImageUnselected;
	}
	
	if (mImageUnselected.isNull())
	{
		llwarns << "Button: " << getName() << " with no image!" << llendl;
	}
	
	if (p.click_callback.isProvided())
	{
		setCommitCallback(initCommitCallback(p.click_callback)); // alias -> commit_callback
	}
	if (p.mouse_down_callback.isProvided())
	{
		setMouseDownCallback(initCommitCallback(p.mouse_down_callback));
	}
	if (p.mouse_up_callback.isProvided())
	{
		setMouseUpCallback(initCommitCallback(p.mouse_up_callback));
	}
	if (p.mouse_held_callback.isProvided())
	{
		setHeldDownCallback(initCommitCallback(p.mouse_held_callback));
	}
}

LLButton::~LLButton()
{
	delete mMouseDownSignal;
	delete mMouseUpSignal;
	delete mHeldDownSignal;
}

// HACK: Committing a button is the same as instantly clicking it.
// virtual
void LLButton::onCommit()
{
	// WARNING: Sometimes clicking a button destroys the floater or
	// panel containing it.  Therefore we need to call 	LLUICtrl::onCommit()
	// LAST, otherwise this becomes deleted memory.

	if (mMouseDownSignal) (*mMouseDownSignal)(this, LLSD());
	
	if (mMouseUpSignal) (*mMouseUpSignal)(this, LLSD());

	if (getSoundFlags() & MOUSE_DOWN)
	{
		make_ui_sound("UISndClick");
	}

	if (getSoundFlags() & MOUSE_UP)
	{
		make_ui_sound("UISndClickRelease");
	}

	if (mIsToggle)
	{
		toggleState();
	}

	// do this last, as it can result in destroying this button
	LLUICtrl::onCommit();
}

boost::signals2::connection LLButton::setClickedCallback( const commit_signal_t::slot_type& cb )
{
	if (!mCommitSignal) mCommitSignal = new commit_signal_t();
	return mCommitSignal->connect(cb);
}
boost::signals2::connection LLButton::setMouseDownCallback( const commit_signal_t::slot_type& cb )
{
	if (!mMouseDownSignal) mMouseDownSignal = new commit_signal_t();
	return mMouseDownSignal->connect(cb);
}
boost::signals2::connection LLButton::setMouseUpCallback( const commit_signal_t::slot_type& cb )
{
	if (!mMouseUpSignal) mMouseUpSignal = new commit_signal_t();
	return mMouseUpSignal->connect(cb);
}
boost::signals2::connection LLButton::setHeldDownCallback( const commit_signal_t::slot_type& cb )
{
	if (!mHeldDownSignal) mHeldDownSignal = new commit_signal_t();
	return mHeldDownSignal->connect(cb);
}


// *TODO: Deprecate (for backwards compatability only)
boost::signals2::connection LLButton::setClickedCallback( button_callback_t cb, void* data )
{
	return setClickedCallback(boost::bind(cb, data));
}
boost::signals2::connection LLButton::setMouseDownCallback( button_callback_t cb, void* data )
{
	return setMouseDownCallback(boost::bind(cb, data));
}
boost::signals2::connection LLButton::setMouseUpCallback( button_callback_t cb, void* data )
{
	return setMouseUpCallback(boost::bind(cb, data));
}
boost::signals2::connection LLButton::setHeldDownCallback( button_callback_t cb, void* data )
{
	return setHeldDownCallback(boost::bind(cb, data));
}

BOOL LLButton::postBuild()
{
	autoResize();
	return TRUE;
}
BOOL LLButton::handleUnicodeCharHere(llwchar uni_char)
{
	BOOL handled = FALSE;
	if(' ' == uni_char 
		&& !gKeyboard->getKeyRepeated(' '))
	{
		if (mIsToggle)
		{
			toggleState();
		}

		LLUICtrl::onCommit();
		
		handled = TRUE;		
	}
	return handled;	
}

BOOL LLButton::handleKeyHere(KEY key, MASK mask )
{
	BOOL handled = FALSE;
	if( mCommitOnReturn && KEY_RETURN == key && mask == MASK_NONE && !gKeyboard->getKeyRepeated(key))
	{
		if (mIsToggle)
		{
			toggleState();
		}

		handled = TRUE;

		LLUICtrl::onCommit();
	}
	return handled;
}


BOOL LLButton::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (!childrenHandleMouseDown(x, y, mask))
	{
		// Route future Mouse messages here preemptively.  (Release on mouse up.)
		gFocusMgr.setMouseCapture( this );

		if (hasTabStop() && !getIsChrome())
		{
			setFocus(TRUE);
		}

		/*
		 * ATTENTION! This call fires another mouse down callback.
		 * If you wish to remove this call emit that signal directly
		 * by calling LLUICtrl::mMouseDownSignal(x, y, mask);
		 */
		LLUICtrl::handleMouseDown(x, y, mask);

		if(mMouseDownSignal) (*mMouseDownSignal)(this, LLSD());

		mMouseDownTimer.start();
		mMouseDownFrame = (S32) LLFrameTimer::getFrameCount();
		mMouseHeldDownCount = 0;

		
		if (getSoundFlags() & MOUSE_DOWN)
		{
			make_ui_sound("UISndClick");
		}
	}
	return TRUE;
}


BOOL LLButton::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// We only handle the click if the click both started and ended within us
	if( hasMouseCapture() )
	{
		// Always release the mouse
		gFocusMgr.setMouseCapture( NULL );

		/*
		 * ATTENTION! This call fires another mouse up callback.
		 * If you wish to remove this call emit that signal directly
		 * by calling LLUICtrl::mMouseUpSignal(x, y, mask);
		 */
		LLUICtrl::handleMouseUp(x, y, mask);

		// Regardless of where mouseup occurs, handle callback
		if(mMouseUpSignal) (*mMouseUpSignal)(this, LLSD());

		resetMouseDownTimer();

		// DO THIS AT THE VERY END to allow the button to be destroyed as a result of being clicked.
		// If mouseup in the widget, it's been clicked
		if (pointInView(x, y))
		{
			if (getSoundFlags() & MOUSE_UP)
			{
				make_ui_sound("UISndClickRelease");
			}

			if (mIsToggle)
			{
				toggleState();
			}

			LLUICtrl::onCommit();
		}
	}
	else
	{
		childrenHandleMouseUp(x, y, mask);
	}

	return TRUE;
}

BOOL	LLButton::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if (!childrenHandleRightMouseDown(x, y, mask))
	{
		// Route future Mouse messages here preemptively.  (Release on mouse up.)
		gFocusMgr.setMouseCapture( this );

		if (hasTabStop() && !getIsChrome())
		{
			setFocus(TRUE);
		}

//		if (pointInView(x, y))
//		{
//		}
	}
	// send the mouse down signal
	LLUICtrl::handleRightMouseDown(x,y,mask);
	// *TODO: Return result of LLUICtrl call above?  Should defer to base class
	// but this might change the mouse handling of existing buttons in a bad way
	// if they are not mouse opaque.
	return TRUE;
}

BOOL	LLButton::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	// We only handle the click if the click both started and ended within us
	if( hasMouseCapture() )
	{
		// Always release the mouse
		gFocusMgr.setMouseCapture( NULL );

//		if (pointInView(x, y))
//		{
//			mRightMouseUpSignal(this, x,y,mask);
//		}
	}
	else 
	{
		childrenHandleRightMouseUp(x, y, mask);
	}
	// send the mouse up signal
	LLUICtrl::handleRightMouseUp(x,y,mask);
	// *TODO: Return result of LLUICtrl call above?  Should defer to base class
	// but this might change the mouse handling of existing buttons in a bad way.
	// if they are not mouse opaque.
	return TRUE;
}


void LLButton::onMouseEnter(S32 x, S32 y, MASK mask)
{
	LLUICtrl::onMouseEnter(x, y, mask);

	if (isInEnabledChain())
		mNeedsHighlight = TRUE;
}

void LLButton::onMouseLeave(S32 x, S32 y, MASK mask)
{
	LLUICtrl::onMouseLeave(x, y, mask);

	mNeedsHighlight = FALSE;
}

void LLButton::setHighlight(bool b)
{
	mNeedsHighlight = b;
}

BOOL LLButton::handleHover(S32 x, S32 y, MASK mask)
{
	if (!childrenHandleHover(x, y, mask))
	{
		if (mMouseDownTimer.getStarted())
		{
			F32 elapsed = getHeldDownTime();
			if( mHeldDownDelay <= elapsed && mHeldDownFrameDelay <= (S32)LLFrameTimer::getFrameCount() - mMouseDownFrame)
			{
				LLSD param;
				param["count"] = mMouseHeldDownCount++;
				if (mHeldDownSignal) (*mHeldDownSignal)(this, param);
			}
		}

		// We only handle the click if the click both started and ended within us
		getWindow()->setCursor(UI_CURSOR_ARROW);
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << llendl;
	}
	return TRUE;
}


// virtual
void LLButton::draw()
{
	F32 alpha = getDrawContext().mAlpha;
	bool flash = FALSE;
	static LLUICachedControl<F32> button_flash_rate("ButtonFlashRate", 0);
	static LLUICachedControl<S32> button_flash_count("ButtonFlashCount", 0);

	if( mFlashing )
	{
		F32 elapsed = mFlashingTimer.getElapsedTimeF32();
		S32 flash_count = S32(elapsed * button_flash_rate * 2.f);
		// flash on or off?
		flash = (flash_count % 2 == 0) || flash_count > S32((F32)button_flash_count * 2.f);
	}

	bool pressed_by_keyboard = FALSE;
	if (hasFocus())
	{
		pressed_by_keyboard = gKeyboard->getKeyDown(' ') || (mCommitOnReturn && gKeyboard->getKeyDown(KEY_RETURN));
	}

	// Unselected image assignments
	S32 local_mouse_x;
	S32 local_mouse_y;
	LLUI::getMousePositionLocal(this, &local_mouse_x, &local_mouse_y);

	bool enabled = isInEnabledChain();

	bool pressed = pressed_by_keyboard 
					|| (hasMouseCapture() && pointInView(local_mouse_x, local_mouse_y))
					|| mForcePressedState;
	bool selected = getToggleState();
	
	bool use_glow_effect = FALSE;
	LLColor4 glow_color = LLColor4::white;
	LLRender::eBlendType glow_type = LLRender::BT_ADD_WITH_ALPHA;
	LLUIImage* imagep = NULL;
	if (pressed)
	{
		imagep = selected ? mImagePressedSelected : mImagePressed;
	}
	else if ( mNeedsHighlight )
	{
		if (selected)
		{
			if (mImageHoverSelected)
			{
				imagep = mImageHoverSelected;
			}
			else
			{
				imagep = mImageSelected;
				use_glow_effect = TRUE;
			}
		}
		else
		{
			if (mImageHoverUnselected)
			{
				imagep = mImageHoverUnselected;
			}
			else
			{
				imagep = mImageUnselected;
				use_glow_effect = TRUE;
			}
		}
	}
	else 
	{
		imagep = selected ? mImageSelected : mImageUnselected;
	}

	// Override if more data is available
	// HACK: Use gray checked state to mean either:
	//   enabled and tentative
	// or
	//   disabled but checked
	if (!mImageDisabledSelected.isNull() 
		&& 
			( (enabled && getTentative()) 
			|| (!enabled && selected ) ) )
	{
		imagep = mImageDisabledSelected;
	}
	else if (!mImageDisabled.isNull() 
		&& !enabled 
		&& !selected)
	{
		imagep = mImageDisabled;
	}

	if (mFlashing)
	{
		LLColor4 flash_color = mFlashBgColor.get();
		use_glow_effect = TRUE;
		glow_type = LLRender::BT_ALPHA; // blend the glow

		if (mNeedsHighlight) // highlighted AND flashing
			glow_color = (glow_color*0.5f + flash_color*0.5f) % 2.0f; // average between flash and highlight colour, with sum of the opacity
		else
			glow_color = flash_color;
	}

	if (mNeedsHighlight && !imagep)
	{
		use_glow_effect = TRUE;
	}

	// Figure out appropriate color for the text
	LLColor4 label_color;

	// label changes when button state changes, not when pressed
	if ( enabled )
	{
		if ( getToggleState() )
		{
			label_color = mSelectedLabelColor.get();
		}
		else
		{
			label_color = mUnselectedLabelColor.get();
		}
	}
	else
	{
		if ( getToggleState() )
		{
			label_color = mDisabledSelectedLabelColor.get();
		}
		else
		{
			label_color = mDisabledLabelColor.get();
		}
	}

	// Unselected label assignments
	LLWString label;

	if( getToggleState() )
	{
		label = mSelectedLabel;
	}
	else
	{
		label = mUnselectedLabel;
	}

	// overlay with keyboard focus border
	if (hasFocus())
	{
		F32 lerp_amt = gFocusMgr.getFocusFlashAmt();
		drawBorder(imagep, gFocusMgr.getFocusColor() % alpha, llround(lerp(1.f, 3.f, lerp_amt)));
	}
	
	if (use_glow_effect)
	{
		mCurGlowStrength = lerp(mCurGlowStrength,
					mFlashing ? (flash? 1.0 : 0.0)
					: mHoverGlowStrength,
					LLCriticalDamp::getInterpolant(0.05f));
	}
	else
	{
		mCurGlowStrength = lerp(mCurGlowStrength, 0.f, LLCriticalDamp::getInterpolant(0.05f));
	}

	// Draw button image, if available.
	// Otherwise draw basic rectangular button.
	if (imagep != NULL)
	{
		// apply automatic 50% alpha fade to disabled image
		LLColor4 disabled_color = mFadeWhenDisabled ? mDisabledImageColor.get() % 0.5f : mDisabledImageColor.get();
		if ( mScaleImage)
		{
			imagep->draw(getLocalRect(), (enabled ? mImageColor.get() : disabled_color) % alpha  );
			if (mCurGlowStrength > 0.01f)
			{
				gGL.setSceneBlendType(glow_type);
				imagep->drawSolid(0, 0, getRect().getWidth(), getRect().getHeight(), glow_color % (mCurGlowStrength * alpha));
				gGL.setSceneBlendType(LLRender::BT_ALPHA);
			}
		}
		else
		{
			imagep->draw(0, 0, (enabled ? mImageColor.get() : disabled_color) % alpha );
			if (mCurGlowStrength > 0.01f)
			{
				gGL.setSceneBlendType(glow_type);
				imagep->drawSolid(0, 0, glow_color % (mCurGlowStrength * alpha));
				gGL.setSceneBlendType(LLRender::BT_ALPHA);
			}
		}
	}
	else
	{
		// no image
		lldebugs << "No image for button " << getName() << llendl;
		// draw it in pink so we can find it
		gl_rect_2d(0, getRect().getHeight(), getRect().getWidth(), 0, LLColor4::pink1 % alpha, FALSE);
	}

	// let overlay image and text play well together
	S32 text_left = mLeftHPad;
	S32 text_right = getRect().getWidth() - mRightHPad;
	S32 text_width = getRect().getWidth() - mLeftHPad - mRightHPad;

	// draw overlay image
	if (mImageOverlay.notNull())
	{
		// get max width and height (discard level 0)
		S32 overlay_width = mImageOverlay->getWidth();
		S32 overlay_height = mImageOverlay->getHeight();

		F32 scale_factor = llmin((F32)getRect().getWidth() / (F32)overlay_width, (F32)getRect().getHeight() / (F32)overlay_height, 1.f);
		overlay_width = llround((F32)overlay_width * scale_factor);
		overlay_height = llround((F32)overlay_height * scale_factor);

		S32 center_x = getLocalRect().getCenterX();
		S32 center_y = getLocalRect().getCenterY();

		//FUGLY HACK FOR "DEPRESSED" BUTTONS
		if (pressed)
		{
			center_y--;
			center_x++;
		}

		center_y += (mImageOverlayBottomPad - mImageOverlayTopPad);
		// fade out overlay images on disabled buttons
		LLColor4 overlay_color = mImageOverlayColor.get();
		if (!enabled)
		{
			overlay_color.mV[VALPHA] = 0.5f;
		}
		overlay_color.mV[VALPHA] *= alpha;

		switch(mImageOverlayAlignment)
		{
		case LLFontGL::LEFT:
			text_left += overlay_width + 1;
			mImageOverlay->draw(
				mImageOverlayLeftPad,
				center_y - (overlay_height / 2), 
				overlay_width, 
				overlay_height, 
				overlay_color);
			break;
		case LLFontGL::HCENTER:
			mImageOverlay->draw(
				center_x - (overlay_width / 2), 
				center_y - (overlay_height / 2), 
				overlay_width, 
				overlay_height, 
				overlay_color);
			break;
		case LLFontGL::RIGHT:
			text_right -= overlay_width + 1;
			mImageOverlay->draw(
				getRect().getWidth() - mImageOverlayRightPad - overlay_width,
				center_y - (overlay_height / 2), 
				overlay_width, 
				overlay_height, 
				overlay_color);
			break;
		default:
			// draw nothing
			break;
		}
	}

	// Draw label
	if( !label.empty() )
	{
		LLWStringUtil::trim(label);

		S32 x;
		switch( mHAlign )
		{
		case LLFontGL::RIGHT:
			x = text_right;
			break;
		case LLFontGL::HCENTER:
			x = getRect().getWidth() / 2;
			break;
		case LLFontGL::LEFT:
		default:
			x = text_left;
			break;
		}

		S32 y_offset = 2 + (getRect().getHeight() - 20)/2;
	
		if (pressed)
		{
			y_offset--;
			x++;
		}

		// *NOTE: mantipov: before mUseEllipses is implemented in EXT-279 U32_MAX has been passed as
		// max_chars.
		// LLFontGL::render expects S32 max_chars variable but process in a separate way -1 value.
		// Due to U32_MAX is equal to S32 -1 value I have rest this value for non-ellipses mode.
		// Not sure if it is really needed. Probably S32_MAX should be always passed as max_chars.
		mLastDrawCharsCount = mGLFont->render(label, 0,
			(F32)x,
			(F32)(mBottomVPad + y_offset),
			label_color % alpha,
			mHAlign, LLFontGL::BOTTOM,
			LLFontGL::NORMAL,
			mDropShadowedText ? LLFontGL::DROP_SHADOW_SOFT : LLFontGL::NO_SHADOW,
			S32_MAX, text_width,
			NULL, mUseEllipses);
	}

	LLUICtrl::draw();
}

void LLButton::drawBorder(LLUIImage* imagep, const LLColor4& color, S32 size)
{
	if (imagep == NULL) return;
	if (mScaleImage)
	{
		imagep->drawBorder(getLocalRect(), color, size);
	}
	else
	{
		imagep->drawBorder(0, 0, color, size);
	}
}

BOOL LLButton::getToggleState() const
{
    return getValue().asBoolean();
}

void LLButton::setToggleState(BOOL b)
{
	if( b != getToggleState() )
	{
		setControlValue(b); // will fire LLControlVariable callbacks (if any)
		setValue(b);        // may or may not be redundant
		// Unselected label assignments
		autoResize();
	}
}

void LLButton::setFlashing( BOOL b )	
{ 
	if (b != mFlashing)
	{
		mFlashing = b; 
		mFlashingTimer.reset();
	}
}


BOOL LLButton::toggleState()			
{
    bool flipped = ! getToggleState();
	setToggleState(flipped); 

	return flipped; 
}

void LLButton::setLabel( const LLStringExplicit& label )
{
	setLabelUnselected(label);
	setLabelSelected(label);
}

//virtual
BOOL LLButton::setLabelArg( const std::string& key, const LLStringExplicit& text )
{
	mUnselectedLabel.setArg(key, text);
	mSelectedLabel.setArg(key, text);
	return TRUE;
}

void LLButton::setLabelUnselected( const LLStringExplicit& label )
{
	mUnselectedLabel = label;
}

void LLButton::setLabelSelected( const LLStringExplicit& label )
{
	mSelectedLabel = label;
}

void LLButton::setImageUnselected(LLPointer<LLUIImage> image)
{
	mImageUnselected = image;
	if (mImageUnselected.isNull())
	{
		llwarns << "Setting default button image for: " << getName() << " to NULL" << llendl;
	}
}

void LLButton::autoResize()
{
	LLUIString label;
	if(getToggleState())
	{
		label = mSelectedLabel;
	}
	else
	{
		label = mUnselectedLabel;
	}
	resize(label);
}

void LLButton::resize(LLUIString label)
{
	// get label length 
	S32 label_width = mGLFont->getWidth(label.getString());
	// get current btn length 
	S32 btn_width =getRect().getWidth();
    // check if it need resize 
	if (mAutoResize == TRUE)
	{ 
		if (btn_width - (mRightHPad + mLeftHPad) < label_width)
		{
			setRect(LLRect( getRect().mLeft, getRect().mTop, getRect().mLeft + label_width + mLeftHPad + mRightHPad , getRect().mBottom));
		}
	} 
}
void LLButton::setImages( const std::string &image_name, const std::string &selected_name )
{
	setImageUnselected(LLUI::getUIImage(image_name));
	setImageSelected(LLUI::getUIImage(selected_name));
}

void LLButton::setImageSelected(LLPointer<LLUIImage> image)
{
	mImageSelected = image;
}

void LLButton::setImageColor(const LLColor4& c)		
{ 
	mImageColor = c; 
}

void LLButton::setColor(const LLColor4& color)
{
	setImageColor(color);
}

void LLButton::setImageDisabled(LLPointer<LLUIImage> image)
{
	mImageDisabled = image;
	mDisabledImageColor = mImageColor;
	mFadeWhenDisabled = TRUE;
}

void LLButton::setImageDisabledSelected(LLPointer<LLUIImage> image)
{
	mImageDisabledSelected = image;
	mDisabledImageColor = mImageColor;
	mFadeWhenDisabled = TRUE;
}

void LLButton::setImageHoverSelected(LLPointer<LLUIImage> image)
{
	mImageHoverSelected = image;
}

void LLButton::setImageHoverUnselected(LLPointer<LLUIImage> image)
{
	mImageHoverUnselected = image;
}

void LLButton::setImageOverlay(const std::string& image_name, LLFontGL::HAlign alignment, const LLColor4& color)
{
	if (image_name.empty())
	{
		mImageOverlay = NULL;
	}
	else
	{
		mImageOverlay = LLUI::getUIImage(image_name);
		mImageOverlayAlignment = alignment;
		mImageOverlayColor = color;
	}
}

void LLButton::setImageOverlay(const LLUUID& image_id, LLFontGL::HAlign alignment, const LLColor4& color)
{
	if (image_id.isNull())
	{
		mImageOverlay = NULL;
	}
	else
	{
		mImageOverlay = LLUI::getUIImageByID(image_id);
		mImageOverlayAlignment = alignment;
		mImageOverlayColor = color;
	}
}

void LLButton::onMouseCaptureLost()
{
	resetMouseDownTimer();
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

void LLButton::addImageAttributeToXML(LLXMLNodePtr node, 
									  const std::string& image_name,
									  const LLUUID&	image_id,
									  const std::string& xml_tag_name) const
{
	if( !image_name.empty() )
	{
		node->createChild(xml_tag_name.c_str(), TRUE)->setStringValue(image_name);
	}
	else if( image_id != LLUUID::null )
	{
		node->createChild((xml_tag_name + "_id").c_str(), TRUE)->setUUIDValue(image_id);
	}
}


// static
void LLButton::toggleFloaterAndSetToggleState(LLUICtrl* ctrl, const LLSD& sdname)
{
	bool floater_vis = LLFloaterReg::toggleInstance(sdname.asString());
	LLButton* button = dynamic_cast<LLButton*>(ctrl);
	if (button)
		button->setToggleState(floater_vis);
}

// static
// Gets called once
void LLButton::setFloaterToggle(LLUICtrl* ctrl, const LLSD& sdname)
{
	LLButton* button = dynamic_cast<LLButton*>(ctrl);
	if (!button)
		return;
	// Get the visibility control name for the floater
	std::string vis_control_name = LLFloaterReg::declareVisibilityControl(sdname.asString());
	// Set the button control value (toggle state) to the floater visibility control (Sets the value as well)
	button->setControlVariable(LLUI::sSettingGroups["floater"]->getControl(vis_control_name));
	// Set the clicked callback to toggle the floater
	button->setClickedCallback(boost::bind(&LLFloaterReg::toggleFloaterInstance, sdname));
}

// static
void LLButton::setDockableFloaterToggle(LLUICtrl* ctrl, const LLSD& sdname)
{
	LLButton* button = dynamic_cast<LLButton*>(ctrl);
	if (!button)
		return;
	// Get the visibility control name for the floater
	std::string vis_control_name = LLFloaterReg::declareVisibilityControl(sdname.asString());
	// Set the button control value (toggle state) to the floater visibility control (Sets the value as well)
	button->setControlVariable(LLUI::sSettingGroups["floater"]->getControl(vis_control_name));
	// Set the clicked callback to toggle the floater
	button->setClickedCallback(boost::bind(&LLDockableFloater::toggleInstance, sdname));
}

// static
void LLButton::showHelp(LLUICtrl* ctrl, const LLSD& sdname)
{
	// search back through the button's parents for a panel
	// with a help_topic string defined
	std::string help_topic;
	if (LLUI::sHelpImpl &&
	    ctrl->findHelpTopic(help_topic))
	{
		LLUI::sHelpImpl->showTopic(help_topic);
		return; // success
	}

	// display an error if we can't find a help_topic string.
	// fix this by adding a help_topic attribute to the xui file
	LLNotificationsUtil::add("UnableToFindHelpTopic");
}

void LLButton::resetMouseDownTimer()
{
	mMouseDownTimer.stop();
	mMouseDownTimer.reset();
}

/** 
 * @file llcolorswatch.cpp
 * @brief LLColorSwatch class implementation
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

#include "llviewerprecompiledheaders.h"

// File include
#include "llcolorswatch.h"

// Linden library includes
#include "v4color.h"
#include "llwindow.h"	// setCursor()

// Project includes
#include "llui.h"
#include "llrender.h"
#include "lluiconstants.h"
#include "llviewercontrol.h"
#include "llbutton.h"
#include "lltextbox.h"
#include "llfloatercolorpicker.h"
#include "llviewborder.h"
#include "llviewertexturelist.h"
#include "llfocusmgr.h"

static LLDefaultChildRegistry::Register<LLColorSwatchCtrl> r("color_swatch");

LLColorSwatchCtrl::Params::Params()
:	color("color", LLColor4::white),
	can_apply_immediately("can_apply_immediately", false),
	alpha_background_image("alpha_background_image"),
	border_color("border_color"),
    label_width("label_width", -1),
	label_height("label_height", -1),
	caption_text("caption_text"),
	border("border")
{
}

LLColorSwatchCtrl::LLColorSwatchCtrl(const Params& p)
:	LLUICtrl(p),
	mValid( TRUE ),
	mColor(p.color()),
	mCanApplyImmediately(p.can_apply_immediately),
	mAlphaGradientImage(p.alpha_background_image),
	mOnCancelCallback(p.cancel_callback()),
	mOnSelectCallback(p.select_callback()),
	mBorderColor(p.border_color()),
	mLabelWidth(p.label_width),
	mLabelHeight(p.label_height)
{	
	LLTextBox::Params tp = p.caption_text;
	// use custom label height if it is provided
	mLabelHeight = mLabelHeight != -1 ? mLabelHeight : BTN_HEIGHT_SMALL;
	// label_width is specified, not -1
	if(mLabelWidth!= -1)
	{
		tp.rect(LLRect( 0, mLabelHeight, mLabelWidth, 0 ));
	}
	else
	{
		tp.rect(LLRect( 0, mLabelHeight, getRect().getWidth(), 0 ));
	}
	
	tp.initial_value(p.label());
	mCaption = LLUICtrlFactory::create<LLTextBox>(tp);
	addChild( mCaption );

	LLRect border_rect = getLocalRect();
	border_rect.mTop -= 1;
	border_rect.mRight -=1;
	border_rect.mBottom += mLabelHeight;

	LLViewBorder::Params params = p.border;
	params.rect(border_rect);
	mBorder = LLUICtrlFactory::create<LLViewBorder> (params);
	addChild(mBorder);
}

LLColorSwatchCtrl::~LLColorSwatchCtrl ()
{
	// parent dialog is destroyed so we are too and we need to cancel selection
	LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)mPickerHandle.get();
	if (pickerp)
	{
		pickerp->cancelSelection();
		pickerp->closeFloater();
	}
}

BOOL LLColorSwatchCtrl::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	return handleMouseDown(x, y, mask);
}

BOOL LLColorSwatchCtrl::handleHover(S32 x, S32 y, MASK mask)
{
	getWindow()->setCursor(UI_CURSOR_HAND);
	return TRUE;
}

BOOL LLColorSwatchCtrl::handleUnicodeCharHere(llwchar uni_char)
{
	if( ' ' == uni_char )
	{
		showPicker(TRUE);
	}
	return LLUICtrl::handleUnicodeCharHere(uni_char);
}

// forces color of this swatch and any associated floater to the input value, if currently invalid
void LLColorSwatchCtrl::setOriginal(const LLColor4& color)
{
	mColor = color;
	LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)mPickerHandle.get();
	if (pickerp)
	{
		pickerp->setOrigRgb(mColor.mV[VRED], mColor.mV[VGREEN], mColor.mV[VBLUE]);
	}
}

void LLColorSwatchCtrl::set(const LLColor4& color, BOOL update_picker, BOOL from_event)
{
	mColor = color; 
	LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)mPickerHandle.get();
	if (pickerp && update_picker)
	{
		pickerp->setCurRgb(mColor.mV[VRED], mColor.mV[VGREEN], mColor.mV[VBLUE]);
	}
	if (!from_event)
	{
		setControlValue(mColor.getValue());
	}
}

void LLColorSwatchCtrl::setLabel(const std::string& label)
{
	mCaption->setText(label);
}

BOOL LLColorSwatchCtrl::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// Route future Mouse messages here preemptively.  (Release on mouse up.)
	// No handler is needed for capture lost since this object has no state that depends on it.
	gFocusMgr.setMouseCapture( this );

	return TRUE;
}


BOOL LLColorSwatchCtrl::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// We only handle the click if the click both started and ended within us
	if( hasMouseCapture() )
	{
		// Release the mouse
		gFocusMgr.setMouseCapture( NULL );

		// If mouseup in the widget, it's been clicked
		if ( pointInView(x, y) )
		{
			llassert(getEnabled());
			llassert(getVisible());

			// Focus the widget now in order to return the focus
			// after the color picker is closed.
			setFocus(TRUE);

			showPicker(FALSE);
		}
	}

	return TRUE;
}

// assumes GL state is set for 2D
void LLColorSwatchCtrl::draw()
{
	// If we're in a focused floater, don't apply the floater's alpha to the color swatch (STORM-676).
	F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();

	mBorder->setKeyboardFocusHighlight(hasFocus());
	// Draw border
	LLRect border( 0, getRect().getHeight(), getRect().getWidth(), mLabelHeight );
	gl_rect_2d( border, mBorderColor.get(), FALSE );

	LLRect interior = border;
	interior.stretch( -1 );

	// Check state
	if ( mValid )
	{
		if (!mColor.isOpaque())
		{
			// Draw checker board.
			gl_rect_2d_checkerboard(interior, alpha);
		}

		// Draw the color swatch
		gl_rect_2d(interior, mColor % alpha, TRUE);

		if (!mColor.isOpaque())
		{
			// Draw semi-transparent center area in filled with mColor.
			LLColor4 opaque_color = mColor;
			opaque_color.mV[VALPHA] = alpha;
			gGL.color4fv(opaque_color.mV);
			if (mAlphaGradientImage.notNull())
			{
				gGL.pushMatrix();
				{
					mAlphaGradientImage->draw(interior, mColor % alpha);
				}
				gGL.popMatrix();
			}
		}
	}
	else
	{
		if (!mFallbackImageName.empty())
		{
			LLPointer<LLViewerFetchedTexture> fallback_image = LLViewerTextureManager::getFetchedTextureFromFile(mFallbackImageName, TRUE, 
				LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
			if( fallback_image->getComponents() == 4 )
			{	
				gl_rect_2d_checkerboard( interior );
			}	
			gl_draw_scaled_image( interior.mLeft, interior.mBottom, interior.getWidth(), interior.getHeight(), fallback_image, LLColor4::white % alpha);
			fallback_image->addTextureStats( (F32)(interior.getWidth() * interior.getHeight()) );
		}
		else
		{
			// Draw grey and an X
			gl_rect_2d(interior, LLColor4::grey % alpha, TRUE);
			
			gl_draw_x(interior, LLColor4::black % alpha);
		}
	}

	LLUICtrl::draw();
}

void LLColorSwatchCtrl::setEnabled( BOOL enabled )
{
	mCaption->setEnabled( enabled );
	LLView::setEnabled( enabled );

	if (!enabled)
	{
		LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)mPickerHandle.get();
		if (pickerp)
		{
			pickerp->cancelSelection();
			pickerp->closeFloater();
		}
	}
}


void LLColorSwatchCtrl::setValue(const LLSD& value)
{
	set(LLColor4(value), TRUE, TRUE);
}

//////////////////////////////////////////////////////////////////////////////
// called (infrequently) when the color changes so the subject of the swatch can be updated.
void LLColorSwatchCtrl::onColorChanged ( void* data, EColorPickOp pick_op )
{
	LLColorSwatchCtrl* subject = ( LLColorSwatchCtrl* )data;
	if ( subject )
	{
		LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)subject->mPickerHandle.get();
		if (pickerp)
		{
			// move color across from selector to internal widget storage
			LLColor4 updatedColor ( pickerp->getCurR (), 
									pickerp->getCurG (), 
									pickerp->getCurB (), 
									subject->mColor.mV[VALPHA] ); // keep current alpha
			subject->mColor = updatedColor;
			subject->setControlValue(updatedColor.getValue());

			if (pick_op == COLOR_CANCEL && subject->mOnCancelCallback)
			{
				subject->mOnCancelCallback( subject, LLSD());
			}
			else if (pick_op == COLOR_SELECT && subject->mOnSelectCallback)
			{
				subject->mOnSelectCallback( subject, LLSD() );
			}
			else
			{
				// just commit change
				subject->onCommit ();
			}
		}
	}
}

// This is called when the main floatercustomize panel is closed.
// Since this class has pointers up to its parents, we need to cleanup
// this class first in order to avoid a crash.
void LLColorSwatchCtrl::closeFloaterColorPicker()
{
	LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)mPickerHandle.get();
	if (pickerp)
	{
		pickerp->setSwatch(NULL);
		pickerp->closeFloater();
	}

	mPickerHandle.markDead();
}

void LLColorSwatchCtrl::setValid(BOOL valid )
{
	mValid = valid;

	LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)mPickerHandle.get();
	if (pickerp)
	{
		pickerp->setActive(valid);
	}
}

void LLColorSwatchCtrl::showPicker(BOOL take_focus)
{
	LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)mPickerHandle.get();
	if (!pickerp)
	{
		pickerp = new LLFloaterColorPicker(this, mCanApplyImmediately);
		LLFloater* parent = gFloaterView->getParentFloater(this);
		if (parent)
		{
			parent->addDependentFloater(pickerp);
		}
		mPickerHandle = pickerp->getHandle();
	}

	// initialize picker with current color
	pickerp->initUI ( mColor.mV [ VRED ], mColor.mV [ VGREEN ], mColor.mV [ VBLUE ] );

	// display it
	pickerp->showUI ();

	if (take_focus)
	{
		pickerp->setFocus(TRUE);
	}
}


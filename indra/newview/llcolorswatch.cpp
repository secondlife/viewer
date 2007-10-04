/** 
 * @file llcolorswatch.cpp
 * @brief LLColorSwatch class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

// File include
#include "llcolorswatch.h"

// Linden library includes
#include "v4color.h"

// Project includes
#include "llui.h"
#include "lluiconstants.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "llbutton.h"
#include "lltextbox.h"
#include "llfloatercolorpicker.h"
#include "llviewborder.h"
#include "llviewerimagelist.h"
#include "llfocusmgr.h"

LLColorSwatchCtrl::LLColorSwatchCtrl(const std::string& name, const LLRect& rect, const LLColor4& color,
		void (*commit_callback)(LLUICtrl* ctrl, void* userdata),
		void* userdata )
:	LLUICtrl(name, rect, TRUE, commit_callback, userdata, FOLLOWS_LEFT | FOLLOWS_TOP),
	mValid( TRUE ),
	mColor( color ),
	mBorderColor( gColors.getColor("DefaultHighlightLight") ),
	mCanApplyImmediately(FALSE),
	mOnCancelCallback(NULL),
	mOnSelectCallback(NULL)
{
	mCaption = new LLTextBox( name,
		LLRect( 0, BTN_HEIGHT_SMALL, mRect.getWidth(), 0 ),
		LLString::null,
		LLFontGL::sSansSerifSmall );
	mCaption->setFollows( FOLLOWS_LEFT | FOLLOWS_RIGHT | FOLLOWS_BOTTOM );
	addChild( mCaption );

	// Scalable UI made this off-by-one, I don't know why. JC
	LLRect border_rect(0, mRect.getHeight()-1, mRect.getWidth()-1, 0);
	border_rect.mBottom += BTN_HEIGHT_SMALL;
	mBorder = new LLViewBorder("border", border_rect, LLViewBorder::BEVEL_IN);
	addChild(mBorder);

	mAlphaGradientImage = gImageList.getImageFromUUID(LLUUID(gViewerArt.getString("color_swatch_alpha.tga")),
													  MIPMAP_FALSE, TRUE, GL_ALPHA8, GL_ALPHA);
}

LLColorSwatchCtrl::LLColorSwatchCtrl(const std::string& name, const LLRect& rect, const std::string& label, const LLColor4& color,
		void (*commit_callback)(LLUICtrl* ctrl, void* userdata),
		void* userdata )
:	LLUICtrl(name, rect, TRUE, commit_callback, userdata, FOLLOWS_LEFT | FOLLOWS_TOP),
	mValid( TRUE ),
	mColor( color ),
	mBorderColor( gColors.getColor("DefaultHighlightLight") ),
	mCanApplyImmediately(FALSE),
	mOnCancelCallback(NULL),
	mOnSelectCallback(NULL)
{
	mCaption = new LLTextBox( label,
		LLRect( 0, BTN_HEIGHT_SMALL, mRect.getWidth(), 0 ),
		LLString::null,
		LLFontGL::sSansSerifSmall );
	mCaption->setFollows( FOLLOWS_LEFT | FOLLOWS_RIGHT | FOLLOWS_BOTTOM );
	addChild( mCaption );

	// Scalable UI made this off-by-one, I don't know why. JC
	LLRect border_rect(0, mRect.getHeight()-1, mRect.getWidth()-1, 0);
	border_rect.mBottom += BTN_HEIGHT_SMALL;
	mBorder = new LLViewBorder("border", border_rect, LLViewBorder::BEVEL_IN);
	addChild(mBorder);

	mAlphaGradientImage = gImageList.getImageFromUUID(LLUUID(gViewerArt.getString("color_swatch_alpha.tga")),
													  MIPMAP_FALSE, TRUE, GL_ALPHA8, GL_ALPHA);
}

LLColorSwatchCtrl::~LLColorSwatchCtrl ()
{
	// parent dialog is destroyed so we are too and we need to cancel selection
	LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)LLFloater::getFloaterByHandle(mPickerHandle);
	if (pickerp)
	{
		pickerp->cancelSelection();
		pickerp->close();
	}
	mAlphaGradientImage = NULL;
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

BOOL LLColorSwatchCtrl::handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent)
{
	if( getVisible() && mEnabled && !called_from_parent && ' ' == uni_char )
	{
		showPicker(TRUE);
	}
	return LLUICtrl::handleUnicodeCharHere(uni_char, called_from_parent);
}

// forces color of this swatch and any associated floater to the input value, if currently invalid
void LLColorSwatchCtrl::setOriginal(const LLColor4& color)
{
	mColor = color;
	LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)LLFloater::getFloaterByHandle(mPickerHandle);
	if (pickerp)
	{
		pickerp->setOrigRgb(mColor.mV[VRED], mColor.mV[VGREEN], mColor.mV[VBLUE]);
	}
}

void LLColorSwatchCtrl::set(const LLColor4& color, BOOL update_picker, BOOL from_event)
{
	mColor = color; 
	LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)LLFloater::getFloaterByHandle(mPickerHandle);
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
	gViewerWindow->setMouseCapture( this );

	return TRUE;
}


BOOL LLColorSwatchCtrl::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// We only handle the click if the click both started and ended within us
	if( hasMouseCapture() )
	{
		// Release the mouse
		gViewerWindow->setMouseCapture( NULL );

		// If mouseup in the widget, it's been clicked
		if ( pointInView(x, y) )
		{
			llassert(mEnabled);
			llassert(getVisible());

			showPicker(FALSE);
		}
	}

	return TRUE;
}


// assumes GL state is set for 2D
void LLColorSwatchCtrl::draw()
{
	if( getVisible() )
	{
		mBorder->setKeyboardFocusHighlight(hasFocus());
		// Draw border
		LLRect border( 0, mRect.getHeight(), mRect.getWidth(), BTN_HEIGHT_SMALL );
		gl_rect_2d( border, mBorderColor, FALSE );

		LLRect interior = border;
		interior.stretch( -1 );

		// Check state
		if ( mValid )
		{
			LLGLSTexture gls_texture;

			// Draw the color swatch
			gl_rect_2d_checkerboard( interior );
			gl_rect_2d(interior, mColor, TRUE);
			LLColor4 opaque_color = mColor;
			opaque_color.mV[VALPHA] = 1.f;
			glColor4fv(opaque_color.mV);
			if (mAlphaGradientImage.notNull())
			{
				glPushMatrix();
				{
					glTranslatef((F32)interior.mLeft, (F32)interior.mBottom, 0.f);
					LLViewerImage::bindTexture(mAlphaGradientImage);
					gl_rect_2d_simple_tex(interior.getWidth(), interior.getHeight());
				}
				glPopMatrix();
			}
		}
		else
		{
			// Draw grey and an X
			gl_rect_2d(interior, LLColor4::grey, TRUE);

			gl_draw_x(interior, LLColor4::black);
		}

		LLUICtrl::draw();
	}
}

void LLColorSwatchCtrl::setEnabled( BOOL enabled )
{
	mCaption->setEnabled( enabled );
	LLView::setEnabled( enabled );

	if (!enabled)
	{
		LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)LLFloater::getFloaterByHandle(mPickerHandle);
		if (pickerp)
		{
			pickerp->cancelSelection();
			pickerp->close();
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
		LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)LLFloater::getFloaterByHandle(subject->mPickerHandle);
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
				subject->mOnCancelCallback(subject, subject->mCallbackUserData);
			}
			else if (pick_op == COLOR_SELECT && subject->mOnSelectCallback)
			{
				subject->mOnSelectCallback(subject, subject->mCallbackUserData);
			}
			else
			{
				// just commit change
				subject->onCommit ();
			}
		}
	};
}

void LLColorSwatchCtrl::setValid(BOOL valid )
{
	mValid = valid;

	LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)LLFloater::getFloaterByHandle(mPickerHandle);
	if (pickerp)
	{
		pickerp->setActive(valid);
	}
}

void LLColorSwatchCtrl::showPicker(BOOL take_focus)
{
	LLFloaterColorPicker* pickerp = (LLFloaterColorPicker*)LLFloater::getFloaterByHandle(mPickerHandle);
	if (!pickerp)
	{
		pickerp = new LLFloaterColorPicker(this, mCanApplyImmediately);
		gFloaterView->getParentFloater(this)->addDependentFloater(pickerp);
		mPickerHandle = pickerp->getHandle();
	}

	// initialize picker singleton with current color
	pickerp->initUI ( mColor.mV [ VRED ], mColor.mV [ VGREEN ], mColor.mV [ VBLUE ] );

	// display it
	pickerp->showUI ();

	if (take_focus)
	{
		pickerp->setFocus(TRUE);
	}
}

// virtual
LLXMLNodePtr LLColorSwatchCtrl::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	node->createChild("color", TRUE)->setFloatValue(4, mColor.mV);

	node->createChild("border_color", TRUE)->setFloatValue(4, mBorderColor.mV);

	if (mCaption)
	{
		node->createChild("label", TRUE)->setStringValue(mCaption->getText());
	}

	node->createChild("can_apply_immediately", TRUE)->setBoolValue(mCanApplyImmediately);

	return node;
}

LLView* LLColorSwatchCtrl::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("colorswatch");
	node->getAttributeString("name", name);

	LLString label;
	node->getAttributeString("label", label);

	LLColor4 color(1.f, 1.f, 1.f, 1.f);
	node->getAttributeColor("initial_color", color);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	BOOL can_apply_immediately = FALSE;
	node->getAttributeBOOL("can_apply_immediately", can_apply_immediately);

	LLUICtrlCallback callback = NULL;

	if (label.empty())
	{
		label.assign(node->getValue());
	}

	LLColorSwatchCtrl* color_swatch = new LLColorSwatchCtrl(
		name, 
		rect,
		label,
		color,
		callback,
		NULL );

	color_swatch->setCanApplyImmediately(can_apply_immediately);
	color_swatch->initFromXML(node, parent);

	return color_swatch;
}

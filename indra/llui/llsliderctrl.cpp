/** 
 * @file llsliderctrl.cpp
 * @brief LLSliderCtrl base class
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

#include "llsliderctrl.h"

#include "audioengine.h"
#include "sound_ids.h"

#include "llmath.h"
#include "llfontgl.h"
#include "llgl.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llslider.h"
#include "llstring.h"
#include "lltextbox.h"
#include "llui.h"
#include "lluiconstants.h"
#include "llcontrol.h"
#include "llfocusmgr.h"
#include "llresmgr.h"

const U32 MAX_STRING_LENGTH = 10;

 
LLSliderCtrl::LLSliderCtrl(const LLString& name, const LLRect& rect, 
						   const LLString& label,
						   const LLFontGL* font,
						   S32 label_width,
						   S32 text_left,
						   BOOL show_text,
						   BOOL can_edit_text,
						   BOOL volume,
						   void (*commit_callback)(LLUICtrl*, void*),
						   void* callback_user_data,
						   F32 initial_value, F32 min_value, F32 max_value, F32 increment,
						   const LLString& control_which)
	: LLUICtrl(name, rect, TRUE, commit_callback, callback_user_data ),
	  mFont(font),
	  mShowText( show_text ),
	  mCanEditText( can_edit_text ),
	  mVolumeSlider( volume ),
	  mPrecision( 3 ),
	  mLabelBox( NULL ),
	  mLabelWidth( label_width ),
	  mValue( initial_value ),
	  mEditor( NULL ),
	  mTextBox( NULL ),
	  mTextEnabledColor( LLUI::sColorsGroup->getColor( "LabelTextColor" ) ),
	  mTextDisabledColor( LLUI::sColorsGroup->getColor( "LabelDisabledColor" ) ),
	  mSliderMouseUpCallback( NULL ),
	  mSliderMouseDownCallback( NULL )
{
	S32 top = mRect.getHeight();
	S32 bottom = 0;
	S32 left = 0;

	// Label
	if( !label.empty() )
	{
		if (label_width == 0)
		{
			label_width = font->getWidth(label);
		}
		LLRect label_rect( left, top, label_width, bottom );
		mLabelBox = new LLTextBox( "SliderCtrl Label", label_rect, label.c_str(), font );
		addChild(mLabelBox);
	}

	S32 slider_right = mRect.getWidth();
	if( show_text )
	{
		slider_right = text_left - SLIDERCTRL_SPACING;
	}

	S32 slider_left = label_width ? label_width + SLIDERCTRL_SPACING : 0;
	LLRect slider_rect( slider_left, top, slider_right, bottom );
	mSlider = new LLSlider( 
		"slider",
		slider_rect, 
		LLSliderCtrl::onSliderCommit, this, 
		initial_value, min_value, max_value, increment, volume,
		control_which );
	addChild( mSlider );
	
	if( show_text )
	{
		LLRect text_rect( text_left, top, mRect.getWidth(), bottom );
		if( can_edit_text )
		{
			mEditor = new LLLineEditor( "SliderCtrl Editor", text_rect,
				"", font,
				MAX_STRING_LENGTH,
				&LLSliderCtrl::onEditorCommit, NULL, NULL, this,
				&LLLineEditor::prevalidateFloat );
			mEditor->setFollowsLeft();
			mEditor->setFollowsBottom();
			mEditor->setFocusReceivedCallback( &LLSliderCtrl::onEditorGainFocus );
			mEditor->setIgnoreTab(TRUE);
			// don't do this, as selecting the entire text is single clicking in some cases
			// and double clicking in others
			//mEditor->setSelectAllonFocusReceived(TRUE);
			addChild(mEditor);
		}
		else
		{
			mTextBox = new LLTextBox( "SliderCtrl Text", text_rect,	"",	font);
			mTextBox->setFollowsLeft();
			mTextBox->setFollowsBottom();
			addChild(mTextBox);
		}
	}

	updateText();
}

LLSliderCtrl::~LLSliderCtrl()
{
	// Children all cleaned up by default view destructor.
}

// static
void LLSliderCtrl::onEditorGainFocus( LLUICtrl* caller, void *userdata )
{
	LLSliderCtrl* self = (LLSliderCtrl*) userdata;
	llassert( caller == self->mEditor );

	self->onFocusReceived();
}

F32 LLSliderCtrl::getValueF32() const
{
	return mSlider->getValueF32();
}

void LLSliderCtrl::setValue(F32 v, BOOL from_event)
{
	mSlider->setValue( v, from_event );
	mValue = mSlider->getValueF32();
	updateText();
}

BOOL LLSliderCtrl::setLabelArg( const LLString& key, const LLStringExplicit& text )
{
	BOOL res = FALSE;
	if (mLabelBox)
	{
		res = mLabelBox->setTextArg(key, text);
		if (res && mLabelWidth == 0)
		{
			S32 label_width = mFont->getWidth(mLabelBox->getText());
			LLRect rect = mLabelBox->getRect();
			S32 prev_right = rect.mRight;
			rect.mRight = rect.mLeft + label_width;
			mLabelBox->setRect(rect);
				
			S32 delta = rect.mRight - prev_right;
			rect = mSlider->getRect();
			S32 left = rect.mLeft + delta;
			left = llclamp(left, 0, rect.mRight-SLIDERCTRL_SPACING);
			rect.mLeft = left;
			mSlider->setRect(rect);
		}
	}
	return res;
}

void LLSliderCtrl::clear()
{
	setValue(0.0f);
	if( mEditor )
	{
		mEditor->setText( LLString::null );
	}
	if( mTextBox )
	{
		mTextBox->setText( LLString::null );
	}

}

BOOL LLSliderCtrl::isMouseHeldDown()
{
	return mSlider->hasMouseCapture();
}

void LLSliderCtrl::updateText()
{
	if( mEditor || mTextBox )
	{
		LLLocale locale(LLLocale::USER_LOCALE);

		// Don't display very small negative values as -0.000
		F32 displayed_value = (F32)(floor(getValueF32() * pow(10.0, (F64)mPrecision) + 0.5) / pow(10.0, (F64)mPrecision));

		LLString format = llformat("%%.%df", mPrecision);
		LLString text = llformat(format.c_str(), displayed_value);
		if( mEditor )
		{
			mEditor->setText( text );
		}
		else
		{
			mTextBox->setText( text );
		}
	}
}

// static
void LLSliderCtrl::onEditorCommit( LLUICtrl* caller, void *userdata )
{
	LLSliderCtrl* self = (LLSliderCtrl*) userdata;
	llassert( caller == self->mEditor );

	BOOL success = FALSE;
	F32 val = self->mValue;
	F32 saved_val = self->mValue;

	LLString text = self->mEditor->getText();
	if( LLLineEditor::postvalidateFloat( text ) )
	{
		LLLocale locale(LLLocale::USER_LOCALE);
		val = (F32) atof( text.c_str() );
		if( self->mSlider->getMinValue() <= val && val <= self->mSlider->getMaxValue() )
		{
			if( self->mValidateCallback )
			{
				self->setValue( val );  // set the value temporarily so that the callback can retrieve it.
				if( self->mValidateCallback( self, self->mCallbackUserData ) )
				{
					success = TRUE;
				}
			}
			else
			{
				self->setValue( val );
				success = TRUE;
			}
		}
	}

	if( success )
	{
		self->onCommit();
	}
	else
	{
		if( self->getValueF32() != saved_val )
		{
			self->setValue( saved_val );
		}
		self->reportInvalidData();		
	}
	self->updateText();
}

// static
void LLSliderCtrl::onSliderCommit( LLUICtrl* caller, void *userdata )
{
	LLSliderCtrl* self = (LLSliderCtrl*) userdata;
	llassert( caller == self->mSlider );

	BOOL success = FALSE;
	F32 saved_val = self->mValue;
	F32 new_val = self->mSlider->getValueF32();

	if( self->mValidateCallback )
	{
		self->mValue = new_val;  // set the value temporarily so that the callback can retrieve it.
		if( self->mValidateCallback( self, self->mCallbackUserData ) )
		{
			success = TRUE;
		}
	}
	else
	{
		self->mValue = new_val;
		success = TRUE;
	}

	if( success )
	{
		self->onCommit();
	}
	else
	{
		if( self->mValue != saved_val )
		{
			self->setValue( saved_val );
		}
		self->reportInvalidData();		
	}
	self->updateText();
}

void LLSliderCtrl::setEnabled(BOOL b)
{
	LLView::setEnabled( b );

	if( mLabelBox )
	{
		mLabelBox->setColor( b ? mTextEnabledColor : mTextDisabledColor );
	}

	mSlider->setEnabled( b );

	if( mEditor )
	{
		mEditor->setEnabled( b );
	}

	if( mTextBox )
	{
		mTextBox->setColor( b ? mTextEnabledColor : mTextDisabledColor );
	}
}


void LLSliderCtrl::setTentative(BOOL b)
{
	if( mEditor )
	{
		mEditor->setTentative(b);
	}
	LLUICtrl::setTentative(b);
}


void LLSliderCtrl::onCommit()
{
	setTentative(FALSE);

	if( mEditor )
	{
		mEditor->setTentative(FALSE);
	}

	LLUICtrl::onCommit();
}


void LLSliderCtrl::setPrecision(S32 precision)
{
	if (precision < 0 || precision > 10)
	{
		llerrs << "LLSliderCtrl::setPrecision - precision out of range" << llendl;
		return;
	}

	mPrecision = precision;
	updateText();
}

void LLSliderCtrl::setSliderMouseDownCallback( void (*slider_mousedown_callback)(LLUICtrl* caller, void* userdata) )
{
	mSliderMouseDownCallback = slider_mousedown_callback;
	mSlider->setMouseDownCallback( LLSliderCtrl::onSliderMouseDown );
}

// static
void LLSliderCtrl::onSliderMouseDown(LLUICtrl* caller, void* userdata)
{
	LLSliderCtrl* self = (LLSliderCtrl*) userdata;
	if( self->mSliderMouseDownCallback )
	{
		self->mSliderMouseDownCallback( self, self->mCallbackUserData );
	}
}


void LLSliderCtrl::setSliderMouseUpCallback( void (*slider_mouseup_callback)(LLUICtrl* caller, void* userdata) )
{
	mSliderMouseUpCallback = slider_mouseup_callback;
	mSlider->setMouseUpCallback( LLSliderCtrl::onSliderMouseUp );
}

// static
void LLSliderCtrl::onSliderMouseUp(LLUICtrl* caller, void* userdata)
{
	LLSliderCtrl* self = (LLSliderCtrl*) userdata;
	if( self->mSliderMouseUpCallback )
	{
		self->mSliderMouseUpCallback( self, self->mCallbackUserData );
	}
}

void LLSliderCtrl::onTabInto()
{
	if( mEditor )
	{
		mEditor->onTabInto(); 
	}
}

void LLSliderCtrl::reportInvalidData()
{
	make_ui_sound("UISndBadKeystroke");
}

//virtual
LLString LLSliderCtrl::getControlName() const
{
	return mSlider->getControlName();
}

// virtual
void LLSliderCtrl::setControlName(const LLString& control_name, LLView* context)
{
	mSlider->setControlName(control_name, context);
}

// virtual
LLXMLNodePtr LLSliderCtrl::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	node->createChild("show_text", TRUE)->setBoolValue(mShowText);

	node->createChild("can_edit_text", TRUE)->setBoolValue(mCanEditText);

	node->createChild("volume", TRUE)->setBoolValue(mVolumeSlider);
	
	node->createChild("decimal_digits", TRUE)->setIntValue(mPrecision);

	if (mLabelBox)
	{
		node->createChild("label", TRUE)->setStringValue(mLabelBox->getText());
	}

	// TomY TODO: Do we really want to export the transient state of the slider?
	node->createChild("value", TRUE)->setFloatValue(mValue);

	if (mSlider)
	{
		node->createChild("initial_val", TRUE)->setFloatValue(mSlider->getInitialValue());
		node->createChild("min_val", TRUE)->setFloatValue(mSlider->getMinValue());
		node->createChild("max_val", TRUE)->setFloatValue(mSlider->getMaxValue());
		node->createChild("increment", TRUE)->setFloatValue(mSlider->getIncrement());
	}
	addColorXML(node, mTextEnabledColor, "text_enabled_color", "LabelTextColor");
	addColorXML(node, mTextDisabledColor, "text_disabled_color", "LabelDisabledColor");

	return node;
}

LLView* LLSliderCtrl::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("slider");
	node->getAttributeString("name", name);

	LLString label;
	node->getAttributeString("label", label);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	LLFontGL* font = LLView::selectFont(node);

	// HACK: Font might not be specified.
	if (!font)
	{
		font = LLFontGL::sSansSerifSmall;
	}

	S32 label_width = 0;
	node->getAttributeS32("label_width", label_width);

	BOOL show_text = TRUE;
	node->getAttributeBOOL("show_text", show_text);

	BOOL can_edit_text = FALSE;
	node->getAttributeBOOL("can_edit_text", can_edit_text);

	BOOL volume = FALSE;
	node->getAttributeBOOL("volume", volume);

	F32 initial_value = 0.f;
	node->getAttributeF32("initial_val", initial_value);

	F32 min_value = 0.f;
	node->getAttributeF32("min_val", min_value);

	F32 max_value = 1.f; 
	node->getAttributeF32("max_val", max_value);

	F32 increment = 0.1f;
	node->getAttributeF32("increment", increment);

	U32 precision = 3;
	node->getAttributeU32("decimal_digits", precision);

	S32 text_left = 0;
	if (show_text)
	{
		// calculate the size of the text box (log max_value is number of digits - 1 so plus 1)
		if ( max_value )
			text_left = font->getWidth("0") * ( static_cast < S32 > ( log10  ( max_value ) ) + precision + 1 );

		if ( increment < 1.0f )
			text_left += font->getWidth(".");	// (mostly) take account of decimal point in value

		if ( min_value < 0.0f || max_value < 0.0f )
			text_left += font->getWidth("-");	// (mostly) take account of minus sign 

		// padding to make things look nicer
		text_left += 8;
	}

	LLUICtrlCallback callback = NULL;

	if (label.empty())
	{
		label.assign(node->getTextContents());
	}

	LLSliderCtrl* slider = new LLSliderCtrl(name,
							rect,
							label,
							font,
							label_width,
							rect.getWidth() - text_left,
							show_text,
							can_edit_text,
							volume,
							callback,
							NULL,
							initial_value, 
							min_value, 
							max_value, 
							increment);

	slider->setPrecision(precision);

	slider->initFromXML(node, parent);

	slider->updateText();
	
	return slider;
}

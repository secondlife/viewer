/** 
 * @file llsliderctrl.cpp
 * @brief LLSliderCtrl base class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llsliderctrl.h"

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
#include "lluictrlfactory.h"

const U32 MAX_STRING_LENGTH = 10;

static LLDefaultChildRegistry::Register<LLSliderCtrl> r("slider");

LLSliderCtrl::LLSliderCtrl(const LLSliderCtrl::Params& p)
:	LLF32UICtrl(p),
	mLabelBox( NULL ),
	mEditor( NULL ),
	mTextBox( NULL ),
	mFont(p.font),
	mShowText(p.show_text),
	mCanEditText(p.can_edit_text),
	mPrecision(p.decimal_digits),
	mTextEnabledColor(p.text_color()),
	mTextDisabledColor(p.text_disabled_color()),
	mLabelWidth(p.label_width)
{
	S32 top = getRect().getHeight();
	S32 bottom = 0;
	S32 left = 0;

	S32 label_width = p.label_width;
	S32 text_width = p.text_width;

	// Label
	if( !p.label().empty() )
	{
		if (!p.label_width.isProvided())
		{
			label_width = p.font()->getWidth(p.label);
		}
		LLRect label_rect( left, top, label_width, bottom );
		LLTextBox::Params params(p.slider_label);
		params.rect.setIfNotProvided(label_rect);
		params.font.setIfNotProvided(p.font);
		params.initial_value(p.label());
		mLabelBox = LLUICtrlFactory::create<LLTextBox> (params);
		addChild(mLabelBox);
		mLabelFont = params.font();
	}

	if (p.show_text && !p.text_width.isProvided())
	{
		// calculate the size of the text box (log max_value is number of digits - 1 so plus 1)
		if ( p.max_value )
			text_width = p.font()->getWidth(std::string("0")) * ( static_cast < S32 > ( log10  ( p.max_value ) ) + p.decimal_digits + 1 );

		if ( p.increment < 1.0f )
			text_width += p.font()->getWidth(std::string("."));	// (mostly) take account of decimal point in value

		if ( p.min_value < 0.0f || p.max_value < 0.0f )
			text_width += p.font()->getWidth(std::string("-"));	// (mostly) take account of minus sign 

		// padding to make things look nicer
		text_width += 8;
	}


	S32 text_left = getRect().getWidth() - text_width;
	static LLUICachedControl<S32> sliderctrl_spacing ("UISliderctrlSpacing", 0);

	S32 slider_right = getRect().getWidth();
	if( p.show_text )
	{
		slider_right = text_left - sliderctrl_spacing;
	}

	S32 slider_left = label_width ? label_width + sliderctrl_spacing : 0;
	LLSlider::Params slider_p(p.slider_bar);
	slider_p.name("slider_bar");
	slider_p.rect.setIfNotProvided(LLRect(slider_left,top,slider_right,bottom));
	slider_p.initial_value.setIfNotProvided(p.initial_value().asReal());
	slider_p.min_value.setIfNotProvided(p.min_value);
	slider_p.max_value.setIfNotProvided(p.max_value);
	slider_p.increment.setIfNotProvided(p.increment);
	slider_p.orientation.setIfNotProvided(p.orientation);
	
	slider_p.commit_callback.function(&LLSliderCtrl::onSliderCommit);
	slider_p.control_name(p.control_name);
	slider_p.mouse_down_callback( p.mouse_down_callback );
	slider_p.mouse_up_callback( p.mouse_up_callback );
	mSlider = LLUICtrlFactory::create<LLSlider> (slider_p);

	addChild( mSlider );
	
	if( p.show_text() )
	{
		LLRect text_rect( text_left, top, getRect().getWidth(), bottom );
		if( p.can_edit_text() )
		{
			LLLineEditor::Params line_p(p.value_editor);
			line_p.rect.setIfNotProvided(text_rect);
			line_p.font.setIfNotProvided(p.font);
			line_p.commit_callback.function(&LLSliderCtrl::onEditorCommit);
			line_p.prevalidate_callback(&LLTextValidate::validateFloat);
			mEditor = LLUICtrlFactory::create<LLLineEditor>(line_p);

			mEditor->setFocusReceivedCallback( boost::bind(&LLSliderCtrl::onEditorGainFocus, _1, this ));
			// don't do this, as selecting the entire text is single clicking in some cases
			// and double clicking in others
			//mEditor->setSelectAllonFocusReceived(TRUE);
			addChild(mEditor);
		}
		else
		{
			LLTextBox::Params text_p(p.value_text);
			text_p.rect.setIfNotProvided(text_rect);
			text_p.font.setIfNotProvided(p.font);
			mTextBox = LLUICtrlFactory::create<LLTextBox>(text_p);
			addChild(mTextBox);
		}
	}

	updateText();
}

// static
void LLSliderCtrl::onEditorGainFocus( LLFocusableElement* caller, void *userdata )
{
	LLSliderCtrl* self = (LLSliderCtrl*) userdata;
	llassert( caller == self->mEditor );

	self->onFocusReceived();
}


void LLSliderCtrl::setValue(F32 v, BOOL from_event)
{
	mSlider->setValue( v, from_event );
	mValue = mSlider->getValueF32();
	updateText();
}

BOOL LLSliderCtrl::setLabelArg( const std::string& key, const LLStringExplicit& text )
{
	BOOL res = FALSE;
	if (mLabelBox)
	{
		res = mLabelBox->setTextArg(key, text);
		if (res && mLabelFont && mLabelWidth == 0)
		{
			S32 label_width = mLabelFont->getWidth(mLabelBox->getText());
			LLRect rect = mLabelBox->getRect();
			S32 prev_right = rect.mRight;
			rect.mRight = rect.mLeft + label_width;
			mLabelBox->setRect(rect);
				
			S32 delta = rect.mRight - prev_right;
			rect = mSlider->getRect();
			S32 left = rect.mLeft + delta;
			static LLUICachedControl<S32> sliderctrl_spacing ("UISliderctrlSpacing", 0);
			left = llclamp(left, 0, rect.mRight - sliderctrl_spacing);
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
		mEditor->setText( LLStringUtil::null );
	}
	if( mTextBox )
	{
		mTextBox->setText( LLStringUtil::null );
	}

}

void LLSliderCtrl::updateText()
{
	if( mEditor || mTextBox )
	{
		LLLocale locale(LLLocale::USER_LOCALE);

		// Don't display very small negative values as -0.000
		F32 displayed_value = (F32)(floor(getValueF32() * pow(10.0, (F64)mPrecision) + 0.5) / pow(10.0, (F64)mPrecision));

		std::string format = llformat("%%.%df", mPrecision);
		std::string text = llformat(format.c_str(), displayed_value);
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
void LLSliderCtrl::onEditorCommit( LLUICtrl* ctrl, const LLSD& userdata )
{
	LLSliderCtrl* self = dynamic_cast<LLSliderCtrl*>(ctrl->getParent());
	if (!self)
		return;

	BOOL success = FALSE;
	F32 val = self->mValue;
	F32 saved_val = self->mValue;

	std::string text = self->mEditor->getText();
	if( LLLineEditor::postvalidateFloat( text ) )
	{
		LLLocale locale(LLLocale::USER_LOCALE);
		val = (F32) atof( text.c_str() );
		if( self->mSlider->getMinValue() <= val && val <= self->mSlider->getMaxValue() )
		{
			self->setValue( val );  // set the value temporarily so that the callback can retrieve it.
			if( !self->mValidateSignal || (*(self->mValidateSignal))( self, val ) )
			{
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
void LLSliderCtrl::onSliderCommit( LLUICtrl* ctrl, const LLSD& userdata )
{
	LLSliderCtrl* self = dynamic_cast<LLSliderCtrl*>(ctrl->getParent());
	if (!self)
		return;

	BOOL success = FALSE;
	F32 saved_val = self->mValue;
	F32 new_val = self->mSlider->getValueF32();

	self->mValue = new_val;  // set the value temporarily so that the callback can retrieve it.
	if( !self->mValidateSignal || (*(self->mValidateSignal))( self, new_val ) )
	{
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
		mLabelBox->setColor( b ? mTextEnabledColor.get() : mTextDisabledColor.get() );
	}

	mSlider->setEnabled( b );

	if( mEditor )
	{
		mEditor->setEnabled( b );
	}

	if( mTextBox )
	{
		mTextBox->setColor( b ? mTextEnabledColor.get() : mTextDisabledColor.get() );
	}
}


void LLSliderCtrl::setTentative(BOOL b)
{
	if( mEditor )
	{
		mEditor->setTentative(b);
	}
	LLF32UICtrl::setTentative(b);
}


void LLSliderCtrl::onCommit()
{
	setTentative(FALSE);

	if( mEditor )
	{
		mEditor->setTentative(FALSE);
	}
	
	setControlValue(getValueF32());
	LLF32UICtrl::onCommit();
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

boost::signals2::connection LLSliderCtrl::setSliderMouseDownCallback( const commit_signal_t::slot_type& cb )
{
	return mSlider->setMouseDownCallback( cb );
}

boost::signals2::connection LLSliderCtrl::setSliderMouseUpCallback( const commit_signal_t::slot_type& cb )
{
	return mSlider->setMouseUpCallback( cb );
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


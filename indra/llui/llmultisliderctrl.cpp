/** 
 * @file llmultisliderctrl.cpp
 * @brief LLMultiSliderCtrl base class
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llmultisliderctrl.h"

#include "llmath.h"
#include "llfontgl.h"
#include "llgl.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llmultislider.h"
#include "llstring.h"
#include "lltextbox.h"
#include "llui.h"
#include "lluiconstants.h"
#include "llcontrol.h"
#include "llfocusmgr.h"
#include "llresmgr.h"
#include "lluictrlfactory.h"

static LLDefaultChildRegistry::Register<LLMultiSliderCtrl> r("multi_slider");

const U32 MAX_STRING_LENGTH = 10;
LLMultiSliderCtrl::Params::Params()
:	text_width("text_width"),
	label_width("label_width"),
	show_text("show_text", true),
	can_edit_text("can_edit_text", false),
	max_sliders("max_sliders", 1),
	allow_overlap("allow_overlap", false),
	draw_track("draw_track", true),
	use_triangle("use_triangle", false),
	decimal_digits("decimal_digits", 3),
	text_color("text_color"),
	text_disabled_color("text_disabled_color"),
	mouse_down_callback("mouse_down_callback"),
	mouse_up_callback("mouse_up_callback"),
	sliders("slider")
{
	mouse_opaque = true;
}
 
LLMultiSliderCtrl::LLMultiSliderCtrl(const LLMultiSliderCtrl::Params& p)
:	LLF32UICtrl(p),
	mLabelBox( NULL ),
	mEditor( NULL ),
	mTextBox( NULL ),
	mTextEnabledColor(p.text_color()),
	mTextDisabledColor(p.text_disabled_color())
{
	static LLUICachedControl<S32> multi_sliderctrl_spacing ("UIMultiSliderctrlSpacing", 0);

	S32 top = getRect().getHeight();
	S32 bottom = 0;
	S32 left = 0;

	S32 label_width = p.label_width;
	S32 text_width = p.text_width;

	// Label
	if( !p.label().empty() )
	{
		if (p.label_width == 0)
		{
			label_width = p.font()->getWidth(p.label);
		}
		LLRect label_rect( left, top, label_width, bottom );
		LLTextBox::Params params;
		params.name("MultiSliderCtrl Label");
		params.rect(label_rect);
		params.initial_value(p.label());
		params.font(p.font);
		mLabelBox = LLUICtrlFactory::create<LLTextBox> (params);
		addChild(mLabelBox);
	}

	S32 slider_right = getRect().getWidth();

	if (p.show_text)
	{
		if (!p.text_width.isProvided())
		{
			text_width = 0;
			// calculate the size of the text box (log max_value is number of digits - 1 so plus 1)
			if ( p.max_value() )
				text_width = p.font()->getWidth(std::string("0")) * ( static_cast < S32 > ( log10  ( p.max_value ) ) + p.decimal_digits + 1 );

			if ( p.increment < 1.0f )
				text_width += p.font()->getWidth(std::string("."));	// (mostly) take account of decimal point in value

			if ( p.min_value < 0.0f || p.max_value < 0.0f )
				text_width += p.font()->getWidth(std::string("-"));	// (mostly) take account of minus sign 

			// padding to make things look nicer
			text_width += 8;
		}
		S32 text_left = getRect().getWidth() - text_width;

		slider_right = text_left - multi_sliderctrl_spacing;

		LLRect text_rect( text_left, top, getRect().getWidth(), bottom );
		if( p.can_edit_text )
		{
			LLLineEditor::Params params;
			params.name("MultiSliderCtrl Editor");
			params.rect(text_rect);
			params.font(p.font);
			params.max_length_bytes(MAX_STRING_LENGTH);
			params.commit_callback.function(LLMultiSliderCtrl::onEditorCommit);
			params.prevalidate_callback(&LLTextValidate::validateFloat);
			params.follows.flags(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
			mEditor = LLUICtrlFactory::create<LLLineEditor> (params);
			mEditor->setFocusReceivedCallback( boost::bind(LLMultiSliderCtrl::onEditorGainFocus, _1, this) );
			// don't do this, as selecting the entire text is single clicking in some cases
			// and double clicking in others
			//mEditor->setSelectAllonFocusReceived(TRUE);
			addChild(mEditor);
		}
		else
		{
			LLTextBox::Params params;
			params.name("MultiSliderCtrl Text");
			params.rect(text_rect);
			params.font(p.font);
			params.follows.flags(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
			mTextBox = LLUICtrlFactory::create<LLTextBox> (params);
			addChild(mTextBox);
		}
	}

	S32 slider_left = label_width ? label_width + multi_sliderctrl_spacing : 0;
	LLRect slider_rect( slider_left, top, slider_right, bottom );
	LLMultiSlider::Params params;
	params.sliders = p.sliders;
	params.rect(slider_rect);
	params.commit_callback.function( LLMultiSliderCtrl::onSliderCommit );
	params.mouse_down_callback( p.mouse_down_callback );
	params.mouse_up_callback( p.mouse_up_callback );
	params.initial_value(p.initial_value());
	params.min_value(p.min_value);
	params.max_value(p.max_value);
	params.increment(p.increment);
	params.max_sliders(p.max_sliders);
	params.allow_overlap(p.allow_overlap);
	params.draw_track(p.draw_track);
	params.use_triangle(p.use_triangle);
	params.control_name(p.control_name);
	mMultiSlider = LLUICtrlFactory::create<LLMultiSlider> (params);
	addChild( mMultiSlider );
	mCurValue = mMultiSlider->getCurSliderValue();


	updateText();
}

LLMultiSliderCtrl::~LLMultiSliderCtrl()
{
	// Children all cleaned up by default view destructor.
}

// static
void LLMultiSliderCtrl::onEditorGainFocus( LLFocusableElement* caller, void *userdata )
{
	LLMultiSliderCtrl* self = (LLMultiSliderCtrl*) userdata;
	llassert( caller == self->mEditor );

	self->onFocusReceived();
}


void LLMultiSliderCtrl::setValue(const LLSD& value)
{
	mMultiSlider->setValue(value);
	mCurValue = mMultiSlider->getCurSliderValue();
	updateText();
}

void LLMultiSliderCtrl::setSliderValue(const std::string& name, F32 v, BOOL from_event)
{
	mMultiSlider->setSliderValue(name, v, from_event );
	mCurValue = mMultiSlider->getCurSliderValue();
	updateText();
}

void LLMultiSliderCtrl::setCurSlider(const std::string& name)
{
	mMultiSlider->setCurSlider(name);
	mCurValue = mMultiSlider->getCurSliderValue();
}

BOOL LLMultiSliderCtrl::setLabelArg( const std::string& key, const LLStringExplicit& text )
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
			rect = mMultiSlider->getRect();
			S32 left = rect.mLeft + delta;
			static LLUICachedControl<S32> multi_slider_ctrl_spacing ("UIMultiSliderctrlSpacing", 0);
			left = llclamp(left, 0, rect.mRight - multi_slider_ctrl_spacing);
			rect.mLeft = left;
			mMultiSlider->setRect(rect);
		}
	}
	return res;
}

const std::string& LLMultiSliderCtrl::addSlider()
{
	const std::string& name = mMultiSlider->addSlider();
	
	// if it returns null, pass it on
	if(name == LLStringUtil::null) {
		return LLStringUtil::null;
	}

	// otherwise, update stuff
	mCurValue = mMultiSlider->getCurSliderValue();
	updateText();
	return name;
}

const std::string& LLMultiSliderCtrl::addSlider(F32 val)
{
	const std::string& name = mMultiSlider->addSlider(val);

	// if it returns null, pass it on
	if(name == LLStringUtil::null) {
		return LLStringUtil::null;
	}

	// otherwise, update stuff
	mCurValue = mMultiSlider->getCurSliderValue();
	updateText();
	return name;
}

void LLMultiSliderCtrl::deleteSlider(const std::string& name)
{
	mMultiSlider->deleteSlider(name);
	mCurValue = mMultiSlider->getCurSliderValue();
	updateText();
}


void LLMultiSliderCtrl::clear()
{
	setCurSliderValue(0.0f);
	if( mEditor )
	{
		mEditor->setText(std::string(""));
	}
	if( mTextBox )
	{
		mTextBox->setText(std::string(""));
	}

	// get rid of sliders
	mMultiSlider->clear();

}

BOOL LLMultiSliderCtrl::isMouseHeldDown()
{
	return gFocusMgr.getMouseCapture() == mMultiSlider;
}

void LLMultiSliderCtrl::updateText()
{
	if( mEditor || mTextBox )
	{
		LLLocale locale(LLLocale::USER_LOCALE);

		// Don't display very small negative values as -0.000
		F32 displayed_value = (F32)(floor(getCurSliderValue() * pow(10.0, (F64)mPrecision) + 0.5) / pow(10.0, (F64)mPrecision));

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
void LLMultiSliderCtrl::onEditorCommit( LLUICtrl* ctrl, const LLSD& userdata)
{
	llassert(ctrl);
	if (!ctrl)
		return;

	LLMultiSliderCtrl* self = dynamic_cast<LLMultiSliderCtrl*>(ctrl->getParent());
	llassert(self);
	if (!self) // cast failed - wrong type! :O
		return;
	
	BOOL success = FALSE;
	F32 val = self->mCurValue;
	F32 saved_val = self->mCurValue;

	std::string text = self->mEditor->getText();
	if( LLLineEditor::postvalidateFloat( text ) )
	{
		LLLocale locale(LLLocale::USER_LOCALE);
		val = (F32) atof( text.c_str() );
		if( self->mMultiSlider->getMinValue() <= val && val <= self->mMultiSlider->getMaxValue() )
		{
			self->setCurSliderValue( val );  // set the value temporarily so that the callback can retrieve it.
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
		if( self->getCurSliderValue() != saved_val )
		{
			self->setCurSliderValue( saved_val );
		}
		self->reportInvalidData();		
	}
	self->updateText();
}

// static
void LLMultiSliderCtrl::onSliderCommit(LLUICtrl* ctrl, const LLSD& userdata)
{
	LLMultiSliderCtrl* self = dynamic_cast<LLMultiSliderCtrl*>(ctrl->getParent());
	if (!self)
		return;
	
	BOOL success = FALSE;
	F32 saved_val = self->mCurValue;
	F32 new_val = self->mMultiSlider->getCurSliderValue();

	self->mCurValue = new_val;  // set the value temporarily so that the callback can retrieve it.
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
		if( self->mCurValue != saved_val )
		{
			self->setCurSliderValue( saved_val );
		}
		self->reportInvalidData();		
	}
	self->updateText();
}

void LLMultiSliderCtrl::setEnabled(BOOL b)
{
	LLF32UICtrl::setEnabled( b );

	if( mLabelBox )
	{
		mLabelBox->setColor( b ? mTextEnabledColor.get() : mTextDisabledColor.get() );
	}

	mMultiSlider->setEnabled( b );

	if( mEditor )
	{
		mEditor->setEnabled( b );
	}

	if( mTextBox )
	{
		mTextBox->setColor( b ? mTextEnabledColor.get() : mTextDisabledColor.get() );
	}
}


void LLMultiSliderCtrl::setTentative(BOOL b)
{
	if( mEditor )
	{
		mEditor->setTentative(b);
	}
	LLF32UICtrl::setTentative(b);
}


void LLMultiSliderCtrl::onCommit()
{
	setTentative(FALSE);

	if( mEditor )
	{
		mEditor->setTentative(FALSE);
	}

	setControlValue(getValueF32());
	LLF32UICtrl::onCommit();
}


void LLMultiSliderCtrl::setPrecision(S32 precision)
{
	if (precision < 0 || precision > 10)
	{
		llerrs << "LLMultiSliderCtrl::setPrecision - precision out of range" << llendl;
		return;
	}

	mPrecision = precision;
	updateText();
}

boost::signals2::connection LLMultiSliderCtrl::setSliderMouseDownCallback( const commit_signal_t::slot_type& cb )
{
	return mMultiSlider->setMouseDownCallback( cb );
}

boost::signals2::connection LLMultiSliderCtrl::setSliderMouseUpCallback( const commit_signal_t::slot_type& cb )
{
	return mMultiSlider->setMouseUpCallback( cb );
}

void LLMultiSliderCtrl::onTabInto()
{
	if( mEditor )
	{
		mEditor->onTabInto(); 
	}
}

void LLMultiSliderCtrl::reportInvalidData()
{
	make_ui_sound("UISndBadKeystroke");
}

// virtual
void LLMultiSliderCtrl::setControlName(const std::string& control_name, LLView* context)
{
	mMultiSlider->setControlName(control_name, context);
}


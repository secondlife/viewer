/** 
 * @file llspinctrl.cpp
 * @brief LLSpinCtrl base class
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
 
#include "llspinctrl.h"

#include "llgl.h"
#include "llui.h"
#include "lluiconstants.h"

#include "llstring.h"
#include "llfontgl.h"
#include "lllineeditor.h"
#include "llbutton.h"
#include "lltextbox.h"
#include "llkeyboard.h"
#include "llmath.h"
#include "llcontrol.h"
#include "llfocusmgr.h"
#include "llresmgr.h"
#include "lluictrlfactory.h"

const U32 MAX_STRING_LENGTH = 32;

static LLDefaultChildRegistry::Register<LLSpinCtrl> r2("spinner");

LLSpinCtrl::Params::Params()
:	label_width("label_width"),
	decimal_digits("decimal_digits"),
	allow_text_entry("allow_text_entry", true),
	text_enabled_color("text_enabled_color"),
	text_disabled_color("text_disabled_color"),
	up_button("up_button"),
	down_button("down_button")
{}

LLSpinCtrl::LLSpinCtrl(const LLSpinCtrl::Params& p)
:	LLF32UICtrl(p),
	mLabelBox(NULL),
	mbHasBeenSet( FALSE ),
	mPrecision(p.decimal_digits),
	mTextEnabledColor(p.text_enabled_color()),
	mTextDisabledColor(p.text_disabled_color())
{
	static LLUICachedControl<S32> spinctrl_spacing ("UISpinctrlSpacing", 0);
	static LLUICachedControl<S32> spinctrl_btn_width ("UISpinctrlBtnWidth", 0);
	static LLUICachedControl<S32> spinctrl_btn_height ("UISpinctrlBtnHeight", 0);
	S32 centered_top = getRect().getHeight();
	S32 centered_bottom = getRect().getHeight() - 2 * spinctrl_btn_height;
	S32 btn_left = 0;
	// reserve space for spinner
	S32 label_width = llclamp(p.label_width(), 0, llmax(0, getRect().getWidth() - 40));

	// Label
	if( !p.label().empty() )
	{
		LLRect label_rect( 0, centered_top, label_width, centered_bottom );
		LLTextBox::Params params;
		params.name("SpinCtrl Label");
		params.rect(label_rect);
		params.initial_value(p.label());
		if (p.font.isProvided())
		{
			params.font(p.font);
		}
		mLabelBox = LLUICtrlFactory::create<LLTextBox> (params);
		addChild(mLabelBox);

		btn_left += label_rect.mRight + spinctrl_spacing;
	}

	S32 btn_right = btn_left + spinctrl_btn_width;
	
	// Spin buttons
	LLButton::Params up_button_params(p.up_button);
	up_button_params.rect = LLRect(btn_left, getRect().getHeight(), btn_right, getRect().getHeight() - spinctrl_btn_height);
	up_button_params.click_callback.function(boost::bind(&LLSpinCtrl::onUpBtn, this, _2));
	up_button_params.mouse_held_callback.function(boost::bind(&LLSpinCtrl::onUpBtn, this, _2));

	mUpBtn = LLUICtrlFactory::create<LLButton>(up_button_params);
	addChild(mUpBtn);

	LLButton::Params down_button_params(p.down_button);
	down_button_params.rect = LLRect(btn_left, getRect().getHeight() - spinctrl_btn_height, btn_right, getRect().getHeight() - 2 * spinctrl_btn_height);
	down_button_params.click_callback.function(boost::bind(&LLSpinCtrl::onDownBtn, this, _2));
	down_button_params.mouse_held_callback.function(boost::bind(&LLSpinCtrl::onDownBtn, this, _2));
	mDownBtn = LLUICtrlFactory::create<LLButton>(down_button_params);
	addChild(mDownBtn);

	LLRect editor_rect( btn_right + 1, centered_top, getRect().getWidth(), centered_bottom );
	LLLineEditor::Params params;
	params.name("SpinCtrl Editor");
	params.rect(editor_rect);
	if (p.font.isProvided())
	{
		params.font(p.font);
	}
	params.max_length_bytes(MAX_STRING_LENGTH);
	params.commit_callback.function((boost::bind(&LLSpinCtrl::onEditorCommit, this, _2)));
	params.prevalidate_callback(&LLLineEditor::prevalidateFloat);
	params.follows.flags(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
	mEditor = LLUICtrlFactory::create<LLLineEditor> (params);
	mEditor->setFocusReceivedCallback( boost::bind(&LLSpinCtrl::onEditorGainFocus, _1, this ));
	//RN: this seems to be a BAD IDEA, as it makes the editor behavior different when it has focus
	// than when it doesn't.  Instead, if you always have to double click to select all the text, 
	// it's easier to understand
	//mEditor->setSelectAllonFocusReceived(TRUE);
	addChild(mEditor);

	updateEditor();
	setUseBoundingRect( TRUE );
}

F32 clamp_precision(F32 value, S32 decimal_precision)
{
	// pow() isn't perfect
	
	F64 clamped_value = value;
	for (S32 i = 0; i < decimal_precision; i++)
		clamped_value *= 10.0;

	clamped_value = llround((F32)clamped_value);

	for (S32 i = 0; i < decimal_precision; i++)
		clamped_value /= 10.0;
	
	return (F32)clamped_value;
}


void LLSpinCtrl::onUpBtn( const LLSD& data )
{
	if( getEnabled() )
	{
		std::string text = mEditor->getText();
		if( LLLineEditor::postvalidateFloat( text ) )
		{
			
			LLLocale locale(LLLocale::USER_LOCALE);
			F32 cur_val = (F32) atof(text.c_str());
		
			// use getValue()/setValue() to force reload from/to control
			F32 val = cur_val + mIncrement;
			val = clamp_precision(val, mPrecision);
			val = llmin( val, mMaxValue );
			if (val < mMinValue) val = mMinValue;
			if (val > mMaxValue) val = mMaxValue;
		
			F32 saved_val = (F32)getValue().asReal();
			setValue(val);
			if( mValidateSignal && !(*mValidateSignal)( this, val ) )
			{
				setValue( saved_val );
				reportInvalidData();
				updateEditor();
				return;
			}

		updateEditor();
		onCommit();
		}
	}
}

void LLSpinCtrl::onDownBtn( const LLSD& data )
{
	if( getEnabled() )
	{
		std::string text = mEditor->getText();
		if( LLLineEditor::postvalidateFloat( text ) )
		{

			LLLocale locale(LLLocale::USER_LOCALE);
			F32 cur_val = (F32) atof(text.c_str());
		
			F32 val = cur_val - mIncrement;
			val = clamp_precision(val, mPrecision);
			val = llmax( val, mMinValue );

			if (val < mMinValue) val = mMinValue;
			if (val > mMaxValue) val = mMaxValue;
			
			F32 saved_val = (F32)getValue().asReal();
			setValue(val);
			if( mValidateSignal && !(*mValidateSignal)( this, val ) )
			{
				setValue( saved_val );
				reportInvalidData();
				updateEditor();
				return;
			}
		
			updateEditor();
			onCommit();
		}
	}
}

// static
void LLSpinCtrl::onEditorGainFocus( LLFocusableElement* caller, void *userdata )
{
	LLSpinCtrl* self = (LLSpinCtrl*) userdata;
	llassert( caller == self->mEditor );

	self->onFocusReceived();
}

void LLSpinCtrl::setValue(const LLSD& value )
{
	F32 v = (F32)value.asReal();
	if (getValueF32() != v || !mbHasBeenSet)
	{
		mbHasBeenSet = TRUE;
        LLF32UICtrl::setValue(value);
		
		if (!mEditor->hasFocus())
		{
			updateEditor();
		}
	}
}

//no matter if Editor has the focus, update the value
void LLSpinCtrl::forceSetValue(const LLSD& value )
{
	F32 v = (F32)value.asReal();
	if (getValueF32() != v || !mbHasBeenSet)
	{
		mbHasBeenSet = TRUE;
        LLF32UICtrl::setValue(value);
		
		updateEditor();
	}
}

void LLSpinCtrl::clear()
{
	setValue(mMinValue);
	mEditor->clear();
	mbHasBeenSet = FALSE;
}



void LLSpinCtrl::updateEditor()
{
	LLLocale locale(LLLocale::USER_LOCALE);

	// Don't display very small negative values as -0.000
	F32 displayed_value = clamp_precision((F32)getValue().asReal(), mPrecision);

//	if( S32( displayed_value * pow( 10, mPrecision ) ) == 0 )
//	{
//		displayed_value = 0.f;
//	}

	std::string format = llformat("%%.%df", mPrecision);
	std::string text = llformat(format.c_str(), displayed_value);
	mEditor->setText( text );
}

void LLSpinCtrl::onEditorCommit( const LLSD& data )
{
	BOOL success = FALSE;
	
	std::string text = mEditor->getText();
	if( LLLineEditor::postvalidateFloat( text ) )
	{
		LLLocale locale(LLLocale::USER_LOCALE);
		F32 val = (F32) atof(text.c_str());

		if (val < mMinValue) val = mMinValue;
		if (val > mMaxValue) val = mMaxValue;

		F32 saved_val = getValueF32();
		setValue(val);
		if( !mValidateSignal || (*mValidateSignal)( this, val ) )
		{
			success = TRUE;
			onCommit();
		}
		else
		{
			setValue(saved_val);
		}
	}
	updateEditor();

	if( !success )
	{
		reportInvalidData();		
	}
}


void LLSpinCtrl::forceEditorCommit()
{
	onEditorCommit( LLSD() );
}


void LLSpinCtrl::setFocus(BOOL b)
{
	LLUICtrl::setFocus( b );
	mEditor->setFocus( b );
}

void LLSpinCtrl::setEnabled(BOOL b)
{
	LLView::setEnabled( b );
	mEditor->setEnabled( b );
	if( mLabelBox )
	{
		mLabelBox->setColor( b ? mTextEnabledColor.get() : mTextDisabledColor.get() );
	}
}


void LLSpinCtrl::setTentative(BOOL b)
{
	mEditor->setTentative(b);
	LLUICtrl::setTentative(b);
}


BOOL LLSpinCtrl::isMouseHeldDown() const
{
	return 
		mDownBtn->hasMouseCapture()
		|| mUpBtn->hasMouseCapture();
}

void LLSpinCtrl::onCommit()
{
	setTentative(FALSE);
	setControlValue(getValueF32());
	LLF32UICtrl::onCommit();
}


void LLSpinCtrl::setPrecision(S32 precision)
{
	if (precision < 0 || precision > 10)
	{
		llerrs << "LLSpinCtrl::setPrecision - precision out of range" << llendl;
		return;
	}

	mPrecision = precision;
	updateEditor();
}

void LLSpinCtrl::setLabel(const LLStringExplicit& label)
{
	if (mLabelBox)
	{
		mLabelBox->setText(label);
	}
	else
	{
		llwarns << "Attempting to set label on LLSpinCtrl constructed without one " << getName() << llendl;
	}
}

void LLSpinCtrl::setAllowEdit(BOOL allow_edit)
{
	mEditor->setEnabled(allow_edit);
	mAllowEdit = allow_edit;
}

void LLSpinCtrl::onTabInto()
{
	mEditor->onTabInto(); 
}


void LLSpinCtrl::reportInvalidData()
{
	make_ui_sound("UISndBadKeystroke");
}

BOOL LLSpinCtrl::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	if( clicks > 0 )
	{
		while( clicks-- )
		{
			onDownBtn(getValue());
		}
	}
	else
	while( clicks++ )
	{
		onUpBtn(getValue());
	}

	return TRUE;
}

BOOL LLSpinCtrl::handleKeyHere(KEY key, MASK mask)
{
	if (mEditor->hasFocus())
	{
		if(key == KEY_ESCAPE)
		{
			// text editors don't support revert normally (due to user confusion)
			// but not allowing revert on a spinner seems dangerous
			updateEditor();
			mEditor->setFocus(FALSE);
			return TRUE;
		}
		if(key == KEY_UP)
		{
			onUpBtn(getValue());
			return TRUE;
		}
		if(key == KEY_DOWN)
		{
			onDownBtn(getValue());
			return TRUE;
		}
	}
	return FALSE;
}


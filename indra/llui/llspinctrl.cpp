/** 
 * @file llspinctrl.cpp
 * @brief LLSpinCtrl base class
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
#include "sound_ids.h"
#include "audioengine.h"
#include "llcontrol.h"
#include "llfocusmgr.h"
#include "llresmgr.h"

const U32 MAX_STRING_LENGTH = 32;

 
LLSpinCtrl::LLSpinCtrl(	const LLString& name, const LLRect& rect, const LLString& label, const LLFontGL* font,
	void (*commit_callback)(LLUICtrl*, void*),
	void* callback_user_data,
	F32 initial_value, F32 min_value, F32 max_value, F32 increment,
	const LLString& control_name,
	S32 label_width)
	:
	LLUICtrl(name, rect, TRUE, commit_callback, callback_user_data, FOLLOWS_LEFT | FOLLOWS_TOP ),
	mValue( initial_value ),
	mInitialValue( initial_value ),
	mMaxValue( max_value ),
	mMinValue( min_value ),
	mIncrement( increment ),
	mPrecision( 3 ),
	mLabelBox( NULL ),
	mTextEnabledColor( LLUI::sColorsGroup->getColor( "LabelTextColor" ) ),
	mTextDisabledColor( LLUI::sColorsGroup->getColor( "LabelDisabledColor" ) ),
	mbHasBeenSet( FALSE )
{
	S32 top = mRect.getHeight();
	S32 bottom = top - 2 * SPINCTRL_BTN_HEIGHT;
	S32 centered_top = top;
	S32 centered_bottom = bottom;
	S32 btn_left = 0;

	// Label
	if( !label.empty() )
	{
		LLRect label_rect( 0, centered_top, label_width, centered_bottom );
		mLabelBox = new LLTextBox( "SpinCtrl Label", label_rect, label.c_str(), font );
		addChild(mLabelBox);

		btn_left += label_rect.mRight + SPINCTRL_SPACING;
	}

	S32 btn_right = btn_left + SPINCTRL_BTN_WIDTH;
	
	// Spin buttons
	LLRect up_rect( btn_left, top, btn_right, top - SPINCTRL_BTN_HEIGHT );
	LLString out_id = "UIImgBtnSpinUpOutUUID";
	LLString in_id = "UIImgBtnSpinUpInUUID";
	mUpBtn = new LLButton(
		"SpinCtrl Up", up_rect,
		out_id,
		in_id,
		"",
		&LLSpinCtrl::onUpBtn, this, LLFontGL::sSansSerif );
	mUpBtn->setFollowsLeft();
	mUpBtn->setFollowsBottom();
	mUpBtn->setHeldDownCallback( &LLSpinCtrl::onUpBtn );
	mUpBtn->setTabStop(FALSE);
	addChild(mUpBtn);

	LLRect down_rect( btn_left, top - SPINCTRL_BTN_HEIGHT, btn_right, bottom );
	out_id = "UIImgBtnSpinDownOutUUID";
	in_id = "UIImgBtnSpinDownInUUID";
	mDownBtn = new LLButton(
		"SpinCtrl Down", down_rect,
		out_id,
		in_id,
		"",
		&LLSpinCtrl::onDownBtn, this, LLFontGL::sSansSerif );
	mDownBtn->setFollowsLeft();
	mDownBtn->setFollowsBottom();
	mDownBtn->setHeldDownCallback( &LLSpinCtrl::onDownBtn );
	mDownBtn->setTabStop(FALSE);
	addChild(mDownBtn);

	LLRect editor_rect( btn_right + 1, centered_top, mRect.getWidth(), centered_bottom );
	mEditor = new LLLineEditor( "SpinCtrl Editor", editor_rect, "",	font,
		MAX_STRING_LENGTH,
		&LLSpinCtrl::onEditorCommit, NULL, NULL, this,
		&LLLineEditor::prevalidateFloat );
	mEditor->setFollowsLeft();
	mEditor->setFollowsBottom();
	mEditor->setFocusReceivedCallback( &LLSpinCtrl::onEditorGainFocus, this );
	//RN: this seems to be a BAD IDEA, as it makes the editor behavior different when it has focus
	// than when it doesn't.  Instead, if you always have to double click to select all the text, 
	// it's easier to understand
	//mEditor->setSelectAllonFocusReceived(TRUE);
	mEditor->setIgnoreTab(TRUE);
	addChild(mEditor);

	updateEditor();
	setUseBoundingRect( TRUE );
}

LLSpinCtrl::~LLSpinCtrl()
{
	// Children all cleaned up by default view destructor.
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


// static
void LLSpinCtrl::onUpBtn( void *userdata )
{
	LLSpinCtrl* self = (LLSpinCtrl*) userdata;
	if( self->getEnabled() )
	{
		// use getValue()/setValue() to force reload from/to control
		F32 val = (F32)self->getValue().asReal() + self->mIncrement;
		val = clamp_precision(val, self->mPrecision);
		val = llmin( val, self->mMaxValue );
		
		if( self->mValidateCallback )
		{
			F32 saved_val = (F32)self->getValue().asReal();
			self->setValue(val);
			if( !self->mValidateCallback( self, self->mCallbackUserData ) )
			{
				self->setValue( saved_val );
				self->reportInvalidData();
				self->updateEditor();
				return;
			}
		}
		else
		{
			self->setValue(val);
		}

		self->updateEditor();
		self->onCommit();
	}
}

// static
void LLSpinCtrl::onDownBtn( void *userdata )
{
	LLSpinCtrl* self = (LLSpinCtrl*) userdata;

	if( self->getEnabled() )
	{
		F32 val = (F32)self->getValue().asReal() - self->mIncrement;
		val = clamp_precision(val, self->mPrecision);
		val = llmax( val, self->mMinValue );

		if( self->mValidateCallback )
		{
			F32 saved_val = (F32)self->getValue().asReal();
			self->setValue(val);
			if( !self->mValidateCallback( self, self->mCallbackUserData ) )
			{
				self->setValue( saved_val );
				self->reportInvalidData();
				self->updateEditor();
				return;
			}
		}
		else
		{
			self->setValue(val);
		}
		
		self->updateEditor();
		self->onCommit();
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
	if (mValue != v || !mbHasBeenSet)
	{
		mbHasBeenSet = TRUE;
		mValue = v;
		
		if (!mEditor->hasFocus())
		{
			updateEditor();
		}
	}
}

LLSD LLSpinCtrl::getValue() const
{
	return mValue;
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

	LLString format = llformat("%%.%df", mPrecision);
	LLString text = llformat(format.c_str(), displayed_value);
	mEditor->setText( text );
}

void LLSpinCtrl::onEditorCommit( LLUICtrl* caller, void *userdata )
{
	BOOL success = FALSE;
	
	LLSpinCtrl* self = (LLSpinCtrl*) userdata;
	llassert( caller == self->mEditor );

	LLString text = self->mEditor->getText();
	if( LLLineEditor::postvalidateFloat( text ) )
	{
		LLLocale locale(LLLocale::USER_LOCALE);
		F32 val = (F32) atof(text.c_str());

		if (val < self->mMinValue) val = self->mMinValue;
		if (val > self->mMaxValue) val = self->mMaxValue;

		if( self->mValidateCallback )
		{
			F32 saved_val = self->mValue;
			self->mValue = val;
			if( self->mValidateCallback( self, self->mCallbackUserData ) )
			{
				success = TRUE;
				self->onCommit();
			}
			else
			{
				self->mValue = saved_val;
			}
		}
		else
		{
			self->mValue = val;
			self->onCommit();
			success = TRUE;
		}
	}
	self->updateEditor();

	if( !success )
	{
		self->reportInvalidData();		
	}
}


void LLSpinCtrl::forceEditorCommit()
{
	onEditorCommit(mEditor, this);
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
}


void LLSpinCtrl::setTentative(BOOL b)
{
	mEditor->setTentative(b);
	LLUICtrl::setTentative(b);
}


BOOL LLSpinCtrl::isMouseHeldDown()
{
	return 
		mDownBtn->hasMouseCapture()
		|| mUpBtn->hasMouseCapture();
}

void LLSpinCtrl::onCommit()
{
	setTentative(FALSE);

	setControlValue(mValue);

	LLUICtrl::onCommit();
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

void LLSpinCtrl::onTabInto()
{
	mEditor->onTabInto(); 
}


void LLSpinCtrl::reportInvalidData()
{
	make_ui_sound("UISndBadKeystroke");
}

void LLSpinCtrl::draw()
{
	if( mLabelBox )
	{
		mLabelBox->setColor( mEnabled ? mTextEnabledColor : mTextDisabledColor );
	}
	LLUICtrl::draw();
}


BOOL LLSpinCtrl::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	if( mEnabled )
	{
		if( clicks > 0 )
		{
			while( clicks-- )
			{
				LLSpinCtrl::onDownBtn(this);
			}
		}
		else
		while( clicks++ )
		{
			LLSpinCtrl::onUpBtn(this);
		}
	}

	return TRUE;
}

BOOL LLSpinCtrl::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent)
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
			LLSpinCtrl::onUpBtn(this);
			return TRUE;
		}
		if(key == KEY_DOWN)
		{
			LLSpinCtrl::onDownBtn(this);
			return TRUE;
		}
	}
	return FALSE;
}

// virtual
LLXMLNodePtr LLSpinCtrl::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	node->createChild("decimal_digits", TRUE)->setIntValue(mPrecision);

	if (mLabelBox)
	{
		node->createChild("label", TRUE)->setStringValue(mLabelBox->getText());

		node->createChild("label_width", TRUE)->setIntValue(mLabelBox->getRect().getWidth());
	}

	node->createChild("initial_val", TRUE)->setFloatValue(mInitialValue);

	node->createChild("min_val", TRUE)->setFloatValue(mMinValue);

	node->createChild("max_val", TRUE)->setFloatValue(mMaxValue);

	node->createChild("increment", TRUE)->setFloatValue(mIncrement);
	
	addColorXML(node, mTextEnabledColor, "text_enabled_color", "LabelTextColor");
	addColorXML(node, mTextDisabledColor, "text_disabled_color", "LabelDisabledColor");

	return node;
}

LLView* LLSpinCtrl::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("spinner");
	node->getAttributeString("name", name);

	LLString label;
	node->getAttributeString("label", label);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	LLFontGL* font = LLView::selectFont(node);

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
	
	S32 label_width = llmin(40, rect.getWidth() - 40);
	node->getAttributeS32("label_width", label_width);

	LLUICtrlCallback callback = NULL;

	if(label.empty())
	{
		label.assign( node->getValue() );
	}

	LLSpinCtrl* spinner = new LLSpinCtrl(name,
							rect,
							label,
							font,
							callback,
							NULL,
							initial_value, 
							min_value, 
							max_value, 
							increment,
							"",
							label_width);

	spinner->setPrecision(precision);

	spinner->initFromXML(node, parent);

	return spinner;
}

BOOL LLSpinCtrl::isDirty() const
{
	return( mValue != mInitialValue );
}

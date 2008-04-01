/** 
 * @file llmultisliderctrl.cpp
 * @brief LLMultiSliderCtrl base class
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2007, Linden Research, Inc.
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

#include "llmultisliderctrl.h"

#include "audioengine.h"
#include "sound_ids.h"

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

static LLRegisterWidget<LLMultiSliderCtrl> r("multi_slider");

const U32 MAX_STRING_LENGTH = 10;

 
LLMultiSliderCtrl::LLMultiSliderCtrl(const LLString& name, const LLRect& rect, 
						   const LLString& label,
						   const LLFontGL* font,
						   S32 label_width,
						   S32 text_left,
						   BOOL show_text,
						   BOOL can_edit_text,
						   void (*commit_callback)(LLUICtrl*, void*),
						   void* callback_user_data,
						   F32 initial_value, F32 min_value, F32 max_value, F32 increment,
						   S32 max_sliders, BOOL allow_overlap,
						   BOOL draw_track,
						   BOOL use_triangle,
						   const LLString& control_which)
	: LLUICtrl(name, rect, TRUE, commit_callback, callback_user_data ),
	  mFont(font),
	  mShowText( show_text ),
	  mCanEditText( can_edit_text ),
	  mPrecision( 3 ),
	  mLabelBox( NULL ),
	  mLabelWidth( label_width ),

	  mEditor( NULL ),
	  mTextBox( NULL ),
	  mTextEnabledColor( LLUI::sColorsGroup->getColor( "LabelTextColor" ) ),
	  mTextDisabledColor( LLUI::sColorsGroup->getColor( "LabelDisabledColor" ) ),
	  mSliderMouseUpCallback( NULL ),
	  mSliderMouseDownCallback( NULL )
{
	S32 top = getRect().getHeight();
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
		mLabelBox = new LLTextBox( "MultiSliderCtrl Label", label_rect, label.c_str(), font );
		addChild(mLabelBox);
	}

	S32 slider_right = getRect().getWidth();
	if( show_text )
	{
		slider_right = text_left - MULTI_SLIDERCTRL_SPACING;
	}

	S32 slider_left = label_width ? label_width + MULTI_SLIDERCTRL_SPACING : 0;
	LLRect slider_rect( slider_left, top, slider_right, bottom );
	mMultiSlider = new LLMultiSlider( 
		"multi_slider",
		slider_rect, 
		LLMultiSliderCtrl::onSliderCommit, this, 
		initial_value, min_value, max_value, increment,
		max_sliders, allow_overlap, draw_track,
		use_triangle,
		control_which );
	addChild( mMultiSlider );
	mCurValue = mMultiSlider->getCurSliderValue();
	
	if( show_text )
	{
		LLRect text_rect( text_left, top, getRect().getWidth(), bottom );
		if( can_edit_text )
		{
			mEditor = new LLLineEditor( "MultiSliderCtrl Editor", text_rect,
				"", font,
				MAX_STRING_LENGTH,
				&LLMultiSliderCtrl::onEditorCommit, NULL, NULL, this,
				&LLLineEditor::prevalidateFloat );
			mEditor->setFollowsLeft();
			mEditor->setFollowsBottom();
			mEditor->setFocusReceivedCallback( &LLMultiSliderCtrl::onEditorGainFocus );
			mEditor->setIgnoreTab(TRUE);
			// don't do this, as selecting the entire text is single clicking in some cases
			// and double clicking in others
			//mEditor->setSelectAllonFocusReceived(TRUE);
			addChild(mEditor);
		}
		else
		{
			mTextBox = new LLTextBox( "MultiSliderCtrl Text", text_rect,	"",	font);
			mTextBox->setFollowsLeft();
			mTextBox->setFollowsBottom();
			addChild(mTextBox);
		}
	}

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

void LLMultiSliderCtrl::setSliderValue(const LLString& name, F32 v, BOOL from_event)
{
	mMultiSlider->setSliderValue(name, v, from_event );
	mCurValue = mMultiSlider->getCurSliderValue();
	updateText();
}

void LLMultiSliderCtrl::setCurSlider(const LLString& name)
{
	mMultiSlider->setCurSlider(name);
	mCurValue = mMultiSlider->getCurSliderValue();
}

BOOL LLMultiSliderCtrl::setLabelArg( const LLString& key, const LLString& text )
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
			left = llclamp(left, 0, rect.mRight-MULTI_SLIDERCTRL_SPACING);
			rect.mLeft = left;
			mMultiSlider->setRect(rect);
		}
	}
	return res;
}

const LLString& LLMultiSliderCtrl::addSlider()
{
	const LLString& name = mMultiSlider->addSlider();
	
	// if it returns null, pass it on
	if(name == LLString::null) {
		return LLString::null;
	}

	// otherwise, update stuff
	mCurValue = mMultiSlider->getCurSliderValue();
	updateText();
	return name;
}

const LLString& LLMultiSliderCtrl::addSlider(F32 val)
{
	const LLString& name = mMultiSlider->addSlider(val);

	// if it returns null, pass it on
	if(name == LLString::null) {
		return LLString::null;
	}

	// otherwise, update stuff
	mCurValue = mMultiSlider->getCurSliderValue();
	updateText();
	return name;
}

void LLMultiSliderCtrl::deleteSlider(const LLString& name)
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
		mEditor->setText(LLString(""));
	}
	if( mTextBox )
	{
		mTextBox->setText(LLString(""));
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
void LLMultiSliderCtrl::onEditorCommit( LLUICtrl* caller, void *userdata )
{
	LLMultiSliderCtrl* self = (LLMultiSliderCtrl*) userdata;
	llassert( caller == self->mEditor );

	BOOL success = FALSE;
	F32 val = self->mCurValue;
	F32 saved_val = self->mCurValue;

	LLString text = self->mEditor->getText();
	if( LLLineEditor::postvalidateFloat( text ) )
	{
		LLLocale locale(LLLocale::USER_LOCALE);
		val = (F32) atof( text.c_str() );
		if( self->mMultiSlider->getMinValue() <= val && val <= self->mMultiSlider->getMaxValue() )
		{
			if( self->mValidateCallback )
			{
				self->setCurSliderValue( val );  // set the value temporarily so that the callback can retrieve it.
				if( self->mValidateCallback( self, self->mCallbackUserData ) )
				{
					success = TRUE;
				}
			}
			else
			{
				self->setCurSliderValue( val );
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
void LLMultiSliderCtrl::onSliderCommit( LLUICtrl* caller, void *userdata )
{
	LLMultiSliderCtrl* self = (LLMultiSliderCtrl*) userdata;
	//llassert( caller == self->mSlider );

	BOOL success = FALSE;
	F32 saved_val = self->mCurValue;
	F32 new_val = self->mMultiSlider->getCurSliderValue();

	if( self->mValidateCallback )
	{
		self->mCurValue = new_val;  // set the value temporarily so that the callback can retrieve it.
		if( self->mValidateCallback( self, self->mCallbackUserData ) )
		{
			success = TRUE;
		}
	}
	else
	{
		self->mCurValue = new_val;
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
	LLUICtrl::setEnabled( b );

	if( mLabelBox )
	{
		mLabelBox->setColor( b ? mTextEnabledColor : mTextDisabledColor );
	}

	mMultiSlider->setEnabled( b );

	if( mEditor )
	{
		mEditor->setEnabled( b );
	}

	if( mTextBox )
	{
		mTextBox->setColor( b ? mTextEnabledColor : mTextDisabledColor );
	}
}


void LLMultiSliderCtrl::setTentative(BOOL b)
{
	if( mEditor )
	{
		mEditor->setTentative(b);
	}
	LLUICtrl::setTentative(b);
}


void LLMultiSliderCtrl::onCommit()
{
	setTentative(FALSE);

	if( mEditor )
	{
		mEditor->setTentative(FALSE);
	}

	LLUICtrl::onCommit();
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

void LLMultiSliderCtrl::setSliderMouseDownCallback( void (*slider_mousedown_callback)(LLUICtrl* caller, void* userdata) )
{
	mSliderMouseDownCallback = slider_mousedown_callback;
	mMultiSlider->setMouseDownCallback( LLMultiSliderCtrl::onSliderMouseDown );
}

// static
void LLMultiSliderCtrl::onSliderMouseDown(LLUICtrl* caller, void* userdata)
{
	LLMultiSliderCtrl* self = (LLMultiSliderCtrl*) userdata;
	if( self->mSliderMouseDownCallback )
	{
		self->mSliderMouseDownCallback( self, self->mCallbackUserData );
	}
}


void LLMultiSliderCtrl::setSliderMouseUpCallback( void (*slider_mouseup_callback)(LLUICtrl* caller, void* userdata) )
{
	mSliderMouseUpCallback = slider_mouseup_callback;
	mMultiSlider->setMouseUpCallback( LLMultiSliderCtrl::onSliderMouseUp );
}

// static
void LLMultiSliderCtrl::onSliderMouseUp(LLUICtrl* caller, void* userdata)
{
	LLMultiSliderCtrl* self = (LLMultiSliderCtrl*) userdata;
	if( self->mSliderMouseUpCallback )
	{
		self->mSliderMouseUpCallback( self, self->mCallbackUserData );
	}
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

//virtual
LLString LLMultiSliderCtrl::getControlName() const
{
	return mMultiSlider->getControlName();
}

// virtual
void LLMultiSliderCtrl::setControlName(const LLString& control_name, LLView* context)
{
	mMultiSlider->setControlName(control_name, context);
}

// virtual
LLXMLNodePtr LLMultiSliderCtrl::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	node->createChild("show_text", TRUE)->setBoolValue(mShowText);

	node->createChild("can_edit_text", TRUE)->setBoolValue(mCanEditText);

	node->createChild("decimal_digits", TRUE)->setIntValue(mPrecision);

	if (mLabelBox)
	{
		node->createChild("label", TRUE)->setStringValue(mLabelBox->getText());
	}

	// TomY TODO: Do we really want to export the transient state of the slider?
	node->createChild("value", TRUE)->setFloatValue(mCurValue);

	if (mMultiSlider)
	{
		node->createChild("initial_val", TRUE)->setFloatValue(mMultiSlider->getInitialValue());
		node->createChild("min_val", TRUE)->setFloatValue(mMultiSlider->getMinValue());
		node->createChild("max_val", TRUE)->setFloatValue(mMultiSlider->getMaxValue());
		node->createChild("increment", TRUE)->setFloatValue(mMultiSlider->getIncrement());
	}
	addColorXML(node, mTextEnabledColor, "text_enabled_color", "LabelTextColor");
	addColorXML(node, mTextDisabledColor, "text_disabled_color", "LabelDisabledColor");

	return node;
}

LLView* LLMultiSliderCtrl::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("multi_slider");
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
	
	BOOL allow_overlap = FALSE;
	node->getAttributeBOOL("allow_overlap", allow_overlap);

	BOOL draw_track = TRUE;
	node->getAttributeBOOL("draw_track", draw_track);

	BOOL use_triangle = FALSE;
	node->getAttributeBOOL("use_triangle", use_triangle);

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

	S32 max_sliders = 1;
	node->getAttributeS32("max_sliders", max_sliders);


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

	LLMultiSliderCtrl* slider = new LLMultiSliderCtrl(name,
							rect,
							label,
							font,
							label_width,
							rect.getWidth() - text_left,
							show_text,
							can_edit_text,
							callback,
							NULL,
							initial_value,
							min_value, 
							max_value,
							increment,
							max_sliders,
							allow_overlap,
							draw_track,
							use_triangle);

	slider->setPrecision(precision);

	slider->initFromXML(node, parent);

	slider->updateText();
	
	return slider;
}

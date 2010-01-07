/** 
 * @file llcheckboxctrl.cpp
 * @brief LLCheckBoxCtrl base class
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

// The mutants are coming!
#include "linden_common.h"

#define LLCHECKBOXCTRL_CPP
#include "llcheckboxctrl.h"

#include "llgl.h"
#include "llui.h"
#include "lluiconstants.h"
#include "lluictrlfactory.h"
#include "llcontrol.h"

#include "llstring.h"
#include "llfontgl.h"
#include "lltextbox.h"
#include "llkeyboard.h"

const U32 MAX_STRING_LENGTH = 10;

static LLDefaultChildRegistry::Register<LLCheckBoxCtrl> r("check_box");

// Compiler optimization, generate extern template
template class LLCheckBoxCtrl* LLView::getChild<class LLCheckBoxCtrl>(
	const std::string& name, BOOL recurse) const;

LLCheckBoxCtrl::Params::Params()
:	text_enabled_color("text_enabled_color"),
	text_disabled_color("text_disabled_color"),
	initial_value("initial_value", false),
	label_text("label_text"),
	check_button("check_button"),
	radio_style("radio_style")
{}


LLCheckBoxCtrl::LLCheckBoxCtrl(const LLCheckBoxCtrl::Params& p)
:	LLUICtrl(p),
	mTextEnabledColor(p.text_enabled_color()),
	mTextDisabledColor(p.text_disabled_color()),
	mFont(p.font())
{
	mViewModel->setValue(LLSD(p.initial_value));
	mViewModel->resetDirty();
	static LLUICachedControl<S32> llcheckboxctrl_spacing ("UICheckboxctrlSpacing", 0);
	static LLUICachedControl<S32> llcheckboxctrl_hpad ("UICheckboxctrlHPad", 0);
	static LLUICachedControl<S32> llcheckboxctrl_vpad ("UICheckboxctrlVPad", 0);
	static LLUICachedControl<S32> llcheckboxctrl_btn_size ("UICheckboxctrlBtnSize", 0);

	// must be big enough to hold all children
	setUseBoundingRect(TRUE);

	// Label (add a little space to make sure text actually renders)
	const S32 FUDGE = 10;
	S32 text_width = mFont->getWidth( p.label ) + FUDGE;
	S32 text_height = llround(mFont->getLineHeight());
	LLRect label_rect;
	label_rect.setOriginAndSize(
		llcheckboxctrl_hpad + llcheckboxctrl_btn_size + llcheckboxctrl_spacing,
		llcheckboxctrl_vpad + 1, // padding to get better alignment
		text_width + llcheckboxctrl_hpad,
		text_height );

	// *HACK Get rid of this with SL-55508... 
	// this allows blank check boxes and radio boxes for now
	std::string local_label = p.label;
	if(local_label.empty())
	{
		local_label = " ";
	}

	LLTextBox::Params tbparams = p.label_text;
	tbparams.rect(label_rect);
	tbparams.initial_value(local_label);
	if (p.font.isProvided())
	{
		tbparams.font(p.font);
	}
	tbparams.text_color( p.enabled() ? p.text_enabled_color() : p.text_disabled_color() );
	mLabel = LLUICtrlFactory::create<LLTextBox> (tbparams);
	addChild(mLabel);

	// Button
	// Note: button cover the label by extending all the way to the right.
	LLRect btn_rect;
	btn_rect.setOriginAndSize(
		llcheckboxctrl_hpad,
		llcheckboxctrl_vpad,
		llcheckboxctrl_btn_size + llcheckboxctrl_spacing + text_width + llcheckboxctrl_hpad,
		llmax( text_height, llcheckboxctrl_btn_size() ) + llcheckboxctrl_vpad);
	std::string active_true_id, active_false_id;
	std::string inactive_true_id, inactive_false_id;

	LLButton::Params params = p.check_button;
	params.rect(btn_rect);
	//params.control_name(p.control_name);
	params.click_callback.function(boost::bind(&LLCheckBoxCtrl::onButtonPress, this, _2));
	params.commit_on_return(false);
	// Checkboxes only allow boolean initial values, but buttons can
	// take any LLSD.
	params.initial_value(LLSD(p.initial_value));
	params.follows.flags(FOLLOWS_LEFT | FOLLOWS_BOTTOM);

	mButton = LLUICtrlFactory::create<LLButton>(params);
	addChild(mButton);
}

LLCheckBoxCtrl::~LLCheckBoxCtrl()
{
	// Children all cleaned up by default view destructor.
}


// static
void LLCheckBoxCtrl::onButtonPress( const LLSD& data )
{
	//if (mRadioStyle)
	//{
	//	setValue(TRUE);
	//}

	onCommit();
}

void LLCheckBoxCtrl::onCommit()
{
	if( getEnabled() )
	{
		setTentative(FALSE);
		setControlValue(getValue());
		LLUICtrl::onCommit();
	}
}

void LLCheckBoxCtrl::setEnabled(BOOL b)
{
	LLView::setEnabled(b);

	if (b)
	{
		mLabel->setColor( mTextEnabledColor.get() );
	}
	else
	{
		mLabel->setColor( mTextDisabledColor.get() );
	}
}

void LLCheckBoxCtrl::clear()
{
	setValue( FALSE );
}

void LLCheckBoxCtrl::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	//stretch or shrink bounding rectangle of label when rebuilding UI at new scale
	static LLUICachedControl<S32> llcheckboxctrl_spacing ("UICheckboxctrlSpacing", 0);
	static LLUICachedControl<S32> llcheckboxctrl_hpad ("UICheckboxctrlHPad", 0);
	static LLUICachedControl<S32> llcheckboxctrl_vpad ("UICheckboxctrlVPad", 0);
	static LLUICachedControl<S32> llcheckboxctrl_btn_size ("UICheckboxctrlBtnSize", 0);

	const S32 FUDGE = 10;
	S32 text_width = mFont->getWidth( mLabel->getText() ) + FUDGE;
	S32 text_height = llround(mFont->getLineHeight());
	LLRect label_rect;
	label_rect.setOriginAndSize(
		llcheckboxctrl_hpad + llcheckboxctrl_btn_size + llcheckboxctrl_spacing,
		llcheckboxctrl_vpad,
		text_width,
		text_height );
	mLabel->setRect(label_rect);

	LLRect btn_rect;
	btn_rect.setOriginAndSize(
		llcheckboxctrl_hpad,
		llcheckboxctrl_vpad,
		llcheckboxctrl_btn_size + llcheckboxctrl_spacing + text_width,
		llmax( text_height, llcheckboxctrl_btn_size() ) );
	mButton->setRect( btn_rect );
	
	LLUICtrl::reshape(width, height, called_from_parent);
}

//virtual
void LLCheckBoxCtrl::setValue(const LLSD& value )
{
	mButton->setValue( value );
}

//virtual
LLSD LLCheckBoxCtrl::getValue() const
{
	return mButton->getValue();
}

//virtual
void LLCheckBoxCtrl::setTentative(BOOL b)
{
	mButton->setTentative(b);
}

//virtual
BOOL LLCheckBoxCtrl::getTentative() const
{
	return mButton->getTentative();
}

void LLCheckBoxCtrl::setLabel( const LLStringExplicit& label )
{
	mLabel->setText( label );
	reshape(getRect().getWidth(), getRect().getHeight(), FALSE);
}

std::string LLCheckBoxCtrl::getLabel() const
{
	return mLabel->getText();
}

BOOL LLCheckBoxCtrl::setLabelArg( const std::string& key, const LLStringExplicit& text )
{
	BOOL res = mLabel->setTextArg(key, text);
	reshape(getRect().getWidth(), getRect().getHeight(), FALSE);
	return res;
}

// virtual
void LLCheckBoxCtrl::setControlName(const std::string& control_name, LLView* context)
{
	mButton->setControlName(control_name, context);
}


// virtual		Returns TRUE if the user has modified this control.
BOOL	 LLCheckBoxCtrl::isDirty() const
{
	if ( mButton )
	{
		return mButton->isDirty();
	}
	return FALSE;		// Shouldn't get here
}


// virtual			Clear dirty state
void	LLCheckBoxCtrl::resetDirty()
{
	if ( mButton )
	{
		mButton->resetDirty();
	}
}

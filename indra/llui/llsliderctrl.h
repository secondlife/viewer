/** 
 * @file llsliderctrl.h
 * @brief Decorated wrapper for a LLSlider.
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

#ifndef LL_LLSLIDERCTRL_H
#define LL_LLSLIDERCTRL_H

#include "lluictrl.h"
#include "v4color.h"
#include "llslider.h"
#include "lltextbox.h"
#include "llrect.h"
#include "lllineeditor.h"


class LLSliderCtrl : public LLF32UICtrl
{
public:
	struct Params : public LLInitParam::Block<Params, LLF32UICtrl::Params>
	{
		Optional<std::string>   orientation;
		Optional<S32>			label_width;
		Optional<S32>			text_width;
		Optional<bool>			show_text;
		Optional<bool>			can_edit_text;
		Optional<bool>			is_volume_slider;
		Optional<S32>			decimal_digits;

		Optional<LLUIColor>		text_color,
								text_disabled_color;

		Optional<CommitCallbackParam>	mouse_down_callback,
										mouse_up_callback;

		Optional<LLSlider::Params>		slider_bar;
		Optional<LLLineEditor::Params>	value_editor;
		Optional<LLTextBox::Params>		value_text;
		Optional<LLTextBox::Params>		slider_label;

		Params()
		:	text_width("text_width"),
			label_width("label_width"),
			show_text("show_text"),
			can_edit_text("can_edit_text"),
			is_volume_slider("volume"),
			decimal_digits("decimal_digits", 3),
			text_color("text_color"),
			text_disabled_color("text_disabled_color"),
			slider_bar("slider_bar"),
			value_editor("value_editor"),
			value_text("value_text"),
			slider_label("slider_label"),
			mouse_down_callback("mouse_down_callback"),
			mouse_up_callback("mouse_up_callback"),
			orientation("orientation", std::string ("horizontal"))
		{}
	};
protected:
	LLSliderCtrl(const Params&);
	friend class LLUICtrlFactory;
public:
	virtual ~LLSliderCtrl() {} // Children all cleaned up by default view destructor.

	/*virtual*/ F32	getValueF32() const { return mSlider->getValueF32(); }
	void			setValue(F32 v, BOOL from_event = FALSE);

	/*virtual*/ void	setValue(const LLSD& value)	{ setValue((F32)value.asReal(), TRUE); }
	/*virtual*/ LLSD	getValue() const			{ return LLSD(getValueF32()); }
	/*virtual*/ BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text );

	BOOL			isMouseHeldDown() const { return mSlider->hasMouseCapture(); }

	virtual void	setPrecision(S32 precision);

	/*virtual*/ void    setEnabled( BOOL b );
	/*virtual*/ void	clear();

	/*virtual*/ void	setMinValue(const LLSD& min_value)  { setMinValue((F32)min_value.asReal()); }
	/*virtual*/ void	setMaxValue(const LLSD& max_value)  { setMaxValue((F32)max_value.asReal()); }
	/*virtual*/ void	setMinValue(F32 min_value)  { mSlider->setMinValue(min_value); updateText(); }
	/*virtual*/ void	setMaxValue(F32 max_value)  { mSlider->setMaxValue(max_value); updateText(); }
	/*virtual*/ void	setIncrement(F32 increment) { mSlider->setIncrement(increment);}

	F32				getMinValue() const { return mSlider->getMinValue(); }
	F32				getMaxValue() const { return mSlider->getMaxValue(); }

	void			setLabel(const LLStringExplicit& label)		{ if (mLabelBox) mLabelBox->setText(label); }
	void			setLabelColor(const LLColor4& c)			{ mTextEnabledColor = c; }
	void			setDisabledLabelColor(const LLColor4& c)	{ mTextDisabledColor = c; }

	boost::signals2::connection setSliderMouseDownCallback(	const commit_signal_t::slot_type& cb );
	boost::signals2::connection setSliderMouseUpCallback( const commit_signal_t::slot_type& cb );

	/*virtual*/ void	onTabInto();

	/*virtual*/ void	setTentative(BOOL b);			// marks value as tentative
	/*virtual*/ void	onCommit();						// mark not tentative, then commit

	/*virtual*/ void	setControlName(const std::string& control_name, LLView* context)
	{
		LLUICtrl::setControlName(control_name, context);
		mSlider->setControlName(control_name, context);
	}

	static void		onSliderCommit(LLUICtrl* caller, const LLSD& userdata);
	
	static void		onEditorCommit(LLUICtrl* ctrl, const LLSD& userdata);
	static void		onEditorGainFocus(LLFocusableElement* caller, void *userdata);
	static void		onEditorChangeFocus(LLUICtrl* caller, S32 direction, void *userdata);

private:
	void			updateText();
	void			reportInvalidData();

	const LLFontGL*	mFont;
	const LLFontGL*	mLabelFont;
	BOOL			mShowText;
	BOOL			mCanEditText;
	
	S32				mPrecision;
	LLTextBox*		mLabelBox;
	S32				mLabelWidth;
	
	F32				mValue;
	LLSlider*		mSlider;
	class LLLineEditor*	mEditor;
	LLTextBox*		mTextBox;

	LLUIColor	mTextEnabledColor;
	LLUIColor	mTextDisabledColor;
};

#endif  // LL_LLSLIDERCTRL_H


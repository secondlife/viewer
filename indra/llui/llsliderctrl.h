/** 
 * @file llsliderctrl.h
 * @brief Decorated wrapper for a LLSlider.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLSLIDERCTRL_H
#define LL_LLSLIDERCTRL_H

#include "lluictrl.h"
#include "v4color.h"
#include "llslider.h"
#include "lltextbox.h"
#include "llrect.h"
#include "lllineeditor.h"


class LLSliderCtrl: public LLF32UICtrl, public ll::ui::SearchableControl
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
	virtual ~LLSliderCtrl();

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
	boost::signals2::connection setSliderEditorCommitCallback( const commit_signal_t::slot_type& cb );

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

protected:
	virtual std::string _getSearchText() const
	{
		std::string strLabel;
		if( mLabelBox )
			strLabel = mLabelBox->getLabel();
		return strLabel + getToolTip();
	}
	virtual void onSetHighlight() const  // When highlight, really do highlight the label
	{
		if( mLabelBox )
			mLabelBox->ll::ui::SearchableControl::setHighlighted( ll::ui::SearchableControl::getHighlighted() );
	}
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

	commit_signal_t*	mEditorCommitSignal;
};

#endif  // LL_LLSLIDERCTRL_H


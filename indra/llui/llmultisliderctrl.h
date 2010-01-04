/** 
 * @file llmultisliderctrl.h
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

#ifndef LL_MULTI_SLIDERCTRL_H
#define LL_MULTI_SLIDERCTRL_H

#include "llf32uictrl.h"
#include "v4color.h"
#include "llmultislider.h"
#include "lltextbox.h"
#include "llrect.h"


//
// Classes
//
class LLFontGL;
class LLLineEditor;
class LLSlider;


class LLMultiSliderCtrl : public LLF32UICtrl
{
public:
	struct Params : public LLInitParam::Block<Params, LLF32UICtrl::Params>
	{
		Optional<S32>			label_width,
								text_width;
		Optional<bool>			show_text,
								can_edit_text;
		Optional<S32>			decimal_digits;
		Optional<S32>			max_sliders;	
		Optional<bool>			allow_overlap,
								draw_track,
								use_triangle;

		Optional<LLUIColor>		text_color,
								text_disabled_color;

		Optional<CommitCallbackParam>	mouse_down_callback,
										mouse_up_callback;

		Multiple<LLMultiSlider::SliderParams>	sliders;

		Params();
	};

protected:
	LLMultiSliderCtrl(const Params&);
	friend class LLUICtrlFactory;
public:
	virtual ~LLMultiSliderCtrl();

	F32				getSliderValue(const std::string& name) const;
	void			setSliderValue(const std::string& name, F32 v, BOOL from_event = FALSE);

	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const		{ return mMultiSlider->getValue(); }
	virtual BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text );

	const std::string& getCurSlider() const					{ return mMultiSlider->getCurSlider(); }
	F32				getCurSliderValue() const				{ return mCurValue; }
	void			setCurSlider(const std::string& name);		
	void			setCurSliderValue(F32 val, BOOL from_event = false) { setSliderValue(mMultiSlider->getCurSlider(), val, from_event); }

	virtual void	setMinValue(const LLSD& min_value)	{ setMinValue((F32)min_value.asReal()); }
	virtual void	setMaxValue(const LLSD& max_value)	{ setMaxValue((F32)max_value.asReal());  }

	BOOL			isMouseHeldDown();

	virtual void    setEnabled( BOOL b );
	virtual void	clear();
	virtual void	setPrecision(S32 precision);
	void			setMinValue(F32 min_value) {mMultiSlider->setMinValue(min_value);}
	void			setMaxValue(F32 max_value) {mMultiSlider->setMaxValue(max_value);}
	void			setIncrement(F32 increment) {mMultiSlider->setIncrement(increment);}

	/// for adding and deleting sliders
	const std::string&	addSlider();
	const std::string&	addSlider(F32 val);
	void			deleteSlider(const std::string& name);
	void			deleteCurSlider()			{ deleteSlider(mMultiSlider->getCurSlider()); }

	F32				getMinValue() const { return mMultiSlider->getMinValue(); }
	F32				getMaxValue() const { return mMultiSlider->getMaxValue(); }

	void			setLabel(const std::string& label)				{ if (mLabelBox) mLabelBox->setText(label); }
	void			setLabelColor(const LLColor4& c)			{ mTextEnabledColor = c; }
	void			setDisabledLabelColor(const LLColor4& c)	{ mTextDisabledColor = c; }

	boost::signals2::connection setSliderMouseDownCallback( const commit_signal_t::slot_type& cb );
	boost::signals2::connection setSliderMouseUpCallback( const commit_signal_t::slot_type& cb );

	virtual void	onTabInto();

	virtual void	setTentative(BOOL b);			// marks value as tentative
	virtual void	onCommit();						// mark not tentative, then commit

	virtual void		setControlName(const std::string& control_name, LLView* context);
	
	static void		onSliderCommit(LLUICtrl* caller, const LLSD& userdata);
	
	static void		onEditorCommit(LLUICtrl* ctrl, const LLSD& userdata);
	static void		onEditorGainFocus(LLFocusableElement* caller, void *userdata);
	static void		onEditorChangeFocus(LLUICtrl* caller, S32 direction, void *userdata);

private:
	void			updateText();
	void			reportInvalidData();

private:
	const LLFontGL*	mFont;
	BOOL			mShowText;
	BOOL			mCanEditText;

	S32				mPrecision;
	LLTextBox*		mLabelBox;
	S32				mLabelWidth;

	F32				mCurValue;
	LLMultiSlider*	mMultiSlider;
	LLLineEditor*	mEditor;
	LLTextBox*		mTextBox;

	LLUIColor	mTextEnabledColor;
	LLUIColor	mTextDisabledColor;
};

#endif  // LL_MULTI_SLIDERCTRL_H

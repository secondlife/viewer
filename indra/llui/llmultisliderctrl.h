/** 
 * @file llmultisliderctrl.h
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

#ifndef LL_MULTI_SLIDERCTRL_H
#define LL_MULTI_SLIDERCTRL_H

#include "lluictrl.h"
#include "v4color.h"
#include "llmultislider.h"
#include "lltextbox.h"
#include "llrect.h"

//
// Constants
//
const S32	MULTI_SLIDERCTRL_SPACING	=  4;			// space between label, slider, and text
const S32	MULTI_SLIDERCTRL_HEIGHT		=  16;

//
// Classes
//
class LLFontGL;
class LLLineEditor;
class LLSlider;


class LLMultiSliderCtrl : public LLUICtrl
{
public:
	LLMultiSliderCtrl(const LLString& name, 
		const LLRect& rect, 
		const LLString& label, 
		const LLFontGL* font,
		S32 slider_left,
		S32 text_left,
		BOOL show_text,
		BOOL can_edit_text,
		void (*commit_callback)(LLUICtrl*, void*),
		void* callback_userdata,
		F32 initial_value, F32 min_value, F32 max_value, F32 increment,
		S32 max_sliders, BOOL allow_overlap, BOOL draw_track,
		BOOL use_triangle,
		const LLString& control_which = LLString::null );

	virtual ~LLMultiSliderCtrl();
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	F32				getSliderValue(const LLString& name) const;
	void			setSliderValue(const LLString& name, F32 v, BOOL from_event = FALSE);

	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const		{ return mMultiSlider->getValue(); }
	virtual BOOL	setLabelArg( const LLString& key, const LLStringExplicit& text );

	const LLString& getCurSlider() const					{ return mMultiSlider->getCurSlider(); }
	F32				getCurSliderValue() const				{ return mCurValue; }
	void			setCurSlider(const LLString& name);		
	void			setCurSliderValue(F32 val, BOOL from_event = false) { setSliderValue(mMultiSlider->getCurSlider(), val, from_event); }

	virtual void	setMinValue(LLSD min_value)	{ setMinValue((F32)min_value.asReal()); }
	virtual void	setMaxValue(LLSD max_value)	{ setMaxValue((F32)max_value.asReal());  }

	BOOL			isMouseHeldDown();

	virtual void    setEnabled( BOOL b );
	virtual void	clear();
	virtual void	setPrecision(S32 precision);
	void			setMinValue(F32 min_value) {mMultiSlider->setMinValue(min_value);}
	void			setMaxValue(F32 max_value) {mMultiSlider->setMaxValue(max_value);}
	void			setIncrement(F32 increment) {mMultiSlider->setIncrement(increment);}

	/// for adding and deleting sliders
	const LLString&	addSlider();
	const LLString&	addSlider(F32 val);
	void			deleteSlider(const LLString& name);
	void			deleteCurSlider()			{ deleteSlider(mMultiSlider->getCurSlider()); }

	F32				getMinValue() { return mMultiSlider->getMinValue(); }
	F32				getMaxValue() { return mMultiSlider->getMaxValue(); }

	void			setLabel(const LLString& label)				{ if (mLabelBox) mLabelBox->setText(label); }
	void			setLabelColor(const LLColor4& c)			{ mTextEnabledColor = c; }
	void			setDisabledLabelColor(const LLColor4& c)	{ mTextDisabledColor = c; }

	void			setSliderMouseDownCallback(	void (*slider_mousedown_callback)(LLUICtrl* caller, void* userdata) );
	void			setSliderMouseUpCallback(	void (*slider_mouseup_callback)(LLUICtrl* caller, void* userdata) );

	virtual void	onTabInto();

	virtual void	setTentative(BOOL b);			// marks value as tentative
	virtual void	onCommit();						// mark not tentative, then commit

	virtual void		setControlName(const LLString& control_name, LLView* context);
	virtual LLString 	getControlName() const;
	
	static void		onSliderCommit(LLUICtrl* caller, void* userdata);
	static void		onSliderMouseDown(LLUICtrl* caller,void* userdata);
	static void		onSliderMouseUp(LLUICtrl* caller,void* userdata);

	static void		onEditorCommit(LLUICtrl* caller, void* userdata);
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

	LLColor4		mTextEnabledColor;
	LLColor4		mTextDisabledColor;

	void			(*mSliderMouseUpCallback)( LLUICtrl* ctrl, void* userdata );
	void			(*mSliderMouseDownCallback)( LLUICtrl* ctrl, void* userdata );
};

#endif  // LL_MULTI_SLIDERCTRL_H

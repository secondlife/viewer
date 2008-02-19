/** 
 * @file llsliderctrl.h
 * @brief Decorated wrapper for a LLSlider.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#ifndef LL_LLSLIDERCTRL_H
#define LL_LLSLIDERCTRL_H

#include "lluictrl.h"
#include "v4color.h"
#include "llslider.h"
#include "lltextbox.h"
#include "llrect.h"

//
// Constants
//
const S32	SLIDERCTRL_SPACING		=  4;				// space between label, slider, and text
const S32	SLIDERCTRL_HEIGHT		=  16;


class LLSliderCtrl : public LLUICtrl
{
public:
	LLSliderCtrl(const LLString& name, 
		const LLRect& rect, 
		const LLString& label, 
		const LLFontGL* font,
		S32 slider_left,
		S32 text_left,
		BOOL show_text,
		BOOL can_edit_text,
		BOOL volume, //TODO: create a "volume" slider sub-class or just use image art, no?  -MG
		void (*commit_callback)(LLUICtrl*, void*),
		void* callback_userdata,
		F32 initial_value, F32 min_value, F32 max_value, F32 increment,
		const LLString& control_which = LLString::null );

	virtual ~LLSliderCtrl() {} // Children all cleaned up by default view destructor.
	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_SLIDER; }
	virtual LLString getWidgetTag() const { return LL_SLIDER_CTRL_TAG; }
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	F32				getValueF32() const { return mSlider->getValueF32(); }
	void			setValue(F32 v, BOOL from_event = FALSE);

	virtual void	setValue(const LLSD& value)	{ setValue((F32)value.asReal(), TRUE); }
	virtual LLSD	getValue() const			{ return LLSD(getValueF32()); }
	virtual BOOL	setLabelArg( const LLString& key, const LLStringExplicit& text );

	virtual void	setMinValue(LLSD min_value)	{ setMinValue((F32)min_value.asReal()); }
	virtual void	setMaxValue(LLSD max_value)	{ setMaxValue((F32)max_value.asReal()); }

	BOOL			isMouseHeldDown() const { return mSlider->hasMouseCapture(); }

	virtual void    setEnabled( BOOL b );
	virtual void	clear();
	virtual void	setPrecision(S32 precision);
	void			setMinValue(F32 min_value)	{ mSlider->setMinValue(min_value); }
	void			setMaxValue(F32 max_value)	{ mSlider->setMaxValue(max_value); }
	void			setIncrement(F32 increment)	{ mSlider->setIncrement(increment); }

	F32				getMinValue() { return mSlider->getMinValue(); }
	F32				getMaxValue() { return mSlider->getMaxValue(); }

	void			setLabel(const LLStringExplicit& label)		{ if (mLabelBox) mLabelBox->setText(label); }
	void			setLabelColor(const LLColor4& c)			{ mTextEnabledColor = c; }
	void			setDisabledLabelColor(const LLColor4& c)	{ mTextDisabledColor = c; }

	void			setSliderMouseDownCallback(	void (*slider_mousedown_callback)(LLUICtrl* caller, void* userdata) );
	void			setSliderMouseUpCallback(	void (*slider_mouseup_callback)(LLUICtrl* caller, void* userdata) );

	virtual void	onTabInto();

	virtual void	setTentative(BOOL b);			// marks value as tentative
	virtual void	onCommit();						// mark not tentative, then commit

	virtual void		setControlName(const LLString& control_name, LLView* context) { mSlider->setControlName(control_name, context); }
	virtual LLString 	getControlName() const { return mSlider->getControlName(); }
	
	static void		onSliderCommit(LLUICtrl* caller, void* userdata);
	static void		onSliderMouseDown(LLUICtrl* caller,void* userdata);
	static void		onSliderMouseUp(LLUICtrl* caller,void* userdata);

	static void		onEditorCommit(LLUICtrl* caller, void* userdata);
	static void		onEditorGainFocus(LLFocusableElement* caller, void *userdata);
	static void		onEditorChangeFocus(LLUICtrl* caller, S32 direction, void *userdata);

private:
	void			updateText();
	void			reportInvalidData();

	const LLFontGL*	mFont;
	BOOL			mShowText;
	BOOL			mCanEditText;
	BOOL			mVolumeSlider;
	
	S32				mPrecision;
	LLTextBox*		mLabelBox;
	S32				mLabelWidth;
	
	F32				mValue;
	LLSlider*		mSlider;
	class LLLineEditor*	mEditor;
	LLTextBox*		mTextBox;

	LLColor4		mTextEnabledColor;
	LLColor4		mTextDisabledColor;

	void			(*mSliderMouseUpCallback)( LLUICtrl* ctrl, void* userdata );
	void			(*mSliderMouseDownCallback)( LLUICtrl* ctrl, void* userdata );
};

#endif  // LL_LLSLIDERCTRL_H

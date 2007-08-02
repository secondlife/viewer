/** 
 * @file llsliderctrl.h
 * @brief LLSliderCtrl base class
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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

//
// Classes
//
class LLFontGL;
class LLLineEditor;
class LLSlider;


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
		BOOL volume,		 
		void (*commit_callback)(LLUICtrl*, void*),
		void* callback_userdata,
		F32 initial_value, F32 min_value, F32 max_value, F32 increment,
		const LLString& control_which = LLString::null );

	virtual ~LLSliderCtrl();
	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_SLIDER; }
	virtual LLString getWidgetTag() const { return LL_SLIDER_CTRL_TAG; }
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	F32				getValueF32() const;
	void			setValue(F32 v, BOOL from_event = FALSE);

	virtual void	setValue(const LLSD& value )	{ setValue((F32)value.asReal(), TRUE); }
	virtual LLSD	getValue() const		{ return LLSD(getValueF32()); }
	virtual BOOL	setLabelArg( const LLString& key, const LLString& text );

	virtual void	setMinValue(LLSD min_value)	{ setMinValue((F32)min_value.asReal()); }
	virtual void	setMaxValue(LLSD max_value)	{ setMaxValue((F32)max_value.asReal());  }

	BOOL			isMouseHeldDown();

	virtual void    setEnabled( BOOL b );
	virtual void	clear();
	virtual void	setPrecision(S32 precision);
	void			setMinValue(F32 min_value) {mSlider->setMinValue(min_value);}
	void			setMaxValue(F32 max_value) {mSlider->setMaxValue(max_value);}
	void			setIncrement(F32 increment) {mSlider->setIncrement(increment);}

	F32				getMinValue() { return mSlider->getMinValue(); }
	F32				getMaxValue() { return mSlider->getMaxValue(); }

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
	static void		onEditorGainFocus(LLUICtrl* caller, void *userdata);
	static void		onEditorChangeFocus(LLUICtrl* caller, S32 direction, void *userdata);

private:
	void			updateText();
	void			reportInvalidData();

private:
	const LLFontGL*	mFont;
	BOOL			mShowText;
	BOOL			mCanEditText;
	BOOL			mVolumeSlider;
	
	S32				mPrecision;
	LLTextBox*		mLabelBox;
	S32				mLabelWidth;
	
	F32				mValue;
	LLSlider*		mSlider;
	LLLineEditor*	mEditor;
	LLTextBox*		mTextBox;

	LLColor4		mTextEnabledColor;
	LLColor4		mTextDisabledColor;

	void			(*mSliderMouseUpCallback)( LLUICtrl* ctrl, void* userdata );
	void			(*mSliderMouseDownCallback)( LLUICtrl* ctrl, void* userdata );
};

#endif  // LL_LLSLIDERCTRL_H

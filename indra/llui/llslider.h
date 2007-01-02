/** 
 * @file llslider.h
 * @brief A simple slider with no label.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLSLIDER_H
#define LL_LLSLIDER_H

#include "lluictrl.h"
#include "v4color.h"

class LLSlider : public LLUICtrl
{
public:
	LLSlider( 
		const LLString& name,
		const LLRect& rect,
		void (*on_commit_callback)(LLUICtrl* ctrl, void* userdata),
		void* callback_userdata,
		F32 initial_value,
		F32 min_value,
		F32 max_value,
		F32 increment,
		const LLString& control_name = LLString::null );

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static  LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	void			setValue( F32 value, BOOL from_event = FALSE );
	F32				getValueF32() const;

	virtual void	setValue(const LLSD& value )	{ setValue((F32)value.asReal(), TRUE); }
	virtual LLSD	getValue() const		{ return LLSD(getValueF32()); }

	virtual void	setMinValue(LLSD min_value)	{ setMinValue((F32)min_value.asReal()); }
	virtual void	setMaxValue(LLSD max_value)	{ setMaxValue((F32)max_value.asReal());  }

	F32				getInitialValue() const { return mInitialValue; }
	F32				getMinValue() const		{ return mMinValue; }
	F32				getMaxValue() const		{ return mMaxValue; }
	F32				getIncrement() const	{ return mIncrement; }
	void			setMinValue(F32 min_value) {mMinValue = min_value;}
	void			setMaxValue(F32 max_value) {mMaxValue = max_value;}
	void			setIncrement(F32 increment) {mIncrement = increment;}
	void			setMouseDownCallback( void (*cb)(LLUICtrl* ctrl, void* userdata) ) { mMouseDownCallback = cb; }
	void			setMouseUpCallback(	void (*cb)(LLUICtrl* ctrl, void* userdata) ) { mMouseUpCallback = cb; }

	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);
	virtual void	draw();

protected:
	F32				mValue;
	F32				mInitialValue;
	F32				mMinValue;
	F32				mMaxValue;
	F32				mIncrement;

	S32				mMouseOffset;
	LLRect			mDragStartThumbRect;

	LLRect			mThumbRect;
	LLColor4		mTrackColor;
	LLColor4		mThumbOutlineColor;
	LLColor4		mThumbCenterColor;
	LLColor4		mDisabledThumbColor;
	
	void			(*mMouseDownCallback)(LLUICtrl* ctrl, void* userdata);
	void			(*mMouseUpCallback)(LLUICtrl* ctrl, void* userdata);
};

#endif  // LL_LLSLIDER_H

/** 
 * @file llspinctrl.h
 * @brief LLSpinCtrl base class
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLSPINCTRL_H
#define LL_LLSPINCTRL_H


#include "stdtypes.h"
#include "lluictrl.h"
#include "v4color.h"
#include "llrect.h"

//
// Constants
//
const S32	SPINCTRL_BTN_HEIGHT		=  8;
const S32	SPINCTRL_BTN_WIDTH		= 16;
const S32	SPINCTRL_SPACING		=  2;							// space between label right and button left
const S32	SPINCTRL_HEIGHT			=  2 * SPINCTRL_BTN_HEIGHT;
const S32	SPINCTRL_DEFAULT_LABEL_WIDTH = 10;

//
// Classes
//
class LLFontGL;
class LLButton;
class LLTextBox;
class LLLineEditor;


class LLSpinCtrl
: public LLUICtrl
{
public:
	LLSpinCtrl(const LLString& name, const LLRect& rect,
		const LLString& label,
		const LLFontGL* font,
		void (*commit_callback)(LLUICtrl*, void*),
		void* callback_userdata,
		F32 initial_value, F32 min_value, F32 max_value, F32 increment,
		const LLString& control_name = LLString(),
		S32 label_width = SPINCTRL_DEFAULT_LABEL_WIDTH );

	virtual ~LLSpinCtrl();
	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_SPINNER; }
	virtual LLString getWidgetTag() const { return LL_SPIN_CTRL_TAG; }
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const;
			F32		get() { return (F32)getValue().asReal(); }
			void	set(F32 value) { setValue(value); }

	virtual void	setMinValue(LLSD min_value)	{ setMinValue((F32)min_value.asReal()); }
	virtual void	setMaxValue(LLSD max_value)	{ setMaxValue((F32)max_value.asReal());  }

	BOOL			isMouseHeldDown();

	virtual void    setEnabled( BOOL b );
	virtual void	setFocus( BOOL b );
	virtual void	clear();
	virtual void	setPrecision(S32 precision);
	virtual void	setMinValue(F32 min)			{ mMinValue = min; }
	virtual void	setMaxValue(F32 max)			{ mMaxValue = max; }
	virtual void	setIncrement(F32 inc)			{ mIncrement = inc; }

	void			setLabel(const LLString& label);
	void			setLabelColor(const LLColor4& c)			{ mTextEnabledColor = c; }
	void			setDisabledLabelColor(const LLColor4& c)	{ mTextDisabledColor = c; }

	virtual void	onTabInto();

	virtual void	setTentative(BOOL b);			// marks value as tentative
	virtual void	onCommit();						// mark not tentative, then commit

	void 			forceEditorCommit();			// for commit on external button

	virtual BOOL	handleScrollWheel(S32 x,S32 y,S32 clicks);
	virtual BOOL	handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);

	virtual void	draw();

	static void		onEditorCommit(LLUICtrl* caller, void* userdata);
	static void		onEditorGainFocus(LLUICtrl* caller, void *userdata);
	static void		onEditorChangeFocus(LLUICtrl* caller, S32 direction, void *userdata);

	static void		onUpBtn(void *userdata);
	static void		onDownBtn(void *userdata);

protected:
	void			updateEditor();
	void			reportInvalidData();

protected:

	F32				mValue;
	F32				mInitialValue;
	F32				mMaxValue;
	F32				mMinValue;
	F32				mIncrement;

	S32				mPrecision;
	LLTextBox*		mLabelBox;

	LLLineEditor*	mEditor;
	LLColor4		mTextEnabledColor;
	LLColor4		mTextDisabledColor;

	LLButton*		mUpBtn;
	LLButton*		mDownBtn;

	BOOL			mbHasBeenSet;
};

#endif  // LL_LLSPINCTRL_H

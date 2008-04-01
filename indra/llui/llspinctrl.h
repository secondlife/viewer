/** 
 * @file llspinctrl.h
 * @brief Typical spinner with "up" and "down" arrow buttons.
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

	virtual ~LLSpinCtrl() {} // Children all cleaned up by default view destructor.

	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, class LLUICtrlFactory *factory);

	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const { return mValue; }
			F32		get() const { return (F32)getValue().asReal(); }
			void	set(F32 value) { setValue(value); mInitialValue = value; }

	virtual void	setMinValue(LLSD min_value)	{ setMinValue((F32)min_value.asReal()); }
	virtual void	setMaxValue(LLSD max_value)	{ setMaxValue((F32)max_value.asReal());  }

	BOOL			isMouseHeldDown() const;

	virtual void    setEnabled( BOOL b );
	virtual void	setFocus( BOOL b );
	virtual void	clear();
	virtual BOOL	isDirty() const { return( mValue != mInitialValue ); }

	virtual void	setPrecision(S32 precision);
	virtual void	setMinValue(F32 min)			{ mMinValue = min; }
	virtual void	setMaxValue(F32 max)			{ mMaxValue = max; }
	virtual void	setIncrement(F32 inc)			{ mIncrement = inc; }
	virtual F32		getMinValue()			{ return mMinValue ; }
	virtual F32 	getMaxValue()			{ return mMaxValue ; }

	void			setLabel(const LLStringExplicit& label);
	void			setLabelColor(const LLColor4& c)			{ mTextEnabledColor = c; }
	void			setDisabledLabelColor(const LLColor4& c)	{ mTextDisabledColor = c; }

	virtual void	onTabInto();

	virtual void	setTentative(BOOL b);			// marks value as tentative
	virtual void	onCommit();						// mark not tentative, then commit

	void 			forceEditorCommit();			// for commit on external button

	virtual BOOL	handleScrollWheel(S32 x,S32 y,S32 clicks);
	virtual BOOL	handleKeyHere(KEY key, MASK mask);

	virtual void	draw();

	static void		onEditorCommit(LLUICtrl* caller, void* userdata);
	static void		onEditorGainFocus(LLFocusableElement* caller, void *userdata);
	static void		onEditorChangeFocus(LLUICtrl* caller, S32 direction, void *userdata);

	static void		onUpBtn(void *userdata);
	static void		onDownBtn(void *userdata);

private:
	void			updateEditor();
	void			reportInvalidData();

	F32				mValue;
	F32				mInitialValue;
	F32				mMaxValue;
	F32				mMinValue;
	F32				mIncrement;

	S32				mPrecision;
	class LLTextBox*	mLabelBox;

	class LLLineEditor*	mEditor;
	LLColor4		mTextEnabledColor;
	LLColor4		mTextDisabledColor;

	class LLButton*		mUpBtn;
	class LLButton*		mDownBtn;

	BOOL			mbHasBeenSet;
};

#endif  // LL_LLSPINCTRL_H

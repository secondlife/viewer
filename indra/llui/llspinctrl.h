/** 
 * @file llspinctrl.h
 * @brief Typical spinner with "up" and "down" arrow buttons.
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

#ifndef LL_LLSPINCTRL_H
#define LL_LLSPINCTRL_H


#include "stdtypes.h"
#include "llbutton.h"
#include "llf32uictrl.h"
#include "v4color.h"
#include "llrect.h"


class LLSpinCtrl
: public LLF32UICtrl
{
public:
	struct Params : public LLInitParam::Block<Params, LLF32UICtrl::Params>
	{
		Optional<S32> label_width;
		Optional<U32> decimal_digits;
		Optional<bool> allow_text_entry;

		Optional<LLUIColor> text_enabled_color;
		Optional<LLUIColor> text_disabled_color;

		Optional<LLButton::Params> up_button;
		Optional<LLButton::Params> down_button;

		Params();
	};
protected:
	LLSpinCtrl(const Params&);
	friend class LLUICtrlFactory;
public:
	virtual ~LLSpinCtrl() {} // Children all cleaned up by default view destructor.

	virtual void    forceSetValue(const LLSD& value ) ;
	virtual void	setValue(const LLSD& value );
			F32		get() const { return getValueF32(); }
			void	set(F32 value) { setValue(value); mInitialValue = value; }

	BOOL			isMouseHeldDown() const;

	virtual void    setEnabled( BOOL b );
	virtual void	setFocus( BOOL b );
	virtual void	clear();
	virtual BOOL	isDirty() const { return( getValueF32() != mInitialValue ); }
	virtual void    resetDirty() { mInitialValue = getValueF32(); }

	virtual void	setPrecision(S32 precision);

	void			setLabel(const LLStringExplicit& label);
	void			setLabelColor(const LLColor4& c)			{ mTextEnabledColor = c; updateLabelColor(); }
	void			setDisabledLabelColor(const LLColor4& c)	{ mTextDisabledColor = c; updateLabelColor();}
	void			setAllowEdit(BOOL allow_edit);

	virtual void	onTabInto();

	virtual void	setTentative(BOOL b);			// marks value as tentative
	virtual void	onCommit();						// mark not tentative, then commit

	void 			forceEditorCommit();			// for commit on external button

	virtual BOOL	handleScrollWheel(S32 x,S32 y,S32 clicks);
	virtual BOOL	handleKeyHere(KEY key, MASK mask);

	void			onEditorCommit(const LLSD& data);
	static void		onEditorGainFocus(LLFocusableElement* caller, void *userdata);
	static void		onEditorChangeFocus(LLUICtrl* caller, S32 direction, void *userdata);

	void			onUpBtn(const LLSD& data);
	void			onDownBtn(const LLSD& data);

private:
	void			updateLabelColor();
	void			updateEditor();
	void			reportInvalidData();

	S32				mPrecision;
	class LLTextBox*	mLabelBox;

	class LLLineEditor*	mEditor;
	LLUIColor	mTextEnabledColor;
	LLUIColor	mTextDisabledColor;

	class LLButton*		mUpBtn;
	class LLButton*		mDownBtn;

	BOOL			mbHasBeenSet;
	BOOL			mAllowEdit;
};

#endif  // LL_LLSPINCTRL_H

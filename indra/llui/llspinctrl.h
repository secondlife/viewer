/** 
 * @file llspinctrl.h
 * @brief Typical spinner with "up" and "down" arrow buttons.
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
		Optional<bool> label_wrap;

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

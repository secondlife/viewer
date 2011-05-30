/**
 * @file lltimectrl.h
 * @brief Time control
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LLTIMECTRL_H_
#define LLTIMECTRL_H_

#include "stdtypes.h"
#include "llbutton.h"
#include "v4color.h"
#include "llrect.h"

class LLLineEditor;

class LLTimeCtrl
: public LLUICtrl
{
	LOG_CLASS(LLTimeCtrl);
public:
	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<S32> label_width;
		Optional<bool> allow_text_entry;

		Optional<LLUIColor> text_enabled_color;
		Optional<LLUIColor> text_disabled_color;

		Optional<LLButton::Params> up_button;
		Optional<LLButton::Params> down_button;

		Params();
	};

	F32 getTime24() const;		// 0.0 - 24.0
	U32 getHours24() const;		// 0 - 23
	U32 getMinutes() const;		// 0 - 59

	void setTime24(F32 time);	// 0.0 - 23.98(3)

protected:
	LLTimeCtrl(const Params&);
	friend class LLUICtrlFactory;

private:

	enum EDayPeriod
	{
		AM,
		PM
	};

	enum EEditingPart
	{
		HOURS,
		MINUTES,
		DAYPART,
		NONE
	};

	virtual void	onFocusLost();
	virtual BOOL	handleKeyHere(KEY key, MASK mask);

	void	onUpBtn();
	void	onDownBtn();

	void	onTextEntry(LLLineEditor* line_editor);

	void	validateHours(const LLWString& wstr);
	void	validateMinutes(const LLWString& wstr);
	bool	isTimeStringValid(const LLWString& wstr);

	bool	isPMAMStringValid(const LLWString& wstr);
	bool	isHoursStringValid(const LLWString& wstr);
	bool 	isMinutesStringValid(const LLWString& wstr);

	LLWString getHoursWString(const LLWString& wstr);
	LLWString getMinutesWString(const LLWString& wstr);

	void increaseMinutes();
	void increaseHours();

	void decreaseMinutes();
	void decreaseHours();

	void switchDayPeriod();

	void updateText();

	EEditingPart getEditingPart();

	class LLTextBox*	mLabelBox;

	class LLLineEditor*	mEditor;
	LLUIColor			mTextEnabledColor;
	LLUIColor			mTextDisabledColor;

	class LLButton*		mUpBtn;
	class LLButton*		mDownBtn;

	U32 			mHours;				// 1 - 12
	U32 			mMinutes;			// 0 - 59
	EDayPeriod 		mCurrentDayPeriod;	// AM/PM

	BOOL			mAllowEdit;
};
#endif /* LLTIMECTRL_H_ */

/**
 * @file   llf32uictrl.h
 * @author Nat Goodspeed
 * @date   2008-09-08
 * @brief  Base class for float-valued LLUICtrl widgets
 * 
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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

#if ! defined(LL_LLF32UICTRL_H)
#define LL_LLF32UICTRL_H

#include "lluictrl.h"

class LLF32UICtrl: public LLUICtrl
{
public:
    struct Params: public LLInitParam::Block<Params, LLUICtrl::Params>
    {
		Optional<F32>	min_value,
						max_value,
						increment;

		Params()
		:	min_value("min_val", 0.f),
			max_value("max_val", 1.f),
			increment("increment", 0.1f)
        {}			
    };

protected:
    LLF32UICtrl(const Params& p);

public:
	virtual F32		getValueF32() const;

	virtual void	setValue(const LLSD& value ) { mViewModel->setValue(value); }
	virtual LLSD	getValue() const		{ return LLSD(getValueF32()); }

	virtual void	setMinValue(const LLSD& min_value)	{ setMinValue((F32)min_value.asReal()); }
	virtual void	setMaxValue(const LLSD& max_value)	{ setMaxValue((F32)max_value.asReal()); }

	virtual F32		getInitialValue() const { return mInitialValue; }
	virtual F32		getMinValue() const		{ return mMinValue; }
	virtual F32		getMaxValue() const		{ return mMaxValue; }
	virtual F32		getIncrement() const	{ return mIncrement; }
	virtual void	setMinValue(F32 min_value) { mMinValue = min_value; }
	virtual void	setMaxValue(F32 max_value) { mMaxValue = max_value; }
	virtual void	setIncrement(F32 increment) { mIncrement = increment;}

protected:
	F32				mInitialValue;
	F32				mMinValue;
	F32				mMaxValue;
	F32				mIncrement;
};

#endif /* ! defined(LL_LLF32UICTRL_H) */

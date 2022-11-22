/**
 * @file   llf32uictrl.h
 * @author Nat Goodspeed
 * @date   2008-09-08
 * @brief  Base class for float-valued LLUICtrl widgets
 * 
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

    virtual void    setValue(const LLSD& value ) { LLUICtrl::setValue(value); }
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

/**
 * @file   llf32uictrl.cpp
 * @author Nat Goodspeed
 * @date   2008-09-08
 * @brief  Implementation for llf32uictrl.
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

// Precompiled header
#include "linden_common.h"
// associated header
#include "llf32uictrl.h"
// STL headers
// std headers
// external library headers
// other Linden headers

LLF32UICtrl::LLF32UICtrl(const Params& p)
:	LLUICtrl(p),
	mInitialValue(p.initial_value().asReal()),
	mMinValue(p.min_value),
	mMaxValue(p.max_value),
    mIncrement(p.increment)
{
	mViewModel->setValue(p.initial_value);
}

F32 LLF32UICtrl::getValueF32() const
{
    return mViewModel->getValue().asReal();
}

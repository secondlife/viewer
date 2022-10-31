/**
 * @file   llf32uictrl.cpp
 * @author Nat Goodspeed
 * @date   2008-09-08
 * @brief  Implementation for llf32uictrl.
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

// Precompiled header
#include "linden_common.h"
// associated header
#include "llf32uictrl.h"
// STL headers
// std headers
// external library headers
// other Linden headers

LLF32UICtrl::LLF32UICtrl(const Params& p)
:   LLUICtrl(p),
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

/**
 * @file   llfloaterreglistener.h
 * @author Nat Goodspeed
 * @date   2009-08-12
 * @brief  Wrap (subset of) LLFloaterReg API with an event API
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#if ! defined(LL_LLFLOATERREGLISTENER_H)
#define LL_LLFLOATERREGLISTENER_H

#include "lleventapi.h"
#include <string>

class LLSD;

/// Event API wrapper for LLFloaterReg
class LLFloaterRegListener: public LLEventAPI
{
public:
    /// As all public LLFloaterReg methods are static, there's no point in
    /// binding an LLFloaterReg instance.
    LLFloaterRegListener();

private:
    void getBuildMap(const LLSD& event) const;
    void showInstance(const LLSD& event) const;
    void hideInstance(const LLSD& event) const;
    void toggleInstance(const LLSD& event) const;
    void instanceVisible(const LLSD& event) const;
    void clickButton(const LLSD& event) const;
};

#endif /* ! defined(LL_LLFLOATERREGLISTENER_H) */

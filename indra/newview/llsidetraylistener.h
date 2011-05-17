/**
 * @file   llsidetraylistener.h
 * @author Nat Goodspeed
 * @date   2011-02-15
 * @brief  
 * 
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
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

#if ! defined(LL_LLSIDETRAYLISTENER_H)
#define LL_LLSIDETRAYLISTENER_H

#include "lleventapi.h"
#include <boost/function.hpp>

class LLSideTray;
class LLSD;

class LLSideTrayListener: public LLEventAPI
{
    typedef boost::function<LLSideTray*()> Getter;

public:
    LLSideTrayListener(const Getter& getter);

private:
    void getCollapsed(const LLSD& event) const;
    void getTabs(const LLSD& event) const;
    void getPanels(const LLSD& event) const;

    Getter mGetter;
};

#endif /* ! defined(LL_LLSIDETRAYLISTENER_H) */

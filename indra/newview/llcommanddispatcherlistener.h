/**
 * @file   llcommanddispatcherlistener.h
 * @author Nat Goodspeed
 * @date   2009-12-10
 * @brief  LLEventAPI for LLCommandDispatcher
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

#if ! defined(LL_LLCOMMANDDISPATCHERLISTENER_H)
#define LL_LLCOMMANDDISPATCHERLISTENER_H

#include "lleventapi.h"
class LLCommandDispatcher;
class LLSD;

class LLCommandDispatcherListener: public LLEventAPI
{
public:
    LLCommandDispatcherListener(/* LLCommandDispatcher* instance */); // all static members

private:
    void dispatch(const LLSD& params) const;
    void enumerate(const LLSD& params) const;

    //LLCommandDispatcher* mDispatcher;
};

#endif /* ! defined(LL_LLCOMMANDDISPATCHERLISTENER_H) */

/**
 * @file   llpanelloginlistener.h
 * @author Nat Goodspeed
 * @date   2009-12-10
 * @brief  LLEventAPI for LLPanelLogin
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

#if ! defined(LL_LLPANELLOGINLISTENER_H)
#define LL_LLPANELLOGINLISTENER_H

#include "lleventapi.h"
class LLPanelLogin;
class LLSD;

class LLPanelLoginListener: public LLEventAPI
{
public:
    LLPanelLoginListener(LLPanelLogin* instance);

private:
    void onClickConnect(const LLSD&) const;

    LLPanelLogin* mPanel;
};

#endif /* ! defined(LL_LLPANELLOGINLISTENER_H) */

/**
 * @file   lluilistener.h
 * @author Nat Goodspeed
 * @date   2009-08-18
 * @brief  Engage named functions as specified by XUI
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

#if ! defined(LL_LLUILISTENER_H)
#define LL_LLUILISTENER_H

#include "lleventapi.h"
#include <string>

class LLSD;

class LLUIListener: public LLEventAPI
{
public:
    LLUIListener();

private:
    void call(const LLSD& event);
    void callables(const LLSD& event) const;
    void getValue(const LLSD& event) const;
    void getTopMenus(const LLSD& event) const;

    void addMenu(const LLSD&event) const;
    void addMenuBranch(const LLSD&event) const;
    void addMenuItem(const LLSD&event) const;
    void addMenuSeparator(const LLSD&event) const;
    void setMenuVisible(const LLSD &event) const;

    void restoreDefaultToolbars(const LLSD &event) const;
    void clearAllToolbars(const LLSD &event) const;
    void addToolbarBtn(const LLSD &event) const;
    void removeToolbarBtn(const LLSD &event) const;
    void getToolbarBtnNames(const LLSD &event) const;

    void closeAllFloaters(const LLSD &event) const;

    F64 mLastUntrustedThrottle {0};
    F64 mLastMinThrottle {0};
};

#endif /* ! defined(LL_LLUILISTENER_H) */

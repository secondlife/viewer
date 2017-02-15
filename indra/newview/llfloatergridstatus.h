/** 
 * @file llfloatergridstatus.h
 * @brief Grid status floater - uses an embedded web browser to show Grid status info
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2017, Linden Research, Inc.
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

#ifndef LL_LLFLOATERGRIDSTATUS_H
#define LL_LLFLOATERGRIDSTATUS_H

#include "llfloaterwebcontent.h"
#include "llviewermediaobserver.h"

#include <string>

class LLMediaCtrl;


class LLFloaterGridStatus :
    public LLFloaterWebContent
{
public:
    typedef LLSDParamAdapter<_Params> Params;

    LLFloaterGridStatus(const Params& key);

    /*virtual*/ void onOpen(const LLSD& key);
    /*virtual*/ void handleReshape(const LLRect& new_rect, bool by_user = false);

    static bool checkGridStatusRSS();
    static void getGridStatusRSSCoro();

    void startGridStatusTimer();
    BOOL isFirstUpdate() { return mIsFirstUpdate; }
    void setFirstUpdate(BOOL first_update) { mIsFirstUpdate = first_update; }

    static LLFloaterGridStatus* getInstance();


private:
    /*virtual*/ BOOL postBuild();

    void applyPreferredRect();

    static std::map<std::string, std::string> sItemsMap;

    LLFrameTimer    mGridStatusTimer;
    BOOL            mIsFirstUpdate;
};

#endif  // LL_LLFLOATERGRIDSTATUS_H


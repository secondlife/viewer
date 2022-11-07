/** 
 * @file llfloaterlandholdings.h
 * @brief "My Land" floater showing all your land parcels.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLFLOATERLANDHOLDINGS_H
#define LL_LLFLOATERLANDHOLDINGS_H

#include "llfloater.h"

class LLMessageSystem;
class LLTextBox;
class LLScrollListCtrl;
class LLButton;

class LLFloaterLandHoldings
:   public LLFloater
{
public:
    LLFloaterLandHoldings(const LLSD& key);
    virtual ~LLFloaterLandHoldings();
    
    virtual BOOL postBuild();
    virtual void onOpen(const LLSD& key);
    virtual void draw();

    void refresh();

    void buttonCore(S32 which);

    static void processPlacesReply(LLMessageSystem* msg, void**);

    static void onClickTeleport(void*);
    static void onClickMap(void*);
    static void onClickLandmark(void*);

    static void onGrantList(void* data);

    static bool sHasLindenHome;

protected:
    void refreshAggregates();

protected:
    // Sum up as packets arrive the total holdings
    S32 mActualArea;
    S32 mBillableArea;

    // Has a packet of data been received?
    // Used to clear out the mParcelList's "Loading..." indicator
    BOOL mFirstPacketReceived;

    std::string mSortColumn;
    BOOL mSortAscending;
};

#endif

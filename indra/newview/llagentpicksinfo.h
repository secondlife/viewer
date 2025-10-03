/**
 * @file llagentpicksinfo.h
 * @brief LLAgentPicksInfo class header file
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLAGENTPICKS_H
#define LL_LLAGENTPICKS_H

#include "llsingleton.h"

struct LLAvatarData;

/**
 * Class that provides information about Agent Picks
 */
class LLAgentPicksInfo : public LLSingleton<LLAgentPicksInfo>
{
    LLSINGLETON(LLAgentPicksInfo);
    virtual ~LLAgentPicksInfo();

    class LLAgentPicksObserver;

public:
    /**
     * Requests number of picks from server.
     *
     * Number of Picks is requested from server, thus it is not available immediately.
     */
    void requestNumberOfPicks();

    /**
     * Returns number of Picks.
     */
    S32 getNumberOfPicks() const { return mNumberOfPicks; }

    /**
     * Returns maximum number of Picks.
     */
    static S32 getMaxNumberOfPicks();

    /**
     * Returns true if Agent has maximum allowed number of Picks.
     */
    bool isPickLimitReached() const;

    /**
     * After creating or deleting a Pick we can assume operation on server will be
     * completed successfully. Incrementing/decrementing number of picks makes new number
     * of picks available immediately. Actual number of picks will be updated when we receive
     * response from server.
     */
    void incrementNumberOfPicks() { ++mNumberOfPicks; }

    void decrementNumberOfPicks() { --mNumberOfPicks; }

    void onServerRespond(LLAvatarData* picks);

private:

    /**
    * Sets number of Picks.
    */
    void setNumberOfPicks(S32 number) { mNumberOfPicks = number; }

private:

    LLAgentPicksObserver* mAgentPicksObserver;
    S32 mNumberOfPicks;
};

#endif //LL_LLAGENTPICKS_H

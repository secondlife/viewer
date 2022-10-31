/** 
 * @file lldelayedgestureerror.h
 * @brief Delayed gesture error message -- try to wait until name has been retrieved
 * @author Dale Glass
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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


#ifndef LL_DELAYEDGESTUREERROR_H
#define LL_DELAYEDGESTUREERROR_H

#include <list>
#include "lltimer.h"

// TODO: Refactor to be more generic - this may be useful for other delayed notifications in the future

class LLDelayedGestureError
{
public:
    /**
     * @brief Generates a missing gesture error
     * @param id UUID of missing gesture
     * Delays message for up to 5 seconds if UUID can't be immediately converted to a text description
     */
    static void gestureMissing(const LLUUID &id);

    /**
     * @brief Generates a gesture failed to load error
     * @param id UUID of missing gesture
     * Delays message for up to 5 seconds if UUID can't be immediately converted to a text description
     */
    static void gestureFailedToLoad(const LLUUID &id);

private:
    

    struct LLErrorEntry
    {
        LLErrorEntry(const std::string& notify, const LLUUID &item) : mTimer(), mNotifyName(notify), mItemID(item) {}

        LLTimer mTimer;
        std::string mNotifyName;
        LLUUID mItemID;
    };


    static bool doDialog(const LLErrorEntry &ent, bool uuid_ok = false);
    static void enqueue(const LLErrorEntry &ent);
    static void onIdle(void *userdata);

    typedef std::list<LLErrorEntry> ErrorQueue;

    static ErrorQueue sQueue;
};


#endif

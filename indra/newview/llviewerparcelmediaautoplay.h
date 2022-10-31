/**
 * @file llviewerparcelmediaautoplay.h
 * @brief timer to automatically play media in a parcel
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LLVIEWERPARCELMEDIAAUTOPLAY_H
#define LLVIEWERPARCELMEDIAAUTOPLAY_H

#include "lleventtimer.h"
#include "lluuid.h"

// timer to automatically play media
class LLViewerParcelMediaAutoPlay : LLEventTimer, public LLSingleton<LLViewerParcelMediaAutoPlay>
{
    LLSINGLETON(LLViewerParcelMediaAutoPlay);
public:
    virtual BOOL tick();
    static void playStarted();

 private:
    // for askToPlay
    static void onStartMediaResponse(const LLUUID &region_id, const S32 &parcel_id, const std::string &url, const bool &play);

 private:
    S32 mLastParcelID;
    LLUUID mLastRegionID;
    BOOL mPlayed;
    F32 mTimeInParcel;
};


#endif // LLVIEWERPARCELMEDIAAUTOPLAY_H

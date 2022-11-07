/**
 * @file llwlparammanager.h
 * @brief Implementation for the LLWLParamManager class.
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

#ifndef LL_WL_DAY_CYCLE_H
#define LL_WL_DAY_CYCLE_H

class LLWLDayCycle;

#include <vector>
#include <map>
#include <string>
#include "llenvmanager.h"
struct LLWLParamKey;

class LLWLDayCycle
{
    LOG_CLASS(LLWLDayCycle);
public:

    // lists what param sets are used when during the day
    std::map<F32, LLWLParamKey> mTimeMap;

    // how long is my day
    F32 mDayRate;

public:

    /// simple constructor
    LLWLDayCycle();

    /// simple destructor
    ~LLWLDayCycle();

    /// load a day cycle
    void loadDayCycle(const LLSD& llsd, LLEnvKey::EScope scope);

    /// load a day cycle
    void loadDayCycleFromFile(const std::string & fileName);

    /// save a day cycle
    void saveDayCycle(const std::string & fileName);

    /// save a day cycle
    void save(const std::string& file_path);

    /// load the LLSD data from a file (returns the undefined LLSD if not found)
    static LLSD loadCycleDataFromFile(const std::string & fileName);

    /// load the LLSD data from a file specified by full path
    static LLSD loadDayCycleFromPath(const std::string& file_path);

    /// get the LLSD data for this day cycle
    LLSD asLLSD();

    // get skies referenced by this day cycle
//  bool getSkyRefs(std::map<LLWLParamKey, LLWLParamSet>& refs) const;

    // get referenced skies as LLSD
    bool getSkyMap(LLSD& sky_map) const;

    /// clear keyframes
    void clearKeyframes();

    /// Getters and Setters
    /// add a new key frame to the day cycle
    /// returns true if successful
    /// no negative time
    bool addKeyframe(F32 newTime, LLWLParamKey key);

    /// adjust a keyframe's placement in the day cycle
    /// returns true if successful
    bool changeKeyframeTime(F32 oldTime, F32 newTime);

    /// adjust a keyframe's parameter used
    /// returns true if successful
    bool changeKeyframeParam(F32 time, LLWLParamKey key);

    /// remove a key frame from the day cycle
    /// returns true if successful
    bool removeKeyframe(F32 time);

    /// get the first key time for a parameter
    /// returns false if not there
    bool getKeytime(LLWLParamKey keyFrame, F32& keyTime) const;

    /// get the param set at a given time
    /// returns true if found one
//  bool getKeyedParam(F32 time, LLWLParamSet& param);

    /// get the name
    /// returns true if it found one
    bool getKeyedParamName(F32 time, std::string & name);

    /// @return true if there are references to the given sky
    bool hasReferencesTo(const LLWLParamKey& keyframe) const;

    /// removes all references to the sky (paramkey)
    /// does nothing if the sky doesn't exist in the day
    void removeReferencesTo(const LLWLParamKey& keyframe);
};


#endif

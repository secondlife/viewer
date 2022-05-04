/**
 * @file llreflectionmapmanager.h
 * @brief LLReflectionMapManager class declaration
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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

#pragma once

#include "llreflectionmap.h"

class LLReflectionMapManager
{
public:
    // allocate an environment map of the given resolution 
    LLReflectionMapManager();
    
    // maintain reflection probes
    void update();

    // drop a reflection probe at the specified position in agent space
    void addProbe(const LLVector3& pos);

    // Populate "maps" with the N most relevant Reflection Maps where N is no more than maps.size()
    // If less than maps.size() ReflectionMaps are available, will assign trailing elements to nullptr.
    //  maps -- presized array of Reflection Map pointers
    void getReflectionMaps(std::vector<LLReflectionMap*>& maps);

    // list of active reflection maps
    std::vector<LLReflectionMap> mProbes;
};


/** 
 * @file llregionposition.h
 * @brief Region position storing class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLREGIONPOSITION_H
#define LL_LLREGIONPOSITION_H

/**
 * This class maintains a region, offset pair to store position, so when our "global"
 * coordinate frame shifts, all calculations are still correct.
 */

#include "v3math.h"
#include "v3dmath.h"

class LLViewerRegion;

class LLRegionPosition
{
private:
    LLViewerRegion *mRegionp;
public:
    LLVector3       mPositionRegion;
    LLRegionPosition();
    LLRegionPosition(LLViewerRegion *regionp, const LLVector3 &position_local);
    LLRegionPosition(const LLVector3d &global_position); // From global coords ONLY!

    LLViewerRegion*     getRegion() const;
    void                setPositionGlobal(const LLVector3d& global_pos);
    LLVector3d          getPositionGlobal() const;
    const LLVector3&    getPositionRegion() const;
    const LLVector3     getPositionAgent() const;


    void clear() { mRegionp = NULL; mPositionRegion.clearVec(); }
//  LLRegionPosition operator+(const LLRegionPosition &pos) const;
};

#endif // LL_REGION_POSITION_H

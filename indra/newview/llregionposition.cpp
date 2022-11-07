/** 
 * @file llregionposition.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llregionposition.h"

#include "llagent.h"
#include "llworld.h"
#include "llviewerregion.h"

LLRegionPosition::LLRegionPosition()
{
    mRegionp = NULL;
}

LLRegionPosition::LLRegionPosition(LLViewerRegion *regionp, const LLVector3 &position)
{
    mRegionp = regionp;
    mPositionRegion = position;
}

LLRegionPosition::LLRegionPosition(const LLVector3d &global_position)
{
    setPositionGlobal(global_position);
}

LLViewerRegion *LLRegionPosition::getRegion() const
{
    return mRegionp;
}

const LLVector3 &LLRegionPosition::getPositionRegion() const
{
    return mPositionRegion;
}

const LLVector3 LLRegionPosition::getPositionAgent() const
{
    return mRegionp->getPosAgentFromRegion( mPositionRegion );
}

LLVector3d LLRegionPosition::getPositionGlobal() const
{
    if (mRegionp)
    {
        return mRegionp->getPosGlobalFromRegion(mPositionRegion);
    }
    else
    {
        LLVector3d pos_global;
        pos_global.setVec(mPositionRegion);
        return pos_global;
    }
}


void LLRegionPosition::setPositionGlobal(const LLVector3d& position_global )
{
    mRegionp = LLWorld::getInstance()->getRegionFromPosGlobal(position_global);
    if (mRegionp)
    {
        mPositionRegion = mRegionp->getPosRegionFromGlobal(position_global);
    }
    else
    {
        mRegionp = gAgent.getRegion();
        llassert(mRegionp);
        mPositionRegion = mRegionp->getPosRegionFromGlobal(position_global);
    }
}

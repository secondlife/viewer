/**
 * @file llbbox.cpp
 * @brief General purpose bounding box class (Not axis aligned)
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

#include "linden_common.h"

// self include
#include "llbbox.h"

// library includes
#include "m4math.h"

#include <glm/gtc/quaternion.hpp>

void LLBBox::addPointLocal(const LLVector3& p)
{
    if (mEmpty)
    {
        mMinLocal = glm::vec3(p.mV[0], p.mV[1], p.mV[2]);
        mMaxLocal = glm::vec3(p.mV[0], p.mV[1], p.mV[2]);
        mEmpty = false;
    }
    else
    {
        mMinLocal.x = llmin( p.mV[VX], mMinLocal.x );
        mMinLocal.y = llmin( p.mV[VY], mMinLocal.y );
        mMinLocal.z = llmin( p.mV[VZ], mMinLocal.z );
        mMaxLocal.x = llmax( p.mV[VX], mMaxLocal.x );
        mMaxLocal.y = llmax( p.mV[VY], mMaxLocal.y );
        mMaxLocal.z = llmax( p.mV[VZ], mMaxLocal.z );
    }
}

void LLBBox::addPointAgent( LLVector3 p)
{
    p -= LLVector3(mPosAgent);
    p.rotVec( glm::conjugate(mRotation) );
    addPointLocal( p );
}


void LLBBox::addBBoxAgent(const LLBBox& b)
{
    if (mEmpty)
    {
        mPosAgent = b.mPosAgent;
        mRotation = b.mRotation;
        mMinLocal = glm::vec3(0.f);
        mMaxLocal = glm::vec3(0.f);
    }
    LLVector3 vertex[8];
    vertex[0].set( b.mMinLocal.x, b.mMinLocal.y, b.mMinLocal.z );
    vertex[1].set( b.mMinLocal.x, b.mMinLocal.y, b.mMaxLocal.z );
    vertex[2].set( b.mMinLocal.x, b.mMaxLocal.y, b.mMinLocal.z );
    vertex[3].set( b.mMinLocal.x, b.mMaxLocal.y, b.mMaxLocal.z );
    vertex[4].set( b.mMaxLocal.x, b.mMinLocal.y, b.mMinLocal.z );
    vertex[5].set( b.mMaxLocal.x, b.mMinLocal.y, b.mMaxLocal.z );
    vertex[6].set( b.mMaxLocal.x, b.mMaxLocal.y, b.mMinLocal.z );
    vertex[7].set( b.mMaxLocal.x, b.mMaxLocal.y, b.mMaxLocal.z );

    LLMatrix4 m( b.mRotation );
    m.translate( LLVector3(b.mPosAgent) );
    m.translate( -LLVector3(mPosAgent) );
    m.rotate( glm::conjugate(mRotation) );

    for(auto i : vertex)
    {
        addPointLocal( i * m );
    }
}

LLBBox LLBBox::getAxisAligned() const
{
    // no rotation = axis aligned rotation
    LLBBox aligned(mPosAgent, glm::quat(1.f, 0.f, 0.f, 0.f), LLVector3(), LLVector3());

    // add the center point so that it's not empty
    aligned.addPointAgent(mPosAgent);

    // add our BBox
    aligned.addBBoxAgent(*this);

    return aligned;
}

void LLBBox::expand( F32 delta )
{
    mMinLocal.x -= delta;
    mMinLocal.y -= delta;
    mMinLocal.z -= delta;
    mMaxLocal.x += delta;
    mMaxLocal.y += delta;
    mMaxLocal.z += delta;
}

LLVector3 LLBBox::localToAgent(const LLVector3& v) const
{
    LLMatrix4 m( mRotation );
    m.translate( LLVector3(mPosAgent) );
    return v * m;
}

LLVector3 LLBBox::agentToLocal(const LLVector3& v) const
{
    LLMatrix4 m;
    m.translate( -LLVector3(mPosAgent) );
    m.rotate( glm::conjugate(mRotation) );  // inverse rotation
    return v * m;
}

LLVector3 LLBBox::localToAgentBasis(const LLVector3& v) const
{
    LLMatrix4 m( mRotation );
    return v * m;
}

LLVector3 LLBBox::agentToLocalBasis(const LLVector3& v) const
{
    LLMatrix4 m( glm::conjugate(mRotation) );  // inverse rotation
    return v * m;
}

bool LLBBox::containsPointLocal(const LLVector3& p) const
{
    return !((p.mV[VX] < mMinLocal.x)
        ||(p.mV[VX] > mMaxLocal.x)
        ||(p.mV[VY] < mMinLocal.y)
        ||(p.mV[VY] > mMaxLocal.y)
        ||(p.mV[VZ] < mMinLocal.z)
        ||(p.mV[VZ] > mMaxLocal.z));
}

bool LLBBox::containsPointAgent(const LLVector3& p) const
{
    LLVector3 point_local = agentToLocal(p);
    return containsPointLocal(point_local);
}

LLVector3 LLBBox::getMinAgent() const
{
    return localToAgent(LLVector3(mMinLocal));
}

LLVector3 LLBBox::getMaxAgent() const
{
    return localToAgent(LLVector3(mMaxLocal));
}

/*
LLBBox operator*(const LLBBox &a, const LLMatrix4 &b)
{
    return LLBBox( a.mMin * b, a.mMax * b );
}
*/

/**
 * @file llbbox.h
 * @brief General purpose bounding box class
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

#pragma once

#include "v3math.h"
#include "llquaternion.h"

#include <glm/gtc/quaternion.hpp>

// Note: "local space" for an LLBBox is defined relative to agent space in terms of
// a translation followed by a rotation.  There is no scale term since the LLBBox's min and
// max are not necessarily symetrical and define their own extents.

class LLBBox
{
public:
    LLBBox() {mEmpty = true;}
    LLBBox( const LLVector3& pos_agent,
        const glm::quat& rot,
        const LLVector3& min_local,
        const LLVector3& max_local )
        :
        mMinLocal(min_local.mV[0], min_local.mV[1], min_local.mV[2]),
        mMaxLocal(max_local.mV[0], max_local.mV[1], max_local.mV[2]),
        mPosAgent(pos_agent.mV[0], pos_agent.mV[1], pos_agent.mV[2]),
        mRotation( rot), mEmpty( true )
        {}

    // Default copy constructor is OK.

    LLVector3           getPositionAgent() const            { return LLVector3(mPosAgent); }
    const glm::quat&    getRotation() const                 { return mRotation; }

    LLVector3           getMinAgent() const;
    LLVector3           getMinLocal() const                 { return LLVector3(mMinLocal); }
    void                setMinLocal( const LLVector3& min ) { mMinLocal = glm::vec3(min.mV[0], min.mV[1], min.mV[2]); }

    LLVector3           getMaxAgent() const;
    LLVector3           getMaxLocal() const                 { return LLVector3(mMaxLocal); }
    void                setMaxLocal( const LLVector3& max ) { mMaxLocal = glm::vec3(max.mV[0], max.mV[1], max.mV[2]); }

    LLVector3           getCenterLocal() const              { return LLVector3((mMaxLocal - mMinLocal) * 0.5f + mMinLocal); }
    LLVector3           getCenterAgent() const              { return localToAgent( getCenterLocal() ); }

    LLVector3           getExtentLocal() const              { return LLVector3(mMaxLocal - mMinLocal); }

    bool                containsPointLocal(const LLVector3& p) const;
    bool                containsPointAgent(const LLVector3& p) const;

    void                addPointAgent(LLVector3 p);
    void                addBBoxAgent(const LLBBox& b);

    void                addPointLocal(const LLVector3& p);
    void                addBBoxLocal(const LLBBox& b) { addPointLocal( LLVector3(b.mMinLocal) ); addPointLocal( LLVector3(b.mMaxLocal) ); }

    void                expand( F32 delta );

    LLVector3           localToAgent( const LLVector3& v ) const;
    LLVector3           agentToLocal( const LLVector3& v ) const;

    // Changes rotation but not position
    LLVector3           localToAgentBasis(const LLVector3& v) const;
    LLVector3           agentToLocalBasis(const LLVector3& v) const;

    // Get the smallest possible axis aligned bbox that contains this bbox
    LLBBox              getAxisAligned() const;

//  friend LLBBox operator*(const LLBBox& a, const LLMatrix4& b);

private:
    glm::vec3           mMinLocal{0.f};
    glm::vec3           mMaxLocal{0.f};
    glm::vec3           mPosAgent{0.f};  // Position relative to Agent's Region
    glm::quat           mRotation{1.f, 0.f, 0.f, 0.f};   // identity (w, x, y, z)
    bool                mEmpty;     // Nothing has been added to this bbox yet
};

static_assert(std::is_trivially_copyable<LLBBox>::value, "LLBBox must be trivial copy");
static_assert(std::is_trivially_move_assignable<LLBBox>::value, "LLBBox must be trivial move");
static_assert(std::is_standard_layout<LLBBox>::value, "LLBBox must be a standard layout type");

//LLBBox operator*(const LLBBox &a, const LLMatrix4 &b);



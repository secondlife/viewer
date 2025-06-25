/**
 * @file lljointdata.h
 * @brief LLJointData class for holding individual joint data and skeleton
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

#ifndef LL_LLJOINTDATA_H
#define LL_LLJOINTDATA_H

#include "v4math.h"

// may be just move LLAvatarBoneInfo
class LLJointData
{
public:
    std::string mName;
    std::string mGroup;
    glm::mat4 mJointMatrix;
    glm::mat4 mRestMatrix;
    glm::vec3 mScale;
    LLVector3 mRotation;

    typedef std::vector<LLJointData> bones_t;
    bones_t mChildren;

    bool mIsJoint; // if not, collision_volume
    enum SupportCategory
    {
        SUPPORT_BASE,
        SUPPORT_EXTENDED
    };
    SupportCategory mSupport;
    void setSupport(const std::string& support)
    {
        if (support == "extended")
        {
            mSupport = SUPPORT_EXTENDED;
        }
        else
        {
            mSupport = SUPPORT_BASE;
        }
    }
};

#endif //LL_LLJOINTDATA_H

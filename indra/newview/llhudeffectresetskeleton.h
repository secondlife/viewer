/**
 * @file llhudeffectresetskeleton.h
 * @brief LLHUDEffectResetSkeleton class definition
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

#ifndef LL_LLHUDEFFECTRESETSKELETON_H
#define LL_LLHUDEFFECTRESETSKELETON_H

#include "llhudeffect.h"

class LLViewerObject;
class LLVOAvatar;


class LLHUDEffectResetSkeleton final : public LLHUDEffect
{
public:
    friend class LLHUDObject;

    /*virtual*/ void markDead() override;
    /*virtual*/ void setSourceObject(LLViewerObject* objectp) override;

    void setTargetObject(LLViewerObject *objp) override;
    void setResetAnimations(bool enable){ mResetAnimations = enable; };

protected:
    LLHUDEffectResetSkeleton(const U8 type);
    ~LLHUDEffectResetSkeleton();

    void render() override;
    void packData(LLMessageSystem *mesgsys) override;
    void unpackData(LLMessageSystem *mesgsys, S32 blocknum) override;

    void update() override;
private:
    bool                        mResetAnimations;
};

#endif // LL_LLHUDEFFECTRESETSKELETON_H

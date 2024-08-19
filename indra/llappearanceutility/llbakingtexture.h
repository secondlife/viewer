/**
 * @file llbakingtexture.h
 * @brief Declaration of LLBakingTexture class.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LL_LLBAKINGTEXTURE_H
#define LL_LLBAKINGTEXTURE_H

#include "llgltexture.h"
#include "lluuid.h"

class LLImageRaw;

class LLBakingTexture : public LLGLTexture
{
public:
    enum
    {
        LOCAL_TEXTURE,
        BAKING_TEXTURE,
        INVALID_TEXTURE_TYPE
    };
    LLBakingTexture(const LLUUID& id, const LLImageRaw* raw);
    LLBakingTexture(BOOL usemipmaps);
    LLBakingTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps = TRUE);
    /*virtual*/ const LLUUID& getID() const { return mID; }
    virtual S8 getType() const { return BAKING_TEXTURE; }

    // Not implemented.
    /*virtual*/ void setKnownDrawSize(S32 width, S32 height);
    /*virtual*/ bool bindDefaultImage(const S32 stage = 0) ;
    /*virtual*/ void forceImmediateUpdate();
    /*virtual*/ void updateBindStatsForTester() ;
    /*virtual*/ bool bindDebugImage(const S32 stage = 0) { return false; }
    /*virtual*/ bool isActiveFetching() { return false; }

protected:
    virtual ~LLBakingTexture();
    LOG_CLASS(LLBakingTexture);
private:
    // Hide default constructor.
    LLBakingTexture() {}

    LLUUID  mID;
};

#endif /* LL_LLBAKINGTEXTURE_H */


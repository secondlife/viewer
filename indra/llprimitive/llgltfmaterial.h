/**
 * @file llgltfmaterial.h
 * @brief Material definition
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

#include "llrefcount.h"
#include "v4color.h"
#include "v3color.h"
#include "lluuid.h"
#include "llmd5.h"

class LLGLTFMaterial : public LLRefCount
{
public:

    enum AlphaMode
    {
        ALPHA_MODE_OPAQUE = 0,
        ALPHA_MODE_BLEND,
        ALPHA_MODE_MASK
    };

    LLUUID mBaseColorId;
    LLUUID mNormalId;
    LLUUID mMetallicRoughnessId;
    LLUUID mEmissiveId;

    LLColor4 mBaseColor = LLColor4(1,1,1,1);
    LLColor3 mEmissiveColor = LLColor3(0,0,0);

    F32 mMetallicFactor = 0.f;
    F32 mRoughnessFactor = 0.f;
    F32 mAlphaCutoff = 0.f;

    bool mDoubleSided = false;
    AlphaMode mAlphaMode = ALPHA_MODE_OPAQUE;

    // get a UUID based on a hash of this LLGLTFMaterial
    LLUUID getHash() const
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
        LLMD5 md5;
        md5.update((unsigned char*) this, sizeof(this));
        md5.finalize();
        LLUUID id;
        md5.raw_digest(id.mData);
        return id;
    }

    // set mAlphaMode from string.
    // Anything otherthan "MASK" or "BLEND" sets mAlphaMode to ALPHA_MODE_OPAQUE
    void setAlphaMode(const std::string& mode)
    {
        if (mode == "MASK")
        {
            mAlphaMode = ALPHA_MODE_MASK;
        }
        else if (mode == "BLEND")
        {
            mAlphaMode = ALPHA_MODE_BLEND;
        }
        else
        {
            mAlphaMode = ALPHA_MODE_OPAQUE;
        }
    }

    const char* getAlphaMode()
    {
        switch (mAlphaMode)
        {
        case ALPHA_MODE_MASK: return "MASK";
        case ALPHA_MODE_BLEND: return "BLEND";
        default: return "OPAQUE";
        }
    }
};




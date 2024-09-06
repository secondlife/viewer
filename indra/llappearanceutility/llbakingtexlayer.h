/**
 * @file llbakingtexlayer.h
 * @brief Declaration of LLBakingTexLayerSet.
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

#ifndef LL_LLBAKINGTEXLAYER_H
#define LL_LLBAKINGTEXLAYER_H

#include "llbakingtexture.h"
#include "llimagej2c.h"
#include "llpointer.h"
#include "lltexlayer.h"

class LLMD5;

class LLBakingTexLayerSetBuffer : public LLTexLayerSetBuffer, public LLBakingTexture
{
    LOG_CLASS(LLBakingTexLayerSetBuffer);
public:
    LLBakingTexLayerSetBuffer(LLTexLayerSet* const owner, S32 width, S32 height);
    virtual ~LLBakingTexLayerSetBuffer();

    LLImageJ2C* getCompressedImage() { return mCompressedImage; }

    bool                   render();
    /*virtual*/ bool       bindDebugImage(const S32 stage = 0) { return false; }
    /*virtual*/ bool       isActiveFetching() { return false; };

private:
    /*virtual*/ S32         getCompositeOriginX() const { return 0; }
    /*virtual*/ S32         getCompositeOriginY() const { return 0; }
    /*virtual*/ S32         getCompositeWidth() const { return getFullWidth(); }
    /*virtual*/ S32         getCompositeHeight() const { return getFullHeight(); }
    /*virtual*/ void        midRenderTexLayerSet(bool success);

    LLPointer<LLImageJ2C> mCompressedImage;
};

class LLBakingTexLayerSet : public LLTexLayerSet
{
public:
    LLBakingTexLayerSet(LLAvatarAppearance* const appearance);
    virtual ~LLBakingTexLayerSet();

    LLSD computeTextureIDs() const;
    bool computeLayerListTextureIDs(LLMD5& hash,
            std::set<LLUUID>& texture_ids,
            const layer_list_t& layer_list,
            bool& is_visible) const;

    /*virtual*/void             requestUpdate();
    /*virtual*/void             createComposite();
};

#endif /* LL_LLBAKINGTEXLAYER_H */

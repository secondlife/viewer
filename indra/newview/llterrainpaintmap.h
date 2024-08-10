/**
 * @file llterrainpaintmap.h
 * @brief Utilities for managing terrain paint maps
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#pragma once

#include "llviewerprecompiledheaders.h"

class LLViewerRegion;
class LLViewerTexture;
class LLTerrainPaintQueue;

class LLTerrainPaintMap
{
public:

    // Convert a region's heightmap and composition into a paint map texture which
    // approximates how the terrain would be rendered with the heightmap.
    // In effect, this allows converting terrain of type TERRAIN_PAINT_TYPE_HEIGHTMAP_WITH_NOISE
    // to type TERRAIN_PAINT_TYPE_PBR_PAINTMAP.
    // Returns true if successful
    static bool bakeHeightNoiseIntoPBRPaintMapRGB(const LLViewerRegion& region, LLViewerTexture& tex);

    static void applyPaintQueueRGB(LLViewerTexture& tex, LLTerrainPaintQueue& queue);
    static LLTerrainPaintQueue convertPaintQueueRGBAToRGB(LLViewerTexture& tex, LLTerrainPaintQueue& queue_in);
};

// Enqueued paint operations, in texture coordinates.
// mData is always RGB or RGBA (determined by mComponents), with each U8
// storing one color with a max value of (1 >> mBitDepth) - 1
struct LLTerrainPaint
{
    using ptr_t = std::shared_ptr<LLTerrainPaint>;

    U16 mStartX;
    U16 mStartY;
    U16 mWidthX;
    U16 mWidthY;
    U8 mBitDepth;
    U8 mComponents;
    const static U8 RGB = 3;
    const static U8 RGBA = 4;
    std::vector<U8> mData;
};

class LLTerrainPaintQueue
{
public:
    // components determines what type of LLTerrainPaint is allowed. Must be 3 (RGB) or 4 (RGBA)
    LLTerrainPaintQueue(U8 components);
    LLTerrainPaintQueue(const LLTerrainPaintQueue& other);
    LLTerrainPaintQueue& operator=(const LLTerrainPaintQueue& other);

    bool enqueue(LLTerrainPaint::ptr_t& paint, bool dry_run = false);
    bool enqueue(LLTerrainPaintQueue& paint_queue);
    size_t size() const;
    bool empty() const;
    void clear();

    const std::vector<LLTerrainPaint::ptr_t>& get() const { return mList; }

    U8 getComponents() const { return mComponents; }
    // Convert mBitDepth for the LLTerrainPaint in the queue at index
    // If mBitDepth is already equal to target_bit_depth, no conversion takes
    // place.
    // It is currently the responsibility of the paint queue to convert
    // incoming bits to the right bit depth for the paintmap (this could
    // change in the future).
    void convertBitDepths(size_t index, U8 target_bit_depth);

private:
    U8 mComponents;
    std::vector<LLTerrainPaint::ptr_t> mList;
};

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

#include <memory>
#include <vector>

#include "stdtypes.h"
#include "v2math.h"

class LLTexture;
class LLViewerRegion;
class LLViewerTexture;
class LLTerrainPaintQueue;
class LLTerrainBrushQueue;

// TODO: Terrain painting across regions. Assuming painting is confined to one
// region for now.
class LLTerrainPaintMap
{
public:

    // Convert a region's heightmap and composition into a paint map texture which
    // approximates how the terrain would be rendered with the heightmap.
    // In effect, this allows converting terrain of type TERRAIN_PAINT_TYPE_HEIGHTMAP_WITH_NOISE
    // to type TERRAIN_PAINT_TYPE_PBR_PAINTMAP.
    // Returns true if successful
    static bool bakeHeightNoiseIntoPBRPaintMapRGB(const LLViewerRegion& region, LLViewerTexture& tex);

    // This operation clears the queue
    // TODO: Decide if clearing the queue is needed - seems inconsistent
    static void applyPaintQueueRGB(LLViewerTexture& tex, LLTerrainPaintQueue& queue);

    static LLTerrainPaintQueue convertPaintQueueRGBAToRGB(LLViewerTexture& tex, LLTerrainPaintQueue& queue_in);
    // TODO: Implement (it's similar to convertPaintQueueRGBAToRGB but different shader + need to calculate the dimensions + need a different vertex buffer for each brush stroke)
    static LLTerrainPaintQueue convertBrushQueueToPaintRGB(const LLViewerRegion& region, LLViewerTexture& tex, LLTerrainBrushQueue& queue_in);
};

template<typename T>
class LLTerrainQueue
{
public:
    LLTerrainQueue() = default;
    LLTerrainQueue(LLTerrainQueue<T>& other);
    LLTerrainQueue& operator=(LLTerrainQueue<T>& other);

    bool enqueue(std::shared_ptr<T>& t, bool dry_run = false);
    size_t size() const;
    bool empty() const;
    void clear();

    const std::vector<std::shared_ptr<T>>& get() const { return mList; }

protected:
    bool enqueue(std::vector<std::shared_ptr<T>>& list);

    std::vector<std::shared_ptr<T>> mList;
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
    static constexpr U8 RGB = 3;
    static constexpr U8 RGBA = 4;
    std::vector<U8> mData;

    // Asserts that this paint's start/width fit within the bounds of the
    // provided texture dimensions.
    void assert_confined_to(const LLTexture& tex) const;
    // Confines this paint's start/width so it fits within the bounds of the
    // provided texture dimensions.
    // Does not allocate mData.
    void confine_to(const LLTexture& tex);
};

class LLTerrainPaintQueue : public LLTerrainQueue<LLTerrainPaint>
{
public:
    LLTerrainPaintQueue() = delete;
    // components determines what type of LLTerrainPaint is allowed. Must be 3 (RGB) or 4 (RGBA)
    LLTerrainPaintQueue(U8 components);
    LLTerrainPaintQueue(LLTerrainPaintQueue& other);
    LLTerrainPaintQueue& operator=(LLTerrainPaintQueue& other);

    bool enqueue(LLTerrainPaint::ptr_t& paint, bool dry_run = false);
    bool enqueue(LLTerrainPaintQueue& queue);

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
};

struct LLTerrainBrush
{
    using ptr_t = std::shared_ptr<LLTerrainBrush>;

    // Width of the brush in region space. The brush is a square texture with
    // alpha.
    F32 mBrushSize;
    // Brush path points in region space, excluding the vertical axis, which
    // does not contribute to the paint map.
    std::vector<LLVector2> mPath;
    // Offset of the brush path to actually start drawing at. An offset of 0
    // indicates that a brush stroke has just started (i.e. the user just
    // pressed down the mouse button). An offset greater than 0 indicates the
    // continuation of a brush stroke. Skipped entries in mPath are not drawn
    // directly, but are used for stroke orientation and path interpolation.
    // TODO: For the initial implementation, mPathOffset will be 0 and mPath
    // will be of length of at most 1, leading to discontinuous paint paths.
    // Then, mPathOffset may be 1 or 0, 1 indicating the continuation of a
    // stroke with linear interpolation. It is unlikely that we will implement
    // anything more sophisticated than that for now.
    U8 mPathOffset;
    // Indicates if this is the end of the brush stroke. Can occur if the mouse
    // button is lifted, or if the mouse temporarily stops while held down.
    bool mPathEnd;
};

class LLTerrainBrushQueue : public LLTerrainQueue<LLTerrainBrush>
{
public:
    LLTerrainBrushQueue();
    LLTerrainBrushQueue(LLTerrainBrushQueue& other);
    LLTerrainBrushQueue& operator=(LLTerrainBrushQueue& other);

    bool enqueue(LLTerrainBrush::ptr_t& brush, bool dry_run = false);
    bool enqueue(LLTerrainBrushQueue& queue);
};

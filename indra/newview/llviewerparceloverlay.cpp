/**
 * @file llviewerparceloverlay.cpp
 * @brief LLViewerParcelOverlay class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llviewerparceloverlay.h"

// indra includes
#include "llparcel.h"
#include "llfloaterreg.h"
#include "llgl.h"
#include "llrender.h"
#include "lluicolor.h"
#include "v4color.h"
#include "v2math.h"

// newview includes
#include "llagentcamera.h"
#include "llviewertexture.h"
#include "llviewercontrol.h"
#include "llsurface.h"
#include "llviewerregion.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llselectmgr.h"
#include "llfloatertools.h"
#include "llglheaders.h"
#include "pipeline.h"


static constexpr U8  OVERLAY_IMG_COMPONENTS = 4;
static constexpr F32 LINE_WIDTH = 0.0625f;

bool LLViewerParcelOverlay::sColorSetInitialized = false;
LLUIColor LLViewerParcelOverlay::sAvailColor;
LLUIColor LLViewerParcelOverlay::sOwnedColor;
LLUIColor LLViewerParcelOverlay::sGroupColor;
LLUIColor LLViewerParcelOverlay::sSelfColor;
LLUIColor LLViewerParcelOverlay::sForSaleColor;
LLUIColor LLViewerParcelOverlay::sAuctionColor;

LLViewerParcelOverlay::LLViewerParcelOverlay(LLViewerRegion* region, F32 region_width_meters)
:   mRegion(region),
    mParcelGridsPerEdge(S32(region_width_meters / PARCEL_GRID_STEP_METERS)),
    mDirty(false),
    mTimeSinceLastUpdate(),
    mOverlayTextureIdx(-1)
{
    if (!sColorSetInitialized)
    {
        sColorSetInitialized = true;
        sAvailColor = LLUIColorTable::instance().getColor("PropertyColorAvail").get();
        sOwnedColor = LLUIColorTable::instance().getColor("PropertyColorOther").get();
        sGroupColor = LLUIColorTable::instance().getColor("PropertyColorGroup").get();
        sSelfColor = LLUIColorTable::instance().getColor("PropertyColorSelf").get();
        sForSaleColor = LLUIColorTable::instance().getColor("PropertyColorForSale").get();
        sAuctionColor = LLUIColorTable::instance().getColor("PropertyColorAuction").get();
    }

    // Create a texture to hold color information.
    // 4 components
    // Use mipmaps = false, clamped, NEAREST filter, for sharp edges
    mImageRaw = new LLImageRaw(mParcelGridsPerEdge, mParcelGridsPerEdge, OVERLAY_IMG_COMPONENTS);
    mTexture = LLViewerTextureManager::getLocalTexture(mImageRaw.get(), false);
    mTexture->setAddressMode(LLTexUnit::TAM_CLAMP);
    mTexture->setFilteringOption(LLTexUnit::TFO_POINT);

    //
    // Initialize the GL texture with empty data.
    //
    // Create the base texture.
    U8* raw = mImageRaw->getData();
    const S32 COUNT = mParcelGridsPerEdge * mParcelGridsPerEdge * OVERLAY_IMG_COMPONENTS;
    for (S32 i = 0; i < COUNT; i++)
    {
        raw[i] = 0;
    }
    //mTexture->setSubImage(mImageRaw, 0, 0, mParcelGridsPerEdge, mParcelGridsPerEdge);

    // Create storage for ownership information from simulator
    // and initialize it.
    mOwnership = new U8[ mParcelGridsPerEdge * mParcelGridsPerEdge ];
    for (S32 i = 0; i < mParcelGridsPerEdge * mParcelGridsPerEdge; i++)
    {
        mOwnership[i] = PARCEL_PUBLIC;
    }

    gPipeline.markGLRebuild(this);
}


LLViewerParcelOverlay::~LLViewerParcelOverlay()
{
    delete[] mOwnership;
    mOwnership = NULL;
    mImageRaw = NULL;
}

//---------------------------------------------------------------------------
// ACCESSORS
//---------------------------------------------------------------------------
bool LLViewerParcelOverlay::isOwned(const LLVector3& pos) const
{
    S32 row =    S32(pos.mV[VY] / PARCEL_GRID_STEP_METERS);
    S32 column = S32(pos.mV[VX] / PARCEL_GRID_STEP_METERS);
    return (PARCEL_PUBLIC != ownership(row, column));
}

bool LLViewerParcelOverlay::isOwnedSelf(const LLVector3& pos) const
{
    S32 row =    S32(pos.mV[VY] / PARCEL_GRID_STEP_METERS);
    S32 column = S32(pos.mV[VX] / PARCEL_GRID_STEP_METERS);
    return (PARCEL_SELF == ownership(row, column));
}

bool LLViewerParcelOverlay::isOwnedGroup(const LLVector3& pos) const
{
    S32 row =    S32(pos.mV[VY] / PARCEL_GRID_STEP_METERS);
    S32 column = S32(pos.mV[VX] / PARCEL_GRID_STEP_METERS);
    return (PARCEL_GROUP == ownership(row, column));
}

bool LLViewerParcelOverlay::isOwnedOther(const LLVector3& pos) const
{
    S32 row =    S32(pos.mV[VY] / PARCEL_GRID_STEP_METERS);
    S32 column = S32(pos.mV[VX] / PARCEL_GRID_STEP_METERS);
    U8 overlay = ownership(row, column);
    return (PARCEL_OWNED == overlay || PARCEL_FOR_SALE == overlay);
}

bool LLViewerParcelOverlay::encroachesOwned(const std::vector<LLBBox>& boxes) const
{
    // boxes are expected to already be axis aligned
    for (U32 i = 0; i < boxes.size(); ++i)
    {
        LLVector3 min = boxes[i].getMinAgent();
        LLVector3 max = boxes[i].getMaxAgent();

        S32 left   = S32(llclamp((min.mV[VX] / PARCEL_GRID_STEP_METERS), 0.f, REGION_WIDTH_METERS - 1.f));
        S32 right  = S32(llclamp((max.mV[VX] / PARCEL_GRID_STEP_METERS), 0.f, REGION_WIDTH_METERS - 1.f));
        S32 top    = S32(llclamp((min.mV[VY] / PARCEL_GRID_STEP_METERS), 0.f, REGION_WIDTH_METERS - 1.f));
        S32 bottom = S32(llclamp((max.mV[VY] / PARCEL_GRID_STEP_METERS), 0.f, REGION_WIDTH_METERS - 1.f));

        for (S32 row = top; row <= bottom; row++)
        {
            for (S32 column = left; column <= right; column++)
            {
                U8 type = ownership(row, column);
                if ((PARCEL_SELF == type)
                    || (PARCEL_GROUP == type))
                {
                    return true;
                }
            }
        }
    }
    return false;
}
bool LLViewerParcelOverlay::encroachesOnUnowned(const std::vector<LLBBox>& boxes) const
{
    // boxes are expected to already be axis aligned
    for (U32 i = 0; i < boxes.size(); ++i)
    {
        LLVector3 min = boxes[i].getMinAgent();
        LLVector3 max = boxes[i].getMaxAgent();

        S32 left   = S32(llclamp((min.mV[VX] / PARCEL_GRID_STEP_METERS), 0.f, REGION_WIDTH_METERS - 1.f));
        S32 right  = S32(llclamp((max.mV[VX] / PARCEL_GRID_STEP_METERS), 0.f, REGION_WIDTH_METERS - 1.f));
        S32 top    = S32(llclamp((min.mV[VY] / PARCEL_GRID_STEP_METERS), 0.f, REGION_WIDTH_METERS - 1.f));
        S32 bottom = S32(llclamp((max.mV[VY] / PARCEL_GRID_STEP_METERS), 0.f, REGION_WIDTH_METERS - 1.f));

        for (S32 row = top; row <= bottom; row++)
        {
            for (S32 column = left; column <= right; column++)
            {
                U8 type = ownership(row, column);
                if ((PARCEL_SELF != type))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool LLViewerParcelOverlay::encroachesOnNearbyParcel(const std::vector<LLBBox>& boxes) const
{
    // boxes are expected to already be axis aligned
    for (U32 i = 0; i < boxes.size(); ++i)
    {
        LLVector3 min = boxes[i].getMinAgent();
        LLVector3 max = boxes[i].getMaxAgent();

        // If an object crosses region borders it crosses a parcel
        if (   min.mV[VX] < 0
            || min.mV[VY] < 0
            || max.mV[VX] > REGION_WIDTH_METERS
            || max.mV[VY] > REGION_WIDTH_METERS)
        {
            return true;
        }

        S32 left   = S32(llclamp((min.mV[VX] / PARCEL_GRID_STEP_METERS), 0.f, REGION_WIDTH_METERS - 1.f));
        S32 right  = S32(llclamp((max.mV[VX] / PARCEL_GRID_STEP_METERS), 0.f, REGION_WIDTH_METERS - 1.f));
        S32 bottom = S32(llclamp((min.mV[VY] / PARCEL_GRID_STEP_METERS), 0.f, REGION_WIDTH_METERS - 1.f));
        S32 top    = S32(llclamp((max.mV[VY] / PARCEL_GRID_STEP_METERS), 0.f, REGION_WIDTH_METERS - 1.f));

        const S32 GRIDS_PER_EDGE = mParcelGridsPerEdge;

        for (S32 row = bottom; row <= top; row++)
        {
            for (S32 col = left; col <= right; col++)
            {
                // This is not the rightmost column
                if (col < GRIDS_PER_EDGE-1)
                {
                    U8 east_overlay = mOwnership[row*GRIDS_PER_EDGE+col+1];
                    // If the column to the east of the current one marks
                    // the other parcel's west edge and the box extends
                    // to the west it crosses the parcel border.
                    if ((east_overlay & PARCEL_WEST_LINE) && col < right)
                    {
                        return true;
                    }
                }

                // This is not the topmost column
                if (row < GRIDS_PER_EDGE-1)
                {
                    U8 north_overlay = mOwnership[(row+1)*GRIDS_PER_EDGE+col];
                    // If the row to the north of the current one marks
                    // the other parcel's south edge and the box extends
                    // to the south it crosses the parcel border.
                    if ((north_overlay & PARCEL_SOUTH_LINE) && row < top)
                    {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool LLViewerParcelOverlay::isSoundLocal(const LLVector3& pos) const
{
    S32 row =    S32(pos.mV[VY] / PARCEL_GRID_STEP_METERS);
    S32 column = S32(pos.mV[VX] / PARCEL_GRID_STEP_METERS);
    return parcelFlags(row, column, PARCEL_SOUND_LOCAL);
}

U8 LLViewerParcelOverlay::ownership( const LLVector3& pos) const
{
    S32 row =    S32(pos.mV[VY] / PARCEL_GRID_STEP_METERS);
    S32 column = S32(pos.mV[VX] / PARCEL_GRID_STEP_METERS);
    return ownership(row, column);
}

U8 LLViewerParcelOverlay::parcelLineFlags(const LLVector3& pos) const
{
    S32 row = S32(pos.mV[VY] / PARCEL_GRID_STEP_METERS);
    S32 column = S32(pos.mV[VX] / PARCEL_GRID_STEP_METERS);
    return parcelFlags(row, column, PARCEL_WEST_LINE | PARCEL_SOUTH_LINE);
}
U8 LLViewerParcelOverlay::parcelLineFlags(S32 row, S32 col) const
{
    return parcelFlags(row, col, PARCEL_WEST_LINE | PARCEL_SOUTH_LINE);
}

U8 LLViewerParcelOverlay::parcelFlags(S32 row, S32 col, U8 flags) const
{
    if (row >= mParcelGridsPerEdge
        || col >= mParcelGridsPerEdge
        || row < 0
        || col < 0)
    {
        LL_WARNS() << "Attempted to get ownership out of region's overlay, row: " << row << " col: " << col << LL_ENDL;
        return flags;
    }
    return mOwnership[row * mParcelGridsPerEdge + col] & flags;
}

F32 LLViewerParcelOverlay::getOwnedRatio() const
{
    S32 size = mParcelGridsPerEdge * mParcelGridsPerEdge;
    S32 total = 0;

    for (S32 i = 0; i < size; i++)
    {
        if ((mOwnership[i] & PARCEL_COLOR_MASK) != PARCEL_PUBLIC)
        {
            total++;
        }
    }

    return (F32)total / (F32)size;
}

//---------------------------------------------------------------------------
// MANIPULATORS
//---------------------------------------------------------------------------

// Color tables for owned land
// Available = index 0
// Other     = index 1
// Group     = index 2
// Self      = index 3

// Make sure the texture colors match the ownership data.
// Note: Assumes that the ownership array and
void LLViewerParcelOverlay::updateOverlayTexture()
{
    if (mOverlayTextureIdx < 0)
    {
        if (!mDirty)
            return;
        mOverlayTextureIdx = 0;
    }

    const LLColor4U avail = sAvailColor.get();
    const LLColor4U owned = sOwnedColor.get();
    const LLColor4U group = sGroupColor.get();
    const LLColor4U self  = sSelfColor.get();
    const LLColor4U for_sale = sForSaleColor.get();
    const LLColor4U auction = sAuctionColor.get();

    // Create the base texture.
    U8* raw = mImageRaw->getData();
    const S32 COUNT = mParcelGridsPerEdge * mParcelGridsPerEdge;
    S32 max = mOverlayTextureIdx + mParcelGridsPerEdge;
    if (max > COUNT) max = COUNT;
    S32 pixel_index = mOverlayTextureIdx * OVERLAY_IMG_COMPONENTS;
    S32 i;
    for (i = mOverlayTextureIdx; i < max; i++)
    {
        U8 ownership = mOwnership[i];

        U8 r,g,b,a;

        // Color stored in low three bits
        switch (ownership & 0x7)
        {
        case PARCEL_PUBLIC:
            r = avail.mV[VRED];
            g = avail.mV[VGREEN];
            b = avail.mV[VBLUE];
            a = avail.mV[VALPHA];
            break;
        case PARCEL_OWNED:
            r = owned.mV[VRED];
            g = owned.mV[VGREEN];
            b = owned.mV[VBLUE];
            a = owned.mV[VALPHA];
            break;
        case PARCEL_GROUP:
            r = group.mV[VRED];
            g = group.mV[VGREEN];
            b = group.mV[VBLUE];
            a = group.mV[VALPHA];
            break;
        case PARCEL_SELF:
            r = self.mV[VRED];
            g = self.mV[VGREEN];
            b = self.mV[VBLUE];
            a = self.mV[VALPHA];
            break;
        case PARCEL_FOR_SALE:
            r = for_sale.mV[VRED];
            g = for_sale.mV[VGREEN];
            b = for_sale.mV[VBLUE];
            a = for_sale.mV[VALPHA];
            break;
        case PARCEL_AUCTION:
            r = auction.mV[VRED];
            g = auction.mV[VGREEN];
            b = auction.mV[VBLUE];
            a = auction.mV[VALPHA];
            break;
        default:
            r = self.mV[VRED];
            g = self.mV[VGREEN];
            b = self.mV[VBLUE];
            a = self.mV[VALPHA];
            break;
        }

        raw[pixel_index + VRED]   = (U8)r;
        raw[pixel_index + VGREEN] = (U8)g;
        raw[pixel_index + VBLUE]  = (U8)b;
        raw[pixel_index + VALPHA] = (U8)a;

        pixel_index += OVERLAY_IMG_COMPONENTS;
    }

    // Copy data into GL texture from raw data
    if (i >= COUNT)
    {
        if (!mTexture->hasGLTexture())
        {
            mTexture->createGLTexture(0, mImageRaw);
        }
        mTexture->setSubImage(mImageRaw, 0, 0, mParcelGridsPerEdge, mParcelGridsPerEdge);
        mOverlayTextureIdx = -1;
    }
    else
    {
        mOverlayTextureIdx = i;
    }
}

void LLViewerParcelOverlay::uncompressLandOverlay(S32 chunk, U8* packed_overlay)
{
    // Unpack the message data into the ownership array
    S32 size = mParcelGridsPerEdge * mParcelGridsPerEdge;
    S32 chunk_size = size / PARCEL_OVERLAY_CHUNKS;

    memcpy(mOwnership + chunk*chunk_size, packed_overlay, chunk_size);      /*Flawfinder: ignore*/

    // Force property lines and overlay texture to update
    setDirty();
}

void LLViewerParcelOverlay::updatePropertyLines()
{
    static LLCachedControl<bool> show(gSavedSettings, "ShowPropertyLines");

    if (!show)
        return;

    LLColor4U colors[PARCEL_COLOR_MASK + 1];
    colors[PARCEL_SELF] = sSelfColor.get();
    colors[PARCEL_OWNED] = sOwnedColor.get();
    colors[PARCEL_GROUP] = sGroupColor.get();
    colors[PARCEL_FOR_SALE] = sForSaleColor.get();
    colors[PARCEL_AUCTION] = sAuctionColor.get();

    mEdges.clear();

    constexpr F32 GRID_STEP = PARCEL_GRID_STEP_METERS;
    const S32 GRIDS_PER_EDGE = mParcelGridsPerEdge;

    for (S32 row = 0; row < GRIDS_PER_EDGE; row++)
    {
        for (S32 col = 0; col < GRIDS_PER_EDGE; col++)
        {
            U8 overlay = mOwnership[row * GRIDS_PER_EDGE + col];
            S32 colorIndex = overlay & PARCEL_COLOR_MASK;
            switch (colorIndex)
            {
            case PARCEL_SELF:
            case PARCEL_GROUP:
            case PARCEL_OWNED:
            case PARCEL_FOR_SALE:
            case PARCEL_AUCTION:
                break;
            default:
                continue;
            }

            const LLColor4U& color = colors[colorIndex];

            F32 left = col * GRID_STEP;
            F32 right = left + GRID_STEP;

            F32 bottom = row * GRID_STEP;
            F32 top = bottom + GRID_STEP;

            // West edge
            if (overlay & PARCEL_WEST_LINE)
            {
                addPropertyLine(left, bottom, 0, 1, LINE_WIDTH, 0, color);
            }

            // East edge
            if (col == GRIDS_PER_EDGE - 1 || mOwnership[row * GRIDS_PER_EDGE + col + 1] & PARCEL_WEST_LINE)
            {
                addPropertyLine(right, bottom, 0, 1, -LINE_WIDTH, 0, color);
            }

            // South edge
            if (overlay & PARCEL_SOUTH_LINE)
            {
                addPropertyLine(left, bottom, 1, 0, 0, LINE_WIDTH, color);
            }

            // North edge
            if (row == GRIDS_PER_EDGE - 1 || mOwnership[(row + 1) * GRIDS_PER_EDGE + col] & PARCEL_SOUTH_LINE)
            {
                addPropertyLine(left, top, 1, 0, 0, -LINE_WIDTH, color);
            }
        }
    }

    // Everything's clean now
    mDirty = false;
}

void LLViewerParcelOverlay::addPropertyLine(F32 start_x, F32 start_y, F32 dx, F32 dy, F32 tick_dx, F32 tick_dy, const LLColor4U& color)
{
    LLSurface& land = mRegion->getLand();
    F32 water_z = land.getWaterHeight();

    mEdges.resize(mEdges.size() + 1);
    Edge& edge = mEdges.back();
    edge.color = color;

    // Detailized rendering vertices:
    // A B      C        D        E      F G
    // *-*------*--------*--------*------*-* : 'outside' vertices are placed right on the border
    //   *------*--------*--------*------*   : 'inside' vertices are shifted on LINE_WIDTH inside

    // Simplified rendering vertices:
    // A                                   G
    // *-----------------------------------*
    // *-----------------------------------*

    F32 outside_x = start_x;
    F32 outside_y = start_y;
    F32 outside_z = land.resolveHeightRegion(outside_x, outside_y);
    F32 inside_x = start_x + tick_dx;
    F32 inside_y = start_y + tick_dy;
    F32 inside_z = land.resolveHeightRegion(inside_x, inside_y);

    auto move = [&](F32 distance)
        {
            outside_x += dx * distance;
            outside_y += dy * distance;
            outside_z = land.resolveHeightRegion(outside_x, outside_y);
            inside_x += dx * distance;
            inside_y += dy * distance;
            inside_z = land.resolveHeightRegion(inside_x, inside_y);
        };

    auto split = [&](U32 lod, const LLVector4a& start, F32 x, F32 y, F32 z, F32 part)
        {
            F32 new_x = start[VX] + (x - start[VX]) * part;
            F32 new_y = start[VY] + (y - start[VY]) * part;
            edge.pushVertex(lod, new_x, new_y, water_z, 0);
        };

    auto checkForSplit = [&](U32 lod)
        {
            const std::vector<LLVector4a>& vertices = edge.verticesUnderWater[lod];
            const LLVector4a& last_outside = vertices.back();
            F32 z0 = last_outside[VZ];
            F32 z1 = outside_z;
            if ((z0 >= water_z && z1 >= water_z) || (z0 < water_z && z1 < water_z))
                return;
            F32 part = (water_z - z0) / (z1 - z0);
            const LLVector4a& last_inside = vertices[vertices.size() - 2];
            split(lod, last_inside, inside_x, inside_y, inside_z, part);
            split(lod, last_outside, outside_x, outside_y, outside_z, part);
        };

    auto pushTwoVertices = [&](U32 lod)
        {
            LLVector3 out(outside_x, outside_y, outside_z);
            LLVector3 in(inside_x, inside_y, inside_z);
            if (fabs(inside_z - outside_z) < LINE_WIDTH / 5)
            {
                edge.pushVertex(lod, inside_x, inside_y, inside_z, water_z);
            }
            else
            {
                // Make the line thinner if heights differ too much
                LLVector3 dist(in - out);
                F32 coef = dist.length() / LINE_WIDTH;
                LLVector3 new_in(out + dist / coef);
                edge.pushVertex(lod, new_in[VX], new_in[VY], new_in[VZ], water_z);
            }
            edge.pushVertex(lod, outside_x, outside_y, outside_z, water_z);
        };

    // Point A simplified (first two vertices)
    pushTwoVertices(1);

    // Point A detailized (only one vertex)
    edge.pushVertex(0, outside_x, outside_y, outside_z, water_z);

    // Point B (two vertices)
    move(LINE_WIDTH);
    pushTwoVertices(0);

    // Points C, D, E
    F32 distance = 1.f - LINE_WIDTH;
    constexpr U32 GRID_STEP = (U32)PARCEL_GRID_STEP_METERS;
    for (U32 i = 1; i < GRID_STEP; ++i)
    {
        move(distance);
        checkForSplit(0);
        pushTwoVertices(0);
        distance = 1.f;
    }

    // Point F (two vertices)
    move(1.f - LINE_WIDTH);
    checkForSplit(0);
    pushTwoVertices(0);

    // Point G simplified (last two vertices)
    move(LINE_WIDTH);
    pushTwoVertices(1);

    // Point G detailized (only one vertex)
    edge.pushVertex(0, outside_x, outside_y, outside_z, water_z);
}

void LLViewerParcelOverlay::Edge::pushVertex(U32 lod, F32 x, F32 y, F32 z, F32 water_z)
{
    verticesUnderWater[lod].emplace_back(x, y, z);
    gGL.transform(verticesUnderWater[lod].back());

    if (z >= water_z)
    {
        verticesAboveWater[lod].push_back(verticesUnderWater[lod].back());
    }
    else
    {
        verticesAboveWater[lod].emplace_back(x, y, water_z);
        gGL.transform(verticesAboveWater[lod].back());
    }
}

void LLViewerParcelOverlay::setDirty()
{
    mDirty = true;
}

void LLViewerParcelOverlay::updateGL()
{
    LL_PROFILE_ZONE_SCOPED;
    updateOverlayTexture();
}

void LLViewerParcelOverlay::idleUpdate(bool force_update)
{
    if (gGLManager.mIsDisabled)
    {
        return;
    }
    if (mOverlayTextureIdx >= 0 && (!(mDirty && force_update)))
    {
        // We are in the middle of updating the overlay texture
        gPipeline.markGLRebuild(this);
        return;
    }
    // Only if we're dirty and it's been a while since the last update.
    if (mDirty)
    {
        if (force_update || mTimeSinceLastUpdate.getElapsedTimeF32() > 4.0f)
        {
            updateOverlayTexture();
            updatePropertyLines();
            mTimeSinceLastUpdate.reset();
        }
    }
}

void LLViewerParcelOverlay::renderPropertyLines()
{
    static LLCachedControl<bool> show(gSavedSettings, "ShowPropertyLines");

    if (!show)
        return;

    LLSurface& land = mRegion->getLand();
    F32 water_z = land.getWaterHeight() + 0.01f;

    LLGLSUIDefault gls_ui; // called from pipeline
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    LLGLDepthTest mDepthTest(GL_TRUE);

    // Find camera height off the ground (not from zero)
    F32 ground_height_at_camera = land.resolveHeightGlobal( gAgentCamera.getCameraPositionGlobal() );
    F32 camera_z = LLViewerCamera::getInstance()->getOrigin().mV[VZ];
    F32 camera_height = camera_z - ground_height_at_camera;

    camera_height = llclamp(camera_height, 0.f, 100.f);

    // Pull lines toward camera by 1 cm per meter off the ground.
    const LLVector3& CAMERA_AT = LLViewerCamera::getInstance()->getAtAxis();
    F32 pull_toward_camera_scale = 0.01f * camera_height;
    LLVector3 pull_toward_camera = CAMERA_AT;
    pull_toward_camera *= -pull_toward_camera_scale;

    // Always fudge a little vertically.
    pull_toward_camera.mV[VZ] += 0.01f;

    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();

    // Move to appropriate region coords
    LLVector3 origin = mRegion->getOriginAgent();
    gGL.translatef(origin.mV[VX], origin.mV[VY], origin.mV[VZ]);

    gGL.translatef(pull_toward_camera.mV[VX], pull_toward_camera.mV[VY],
        pull_toward_camera.mV[VZ]);

    // Stomp the camera into two dimensions
    LLVector3 camera_region = mRegion->getPosRegionFromGlobal( gAgentCamera.getCameraPositionGlobal() );
    bool draw_underwater = camera_region.mV[VZ] < water_z ||
        !gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_WATER);

    // Set up a cull plane 2 * PARCEL_GRID_STEP_METERS behind
    // the camera.  The cull plane normal is the camera's at axis.
    LLVector3 cull_plane_point = LLViewerCamera::getInstance()->getAtAxis();
    cull_plane_point *= -2.f * PARCEL_GRID_STEP_METERS;
    cull_plane_point += camera_region;

    bool render_hidden = !draw_underwater &&
        LLSelectMgr::sRenderHiddenSelections &&
        LLFloaterReg::instanceVisible("build");

    constexpr F32 PROPERTY_LINE_CLIP_DIST_SQUARED = 256.f * 256.f;
    constexpr F32 PROPERTY_LINE_LOD0_DIST_SQUARED = PROPERTY_LINE_CLIP_DIST_SQUARED / 25.f;

    for (const Edge& edge : mEdges)
    {
        const std::vector<LLVector4a>& vertices0 = edge.verticesAboveWater[0];
        const F32* first = vertices0.front().getF32ptr();
        const F32* last = vertices0.back().getF32ptr();
        LLVector3 center((first[VX] + last[VX]) / 2, (first[VY] + last[VY]) / 2, (first[VZ] + last[VZ]) / 2);
        gGL.untransform(center);

        F32 dist_squared = dist_vec_squared(center, camera_region);
        if (dist_squared > PROPERTY_LINE_CLIP_DIST_SQUARED)
        {
            continue;
        }

        // Destroy vertex, transform to plane-local.
        center -= cull_plane_point;

        // Negative dot product means it is in back of the plane
        if (center * CAMERA_AT < 0.f)
        {
            continue;
        }

        U32 lod = dist_squared < PROPERTY_LINE_LOD0_DIST_SQUARED ? 0 : 1;

        gGL.begin(LLRender::TRIANGLE_STRIP);

        gGL.color4ubv(edge.color.mV);

        if (draw_underwater)
        {
            gGL.vertexBatchPreTransformed(edge.verticesUnderWater[lod]);
        }
        else
        {
            gGL.vertexBatchPreTransformed(edge.verticesAboveWater[lod]);

            if (render_hidden)
            {
                LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_GREATER);

                LLColor4U color = edge.color;
                color.mV[VALPHA] /= 4;
                gGL.color4ubv(color.mV);

                gGL.vertexBatchPreTransformed(edge.verticesUnderWater[lod]);
            }
        }

        gGL.end();
    }

    gGL.popMatrix();
}

// Draw half of a single cell (no fill) in a grid drawn from left to right and from bottom to top
void grid_2d_part_lines(const F32 left, const F32 top, const F32 right, const F32 bottom, bool has_left, bool has_bottom)
{
    gGL.begin(LLRender::LINES);

    if (has_left)
    {
        gGL.vertex2f(left, bottom);
        gGL.vertex2f(left, top);
    }
    if (has_bottom)
    {
        gGL.vertex2f(left, bottom);
        gGL.vertex2f(right, bottom);
    }

    gGL.end();
}

void LLViewerParcelOverlay::renderPropertyLinesOnMinimap(F32 scale_pixels_per_meter, const F32* parcel_outline_color)
{
    static LLCachedControl<bool> show(gSavedSettings, "MiniMapShowPropertyLines");

    if (!mOwnership || !show)
    {
        return;
    }

    LLVector3 origin_agent     = mRegion->getOriginAgent();
    LLVector3 rel_region_pos   = origin_agent - gAgentCamera.getCameraPositionAgent();
    F32       region_left      = rel_region_pos.mV[VX] * scale_pixels_per_meter;
    F32       region_bottom    = rel_region_pos.mV[VY] * scale_pixels_per_meter;
    F32       map_parcel_width = PARCEL_GRID_STEP_METERS * scale_pixels_per_meter;
    const S32 GRIDS_PER_EDGE   = mParcelGridsPerEdge;

    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    glLineWidth(1.0f);
    gGL.color4fv(parcel_outline_color);
    for (S32 i = 0; i <= GRIDS_PER_EDGE; i++)
    {
        const F32 bottom = region_bottom + (i * map_parcel_width);
        const F32 top    = bottom + map_parcel_width;
        for (S32 j = 0; j <= GRIDS_PER_EDGE; j++)
        {
            const F32  left               = region_left + (j * map_parcel_width);
            const F32  right              = left + map_parcel_width;
            const bool is_region_boundary = i == GRIDS_PER_EDGE || j == GRIDS_PER_EDGE;
            const U8   overlay            = is_region_boundary ? 0 : mOwnership[(i * GRIDS_PER_EDGE) + j];
            // The property line vertices are three-dimensional, but here we only care about the x and y coordinates, as we are drawing on a
            // 2D map
            const bool has_left   = i != GRIDS_PER_EDGE && (j == GRIDS_PER_EDGE || (overlay & PARCEL_WEST_LINE));
            const bool has_bottom = j != GRIDS_PER_EDGE && (i == GRIDS_PER_EDGE || (overlay & PARCEL_SOUTH_LINE));
            grid_2d_part_lines(left, top, right, bottom, has_left, has_bottom);
        }
    }
}

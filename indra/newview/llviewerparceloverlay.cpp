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


const U8  OVERLAY_IMG_COMPONENTS = 4;

LLViewerParcelOverlay::LLViewerParcelOverlay(LLViewerRegion* region, F32 region_width_meters)
:	mRegion( region ),
	mParcelGridsPerEdge( S32( region_width_meters / PARCEL_GRID_STEP_METERS ) ),
	mDirty( FALSE ),
	mTimeSinceLastUpdate(),
	mOverlayTextureIdx(-1),
	mVertexCount(0),
	mVertexArray(NULL),
	mColorArray(NULL)
//	mTexCoordArray(NULL),
{
	// Create a texture to hold color information.
	// 4 components
	// Use mipmaps = FALSE, clamped, NEAREST filter, for sharp edges	
	mImageRaw = new LLImageRaw(mParcelGridsPerEdge, mParcelGridsPerEdge, OVERLAY_IMG_COMPONENTS);
	mTexture = LLViewerTextureManager::getLocalTexture(mImageRaw.get(), FALSE);
	mTexture->setAddressMode(LLTexUnit::TAM_CLAMP);
	mTexture->setFilteringOption(LLTexUnit::TFO_POINT);

	//
	// Initialize the GL texture with empty data.
	//
	// Create the base texture.
	U8 *raw = mImageRaw->getData();
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

	delete[] mVertexArray;
	mVertexArray = NULL;

	delete[] mColorArray;
	mColorArray = NULL;

// JC No textures.
//	delete mTexCoordArray;
//	mTexCoordArray = NULL;

	mImageRaw = NULL;
}

//---------------------------------------------------------------------------
// ACCESSORS
//---------------------------------------------------------------------------
BOOL LLViewerParcelOverlay::isOwned(const LLVector3& pos) const
{
	S32 row =    S32(pos.mV[VY] / PARCEL_GRID_STEP_METERS);
	S32 column = S32(pos.mV[VX] / PARCEL_GRID_STEP_METERS);
	return (PARCEL_PUBLIC != ownership(row, column));
}

BOOL LLViewerParcelOverlay::isOwnedSelf(const LLVector3& pos) const
{
	S32 row =    S32(pos.mV[VY] / PARCEL_GRID_STEP_METERS);
	S32 column = S32(pos.mV[VX] / PARCEL_GRID_STEP_METERS);
	return (PARCEL_SELF == ownership(row, column));
}

BOOL LLViewerParcelOverlay::isOwnedGroup(const LLVector3& pos) const
{
	S32 row =    S32(pos.mV[VY] / PARCEL_GRID_STEP_METERS);
	S32 column = S32(pos.mV[VX] / PARCEL_GRID_STEP_METERS);
	return (PARCEL_GROUP == ownership(row, column));
}

BOOL LLViewerParcelOverlay::isOwnedOther(const LLVector3& pos) const
{
	S32 row =    S32(pos.mV[VY] / PARCEL_GRID_STEP_METERS);
	S32 column = S32(pos.mV[VX] / PARCEL_GRID_STEP_METERS);
	U8 overlay = ownership(row, column);
	return (PARCEL_OWNED == overlay || PARCEL_FOR_SALE == overlay);
}

bool LLViewerParcelOverlay::encroachesOwned(const std::vector<LLBBox>& boxes) const
{
	// boxes are expected to already be axis aligned
	for (S32 i = 0; i < boxes.size(); ++i)
	{
		LLVector3 min = boxes[i].getMinAgent();
		LLVector3 max = boxes[i].getMaxAgent();
		
		S32 left   = S32(llclamp((min.mV[VX] / PARCEL_GRID_STEP_METERS), 0.f, REGION_WIDTH_METERS - 1));
		S32 right  = S32(llclamp((max.mV[VX] / PARCEL_GRID_STEP_METERS), 0.f, REGION_WIDTH_METERS - 1));
		S32 top    = S32(llclamp((min.mV[VY] / PARCEL_GRID_STEP_METERS), 0.f, REGION_WIDTH_METERS - 1));
		S32 bottom = S32(llclamp((max.mV[VY] / PARCEL_GRID_STEP_METERS), 0.f, REGION_WIDTH_METERS - 1));
	
		for (S32 row = top; row <= bottom; row++)
			for (S32 column = left; column <= right; column++)
			{
				U8 type = ownership(row, column);
				if (PARCEL_SELF == type
					|| PARCEL_GROUP == type )
					return true;
			}
	}
	return false;
}

BOOL LLViewerParcelOverlay::isSoundLocal(const LLVector3& pos) const
{
	S32 row =    S32(pos.mV[VY] / PARCEL_GRID_STEP_METERS);
	S32 column = S32(pos.mV[VX] / PARCEL_GRID_STEP_METERS);
	return PARCEL_SOUND_LOCAL & mOwnership[row * mParcelGridsPerEdge + column];
}

U8 LLViewerParcelOverlay::ownership( const LLVector3& pos) const
{
	S32 row =    S32(pos.mV[VY] / PARCEL_GRID_STEP_METERS);
	S32 column = S32(pos.mV[VX] / PARCEL_GRID_STEP_METERS);
	return ownership(row, column);
}

F32 LLViewerParcelOverlay::getOwnedRatio() const
{
	S32	size = mParcelGridsPerEdge * mParcelGridsPerEdge;
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
	if (mOverlayTextureIdx < 0 && mDirty)
	{
		mOverlayTextureIdx = 0;
	}
	if (mOverlayTextureIdx < 0)
	{
		return;
	}
	const LLColor4U avail = LLUIColorTable::instance().getColor("PropertyColorAvail").get();
	const LLColor4U owned = LLUIColorTable::instance().getColor("PropertyColorOther").get();
	const LLColor4U group = LLUIColorTable::instance().getColor("PropertyColorGroup").get();
	const LLColor4U self  = LLUIColorTable::instance().getColor("PropertyColorSelf").get();
	const LLColor4U for_sale  = LLUIColorTable::instance().getColor("PropertyColorForSale").get();
	const LLColor4U auction  = LLUIColorTable::instance().getColor("PropertyColorAuction").get();

	// Create the base texture.
	U8 *raw = mImageRaw->getData();
	const S32 COUNT = mParcelGridsPerEdge * mParcelGridsPerEdge;
	S32 max = mOverlayTextureIdx + mParcelGridsPerEdge;
	if (max > COUNT) max = COUNT;
	S32 pixel_index = mOverlayTextureIdx*OVERLAY_IMG_COMPONENTS;
	S32 i;
	for (i = mOverlayTextureIdx; i < max; i++)
	{
		U8 ownership = mOwnership[i];

		F32 r,g,b,a;

		// Color stored in low three bits
		switch( ownership & 0x7 )
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

		raw[pixel_index + 0] = (U8)r;
		raw[pixel_index + 1] = (U8)g;
		raw[pixel_index + 2] = (U8)b;
		raw[pixel_index + 3] = (U8)a;

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


void LLViewerParcelOverlay::uncompressLandOverlay(S32 chunk, U8 *packed_overlay)
{
	// Unpack the message data into the ownership array
	S32	size	= mParcelGridsPerEdge * mParcelGridsPerEdge;
	S32 chunk_size = size / PARCEL_OVERLAY_CHUNKS;

	memcpy(mOwnership + chunk*chunk_size, packed_overlay, chunk_size);		/*Flawfinder: ignore*/

	// Force property lines and overlay texture to update
	setDirty();
}


void LLViewerParcelOverlay::updatePropertyLines()
{
	if (!gSavedSettings.getBOOL("ShowPropertyLines"))
		return;
	
	S32 row, col;

	const LLColor4U self_coloru  = LLUIColorTable::instance().getColor("PropertyColorSelf").get();
	const LLColor4U other_coloru = LLUIColorTable::instance().getColor("PropertyColorOther").get();
	const LLColor4U group_coloru = LLUIColorTable::instance().getColor("PropertyColorGroup").get();
	const LLColor4U for_sale_coloru = LLUIColorTable::instance().getColor("PropertyColorForSale").get();
	const LLColor4U auction_coloru = LLUIColorTable::instance().getColor("PropertyColorAuction").get();

	// Build into dynamic arrays, then copy into static arrays.
	LLDynamicArray<LLVector3, 256> new_vertex_array;
	LLDynamicArray<LLColor4U, 256> new_color_array;
	LLDynamicArray<LLVector2, 256> new_coord_array;

	U8 overlay = 0;
	BOOL add_edge = FALSE;
	const F32 GRID_STEP = PARCEL_GRID_STEP_METERS;
	const S32 GRIDS_PER_EDGE = mParcelGridsPerEdge;

	for (row = 0; row < GRIDS_PER_EDGE; row++)
	{
		for (col = 0; col < GRIDS_PER_EDGE; col++)
		{
			overlay = mOwnership[row*GRIDS_PER_EDGE+col];

			F32 left = col*GRID_STEP;
			F32 right = left+GRID_STEP;

			F32 bottom = row*GRID_STEP;
			F32 top = bottom+GRID_STEP;

			// West edge
			if (overlay & PARCEL_WEST_LINE)
			{
				switch(overlay & PARCEL_COLOR_MASK)
				{
				case PARCEL_SELF:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						left, bottom, WEST, self_coloru);
					break;
				case PARCEL_GROUP:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						left, bottom, WEST, group_coloru);
					break;
				case PARCEL_OWNED:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						left, bottom, WEST, other_coloru);
					break;
				case PARCEL_FOR_SALE:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						left, bottom, WEST, for_sale_coloru);
					break;
				case PARCEL_AUCTION:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						left, bottom, WEST, auction_coloru);
					break;
				default:
					break;
				}
			}

			// East edge
			if (col < GRIDS_PER_EDGE-1)
			{
				U8 east_overlay = mOwnership[row*GRIDS_PER_EDGE+col+1];
				add_edge = east_overlay & PARCEL_WEST_LINE;
			}
			else
			{
				add_edge = TRUE;
			}

			if (add_edge)
			{
				switch(overlay & PARCEL_COLOR_MASK)
				{
				case PARCEL_SELF:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						right, bottom, EAST, self_coloru);
					break;
				case PARCEL_GROUP:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						right, bottom, EAST, group_coloru);
					break;
				case PARCEL_OWNED:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						right, bottom, EAST, other_coloru);
					break;
				case PARCEL_FOR_SALE:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						right, bottom, EAST, for_sale_coloru);
					break;
				case PARCEL_AUCTION:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						right, bottom, EAST, auction_coloru);
					break;
				default:
					break;
				}
			}

			// South edge
			if (overlay & PARCEL_SOUTH_LINE)
			{
				switch(overlay & PARCEL_COLOR_MASK)
				{
				case PARCEL_SELF:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						left, bottom, SOUTH, self_coloru);
					break;
				case PARCEL_GROUP:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						left, bottom, SOUTH, group_coloru);
					break;
				case PARCEL_OWNED:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						left, bottom, SOUTH, other_coloru);
					break;
				case PARCEL_FOR_SALE:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						left, bottom, SOUTH, for_sale_coloru);
					break;
				case PARCEL_AUCTION:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						left, bottom, SOUTH, auction_coloru);
					break;
				default:
					break;
				}
			}


			// North edge
			if (row < GRIDS_PER_EDGE-1)
			{
				U8 north_overlay = mOwnership[(row+1)*GRIDS_PER_EDGE+col];
				add_edge = north_overlay & PARCEL_SOUTH_LINE;
			}
			else
			{
				add_edge = TRUE;
			}

			if (add_edge)
			{
				switch(overlay & PARCEL_COLOR_MASK)
				{
				case PARCEL_SELF:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						left, top, NORTH, self_coloru);
					break;
				case PARCEL_GROUP:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						left, top, NORTH, group_coloru);
					break;
				case PARCEL_OWNED:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						left, top, NORTH, other_coloru);
					break;
				case PARCEL_FOR_SALE:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						left, top, NORTH, for_sale_coloru);
					break;
				case PARCEL_AUCTION:
					addPropertyLine(new_vertex_array, new_color_array, new_coord_array,
						left, top, NORTH, auction_coloru);
					break;
				default:
					break;
				}
			}
		}
	}

	// Now copy into static arrays for faster rendering.
	// Attempt to recycle old arrays if possible to avoid memory
	// shuffling.
	S32 new_vertex_count = new_vertex_array.count();
	
	if (!(mVertexArray && mColorArray && new_vertex_count == mVertexCount))
	{
		// ...need new arrays
		delete[] mVertexArray;
		mVertexArray = NULL;
		delete[] mColorArray;
		mColorArray = NULL;

		mVertexCount = new_vertex_count;

		if (new_vertex_count > 0)
		{
			mVertexArray   = new F32[3 * mVertexCount];
			mColorArray    = new U8 [4 * mVertexCount];
		}
	}

	// Copy the new data into the arrays
	S32 i;
	F32* vertex = mVertexArray;
	for (i = 0; i < mVertexCount; i++)
	{
		const LLVector3& point = new_vertex_array.get(i);
		*vertex = point.mV[VX];
		vertex++;
		*vertex = point.mV[VY];
		vertex++;
		*vertex = point.mV[VZ];
		vertex++;
	}

	U8* colorp = mColorArray;
	for (i = 0; i < mVertexCount; i++)
	{
		const LLColor4U& color = new_color_array.get(i);
		*colorp = color.mV[VRED];
		colorp++;
		*colorp = color.mV[VGREEN];
		colorp++;
		*colorp = color.mV[VBLUE];
		colorp++;
		*colorp = color.mV[VALPHA];
		colorp++;
	}
	
	// Everything's clean now
	mDirty = FALSE;
}


void LLViewerParcelOverlay::addPropertyLine(
				LLDynamicArray<LLVector3, 256>& vertex_array,
				LLDynamicArray<LLColor4U, 256>& color_array,
				LLDynamicArray<LLVector2, 256>& coord_array,
				const F32 start_x, const F32 start_y, 
				const U32 edge,
				const LLColor4U& color)
{
	LLColor4U underwater( color );
	underwater.mV[VALPHA] /= 2;

	LLSurface& land = mRegion->getLand();

	F32 dx;
	F32 dy;
	F32 tick_dx;
	F32 tick_dy;
	//const F32 LINE_WIDTH = 0.125f;
	const F32 LINE_WIDTH = 0.0625f;

	switch(edge)
	{
	case WEST:
		dx = 0.f;
		dy = 1.f;
		tick_dx = LINE_WIDTH;
		tick_dy = 0.f;
		break;

	case EAST:
		dx = 0.f;
		dy = 1.f;
		tick_dx = -LINE_WIDTH;
		tick_dy = 0.f;
		break;

	case NORTH:
		dx = 1.f;
		dy = 0.f;
		tick_dx = 0.f;
		tick_dy = -LINE_WIDTH;
		break;

	case SOUTH:
		dx = 1.f;
		dy = 0.f;
		tick_dx = 0.f;
		tick_dy = LINE_WIDTH;
		break;

	default:
		llerrs << "Invalid edge in addPropertyLine" << llendl;
		return;
	}

	F32 outside_x = start_x;
	F32 outside_y = start_y;
	F32 outside_z = 0.f;
	F32 inside_x  = start_x + tick_dx;
	F32 inside_y  = start_y + tick_dy;
	F32 inside_z  = 0.f;

	// First part, only one vertex
	outside_z = land.resolveHeightRegion( outside_x, outside_y );

	if (outside_z > 20.f) color_array.put( color );
	else color_array.put( underwater );

	vertex_array.put( LLVector3(outside_x, outside_y, outside_z) );
	coord_array.put(  LLVector2(outside_x - start_x, 0.f) );

	inside_x += dx * LINE_WIDTH;
	inside_y += dy * LINE_WIDTH;

	outside_x += dx * LINE_WIDTH;
	outside_y += dy * LINE_WIDTH;

	// Then the "actual edge"
	inside_z = land.resolveHeightRegion( inside_x, inside_y );
	outside_z = land.resolveHeightRegion( outside_x, outside_y );

	if (inside_z > 20.f) color_array.put( color );
	else color_array.put( underwater );

	if (outside_z > 20.f) color_array.put( color );
	else color_array.put( underwater );

	vertex_array.put( LLVector3(inside_x, inside_y, inside_z) );
	vertex_array.put( LLVector3(outside_x, outside_y, outside_z) );

	coord_array.put(  LLVector2(outside_x - start_x, 1.f) );
	coord_array.put(  LLVector2(outside_x - start_x, 0.f) );

	inside_x += dx * (dx - LINE_WIDTH);
	inside_y += dy * (dy - LINE_WIDTH);

	outside_x += dx * (dx - LINE_WIDTH);
	outside_y += dy * (dy - LINE_WIDTH);

	// Middle part, full width
	S32 i;
	const S32 GRID_STEP = S32( PARCEL_GRID_STEP_METERS );
	for (i = 1; i < GRID_STEP; i++)
	{
		inside_z = land.resolveHeightRegion( inside_x, inside_y );
		outside_z = land.resolveHeightRegion( outside_x, outside_y );

		if (inside_z > 20.f) color_array.put( color );
		else color_array.put( underwater );

		if (outside_z > 20.f) color_array.put( color );
		else color_array.put( underwater );

		vertex_array.put( LLVector3(inside_x, inside_y, inside_z) );
		vertex_array.put( LLVector3(outside_x, outside_y, outside_z) );

		coord_array.put(  LLVector2(outside_x - start_x, 1.f) );
		coord_array.put(  LLVector2(outside_x - start_x, 0.f) );

		inside_x += dx;
		inside_y += dy;

		outside_x += dx;
		outside_y += dy;
	}

	// Extra buffer for edge
	inside_x -= dx * LINE_WIDTH;
	inside_y -= dy * LINE_WIDTH;

	outside_x -= dx * LINE_WIDTH;
	outside_y -= dy * LINE_WIDTH;

	inside_z = land.resolveHeightRegion( inside_x, inside_y );
	outside_z = land.resolveHeightRegion( outside_x, outside_y );

	if (inside_z > 20.f) color_array.put( color );
	else color_array.put( underwater );

	if (outside_z > 20.f) color_array.put( color );
	else color_array.put( underwater );

	vertex_array.put( LLVector3(inside_x, inside_y, inside_z) );
	vertex_array.put( LLVector3(outside_x, outside_y, outside_z) );

	coord_array.put(  LLVector2(outside_x - start_x, 1.f) );
	coord_array.put(  LLVector2(outside_x - start_x, 0.f) );

	inside_x += dx * LINE_WIDTH;
	inside_y += dy * LINE_WIDTH;

	outside_x += dx * LINE_WIDTH;
	outside_y += dy * LINE_WIDTH;

	// Last edge is not drawn to the edge
	outside_z = land.resolveHeightRegion( outside_x, outside_y );

	if (outside_z > 20.f) color_array.put( color );
	else color_array.put( underwater );

	vertex_array.put( LLVector3(outside_x, outside_y, outside_z) );
	coord_array.put(  LLVector2(outside_x - start_x, 0.f) );
}


void LLViewerParcelOverlay::setDirty()
{
	mDirty = TRUE;
}

void LLViewerParcelOverlay::updateGL()
{
	updateOverlayTexture();
}

void LLViewerParcelOverlay::idleUpdate(bool force_update)
{
	LLMemType mt_iup(LLMemType::MTYPE_IDLE_UPDATE_PARCEL_OVERLAY);
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

S32 LLViewerParcelOverlay::renderPropertyLines	() 
{
	if (!gSavedSettings.getBOOL("ShowPropertyLines"))
	{
		return 0;
	}
	if (!mVertexArray || !mColorArray)
	{
		return 0;
	}

	LLSurface& land = mRegion->getLand();

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

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();

	// Move to appropriate region coords
	LLVector3 origin = mRegion->getOriginAgent();
	glTranslatef( origin.mV[VX], origin.mV[VY], origin.mV[VZ] );

	glTranslatef(pull_toward_camera.mV[VX], pull_toward_camera.mV[VY],
		pull_toward_camera.mV[VZ]);

	// Include +1 because vertices are fenceposts.
	// *2 because it's a quad strip
	const S32 GRID_STEP = S32( PARCEL_GRID_STEP_METERS );
	const S32 vertex_per_edge = 3 + 2 * (GRID_STEP-1) + 3;

	// Stomp the camera into two dimensions
	LLVector3 camera_region = mRegion->getPosRegionFromGlobal( gAgentCamera.getCameraPositionGlobal() );

	// Set up a cull plane 2 * PARCEL_GRID_STEP_METERS behind
	// the camera.  The cull plane normal is the camera's at axis.
	LLVector3 cull_plane_point = LLViewerCamera::getInstance()->getAtAxis();
	cull_plane_point *= -2.f * PARCEL_GRID_STEP_METERS;
	cull_plane_point += camera_region;

	LLVector3 vertex;

	const S32 BYTES_PER_COLOR = 4;
	const S32 FLOATS_PER_VERTEX = 3;
	//const S32 FLOATS_PER_TEX_COORD = 2;
	S32 i, j;
	S32 drawn = 0;
	F32* vertexp;
	U8* colorp;
	bool render_hidden = LLSelectMgr::sRenderHiddenSelections && LLFloaterReg::instanceVisible("build");

	const F32 PROPERTY_LINE_CLIP_DIST = 256.f;

	for (i = 0; i < mVertexCount; i += vertex_per_edge)
	{
		colorp  = mColorArray  + BYTES_PER_COLOR   * i;
		vertexp = mVertexArray + FLOATS_PER_VERTEX * i;

		vertex.mV[VX] = *(vertexp);
		vertex.mV[VY] = *(vertexp+1);
		vertex.mV[VZ] = *(vertexp+2);

		if (dist_vec_squared2D(vertex, camera_region) > PROPERTY_LINE_CLIP_DIST*PROPERTY_LINE_CLIP_DIST)
		{
			continue;
		}

		// Destroy vertex, transform to plane-local.
		vertex -= cull_plane_point;

		// negative dot product means it is in back of the plane
		if ( vertex * CAMERA_AT < 0.f )
		{
			continue;
		}

		gGL.begin(LLRender::TRIANGLE_STRIP);

		for (j = 0; j < vertex_per_edge; j++)
		{
			gGL.color4ubv(colorp);
			gGL.vertex3fv(vertexp);

			colorp  += BYTES_PER_COLOR;
			vertexp += FLOATS_PER_VERTEX;			
		}

		drawn += vertex_per_edge;

		gGL.end();
		
		if (render_hidden)
		{
			LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_GREATER);
			
			colorp  = mColorArray  + BYTES_PER_COLOR   * i;
			vertexp = mVertexArray + FLOATS_PER_VERTEX * i;

			gGL.begin(LLRender::TRIANGLE_STRIP);

			for (j = 0; j < vertex_per_edge; j++)
			{
				U8 color[4];
				color[0] = colorp[0];
				color[1] = colorp[1];
				color[2] = colorp[2];
				color[3] = colorp[3]/4;

				gGL.color4ubv(color);
				gGL.vertex3fv(vertexp);

				colorp  += BYTES_PER_COLOR;
				vertexp += FLOATS_PER_VERTEX;			
			}

			drawn += vertex_per_edge;

			gGL.end();
		}
		
	}

	glPopMatrix();

	return drawn;
}

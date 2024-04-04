/** 
 * @file llviewerparceloverlay.h
 * @brief LLViewerParcelOverlay class header file
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

#ifndef LL_LLVIEWERPARCELOVERLAY_H
#define LL_LLVIEWERPARCELOVERLAY_H

// The ownership data for land parcels.
// One of these structures per region.

#include "llbbox.h"
#include "llframetimer.h"
#include "lluuid.h"
#include "llviewertexture.h"
#include "llgl.h"

class LLViewerRegion;
class LLVector3;
class LLColor4U;
class LLVector2;

class LLViewerParcelOverlay : public LLGLUpdate
{
public:
	LLViewerParcelOverlay(LLViewerRegion* region, F32 region_width_meters);
	~LLViewerParcelOverlay();

	// ACCESS
	LLViewerTexture*		getTexture() const		{ return mTexture; }

	bool			isOwned(const LLVector3& pos) const;
	bool			isOwnedSelf(const LLVector3& pos) const;
	bool			isOwnedGroup(const LLVector3& pos) const;
	bool			isOwnedOther(const LLVector3& pos) const;

	// "encroaches" means the prim hangs over the parcel, but its center
	// might be in another parcel. for now, we simply test axis aligned 
	// bounding boxes which isn't perfect, but is close
	bool encroachesOwned(const std::vector<LLBBox>& boxes) const;
	bool encroachesOnUnowned(const std::vector<LLBBox>& boxes) const;
	bool encroachesOnNearbyParcel(const std::vector<LLBBox>& boxes) const;
	
	bool			isSoundLocal(const LLVector3& pos) const;

	bool			isBuildCameraAllowed(const LLVector3& pos) const;
	F32				getOwnedRatio() const;

	// Returns the number of vertices drawn
	S32				renderPropertyLines();
    void			renderPropertyLinesOnMinimap(F32 scale_pixels_per_meter, const F32* parcel_outline_color);

	U8				ownership( const LLVector3& pos) const;
	U8				parcelLineFlags( const LLVector3& pos) const;
	U8				parcelLineFlags(S32 row, S32 col) const;

	// MANIPULATE
	void	uncompressLandOverlay(S32 chunk, U8 *compressed_overlay);

	// Indicate property lines and overlay texture need to be rebuilt.
	void	setDirty();

	void	idleUpdate(bool update_now = false);
	void	updateGL();
    
private:
	// This is in parcel rows and columns, not grid rows and columns
	// Stored in bottom three bits.
	U8		ownership(S32 row, S32 col) const	
				{ return parcelFlags(row, col, (U8)0x7); }

    U8		parcelFlags(S32 row, S32 col, U8 flags) const;

	void	addPropertyLine(std::vector<LLVector3>& vertex_array,
				std::vector<LLColor4U>& color_array,
				std::vector<LLVector2>& coord_array,
				const F32 start_x, const F32 start_y, 
				const U32 edge, 
				const LLColor4U& color);

	void 	updateOverlayTexture();
	void	updatePropertyLines();
	
private:
	// Back pointer to the region that owns this structure.
	LLViewerRegion*	mRegion;

	S32				mParcelGridsPerEdge;

	LLPointer<LLViewerTexture> mTexture;
	LLPointer<LLImageRaw> mImageRaw;
	
	// Size: mParcelGridsPerEdge * mParcelGridsPerEdge
	// Each value is 0-3, PARCEL_AVAIL to PARCEL_SELF in the two low bits
	// and other flags in the upper bits.
	U8				*mOwnership;

	// Update propery lines and overlay texture
	bool			mDirty;
	LLFrameTimer	mTimeSinceLastUpdate;
	S32				mOverlayTextureIdx;
	
	S32				mVertexCount;
	F32*			mVertexArray;
	U8*				mColorArray;
};

#endif

/** 
 * @file llviewerparceloverlay.h
 * @brief LLViewerParcelOverlay class header file
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVIEWERPARCELOVERLAY_H
#define LL_LLVIEWERPARCELOVERLAY_H

// The ownership data for land parcels.
// One of these structures per region.

#include "lldarray.h"
#include "llframetimer.h"
#include "lluuid.h"
#include "llviewerimage.h"

class LLViewerRegion;
class LLVector3;
class LLColor4U;
class LLVector2;

class LLViewerParcelOverlay
{
public:
	LLViewerParcelOverlay(LLViewerRegion* region, F32 region_width_meters);
	~LLViewerParcelOverlay();

	// ACCESS
	LLImageGL*		getTexture() const		{ return mTexture; }

	BOOL			isOwned(const LLVector3& pos) const;
	BOOL			isOwnedSelf(const LLVector3& pos) const;
	BOOL			isOwnedGroup(const LLVector3& pos) const;
	BOOL			isOwnedOther(const LLVector3& pos) const;
	BOOL			isSoundLocal(const LLVector3& pos) const;

	BOOL			isBuildCameraAllowed(const LLVector3& pos) const;
	F32				getOwnedRatio() const;

	// Returns the number of vertices drawn
	S32				renderPropertyLines();

	U8				ownership( const LLVector3& pos) const;

	// MANIPULATE
	void	uncompressLandOverlay(S32 chunk, U8 *compressed_overlay);

	// Indicate property lines and overlay texture need to be rebuilt.
	void	setDirty();

	void	idleUpdate(bool update_now = false);

private:
	// This is in parcel rows and columns, not grid rows and columns
	// Stored in bottom three bits.
	U8		ownership(S32 row, S32 col) const	
				{ return 0x7 & mOwnership[row * mParcelGridsPerEdge + col]; }

	void	addPropertyLine(LLDynamicArray<LLVector3, 256>& vertex_array,
				LLDynamicArray<LLColor4U, 256>& color_array,
				LLDynamicArray<LLVector2, 256>& coord_array,
				const F32 start_x, const F32 start_y, 
				const U32 edge, 
				const LLColor4U& color);

	void 	updateOverlayTexture();
	void	updatePropertyLines();
	
private:
	// Back pointer to the region that owns this structure.
	LLViewerRegion*	mRegion;

	S32				mParcelGridsPerEdge;

	LLPointer<LLImageGL> mTexture;
	LLPointer<LLImageRaw> mImageRaw;
	
	// Size: mParcelGridsPerEdge * mParcelGridsPerEdge
	// Each value is 0-3, PARCEL_AVAIL to PARCEL_SELF in the two low bits
	// and other flags in the upper bits.
	U8				*mOwnership;

	// Update propery lines and overlay texture
	BOOL			mDirty;
	LLFrameTimer	mTimeSinceLastUpdate;
	S32				mOverlayTextureIdx;
	
	LLUUID			mLineImageID;
	S32				mVertexCount;
	F32*			mVertexArray;
	U8*				mColorArray;
};

#endif

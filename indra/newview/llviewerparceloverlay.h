/** 
 * @file llviewerparceloverlay.h
 * @brief LLViewerParcelOverlay class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLVIEWERPARCELOVERLAY_H
#define LL_LLVIEWERPARCELOVERLAY_H

// The ownership data for land parcels.
// One of these structures per region.

#include "lldarray.h"
#include "llframetimer.h"
#include "lluuid.h"
#include "llviewertexture.h"

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
	LLViewerTexture*		getTexture() const		{ return mTexture; }

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

	LLPointer<LLViewerTexture> mTexture;
	LLPointer<LLImageRaw> mImageRaw;
	
	// Size: mParcelGridsPerEdge * mParcelGridsPerEdge
	// Each value is 0-3, PARCEL_AVAIL to PARCEL_SELF in the two low bits
	// and other flags in the upper bits.
	U8				*mOwnership;

	// Update propery lines and overlay texture
	BOOL			mDirty;
	LLFrameTimer	mTimeSinceLastUpdate;
	S32				mOverlayTextureIdx;
	
	S32				mVertexCount;
	F32*			mVertexArray;
	U8*				mColorArray;
};

#endif

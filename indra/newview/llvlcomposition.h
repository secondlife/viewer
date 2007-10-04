/** 
 * @file llvlcomposition.h
 * @brief Viewer-side representation of a composition layer...
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLVLCOMPOSITION_H
#define LL_LLVLCOMPOSITION_H

#include "llviewerlayer.h"
#include "llviewerimage.h"

class LLSurface;

class LLVLComposition : public LLViewerLayer
{
public:
	LLVLComposition(LLSurface *surfacep, const U32 width, const F32 scale);
	/*virtual*/ ~LLVLComposition();

	void setSurface(LLSurface *surfacep);

	// Viewer side hack to generate composition values
	BOOL generateHeights(const F32 x, const F32 y, const F32 width, const F32 height);
	BOOL generateComposition();
	// Generate texture from composition values.
	BOOL generateTexture(const F32 x, const F32 y, const F32 width, const F32 height);		

	// Use these as indeces ito the get/setters below that use 'corner'
	enum ECorner
	{
		SOUTHWEST = 0,
		SOUTHEAST = 1,
		NORTHWEST = 2,
		NORTHEAST = 3,
		CORNER_COUNT = 4
	};
	LLUUID getDetailTextureID(S32 corner);
	LLViewerImage* getDetailTexture(S32 corner);
	F32 getStartHeight(S32 corner);
	F32 getHeightRange(S32 corner);

	void setDetailTextureID(S32 corner, const LLUUID& id);
	void setStartHeight(S32 corner, F32 start_height);
	void setHeightRange(S32 corner, F32 range);

	friend class LLVOSurfacePatch;
	friend class LLDrawPoolTerrain;
	void setParamsReady()		{ mParamsReady = TRUE; }
	BOOL getParamsReady() const	{ return mParamsReady; }
protected:
	BOOL mParamsReady;
	LLSurface *mSurfacep;
	BOOL mTexturesLoaded;

	LLPointer<LLViewerImage> mDetailTextures[CORNER_COUNT];
	LLPointer<LLImageRaw> mRawImages[CORNER_COUNT];

	F32 mStartHeight[CORNER_COUNT];
	F32 mHeightRange[CORNER_COUNT];

	F32 mTexScaleX;
	F32 mTexScaleY;
};

#endif //LL_LLVLCOMPOSITION_H

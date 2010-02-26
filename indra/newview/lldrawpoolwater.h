/** 
 * @file lldrawpoolwater.h
 * @brief LLDrawPoolWater class definition
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

#ifndef LL_LLDRAWPOOLWATER_H
#define LL_LLDRAWPOOLWATER_H

#include "lldrawpool.h"


class LLFace;
class LLHeavenBody;
class LLWaterSurface;

class LLDrawPoolWater: public LLFacePool
{
protected:
	LLPointer<LLViewerTexture> mHBTex[2];
	LLPointer<LLViewerTexture> mWaterImagep;
	LLPointer<LLViewerTexture> mWaterNormp;

public:
	static BOOL sSkipScreenCopy;
	static BOOL sNeedsReflectionUpdate;
	static BOOL sNeedsDistortionUpdate;
	static LLVector3 sLightDir;

	static LLColor4 sWaterFogColor;

	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD0	
	};

	virtual U32 getVertexDataMask() { return VERTEX_DATA_MASK; }

	LLDrawPoolWater();
	/*virtual*/ ~LLDrawPoolWater();

	/*virtual*/ LLDrawPool *instancePool();
	static void restoreGL();
	
	/*virtual*/ S32 getNumPostDeferredPasses() { return 0; } //getNumPasses(); }
	/*virtual*/ void beginPostDeferredPass(S32 pass);
	/*virtual*/ void endPostDeferredPass(S32 pass);
	/*virtual*/ void renderPostDeferred(S32 pass) { render(pass); }
	/*virtual*/ S32 getNumDeferredPasses() { return 1; }
	/*virtual*/ void renderDeferred(S32 pass = 0);

	/*virtual*/ S32 getNumPasses();
	/*virtual*/ void render(S32 pass = 0);
	/*virtual*/ void prerender();
	/*virtual*/ void renderForSelect();

	/*virtual*/ LLViewerTexture *getDebugTexture();
	/*virtual*/ LLColor3 getDebugColor() const; // For AGP debug display

	void renderReflection(LLFace* face);
	void shade();
};

void cgErrorCallback();

#endif // LL_LLDRAWPOOLWATER_H

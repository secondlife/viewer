/** 
 * @file lldrawpoolbump.h
 * @brief LLDrawPoolBump class definition
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#ifndef LL_LLDRAWPOOLBUMP_H
#define LL_LLDRAWPOOLBUMP_H

#include "lldrawpool.h"
#include "llstring.h"
#include "lltextureentry.h"
#include "lluuid.h"

class LLImageRaw;
class LLSpatialGroup;
class LLDrawInfo;
class LLViewerFetchedTexture;

class LLDrawPoolBump : public LLRenderPass
{
protected :
	LLDrawPoolBump(const U32 type):LLRenderPass(type) { mShiny = FALSE; }
public:
	static U32 sVertexMask;
	BOOL mShiny;
	
	virtual U32 getVertexDataMask() { return sVertexMask; }

	LLDrawPoolBump();

	virtual void render(S32 pass = 0);
	virtual void beginRenderPass( S32 pass );
	virtual void endRenderPass( S32 pass );
	virtual S32	 getNumPasses();
	/*virtual*/ void prerender();
	/*virtual*/ void pushBatch(LLDrawInfo& params, U32 mask, BOOL texture);

	void renderBump(U32 type, U32 mask);
	void renderGroup(LLSpatialGroup* group, U32 type, U32 mask, BOOL texture);
		
	S32 numBumpPasses();
	
	void beginShiny(bool invisible = false);
	void renderShiny(bool invisible = false);
	void endShiny(bool invisible = false);
	
	void beginFullbrightShiny();
	void renderFullbrightShiny();
	void endFullbrightShiny();

	void beginBump(U32 pass = LLRenderPass::PASS_BUMP);
	void renderBump(U32 pass = LLRenderPass::PASS_BUMP);
	void endBump(U32 pass = LLRenderPass::PASS_BUMP);

	virtual S32 getNumDeferredPasses();
	/*virtual*/ void beginDeferredPass(S32 pass);
	/*virtual*/ void endDeferredPass(S32 pass);
	/*virtual*/ void renderDeferred(S32 pass);

	virtual S32 getNumPostDeferredPasses() { return 2; }
	/*virtual*/ void beginPostDeferredPass(S32 pass);
	/*virtual*/ void endPostDeferredPass(S32 pass);
	/*virtual*/ void renderPostDeferred(S32 pass);

	BOOL bindBumpMap(LLDrawInfo& params, S32 channel = -2);
};

enum EBumpEffect
{
	BE_NO_BUMP = 0,
	BE_BRIGHTNESS = 1,
	BE_DARKNESS = 2,
	BE_STANDARD_0 = 3,  // Standard must always be the last one
	BE_COUNT = 4
};

////////////////////////////////////////////////////////////////
// List of standard bumpmaps that are specificed by LLTextureEntry::mBump's lower bits

class LLStandardBumpmap
{
public: 
	LLStandardBumpmap() : mLabel() {} 
	LLStandardBumpmap( const std::string& label ) : mLabel(label) {}
	
	std::string	mLabel;
	LLPointer<LLViewerFetchedTexture> mImage;

	static	U32 sStandardBumpmapCount;  // Number of valid values in gStandardBumpmapList[]

	static void init();
	static void shutdown();
	static void	restoreGL();
	static void destroyGL();
};

extern LLStandardBumpmap gStandardBumpmapList[TEM_BUMPMAP_COUNT];

////////////////////////////////////////////////////////////////
// List of one-component bump-maps created from other texures.

struct LLBumpImageEntry;

class LLBumpImageList
{
public:
	LLBumpImageList() {}
	~LLBumpImageList();

	void		init();
	void		shutdown();
	void            clear();
	void		destroyGL();
	void		restoreGL();
	void		updateImages();


	LLViewerTexture*	getBrightnessDarknessImage(LLViewerFetchedTexture* src_image, U8 bump_code);
	void		addTextureStats(U8 bump, const LLUUID& base_image_id, F32 virtual_size);

	static void onSourceBrightnessLoaded( BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata );
	static void onSourceDarknessLoaded( BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata );
	static void onSourceStandardLoaded( BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata );
	static void generateNormalMapFromAlpha(LLImageRaw* src, LLImageRaw* nrm_image);


private:
	static void onSourceLoaded( BOOL success, LLViewerTexture *src_vi, LLImageRaw* src, LLUUID& source_asset_id, EBumpEffect bump );

private:
	typedef std::map<LLUUID, LLPointer<LLViewerTexture> > bump_image_map_t;
	bump_image_map_t mBrightnessEntries;
	bump_image_map_t mDarknessEntries;
};

extern LLBumpImageList gBumpImageList;

class LLDrawPoolInvisible : public LLDrawPoolBump
{
public:
	LLDrawPoolInvisible() : LLDrawPoolBump(LLDrawPool::POOL_INVISIBLE) { }

	enum
	{
		VERTEX_DATA_MASK = LLVertexBuffer::MAP_VERTEX
	};
	
	virtual U32 getVertexDataMask() { return VERTEX_DATA_MASK; }

	virtual void prerender() { }

	virtual void render(S32 pass = 0);
	virtual void beginRenderPass( S32 pass ) { }
	virtual void endRenderPass( S32 pass ) { }
	virtual S32	 getNumPasses() {return 1;}

	virtual S32 getNumDeferredPasses() { return 1; }
	/*virtual*/ void beginDeferredPass(S32 pass);
	/*virtual*/ void endDeferredPass(S32 pass);
	/*virtual*/ void renderDeferred(S32 pass);
};


#endif // LL_LLDRAWPOOLBUMP_H

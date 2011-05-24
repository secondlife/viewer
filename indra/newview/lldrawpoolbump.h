/** 
 * @file lldrawpoolbump.h
 * @brief LLDrawPoolBump class definition
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLDRAWPOOLBUMP_H
#define LL_LLDRAWPOOLBUMP_H

#include "lldrawpool.h"
#include "llstring.h"
#include "lltextureentry.h"
#include "lluuid.h"

class LLImageRaw;
class LLSpatialGroup;
class LLDrawInfo;
class LLGLSLShader;
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

	static void bindCubeMap(LLGLSLShader* shader, S32 shader_level, S32& diffuse_channel, S32& cube_channel, bool invisible);
	static void unbindCubeMap(LLGLSLShader* shader, S32 shader_level, S32& diffuse_channel, S32& cube_channel, bool invisible);

	virtual S32 getNumDeferredPasses();
	/*virtual*/ void beginDeferredPass(S32 pass);
	/*virtual*/ void endDeferredPass(S32 pass);
	/*virtual*/ void renderDeferred(S32 pass);

	virtual S32 getNumPostDeferredPasses() { return 2; }
	/*virtual*/ void beginPostDeferredPass(S32 pass);
	/*virtual*/ void endPostDeferredPass(S32 pass);
	/*virtual*/ void renderPostDeferred(S32 pass);

	static BOOL bindBumpMap(LLDrawInfo& params, S32 channel = -2);
	static BOOL bindBumpMap(LLFace* face, S32 channel = -2);

private:
	static BOOL bindBumpMap(U8 bump_code, LLViewerTexture* tex, F32 vsize, S32 channel);

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

	static void clear();
	static void addstandard();

	static void init();
	static void shutdown();
	static void restoreGL();
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

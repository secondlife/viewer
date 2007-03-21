/** 
 * @file lldrawpoolbump.h
 * @brief LLDrawPoolBump class definition
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
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

class LLDrawPoolBump : public LLRenderPass
{
public:
	static U32 sVertexMask;

	virtual U32 getVertexDataMask() { return sVertexMask; }

	LLDrawPoolBump();

	virtual void render(S32 pass = 0);
	/*virtual*/ void beginRenderPass( S32 pass );
	/*virtual*/ void endRenderPass( S32 pass );
	/*virtual*/ S32	 getNumPasses();
	/*virtual*/ void prerender();
	/*virtual*/ void pushBatch(LLDrawInfo& params, U32 mask, BOOL texture);

	void renderBump(U32 type, U32 mask);
	void renderBumpActive(U32 type, U32 mask);
	void renderGroup(LLSpatialGroup* group, U32 type, U32 mask, BOOL texture);
	void renderGroupBump(LLSpatialGroup* group, U32 type, U32 mask);
	
	S32 numBumpPasses();
	
	void beginShiny();
	void renderShiny();
	void endShiny();
	void renderActive(U32 type, U32 mask, BOOL texture = TRUE);

	void beginBump();
	void renderBump();
	void endBump();
	BOOL bindBumpMap(LLDrawInfo& params);
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
	LLStandardBumpmap() : mLabel("") {} 
	LLStandardBumpmap( const char* label ) : mLabel(label) {}
	
	LLString	mLabel;
	LLPointer<LLViewerImage> mImage;

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
	void		destroyGL();
	void		restoreGL();
	void		updateImages();


	LLImageGL*	getBrightnessDarknessImage(LLViewerImage* src_image, U8 bump_code);
//	LLImageGL*	getTestImage();
	void		addTextureStats(U8 bump, const LLUUID& base_image_id,
								F32 pixel_area, F32 texel_area_ratio, F32 cos_center_angle);

	static void onSourceBrightnessLoaded( BOOL success, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata );
	static void onSourceDarknessLoaded( BOOL success, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata );

private:
	static void onSourceLoaded( BOOL success, LLViewerImage *src_vi, LLImageRaw* src, LLUUID& source_asset_id, EBumpEffect bump );

private:
	typedef std::map<LLUUID, LLPointer<LLImageGL> > bump_image_map_t;
	bump_image_map_t mBrightnessEntries;
	bump_image_map_t mDarknessEntries;
};

extern LLBumpImageList gBumpImageList;



#endif // LL_LLDRAWPOOLBUMP_H

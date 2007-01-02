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

class LLDrawPoolBump : public LLDrawPool
{
protected:
	LLPointer<LLViewerImage> mTexturep;  // The primary texture, not the bump texture

public:
	LLDrawPoolBump(LLViewerImage *texturep);

	/*virtual*/ LLDrawPool *instancePool();

	/*virtual*/ void render(S32 pass = 0);
	/*virtual*/ void beginRenderPass( S32 pass );
	/*virtual*/ void endRenderPass( S32 pass );
	/*virtual*/ S32	 getNumPasses();
	/*virtual*/ void renderFaceSelected(LLFace *facep, LLImageGL *image, const LLColor4 &color,
										const S32 index_offset = 0, const S32 index_count = 0);
	/*virtual*/ void prerender();
	/*virtual*/ void renderForSelect();
	/*virtual*/ void dirtyTexture(const LLViewerImage *texturep);
	/*virtual*/ LLViewerImage *getTexture();
	/*virtual*/ LLViewerImage *getDebugTexture();
	/*virtual*/ LLColor3 getDebugColor() const; // For AGP debug display
	/*virtual*/ BOOL match(LLFace* last_face, LLFace* facep);
	
	virtual S32 getMaterialAttribIndex();
	static S32 numBumpPasses();
	
	static void beginPass0(LLDrawPool* pool);
	static S32  renderPass0(LLDrawPool* pool, face_array_t& face_list, const U32* index_array, LLViewerImage* tex);
	static void endPass0(LLDrawPool* pool);

	static void beginPass1();
	static S32  renderPass1(face_array_t& face_list, const U32* index_array, LLViewerImage* tex);
	static void endPass1();

	static void beginPass2();
	static S32  renderPass2(face_array_t& face_list, const U32* index_array, LLViewerImage* tex);
	static void endPass2();

	/*virtual*/ void enableShade();
	/*virtual*/ void disableShade();
	/*virtual*/ void setShade(F32 shade);

protected:
	static LLImageGL* getBumpMap(const LLTextureEntry* te, LLViewerImage* tex);

public:
	static S32 sBumpTex;
	static S32 sDiffTex;
	static S32 sEnvTex;
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

/** 
 * @file lldrawpool.h
 * @brief LLDrawPool class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#ifndef LL_LLDRAWPOOL_H
#define LL_LLDRAWPOOL_H

#include "v4coloru.h"
#include "v2math.h"
#include "v3math.h"
#include "llvertexbuffer.h"

class LLFace;
class LLImageGL;
class LLViewerImage;
class LLSpatialGroup;
class LLDrawInfo;

#define DEFAULT_MAX_VERTICES 65535

class LLDrawPool
{
public:
	static S32 sNumDrawPools;

	enum
	{
		// Correspond to LLPipeline render type
		POOL_SKY = 1,
		POOL_STARS,
		POOL_GROUND,
		POOL_TERRAIN,	
		POOL_SIMPLE,
		POOL_BUMP,
		POOL_AVATAR,
		POOL_TREE,
		POOL_GLOW,
		POOL_ALPHA,
		POOL_WATER,
		POOL_ALPHA_POST_WATER,
		NUM_POOL_TYPES,
	};
	
	LLDrawPool(const U32 type);
	virtual ~LLDrawPool();

	virtual BOOL isDead() = 0;

	S32 getId() const { return mId; }
	U32 getType() const { return mType; }

	virtual LLViewerImage *getDebugTexture();
	virtual void beginRenderPass( S32 pass );
	virtual void endRenderPass( S32 pass );
	virtual S32	 getNumPasses() { return 1; }
	virtual void render(S32 pass = 0) = 0;
	virtual void prerender() = 0;
	virtual S32 getMaterialAttribIndex() = 0;
	virtual U32 getVertexDataMask() = 0;
	virtual BOOL verify() const { return TRUE; }		// Verify that all data in the draw pool is correct!
	virtual S32 getVertexShaderLevel() const { return mVertexShaderLevel; }
	
	static LLDrawPool* createPool(const U32 type, LLViewerImage *tex0 = NULL);
	virtual LLDrawPool *instancePool() = 0;	// Create an empty new instance of the pool.
	virtual LLViewerImage* getTexture() = 0;
	virtual BOOL isFacePool() { return FALSE; }
	virtual void resetDrawOrders() = 0;

	U32	getTrianglesDrawn() const;
	void resetTrianglesDrawn();
	void addIndicesDrawn(const U32 indices);

protected:
	S32 mVertexShaderLevel;
	S32	mId;
	U32 mType;				// Type of draw pool
	S32	mIndicesDrawn;
};

class LLRenderPass : public LLDrawPool
{
public:
	enum
	{
		PASS_SIMPLE = NUM_POOL_TYPES,
		PASS_GLOW,
		PASS_FULLBRIGHT,
		PASS_INVISIBLE,
		PASS_SHINY,
		PASS_BUMP,
		PASS_GRASS,
		PASS_ALPHA,
		NUM_RENDER_TYPES,
	};

	LLRenderPass(const U32 type);
	virtual ~LLRenderPass();
	/*virtual*/ LLDrawPool* instancePool();
	/*vritual*/ S32 getMaterialAttribIndex() { return -1; }
	/*virtual*/ LLViewerImage* getDebugTexture() { return NULL; }
	LLViewerImage* getTexture() { return NULL; }
	BOOL isDead() { return FALSE; }
	void resetDrawOrders() { }

	virtual void pushBatch(LLDrawInfo& params, U32 mask, BOOL texture);
	virtual void renderGroup(LLSpatialGroup* group, U32 type, U32 mask, BOOL texture = TRUE);
	virtual void renderStatic(U32 type, U32 mask, BOOL texture = TRUE);
	virtual void renderActive(U32 type, U32 mask, BOOL texture = TRUE);
	virtual void renderInvisible(U32 mask);
	virtual void renderTexture(U32 type, U32 mask);

};

class LLFacePool : public LLDrawPool
{
public:
	typedef std::vector<LLFace*> face_array_t;
	
	enum
	{
		SHADER_LEVEL_SCATTERING = 2
	};

public:
	LLFacePool(const U32 type);
	virtual ~LLFacePool();
	
	virtual void renderForSelect() = 0;
	BOOL isDead() { return mReferences.empty(); }
	virtual void renderFaceSelected(LLFace *facep, LLImageGL *image, const LLColor4 &color,
									const S32 index_offset = 0, const S32 index_count = 0);

	virtual LLViewerImage *getTexture();
	virtual void dirtyTextures(const std::set<LLViewerImage*>& textures);

	virtual void enqueue(LLFace *face);
	virtual BOOL addFace(LLFace *face);
	virtual BOOL removeFace(LLFace *face);

	virtual BOOL verify() const;		// Verify that all data in the draw pool is correct!
	
	virtual void resetDrawOrders();
	void resetAll();

	BOOL moveFace(LLFace *face, LLDrawPool *poolp, BOOL copy_data = FALSE);

	void destroy();

	void buildEdges();

	static S32 drawLoop(face_array_t& face_list);
	static S32 drawLoopSetTex(face_array_t& face_list, S32 stage);
	void drawLoop();

	void renderVisibility();

	void addFaceReference(LLFace *facep);
	void removeFaceReference(LLFace *facep);

	void printDebugInfo() const;
	
	BOOL isFacePool() { return TRUE; }

	friend class LLFace;
	friend class LLPipeline;
public:
	face_array_t	mDrawFace;
	face_array_t	mMoveFace;
	face_array_t	mReferences;

public:
	class LLOverrideFaceColor
	{
	public:
		LLOverrideFaceColor(LLDrawPool* pool)
			: mOverride(sOverrideFaceColor), mPool(pool)
		{
			sOverrideFaceColor = TRUE;
		}
		LLOverrideFaceColor(LLDrawPool* pool, const LLColor4& color)
			: mOverride(sOverrideFaceColor), mPool(pool)
		{
			sOverrideFaceColor = TRUE;
			setColor(color);
		}
		LLOverrideFaceColor(LLDrawPool* pool, const LLColor4U& color)
			: mOverride(sOverrideFaceColor), mPool(pool)
		{
			sOverrideFaceColor = TRUE;
			setColor(color);
		}
		LLOverrideFaceColor(LLDrawPool* pool, F32 r, F32 g, F32 b, F32 a)
			: mOverride(sOverrideFaceColor), mPool(pool)
		{
			sOverrideFaceColor = TRUE;
			setColor(r, g, b, a);
		}
		~LLOverrideFaceColor()
		{
			sOverrideFaceColor = mOverride;
		}
		void setColor(const LLColor4& color);
		void setColor(const LLColor4U& color);
		void setColor(F32 r, F32 g, F32 b, F32 a);
		BOOL mOverride;
		LLDrawPool* mPool;
		static BOOL sOverrideFaceColor;
	};
};

#endif //LL_LLDRAWPOOL_H

/** 
 * @file lldrawpool.h
 * @brief LLDrawPool class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDRAWPOOL_H
#define LL_LLDRAWPOOL_H

#include "llagparray.h"
#include "lldarray.h"
#include "lldlinked.h"
#include "llstrider.h"
#include "llviewerimage.h"
#include "v4coloru.h"
#include "v2math.h"
#include "v3math.h"
#include "llstrider.h"

class LLFace;
class LLImageGL;
class LLViewerImage;

#define DEFAULT_MAX_VERTICES 65535

class LLDrawPool
{
public:
	typedef LLDynamicArray<LLFace*, 128> face_array_t;
	
	enum
	{
		SHADER_LEVEL_SCATTERING = 2
	};

public:
	LLDrawPool(const U32 type, const U32 data_mask_il, const U32 data_mask_nil); 
	virtual ~LLDrawPool();

	static LLDrawPool* createPool(const U32 type, LLViewerImage *tex0 = NULL);

	void flushAGP(); // Flush the AGP buffers so they can be repacked and reallocated.
	void syncAGP();

	virtual LLDrawPool *instancePool() = 0;	// Create an empty new instance of the pool.
	virtual void beginRenderPass( S32 pass );
	virtual void endRenderPass( S32 pass );
	virtual S32	 getNumPasses() { return 1; }
	virtual void render(S32 pass = 0) = 0;
	virtual void renderForSelect() = 0;
	virtual BOOL match(LLFace* last_face, LLFace* facep) { return FALSE; }
	virtual void renderFaceSelected(LLFace *facep, LLImageGL *image, const LLColor4 &color,
									const S32 index_offset = 0, const S32 index_count = 0);

	virtual void prerender() = 0;
	virtual S32 rebuild();

	virtual S32 getMaterialAttribIndex() = 0;

	virtual LLViewerImage *getTexture();
	virtual LLViewerImage *getDebugTexture();
	virtual void dirtyTexture(const LLViewerImage* texturep);

	virtual void enqueue(LLFace *face);
	virtual BOOL addFace(LLFace *face);
	virtual BOOL removeFace(LLFace *face);

	virtual BOOL verify() const;		// Verify that all data in the draw pool is correct!
	virtual LLColor3 getDebugColor() const; // For AGP debug display

	virtual void resetDrawOrders();
	virtual void resetVertexData(S32 reserve_count);
	virtual void resetIndices(S32 num_indices);
	void resetAll();

	BOOL moveFace(LLFace *face, LLDrawPool *poolp, BOOL copy_data = FALSE);


	S32 getId() const { return mId; }
	U32 getType() const { return mType; }

	const U32 getStride() const;
	inline const U32 getStride(const U32 data_type) const;
	inline const U32 getOffset(const U32 data_type) const;

	S32	reserveGeom(U32 count);
	S32	reserveInd (U32 count);
	S32 unReserveGeom(const S32 index, const U32 count);
	S32 unReserveInd(const S32 index, const U32 count);

	void bindGLVertexPointer();
	void bindGLTexCoordPointer(const U32 pass=0);
	void bindGLNormalPointer();
	void bindGLBinormalPointer(S32 index);
	void bindGLColorPointer();
	void bindGLVertexWeightPointer(S32 index);
	void bindGLVertexClothingWeightPointer(S32 index);

	const U32  getIndexCount() const;
	const U32  getTexCoordCount(const U32 pass=0) const;
	const U32  getVertexCount() const;
	const U32  getNormalCount() const;
	const U32  getBinormalCount() const;
	const U32  getColorCount() const;
	const U32  getVertexWeightCount() const;

	void  setDirty();
	void  setDirtyMemory()						{ mMemory.setDirty(); }
	void  setDirtyWeights()						{ mWeights.setDirty(); }

	const U32* getRawIndices() const			{ return mIndices.getMem(); }

	U32 getIndex(const S32 index)				{ return mIndices[index]; } // Use to get one index
	U32 *getIndices(const S32 index); 			// Used to get an array of indices for reading/writing
	void CheckIntegrity(); // DEBUG
	
	const LLVector3& getVertex(const S32 index);
	const LLVector2& getTexCoord(const S32 index, const U32 pass);
	const LLVector3& getNormal(const S32 index);
	const LLVector3& getBinormal(const S32 index);
	const LLColor4U& getColor(const S32 index);
	const F32& getVertexWeight(const S32 index);
	const LLVector4& getClothingWeight(const S32 index);

	void setRebuild(const BOOL rebuild);

	void destroy();

	void buildEdges();

	static S32 drawLoop(face_array_t& face_list, const U32* index_array);
	static S32 drawLoopSetTex(face_array_t& face_list, const U32* index_array, S32 stage);
	void drawLoop();

	void renderVisibility();

	void addFaceReference(LLFace *facep);
	void removeFaceReference(LLFace *facep);
	U32	getTrianglesDrawn() const;
	void resetTrianglesDrawn();
	void addIndicesDrawn(const U32 indices);

	void printDebugInfo() const;
	S32 getMemUsage(const BOOL print = FALSE);
	
	BOOL setUseAGP(BOOL use_agp);
	BOOL canUseAGP() const		{ return mMemory.isAGP(); } // Return TRUE if this pool can use AGP

	S32 getMaxVertices() const	{ return mMaxVertices; }
	S32 getVertexShaderLevel() const { return mVertexShaderLevel; }
	
	friend class LLFace;
	friend class LLPipeline;
public:

	enum
	{
		// Correspond to LLPipeline render type
		POOL_SKY = 1,
		POOL_STARS,
		POOL_GROUND,
		POOL_TERRAIN,	
		POOL_SIMPLE,
		POOL_MEDIA,		// unused
		POOL_BUMP,
		POOL_AVATAR,
		POOL_TREE,
		POOL_TREE_NEW,
		POOL_WATER,		
		POOL_CLOUDS,
		POOL_ALPHA,
		POOL_HUD,
	};


	// If you change the order or add params to these, you also need to adjust the sizes in the
	// mDataSizes array defined in lldrawpool.cpp
	typedef enum e_data_type
	{
		DATA_VERTICES		= 0,
		DATA_TEX_COORDS0	= 1,
		DATA_TEX_COORDS1	= 2,
		DATA_TEX_COORDS2	= 3,
		DATA_TEX_COORDS3	= 4,
		DATA_NORMALS		= 5,
		DATA_VERTEX_WEIGHTS = 6,
		DATA_CLOTHING_WEIGHTS = 7,
		DATA_BINORMALS		= 8,
		DATA_COLORS			= 9,
		DATA_MAX_TYPES		= 10
	} EDataType;

	typedef enum e_data_mask
	{
		DATA_VERTICES_MASK			= 1 << DATA_VERTICES,
		DATA_TEX_COORDS0_MASK		= 1 << DATA_TEX_COORDS0,
		DATA_TEX_COORDS1_MASK		= 1 << DATA_TEX_COORDS1,
		DATA_TEX_COORDS2_MASK		= 1 << DATA_TEX_COORDS2,
		DATA_TEX_COORDS3_MASK		= 1 << DATA_TEX_COORDS3,
		DATA_NORMALS_MASK			= 1 << DATA_NORMALS,
		DATA_VERTEX_WEIGHTS_MASK	= 1 << DATA_VERTEX_WEIGHTS,
		DATA_CLOTHING_WEIGHTS_MASK	= 1 << DATA_CLOTHING_WEIGHTS,
		DATA_BINORMALS_MASK			= 1 << DATA_BINORMALS,
		DATA_COLORS_MASK			= 1 << DATA_COLORS,

		// Masks for standard types.
		// IL for interleaved, NIL for non-interleaved.
		DATA_SIMPLE_IL_MASK				= DATA_VERTICES_MASK | DATA_TEX_COORDS0_MASK | DATA_NORMALS_MASK,
		DATA_SIMPLE_NIL_MASK			= 0,
		DATA_BUMP_IL_MASK				= DATA_SIMPLE_IL_MASK | DATA_BINORMALS_MASK | DATA_TEX_COORDS1_MASK,
	} EDataMask;

	face_array_t	mDrawFace;
	face_array_t	mMoveFace;
	face_array_t	mReferences;

	U32 mDataMaskIL;	// Interleaved data
	U32 mDataMaskNIL;	// Non-interleaved data
	U32 mDataOffsets[DATA_MAX_TYPES];
	S32 mStride;

	S32	mRebuildFreq;
	S32	mRebuildTime;
	S32	mGeneration;

	
	S32 mSkippedVertices;

	static U32 sDataSizes[DATA_MAX_TYPES];
	static S32 sNumDrawPools;

protected:
	LLAGPArray<U8>          mMemory;
	LLAGPArray<F32>			mWeights;
	LLAGPArray<LLVector4>	mClothingWeights;
	LLAGPArray<U32>         mIndices;

public:

	BOOL getVertexStrider      (LLStrider<LLVector3> &vertices,   const U32 index = 0);
	BOOL getTexCoordStrider    (LLStrider<LLVector2> &tex_coords, const U32 index = 0, const U32 pass=0);
	BOOL getNormalStrider      (LLStrider<LLVector3> &normals,    const U32 index = 0);
	BOOL getBinormalStrider    (LLStrider<LLVector3> &binormals,  const U32 index = 0);
	BOOL getColorStrider   	   (LLStrider<LLColor4U> &colors,  const U32 index = 0);
	BOOL getVertexWeightStrider(LLStrider<F32>       &vertex_weights, const U32 index = 0);
	BOOL getClothingWeightStrider(LLStrider<LLVector4>       &clothing_weights, const U32 index = 0);

public:
	enum { NUM_BUCKETS = 8 };	// Need to change freeListBucket() if NUM_BUCKETS changes
	struct FreeListNode
	{
		U32 count;
		S32 next;
	};
protected:
	int freeListBucket(U32 count);
	void freeListAddGeom(S32 index, U32 count);
	void freeListAddInd(S32 index, U32 count);
	S32 freeListFindGeom(U32 count);
	S32 freeListFindInd(U32 count);
	
protected:
	BOOL mUseAGP;
	S32 mVertexShaderLevel;
	S32	mId;
	U32 mType;				// Type of draw pool
	S32 mMaxVertices;
	S32	mIndicesDrawn;
	BOOL mCleanupUnused; // Cleanup unused data when too full

	S32 mFreeListGeomHead[8];
	S32 mFreeListIndHead[8];

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

	virtual void enableShade();
	virtual void disableShade();
	virtual void setShade(F32 shade);
	
};

inline const U32 LLDrawPool::getStride() const
{
	return mStride;
}

inline const U32 LLDrawPool::getOffset(const U32 data_type) const
{
	return  mDataOffsets[data_type];
}

inline const U32 LLDrawPool::getStride(const U32 data_type) const
{
	if (mDataMaskIL & (1 << data_type))
	{
		return mStride;
	}
	else if (mDataMaskNIL & (1 << data_type))
	{
		return 0;
	}
	else
	{
		llerrs << "Getting stride for unsupported data type " << data_type << llendl;
		return 0;
	}
}

#endif //LL_LLDRAWPOOL_H

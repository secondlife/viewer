/** 
 * @file llface.h
 * @brief LLFace class definition
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFACE_H
#define LL_LLFACE_H

#include "llstrider.h"

#include "v2math.h"
#include "v3math.h"
#include "v4math.h"
#include "m4math.h"
#include "v4coloru.h"
#include "llquaternion.h"
#include "xform.h"
#include "lldarrayptr.h"
#include "llpagemem.h"
#include "llstat.h"
#include "lldrawable.h"

#define ENABLE_FACE_LINKING 1 // Causes problems with snapshot rendering

class LLDrawPool;
class LLVolume;
class LLViewerImage;
class LLTextureEntry;
class LLVertexProgram;
class LLViewerImage;

class LLFace
{
public:

	enum EMasks
	{
		SHARED_GEOM	 = 0x0001,
		LIGHT		 = 0x0002,
		REBUILD		 = 0x0004,	
		GLOBAL		 = 0x0008,
		VISIBLE		 = 0x0010,
		BACKLIST	 = 0x0020,
		INTERP		 = 0x0040,
		FULLBRIGHT	 = 0x0080,
		HUD_RENDER	 = 0x0100,
		USE_FACE_COLOR	 = 0x0200,

		POINT_SPRITE = 0x10000,
		BOARD_SPRITE = 0x20000,
		FIXED_SPRITE = 0x40000,
		DEPTH_SPRITE = 0x80000
	};

	enum EDirty
	{
		DIRTY = -2
	};

	static void initClass();

public:
	LLFace(LLDrawable* drawablep, LLViewerObject* objp)   { init(drawablep, objp); }
	~LLFace()  { destroy(); }

	const LLMatrix4& getWorldMatrix()	const	{ return mVObjp->getWorldMatrix(mXform); }
	const LLMatrix4& getRenderMatrix() const;
	const U32		getIndicesCount()	const	{ return mIndicesCount; };
	const S32		getIndicesStart()	const	{ return mIndicesIndex; };
	const S32		getGeomCount()		const	{ return mGeomCount; }		// vertex count for this face
	const S32		getGeomIndex()		const	{ return mGeomIndex; }		// index into draw pool
	const U32		getGeomStart()		const	{ return mGeomIndex; }		// index into draw pool
	LLViewerImage*	getTexture()		const	{ return mTexture; }
	LLXformMatrix*	getXform()			const	{ return mXform; }
	BOOL			hasGeometry()		const	{ return mGeomCount > 0; }
	LLVector3		getPositionAgent()	const;
	void			setPrimType(U32 primType)	{ mPrimType = primType; }
	const U32		getPrimType()		const	{ return mPrimType; }

	U32				getState()			const	{ return mState; }
	void			setState(U32 state)			{ mState |= state; }
	void			clearState(U32 state)		{ mState &= ~state; }
	BOOL			isState(U32 state)	const	{ return ((mState & state) != 0); }

	void			bindTexture(S32 stage = 0)		const	{ LLViewerImage::bindTexture(mTexture, stage); }

	void			enableLights() const;
	void			renderSetColor() const;
	S32				renderElements(const U32 *index_array) const;
	S32				renderIndexed (const U32 *index_array) const;
	S32				pushVertices(const U32* index_array) const;
	
	void			setWorldMatrix(const LLMatrix4& mat);
	const LLTextureEntry* getTextureEntry()	const { return mVObjp->getTE(mTEOffset); }

	LLDrawPool*		getPool()			const	{ return mDrawPoolp; }
	S32				getStride()			const	{ return mDrawPoolp->getStride(); }
	const U32*		getRawIndices()		const	{ return &mDrawPoolp->mIndices[mIndicesIndex]; }
	LLDrawable*		getDrawable()		const	{ return mDrawablep; }
	LLViewerObject*	getViewerObject()	const	{ return mVObjp; }

	void			clearDirty() { mGeneration = mDrawPoolp->mGeneration; };

	S32				backup();
	void			restore();

	void			setViewerObject(LLViewerObject* object);
	void			setPool(LLDrawPool *pool, LLViewerImage *texturep);
	void			setDrawable(LLDrawable *drawable);
	void			setTEOffset(const S32 te_offset);
	S32				getTEOffset() { return mTEOffset; }

	void			setFaceColor(const LLColor4& color); // override material color
	void			unsetFaceColor(); // switch back to material color
	const LLColor4&	getFaceColor() const { return mFaceColor; } 
	const LLColor4& getRenderColor() const;
	
	void unReserve();		// Set Removed from pool
	
	BOOL reserveIfNeeded(); // Reserves data if dirty.
	
	// For avatar
	S32				 getGeometryAvatar(
									LLStrider<LLVector3> &vertices,
									LLStrider<LLVector3> &normals,
									LLStrider<LLVector3> &binormals,
								    LLStrider<LLVector2> &texCoords,
									LLStrider<F32>		 &vertex_weights,
									LLStrider<LLVector4> &clothing_weights);

	// For terrain
	S32				getGeometryTerrain(LLStrider<LLVector3> &vertices,
									   LLStrider<LLVector3> &normals,
									   LLStrider<LLColor4U> &colors,
									   LLStrider<LLVector2> &texCoords0,
									   LLStrider<LLVector2> &texCoords1,
									   U32* &indices);

	// For volumes, etc.
	S32				getGeometry(LLStrider<LLVector3> &vertices,  
								LLStrider<LLVector3> &normals,
								LLStrider<LLVector2> &texCoords, 
								U32*  &indices);

	S32				getGeometryColors(LLStrider<LLVector3> &vertices,  
									  LLStrider<LLVector3> &normals,
									  LLStrider<LLVector2> &texCoords, 
									  LLStrider<LLColor4U> &colors, 
									  U32*  &indices);
	
	S32				getGeometryMultiTexture(LLStrider<LLVector3> &vertices,  
											LLStrider<LLVector3> &normals,
											LLStrider<LLVector3> &binormals,
											LLStrider<LLVector2> &texCoords0, 
											LLStrider<LLVector2> &texCoords1, 
											U32*  &indices);


	S32			getVertices	  (LLStrider<LLVector3> &vertices);
	S32			getColors	  (LLStrider<LLColor4U> &colors);
	S32			getIndices	  (U32*	 &indices);

	void		setSize(const S32 numVertices, const S32 num_indices = 0);
	BOOL		getDirty() const { return (mGeneration != mDrawPoolp->mGeneration); }

	BOOL		genVolumeTriangles(const LLVolume &volume, S32 f,
								   const LLMatrix4& mat, const LLMatrix3& inv_trans_mat, BOOL global_volume = FALSE);
	BOOL		genVolumeTriangles(const LLVolume &volume, S32 fstart, S32 fend,
								   const LLMatrix4& mat, const LLMatrix3& inv_trans_mat, BOOL global_volume = FALSE);
	BOOL 		genLighting(const LLVolume* volume, const LLDrawable* drawablep, S32 fstart, S32 fend, 
							const LLMatrix4& mat_vert, const LLMatrix3& mat_normal, BOOL do_lighting);

	BOOL 		genShadows(const LLVolume* volume, const LLDrawable* drawablep, S32 fstart, S32 fend, 
							const LLMatrix4& mat_vert, const LLMatrix3& mat_normal, BOOL use_shadow_factor);

	void		init(LLDrawable* drawablep, LLViewerObject* objp);
	void		destroy();
	void		update();

	void		updateCenterAgent(); // Update center when xform has changed.
	void renderSelectedUV(const S32 offset = 0, const S32 count = 0);

	void		renderForSelect() const;
	void		renderSelected(LLImageGL *image, const LLColor4 &color, const S32 offset = 0, const S32 count = 0);

	F32			getKey()					const	{ return mDistance; }

	S32			getGeneration() 			const	{ return mGeneration; }
	S32			getReferenceIndex() 		const	{ return mReferenceIndex; }
	void		setReferenceIndex(const S32 index)	{ mReferenceIndex = index; }

	BOOL		verify(const U32* indices_array = NULL) const;
	void		printDebugInfo() const;

	void		link(LLFace* facep);
	
protected:
	S32			allocBackupMem();	// Allocate backup memory based on the draw pool information.
	void		setDirty();

public:
	LLVector3		mCenterLocal;
	LLVector3		mCenterAgent;
	LLVector3		mExtents[2];
	LLVector2		mTexExtents[2];
	F32				mDistance;
	F32				mAlphaFade;
	LLFace*			mNextFace;
	BOOL			mSkipRender;
	
protected:
	S32			mGeneration;
	U32			mState;
	LLDrawPool*	mDrawPoolp;
	S32			mGeomIndex;			// index into draw pool
	LLColor4	mFaceColor;			// overrides material color if state |= USE_FACE_COLOR
	
	U32			mPrimType;
	S32			mGeomCount;			// vertex count for this face
	U32			mIndicesCount;
	S32			mIndicesIndex;		// index into draw pool for indices (yeah, I know!)
	LLXformMatrix* mXform;
	LLPointer<LLViewerImage> mTexture;

	U8				*mBackupMem;

	LLPointer<LLDrawable> mDrawablep;
	LLPointer<LLViewerObject> mVObjp;
	S32			mTEOffset;

	S32			mReferenceIndex;
	
protected:
	static BOOL	sSafeRenderSelect;
	
public:
	struct CompareDistanceGreater
	{
		bool operator()(const LLFace* const& lhs, const LLFace* const& rhs)
		{
			return lhs->mDistance > rhs->mDistance; // farthest = first
		}
	};
	
};

#endif // LL_LLFACE_H

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
#include "llvertexbuffer.h"
#include "llviewerimage.h"
#include "llpagemem.h"
#include "llstat.h"
#include "lldrawable.h"

class LLFacePool;
class LLVolume;
class LLViewerImage;
class LLTextureEntry;
class LLVertexProgram;
class LLViewerImage;
class LLGeometryManager;

class LLFace
{
public:

	enum EMasks
	{
		LIGHT			= 0x0001,
		GLOBAL			= 0x0002,
		FULLBRIGHT		= 0x0004,
		HUD_RENDER		= 0x0008,
		USE_FACE_COLOR	= 0x0010,
		TEXTURE_ANIM	= 0x0020, 
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
	void			setTexture(LLViewerImage* tex) { mTexture = tex; }
	LLXformMatrix*	getXform()			const	{ return mXform; }
	BOOL			hasGeometry()		const	{ return mGeomCount > 0; }
	LLVector3		getPositionAgent()	const;
	
	U32				getState()			const	{ return mState; }
	void			setState(U32 state)			{ mState |= state; }
	void			clearState(U32 state)		{ mState &= ~state; }
	BOOL			isState(U32 state)	const	{ return ((mState & state) != 0) ? TRUE : FALSE; }
	void			setVirtualSize(F32 size) { mVSize = size; }
	void			setPixelArea(F32 area)	{ mPixelArea = area; }
	F32				getVirtualSize() const { return mVSize; }
	F32				getPixelArea() const { return mPixelArea; }
	void			bindTexture(S32 stage = 0)		const	{ LLViewerImage::bindTexture(mTexture, stage); }

	void			enableLights() const;
	void			renderSetColor() const;
	S32				renderElements(const U32 *index_array) const;
	S32				renderIndexed ();
	S32				renderIndexed (U32 mask);
	S32				pushVertices(const U32* index_array) const;
	
	void			setWorldMatrix(const LLMatrix4& mat);
	const LLTextureEntry* getTextureEntry()	const { return mVObjp->getTE(mTEOffset); }

	LLFacePool*		getPool()			const	{ return mDrawPoolp; }
	U32				getPoolType()		const	{ return mPoolType; }
	LLDrawable*		getDrawable()		const	{ return mDrawablep; }
	LLViewerObject*	getViewerObject()	const	{ return mVObjp; }
	S32				getLOD()			const	{ return mVObjp.notNull() ? mVObjp->getLOD() : 0; }
	LLVertexBuffer* getVertexBuffer()	const	{ return mVertexBuffer; }
	void			setPoolType(U32 type)		{ mPoolType = type; }
	S32				getTEOffset()				{ return mTEOffset; }

	void			setViewerObject(LLViewerObject* object);
	void			setPool(LLFacePool *pool, LLViewerImage *texturep);
	
	void			setDrawable(LLDrawable *drawable);
	void			setTEOffset(const S32 te_offset);
	

	void			setFaceColor(const LLColor4& color); // override material color
	void			unsetFaceColor(); // switch back to material color
	const LLColor4&	getFaceColor() const { return mFaceColor; } 
	const LLColor4& getRenderColor() const;
	
	//for volumes
	S32 getGeometryVolume(const LLVolume& volume,
						S32 f,
						LLStrider<LLVector3>& vertices, 
						LLStrider<LLVector3>& normals,
						LLStrider<LLVector2>& texcoords,
						LLStrider<LLVector2>& texcoords2,
						LLStrider<LLColor4U>& colors,
						LLStrider<U32>& indices,
						const LLMatrix4& mat_vert, const LLMatrix3& mat_normal,
						U32& index_offset);

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
									   LLStrider<U32> &indices);

	// For volumes, etc.
	S32				getGeometry(LLStrider<LLVector3> &vertices,  
								LLStrider<LLVector3> &normals,
								LLStrider<LLVector2> &texCoords, 
								LLStrider<U32>  &indices);

	S32				getGeometryColors(LLStrider<LLVector3> &vertices,  
									  LLStrider<LLVector3> &normals,
									  LLStrider<LLVector2> &texCoords, 
									  LLStrider<LLColor4U> &colors, 
									  LLStrider<U32>  &indices);
	
	S32 getVertices(LLStrider<LLVector3> &vertices);
	S32 getColors(LLStrider<LLColor4U> &colors);
	S32 getIndices(LLStrider<U32> &indices);

	void		setSize(const S32 numVertices, const S32 num_indices = 0);
	
	BOOL		genVolumeBBoxes(const LLVolume &volume, S32 f,
								   const LLMatrix4& mat, const LLMatrix3& inv_trans_mat, BOOL global_volume = FALSE);
	
	void		init(LLDrawable* drawablep, LLViewerObject* objp);
	void		destroy();
	void		update();

	void		updateCenterAgent(); // Update center when xform has changed.
	void		renderSelectedUV(const S32 offset = 0, const S32 count = 0);

	void		renderForSelect(U32 data_mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD);
	void		renderSelected(LLImageGL *image, const LLColor4 &color, const S32 offset = 0, const S32 count = 0);

	F32			getKey()					const	{ return mDistance; }

	S32			getReferenceIndex() 		const	{ return mReferenceIndex; }
	void		setReferenceIndex(const S32 index)	{ mReferenceIndex = index; }

	BOOL		verify(const U32* indices_array = NULL) const;
	void		printDebugInfo() const;

	void		setGeomIndex(S32 idx) { mGeomIndex = idx; }
	void		setIndicesIndex(S32 idx) { mIndicesIndex = idx; }
	
protected:

public:
	LLVector3		mCenterLocal;
	LLVector3		mCenterAgent;
	LLVector3		mExtents[2];
	LLVector2		mTexExtents[2];
	F32				mDistance;
	F32				mAlphaFade;
	LLPointer<LLVertexBuffer> mVertexBuffer;
	LLPointer<LLVertexBuffer> mLastVertexBuffer;
	F32			mLastUpdateTime;

protected:
	friend class LLGeometryManager;
	friend class LLVolumeGeometryManager;

	U32			mState;
	LLFacePool*	mDrawPoolp;
	U32			mPoolType;
	LLColor4	mFaceColor;			// overrides material color if state |= USE_FACE_COLOR
	
	S32			mGeomCount;			// vertex count for this face
	S32			mGeomIndex;			// index into draw pool
	U32			mIndicesCount;
	S32			mIndicesIndex;		// index into draw pool for indices (yeah, I know!)

	//previous rebuild's geometry info
	S32			mLastGeomCount;
	S32			mLastGeomIndex;
	U32			mLastIndicesCount;
	S32			mLastIndicesIndex;

	LLXformMatrix* mXform;
	LLPointer<LLViewerImage> mTexture;
	LLPointer<LLDrawable> mDrawablep;
	LLPointer<LLViewerObject> mVObjp;
	S32			mTEOffset;

	S32			mReferenceIndex;
	F32			mVSize;
	F32			mPixelArea;
	
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
	
	struct CompareTexture
	{
		bool operator()(const LLFace* const& lhs, const LLFace* const& rhs)
		{
			return lhs->getTexture() < rhs->getTexture();
		}
	};

	struct CompareTextureAndGeomCount
	{
		bool operator()(const LLFace* const& lhs, const LLFace* const& rhs)
		{
			return lhs->getTexture() == rhs->getTexture() ? 
				lhs->getGeomCount() < rhs->getGeomCount() :
				lhs->getTexture() > rhs->getTexture();
		}
	};

	struct CompareTextureAndLOD
	{
		bool operator()(const LLFace* const& lhs, const LLFace* const& rhs)
		{
			return lhs->getTexture() == rhs->getTexture() ? 
				lhs->getLOD() < rhs->getLOD() :
				lhs->getTexture() < rhs->getTexture();
		}
	};

	struct CompareTextureAndTime
	{
		bool operator()(const LLFace* const& lhs, const LLFace* const& rhs)
		{
			return lhs->getTexture() == rhs->getTexture() ? 
				lhs->mLastUpdateTime < rhs->mLastUpdateTime :
				lhs->getTexture() < rhs->getTexture();
		}
	};
};

#endif // LL_LLFACE_H

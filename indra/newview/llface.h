/** 
 * @file llface.h
 * @brief LLFace class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLFACE_H
#define LL_LLFACE_H

#include "llstrider.h"

#include "llrender.h"
#include "v2math.h"
#include "v3math.h"
#include "v4math.h"
#include "m4math.h"
#include "v4coloru.h"
#include "llquaternion.h"
#include "xform.h"
#include "lldarrayptr.h"
#include "llvertexbuffer.h"
#include "llviewertexture.h"
#include "lldrawable.h"
#include "lltextureatlasmanager.h"

class LLFacePool;
class LLVolume;
class LLViewerTexture;
class LLTextureEntry;
class LLVertexProgram;
class LLViewerTexture;
class LLGeometryManager;
class LLTextureAtlasSlot;

const F32 MIN_ALPHA_SIZE = 1024.f;
const F32 MIN_TEX_ANIM_SIZE = 512.f;

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
	U32				getIndicesCount()	const	{ return mIndicesCount; };
	S32				getIndicesStart()	const	{ return mIndicesIndex; };
	U16				getGeomCount()		const	{ return mGeomCount; }		// vertex count for this face
	U16				getGeomIndex()		const	{ return mGeomIndex; }		// index into draw pool
	U16				getGeomStart()		const	{ return mGeomIndex; }		// index into draw pool
	void			setTexture(LLViewerTexture* tex) ;
	void            switchTexture(LLViewerTexture* new_texture);
	void            dirtyTexture();
	LLXformMatrix*	getXform()			const	{ return mXform; }
	BOOL			hasGeometry()		const	{ return mGeomCount > 0; }
	LLVector3		getPositionAgent()	const;
	LLVector2       surfaceToTexture(LLVector2 surface_coord, LLVector3 position, LLVector3 normal);
	
	U32				getState()			const	{ return mState; }
	void			setState(U32 state)			{ mState |= state; }
	void			clearState(U32 state)		{ mState &= ~state; }
	BOOL			isState(U32 state)	const	{ return ((mState & state) != 0) ? TRUE : FALSE; }
	void			setVirtualSize(F32 size) { mVSize = size; }
	void			setPixelArea(F32 area)	{ mPixelArea = area; }
	F32				getVirtualSize() const { return mVSize; }
	F32				getPixelArea() const { return mPixelArea; }

	S32             getIndexInTex() const {return mIndexInTex ;}
	void            setIndexInTex(S32 index) { mIndexInTex = index ;}

	void			renderSetColor() const;
	S32				renderElements(const U16 *index_array) const;
	S32				renderIndexed ();
	S32				renderIndexed (U32 mask);
	S32				pushVertices(const U16* index_array) const;
	
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
	LLViewerTexture*	getTexture() const;

	void			setViewerObject(LLViewerObject* object);
	void			setPool(LLFacePool *pool, LLViewerTexture *texturep);
	
	void			setDrawable(LLDrawable *drawable);
	void			setTEOffset(const S32 te_offset);
	

	void			setFaceColor(const LLColor4& color); // override material color
	void			unsetFaceColor(); // switch back to material color
	const LLColor4&	getFaceColor() const { return mFaceColor; } 
	const LLColor4& getRenderColor() const;

	//for volumes
	void updateRebuildFlags();
	bool canRenderAsMask(); // logic helper
	BOOL getGeometryVolume(const LLVolume& volume,
						const S32 &f,
						const LLMatrix4& mat_vert, const LLMatrix3& mat_normal,
						const U16 &index_offset);

	// For avatar
	U16			 getGeometryAvatar(
									LLStrider<LLVector3> &vertices,
									LLStrider<LLVector3> &normals,
								    LLStrider<LLVector2> &texCoords,
									LLStrider<F32>		 &vertex_weights,
									LLStrider<LLVector4> &clothing_weights);

	// For volumes, etc.
	U16				getGeometry(LLStrider<LLVector3> &vertices,  
								LLStrider<LLVector3> &normals,
								LLStrider<LLVector2> &texCoords, 
								LLStrider<U16>  &indices);

	S32 getColors(LLStrider<LLColor4U> &colors);
	S32 getIndices(LLStrider<U16> &indices);

	void		setSize(const S32 numVertices, const S32 num_indices = 0);
	
	BOOL		genVolumeBBoxes(const LLVolume &volume, S32 f,
								   const LLMatrix4& mat, const LLMatrix3& inv_trans_mat, BOOL global_volume = FALSE);
	
	void		init(LLDrawable* drawablep, LLViewerObject* objp);
	void		destroy();
	void		update();

	void		updateCenterAgent(); // Update center when xform has changed.
	void		renderSelectedUV();

	void		renderForSelect(U32 data_mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0);
	void		renderSelected(LLViewerTexture *image, const LLColor4 &color);

	F32			getKey()					const	{ return mDistance; }

	S32			getReferenceIndex() 		const	{ return mReferenceIndex; }
	void		setReferenceIndex(const S32 index)	{ mReferenceIndex = index; }

	BOOL		verify(const U32* indices_array = NULL) const;
	void		printDebugInfo() const;

	void		setGeomIndex(U16 idx) { mGeomIndex = idx; }
	void		setIndicesIndex(S32 idx) { mIndicesIndex = idx; }
	void		setDrawInfo(LLDrawInfo* draw_info);

	F32         getTextureVirtualSize() ;
	F32         getImportanceToCamera()const {return mImportanceToCamera ;}

	void        setHasMedia(bool has_media)  { mHasMedia = has_media ;}
	BOOL        hasMedia() const ;

	//for atlas
	LLTextureAtlasSlot*   getAtlasInfo() ;
	void                  setAtlasInUse(BOOL flag);
	void                  setAtlasInfo(LLTextureAtlasSlot* atlasp);
	BOOL                  isAtlasInUse()const;
	BOOL                  canUseAtlas() const;
	const LLVector2*      getTexCoordScale() const ;
	const LLVector2*      getTexCoordOffset()const;
	const LLTextureAtlas* getAtlas()const ;
	void                  removeAtlas() ;
	BOOL                  switchTexture() ;

private:	
	F32         adjustPartialOverlapPixelArea(F32 cos_angle_to_view_dir, F32 radius );
	BOOL        calcPixelArea(F32& cos_angle_to_view_dir, F32& radius) ;
public:
	static F32  calcImportanceToCamera(F32 to_view_dir, F32 dist);

public:
	
	LLVector3		mCenterLocal;
	LLVector3		mCenterAgent;
	LLVector3		mExtents[2];
	LLVector2		mTexExtents[2];
	F32				mDistance;
	LLPointer<LLVertexBuffer> mVertexBuffer;
	LLPointer<LLVertexBuffer> mLastVertexBuffer;
	F32			mLastUpdateTime;
	F32			mLastMoveTime;
	LLMatrix4*	mTextureMatrix;
	LLDrawInfo* mDrawInfo;

private:
	friend class LLGeometryManager;
	friend class LLVolumeGeometryManager;

	U32			mState;
	LLFacePool*	mDrawPoolp;
	U32			mPoolType;
	LLColor4	mFaceColor;			// overrides material color if state |= USE_FACE_COLOR
	
	U16			mGeomCount;			// vertex count for this face
	U16			mGeomIndex;			// index into draw pool
	U32			mIndicesCount;
	U32			mIndicesIndex;		// index into draw pool for indices (yeah, I know!)
	S32         mIndexInTex ;

	//previous rebuild's geometry info
	U16			mLastGeomCount;
	U16			mLastGeomIndex;
	U32			mLastIndicesCount;
	U32			mLastIndicesIndex;

	LLXformMatrix* mXform;
	LLPointer<LLViewerTexture> mTexture;
	LLPointer<LLDrawable> mDrawablep;
	LLPointer<LLViewerObject> mVObjp;
	S32			mTEOffset;

	S32			mReferenceIndex;
	F32			mVSize;
	F32			mPixelArea;

	//importance factor, in the range [0, 1.0].
	//1.0: the most important.
	//based on the distance from the face to the view point and the angle from the face center to the view direction.
	F32         mImportanceToCamera ; 
	F32         mBoundingSphereRadius ;
	bool        mHasMedia ;

	//atlas
	LLPointer<LLTextureAtlasSlot> mAtlasInfop ;
	BOOL                          mUsingAtlas ;
	
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

	struct CompareBatchBreaker
	{
		bool operator()(const LLFace* const& lhs, const LLFace* const& rhs)
		{
			const LLTextureEntry* lte = lhs->getTextureEntry();
			const LLTextureEntry* rte = rhs->getTextureEntry();

			if(lhs->getTexture() != rhs->getTexture())
			{
				return lhs->getTexture() < rhs->getTexture();
			}
			else if (lte->getBumpShinyFullbright() != rte->getBumpShinyFullbright())
			{
				return lte->getBumpShinyFullbright() < rte->getBumpShinyFullbright();
			}
			else 
			{
				return lte->getGlow() < rte->getGlow();
			}
		}
	};

	struct CompareTextureAndGeomCount
	{
		bool operator()(const LLFace* const& lhs, const LLFace* const& rhs)
		{
			return lhs->getTexture() == rhs->getTexture() ? 
				lhs->getGeomCount() < rhs->getGeomCount() :  //smallest = first
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

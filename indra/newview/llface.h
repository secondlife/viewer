/** 
 * @file llface.h
 * @brief LLFace class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "llvertexbuffer.h"
#include "llviewertexture.h"
#include "lldrawable.h"

class LLFacePool;
class LLVolume;
class LLViewerTexture;
class LLTextureEntry;
class LLVertexProgram;
class LLViewerTexture;
class LLGeometryManager;
class LLDrawInfo;
class LLMeshSkinInfo;

const F32 MIN_ALPHA_SIZE = 1024.f;
const F32 MIN_TEX_ANIM_SIZE = 512.f;
const U8 FACE_DO_NOT_BATCH_TEXTURES = 255;

class alignas(16) LLFace
{
    LL_ALIGN_NEW
public:
	LLFace(const LLFace& rhs)
	{
		*this = rhs;
	}

	const LLFace& operator=(const LLFace& rhs)
	{
		LL_ERRS() << "Illegal operation!" << LL_ENDL;
		return *this;
	}

	enum EMasks
	{
		LIGHT			= 0x0001,
		GLOBAL			= 0x0002,
		FULLBRIGHT		= 0x0004,
		HUD_RENDER		= 0x0008,
		USE_FACE_COLOR	= 0x0010,
		TEXTURE_ANIM	= 0x0020, 
		RIGGED			= 0x0040,
		PARTICLE		= 0x0080,
	};

public:
	LLFace(LLDrawable* drawablep, LLViewerObject* objp)
	{
        LL_PROFILE_ZONE_SCOPED;
		init(drawablep, objp);
	}
	~LLFace()  { destroy(); }

	const LLMatrix4& getWorldMatrix()	const	{ return mVObjp->getWorldMatrix(mXform); }
	const LLMatrix4& getRenderMatrix() const;
	U32				getIndicesCount()	const	{ return mIndicesCount; };
	S32				getIndicesStart()	const	{ return mIndicesIndex; };
	U16				getGeomCount()		const	{ return mGeomCount; }		// vertex count for this face
	U16				getGeomIndex()		const	{ return mGeomIndex; }		// index into draw pool
	U16				getGeomStart()		const	{ return mGeomIndex; }		// index into draw pool
	void			setTextureIndex(U8 index);
	U8				getTextureIndex() const		{ return mTextureIndex; }
	void			setTexture(U32 ch, LLViewerTexture* tex);
	void			setTexture(LLViewerTexture* tex) ;
	void			setDiffuseMap(LLViewerTexture* tex);
	void			setNormalMap(LLViewerTexture* tex);
	void			setSpecularMap(LLViewerTexture* tex);
    void			setAlternateDiffuseMap(LLViewerTexture* tex);
	void            switchTexture(U32 ch, LLViewerTexture* new_texture);
	void            dirtyTexture();
	LLXformMatrix*	getXform()			const	{ return mXform; }
	bool			hasGeometry()		const	{ return mGeomCount > 0; }
	LLVector3		getPositionAgent()	const;
	LLVector2       surfaceToTexture(LLVector2 surface_coord, const LLVector4a& position, const LLVector4a& normal);
	void 			getPlanarProjectedParams(LLQuaternion* face_rot, LLVector3* face_pos, F32* scale) const;
	bool			calcAlignedPlanarTE(const LLFace* align_to, LLVector2* st_offset,
										LLVector2* st_scale, F32* st_rot, LLRender::eTexIndex map = LLRender::DIFFUSE_MAP) const;
	
	U32				getState()			const	{ return mState; }
	void			setState(U32 state)			{ mState |= state; }
	void			clearState(U32 state)		{ mState &= ~state; }
	bool			isState(U32 state)	const	{ return ((mState & state) != 0) ? true : false; }
	void			setVirtualSize(F32 size) { mVSize = size; }
	void			setPixelArea(F32 area)	{ mPixelArea = area; }
	F32				getVirtualSize() const { return mVSize; }
	F32				getPixelArea() const { return mPixelArea; }

	S32             getIndexInTex(U32 ch) const {llassert(ch < LLRender::NUM_TEXTURE_CHANNELS); return mIndexInTex[ch];}
	void            setIndexInTex(U32 ch, S32 index) { llassert(ch < LLRender::NUM_TEXTURE_CHANNELS);  mIndexInTex[ch] = index ;}
	
	void			setWorldMatrix(const LLMatrix4& mat);
	const LLTextureEntry* getTextureEntry()	const { return mVObjp->getTE(mTEOffset); }

	LLFacePool*		getPool()			const	{ return mDrawPoolp; }
	U32				getPoolType()		const	{ return mPoolType; }
	LLDrawable*		getDrawable()		const	{ return mDrawablep; }
	LLViewerObject*	getViewerObject()	const	{ return mVObjp; }
	S32				getLOD()			const	{ return mVObjp.notNull() ? mVObjp->getLOD() : 0; }
	void			setPoolType(U32 type)		{ mPoolType = type; }
	S32				getTEOffset()       const   { return mTEOffset; }
	LLViewerTexture*	getTexture(U32 ch = LLRender::DIFFUSE_MAP) const;

	void			setViewerObject(LLViewerObject* object);
	void			setPool(LLFacePool *pool, LLViewerTexture *texturep);
	void			setPool(LLFacePool* pool);

	void			setDrawable(LLDrawable *drawable);
	void			setTEOffset(const S32 te_offset);
	
    void            renderIndexed();

	void			setFaceColor(const LLColor4& color); // override material color
	void			unsetFaceColor(); // switch back to material color
	const LLColor4&	getFaceColor() const { return mFaceColor; } 
	

	//for volumes
	void updateRebuildFlags();
	bool canRenderAsMask(); // logic helper
	bool getGeometryVolume(const LLVolume& volume,
                            S32 face_index,
                            const LLMatrix4& mat_vert,
                            const LLMatrix3& mat_normal,
                            U16 index_offset,
                            bool force_rebuild = false,
                            bool no_debug_assert = false);

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

	void		setSize(S32 numVertices, S32 num_indices = 0, bool align = false);
	
	bool		genVolumeBBoxes(const LLVolume &volume, S32 f,
									const LLMatrix4& mat_vert_in, bool global_volume = false);
	
	void		init(LLDrawable* drawablep, LLViewerObject* objp);
	void		destroy();
	void		update();

	void		updateCenterAgent(); // Update center when xform has changed.
	void		renderSelectedUV();

	void		renderSelected(LLViewerTexture *image, const LLColor4 &color);
	void		renderOneWireframe(const LLColor4 &color, F32 fogCfx, bool wireframe_selection, bool bRenderHiddenSelections, bool shader);

	F32			getKey()					const	{ return mDistance; }

	S32			getReferenceIndex() 		const	{ return mReferenceIndex; }
	void		setReferenceIndex(const S32 index)	{ mReferenceIndex = index; }

	bool		verify(const U32* indices_array = NULL) const;
	void		printDebugInfo() const;

	void		setGeomIndex(U16 idx); 
	void		setIndicesIndex(S32 idx);
	void		setDrawInfo(LLDrawInfo* draw_info);

	F32         getTextureVirtualSize() ;
	F32         getImportanceToCamera()const {return mImportanceToCamera ;}
	void        resetVirtualSize();

	void        setHasMedia(bool has_media)  { mHasMedia = has_media ;}
	bool        hasMedia() const ;

    void        setMediaAllowed(bool is_media_allowed)  { mIsMediaAllowed = is_media_allowed; }
    bool        isMediaAllowed() const { return mIsMediaAllowed; }

	bool		switchTexture() ;

	//vertex buffer tracking
	void setVertexBuffer(LLVertexBuffer* buffer);
	void clearVertexBuffer(); //sets mVertexBuffer to NULL
	LLVertexBuffer* getVertexBuffer()	const	{ return mVertexBuffer; }
	S32 getRiggedIndex(U32 type) const;

    // used to preserve draw order of faces that are batched together. 
    // Allows content creators to manipulate linked sets and face ordering 
    // for consistent alpha sorting results, particularly for rigged attachments
    void setDrawOrderIndex(U32 index) { mDrawOrderIndex = index; }
    U32 getDrawOrderIndex() const { return mDrawOrderIndex; }

    // return true if this face is in an alpha draw pool
    bool isInAlphaPool() const;
public: //aligned members
	LLVector4a		mExtents[2];

private:
    friend class LLViewerTextureList;
	F32         adjustPartialOverlapPixelArea(F32 cos_angle_to_view_dir, F32 radius );
	bool        calcPixelArea(F32& cos_angle_to_view_dir, F32& radius) ;
public:
	static F32 calcImportanceToCamera(F32 to_view_dir, F32 dist);
	static F32 adjustPixelArea(F32 importance, F32 pixel_area) ;

public:
	
	LLVector3		mCenterLocal;
	LLVector3		mCenterAgent;
	
	LLVector2		mTexExtents[2];
	F32				mDistance;
	F32			mLastUpdateTime;
	F32			mLastSkinTime;
	F32			mLastMoveTime;
	LLMatrix4*	mTextureMatrix;
	LLMatrix4*	mSpecMapMatrix;
	LLMatrix4*	mNormalMapMatrix;
	LLDrawInfo* mDrawInfo;
    LLVOAvatar* mAvatar = nullptr;
    LLMeshSkinInfo* mSkinInfo = nullptr;
    
    // return mSkinInfo->mHash or 0 if mSkinInfo is null
    U64 getSkinHash();

private:
	LLPointer<LLVertexBuffer> mVertexBuffer;
		
	U32			mState;
	LLFacePool*	mDrawPoolp;
	U32			mPoolType;
	LLColor4	mFaceColor;			// overrides material color if state |= USE_FACE_COLOR
	
	U16			mGeomCount;			// vertex count for this face
	U16			mGeomIndex;			// starting index into mVertexBuffer's vertex array
	U8			mTextureIndex;		// index of texture channel to use for pseudo-atlasing
	U32			mIndicesCount;
	U32			mIndicesIndex;		// index into mVertexBuffer's index array
	S32         mIndexInTex[LLRender::NUM_TEXTURE_CHANNELS];

	LLXformMatrix* mXform;

	LLPointer<LLViewerTexture> mTexture[LLRender::NUM_TEXTURE_CHANNELS];

	// mDrawablep is not supposed to be null, don't use LLPointer because
	// mDrawablep owns LLFace and LLPointer is a good way to either cause a
	// memory leak or a 'delete each other' situation if something deletes
	// drawable wrongly.
	LLDrawable* mDrawablep;
	// LLViewerObject technically owns drawable, but also it should be strictly managed
	LLPointer<LLViewerObject> mVObjp;
	S32			mTEOffset;

	S32			mReferenceIndex;
	std::vector<S32> mRiggedIndex;
	
	F32			mVSize;
	F32			mPixelArea;

	//importance factor, in the range [0, 1.0].
	//1.0: the most important.
	//based on the distance from the face to the view point and the angle from the face center to the view direction.
	F32         mImportanceToCamera ; 
	F32         mBoundingSphereRadius ;
	bool        mHasMedia ;
	bool        mIsMediaAllowed;

    U32 mDrawOrderIndex = 0; // see setDrawOrderIndex
	
protected:
	static bool	sSafeRenderSelect;
	
public:
	struct CompareDistanceGreater
	{
		bool operator()(const LLFace* const& lhs, const LLFace* const& rhs)
		{
			return !lhs || (rhs && (lhs->mDistance > rhs->mDistance)); // farthest = first
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
			else 
			{
				return lte->getBumpShinyFullbright() < rte->getBumpShinyFullbright();
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

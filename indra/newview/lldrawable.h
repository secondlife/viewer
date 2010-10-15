/** 
 * @file lldrawable.h
 * @brief LLDrawable class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_DRAWABLE_H
#define LL_DRAWABLE_H

#include <vector>
#include <map>

#include "v2math.h"
#include "v3math.h"
#include "v4math.h"
#include "m4math.h"
#include "v4coloru.h"
#include "llquaternion.h"
#include "xform.h"
#include "llmemtype.h"
#include "lldarray.h"
#include "llviewerobject.h"
#include "llrect.h"
#include "llappviewer.h" // for gFrameTimeSeconds

class LLCamera;
class LLDrawPool;
class LLDrawable;
class LLFace;
class LLFacePool;
class LLSpatialGroup;
class LLSpatialBridge;
class LLSpatialPartition;
class LLVOVolume;
class LLViewerTexture;

// Can have multiple silhouettes for each object
const U32 SILHOUETTE_HIGHLIGHT = 0;

// All data for new renderer goes into this class.
class LLDrawable : public LLRefCount
{
public:
	static void initClass();

	LLDrawable()				{ init(); }
	MEM_TYPE_NEW(LLMemType::MTYPE_DRAWABLE);
	
	void markDead();			// Mark this drawable as dead
	BOOL isDead() const			{ return isState(DEAD); }
	BOOL isNew() const			{ return !isState(BUILT); }

	BOOL isLight() const;

	BOOL isVisible() const;	
	BOOL isRecentlyVisible() const;	
	virtual void setVisible(LLCamera& camera_in, std::vector<LLDrawable*>* results = NULL, BOOL for_select = FALSE);


	LLViewerRegion* getRegion()               const { return mVObjp->getRegion(); }
	const LLTextureEntry* getTextureEntry(U8 which) const { return mVObjp->getTE(which); }
	LLPointer<LLViewerObject>& getVObj()							  { return mVObjp; }
	const LLViewerObject *getVObj()	const						  { return mVObjp; }
	LLVOVolume*	getVOVolume() const; // cast mVObjp tp LLVOVolume if OK

	const LLMatrix4&      getWorldMatrix() const		{ return mXform.getWorldMatrix(); }
	const LLMatrix4&	  getRenderMatrix() const		{ return isRoot() ? getWorldMatrix() : getParent()->getWorldMatrix(); }
	void				  setPosition(LLVector3 v) const { }
	const LLVector3&	  getPosition() const			{ return mXform.getPosition(); }
	const LLVector3&      getWorldPosition() const		{ return mXform.getPositionW(); }
	const LLVector3		  getPositionAgent() const;
	const LLVector3d&	  getPositionGroup() const		{ return mPositionGroup; }
	const LLVector3&	  getScale() const				{ return mCurrentScale; }
	void				  setScale(const LLVector3& scale) { mCurrentScale = scale; }
	const LLQuaternion&   getWorldRotation() const		{ return mXform.getWorldRotation(); }
	const LLQuaternion&   getRotation() const			{ return mXform.getRotation(); }
	F32			          getIntensity() const			{ return llmin(mXform.getScale().mV[0], 4.f); }
	S32					  getLOD() const				{ return mVObjp ? mVObjp->getLOD() : 1; }
	F64					  getBinRadius() const			{ return mBinRadius; }
	void  getMinMax(LLVector3& min,LLVector3& max) const { mXform.getMinMax(min,max); }
	LLXformMatrix*		getXform() { return &mXform; }

	U32					getState()           const { return mState; }
	BOOL				isState   (U32 bits) const { return ((mState & bits) != 0); }
	void                setState  (U32 bits)       { mState |= bits; }
	void                clearState(U32 bits)       { mState &= ~bits; }

	BOOL				isAvatar()	const			{ return mVObjp.notNull() && mVObjp->isAvatar(); }
	BOOL				isRoot() const				{ return !mParent || mParent->isAvatar(); }
	BOOL				isSpatialRoot() const		{ return !mParent || mParent->isAvatar(); }
	virtual BOOL		isSpatialBridge() const		{ return FALSE; }
	virtual LLSpatialPartition* asPartition()		{ return NULL; }
	LLDrawable*			getParent() const			{ return mParent; }
	
	// must set parent through LLViewerObject::		()
	//BOOL                setParent(LLDrawable *parent);
	
	inline LLFace*      getFace(const S32 i) const;
	inline S32			getNumFaces()      	 const;

	//void                removeFace(const S32 i); // SJB: Avoid using this, it's slow
	LLFace*				addFace(LLFacePool *poolp, LLViewerTexture *texturep);
	LLFace*				addFace(const LLTextureEntry *te, LLViewerTexture *texturep);
	void				deleteFaces(S32 offset, S32 count);
	void                setNumFaces(const S32 numFaces, LLFacePool *poolp, LLViewerTexture *texturep);
	void                setNumFacesFast(const S32 numFaces, LLFacePool *poolp, LLViewerTexture *texturep);
	void				mergeFaces(LLDrawable* src);

	void init();
	void destroy();

	void update();
	F32 updateXform(BOOL undamped);

	virtual void makeActive();
	/*virtual*/ void makeStatic(BOOL warning_enabled = TRUE);

	BOOL isActive()	const							{ return isState(ACTIVE); }
	BOOL isStatic() const							{ return !isActive(); }
	BOOL isAnimating() const;

	virtual BOOL updateMove();
	virtual void movePartition();
	
	void updateTexture();
	void updateMaterial();
	virtual void updateDistance(LLCamera& camera, bool force_update);
	BOOL updateGeometry(BOOL priority);
	void updateFaceSize(S32 idx);
		
	void updateSpecialHoverCursor(BOOL enabled);

	virtual void shiftPos(const LLVector3 &shift_vector);

	S32 getGeneration() const					{ return mGeneration; }

	BOOL getLit() const							{ return isState(UNLIT) ? FALSE : TRUE; }
	void setLit(BOOL lit)						{ lit ? clearState(UNLIT) : setState(UNLIT); }

	virtual void cleanupReferences();

	void setRadius(const F32 radius);
	F32 getRadius() const						{ return mRadius; }
	F32 getVisibilityRadius() const;

	void updateUVMinMax();	// Updates the cache of sun space bounding box.

	const LLVector3& getBounds(LLVector3& min, LLVector3& max) const;
	virtual void updateSpatialExtents();
	virtual void updateBinRadius();
	const LLVector3* getSpatialExtents() const;
	void setSpatialExtents(LLVector3 min, LLVector3 max);
	void setPositionGroup(const LLVector3d& pos);
	void setPositionGroup(const LLVector3& pos) { setPositionGroup(LLVector3d(pos)); }

	void setRenderType(S32 type) 				{ mRenderType = type; }
	BOOL isRenderType(S32 type) 				{ return mRenderType == type; }
	S32  getRenderType()						{ return mRenderType; }
	
	// Debugging methods
	S32 findReferences(LLDrawable *drawablep); // Not const because of @#$! iterators...

	void setSpatialGroup(LLSpatialGroup *groupp);
	LLSpatialGroup *getSpatialGroup() const			{ return mSpatialGroupp; }
	LLSpatialPartition* getSpatialPartition();
	
	// Statics
	static void incrementVisible();
	static void cleanupDeadDrawables();

protected:
	~LLDrawable() { destroy(); }
	void moveUpdatePipeline(BOOL moved);
	void updatePartition();
	BOOL updateMoveDamped();
	BOOL updateMoveUndamped();
	
public:
	friend class LLPipeline;
	friend class LLDrawPool;
	friend class LLSpatialBridge;
	
	typedef std::set<LLPointer<LLDrawable> > drawable_set_t;
	typedef std::vector<LLPointer<LLDrawable> > drawable_vector_t;
	typedef std::list<LLPointer<LLDrawable> > drawable_list_t;
	typedef std::queue<LLPointer<LLDrawable> > drawable_queue_t;
	
	struct CompareDistanceGreater
	{
		bool operator()(const LLDrawable* const& lhs, const LLDrawable* const& rhs)
		{
			return lhs->mDistanceWRTCamera < rhs->mDistanceWRTCamera; // farthest = last
		}
	};

	struct CompareDistanceGreaterVisibleFirst
	{
		bool operator()(const LLDrawable* const& lhs, const LLDrawable* const& rhs)
		{
			if (lhs->isVisible() && !rhs->isVisible())
			{
				return TRUE; //visible things come first
			}
			else if (!lhs->isVisible() && rhs->isVisible())
			{
				return FALSE; //rhs is visible, comes first
			}
			
			return lhs->mDistanceWRTCamera < rhs->mDistanceWRTCamera; // farthest = last
		}
	};
	
	typedef enum e_drawable_flags
	{
 		IN_REBUILD_Q1	= 0x00000002,
 		IN_REBUILD_Q2	= 0x00000004,
 		IN_LIGHT_Q		= 0x00000008,
		EARLY_MOVE		= 0x00000010,
		MOVE_UNDAMPED	= 0x00000020,
		ON_MOVE_LIST	= 0x00000040,
		USE_BACKLIGHT	= 0x00000080,
		UV				= 0x00000100,
		UNLIT			= 0x00000200,
		LIGHT			= 0x00000400,
		LIGHTING_BUILT	= 0x00000800,
		REBUILD_VOLUME  = 0x00001000,	//volume changed LOD or parameters, or vertex buffer changed
		REBUILD_TCOORD	= 0x00002000,	//texture coordinates changed
		REBUILD_COLOR	= 0x00004000,	//color changed
		REBUILD_POSITION= 0x00010000,	//vertex positions/normals changed
		REBUILD_GEOMETRY= REBUILD_POSITION|REBUILD_TCOORD|REBUILD_COLOR,
		REBUILD_MATERIAL= REBUILD_TCOORD|REBUILD_COLOR,
		REBUILD_ALL		= REBUILD_GEOMETRY|REBUILD_VOLUME,
		ON_SHIFT_LIST	= 0x00100000,
		BLOCKER			= 0x00400000,
		ACTIVE			= 0x00800000,
		DEAD			= 0x01000000,
		INVISIBLE		= 0x02000000, // stay invisible until flag is cleared
 		NEARBY_LIGHT	= 0x04000000, // In gPipeline.mNearbyLightSet
		BUILT			= 0x08000000,
		FORCE_INVISIBLE = 0x10000000, // stay invis until CLEAR_INVISIBLE is set (set of orphaned)
		CLEAR_INVISIBLE = 0x20000000, // clear FORCE_INVISIBLE next draw frame
		REBUILD_SHADOW =  0x40000000,
		HAS_ALPHA		= 0x80000000,
	} EDrawableFlags;

	LLXformMatrix       mXform;

	// vis data
	LLPointer<LLDrawable> mParent;

	F32				mDistanceWRTCamera;
	
	S32				mQuietCount;

	static S32 getCurrentFrame() { return sCurVisible; }
	static S32 getMinVisFrameRange();

	void setSpatialBridge(LLSpatialBridge* bridge) { mSpatialBridge = (LLDrawable*) bridge; }
	LLSpatialBridge* getSpatialBridge() { return (LLSpatialBridge*) (LLDrawable*) mSpatialBridge; }
	
	static F32 sCurPixelAngle; //current pixels per radian

private:
	typedef std::vector<LLFace*> face_list_t;
	
	U32				mState;
	S32				mRenderType;
	LLPointer<LLViewerObject> mVObjp;
	face_list_t     mFaces;
	LLSpatialGroup* mSpatialGroupp;
	LLPointer<LLDrawable> mSpatialBridge;
	
	mutable U32		mVisible;
	F32				mRadius;
	LLVector3		mExtents[2];
	LLVector3d		mPositionGroup;
	F64				mBinRadius;
	S32				mGeneration;
	
	LLVector3		mCurrentScale;
	
	static U32 sCurVisible; // Counter for what value of mVisible means currently visible

	static U32 sNumZombieDrawables;
	static LLDynamicArrayPtr<LLPointer<LLDrawable> > sDeadList;
};


inline LLFace* LLDrawable::getFace(const S32 i) const
{
	//switch these asserts to llerrs -- davep
	//llassert((U32)i < mFaces.size());
	//llassert(mFaces[i]);

	if ((U32) i >= mFaces.size())
	{
		llerrs << "Invalid face index." << llendl;
	}

	if (!mFaces[i])
	{
		llerrs << "Null face found." << llendl;
	}
	
	return mFaces[i];
}


inline S32 LLDrawable::getNumFaces()const
{
	return (S32)mFaces.size();
}

#endif

/** 
 * @file lldrawable.cpp
 * @brief LLDrawable class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "lldrawable.h"

// library includes
#include "material_codes.h"

// viewer includes
#include "llcriticaldamp.h"
#include "llface.h"
#include "lllightconstants.h"
#include "llsky.h"
#include "llsurfacepatch.h"
#include "llviewercamera.h"
#include "llviewerregion.h"
#include "llvolume.h"
#include "llvoavatar.h"
#include "llvovolume.h"
#include "llvosurfacepatch.h" // for debugging
#include "llworld.h"
#include "pipeline.h"
#include "llspatialpartition.h"
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"

const F32 MIN_INTERPOLATE_DISTANCE_SQUARED = 0.001f * 0.001f;
const F32 MAX_INTERPOLATE_DISTANCE_SQUARED = 10.f * 10.f;
const F32 OBJECT_DAMPING_TIME_CONSTANT = 0.06f;
const F32 MIN_SHADOW_CASTER_RADIUS = 2.0f;

static LLFastTimer::DeclareTimer FTM_CULL_REBOUND("Cull Rebound");


////////////////////////
//
// Inline implementations.
//
//



//////////////////////////////
//
// Drawable code
//
//

// static
U32 LLDrawable::sCurVisible = 0;
U32 LLDrawable::sNumZombieDrawables = 0;
F32 LLDrawable::sCurPixelAngle = 0;
LLDynamicArrayPtr<LLPointer<LLDrawable> > LLDrawable::sDeadList;

#define FORCE_INVISIBLE_AREA 16.f

// static
void LLDrawable::incrementVisible() 
{
	sCurVisible++;
	sCurPixelAngle = (F32) gViewerWindow->getWindowHeightRaw()/LLViewerCamera::getInstance()->getView();
}
void LLDrawable::init()
{
	// mXform
	mParent = NULL;
	mRenderType = 0;
	mCurrentScale = LLVector3(1,1,1);
	mDistanceWRTCamera = 0.0f;

	mQuietCount = 0;

	mState     = 0;
	mVObjp   = NULL;
	// mFaces
	mSpatialGroupp = NULL;
	mVisible = sCurVisible - 2;//invisible for the current frame and the last frame.
	mRadius = 0.f;
	
	mGeneration = -1;
	mBinRadius = 1.f;
	mSpatialBridge = NULL;
}

// static
void LLDrawable::initClass()
{
}


void LLDrawable::destroy()
{
	if (isDead())
	{
		sNumZombieDrawables--;
	}

	if (LLSpatialGroup::sNoDelete)
	{
		llerrs << "Illegal deletion of LLDrawable!" << llendl;
	}

	std::for_each(mFaces.begin(), mFaces.end(), DeletePointer());
	mFaces.clear();
		
	
	/*if (!(sNumZombieDrawables % 10))
	{
		llinfos << "- Zombie drawables: " << sNumZombieDrawables << llendl;
	}*/	
}

void LLDrawable::markDead()
{
	if (isDead())
	{
		llwarns << "Warning!  Marking dead multiple times!" << llendl;
		return;
	}

	if (mSpatialBridge)
	{
		mSpatialBridge->markDead();
		mSpatialBridge = NULL;
	}

	sNumZombieDrawables++;

	// We're dead.  Free up all of our references to other objects
	setState(DEAD);
	cleanupReferences();
//	sDeadList.put(this);
}

LLVOVolume* LLDrawable::getVOVolume() const
{
	LLViewerObject* objectp = mVObjp;
	if ( !isDead() && objectp && (objectp->getPCode() == LL_PCODE_VOLUME))
	{
		return ((LLVOVolume*)objectp);
	}
	else
	{
		return NULL;
	}
}

BOOL LLDrawable::isLight() const
{
	LLViewerObject* objectp = mVObjp;
	if ( objectp && (objectp->getPCode() == LL_PCODE_VOLUME) && !isDead())
	{
		return ((LLVOVolume*)objectp)->getIsLight();
	}
	else
	{
		return FALSE;
	}
}

void LLDrawable::cleanupReferences()
{
	LLFastTimer t(FTM_PIPELINE);
	
	std::for_each(mFaces.begin(), mFaces.end(), DeletePointer());
	mFaces.clear();

	gObjectList.removeDrawable(this);
	
	gPipeline.unlinkDrawable(this);
	
	// Cleanup references to other objects
	mVObjp = NULL;
	mParent = NULL;
}

void LLDrawable::cleanupDeadDrawables()
{
	/*
	S32 i;
	for (i = 0; i < sDeadList.count(); i++)
	{
		if (sDeadList[i]->getNumRefs() > 1)
		{
			llwarns << "Dead drawable has " << sDeadList[i]->getNumRefs() << " remaining refs" << llendl;
			gPipeline.findReferences(sDeadList[i]);
		}
	}
	*/
	sDeadList.reset();
}

S32 LLDrawable::findReferences(LLDrawable *drawablep)
{
	S32 count = 0;
	if (mParent == drawablep)
	{
		llinfos << this << ": parent reference" << llendl;
		count++;
	}
	return count;
}

LLFace*	LLDrawable::addFace(LLFacePool *poolp, LLViewerTexture *texturep)
{
	LLMemType mt(LLMemType::MTYPE_DRAWABLE);
	
	LLFace *face = new LLFace(this, mVObjp);
	if (!face) llerrs << "Allocating new Face: " << mFaces.size() << llendl;
	
	if (face)
	{
		mFaces.push_back(face);

		if (poolp)
		{
			face->setPool(poolp, texturep);
		}

		if (isState(UNLIT))
		{
			face->setState(LLFace::FULLBRIGHT);
		}
	}
	return face;
}

LLFace*	LLDrawable::addFace(const LLTextureEntry *te, LLViewerTexture *texturep)
{
	LLMemType mt(LLMemType::MTYPE_DRAWABLE);
	
	LLFace *face;
	face = new LLFace(this, mVObjp);

	face->setTEOffset(mFaces.size());
	face->setTexture(texturep);
	face->setPoolType(gPipeline.getPoolTypeFromTE(te, texturep));

	mFaces.push_back(face);

	if (isState(UNLIT))
	{
		face->setState(LLFace::FULLBRIGHT);
	}

	return face;

}

void LLDrawable::setNumFaces(const S32 newFaces, LLFacePool *poolp, LLViewerTexture *texturep)
{
	if (newFaces == (S32)mFaces.size())
	{
		return;
	}
	else if (newFaces < (S32)mFaces.size())
	{
		std::for_each(mFaces.begin() + newFaces, mFaces.end(), DeletePointer());
		mFaces.erase(mFaces.begin() + newFaces, mFaces.end());
	}
	else // (newFaces > mFaces.size())
	{
		mFaces.reserve(newFaces);
		for (int i = mFaces.size(); i<newFaces; i++)
		{
			addFace(poolp, texturep);
		}
	}

	llassert_always(mFaces.size() == newFaces);
}

void LLDrawable::setNumFacesFast(const S32 newFaces, LLFacePool *poolp, LLViewerTexture *texturep)
{
	if (newFaces <= (S32)mFaces.size() && newFaces >= (S32)mFaces.size()/2)
	{
		return;
	}
	else if (newFaces < (S32)mFaces.size())
	{
		std::for_each(mFaces.begin() + newFaces, mFaces.end(), DeletePointer());
		mFaces.erase(mFaces.begin() + newFaces, mFaces.end());
	}
	else // (newFaces > mFaces.size())
	{
		mFaces.reserve(newFaces);
		for (int i = mFaces.size(); i<newFaces; i++)
		{
			addFace(poolp, texturep);
		}
	}

	llassert_always(mFaces.size() == newFaces) ;
}

void LLDrawable::mergeFaces(LLDrawable* src)
{
	U32 face_count = mFaces.size() + src->mFaces.size();

	mFaces.reserve(face_count);
	for (U32 i = 0; i < src->mFaces.size(); i++)
	{
		LLFace* facep = src->mFaces[i];
		facep->setDrawable(this);
		mFaces.push_back(facep);
	}
	src->mFaces.clear();
}

void LLDrawable::deleteFaces(S32 offset, S32 count)
{
	face_list_t::iterator face_begin = mFaces.begin() + offset;
	face_list_t::iterator face_end = face_begin + count;

	std::for_each(face_begin, face_end, DeletePointer());
	mFaces.erase(face_begin, face_end);
}

void LLDrawable::update()
{
	llerrs << "Shouldn't be called!" << llendl;
}


void LLDrawable::updateMaterial()
{
}

void LLDrawable::makeActive()
{		
#if !LL_RELEASE_FOR_DOWNLOAD
	if (mVObjp.notNull())
	{
		U32 pcode = mVObjp->getPCode();
		if (pcode == LLViewerObject::LL_VO_WATER ||
			pcode == LLViewerObject::LL_VO_SURFACE_PATCH ||
			pcode == LLViewerObject::LL_VO_PART_GROUP ||
			pcode == LLViewerObject::LL_VO_HUD_PART_GROUP ||
			pcode == LLViewerObject::LL_VO_CLOUDS ||
			pcode == LLViewerObject::LL_VO_GROUND ||
			pcode == LLViewerObject::LL_VO_SKY)
		{
			llerrs << "Static viewer object has active drawable!" << llendl;
		}
	}
#endif

	if (!isState(ACTIVE)) // && mGeneration > 0)
	{
		setState(ACTIVE);
		
		//parent must be made active first
		if (!isRoot() && !mParent->isActive())
		{
			mParent->makeActive();
		}

		//all child objects must also be active
		llassert_always(mVObjp);
		
		LLViewerObject::const_child_list_t& child_list = mVObjp->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			 iter != child_list.end(); iter++)
		{
			LLViewerObject* child = *iter;
			LLDrawable* drawable = child->mDrawable;
			if (drawable)
			{
				drawable->makeActive();
			}
		}

		if (mVObjp->getPCode() == LL_PCODE_VOLUME)
		{
			if (mVObjp->isFlexible())
			{
				return;
			}
		}
	
		if (mVObjp->getPCode() == LL_PCODE_VOLUME)
		{
			gPipeline.markRebuild(this, LLDrawable::REBUILD_VOLUME, TRUE);
		}
		updatePartition();
	}

	if (isRoot())
	{
		mQuietCount = 0;
	}
	else
	{
		getParent()->mQuietCount = 0;
	}
}


void LLDrawable::makeStatic(BOOL warning_enabled)
{
	if (isState(ACTIVE))
	{
		clearState(ACTIVE);

		if (mParent.notNull() && mParent->isActive() && warning_enabled)
		{
			LL_WARNS_ONCE("Drawable") << "Drawable becomes static with active parent!" << LL_ENDL;
		}

		LLViewerObject::const_child_list_t& child_list = mVObjp->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			 iter != child_list.end(); iter++)
		{
			LLViewerObject* child = *iter;
			LLDrawable* child_drawable = child->mDrawable;
			if (child_drawable)
			{
				if (child_drawable->getParent() != this)
				{
					llwarns << "Child drawable has unknown parent." << llendl;
				}
				child_drawable->makeStatic(warning_enabled);
			}
		}
		
		if (mVObjp->getPCode() == LL_PCODE_VOLUME)
		{
			gPipeline.markRebuild(this, LLDrawable::REBUILD_VOLUME, TRUE);
		}		
		
		if (mSpatialBridge)
		{
			mSpatialBridge->markDead();
			setSpatialBridge(NULL);
		}
	}
	updatePartition();
}

// Returns "distance" between target destination and resulting xfrom
F32 LLDrawable::updateXform(BOOL undamped)
{
	BOOL damped = !undamped;

	// Position
	LLVector3 old_pos(mXform.getPosition());
	LLVector3 target_pos;
	if (mXform.isRoot())
	{
		// get root position in your agent's region
		target_pos = mVObjp->getPositionAgent();
	}
	else
	{
		// parent-relative position
		target_pos = mVObjp->getPosition();
	}
	
	// Rotation
	LLQuaternion old_rot(mXform.getRotation());
	LLQuaternion target_rot = mVObjp->getRotation();
	//scaling
	LLVector3 target_scale = mVObjp->getScale();
	LLVector3 old_scale = mCurrentScale;
	LLVector3 dest_scale = target_scale;
	
	// Damping
	F32 dist_squared = 0.f;
	F32 camdist2 = (mDistanceWRTCamera * mDistanceWRTCamera);

	if (damped && isVisible())
	{
		F32 lerp_amt = llclamp(LLCriticalDamp::getInterpolant(OBJECT_DAMPING_TIME_CONSTANT), 0.f, 1.f);
		LLVector3 new_pos = lerp(old_pos, target_pos, lerp_amt);
		dist_squared = dist_vec_squared(new_pos, target_pos);

		LLQuaternion new_rot = nlerp(lerp_amt, old_rot, target_rot);
		dist_squared += (1.f - dot(new_rot, target_rot)) * 10.f;

		LLVector3 new_scale = lerp(old_scale, target_scale, lerp_amt);
		dist_squared += dist_vec_squared(new_scale, target_scale);

		if ((dist_squared >= MIN_INTERPOLATE_DISTANCE_SQUARED * camdist2) &&
			(dist_squared <= MAX_INTERPOLATE_DISTANCE_SQUARED))
		{
			// interpolate
			target_pos = new_pos;
			target_rot = new_rot;
			target_scale = new_scale;
		}
		else
		{
			// snap to final position
			dist_squared = 0.0f;
			if (getVOVolume() && !isRoot())
			{ //child prim snapping to some position, needs a rebuild
				gPipeline.markRebuild(this, LLDrawable::REBUILD_POSITION, TRUE);
			}
		}
	}

	if ((mCurrentScale != target_scale) ||
		(!isRoot() && 
		 (dist_squared >= MIN_INTERPOLATE_DISTANCE_SQUARED || 
		 !mVObjp->getAngularVelocity().isExactlyZero() ||
		 target_pos != mXform.getPosition() ||
		 target_rot != mXform.getRotation())))
	{ //child prim moving or scale change requires immediate rebuild
		gPipeline.markRebuild(this, LLDrawable::REBUILD_POSITION, TRUE);
	}
	else if (!getVOVolume() && !isAvatar())
	{
		movePartition();
	}

	// Update
	mXform.setPosition(target_pos);
	mXform.setRotation(target_rot);
	mXform.setScale(LLVector3(1,1,1)); //no scale in drawable transforms (IT'S A RULE!)
	mXform.updateMatrix();
	
	mCurrentScale = target_scale;
	
	if (mSpatialBridge)
	{
		gPipeline.markMoved(mSpatialBridge, FALSE);
	}
	return dist_squared;
}

void LLDrawable::setRadius(F32 radius)
{
	if (mRadius != radius)
	{
		mRadius = radius;
	}
}

void LLDrawable::moveUpdatePipeline(BOOL moved)
{
	makeActive();
	
	// Update the face centers.
	for (S32 i = 0; i < getNumFaces(); i++)
	{
		getFace(i)->updateCenterAgent();
	}
}

void LLDrawable::movePartition()
{
	LLSpatialPartition* part = getSpatialPartition();
	if (part)
	{
		part->move(this, getSpatialGroup());
	}
}

BOOL LLDrawable::updateMove()
{
	if (isDead())
	{
		llwarns << "Update move on dead drawable!" << llendl;
		return TRUE;
	}
	
	if (mVObjp.isNull())
	{
		return FALSE;
	}
	
	makeActive();
	
	BOOL done;

	if (isState(MOVE_UNDAMPED))
	{
		done = updateMoveUndamped();
	}
	else
	{
		done = updateMoveDamped();
	}
	return done;
}

BOOL LLDrawable::updateMoveUndamped()
{
	F32 dist_squared = updateXform(TRUE);

	mGeneration++;

	if (!isState(LLDrawable::INVISIBLE))
	{
		BOOL moved = (dist_squared > 0.001f && dist_squared < 255.99f);	
		moveUpdatePipeline(moved);
		mVObjp->updateText();
	}

	mVObjp->clearChanged(LLXform::MOVED);
	
	return TRUE;
}

void LLDrawable::updatePartition()
{
	if (!getVOVolume())
	{
		movePartition();
	}
	else if (mSpatialBridge)
	{
		gPipeline.markMoved(mSpatialBridge, FALSE);
	}
	else
	{
		//a child prim moved and needs its verts regenerated
		gPipeline.markRebuild(this, LLDrawable::REBUILD_POSITION, TRUE);
	}
}

BOOL LLDrawable::updateMoveDamped()
{
	F32 dist_squared = updateXform(FALSE);

	mGeneration++;

	if (!isState(LLDrawable::INVISIBLE))
	{
		BOOL moved = (dist_squared > 0.001f && dist_squared < 128.0f);
		moveUpdatePipeline(moved);
		mVObjp->updateText();
	}

	BOOL done_moving = (dist_squared == 0.0f) ? TRUE : FALSE;

	if (done_moving)
	{
		mVObjp->clearChanged(LLXform::MOVED);
	}
	
	return done_moving;
}

void LLDrawable::updateDistance(LLCamera& camera, bool force_update)
{
	if (LLViewerCamera::sCurCameraID != LLViewerCamera::CAMERA_WORLD)
	{
		llerrs << "WTF?" << llendl;
	}

	//switch LOD with the spatial group to avoid artifacts
	//LLSpatialGroup* sg = getSpatialGroup();

	LLVector3 pos;

	//if (!sg || sg->changeLOD())
	{
		LLVOVolume* volume = getVOVolume();
		if (volume)
		{
			volume->updateRelativeXform();
			pos = volume->getRelativeXform().getTranslation();
			if (isStatic())
			{
				pos += volume->getRegion()->getOriginAgent();
			}

			if (isState(LLDrawable::HAS_ALPHA))
			{
				for (S32 i = 0; i < getNumFaces(); i++)
				{
					LLFace* facep = getFace(i);
					if (force_update || facep->getPoolType() == LLDrawPool::POOL_ALPHA)
					{
						LLVector3 box = (facep->mExtents[1] - facep->mExtents[0]) * 0.25f;
						LLVector3 v = (facep->mCenterLocal-camera.getOrigin());
						const LLVector3& at = camera.getAtAxis();
						for (U32 j = 0; j < 3; j++)
						{
							v.mV[j] -= box.mV[j] * at.mV[j];
						}
						facep->mDistance = v * camera.getAtAxis();
					}
				}
			}
		}
		else
		{
			pos = LLVector3(getPositionGroup());
		}

		pos -= camera.getOrigin();	
		mDistanceWRTCamera = llround(pos.magVec(), 0.01f);
		mVObjp->updateLOD();
	}
}

void LLDrawable::updateTexture()
{
	LLMemType mt(LLMemType::MTYPE_DRAWABLE);
	
	if (isDead())
	{
		llwarns << "Dead drawable updating texture!" << llendl;
		return;
	}
	
	if (getNumFaces() != mVObjp->getNumTEs())
	{ //drawable is transitioning its face count
		return;
	}

	if (getVOVolume())
	{
		if (isActive())
		{
			if (isRoot())
			{
				mQuietCount = 0;
			}
			else
			{
				getParent()->mQuietCount = 0;
			}
		}
				
		gPipeline.markRebuild(this, LLDrawable::REBUILD_MATERIAL, TRUE);
	}
}

BOOL LLDrawable::updateGeometry(BOOL priority)
{
	llassert(mVObjp.notNull());
	BOOL res = mVObjp->updateGeometry(this);
	return res;
}

void LLDrawable::shiftPos(const LLVector3 &shift_vector)
{
	if (isDead())
	{
		llwarns << "Shifting dead drawable" << llendl;
		return;
	}

	if (mParent)
	{
		mXform.setPosition(mVObjp->getPosition());
	}
	else
	{
		mXform.setPosition(mVObjp->getPositionAgent());
	}

	mXform.setRotation(mVObjp->getRotation());
	mXform.setScale(1,1,1);
	mXform.updateMatrix();

	if (isStatic())
	{
		LLVOVolume* volume = getVOVolume();
		if (!volume)
		{
			gPipeline.markRebuild(this, LLDrawable::REBUILD_ALL, TRUE);
		}

		for (S32 i = 0; i < getNumFaces(); i++)
		{
			LLFace *facep = getFace(i);
			facep->mCenterAgent += shift_vector;
			facep->mExtents[0] += shift_vector;
			facep->mExtents[1] += shift_vector;
			
			if (!volume && facep->hasGeometry())
			{
				facep->mVertexBuffer = NULL;
				facep->mLastVertexBuffer = NULL;
			}
		}
		
		mExtents[0] += shift_vector;
		mExtents[1] += shift_vector;
		mPositionGroup += LLVector3d(shift_vector);
	}
	else if (mSpatialBridge)
	{
		mSpatialBridge->shiftPos(shift_vector);
	}
	else if (isAvatar())
	{
		mExtents[0] += shift_vector;
		mExtents[1] += shift_vector;
		mPositionGroup += LLVector3d(shift_vector);
	}
	
	mVObjp->onShift(shift_vector);
}

const LLVector3& LLDrawable::getBounds(LLVector3& min, LLVector3& max) const
{
	mXform.getMinMax(min,max);
	return mXform.getPositionW();
}

const LLVector3* LLDrawable::getSpatialExtents() const
{
	return mExtents;
}

void LLDrawable::setSpatialExtents(LLVector3 min, LLVector3 max)
{ 
	LLVector3 size = max - min;
	mExtents[0] = min; 
	mExtents[1] = max; 
}

void LLDrawable::setPositionGroup(const LLVector3d& pos)
{
	mPositionGroup.setVec(pos);
}

void LLDrawable::updateSpatialExtents()
{
	if (mVObjp)
	{
		mVObjp->updateSpatialExtents(mExtents[0], mExtents[1]);
	}
	
	updateBinRadius();
	
	if (mSpatialBridge.notNull())
	{
		mPositionGroup.setVec(0,0,0);
	}
}


void LLDrawable::updateBinRadius()
{
	if (mVObjp.notNull())
	{
		mBinRadius = llmin(mVObjp->getBinRadius(), 256.f);
	}
	else
	{
		mBinRadius = llmin(getRadius()*4.f, 256.f);
	}
}

void LLDrawable::updateSpecialHoverCursor(BOOL enabled)
{
	// TODO: maintain a list of objects that have special
	// hover cursors, then use that list for per-frame
	// hover cursor selection. JC
}

F32 LLDrawable::getVisibilityRadius() const
{
	if (isDead())
	{
		return 0.f;
	}
	else if (isLight())
	{
		const LLVOVolume *vov = getVOVolume();
		if (vov)
		{
			return llmax(getRadius(), vov->getLightRadius());
		} else {
			// llwarns ?
		}
	}
	return getRadius();
}

void LLDrawable::updateUVMinMax()
{
}

void LLDrawable::setSpatialGroup(LLSpatialGroup *groupp)
{
/*if (mSpatialGroupp && (groupp != mSpatialGroupp))
	{
		mSpatialGroupp->setState(LLSpatialGroup::GEOM_DIRTY);
	}*/
	mSpatialGroupp = groupp;
}

LLSpatialPartition* LLDrawable::getSpatialPartition()
{ 
	LLSpatialPartition* retval = NULL;
	
	if (!mVObjp || 
		!getVOVolume() ||
		isStatic())
	{
		retval = gPipeline.getSpatialPartition((LLViewerObject*) mVObjp);
	}
	else if (isRoot())
	{	//must be an active volume
		if (!mSpatialBridge)
		{
			if (mVObjp->isHUDAttachment())
			{
				setSpatialBridge(new LLHUDBridge(this));
			}
			else
			{
				setSpatialBridge(new LLVolumeBridge(this));
			}
		}
		return mSpatialBridge->asPartition();
	}
	else 
	{
		retval = getParent()->getSpatialPartition();
	}
	
	if (retval && mSpatialBridge.notNull())
	{
		mSpatialBridge->markDead();
		setSpatialBridge(NULL);
	}
	
	return retval;
}

const S32 MIN_VIS_FRAME_RANGE = 2 ; //two frames:the current one and the last one.
//static 
S32 LLDrawable::getMinVisFrameRange()
{
	return MIN_VIS_FRAME_RANGE ;
}

BOOL LLDrawable::isRecentlyVisible() const
{
	//currently visible or visible in the previous frame.
	BOOL vis = isVisible() || (sCurVisible - mVisible < MIN_VIS_FRAME_RANGE)  ;

	if(!vis)
	{
		LLSpatialGroup* group = getSpatialGroup();
		if (group && group->isRecentlyVisible())
		{
			mVisible = sCurVisible;
			vis = TRUE ;
		}
	}

	return vis ;
}

BOOL LLDrawable::isVisible() const
{
	if (mVisible == sCurVisible)
	{
		return TRUE;
	}
	
#if 0
	//disabling this code fixes DEV-20105.  Leaving in place in case some other bug pops up as a a result.
	//should be safe to just always ask the spatial group for visibility.
	if (isActive())
	{
		if (isRoot())
		{
			LLSpatialGroup* group = mSpatialBridge.notNull() ? mSpatialBridge->getSpatialGroup() :
									getSpatialGroup();
			if (group && group->isVisible())
			{
				mVisible = sCurVisible;
				return TRUE;
			}
		}
		else
		{
			if (getParent()->isVisible())
			{
				mVisible = sCurVisible;
				return TRUE;
			}
		}
	}
	else
#endif
	{
		LLSpatialGroup* group = getSpatialGroup();
		if (group && group->isVisible())
		{
			mVisible = sCurVisible;
			return TRUE;
		}
	}

	return FALSE;
}

//=======================================
// Spatial Partition Bridging Drawable
//=======================================

LLSpatialBridge::LLSpatialBridge(LLDrawable* root, BOOL render_by_group, U32 data_mask)
: LLSpatialPartition(data_mask, render_by_group, FALSE)
{
	mDrawable = root;
	root->setSpatialBridge(this);
	
	mRenderType = mDrawable->mRenderType;
	mDrawableType = mDrawable->mRenderType;
	
	mPartitionType = LLViewerRegion::PARTITION_VOLUME;
	
	mOctree->balance();

	llassert(mDrawable);
	llassert(mDrawable->getRegion());
	LLSpatialPartition *part = mDrawable->getRegion()->getSpatialPartition(mPartitionType);
	llassert(part);
	
	if (part)
	{
		part->put(this);
	}
}

LLSpatialBridge::~LLSpatialBridge()
{	
	LLSpatialGroup* group = getSpatialGroup();
	if (group)
	{
		group->mSpatialPartition->remove(this, group);
	}
}

void LLSpatialBridge::updateSpatialExtents()
{
	LLSpatialGroup* root = (LLSpatialGroup*) mOctree->getListener(0);
	
	{
		LLFastTimer ftm(FTM_CULL_REBOUND);
		root->rebound();
	}
	
	LLXformMatrix* mat = mDrawable->getXform();
	
	LLVector3 offset = root->mBounds[0];
	LLVector3 size = root->mBounds[1];
		
	LLVector3 center = LLVector3(0,0,0) * mat->getWorldMatrix();
	LLQuaternion rotation = LLQuaternion(mat->getWorldMatrix());
	
	offset *= rotation;
	center += offset;
	
	LLVector3 v[4];
	//get 4 corners of bounding box
	v[0] = (size * rotation);
	v[1] = (LLVector3(-size.mV[0], -size.mV[1], size.mV[2]) * rotation);
	v[2] = (LLVector3(size.mV[0], -size.mV[1], -size.mV[2]) * rotation);
	v[3] = (LLVector3(-size.mV[0], size.mV[1], -size.mV[2]) * rotation);

	LLVector3& newMin = mExtents[0];
	LLVector3& newMax = mExtents[1];
	
	newMin = newMax = center;
	
	for (U32 i = 0; i < 4; i++)
	{
		for (U32 j = 0; j < 3; j++)
		{
			F32 delta = fabsf(v[i].mV[j]);
			F32 min = center.mV[j] - delta;
			F32 max = center.mV[j] + delta;
			
			if (min < newMin.mV[j])
			{
				newMin.mV[j] = min;
			}
			
			if (max > newMax.mV[j])
			{
				newMax.mV[j] = max;
			}
		}
	}

	LLVector3 diagonal = newMax - newMin;
	mRadius = diagonal.magVec() * 0.5f;
	
	mPositionGroup.setVec((newMin + newMax) * 0.5f);
	updateBinRadius();
}

void LLSpatialBridge::updateBinRadius()
{
	mBinRadius = llmin((F32) mOctree->getSize().mdV[0]*0.5f, 256.f);
}

LLCamera LLSpatialBridge::transformCamera(LLCamera& camera)
{
	LLCamera ret = camera;
	LLXformMatrix* mat = mDrawable->getXform();
	LLVector3 center = LLVector3(0,0,0) * mat->getWorldMatrix();
	LLQuaternion rotation = LLQuaternion(mat->getWorldMatrix());

	LLVector3 delta = ret.getOrigin() - center;
	LLQuaternion rot = ~mat->getRotation();

	delta *= rot;
	LLVector3 lookAt = ret.getAtAxis();
	LLVector3 up_axis = ret.getUpAxis();
	LLVector3 left_axis = ret.getLeftAxis();

	lookAt *= rot;
	up_axis *= rot;
	left_axis *= rot;

	if (!delta.isFinite())
	{
		delta.clearVec();
	}

	ret.setOrigin(delta);
	ret.setAxes(lookAt, left_axis, up_axis);
		
	return ret;
}

void LLDrawable::setVisible(LLCamera& camera, std::vector<LLDrawable*>* results, BOOL for_select)
{
	mVisible = sCurVisible;
	
#if 0 && !LL_RELEASE_FOR_DOWNLOAD
	//crazy paranoid rules checking
	if (getVOVolume())
	{
		if (!isRoot())
		{
			if (isActive() && !mParent->isActive())
			{
				llerrs << "Active drawable has static parent!" << llendl;
			}
			
			if (isStatic() && !mParent->isStatic())
			{
				llerrs << "Static drawable has active parent!" << llendl;
			}
			
			if (mSpatialBridge)
			{
				llerrs << "Child drawable has spatial bridge!" << llendl;
			}
		}
		else if (isActive() && !mSpatialBridge)
		{
			llerrs << "Active root drawable has no spatial bridge!" << llendl;
		}
		else if (isStatic() && mSpatialBridge.notNull())
		{
			llerrs << "Static drawable has spatial bridge!" << llendl;
		}
	}
#endif
}

class LLOctreeMarkNotCulled: public LLOctreeTraveler<LLDrawable>
{
public:
	LLCamera* mCamera;
	
	LLOctreeMarkNotCulled(LLCamera* camera_in) : mCamera(camera_in) { }
	
	virtual void traverse(const LLOctreeNode<LLDrawable>* node)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) node->getListener(0);
		group->setVisible();
		LLOctreeTraveler<LLDrawable>::traverse(node);
	}
	
	void visit(const LLOctreeNode<LLDrawable>* branch)
	{
		gPipeline.markNotCulled((LLSpatialGroup*) branch->getListener(0), *mCamera);
	}
};

void LLSpatialBridge::setVisible(LLCamera& camera_in, std::vector<LLDrawable*>* results, BOOL for_select)
{
	if (!gPipeline.hasRenderType(mDrawableType))
	{
		return;
	}


	//HACK don't draw attachments for avatars that haven't been visible in more than a frame
	LLViewerObject *vobj = mDrawable->getVObj();
	if (vobj && vobj->isAttachment() && !vobj->isHUDAttachment())
	{
		LLDrawable* av;
		LLDrawable* parent = mDrawable->getParent();

		if (parent)
		{
			LLViewerObject* objparent = parent->getVObj();
			av = objparent->mDrawable;
			LLSpatialGroup* group = av->getSpatialGroup();

			BOOL impostor = FALSE;
			BOOL loaded = FALSE;
			if (objparent->isAvatar())
			{
				LLVOAvatar* avatarp = (LLVOAvatar*) objparent;
				if (avatarp->isVisible())
				{
					impostor = objparent->isAvatar() && ((LLVOAvatar*) objparent)->isImpostor();
					loaded   = objparent->isAvatar() && ((LLVOAvatar*) objparent)->isFullyLoaded();
				}
				else
				{
					return;
				}
			}

			if (!group ||
				LLDrawable::getCurrentFrame() - av->mVisible > 1 ||
				impostor ||
				!loaded)
			{
				return;
			}
		}
	}
	

	LLSpatialGroup* group = (LLSpatialGroup*) mOctree->getListener(0);
	group->rebound();
	
	LLVector3 center = (mExtents[0] + mExtents[1]) * 0.5f;
	LLVector3 size = (mExtents[1]-mExtents[0]) * 0.5f;

	if ((LLPipeline::sShadowRender && camera_in.AABBInFrustum(center, size)) ||
		LLPipeline::sImpostorRender ||
		(camera_in.AABBInFrustumNoFarClip(center, size) && 
		AABBSphereIntersect(mExtents[0], mExtents[1], camera_in.getOrigin(), camera_in.mFrustumCornerDist)))
	{
		if (!LLPipeline::sImpostorRender &&
			!LLPipeline::sShadowRender && 
			LLPipeline::calcPixelArea(center, size, camera_in) < FORCE_INVISIBLE_AREA)
		{
			return;
		}

		LLDrawable::setVisible(camera_in);
		
		if (for_select)
		{
			results->push_back(mDrawable);
			if (mDrawable->getVObj())
			{
				LLViewerObject::const_child_list_t& child_list = mDrawable->getVObj()->getChildren();
				for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
					 iter != child_list.end(); iter++)
				{
					LLViewerObject* child = *iter;
					LLDrawable* drawable = child->mDrawable;					
					results->push_back(drawable);
				}
			}
		}
		else 
		{
			LLCamera trans_camera = transformCamera(camera_in);
			LLOctreeMarkNotCulled culler(&trans_camera);
			culler.traverse(mOctree);
		}		
	}
}

void LLSpatialBridge::updateDistance(LLCamera& camera_in, bool force_update)
{
	if (mDrawable == NULL)
	{
		markDead();
		return;
	}

	if (mDrawable->getVObj())
	{
		if (mDrawable->getVObj()->isAttachment())
		{
			LLDrawable* parent = mDrawable->getParent();
			if (parent && parent->getVObj())
			{
				LLVOAvatar* av = parent->getVObj()->asAvatar();
				if (av && av->isImpostor())
				{
					return;
				}
			}
		}

		LLCamera camera = transformCamera(camera_in);
	
		mDrawable->updateDistance(camera, force_update);
	
		LLViewerObject::const_child_list_t& child_list = mDrawable->getVObj()->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			 iter != child_list.end(); iter++)
		{
			LLViewerObject* child = *iter;
			LLDrawable* drawable = child->mDrawable;					
			if (!drawable)
			{
				continue;
			}

			if (!drawable->isAvatar())
			{
				drawable->updateDistance(camera, force_update);
			}
		}
	}
}

void LLSpatialBridge::makeActive()
{ //it is an error to make a spatial bridge active (it's already active)
	llerrs << "makeActive called on spatial bridge" << llendl;
}

void LLSpatialBridge::move(LLDrawable *drawablep, LLSpatialGroup *curp, BOOL immediate)
{
	LLSpatialPartition::move(drawablep, curp, immediate);
	gPipeline.markMoved(this, FALSE);
}

BOOL LLSpatialBridge::updateMove()
{
	llassert(mDrawable);
	llassert(mDrawable->getRegion());
	LLSpatialPartition* part = mDrawable->getRegion()->getSpatialPartition(mPartitionType);
	llassert(part);

	mOctree->balance();
	if (part)
	{
		part->move(this, getSpatialGroup(), TRUE);
	}
	return TRUE;
}

void LLSpatialBridge::shiftPos(const LLVector3& vec)
{
	mExtents[0] += vec;
	mExtents[1] += vec;
	mPositionGroup += LLVector3d(vec);
}

void LLSpatialBridge::cleanupReferences()
{	
	LLDrawable::cleanupReferences();
	if (mDrawable)
	{
		mDrawable->setSpatialGroup(NULL);
		if (mDrawable->getVObj())
		{
			LLViewerObject::const_child_list_t& child_list = mDrawable->getVObj()->getChildren();
			for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
				 iter != child_list.end(); iter++)
			{
				LLViewerObject* child = *iter;
				LLDrawable* drawable = child->mDrawable;					
				if (drawable)
				{
					drawable->setSpatialGroup(NULL);
				}
			}
		}

		LLDrawable* drawablep = mDrawable;
		mDrawable = NULL;
		drawablep->setSpatialBridge(NULL);
	}
}

const LLVector3	LLDrawable::getPositionAgent() const
{
	if (getVOVolume())
	{
		if (isActive())
		{
			LLVector3 pos(0,0,0);
			if (!isRoot())
			{
				pos = mVObjp->getPosition();
			}
			return pos * getRenderMatrix();
		}
		else
		{
			return mVObjp->getPositionAgent();
		}
	}
	else
	{
		return getWorldPosition();
	}
}

BOOL LLDrawable::isAnimating() const
{
	if (!getVObj())
	{
		return TRUE;
	}

	if (getScale() != mVObjp->getScale())
	{
		return TRUE;
	}

	if (mVObjp->getPCode() == LLViewerObject::LL_VO_PART_GROUP)
	{
		return TRUE;
	}
	if (mVObjp->getPCode() == LLViewerObject::LL_VO_HUD_PART_GROUP)
	{
		return TRUE;
	}
	if (mVObjp->getPCode() == LLViewerObject::LL_VO_CLOUDS)
	{
		return TRUE;
	}

	if (!isRoot() && !mVObjp->getAngularVelocity().isExactlyZero())
	{
		return TRUE;
	}

	return FALSE;
}

void LLDrawable::updateFaceSize(S32 idx)
{
	if (mVObjp.notNull())
	{
		mVObjp->updateFaceSize(idx);
	}
}

LLBridgePartition::LLBridgePartition()
: LLSpatialPartition(0, FALSE, 0) 
{ 
	mDrawableType = LLPipeline::RENDER_TYPE_AVATAR; 
	mPartitionType = LLViewerRegion::PARTITION_BRIDGE;
	mLODPeriod = 16;
	mSlopRatio = 0.25f;
}

LLHUDBridge::LLHUDBridge(LLDrawable* drawablep)
: LLVolumeBridge(drawablep)
{
	mDrawableType = LLPipeline::RENDER_TYPE_HUD;
	mPartitionType = LLViewerRegion::PARTITION_HUD;
	mSlopRatio = 0.0f;
}

F32 LLHUDBridge::calcPixelArea(LLSpatialGroup* group, LLCamera& camera)
{
	return 1024.f;
}


void LLHUDBridge::shiftPos(const LLVector3& vec)
{
	//don't shift hud bridges on region crossing
}


/** 
 * @file llspatialpartition.cpp
 * @brief LLSpatialGroup class implementation and supporting functions
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llspatialpartition.h"

#include "llviewerwindow.h"
#include "llviewerobjectlist.h"
#include "llvovolume.h"
#include "llviewercamera.h"
#include "llface.h"
#include "llviewercontrol.h"
#include "llagent.h"
#include "llviewerregion.h"
#include "llcamera.h"
#include "pipeline.h"

static GLuint sBoxList = 0;

const F32 SG_OCCLUSION_FUDGE = 1.01f;
//const S32 SG_LOD_PERIOD = 16;

#define SG_DISCARD_TOLERANCE 0.25f

#if LL_OCTREE_PARANOIA_CHECK
#define assert_octree_valid(x) x->validate()
#else
#define assert_octree_valid(x)
#endif

static U32 sZombieGroups = 0;

static F32 sLastMaxTexPriority = 1.f;
static F32 sCurMaxTexPriority = 1.f;

//static counter for frame to switch LOD on

void sg_assert(BOOL expr)
{
#if LL_OCTREE_PARANOIA_CHECK
	if (!expr)
	{
		llerrs << "Octree invalid!" << llendl;
	}
#endif
}

#if LL_DEBUG
void validate_drawable(LLDrawable* drawablep)
{
	F64 rad = drawablep->getBinRadius();
	const LLVector3* ext = drawablep->getSpatialExtents();

	if (rad < 0 || rad > 4096 ||
		(ext[1]-ext[0]).magVec() > 4096)
	{
		llwarns << "Invalid drawable found in octree." << llendl;
	}
}
#else
#define validate_drawable(x)
#endif

BOOL earlyFail(LLCamera* camera, LLSpatialGroup* group);

BOOL LLLineSegmentAABB(const LLVector3& start, const LLVector3& end, const LLVector3& center, const LLVector3& size)
{
	float fAWdU[3];
	LLVector3 dir;
	LLVector3 diff;
	
	for (U32 i = 0; i < 3; i++)
	{
		dir.mV[i] = 0.5f * (end.mV[i] - start.mV[i]);
		diff.mV[i] = (0.5f * (end.mV[i] + start.mV[i])) - center.mV[i];
		fAWdU[i] = fabsf(dir.mV[i]);
		if(fabsf(diff.mV[i])>size.mV[i] + fAWdU[i])	return false;
	}

	float f;
	f = dir.mV[1] * diff.mV[2] - dir.mV[2] * diff.mV[1];	if(fabsf(f)>size.mV[1]*fAWdU[2] + size.mV[2]*fAWdU[1])	return false;
	f = dir.mV[2] * diff.mV[0] - dir.mV[0] * diff.mV[2];	if(fabsf(f)>size.mV[0]*fAWdU[2] + size.mV[2]*fAWdU[0])	return false;
	f = dir.mV[0] * diff.mV[1] - dir.mV[1] * diff.mV[0];	if(fabsf(f)>size.mV[0]*fAWdU[1] + size.mV[1]*fAWdU[0])	return false;

	return true;
}

//returns:
//	0 if sphere and AABB are not intersecting 
//	1 if they are
//	2 if AABB is entirely inside sphere

S32 LLSphereAABB(const LLVector3& center, const LLVector3& size, const LLVector3& pos, const F32 &rad)
{
	S32 ret = 2;

	LLVector3 min = center - size;
	LLVector3 max = center + size;
	for (U32 i = 0; i < 3; i++)
	{
		if (min.mV[i] > pos.mV[i] + rad ||
			max.mV[i] < pos.mV[i] - rad)
		{	//totally outside
			return 0;
		}
		
		if (min.mV[i] < pos.mV[i] - rad ||
			max.mV[i] > pos.mV[i] + rad)
		{	//intersecting
			ret = 1;
		}
	}

	return ret;
}

LLSpatialGroup::~LLSpatialGroup()
{
	if (isState(DEAD))
	{
		sZombieGroups--;
	}
	
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	clearDrawMap();
}

void LLSpatialGroup::clearDrawMap()
{
	mDrawMap.clear();
}

class LLRelightPainter : public LLSpatialGroup::OctreeTraveler
{
public:
	LLVector3 mOrigin, mDir;
	F32 mRadius;

	LLRelightPainter(LLVector3 origin, LLVector3 dir, F32 radius)
		: mOrigin(origin), mDir(dir), mRadius(radius) 
	{ }

	virtual void traverse(const LLSpatialGroup::TreeNode* n)
	{
		LLSpatialGroup::OctreeNode* node = (LLSpatialGroup::OctreeNode*) n;
		
		LLSpatialGroup* group = (LLSpatialGroup*) node->getListener(0);
		group->setState(LLSpatialGroup::RESHADOW);
		
		for (U32 i = 0; i < node->getChildCount(); i++)
		{
			const LLSpatialGroup::OctreeNode* child = node->getChild(i);
			LLSpatialGroup* group = (LLSpatialGroup*) child->getListener(0);
			
			LLVector3 res;
			
			LLVector3 center, size;
			
			center = group->mBounds[0];
			size = group->mBounds[1];
			
			if (child->isInside(LLVector3d(mOrigin)) || LLRayAABB(center, size, mOrigin, mDir, res, mRadius))
			{
				traverse(child);
			}
		}	
	}

	virtual void visit(const LLSpatialGroup::OctreeState* branch) { }

};

BOOL LLSpatialGroup::isVisible()
{
	if (LLPipeline::sUseOcclusion)
	{
		return !isState(CULLED | OCCLUDED);
	}
	else
	{
		return !isState(CULLED); 
	}
}

void LLSpatialGroup::validate()
{
#if LL_OCTREE_PARANOIA_CHECK

	sg_assert(!isState(DIRTY));
	sg_assert(!isDead());

	LLVector3 myMin = mBounds[0] - mBounds[1];
	LLVector3 myMax = mBounds[0] + mBounds[1];

	validateDrawMap();

	for (element_iter i = getData().begin(); i != getData().end(); ++i)
	{
		LLDrawable* drawable = *i;
		sg_assert(drawable->getSpatialGroup() == this);
		if (drawable->getSpatialBridge())
		{
			sg_assert(drawable->getSpatialBridge() == mSpatialPartition->asBridge());
		}

		if (drawable->isSpatialBridge())
		{
			LLSpatialPartition* part = drawable->asPartition();
			if (!part)
			{
				llerrs << "Drawable reports it is a spatial bridge but not a partition." << llendl;
			}
			LLSpatialGroup* group = (LLSpatialGroup*) part->mOctree->getListener(0);
			group->validate();
		}
	}

	for (U32 i = 0; i < mOctreeNode->getChildCount(); ++i)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) mOctreeNode->getChild(i)->getListener(0);

		group->validate();
		
		//ensure all children are enclosed in this node
		LLVector3 center = group->mBounds[0];
		LLVector3 size = group->mBounds[1];
		
		LLVector3 min = center - size;
		LLVector3 max = center + size;
		
		for (U32 j = 0; j < 3; j++)
		{
			sg_assert(min.mV[j] >= myMin.mV[j]-0.02f);
			sg_assert(max.mV[j] <= myMax.mV[j]+0.02f);
		}
	}

#endif
}

void validate_draw_info(LLDrawInfo& params)
{
#if LL_OCTREE_PARANOIA_CHECK
	if (params.mVertexBuffer.isNull())
	{
		llerrs << "Draw batch has no vertex buffer." << llendl;
	}
	
	//bad range
	if (params.mStart >= params.mEnd)
	{
		llerrs << "Draw batch has invalid range." << llendl;
	}
	
	if (params.mEnd >= (U32) params.mVertexBuffer->getNumVerts())
	{
		llerrs << "Draw batch has buffer overrun error." << llendl;
	}
	
	if (params.mOffset + params.mCount > (U32) params.mVertexBuffer->getNumIndices())
	{
		llerrs << "Draw batch has index buffer ovverrun error." << llendl;
	}
	
	//bad indices
	U32* indicesp = (U32*) params.mVertexBuffer->getIndicesPointer();
	if (indicesp)
	{
		for (U32 i = params.mOffset; i < params.mOffset+params.mCount; i++)
		{
			if (indicesp[i] < params.mStart)
			{
				llerrs << "Draw batch has vertex buffer index out of range error (index too low)." << llendl;
			}
			
			if (indicesp[i] > params.mEnd)
			{
				llerrs << "Draw batch has vertex buffer index out of range error (index too high)." << llendl;
			}
		}
	}
#endif
}

void LLSpatialGroup::validateDrawMap()
{
#if LL_OCTREE_PARANOIA_CHECK
	for (draw_map_t::iterator i = mDrawMap.begin(); i != mDrawMap.end(); ++i)
	{
		std::vector<LLDrawInfo*>& draw_vec = i->second;
		for (std::vector<LLDrawInfo*>::iterator j = draw_vec.begin(); j != draw_vec.end(); ++j)
		{
			LLDrawInfo& params = **j;
			
			validate_draw_info(params);
		}
	}
#endif
}

void LLSpatialGroup::makeStatic()
{
#if !LL_DARWIN
	if (isState(GEOM_DIRTY | ALPHA_DIRTY))
	{
		return;
	}
	
	if (mSpatialPartition->mRenderByGroup && mBufferUsage != GL_STATIC_DRAW_ARB)
	{
		mBufferUsage = GL_STATIC_DRAW_ARB;
		if (mVertexBuffer.notNull())
		{
			mVertexBuffer->makeStatic();
		}
		
		for (buffer_map_t::iterator i = mBufferMap.begin(); i != mBufferMap.end(); ++i)
		{
			i->second->makeStatic();
		}
		
		mBuilt = 1.f;
	}
#endif
}

BOOL LLSpatialGroup::updateInGroup(LLDrawable *drawablep, BOOL immediate)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
		
	drawablep->updateSpatialExtents();
	validate_drawable(drawablep);

	OctreeNode* parent = mOctreeNode->getOctParent();
	
	if (mOctreeNode->isInside(drawablep->getPositionGroup()) && 
		(mOctreeNode->contains(drawablep) ||
		 (drawablep->getBinRadius() > mOctreeNode->getSize().mdV[0] &&
				parent && parent->getElementCount() >= LL_OCTREE_MAX_CAPACITY)))
	{
		unbound();
		setState(OBJECT_DIRTY);
		setState(GEOM_DIRTY);
		validate_drawable(drawablep);
		return TRUE;
	}
		
	return FALSE;
}


BOOL LLSpatialGroup::addObject(LLDrawable *drawablep, BOOL add_all, BOOL from_octree)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	if (!from_octree)
	{
		mOctreeNode->insert(drawablep);
	}
	else
	{
		drawablep->setSpatialGroup(this);
		validate_drawable(drawablep);
		setState(OBJECT_DIRTY | GEOM_DIRTY);
		mLastAddTime = gFrameTimeSeconds;
		if (drawablep->isSpatialBridge())
		{
			mBridgeList.push_back((LLSpatialBridge*) drawablep);
		}
		if (drawablep->getRadius() > 1.f)
		{
			setState(IMAGE_DIRTY);
		}
	}

	return TRUE;
}

void LLSpatialGroup::rebuildGeom()
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	if (!isDead())
	{
		mSpatialPartition->rebuildGeom(this);
	}
}

void LLSpatialPartition::rebuildGeom(LLSpatialGroup* group)
{
	if (group->changeLOD())
	{
		group->mLastUpdateDistance = group->mDistance;
		group->mLastUpdateViewAngle = group->mViewAngle;
	}

	if (group->isDead() || !group->isState(LLSpatialGroup::GEOM_DIRTY))
	{
		return;
	}

	LLFastTimer ftm(LLFastTimer::FTM_REBUILD_VBO);	

	group->clearDrawMap();
	
	//get geometry count
	group->mIndexCount = 0;
	group->mVertexCount = 0;

	addGeometryCount(group, group->mVertexCount, group->mIndexCount);

	if (group->mVertexCount > 0 && group->mIndexCount > 0)
	{ //create vertex buffer containing volume geometry for this node
		group->mBuilt = 1.f;
		if (group->mVertexBuffer.isNull() || (group->mBufferUsage != group->mVertexBuffer->getUsage() && LLVertexBuffer::sEnableVBOs))
		{
			//LLFastTimer ftm(LLFastTimer::FTM_REBUILD_NONE_VB);
			group->mVertexBuffer = createVertexBuffer(mVertexDataMask, group->mBufferUsage);
			group->mVertexBuffer->allocateBuffer(group->mVertexCount, group->mIndexCount, true);
			stop_glerror();
		}
		else
		{
			//LLFastTimer ftm(LLFastTimer::FTM_REBUILD_NONE_VB);
			group->mVertexBuffer->resizeBuffer(group->mVertexCount, group->mIndexCount);
			stop_glerror();
		}
		
		{
			LLFastTimer ftm((LLFastTimer::EFastTimerType) ((U32) LLFastTimer::FTM_REBUILD_VOLUME_VB + mPartitionType));
			getGeometry(group);
		}
	}
	else
	{
		group->mVertexBuffer = NULL;
		group->mBufferMap.clear();
	}

	group->mLastUpdateTime = gFrameTimeSeconds;
	group->clearState(LLSpatialGroup::GEOM_DIRTY | LLSpatialGroup::MATRIX_DIRTY);
}

BOOL LLSpatialGroup::boundObjects(BOOL empty, LLVector3& minOut, LLVector3& maxOut)
{	
	const OctreeState* node = mOctreeNode->getOctState();

	if (node->getData().empty())
	{	//don't do anything if there are no objects
		if (empty && mOctreeNode->getParent())
		{	//only root is allowed to be empty
			OCT_ERRS << "Empty leaf found in octree." << llendl;
		}
		return FALSE;
	}

	LLVector3& newMin = mObjectExtents[0];
	LLVector3& newMax = mObjectExtents[1];
	
	if (isState(OBJECT_DIRTY))
	{ //calculate new bounding box
		clearState(OBJECT_DIRTY);

		//initialize bounding box to first element
		OctreeState::const_element_iter i = node->getData().begin();
		LLDrawable* drawablep = *i;
		const LLVector3* minMax = drawablep->getSpatialExtents();

		newMin.setVec(minMax[0]);
		newMax.setVec(minMax[1]);

		for (++i; i != node->getData().end(); ++i)
		{
			drawablep = *i;
			minMax = drawablep->getSpatialExtents();
			
			//bin up the object
			for (U32 i = 0; i < 3; i++)
			{
				if (minMax[0].mV[i] < newMin.mV[i])
				{
					newMin.mV[i] = minMax[0].mV[i];
				}
				if (minMax[1].mV[i] > newMax.mV[i])
				{
					newMax.mV[i] = minMax[1].mV[i];
				}
			}
		}
		
		mObjectBounds[0] = (newMin + newMax) * 0.5f;
		mObjectBounds[1] = (newMax - newMin) * 0.5f;
	}
	
	if (empty)
	{
		minOut = newMin;
		maxOut = newMax;
	}
	else
	{
		for (U32 i = 0; i < 3; i++)
		{
			if (newMin.mV[i] < minOut.mV[i])
			{
				minOut.mV[i] = newMin.mV[i];
			}
			if (newMax.mV[i] > maxOut.mV[i])
			{
				maxOut.mV[i] = newMax.mV[i];
			}
		}
	}
		
	return TRUE;
}

void LLSpatialGroup::unbound()
{
	if (isState(DIRTY))
	{
		return;
	}

	setState(DIRTY);
	
	//all the parent nodes need to rebound this child
	if (mOctreeNode)
	{
		OctreeNode* parent = (OctreeNode*) mOctreeNode->getParent();
		while (parent != NULL)
		{
			LLSpatialGroup* group = (LLSpatialGroup*) parent->getListener(0);
			if (group->isState(DIRTY))
			{
				return;
			}
			
			group->setState(DIRTY);
			parent = (OctreeNode*) parent->getParent();
		}
	}
}

LLSpatialGroup* LLSpatialGroup::getParent()
{
	if (isDead())
	{
		return NULL;
	}

	OctreeNode* parent = mOctreeNode->getOctParent();

	if (parent)
	{
		return (LLSpatialGroup*) parent->getListener(0);
	}

	return NULL;
}

BOOL LLSpatialGroup::removeObject(LLDrawable *drawablep, BOOL from_octree)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	unbound();
	if (mOctreeNode && !from_octree)
	{
		if (!mOctreeNode->remove(drawablep))
		{
			OCT_ERRS << "Could not remove drawable from spatial group" << llendl;
		}
	}
	else
	{
		drawablep->setSpatialGroup(NULL);
		setState(GEOM_DIRTY);
		if (drawablep->isSpatialBridge())
		{
			for (bridge_list_t::iterator i = mBridgeList.begin(); i != mBridgeList.end(); ++i)
			{
				if (*i == drawablep)
				{
					mBridgeList.erase(i);
					break;
				}
			}
		}
	}
	return TRUE;
}

void LLSpatialGroup::shift(const LLVector3 &offset)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	LLVector3d offsetd(offset);
	mOctreeNode->setCenter(mOctreeNode->getCenter()+offsetd);
	mOctreeNode->updateMinMax();
	mBounds[0] += offset;
	mExtents[0] += offset;
	mExtents[1] += offset;
	mObjectBounds[0] += offset;
	mObjectExtents[0] += offset;
	mObjectExtents[1] += offset;

	setState(GEOM_DIRTY | MATRIX_DIRTY | OCCLUSION_DIRTY);
}

class LLSpatialSetState : public LLSpatialGroup::OctreeTraveler
{
public:
	U32 mState;
	LLSpatialSetState(U32 state) : mState(state) { }
	virtual void visit(const LLSpatialGroup::OctreeState* branch) { ((LLSpatialGroup*) branch->getListener(0))->setState(mState); }	
};

class LLSpatialSetStateDiff : public LLSpatialSetState
{
public:
	LLSpatialSetStateDiff(U32 state) : LLSpatialSetState(state) { }

	virtual void traverse(const LLSpatialGroup::TreeNode* n)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) n->getListener(0);
		
		if (!group->isState(mState))
		{
			LLSpatialGroup::OctreeTraveler::traverse(n);
		}
	}
};

void LLSpatialGroup::setState(U32 state, S32 mode) 
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	if (mode > STATE_MODE_SINGLE)
	{
		if (mode == STATE_MODE_DIFF)
		{
			LLSpatialSetStateDiff setter(state);
			setter.traverse(mOctreeNode);
		}
		else
		{
			LLSpatialSetState setter(state);
			setter.traverse(mOctreeNode);
		}
	}
	else
	{
		mState |= state;
	}
}

class LLSpatialClearState : public LLSpatialGroup::OctreeTraveler
{
public:
	U32 mState;
	LLSpatialClearState(U32 state) : mState(state) { }
	virtual void visit(const LLSpatialGroup::OctreeState* branch) { ((LLSpatialGroup*) branch->getListener(0))->clearState(mState); }
};

class LLSpatialClearStateDiff : public LLSpatialClearState
{
public:
	LLSpatialClearStateDiff(U32 state) : LLSpatialClearState(state) { }

	virtual void traverse(const LLSpatialGroup::TreeNode* n)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) n->getListener(0);
		
		if (!group->isState(mState))
		{
			LLSpatialGroup::OctreeTraveler::traverse(n);
		}
	}
};


void LLSpatialGroup::clearState(U32 state, S32 mode)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	if (mode > STATE_MODE_SINGLE)
	{
		if (mode == STATE_MODE_DIFF)
		{
			LLSpatialClearStateDiff clearer(state);
			clearer.traverse(mOctreeNode);
		}
		else
		{
			LLSpatialClearState clearer(state);
			clearer.traverse(mOctreeNode);
		}
	}
	else
	{
		mState &= ~state;
	}

#if LL_OCTREE_PARANOIA_CHECK
	if (state & LLSpatialGroup::ACTIVE_OCCLUSION)
	{
		LLSpatialPartition* part = mSpatialPartition;
		for (U32 i = 0; i < part->mOccludedList.size(); i++)
		{
			if (part->mOccludedList[i] == this)
			{
				llerrs << "LLSpatialGroup state error: " << mState << llendl;
			}
		}
	}
#endif
}

//======================================
//		Octree Listener Implementation
//======================================

LLSpatialGroup::LLSpatialGroup(OctreeNode* node, LLSpatialPartition* part) :
	mState(0),
	mBuilt(0.f),
	mOctreeNode(node),
	mSpatialPartition(part),
	mVertexBuffer(NULL), 
	mBufferUsage(GL_STATIC_DRAW_ARB),
	mDistance(0.f),
	mDepth(0.f),
	mLastUpdateDistance(-1.f), 
	mLastUpdateTime(gFrameTimeSeconds),
	mLastAddTime(gFrameTimeSeconds),
	mLastRenderTime(gFrameTimeSeconds),
	mViewAngle(0.f),
	mLastUpdateViewAngle(-1.f)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);

	sg_assert(mOctreeNode->getListenerCount() == 0);
	mOctreeNode->addListener(this);
	setState(SG_INITIAL_STATE_MASK);

	mBounds[0] = LLVector3(node->getCenter());
	mBounds[1] = LLVector3(node->getSize());

	part->mLODSeed = (part->mLODSeed+1)%part->mLODPeriod;
	mLODHash = part->mLODSeed;
	
	mRadius = 1;
	mPixelArea = 1024.f;
}

void LLSpatialGroup::updateDistance(LLCamera &camera)
{
#if !LL_RELEASE_FOR_DOWNLOAD
	if (isState(LLSpatialGroup::OBJECT_DIRTY))
	{
		llerrs << "Spatial group dirty on distance update." << llendl;
	}
#endif
	if (!getData().empty())
	{
		mRadius = mSpatialPartition->mRenderByGroup ? mObjectBounds[1].magVec() :
						(F32) mOctreeNode->getSize().magVec();
		mDistance = mSpatialPartition->calcDistance(this, camera);
		mPixelArea = mSpatialPartition->calcPixelArea(this, camera);
	}
}

F32 LLSpatialPartition::calcDistance(LLSpatialGroup* group, LLCamera& camera)
{
	LLVector3 eye = group->mObjectBounds[0] - camera.getOrigin();

	F32 dist = 0.f;

	if (group->mDrawMap.find(LLRenderPass::PASS_ALPHA) != group->mDrawMap.end())
	{
		LLVector3 v = eye;
		dist = eye.normVec();

		if (!group->isState(LLSpatialGroup::ALPHA_DIRTY))
		{
			LLVector3 view_angle = LLVector3(eye * LLVector3(1,0,0),
											eye * LLVector3(0,1,0),
											eye * LLVector3(0,0,1));

			if ((view_angle-group->mLastUpdateViewAngle).magVec() > 0.64f)
			{
				group->mViewAngle = view_angle;
				group->mLastUpdateViewAngle = view_angle;
				//for occasional alpha sorting within the group
				//NOTE: If there is a trivial way to detect that alpha sorting here would not change the render order,
				//not setting this node to dirty would be a very good thing
				group->setState(LLSpatialGroup::ALPHA_DIRTY);
			}		
		}

		//calculate depth of node for alpha sorting

		LLVector3 at = camera.getAtAxis();

		//front of bounding box
		for (U32 i = 0; i < 3; i++)
		{
			v.mV[i] -= group->mObjectBounds[1].mV[i]*0.25f * at.mV[i];
		}

		group->mDepth = v * at;

		F32 water_height = gAgent.getRegion()->getWaterHeight();
		//figure out if this node is above or below water
		if (group->mObjectBounds[0].mV[2] < water_height)
		{
			group->setState(LLSpatialGroup::BELOW_WATER);
		}
		else
		{
			group->clearState(LLSpatialGroup::BELOW_WATER);
		}
	}
	else
	{
		dist = eye.magVec();
	}

	if (dist < 16.f)
	{
		dist /= 16.f;
		dist *= dist;
		dist *= 16.f;
	}

	return dist;
}

F32 LLSpatialPartition::calcPixelArea(LLSpatialGroup* group, LLCamera& camera)
{
	return LLPipeline::calcPixelArea(group->mObjectBounds[0], group->mObjectBounds[1], camera);
}

BOOL LLSpatialGroup::changeLOD()
{
	if (isState(ALPHA_DIRTY))
	{ ///an alpha sort is going to happen, update distance and LOD
		return TRUE;
	}

	if (mSpatialPartition->mSlopRatio > 0.f)
	{
		F32 ratio = (mDistance - mLastUpdateDistance)/(llmax(mLastUpdateDistance, mRadius));

		if (fabsf(ratio) >= mSpatialPartition->mSlopRatio)
		{
			return TRUE;
		}

		if (mDistance > mRadius)
		{
			return FALSE;
		}
	}
	
	if (LLDrawable::getCurrentFrame()%mSpatialPartition->mLODPeriod == mLODHash)
	{
		return TRUE;
	}
	
	return FALSE;
}

void LLSpatialGroup::handleInsertion(const TreeNode* node, LLDrawable* drawablep)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	addObject(drawablep, FALSE, TRUE);
	unbound();
	setState(OBJECT_DIRTY);
}

void LLSpatialGroup::handleRemoval(const TreeNode* node, LLDrawable* drawable)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	removeObject(drawable, TRUE);
	setState(OBJECT_DIRTY);
}

void LLSpatialGroup::handleDestruction(const TreeNode* node)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	setState(DEAD);
	
	for (element_iter i = getData().begin(); i != getData().end(); ++i)
	{
		LLDrawable* drawable = *i;
		if (drawable->getSpatialGroup() == this)
		{
			drawable->setSpatialGroup(NULL);
		}
	}
	
	clearDrawMap();
	mOcclusionVerts = NULL;
	mVertexBuffer = NULL;
	mBufferMap.clear();
	sZombieGroups++;
	mOctreeNode = NULL;
}

void LLSpatialGroup::handleStateChange(const TreeNode* node)
{
	//drop bounding box upon state change
	if (mOctreeNode != node)
	{
		mOctreeNode = (OctreeNode*) node;
	}
	unbound();
}

void LLSpatialGroup::handleChildAddition(const OctreeNode* parent, OctreeNode* child) 
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	if (child->getListenerCount() == 0)
	{
		LLSpatialGroup* group = new LLSpatialGroup(child, mSpatialPartition);
		group->setState(mState & SG_STATE_INHERIT_MASK);
	}
	else
	{
		OCT_ERRS << "LLSpatialGroup redundancy detected." << llendl;
	}

	unbound();
}

void LLSpatialGroup::handleChildRemoval(const OctreeNode* parent, const OctreeNode* child)
{
	unbound();
}

void LLSpatialGroup::destroyGL() 
{
	setState(LLSpatialGroup::GEOM_DIRTY | 
					LLSpatialGroup::OCCLUSION_DIRTY |
					LLSpatialGroup::IMAGE_DIRTY);
	mLastUpdateTime = gFrameTimeSeconds;
	mVertexBuffer = NULL;
	mBufferMap.clear();

	mOcclusionVerts = NULL;
	mReflectionMap = NULL;
	clearDrawMap();

	for (LLSpatialGroup::element_iter i = getData().begin(); i != getData().end(); ++i)
	{
		LLDrawable* drawable = *i;
		for (S32 j = 0; j < drawable->getNumFaces(); j++)
		{
			LLFace* facep = drawable->getFace(j);
			facep->mVertexBuffer = NULL;
			facep->mLastVertexBuffer = NULL;
		}
	}
}

BOOL LLSpatialGroup::rebound()
{
	if (!isState(DIRTY))
	{	//return TRUE if we're not empty
		return TRUE;
	}
	
	LLVector3 oldBounds[2];
	
	if (mSpatialPartition->isVolatile() && isState(QUERY_OUT))
	{	//a query has been issued, if our bounding box changes significantly
		//we need to discard the issued query
		oldBounds[0] = mBounds[0];
		oldBounds[1] = mBounds[1];
	}
	
	if (mOctreeNode->getChildCount() == 1 && mOctreeNode->getElementCount() == 0)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) mOctreeNode->getChild(0)->getListener(0);
		group->rebound();
		
		//copy single child's bounding box
		mBounds[0] = group->mBounds[0];
		mBounds[1] = group->mBounds[1];
		mExtents[0] = group->mExtents[0];
		mExtents[1] = group->mExtents[1];
		
		group->setState(SKIP_FRUSTUM_CHECK);
	}
	else if (mOctreeNode->hasLeafState())
	{ //copy object bounding box if this is a leaf
		boundObjects(TRUE, mExtents[0], mExtents[1]);
		mBounds[0] = mObjectBounds[0];
		mBounds[1] = mObjectBounds[1];
	}
	else
	{
		LLVector3& newMin = mExtents[0];
		LLVector3& newMax = mExtents[1];
		LLSpatialGroup* group = (LLSpatialGroup*) mOctreeNode->getChild(0)->getListener(0);
		group->clearState(SKIP_FRUSTUM_CHECK);
		group->rebound();
		//initialize to first child
		newMin = group->mExtents[0];
		newMax = group->mExtents[1];

		//first, rebound children
		for (U32 i = 1; i < mOctreeNode->getChildCount(); i++)
		{
			group = (LLSpatialGroup*) mOctreeNode->getChild(i)->getListener(0);
			group->clearState(SKIP_FRUSTUM_CHECK);
			group->rebound();
			const LLVector3& max = group->mExtents[1];
			const LLVector3& min = group->mExtents[0];

			for (U32 j = 0; j < 3; j++)
			{
				if (max.mV[j] > newMax.mV[j])
				{
					newMax.mV[j] = max.mV[j];
				}
				if (min.mV[j] < newMin.mV[j])
				{
					newMin.mV[j] = min.mV[j];
				}
			}
		}

		boundObjects(FALSE, newMin, newMax);
		
		mBounds[0] = (newMin + newMax)*0.5f;
		mBounds[1] = (newMax - newMin)*0.5f;
	}
	
	if (mSpatialPartition->isVolatile() && isState(QUERY_OUT))
	{
		for (U32 i = 0; i < 3 && !isState(DISCARD_QUERY); i++)
		{
			if (fabsf(mBounds[0].mV[i]-oldBounds[0].mV[i]) > SG_DISCARD_TOLERANCE ||
				fabsf(mBounds[1].mV[i]-oldBounds[1].mV[i]) > SG_DISCARD_TOLERANCE)
			{	//bounding box changed significantly, discard last issued
				//occlusion query
				setState(DISCARD_QUERY);
			}
		}
	}
	
	setState(OCCLUSION_DIRTY);
	
	clearState(DIRTY);

	return TRUE;
}

//==============================================

LLSpatialPartition::LLSpatialPartition(U32 data_mask, BOOL is_volatile, U32 buffer_usage)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	mDrawableType = 0;
	mPartitionType = LLPipeline::PARTITION_NONE;
	mVolatile = is_volatile;
	mLODSeed = 0;
	mLODPeriod = 1;
	mVertexDataMask = data_mask;
	mBufferUsage = buffer_usage;
	mDepthMask = FALSE;
	mSlopRatio = 0.25f;
	mRenderByGroup = TRUE;
	mImageEnabled = FALSE;

	mOctree = new LLSpatialGroup::OctreeNode(LLVector3d(0,0,0), 
											LLVector3d(1,1,1), 
											new LLSpatialGroup::OctreeRoot(), NULL);
	new LLSpatialGroup(mOctree, this);
}


LLSpatialPartition::~LLSpatialPartition()
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	
	for (U32 i = 0; i < mOcclusionQueries.size(); i++)
	{
		glDeleteQueriesARB(1, (GLuint*)(&(mOcclusionQueries[i])));
	}
	
	delete mOctree;
	mOctree = NULL;
}


LLSpatialGroup *LLSpatialPartition::put(LLDrawable *drawablep, BOOL was_visible)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	const F32 MAX_MAG = 1000000.f*1000000.f; // 1 million
			
	if (drawablep->getPositionGroup().magVecSquared() > MAX_MAG)
	{
#if 0 //ndef LL_RELEASE_FOR_DOWNLOAD
		llwarns << "LLSpatialPartition::put Object out of range!" << llendl;
		llinfos << drawablep->getPositionGroup() << llendl;

		if (drawablep->getVObj())
		{
			llwarns << "Dumping debugging info: " << llendl;
			drawablep->getVObj()->dump(); 
		}
#endif
		return NULL;
	}
		
	drawablep->updateSpatialExtents();
	validate_drawable(drawablep);

	//keep drawable from being garbage collected
	LLPointer<LLDrawable> ptr = drawablep;
		
	assert_octree_valid(mOctree);
	mOctree->insert(drawablep);
	assert_octree_valid(mOctree);

	LLSpatialGroup::OctreeNode* node = mOctree->getNodeAt(drawablep);
	
	LLSpatialGroup* group = (LLSpatialGroup*) node->getListener(0);
	if (was_visible && group->isState(LLSpatialGroup::QUERY_OUT))
	{
		group->setState(LLSpatialGroup::DISCARD_QUERY);
	}

	return group;
}

BOOL LLSpatialPartition::remove(LLDrawable *drawablep, LLSpatialGroup *curp)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	
	drawablep->setSpatialGroup(NULL);

	if (!curp->removeObject(drawablep))
	{
		OCT_ERRS << "Failed to remove drawable from octree!" << llendl;
	}

	assert_octree_valid(mOctree);
	
	return TRUE;
}

void LLSpatialPartition::move(LLDrawable *drawablep, LLSpatialGroup *curp, BOOL immediate)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	LLFastTimer t(LLFastTimer::FTM_UPDATE_MOVE);

	// sanity check submitted by open source user bushing Spatula
	// who was seeing crashing here. (See VWR-424 reported by Bunny Mayne)
	if (!drawablep) {
		OCT_ERRS << "LLSpatialPartition::move was passed a bad drawable." << llendl;
		return;
	}
		
	BOOL was_visible = curp ? curp->isVisible() : FALSE;

	if (curp && curp->mSpatialPartition != this)
	{
		//keep drawable from being garbage collected
		LLPointer<LLDrawable> ptr = drawablep;
		if (curp->mSpatialPartition->remove(drawablep, curp))
		{
			put(drawablep, was_visible);
			return;
		}
		else
		{
			OCT_ERRS << "Drawable lost between spatial partitions on outbound transition." << llendl;
		}
	}
		
	if (curp && curp->updateInGroup(drawablep, immediate))
	{
		// Already updated, don't need to do anything
		assert_octree_valid(mOctree);
		return;
	}

	//keep drawable from being garbage collected
	LLPointer<LLDrawable> ptr = drawablep;
	if (curp && !remove(drawablep, curp))
	{
		OCT_ERRS << "Move couldn't find existing spatial group!" << llendl;
	}

	put(drawablep, was_visible);
}

class LLSpatialShift : public LLSpatialGroup::OctreeTraveler
{
public:
	LLSpatialShift(LLVector3 offset) : mOffset(offset) { }
	virtual void visit(const LLSpatialGroup::OctreeState* branch) 
	{ 
		((LLSpatialGroup*) branch->getListener(0))->shift(mOffset); 
	}
		
	LLVector3 mOffset;
};

void LLSpatialPartition::shift(const LLVector3 &offset)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	llinfos << "Shifting octree: " << offset << llendl;
	LLSpatialShift shifter(offset);
	shifter.traverse(mOctree);
}

BOOL LLSpatialPartition::checkOcclusion(LLSpatialGroup* group, LLCamera* camera)
{
	if (LLPipeline::sUseOcclusion &&
		!group->isState(LLSpatialGroup::ACTIVE_OCCLUSION | LLSpatialGroup::OCCLUDED) &&
		(!camera || !earlyFail(camera, group)))
	{
		group->setState(LLSpatialGroup::ACTIVE_OCCLUSION);
		mQueryQueue.push(group);
		return TRUE;
	}

	return FALSE;
}

class LLOctreeCull : public LLSpatialGroup::OctreeTraveler
{
public:
	LLOctreeCull(LLCamera* camera)
		: mCamera(camera), mRes(0) { }

	virtual bool earlyFail(const LLSpatialGroup* group)
	{
		if (group->mOctreeNode->getParent() &&	//never occlusion cull the root node
			LLPipeline::sUseOcclusion &&			//never occlusion cull selection
			group->isState(LLSpatialGroup::OCCLUDED))
		{
			return true;
		}
		
		return false;
	}
	
	virtual void traverse(const LLSpatialGroup::TreeNode* n)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) n->getListener(0);

		if (earlyFail(group))
		{
			return;
		}
		
		if (mRes == 2 || 
			(mRes && group->isState(LLSpatialGroup::SKIP_FRUSTUM_CHECK)))
		{	//fully in, just add everything
			LLSpatialGroup::OctreeTraveler::traverse(n);
		}
		else
		{
			mRes = mCamera->AABBInFrustum(group->mBounds[0], group->mBounds[1]);
				
			if (mRes)
			{ //at least partially in, run on down
				LLSpatialGroup::OctreeTraveler::traverse(n);
			}
			else
			{
				lateFail(group);
			}
			mRes = 0;
		}
	}
	
	virtual void lateFail(LLSpatialGroup* group)
	{
		if (!group->isState(LLSpatialGroup::CULLED))
		{ //totally culled, so are all its children
			group->setState(LLSpatialGroup::CULLED, LLSpatialGroup::STATE_MODE_DIFF);
		}
	}

	virtual bool checkObjects(const LLSpatialGroup::OctreeState* branch, const LLSpatialGroup* group)
	{
			
		if (branch->getElementCount() == 0) //no elements
		{
			return false;
		}
		else if (branch->getChildCount() == 0) //leaf state, already checked tightest bounding box
		{
			return true;
		}
		else if (mRes == 1 && !mCamera->AABBInFrustum(group->mObjectBounds[0], group->mObjectBounds[1])) //no objects in frustum
		{
			return false;
		}
		
		return true;
	}

	virtual void preprocess(LLSpatialGroup* group)
	{
		if (group->isState(LLSpatialGroup::CULLED))
		{	//this is the first frame this node is visible
			group->clearState(LLSpatialGroup::CULLED);
			if (group->mOctreeNode->hasLeafState())
			{	//if it's a leaf, force it onto the active occlusion list to prevent
				//massive frame stutters
				group->mSpatialPartition->checkOcclusion(group, mCamera);
			}
		}

		if (LLPipeline::sDynamicReflections &&
			group->mOctreeNode->getSize().mdV[0] == 16.0 && 
			group->mDistance < 64.f &&
			group->mLastAddTime < gFrameTimeSeconds - 2.f)
		{
			group->mSpatialPartition->markReimage(group);
		}
	}
	
	virtual void processGroup(LLSpatialGroup* group)
	{
		gPipeline.markNotCulled(group, *mCamera);
	}
	
	virtual void visit(const LLSpatialGroup::OctreeState* branch) 
	{	
		LLSpatialGroup* group = (LLSpatialGroup*) branch->getListener(0);

		preprocess(group);
		
		if (checkObjects(branch, group))
		{
			processGroup(group);
		}
	}

	LLCamera *mCamera;
	S32 mRes;
};


class LLOctreeSelect : public LLOctreeCull
{
public:
	LLOctreeSelect(LLCamera* camera, std::vector<LLDrawable*>* results)
		: LLOctreeCull(camera), mResults(results) { }

	virtual bool earlyFail(const LLSpatialGroup* group) { return false; }
	virtual void lateFail(LLSpatialGroup* group) { }
	virtual void preprocess(LLSpatialGroup* group) { }

	virtual void processGroup(LLSpatialGroup* group)
	{
		LLSpatialGroup::OctreeState* branch = group->mOctreeNode->getOctState();

		for (LLSpatialGroup::OctreeState::const_element_iter i = branch->getData().begin(); i != branch->getData().end(); ++i)
		{
			LLDrawable* drawable = *i;
			
			if (!drawable->isDead())
			{
				if (drawable->isSpatialBridge())
				{
					drawable->setVisible(*mCamera, mResults, TRUE);
				}
				else
				{
					mResults->push_back(drawable);
				}
			}		
		}
	}
	
	std::vector<LLDrawable*>* mResults;
};


void genBoxList()
{
	if (sBoxList != 0)
	{
		return;
	}

	sBoxList = glGenLists(1);
	glNewList(sBoxList, GL_COMPILE);
	
	LLVector3 c,r;
	c = LLVector3(0,0,0);
	r = LLVector3(1,1,1);

	glBegin(GL_TRIANGLE_STRIP);
	//left front
	glVertex3fv((c+r.scaledVec(LLVector3(-1,1,-1))).mV);
	glVertex3fv((c+r.scaledVec(LLVector3(-1,1,1))).mV);
	//right front
	glVertex3fv((c+r.scaledVec(LLVector3(1,1,-1))).mV);
	glVertex3fv((c+r.scaledVec(LLVector3(1,1,1))).mV);
	//right back
 	glVertex3fv((c+r.scaledVec(LLVector3(1,-1,-1))).mV);
	glVertex3fv((c+r.scaledVec(LLVector3(1,-1,1))).mV);
	//left back
	glVertex3fv((c+r.scaledVec(LLVector3(-1,-1,-1))).mV);
	glVertex3fv((c+r.scaledVec(LLVector3(-1,-1,1))).mV);
	//left front
	glVertex3fv((c+r.scaledVec(LLVector3(-1,1,-1))).mV);
	glVertex3fv((c+r.scaledVec(LLVector3(-1,1,1))).mV);
	glEnd();
	
	//bottom
	glBegin(GL_TRIANGLE_STRIP);
	glVertex3fv((c+r.scaledVec(LLVector3(1,1,-1))).mV);
	glVertex3fv((c+r.scaledVec(LLVector3(1,-1,-1))).mV);
	glVertex3fv((c+r.scaledVec(LLVector3(-1,1,-1))).mV);
	glVertex3fv((c+r.scaledVec(LLVector3(-1,-1,-1))).mV);
	glEnd();

	//top
	glBegin(GL_TRIANGLE_STRIP);
	glVertex3fv((c+r.scaledVec(LLVector3(1,1,1))).mV);
	glVertex3fv((c+r.scaledVec(LLVector3(-1,1,1))).mV);
	glVertex3fv((c+r.scaledVec(LLVector3(1,-1,1))).mV);
	glVertex3fv((c+r.scaledVec(LLVector3(-1,-1,1))).mV);
	glEnd();	

	glEndList();
}

void drawBox(const LLVector3& c, const LLVector3& r)
{
	genBoxList();

	glPushMatrix();
	glTranslatef(c.mV[0], c.mV[1], c.mV[2]);
	glScalef(r.mV[0], r.mV[1], r.mV[2]);
	glCallList(sBoxList);
	glPopMatrix();
}

void drawBoxOutline(const LLVector3& pos, const LLVector3& size)
{
	LLVector3 v1 = size.scaledVec(LLVector3( 1, 1,1));
	LLVector3 v2 = size.scaledVec(LLVector3(-1, 1,1));
	LLVector3 v3 = size.scaledVec(LLVector3(-1,-1,1));
	LLVector3 v4 = size.scaledVec(LLVector3( 1,-1,1));

	glBegin(GL_LINE_LOOP); //top
	glVertex3fv((pos+v1).mV);
	glVertex3fv((pos+v2).mV);
	glVertex3fv((pos+v3).mV);
	glVertex3fv((pos+v4).mV);
	glEnd();

	glBegin(GL_LINE_LOOP); //bottom
	glVertex3fv((pos-v1).mV);
	glVertex3fv((pos-v2).mV);
	glVertex3fv((pos-v3).mV);
	glVertex3fv((pos-v4).mV);
	glEnd();


	glBegin(GL_LINES);

	//right
	glVertex3fv((pos+v1).mV);
	glVertex3fv((pos-v3).mV);
			
	glVertex3fv((pos+v4).mV);
	glVertex3fv((pos-v2).mV);

	//left
	glVertex3fv((pos+v2).mV);
	glVertex3fv((pos-v4).mV);

	glVertex3fv((pos+v3).mV);
	glVertex3fv((pos-v1).mV);

	glEnd();
}

class LLOctreeDirty : public LLOctreeTraveler<LLDrawable>
{
public:
	virtual void visit(const LLOctreeState<LLDrawable>* state)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) state->getListener(0);
		group->destroyGL();

		for (LLSpatialGroup::element_iter i = group->getData().begin(); i != group->getData().end(); ++i)
		{
			LLDrawable* drawable = *i;
			if (drawable->getVObj().notNull() && !group->mSpatialPartition->mRenderByGroup)
			{
				gPipeline.markRebuild(drawable, LLDrawable::REBUILD_ALL, TRUE);
			}
		}

		for (LLSpatialGroup::bridge_list_t::iterator i = group->mBridgeList.begin(); i != group->mBridgeList.end(); ++i)
		{
			LLSpatialBridge* bridge = *i;
			traverse(bridge->mOctree);
		}
	}
};

void LLSpatialPartition::restoreGL()
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	mOcclusionQueries.clear();
	sBoxList = 0;

	//generate query ids
	while (mOcclusionQueries.size() < mOccludedList.size())
	{
		GLuint id;
		glGenQueriesARB(1, &id);
		mOcclusionQueries.push_back(id);
	}

	for (U32 i = 0; i < mOccludedList.size(); i++)
	{	//previously issued queries are now invalid
		mOccludedList[i]->setState(LLSpatialGroup::DISCARD_QUERY);
	}
	
	genBoxList();
}

void LLSpatialPartition::resetVertexBuffers()
{
	LLOctreeDirty dirty;
	dirty.traverse(mOctree);

	mOcclusionIndices = NULL;
}

S32 LLSpatialPartition::cull(LLCamera &camera, std::vector<LLDrawable *>* results, BOOL for_select)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	{
		LLFastTimer ftm(LLFastTimer::FTM_CULL_REBOUND);		
		LLSpatialGroup* group = (LLSpatialGroup*) mOctree->getListener(0);
		group->rebound();
	}
	
	if (for_select)
	{
		LLOctreeSelect selecter(&camera, results);
		selecter.traverse(mOctree);
	}
	else
	{
		LLFastTimer ftm(LLFastTimer::FTM_FRUSTUM_CULL);		
		LLOctreeCull culler(&camera);
		culler.traverse(mOctree);
	}
	
	return 0;
}

class LLOctreeClearOccludedNotActive : public LLSpatialGroup::OctreeTraveler
{
public:
	LLOctreeClearOccludedNotActive() { }

	virtual void traverse(const LLSpatialGroup::TreeNode* n)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) n->getListener(0);
		if ((!group->isState(LLSpatialGroup::ACTIVE_OCCLUSION)) //|| group->isState(LLSpatialGroup::QUERY_PENDING)
			|| group->isState(LLSpatialGroup::DEACTIVATE_OCCLUSION))
		{	//the children are all occluded or culled as well
			group->clearState(LLSpatialGroup::OCCLUDED);
			for (U32 i = 0; i < group->mOctreeNode->getChildCount(); i++)
			{
				traverse(group->mOctreeNode->getChild(i));
			}
		}
	}

	virtual void visit(const LLSpatialGroup::OctreeState* branch) { }
};

class LLQueueNonCulled : public LLSpatialGroup::OctreeTraveler
{
public:
	std::queue<LLSpatialGroup*>* mQueue;
	LLQueueNonCulled(std::queue<LLSpatialGroup*> *queue) : mQueue(queue) { }

	virtual void traverse(const LLSpatialGroup::TreeNode* n)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) n->getListener(0);
		if (group->isState(LLSpatialGroup::OCCLUDED | LLSpatialGroup::CULLED))
		{	//the children are all occluded or culled as well
			return;
		}

		if (!group->isState(LLSpatialGroup::IN_QUEUE))
		{
			group->setState(LLSpatialGroup::IN_QUEUE);
			mQueue->push(group);
		}

		LLSpatialGroup::OctreeTraveler::traverse(n);
	}

	virtual void visit(const LLSpatialGroup::OctreeState* branch) { }
};

BOOL earlyFail(LLCamera* camera, LLSpatialGroup* group)
{
	LLVector3 c = group->mBounds[0];
	LLVector3 r = group->mBounds[1]*SG_OCCLUSION_FUDGE + LLVector3(0.2f,0.2f,0.2f);
    
	//if (group->isState(LLSpatialGroup::CULLED)) // || 
	if (!camera->AABBInFrustum(c, r))
	{
		return TRUE;
	}

	LLVector3 e = camera->getOrigin();
	
	LLVector3 min = c - r;
	LLVector3 max = c + r;
	
	for (U32 j = 0; j < 3; j++)
	{
		if (e.mV[j] < min.mV[j] || e.mV[j] > max.mV[j])
		{
			return FALSE;
		}
	}

	return TRUE;
}

void LLSpatialPartition::markReimage(LLSpatialGroup* group)
{
	if (mImageEnabled && group->isState(LLSpatialGroup::IMAGE_DIRTY))
	{	
		if (!group->isState(LLSpatialGroup::IN_IMAGE_QUEUE))
		{
			group->setState(LLSpatialGroup::IN_IMAGE_QUEUE);
			mImageQueue.push(group);
		}
	}
}

void LLSpatialPartition::processImagery(LLCamera* camera)
{
	if (!mImageEnabled)
	{
		return;
	}

	U32 process_count = 1;

	while (process_count > 0 && !mImageQueue.empty())
	{
		LLPointer<LLSpatialGroup> group = mImageQueue.front();
		mImageQueue.pop();
	
		group->clearState(LLSpatialGroup::IN_IMAGE_QUEUE);

		if (group->isDead())
		{
			continue;
		}

		if (LLPipeline::sDynamicReflections)
		{
			process_count--;
			LLVector3 origin = group->mBounds[0];
			
			LLCamera cube_cam;
			cube_cam.setOrigin(origin);
			cube_cam.setFar(64.f);

			LLPointer<LLCubeMap> cube_map = group->mReflectionMap;
			group->mReflectionMap = NULL;
			if (cube_map.isNull())
			{
				cube_map = new LLCubeMap();
				cube_map->initGL();
			}

			if (gPipeline.mCubeBuffer.isNull())
			{
				gPipeline.mCubeBuffer = new LLCubeMap();
				gPipeline.mCubeBuffer->initGL();
			}

			S32 res = gSavedSettings.getS32("RenderReflectionRes");
			gPipeline.generateReflectionMap(gPipeline.mCubeBuffer, cube_cam, 128);
			gPipeline.blurReflectionMap(gPipeline.mCubeBuffer, cube_map, res);
			group->mReflectionMap = cube_map;
			group->setState(LLSpatialGroup::GEOM_DIRTY);
		}

		group->clearState(LLSpatialGroup::IMAGE_DIRTY);
	}
}

void validate_occlusion_list(std::vector<LLPointer<LLSpatialGroup> >& occluded_list)
{
#if !LL_RELEASE_FOR_DOWNLOAD
	for (U32 i = 0; i < occluded_list.size(); i++)
	{
		LLSpatialGroup* group = occluded_list[i];
		for (U32 j = i+1; j < occluded_list.size(); j++)
		{
			if (occluded_list[i] == occluded_list[j])
			{
				llerrs << "Duplicate node in occlusion list." << llendl;
			}
		}

		LLSpatialGroup::OctreeNode* parent = group->mOctreeNode->getOctParent();
		while (parent)
		{
			LLSpatialGroup* parent_group = (LLSpatialGroup*) parent->getListener(0);
			if (parent_group->isState(LLSpatialGroup::OCCLUDED))
			{
				llerrs << "Child node of occluded node in occlusion list (redundant query)." << llendl;
			}
			parent = parent->getOctParent();
		}
	}
#endif
}

void LLSpatialPartition::processOcclusion(LLCamera* camera)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	LLSpatialGroup* rootGroup = (LLSpatialGroup*) mOctree->getListener(0);
	{
		LLFastTimer ftm(LLFastTimer::FTM_CULL_REBOUND);	
		rootGroup->rebound();
	}
	
	//update potentials 
	if (!rootGroup->isState(LLSpatialGroup::IN_QUEUE))
	{
		rootGroup->setState(LLSpatialGroup::IN_QUEUE);
		mOcclusionQueue.push(rootGroup);
	}

	const U32 MAX_PULLED = 32;
	const U32 MAX_PUSHED = mOcclusionQueue.size();
	U32 count = 0;
	U32 pcount = 0;
	
	while (pcount < MAX_PUSHED && count < MAX_PULLED && !mOcclusionQueue.empty())
	{
		LLFastTimer t(LLFastTimer::FTM_OCCLUSION);

		LLPointer<LLSpatialGroup> group = mOcclusionQueue.front();
		if (!group->isState(LLSpatialGroup::IN_QUEUE))
		{
			OCT_ERRS << "Spatial Group State Error.  Group in queue not tagged as such." << llendl;
		}

		mOcclusionQueue.pop();
		group->clearState(LLSpatialGroup::IN_QUEUE);

		if (group->isDead())
		{
			continue;
		}

		if (group->isState(LLSpatialGroup::CULLED | LLSpatialGroup::OCCLUDED))
		{	//already culled, skip it
			continue;
		}

		//before we process, enqueue this group's children
		for (U32 i = 0; i < group->mOctreeNode->getChildCount(); i++)
		{
			LLSpatialGroup* child = (LLSpatialGroup*) group->mOctreeNode->getChild(i)->getListener(0);

			//if (!child->isState(LLSpatialGroup::OCCLUDED | LLSpatialGroup::CULLED)
			if (!child->isState(LLSpatialGroup::IN_QUEUE | LLSpatialGroup::ACTIVE_OCCLUSION))
			{
				child->setState(LLSpatialGroup::IN_QUEUE);
				mOcclusionQueue.push(child);
			}
		}
		
		if (earlyFail(camera, group))
		{
			sg_assert(!group->isState(LLSpatialGroup::OCCLUDED));
			group->setState(LLSpatialGroup::IN_QUEUE);
			mOcclusionQueue.push(group);
			pcount++;
			continue;
		}
		
		//add to pending queue
		if (!group->isState(LLSpatialGroup::ACTIVE_OCCLUSION))
		{
#if LL_OCTREE_PARANOIA_CHECK
			for (U32 i = 0; i < mOccludedList.size(); ++i)
			{
				sg_assert(mOccludedList[i] != group);
			}
#endif
			group->setState(LLSpatialGroup::ACTIVE_OCCLUSION);
			mQueryQueue.push(group);
			count++;
		}
	}

	//read back results from last frame
	for (U32 i = 0; i < mOccludedList.size(); i++)
	{
	 	LLFastTimer t(LLFastTimer::FTM_OCCLUSION_READBACK);

		if (mOccludedList[i]->isDead() || mOccludedList[i]->isState(LLSpatialGroup::DEACTIVATE_OCCLUSION))
		{
			continue;
		}
		GLuint res = 0;
			
		if (mOccludedList[i]->isState(LLSpatialGroup::EARLY_FAIL | LLSpatialGroup::DISCARD_QUERY) ||
			!mOccludedList[i]->isState(LLSpatialGroup::QUERY_OUT))
		{
			mOccludedList[i]->clearState(LLSpatialGroup::EARLY_FAIL);
			mOccludedList[i]->clearState(LLSpatialGroup::DISCARD_QUERY);
			res = 1;
		}
		else
		{	
			glGetQueryObjectuivARB(mOcclusionQueries[i], GL_QUERY_RESULT_ARB, &res);	
			stop_glerror();
		}
		
		if (res) //NOT OCCLUDED
		{	
			if (mOccludedList[i]->isState(LLSpatialGroup::OCCLUDED))
			{	//this node was occluded last frame
				LLSpatialGroup::OctreeNode* node = mOccludedList[i]->mOctreeNode;
				//add any immediate children to the queue that are not already there
				for (U32 j = 0; j < node->getChildCount(); j++)
				{
					LLSpatialGroup* group = (LLSpatialGroup*) node->getChild(j)->getListener(0);
					checkOcclusion(group, camera);
				}
			}
			
			//clear occlusion status for everything not on the active list
			LLOctreeClearOccludedNotActive clear_occluded;
			mOccludedList[i]->setState(LLSpatialGroup::DEACTIVATE_OCCLUSION);
			mOccludedList[i]->clearState(LLSpatialGroup::OCCLUDED);
			mOccludedList[i]->clearState(LLSpatialGroup::OCCLUDING);
			clear_occluded.traverse(mOccludedList[i]->mOctreeNode);
		}
		else
		{ //OCCLUDED
			if (mOccludedList[i]->isState(LLSpatialGroup::OCCLUDING))
			{
				if (!mOccludedList[i]->isState(LLSpatialGroup::OCCLUDED))
				{
					LLSpatialGroup::OctreeNode* oct_parent = (LLSpatialGroup::OctreeNode*) mOccludedList[i]->mOctreeNode->getParent();
					if (oct_parent)
					{
						LLSpatialGroup* parent = (LLSpatialGroup*) oct_parent->getListener(0);
						
						if (checkOcclusion(parent, camera))
						{	//force a guess on the parent and siblings		
							for (U32 i = 0; i < parent->mOctreeNode->getChildCount(); i++)
							{
								LLSpatialGroup* child = (LLSpatialGroup*) parent->mOctreeNode->getChild(i)->getListener(0);
								checkOcclusion(child, camera);	
							}
						}
					}

					//take children off the active list
					mOccludedList[i]->setState(LLSpatialGroup::DEACTIVATE_OCCLUSION, LLSpatialGroup::STATE_MODE_BRANCH);
					mOccludedList[i]->clearState(LLSpatialGroup::DEACTIVATE_OCCLUSION);
				}
				mOccludedList[i]->setState(LLSpatialGroup::OCCLUDED, LLSpatialGroup::STATE_MODE_DIFF);
			}
			else
			{
				//take children off the active list
				mOccludedList[i]->setState(LLSpatialGroup::DEACTIVATE_OCCLUSION, LLSpatialGroup::STATE_MODE_BRANCH);
							
				//keep this node on the active list
				mOccludedList[i]->clearState(LLSpatialGroup::DEACTIVATE_OCCLUSION);

				//this node is a top level occluder
				mOccludedList[i]->setState(LLSpatialGroup::OCCLUDING);
			}
		}

		mOccludedList[i]->clearState(LLSpatialGroup::QUERY_OUT);
	}

	//remove non-occluded groups from occluded list
	for (U32 i = 0; i < mOccludedList.size(); )
	{
		if (mOccludedList[i]->isDead() || //needs to be deleted
			!mOccludedList[i]->isState(LLSpatialGroup::OCCLUDING) || //is not occluding
			mOccludedList[i]->isState(LLSpatialGroup::DEACTIVATE_OCCLUSION)) //parent is occluded
		{
			LLSpatialGroup* groupp = mOccludedList[i];
			if (!groupp->isDead())
			{
				groupp->clearState(LLSpatialGroup::ACTIVE_OCCLUSION);
				groupp->clearState(LLSpatialGroup::DEACTIVATE_OCCLUSION);
				groupp->clearState(LLSpatialGroup::OCCLUDING);
			}
			mOccludedList.erase(mOccludedList.begin()+i);
		}
		else
		{
			i++;
		}
	}

	validate_occlusion_list(mOccludedList);

	//pump some non-culled items onto the occlusion list
	//count = MAX_PULLED;
	while (!mQueryQueue.empty())
	{
		LLPointer<LLSpatialGroup> group = mQueryQueue.front();
		mQueryQueue.pop();
        //group->clearState(LLSpatialGroup::QUERY_PENDING);
		mOccludedList.push_back(group);
	}

	//generate query ids
	while (mOcclusionQueries.size() < mOccludedList.size())
	{
		GLuint id;
		glGenQueriesARB(1, &id);
		mOcclusionQueries.push_back(id);
	}
}

class LLOcclusionIndexBuffer : public LLVertexBuffer
{
public:
	LLOcclusionIndexBuffer(U32 size)
		: LLVertexBuffer(0, GL_STREAM_DRAW_ARB)
	{
		allocateBuffer(0, size, TRUE);

		LLStrider<U32> idx;

		getIndexStrider(idx);

		//12 triangles' indices
		idx[0] = 1; idx[1] = 0; idx[2] = 2; //front
		idx[3] = 3; idx[4] = 2; idx[5] = 0;

		idx[6] = 4; idx[7] = 5; idx[8] = 1; //top
		idx[9] = 0; idx[10] = 1; idx[11] = 5; 

		idx[12] = 5; idx[13] = 4; idx[14] = 6; //back
		idx[15] = 7; idx[16] = 6; idx[17] = 4;

		idx[18] = 6; idx[19] = 7; idx[20] = 3; //bottom
		idx[21] = 2; idx[22] = 3; idx[23] = 7;

		idx[24] = 0; idx[25] = 5; idx[26] = 3; //left
		idx[27] = 6; idx[28] = 3; idx[29] = 5;

		idx[30] = 4; idx[31] = 1; idx[32] = 7; //right
		idx[33] = 2; idx[34] = 7; idx[35] = 1;
	}

	//virtual BOOL useVBOs() const { return FALSE; }

	void setBuffer(U32 data_mask)
	{
		if (useVBOs())
		{
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mGLIndices);
			sIBOActive = TRUE;
			unmapBuffer();
		}
		else if (sIBOActive)
		{
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
			sIBOActive = FALSE;
		}
		
		sGLRenderIndices = mGLIndices;
	}
};

class LLOcclusionVertexBuffer : public LLVertexBuffer
{
public:
	LLOcclusionVertexBuffer(S32 usage)
		: LLVertexBuffer(MAP_VERTEX, usage)
	{
		allocateBuffer(8, 0, TRUE);
	}

	//virtual BOOL useVBOs() const { return FALSE; }

	void setBuffer(U32 data_mask)
	{
		if (useVBOs())
		{
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, mGLBuffer);
			sVBOActive = TRUE;
			unmapBuffer();
		}
		else if (sVBOActive)
		{
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
			sVBOActive = FALSE;
		}
		
		if (data_mask)
		{
			glVertexPointer(3,GL_FLOAT, 0, useVBOs() ? 0 : mMappedData);
		}
		
		sGLRenderBuffer = mGLBuffer;
	}
};

void LLSpatialPartition::buildOcclusion()
{
	if (mOccludedList.empty())
	{
		return;
	}

	BOOL reset_all = FALSE;
	if (mOcclusionIndices.isNull())
	{
		mOcclusionIndices = new LLOcclusionIndexBuffer(36);
		reset_all = TRUE;
	}
	
	//fill occlusion vertex buffers
	for (U32 i = 0; i < mOccludedList.size(); i++)
	{
		LLSpatialGroup* group = mOccludedList[i];

		if (group->isState(LLSpatialGroup::OCCLUSION_DIRTY) || reset_all)
		{
			LLFastTimer ftm(LLFastTimer::FTM_REBUILD_OCCLUSION_VB);

			if (group->mOcclusionVerts.isNull())
			{
				group->mOcclusionVerts = new LLOcclusionVertexBuffer(GL_STREAM_DRAW_ARB);
			}
	
			group->clearState(LLSpatialGroup::OCCLUSION_DIRTY);
			
			LLStrider<LLVector3> vert;

			group->mOcclusionVerts->getVertexStrider(vert);

			LLVector3 r = group->mBounds[1]*SG_OCCLUSION_FUDGE + LLVector3(0.1f,0.1f,0.1f);

			for (U32 k = 0; k < 3; k++)
			{
				r.mV[k] = llmin(group->mBounds[1].mV[k]+0.25f, r.mV[k]);
			}

			*vert++ = group->mBounds[0] + r.scaledVec(LLVector3(-1,1,1)); //   0 - left top front
			*vert++ = group->mBounds[0] + r.scaledVec(LLVector3(1,1,1));  //   1 - right top front
			*vert++ = group->mBounds[0] + r.scaledVec(LLVector3(1,-1,1)); //   2 - right bottom front
			*vert++ = group->mBounds[0] + r.scaledVec(LLVector3(-1,-1,1)); //  3 - left bottom front

			*vert++ = group->mBounds[0] + r.scaledVec(LLVector3(1,1,-1)); //  4 - left top back
			*vert++ = group->mBounds[0] + r.scaledVec(LLVector3(-1,1,-1)); //   5 - right top back
			*vert++ = group->mBounds[0] + r.scaledVec(LLVector3(-1,-1,-1)); //  6 - right bottom back
			*vert++ = group->mBounds[0] + r.scaledVec(LLVector3(1,-1,-1)); // 7 -left bottom back
		}
	}

/*	for (U32 i = 0; i < mOccludedList.size(); i++)
	{
		LLSpatialGroup* group = mOccludedList[i];
		if (!group->mOcclusionVerts.isNull() && group->mOcclusionVerts->isLocked())
		{
			LLFastTimer ftm(LLFastTimer::FTM_REBUILD_OCCLUSION_VB);
			group->mOcclusionVerts->setBuffer(0);
		}
	}*/
}

void LLSpatialPartition::doOcclusion(LLCamera* camera)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);

	LLFastTimer t(LLFastTimer::FTM_RENDER_OCCLUSION);

#if LL_OCTREE_PARANOIA_CHECK  
	LLSpatialGroup* check = (LLSpatialGroup*) mOctree->getListener(0);
	check->validate();
#endif

	stop_glerror();
	
	U32 num_verts = mOccludedList.size() * 8;

	if (num_verts == 0)
	{
		return;
	}

	//actually perform the occlusion queries
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
	LLGLDisable(GL_TEXTURE_2D);
	gPipeline.disableLights();
	LLGLEnable cull_face(GL_CULL_FACE);
	LLGLDisable blend(GL_BLEND);
	LLGLDisable alpha_test(GL_ALPHA_TEST);
	LLGLDisable fog(GL_FOG);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glColor4f(1,1,1,1);

	mOcclusionIndices->setBuffer(0);

	U32* indicesp = (U32*) mOcclusionIndices->getIndicesPointer();

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
#if !LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkClientArrays(LLVertexBuffer::MAP_VERTEX);
#endif
	for (U32 i = 0; i < mOccludedList.size(); i++)
	{
#if LL_OCTREE_PARANOIA_CHECK
		for (U32 j = i+1; j < mOccludedList.size(); j++)
		{
			sg_assert(mOccludedList[i] != mOccludedList[j]);
		}
#endif
		LLSpatialGroup* group = mOccludedList[i];
		if (group->isDead())
		{
			continue;
		}

		if (earlyFail(camera, group))
		{
			group->setState(LLSpatialGroup::EARLY_FAIL);
		}
		else
		{ //early rejection criteria passed, send some geometry to the query
			group->mOcclusionVerts->setBuffer(LLVertexBuffer::MAP_VERTEX);
			glBeginQueryARB(GL_SAMPLES_PASSED_ARB, mOcclusionQueries[i]);					
			glDrawRangeElements(GL_TRIANGLES, 0, 7, 36,
							GL_UNSIGNED_INT, indicesp);
			glEndQueryARB(GL_SAMPLES_PASSED_ARB);

			group->setState(LLSpatialGroup::QUERY_OUT);
			group->clearState(LLSpatialGroup::DISCARD_QUERY);
		}		
	}
	stop_glerror();
	
	gPipeline.mTrianglesDrawn += mOccludedList.size()*12;

	glFlush();

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
}

class LLOctreeGet : public LLSpatialGroup::OctreeTraveler
{
public:
	LLOctreeGet(LLVector3 pos, F32 rad, LLDrawable::drawable_set_t* results, BOOL lights)
		: mPosition(pos), mRad(rad), mResults(results), mLights(lights), mRes(0)
	{

	}

	virtual void traverse(const LLSpatialGroup::TreeNode* n)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) n->getListener(0);

		if (mRes == 2) 
		{ //fully in, just add everything
			LLSpatialGroup::OctreeTraveler::traverse(n);
		}
		else
		{
			LLVector3 center, size;
			
			center = group->mBounds[0];
			size = group->mBounds[1];
						
			mRes = LLSphereAABB(center, size, mPosition, mRad);		
			if (mRes > 0)
			{
				LLSpatialGroup::OctreeTraveler::traverse(n);
			}
			mRes = 0;
		}
	}

	static BOOL skip(LLDrawable* drawable, BOOL get_lights)
	{
		if (get_lights != drawable->isLight())
		{
			return TRUE;
		}
		if (get_lights && drawable->getVObj()->isHUDAttachment())
		{
			return TRUE; // no lighting from HUD objects
		}
		if (get_lights && drawable->isState(LLDrawable::ACTIVE))
		{
			return TRUE; // ignore active lights
		}
		return FALSE;
	}

	virtual void visit(const LLSpatialGroup::OctreeState* branch) 
	{ 
		for (LLSpatialGroup::OctreeState::const_element_iter i = branch->getData().begin(); i != branch->getData().end(); ++i)
		{
			LLDrawable* drawable = *i;
			if (!skip(drawable, mLights))
			{
				if (mRes == 2)
				{
					mResults->insert(drawable);
				}
				else
				{
					LLVector3 v = LLVector3(drawable->getPositionGroup())-mPosition;
					float dsq = v.magVecSquared();
					float maxd = mRad + drawable->getVisibilityRadius();
					if (dsq <= maxd*maxd)
					{
						mResults->insert(drawable);
					}
				}
			}
		}
	}

	LLVector3 mPosition;
	F32 mRad;
	LLDrawable::drawable_set_t* mResults;
	BOOL mLights;
	U32 mRes;
};

S32 LLSpatialPartition::getDrawables(const LLVector3& pos, F32 rad,
									 LLDrawable::drawable_set_t &results,
									 BOOL get_lights)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	
	LLOctreeGet getter(pos, rad, &results, get_lights);
	getter.traverse(mOctree);

	return results.size();
}

S32 LLSpatialPartition::getObjects(const LLVector3& pos, F32 rad, LLDrawable::drawable_set_t &results)
{
	LLSpatialGroup* group = (LLSpatialGroup*) mOctree->getListener(0);
	group->rebound();
	return getDrawables(pos, rad, results, FALSE);
}

S32 LLSpatialPartition::getLights(const LLVector3& pos, F32 rad, LLDrawable::drawable_set_t &results)
{
	return getDrawables(pos, rad, results, TRUE);
}

void pushVerts(LLDrawInfo* params, U32 mask)
{
	params->mVertexBuffer->setBuffer(mask);
	U32* indicesp = (U32*) params->mVertexBuffer->getIndicesPointer();
	glDrawRangeElements(params->mParticle ? GL_POINTS : GL_TRIANGLES, params->mStart, params->mEnd, params->mCount,
					GL_UNSIGNED_INT, indicesp+params->mOffset);
}

void pushVerts(LLSpatialGroup* group, U32 mask)
{
	LLDrawInfo* params = NULL;

	for (LLSpatialGroup::draw_map_t::iterator i = group->mDrawMap.begin(); i != group->mDrawMap.end(); ++i)
	{
		for (LLSpatialGroup::drawmap_elem_t::iterator j = i->second.begin(); j != i->second.end(); ++j) 
		{
			params = *j;
			pushVerts(params, mask);
		}
	}
}

void pushVertsColorCoded(LLSpatialGroup* group, U32 mask)
{
	LLDrawInfo* params = NULL;

	LLColor4 colors[] = {
		LLColor4::green,
		LLColor4::green1,
		LLColor4::green2,
		LLColor4::green3,
		LLColor4::green4,
		LLColor4::green5,
		LLColor4::green6
	};
		
	static const U32 col_count = sizeof(colors)/sizeof(LLColor4);

	U32 col = 0;

	for (LLSpatialGroup::draw_map_t::iterator i = group->mDrawMap.begin(); i != group->mDrawMap.end(); ++i)
	{
		for (LLSpatialGroup::drawmap_elem_t::iterator j = i->second.begin(); j != i->second.end(); ++j) 
		{
			params = *j;
			glColor4f(colors[col].mV[0], colors[col].mV[1], colors[col].mV[2], 0.5f);
			params->mVertexBuffer->setBuffer(mask);
			U32* indicesp = (U32*) params->mVertexBuffer->getIndicesPointer();
			glDrawRangeElements(params->mParticle ? GL_POINTS : GL_TRIANGLES, params->mStart, params->mEnd, params->mCount,
							GL_UNSIGNED_INT, indicesp+params->mOffset);
			col = (col+1)%col_count;
		}
	}
}

void renderOctree(LLSpatialGroup* group)
{
	//render solid object bounding box, color
	//coded by buffer usage and activity
	LLGLDepthTest depth(GL_TRUE, GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	LLVector4 col;
	if (group->mBuilt > 0.f)
	{
		group->mBuilt -= 2.f * gFrameIntervalSeconds;
		if (group->mBufferUsage == GL_STATIC_DRAW_ARB)
		{
			col.setVec(1.0f, 0, 0, group->mBuilt*0.5f);
		}
		else 
		{
			col.setVec(0.1f,0.1f,1,0.1f);
			//col.setVec(1.0f, 1.0f, 0, sinf(group->mBuilt*3.14159f)*0.5f);
		}

		if (group->mBufferUsage != GL_STATIC_DRAW_ARB)
		{
			if (group->mBufferUsage == GL_DYNAMIC_DRAW_ARB)
			{
				glColor4f(1,0,0,group->mBuilt);
			}
			else
			{
				glColor4f(1,1,0,group->mBuilt);
			}

			LLGLDepthTest gl_depth(FALSE, FALSE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			for (LLSpatialGroup::element_iter i = group->getData().begin(); i != group->getData().end(); ++i)
			{
				LLDrawable* drawable = *i;
				for (S32 j = 0; j < drawable->getNumFaces(); j++)
				{
					LLFace* face = drawable->getFace(j);
					if (gFrameTimeSeconds - face->mLastUpdateTime < 0.5f && face->mVertexBuffer.notNull())
					{ 
						face->mVertexBuffer->setBuffer(LLVertexBuffer::MAP_VERTEX);
						//drawBox((face->mExtents[0] + face->mExtents[1])*0.5f,
						//		(face->mExtents[1]-face->mExtents[0])*0.5f);
						glDrawElements(GL_TRIANGLES, face->getIndicesCount(), GL_UNSIGNED_INT, 
							((U32*) face->mVertexBuffer->getIndicesPointer())+face->getIndicesStart());
					}
				}
			}
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}
	else
	{
		if (group->mBufferUsage == GL_STATIC_DRAW_ARB && !group->getData().empty() 
			&& group->mSpatialPartition->mRenderByGroup)
		{
			col.setVec(0.8f, 0.4f, 0.1f, 0.1f);
		}
		else
		{
			col.setVec(0.1f, 0.1f, 1.f, 0.1f);
		}
	}

	glColor4fv(col.mV);
	drawBox(group->mObjectBounds[0], group->mObjectBounds[1]*1.01f+LLVector3(0.001f, 0.001f, 0.001f));
	glDepthMask(GL_TRUE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//draw opaque outline
	glColor4f(col.mV[0], col.mV[1], col.mV[2], 1.f);
	drawBoxOutline(group->mObjectBounds[0], group->mObjectBounds[1]);

	if (group->mOctreeNode->hasLeafState())
	{
		glColor4f(1,1,1,1);
	}
	else
	{
		glColor4f(0,1,1,1);
	}
					
	drawBoxOutline(group->mBounds[0],group->mBounds[1]);
	
//	LLSpatialGroup::OctreeNode* node = group->mOctreeNode;
//	glColor4f(0,1,0,1);
//	drawBoxOutline(LLVector3(node->getCenter()), LLVector3(node->getSize()));
}

void renderVisibility(LLSpatialGroup* group)
{
	LLGLEnable blend(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	LLGLEnable cull(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	{
		LLGLDepthTest depth_under(GL_TRUE, GL_FALSE, GL_GREATER);
		glColor4f(0, 0.5f, 0, 0.5f);
		pushVerts(group, LLVertexBuffer::MAP_VERTEX);
	}

	{
		LLGLDepthTest depth_over(GL_TRUE, GL_FALSE, GL_LEQUAL);
		pushVertsColorCoded(group, LLVertexBuffer::MAP_VERTEX);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		pushVertsColorCoded(group, LLVertexBuffer::MAP_VERTEX);
	}
}

void renderBoundingBox(LLDrawable* drawable)
{
	if (drawable->isSpatialBridge())
	{
		glColor4f(1,0.5f,0,1);
	}
	else if (drawable->getVOVolume())
	{
		if (drawable->isRoot())
		{
			glColor4f(1,1,0,1);
		}
		else
		{
			glColor4f(0,1,0,1);
		}
	}
	else if (drawable->getVObj())
	{
		switch (drawable->getVObj()->getPCode())
		{
			case LLViewerObject::LL_VO_SURFACE_PATCH:
					glColor4f(0,1,1,1);
					break;
			case LLViewerObject::LL_VO_CLOUDS:
					glColor4f(0.5f,0.5f,0.5f,1.0f);
					break;
			case LLViewerObject::LL_VO_PART_GROUP:
					glColor4f(0,0,1,1);
					break;
			case LLViewerObject::LL_VO_WATER:
					glColor4f(0,0.5f,1,1);
					break;
			case LL_PCODE_LEGACY_TREE:
					glColor4f(0,0.5f,0,1);
			default:
					glColor4f(1,0,1,1);
					break;
		}
	}
	else 
	{
		glColor4f(1,0,0,1);
	}

	const LLVector3* ext;
	LLVector3 pos, size;

	//render face bounding boxes
	for (S32 i = 0; i < drawable->getNumFaces(); i++)
	{
		LLFace* facep = drawable->getFace(i);

		ext = facep->mExtents;

		if (ext[0].isExactlyZero() && ext[1].isExactlyZero())
		{
			continue;
		}
		pos = (ext[0] + ext[1]) * 0.5f;
		size = (ext[1] - ext[0]) * 0.5f;
		drawBoxOutline(pos,size);
	}

	//render drawable bounding box
	ext = drawable->getSpatialExtents();

	pos = (ext[0] + ext[1]) * 0.5f;
	size = (ext[1] - ext[0]) * 0.5f;
	
	drawBoxOutline(pos,size);
}

void renderTexturePriority(LLDrawable* drawable)
{
	for (int face=0; face<drawable->getNumFaces(); ++face)
	{
		LLFace *facep = drawable->getFace(face);
		
		LLVector4 cold(0,0,0.25f);
		LLVector4 hot(1,0.25f,0.25f);
	
		LLVector4 boost_cold(0,0,0,0);
		LLVector4 boost_hot(0,1,0,1);
		
		LLGLDisable blend(GL_BLEND);
		
		//LLViewerImage* imagep = facep->getTexture();
		//if (imagep)
		{
	
			//F32 vsize = LLVOVolume::getTextureVirtualSize(facep);
			//F32 vsize = imagep->mMaxVirtualSize;
			F32 vsize = facep->getPixelArea();

			if (vsize > sCurMaxTexPriority)
			{
				sCurMaxTexPriority = vsize;
			}
			
			F32 t = vsize/sLastMaxTexPriority;
			
			LLVector4 col = lerp(cold, hot, t);
			glColor4fv(col.mV);
		}
		//else
		//{
		//	glColor4f(1,0,1,1);
		//}

		
		
		LLVector3 center = (facep->mExtents[1]+facep->mExtents[0])*0.5f;
		LLVector3 size = (facep->mExtents[1]-facep->mExtents[0])*0.5f + LLVector3(0.01f, 0.01f, 0.01f);
		drawBox(center, size);
		
		/*S32 boost = imagep->getBoostLevel();
		if (boost)
		{
			F32 t = (F32) boost / (F32) (LLViewerImage::BOOST_MAX_LEVEL-1);
			LLVector4 col = lerp(boost_cold, boost_hot, t);
			LLGLEnable blend_on(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glColor4fv(col.mV);
			drawBox(center, size);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}*/
	}
}

void renderPoints(LLDrawable* drawablep)
{
	LLGLDepthTest depth(GL_FALSE, GL_FALSE);
	glBegin(GL_POINTS);
	glColor3f(1,1,1);
	LLVector3 center(drawablep->getPositionGroup());
	for (S32 i = 0; i < drawablep->getNumFaces(); i++)
	{
		glVertex3fv(drawablep->getFace(i)->mCenterLocal.mV);
	}
	glEnd();
}

void renderTextureAnim(LLDrawInfo* params)
{
	if (!params->mTextureMatrix)
	{
		return;
	}
	
	LLGLEnable blend(GL_BLEND);
	glColor4f(1,1,0,0.5f);
	pushVerts(params, LLVertexBuffer::MAP_VERTEX);
}

class LLOctreeRenderNonOccluded : public LLOctreeTraveler<LLDrawable>
{
public:
	LLOctreeRenderNonOccluded() {}
	
	virtual void traverse(const LLSpatialGroup::OctreeNode* node)
	{
		const LLSpatialGroup::OctreeState* state = node->getOctState();
		LLSpatialGroup* group = (LLSpatialGroup*) node->getListener(0);
		
	
		if ((!gPipeline.sUseOcclusion || !group->isState(LLSpatialGroup::OCCLUDED)) &&
			!group->isState(LLSpatialGroup::CULLED))
		{
			state->accept(this);

			for (U32 i = 0; i < state->getChildCount(); i++)
			{
				traverse(state->getChild(i));
			}
			
			//draw tight fit bounding boxes for spatial group
			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCTREE))
			{	
				renderOctree(group);
			}

			//render visibility wireframe
			if (group->mSpatialPartition->mRenderByGroup &&
				gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCCLUSION) &&
				!group->isState(LLSpatialGroup::GEOM_DIRTY))
			{
				renderVisibility(group);
			}
		}
	}

	virtual void visit(const LLSpatialGroup::OctreeState* branch)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) branch->getListener(0);

		if (group->isState(LLSpatialGroup::CULLED | LLSpatialGroup::OCCLUDED))
		{
			return;
		}

		LLVector3 nodeCenter = group->mBounds[0];
		LLVector3 octCenter = LLVector3(group->mOctreeNode->getCenter());

		for (LLSpatialGroup::OctreeState::const_element_iter i = branch->getData().begin(); i != branch->getData().end(); ++i)
		{
			LLDrawable* drawable = *i;
						
			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_BBOXES))
			{
				renderBoundingBox(drawable);			
			}
			
			if (drawable->getVOVolume() && gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY))
			{
				renderTexturePriority(drawable);
			}

			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_POINTS))
			{
				renderPoints(drawable);
			}
		}
		
		for (LLSpatialGroup::draw_map_t::iterator i = group->mDrawMap.begin(); i != group->mDrawMap.end(); ++i)
		{
			LLSpatialGroup::drawmap_elem_t& draw_vec = i->second;	
			for (LLSpatialGroup::drawmap_elem_t::iterator j = draw_vec.begin(); j != draw_vec.end(); ++j)	
			{
				LLDrawInfo* draw_info = *j;
				if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_ANIM))
				{
					renderTextureAnim(draw_info);
				}
			}
		}
	}
};

void LLSpatialPartition::renderDebug()
{
	if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCTREE |
									  LLPipeline::RENDER_DEBUG_OCCLUSION |
									  LLPipeline::RENDER_DEBUG_BBOXES |
									  LLPipeline::RENDER_DEBUG_POINTS |
									  LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY |
									  LLPipeline::RENDER_DEBUG_TEXTURE_ANIM))
	{
		return;
	}
	
	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY))
	{
		//sLastMaxTexPriority = lerp(sLastMaxTexPriority, sCurMaxTexPriority, gFrameIntervalSeconds);
		sLastMaxTexPriority = (F32) gCamera->getScreenPixelArea();
		sCurMaxTexPriority = 0.f;
	}

	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	
	LLGLDisable cullface(GL_CULL_FACE);
	LLGLEnable blend(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	LLGLDisable tex(GL_TEXTURE_2D);
	gPipeline.disableLights();
		
	LLOctreeRenderNonOccluded render_debug;
	render_debug.traverse(mOctree);

	LLGLDisable cull_face(GL_CULL_FACE);
	
	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCCLUSION) && !mOccludedList.empty() &&
		mOcclusionIndices.notNull())
	{
		LLGLDisable fog(GL_FOG);
		LLGLDepthTest gls_depth(GL_FALSE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		mOcclusionIndices->setBuffer(0);
		U32* indicesp = (U32*) mOcclusionIndices->getIndicesPointer();

		LLGLEnable blend(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		LLGLEnable cull(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		for (U32 i = 0; i < mOccludedList.size(); i++)
		{	//draw occluded nodes
			LLSpatialGroup* node = mOccludedList[i];
			if (node->isDead() ||
				!node->isState(LLSpatialGroup::OCCLUDED) ||
				node->mOcclusionVerts.isNull())
			{
				continue;
			}
			
			node->mOcclusionVerts->setBuffer(LLVertexBuffer::MAP_VERTEX);
			{
				LLGLDepthTest depth_under(GL_TRUE, GL_FALSE, GL_GREATER);
				glColor4f(0.5, 0.5f, 0, 0.25f);
				glDrawRangeElements(GL_TRIANGLES, 0, 7, 36,
							GL_UNSIGNED_INT, indicesp);
			}

			{
				LLGLDepthTest depth_over(GL_TRUE, GL_FALSE, GL_LEQUAL);
				glColor4f(0.0,1.0f,1.0f,1.0f);
				glDrawRangeElements(GL_TRIANGLES, 0, 7, 36,
							GL_UNSIGNED_INT, indicesp);
			}
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}	
}


BOOL LLSpatialPartition::isVisible(const LLVector3& v)
{
	if (!gCamera->sphereInFrustum(v, 4.0f))
	{
		return FALSE;
	}

	return TRUE;
}

class LLOctreePick : public LLSpatialGroup::OctreeTraveler
{
public:
	LLVector3 mStart;
	LLVector3 mEnd;
	LLDrawable* mRet;
	
	LLOctreePick(LLVector3 start, LLVector3 end)
	: mStart(start), mEnd(end)
	{
		mRet = NULL;
	}

	virtual LLDrawable* check(const LLSpatialGroup::OctreeNode* node)
	{
		const LLSpatialGroup::OctreeState* state = node->getOctState();
		state->accept(this);
	
		for (U32 i = 0; i < node->getChildCount(); i++)
		{
			const LLSpatialGroup::OctreeNode* child = node->getChild(i);
			LLVector3 res;

			LLSpatialGroup* group = (LLSpatialGroup*) child->getListener(0);
			
			LLVector3 size;
			LLVector3 center;
			
			size = group->mBounds[1];
			center = group->mBounds[0];
			
			if (LLLineSegmentAABB(mStart, mEnd, center, size))
			{
				check(child);
			}
		}	

		return mRet;
	}

	virtual void visit(const LLSpatialGroup::OctreeState* branch) 
	{	
		for (LLSpatialGroup::OctreeState::const_element_iter i = branch->getData().begin(); i != branch->getData().end(); ++i)
		{
			check(*i);
		}
	}

	virtual bool check(LLDrawable* drawable)
	{
		LLViewerObject* vobj = drawable->getVObj();
		if (vobj->lineSegmentIntersect(mStart, mEnd))
		{
			mRet = vobj->mDrawable;
		}
		
		return false;
	}
};

LLDrawable*	LLSpatialPartition::pickDrawable(const LLVector3& start, const LLVector3& end, LLVector3& collision)
{
	LLOctreePick pick(start, end);
	LLDrawable* ret = pick.check(mOctree);
	collision.setVec(pick.mEnd);
	return ret;
}

LLDrawInfo::LLDrawInfo(U32 start, U32 end, U32 count, U32 offset, 
					   LLViewerImage* texture, LLVertexBuffer* buffer,
					   BOOL fullbright, U8 bump, BOOL particle, F32 part_size)
:
	mVertexBuffer(buffer),
	mTexture(texture),
	mTextureMatrix(NULL),
	mStart(start),
	mEnd(end),
	mCount(count),
	mOffset(offset), 
	mFullbright(fullbright),
	mBump(bump),
	mParticle(particle),
	mPartSize(part_size),
	mVSize(0.f)
{
}

LLDrawInfo::~LLDrawInfo()	
{

}

LLVertexBuffer* LLGeometryManager::createVertexBuffer(U32 type_mask, U32 usage)
{
	return new LLVertexBuffer(type_mask, usage);
}

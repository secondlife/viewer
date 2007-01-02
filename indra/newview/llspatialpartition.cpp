/** 
 * @file llspatialpartition.cpp
 * @brief LLSpatialGroup class implementation and supporting functions
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llspatialpartition.h"

#include "llglheaders.h"

#include "llviewerobjectlist.h"
#include "llvovolume.h"
#include "llviewercamera.h"
#include "llface.h"
#include "viewer.h"

#include "llcamera.h"
#include "pipeline.h"

static BOOL sIgnoreOcclusion = TRUE;
static GLuint sBoxList = 0;

const S32 SG_LOD_SWITCH_STAGGER = 4;
const F32 SG_MAX_OBJ_RAD = 1.f;
const F32 SG_OCCLUSION_FUDGE = 1.1f;
const S32 SG_MOVE_PERIOD = 32;
const S32 SG_LOD_PERIOD = 16;

#define SG_DISCARD_TOLERANCE 0.25f

#if LL_OCTREE_PARANOIA_CHECK
#define assert_octree_valid(x) x->validate()
#else
#define assert_octree_valid(x)
#endif

static F32 sLastMaxTexPriority = 1.f;
static F32 sCurMaxTexPriority = 1.f;

//static counter for frame to switch LOD on
S32 LLSpatialGroup::sLODSeed = 0;

void sg_assert(BOOL expr)
{
#if LL_OCTREE_PARANOIA_CHECK
	if (!expr)
	{
		llerrs << "Octree invalid!" << llendl;
	}
#endif
}

#if !LL_RELEASE_FOR_DOWNLOAD
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
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	if (!safeToDelete())
	{
#ifdef LL_RELEASE_FOR_DOWNLOAD
		llwarns << "Spatial Group deleted while being tracked " << ((void*) mState) << llendl;
#else
		llerrs << "Spatial Group deleted while being tracked " << ((void*) mState) << llendl;
#endif
	}

#if LL_OCTREE_PARANOIA_CHECK
	for (U32 i = 0; i < mSpatialPartition->mOccludedList.size(); i++)
	{
		if (mSpatialPartition->mOccludedList[i] == this)
		{
			llerrs << "Spatial Group deleted while being tracked STATE ERROR " << ((void*) mState) << llendl;
		}
	}
#endif
}

BOOL LLSpatialGroup::safeToDelete()
{
	return gQuit || !isState(IN_QUEUE | ACTIVE_OCCLUSION | RESHADOW_QUEUE);
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
	if (sIgnoreOcclusion)
	{
		return !isState(CULLED); 
	}
	else
	{
		return !isState(CULLED | OCCLUDED);
	}
}

void LLSpatialGroup::validate()
{
#if LL_OCTREE_PARANOIA_CHECK

	sg_assert(!isState(DIRTY));

	LLVector3 myMin = mBounds[0] - mBounds[1];
	LLVector3 myMax = mBounds[0] + mBounds[1];

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

BOOL LLSpatialGroup::updateInGroup(LLDrawable *drawablep, BOOL immediate)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
		
	drawablep->updateSpatialExtents();
	validate_drawable(drawablep);

	if (mOctreeNode->isInside(drawablep) && mOctreeNode->contains(drawablep))
	{
		unbound();
		setState(OBJECT_DIRTY);
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
		drawablep->setSpatialGroup(this, 0);
		validate_drawable(drawablep);
	}

	return TRUE;
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

BOOL LLSpatialGroup::removeObject(LLDrawable *drawablep, BOOL from_octree)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	unbound();
	if (!from_octree)
	{
		if (!mOctreeNode->remove(drawablep))
		{
			OCT_ERRS << "Could not remove drawable from spatial group" << llendl;
		}
	}
	else
	{
		drawablep->setSpatialGroup(NULL, -1);
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

LLSpatialGroup::LLSpatialGroup(OctreeNode* node, LLSpatialPartition* part)
: mState(0), mOctreeNode(node), mSpatialPartition(part)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	sg_assert(mOctreeNode->getListenerCount() == 0);
	mOctreeNode->addListener(this);
	setState(DIRTY);

	mBounds[0] = LLVector3(node->getCenter());
	mBounds[1] = LLVector3(node->getSize());

	sLODSeed = (sLODSeed+1)%SG_LOD_PERIOD;
	mLODHash = sLODSeed;
}

BOOL LLSpatialGroup::changeLOD()
{
	return LLDrawable::getCurrentFrame()%SG_LOD_PERIOD == mLODHash;
}

void LLSpatialGroup::handleInsertion(const TreeNode* node, LLDrawable* drawable)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	addObject(drawable, FALSE, TRUE);
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
	
	if (mOctreeNode)
	{
		OctreeState* state = mOctreeNode->getOctState();
		for (OctreeState::element_iter i = state->getData().begin(); i != state->getData().end(); ++i)
		{
			LLDrawable* drawable = *i;
			if (!drawable->isDead())
			{
				drawable->setSpatialGroup(NULL, -1);
			}
		}
	}
	
	if (safeToDelete())
	{
        delete this;
	}
	else
	{
		setState(DEAD);
		mOctreeNode = NULL;
	}
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
		(new LLSpatialGroup(child, mSpatialPartition))->setState(mState & SG_STATE_INHERIT_MASK);
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

BOOL LLSpatialGroup::rebound()
{
	if (!isState(DIRTY))
	{	//return TRUE if we're not empty
		return TRUE;
	}
	
	LLVector3 oldBounds[2];
	
	if (isState(QUERY_OUT))
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
	
	if (isState(QUERY_OUT))
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
	
	clearState(DIRTY);

	return TRUE;
}

//==============================================

LLSpatialPartition::LLSpatialPartition()
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	mOctree = new LLSpatialGroup::OctreeNode(LLVector3d(0,0,0), 
											LLVector3d(1,1,1), 
											new LLSpatialGroup::OctreeRoot(), NULL);
	new LLSpatialGroup(mOctree, this);
}


LLSpatialPartition::~LLSpatialPartition()
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	delete mOctree;
	mOctree = NULL;
}


LLSpatialGroup *LLSpatialPartition::put(LLDrawable *drawablep)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	const F32 MAX_MAG = 1000000.f*1000000.f; // 1 million
			
	if (drawablep->getPositionGroup().magVecSquared() > MAX_MAG)
	{
#ifndef LL_RELEASE_FOR_DOWNLOAD
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
	
	return (LLSpatialGroup*) node->getListener(0);
}

BOOL LLSpatialPartition::remove(LLDrawable *drawablep, LLSpatialGroup *curp)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	
	drawablep->setSpatialGroup(NULL, -1);

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
	
	if (curp && curp->mSpatialPartition != this)
	{
		//keep drawable from being garbage collected
		LLPointer<LLDrawable> ptr = drawablep;
		if (curp->mSpatialPartition->remove(drawablep, curp))
		{
			put(drawablep);
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

	put(drawablep);
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
	if (sIgnoreOcclusion)
	{
		return FALSE;
	}
	
	if (!group->isState(LLSpatialGroup::ACTIVE_OCCLUSION | LLSpatialGroup::OCCLUDED) &&
		!earlyFail(camera, group))
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
		if (mRes &&	//never occlusion cull the root node
			!sIgnoreOcclusion &&			//never occlusion cull selection
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
	}
	
	virtual void processDrawable(LLDrawable* drawable)
	{
		if (!drawable->isDead())
		{
			const LLVector3* extents = drawable->getSpatialExtents();
			
			F32 objRad = drawable->getRadius();
			objRad *= objRad;
			F32 distSqr = ((extents[0]+extents[1])*0.5f - mCamera->getOrigin()).magVecSquared();
			if (objRad/distSqr > SG_MIN_DIST_RATIO)
			{
				gPipeline.markNotCulled(drawable, *mCamera);
			}
		}		
	}
	
	virtual void visit(const LLSpatialGroup::OctreeState* branch) 
	{	
		LLSpatialGroup* group = (LLSpatialGroup*) branch->getListener(0);

		preprocess(group);
		
		if (checkObjects(branch, group))
		{
			for (LLSpatialGroup::OctreeState::const_element_iter i = branch->getData().begin(); i != branch->getData().end(); ++i)
			{
				processDrawable(*i);
			}
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

	virtual void processDrawable(LLDrawable* drawable)
	{
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
	
	std::vector<LLDrawable*>* mResults;
};


void drawBox(const LLVector3& c, const LLVector3& r)
{
	glPushMatrix();
	glTranslatef(c.mV[0], c.mV[1], c.mV[2]);
	glScalef(r.mV[0], r.mV[1], r.mV[2]);
	glCallList(sBoxList);
	glPopMatrix();
}

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
		LLOctreeCull culler(&camera);
		culler.traverse(mOctree);
	}
	
	sIgnoreOcclusion = !(gSavedSettings.getBOOL("UseOcclusion") && gGLManager.mHasOcclusionQuery);
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
	LLVector3 r = group->mBounds[1] * (SG_OCCLUSION_FUDGE*2.f) + LLVector3(0.01f,0.01f,0.01f);
    
	if (group->isState(LLSpatialGroup::CULLED) || !camera->AABBInFrustum(c, r))
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

		LLSpatialGroup* group = mOcclusionQueue.front();
		if (!group->isState(LLSpatialGroup::IN_QUEUE))
		{
			OCT_ERRS << "Spatial Group State Error.  Group in queue not tagged as such." << llendl;
		}

		mOcclusionQueue.pop();
		group->clearState(LLSpatialGroup::IN_QUEUE);

		if (group->isDead())
		{
			if (group->safeToDelete())
			{
				delete group;
			}
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

			if (!child->isState(LLSpatialGroup::OCCLUDED | LLSpatialGroup::CULLED)
				&& !child->isState(LLSpatialGroup::IN_QUEUE | LLSpatialGroup::ACTIVE_OCCLUSION))
			{
				child->setState(LLSpatialGroup::IN_QUEUE);
				mOcclusionQueue.push(child);
			}
		}

		/*if (group->isState(LLSpatialGroup::QUERY_PENDING))
		{ //already on the pending group, put it back
			group->setState(LLSpatialGroup::IN_QUEUE);
			mOcclusionQueue.push(group);
			pcount++;
			continue;
		}*/

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
			//group->setState(LLSpatialGroup::QUERY_PENDING);
			group->setState(LLSpatialGroup::ACTIVE_OCCLUSION);
			mQueryQueue.push(group);
			count++;
		}
	}

	//read back results from last frame
	for (U32 i = 0; i < mOccludedList.size(); i++)
	{
	 	LLFastTimer t(LLFastTimer::FTM_OCCLUSION_READBACK);

		if (mOccludedList[i]->isDead() || !mOccludedList[i]->isState(LLSpatialGroup::ACTIVE_OCCLUSION))
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
				}
				mOccludedList[i]->setState(LLSpatialGroup::OCCLUDED, LLSpatialGroup::STATE_MODE_DIFF);
			}
			else
			{
				//take children off the active list
				mOccludedList[i]->setState(LLSpatialGroup::DEACTIVATE_OCCLUSION, LLSpatialGroup::STATE_MODE_DIFF);
							
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
			mOccludedList.erase(mOccludedList.begin()+i);
			groupp->clearState(LLSpatialGroup::ACTIVE_OCCLUSION);
			groupp->clearState(LLSpatialGroup::DEACTIVATE_OCCLUSION);
			groupp->clearState(LLSpatialGroup::OCCLUDING);

			if (groupp->isDead() && groupp->safeToDelete())
			{
				delete groupp;
			}
		}
		else
		{
			i++;
		}
	}

	//pump some non-culled items onto the occlusion list
	//count = MAX_PULLED;
	while (!mQueryQueue.empty())
	{
		LLSpatialGroup* group = mQueryQueue.front();
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

void LLSpatialPartition::doOcclusion(LLCamera* camera)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);

	sIgnoreOcclusion = gUseWireframe;
 	LLFastTimer t(LLFastTimer::FTM_RENDER_OCCLUSION);

#if LL_OCTREE_PARANOIA_CHECK  
	LLSpatialGroup* check = (LLSpatialGroup*) mOctree->getListener(0);
	check->validate();
#endif

	stop_glerror();
	
	//actually perform the occlusion queries
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
	glDisable(GL_TEXTURE_2D);
	gPipeline.disableLights();
	LLGLEnable cull_face(GL_CULL_FACE);
	LLGLDisable blend(GL_BLEND);
	LLGLDisable alpha_test(GL_ALPHA_TEST);
	LLGLDisable fog(GL_FOG);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glColor4f(1,1,1,1);

	//sort occlusion queries front to back
	/*for (U32 i = 0; i < mOccludedList.size(); i++)
	{
		LLSpatialGroup* group = mOccludedList[i];
		
		LLVector3 v = group->mOctreeNode->getCenter()-camera->getOrigin();
		group->mDistance = v*v;
	}

	std::sort(mOccludedList.begin(), mOccludedList.end(), dist_greater());

	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT);
	LLGLEnable stencil(GL_STENCIL_TEST);
	glStencilFunc(GL_GREATER, 1, 0xFFFFFFFF);
	glStencilOp(GL_KEEP, GL_SET, GL_KEEP);*/

	genBoxList();

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
			LLVector3 c;
			LLVector3 r;
			
			sg_assert(!group->isState(LLSpatialGroup::DIRTY));

			c = group->mBounds[0];
			r = group->mBounds[1]*SG_OCCLUSION_FUDGE + LLVector3(0.01f,0.01f,0.01f);
			for (U32 k = 0; k < 3; k++)
			{
				r.mV[k] = llmin(group->mBounds[1].mV[k]+0.25f, r.mV[k]);
			}
			
#if LL_OCTREE_PARANOIA_CHECK
			LLVector3 e = camera->getOrigin();
			LLVector3 min = c - r;
			LLVector3 max = c + r;
			BOOL outside = FALSE;
			for (U32 j = 0; j < 3; j++)
			{
				outside = outside || (e.mV[j] < min.mV[j] || e.mV[j] > max.mV[j]);
			}
			sg_assert(outside);
#endif
					
			glBeginQueryARB(GL_SAMPLES_PASSED_ARB, mOcclusionQueries[i]);					
			drawBox(c,r);
			glEndQueryARB(GL_SAMPLES_PASSED_ARB);

			group->setState(LLSpatialGroup::QUERY_OUT);
			group->clearState(LLSpatialGroup::DISCARD_QUERY);
		}		
	}
	stop_glerror();
	
	glFlush();

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glEnable(GL_TEXTURE_2D);
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
	return getDrawables(pos, rad, results, FALSE);
}

S32 LLSpatialPartition::getLights(const LLVector3& pos, F32 rad, LLDrawable::drawable_set_t &results)
{
	return getDrawables(pos, rad, results, TRUE);
}

class LLOctreeRenderNonOccluded : public LLOctreeTraveler<LLDrawable>
{
public:
	LLOctreeRenderNonOccluded() {}
	
	virtual void traverse(const LLSpatialGroup::OctreeNode* node)
	{
		const LLSpatialGroup::OctreeState* state = node->getOctState();
		LLSpatialGroup* group = (LLSpatialGroup*) node->getListener(0);
		
		if (!group->isState(LLSpatialGroup::OCCLUDED | LLSpatialGroup::CULLED))
		{
			state->accept(this);

			for (U32 i = 0; i < state->getChildCount(); i++)
			{
				traverse(state->getChild(i));
			}
			
			/*if (state->getElementCount() == 0)
			{
				return;
			}*/

			//draw tight fit bounding box for spatial group
			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCTREE))
			{
				if (node->getElementCount() == 0)
				{
					return;
				}
				
				if (node->hasLeafState())
				{
					glColor4f(1,1,1,1);
				}
				else
				{
					glColor4f(0,1,1,1);
				}


				LLVector3 pos; 
				LLVector3 size;
				
				pos = group->mObjectBounds[0];
				size = group->mObjectBounds[1];
				
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

				LLVector3 nc = LLVector3(node->getCenter());
				LLVector3 ns = LLVector3(node->getSize());

				LLVector3 nv1 = ns.scaledVec(LLVector3( 1, 1,1));
				LLVector3 nv2 = ns.scaledVec(LLVector3(-1, 1,1));
				LLVector3 nv3 = ns.scaledVec(LLVector3(-1,-1,1));
				LLVector3 nv4 = ns.scaledVec(LLVector3( 1,-1,1));

				

				/*if (node->getElementCount() > 0)
				{
					//spokes
				    glColor4f(1,1,0,1);
					glVertex3fv(pos.mV);
					glColor4f(1,1,0,0);
					glVertex3fv(nc.mV);

					glColor4f(1,1,0,1); glVertex3fv((pos+v1).mV); glColor4f(1,1,0,0);	glVertex3fv(pos.mV);
					glColor4f(1,1,0,1); glVertex3fv((pos-v1).mV); glColor4f(1,1,0,0);	glVertex3fv(pos.mV);
					glColor4f(1,1,0,1); glVertex3fv((pos+v2).mV); glColor4f(1,1,0,0);	glVertex3fv(pos.mV);
					glColor4f(1,1,0,1); glVertex3fv((pos-v2).mV); glColor4f(1,1,0,0);	glVertex3fv(pos.mV);
					glColor4f(1,1,0,1); glVertex3fv((pos+v3).mV); glColor4f(1,1,0,0);	glVertex3fv(pos.mV);
					glColor4f(1,1,0,1); glVertex3fv((pos-v3).mV); glColor4f(1,1,0,0);	glVertex3fv(pos.mV);
					glColor4f(1,1,0,1); glVertex3fv((pos+v4).mV); glColor4f(1,1,0,0);	glVertex3fv(pos.mV);
					glColor4f(1,1,0,1); glVertex3fv((pos-v4).mV); glColor4f(1,1,0,0);	glVertex3fv(pos.mV);
				}*/

				

				/*glColor4f(0,1,0,1);
				glBegin(GL_LINE_LOOP); //top
				glVertex3fv((nc+nv1).mV);
				glVertex3fv((nc+nv2).mV);
				glVertex3fv((nc+nv3).mV);
				glVertex3fv((nc+nv4).mV);
				glEnd();

				glBegin(GL_LINE_LOOP); //bottom
				glVertex3fv((nc-nv1).mV);
				glVertex3fv((nc-nv2).mV);
				glVertex3fv((nc-nv3).mV);
				glVertex3fv((nc-nv4).mV);
				glEnd();


				glBegin(GL_LINES);

				//right
				glVertex3fv((nc+nv1).mV);
				glVertex3fv((nc-nv3).mV);
						
				glVertex3fv((nc+nv4).mV);
				glVertex3fv((nc-nv2).mV);

				//left
				glVertex3fv((nc+nv2).mV);
				glVertex3fv((nc-nv4).mV);

				glVertex3fv((nc+nv3).mV);
				glVertex3fv((nc-nv1).mV);
				glEnd();*/

				glLineWidth(1);
				
				glDepthMask(GL_FALSE);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				glColor4f(0.1f,0.1f,1,0.1f);
				drawBox(group->mObjectBounds[0], group->mObjectBounds[1]*1.01f+LLVector3(0.001f, 0.001f, 0.001f));
				glDepthMask(GL_TRUE);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
		}
		/*else
		{
			//occlusion paranoia check
			const LLSpatialGroup::OctreeNode* parent = node;
			while (parent != NULL)
			{
				LLSpatialGroup* grp = (LLSpatialGroup*) parent->getListener(0);
				if (grp->isState(LLSpatialGroup::ACTIVE_OCCLUSION))
				{
					return;
				}
				parent = (const LLSpatialGroup::OctreeNode*) parent->getParent();
			}

			glColor4f(1,0,1,1);
			drawBox(group->mBounds[0], group->mBounds[1]);
		}*/
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

		if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCTREE))
		{
			glBegin(GL_LINES);
			glColor4f(1,0.5f,0,1);
			glVertex3fv(nodeCenter.mV);
			glColor4f(0,1,1,0);
			glVertex3fv(octCenter.mV);
			glEnd();
		}

		for (LLSpatialGroup::OctreeState::const_element_iter i = branch->getData().begin(); i != branch->getData().end(); ++i)
		{
			LLDrawable* drawable = *i;
			
			if (drawable->isSpatialBridge())
			{
				LLSpatialBridge* bridge = (LLSpatialBridge*) drawable;
				glPushMatrix();
				glMultMatrixf((F32*)bridge->mDrawable->getWorldMatrix().mMatrix);
				traverse(bridge->mOctree);
				glPopMatrix();
			}
			
			if (!drawable->isVisible())
			{
				continue;
			}
				
			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_BBOXES))
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
				
										
				const LLVector3* ext = drawable->getSpatialExtents();
				
				LLVector3 pos = (ext[0] + ext[1]) * 0.5f;
				LLVector3 size = (ext[1] - ext[0]) * 0.5f;
				
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

				//render space partition trace
				glBegin(GL_LINE_STRIP);
				glColor4f(1,0,0,1);
				glVertex3fv(pos.mV);
				glColor4f(0,1,0,1);
				glVertex3dv(drawable->getPositionGroup().mdV);
				glColor4f(0,0,1,1);
				glVertex3fv(nodeCenter.mV);
				glColor4f(1,1,0,1);
				glVertex3fv(octCenter.mV);
				glEnd();
			}
			
			if (drawable->getVOVolume() && gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_FACE_CHAINS | LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY))
			{
				glLineWidth(3);
				
				for (int face=0; face<drawable->getNumFaces(); ++face)
				{
					LLFace *facep = drawable->getFace(face);
					
					if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_FACE_CHAINS))
					{
						LLGLDepthTest depth(GL_FALSE);
						if (facep->mNextFace)
						{
							glBegin(GL_LINE_STRIP);

							if (facep->isState(LLFace::GLOBAL))
							{
								glColor4f(0,1,0,1);
							}
							else
							{
								glColor4f(1,0.5f,0.25f,1);
							}
							
							if (drawable->isActive())
							{
								glVertex3fv(facep->mCenterLocal.mV);
								glVertex3fv(facep->mNextFace->mCenterLocal.mV);
							}
							else
							{
								glVertex3fv(facep->mCenterAgent.mV);
								glVertex3fv(facep->mNextFace->mCenterAgent.mV);
							}
							
							glEnd();
						}
						else
						{
							glPointSize(5);
							glBegin(GL_POINTS);
							
							if (!facep->isState(LLFace::GLOBAL))
							{
								glColor4f(1,0.75f,0,1);
								glVertex3fv(facep->mCenterLocal.mV);
							}
							else
							{
								glColor4f(0,0.75f,1,1);
								glVertex3fv(facep->mCenterAgent.mV);
							}
							
							glEnd();
							glPointSize(1);
						}
					}
					
					if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY))
					{
						LLVector4 cold(0,0,0.25f);
						LLVector4 hot(1,0.25f,0.25f);
					
						LLVector4 boost_cold(0,0,0,0);
						LLVector4 boost_hot(0,1,0,1);
						
						LLGLDisable blend(GL_BLEND);
						
						LLViewerImage* imagep = facep->getTexture();
						if (imagep)
						{
					
							//F32 vsize = LLVOVolume::getTextureVirtualSize(facep);
							F32 vsize = imagep->mMaxVirtualSize;
							
							if (vsize > sCurMaxTexPriority)
							{
								sCurMaxTexPriority = vsize;
							}
							
							F32 t = vsize/sLastMaxTexPriority;
							
							LLVector4 col = lerp(cold, hot, t);
							glColor4fv(col.mV);
						}
						else
						{
							glColor4f(1,0,1,1);
						}
						
						LLVector3 center = (facep->mExtents[1]+facep->mExtents[0])*0.5f;
						LLVector3 size = (facep->mExtents[1]-facep->mExtents[0])*0.5f + LLVector3(0.01f, 0.01f, 0.01f);
						drawBox(center, size);
						
						S32 boost = imagep->getBoostLevel();
						if (boost)
						{
							F32 t = (F32) boost / (F32) (LLViewerImage::BOOST_MAX_LEVEL-1);
							LLVector4 col = lerp(boost_cold, boost_hot, t);
							LLGLEnable blend_on(GL_BLEND);
							glBlendFunc(GL_SRC_ALPHA, GL_ONE);
							glColor4fv(col.mV);
							drawBox(center, size);
							glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
						}
							
					}
				}
			}
			
			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_POINTS))
			{
				glPointSize(4);
				glColor4f(1,1,1,1);
				glBegin(GL_POINTS);
				S32 num_faces = drawable->getNumFaces();
				for (S32 i = 0; i < num_faces; i++)
				{
					LLStrider<LLVector3> vertices;
					drawable->getFace(i)->getVertices(vertices);
					
					LLFace* face = drawable->getFace(i);
					
					for (S32 v = 0; v < (S32)drawable->getFace(i)->getGeomCount(); v++)
					{
						if (!face->getDrawable()->isActive())
						{
							//glVertex3fv(vertices[v].mV);
						}
						else
						{
							glVertex3fv((vertices[v]*face->getRenderMatrix()).mV);
						}
					}
				}
				glEnd();
				glPointSize(1);
			}
			
			glLineWidth(1);
		}
	}
};

void LLSpatialPartition::renderDebug()
{
	if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCTREE |
									  LLPipeline::RENDER_DEBUG_OCCLUSION |
									  LLPipeline::RENDER_DEBUG_BBOXES |
									  LLPipeline::RENDER_DEBUG_POINTS |
									  LLPipeline::RENDER_DEBUG_FACE_CHAINS |
									  LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY))
	{
		return;
	}
	
	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY))
	{
		//sLastMaxTexPriority = lerp(sLastMaxTexPriority, sCurMaxTexPriority, gFrameIntervalSeconds);
		sLastMaxTexPriority = sCurMaxTexPriority;
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

	{
		LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
		
		//draw frustum
		glColor4f(0,0,1,0.5f);
		glBegin(GL_QUADS);
		//glVertex3fv(gCamera->mAgentFrustum[0].mV);
		//glVertex3fv(gCamera->mAgentFrustum[1].mV);
		//glVertex3fv(gCamera->mAgentFrustum[2].mV);
		//glVertex3fv(gCamera->mAgentFrustum[3].mV);
	
		//glVertex3fv(gCamera->mAgentFrustum[4].mV);
		//glVertex3fv(gCamera->mAgentFrustum[5].mV);
		//glVertex3fv(gCamera->mAgentFrustum[6].mV);
		//glVertex3fv(gCamera->mAgentFrustum[7].mV);

		glVertex3fv(gCamera->mAgentFrustum[0].mV);
		glVertex3fv(gCamera->mAgentFrustum[1].mV);
		glVertex3fv(gCamera->mAgentFrustum[5].mV);
		glVertex3fv(gCamera->mAgentFrustum[4].mV);

		glVertex3fv(gCamera->mAgentFrustum[1].mV);
		glVertex3fv(gCamera->mAgentFrustum[2].mV);
		glVertex3fv(gCamera->mAgentFrustum[6].mV);
		glVertex3fv(gCamera->mAgentFrustum[5].mV);

		glVertex3fv(gCamera->mAgentFrustum[2].mV);
		glVertex3fv(gCamera->mAgentFrustum[3].mV);
		glVertex3fv(gCamera->mAgentFrustum[7].mV);
		glVertex3fv(gCamera->mAgentFrustum[6].mV);

		glVertex3fv(gCamera->mAgentFrustum[3].mV);
		glVertex3fv(gCamera->mAgentFrustum[0].mV);
		glVertex3fv(gCamera->mAgentFrustum[4].mV);
		glVertex3fv(gCamera->mAgentFrustum[7].mV);

		glEnd();
	}

	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCCLUSION))
	{
		LLGLDisable fog(GL_FOG);
		LLGLDepthTest gls_depth(GL_FALSE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		for (std::vector<LLSpatialGroup*>::iterator i = mOccludedList.begin(); i != mOccludedList.end(); ++i)
		{	//draw occluded nodes
			LLSpatialGroup* node = *i;
			if (node->isDead())
			{
				continue;
			}
			if (!node->isState(LLSpatialGroup::OCCLUDED))
			{
				continue;
			}
			else
			{
				glColor4f(0.25f,0.125f,0.1f,0.125f);
			}
			LLVector3 c;
			LLVector3 r;

			c = node->mBounds[0];
			r = node->mBounds[1]*SG_OCCLUSION_FUDGE + LLVector3(0.01f,0.01f,0.01f);;
			
			drawBox(c,r);
		}
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

/** 
 * @file llvieweroctree.cpp
 * @brief LLViewerOctreeGroup class implementation and supporting functions
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include "llvieweroctree.h"

//-----------------------------------------------------------------------------------
//static variables definitions
//-----------------------------------------------------------------------------------
U32 LLViewerOctreeEntryData::sCurVisible = 0;

//-----------------------------------------------------------------------------------
//some global functions definitions
//-----------------------------------------------------------------------------------
S32 AABBSphereIntersect(const LLVector3& min, const LLVector3& max, const LLVector3 &origin, const F32 &rad)
{
	return AABBSphereIntersectR2(min, max, origin, rad*rad);
}

S32 AABBSphereIntersectR2(const LLVector3& min, const LLVector3& max, const LLVector3 &origin, const F32 &r)
{
	F32 d = 0.f;
	F32 t;
	
	if ((min-origin).magVecSquared() < r &&
		(max-origin).magVecSquared() < r)
	{
		return 2;
	}

	for (U32 i = 0; i < 3; i++)
	{
		if (origin.mV[i] < min.mV[i])
		{
			t = min.mV[i] - origin.mV[i];
			d += t*t;
		}
		else if (origin.mV[i] > max.mV[i])
		{
			t = origin.mV[i] - max.mV[i];
			d += t*t;
		}

		if (d > r)
		{
			return 0;
		}
	}

	return 1;
}


S32 AABBSphereIntersect(const LLVector4a& min, const LLVector4a& max, const LLVector3 &origin, const F32 &rad)
{
	return AABBSphereIntersectR2(min, max, origin, rad*rad);
}

S32 AABBSphereIntersectR2(const LLVector4a& min, const LLVector4a& max, const LLVector3 &origin, const F32 &r)
{
	F32 d = 0.f;
	F32 t;
	
	LLVector4a origina;
	origina.load3(origin.mV);

	LLVector4a v;
	v.setSub(min, origina);
	
	if (v.dot3(v) < r)
	{
		v.setSub(max, origina);
		if (v.dot3(v) < r)
		{
			return 2;
		}
	}


	for (U32 i = 0; i < 3; i++)
	{
		if (origin.mV[i] < min[i])
		{
			t = min[i] - origin.mV[i];
			d += t*t;
		}
		else if (origin.mV[i] > max[i])
		{
			t = origin.mV[i] - max[i];
			d += t*t;
		}

		if (d > r)
		{
			return 0;
		}
	}

	return 1;
}

//-----------------------------------------------------------------------------------
//class LLViewerOctreeEntry definitions
//-----------------------------------------------------------------------------------
LLViewerOctreeEntry::LLViewerOctreeEntry() : mGroup(NULL)
{
	mPositionGroup.clear();
	mExtents[0].clear();
	mExtents[1].clear();
	mBinRadius = 0.f;
	mBinIndex = -1;

	for(S32 i = 0; i < NUM_DATA_TYPE; i++)
	{
		mData[i] = NULL;
	}
}

LLViewerOctreeEntry::~LLViewerOctreeEntry()
{
	llassert(!mGroup);
}

void LLViewerOctreeEntry::addData(LLViewerOctreeEntryData* data)
{
	//llassert(mData[data->getDataType()] == NULL);	
	llassert(data != NULL);

	mData[data->getDataType()] = data;
}

void LLViewerOctreeEntry::removeData(LLViewerOctreeEntryData* data)
{
	//llassert(data->getDataType() != LLVOCACHEENTRY); //can not remove VOCache entry

	if(!mData[data->getDataType()])
	{
		return;
	}

	mData[data->getDataType()] = NULL;
	
	if(mGroup != NULL && !mData[LLDRAWABLE])
	{
		LLviewerOctreeGroup* group = mGroup;
		mGroup = NULL;
		group->removeFromGroup(data);

		llassert(mBinIndex == -1);				
	}
}

//called by group handleDestruction() ONLY when group is destroyed by octree.
void LLViewerOctreeEntry::nullGroup()
{
	mGroup = NULL;
}

void LLViewerOctreeEntry::setGroup(LLviewerOctreeGroup* group)
{
	if(mGroup == group)
	{
		return;
	}

	if(mGroup)
	{
		LLviewerOctreeGroup* group = mGroup;
		mGroup = NULL;
		group->removeFromGroup(this);

		llassert(mBinIndex == -1);
	}

	mGroup = group;
}

//-----------------------------------------------------------------------------------
//class LLViewerOctreeEntryData definitions
//-----------------------------------------------------------------------------------
LLViewerOctreeEntryData::~LLViewerOctreeEntryData()
{
	if(mEntry)
	{
		mEntry->removeData(this);
	}
}

LLViewerOctreeEntryData::LLViewerOctreeEntryData(LLViewerOctreeEntry::eEntryDataType_t data_type)
	: mDataType(data_type),
	  mEntry(NULL)
{
}

//virtual
void LLViewerOctreeEntryData::setOctreeEntry(LLViewerOctreeEntry* entry)
{
	if(mEntry.notNull())
	{
		return; 
	}

	if(!entry)
	{
		mEntry = new LLViewerOctreeEntry();
	}
	else
	{
		mEntry = entry;
	}
	mEntry->addData(this);
}

void LLViewerOctreeEntryData::setSpatialExtents(const LLVector3& min, const LLVector3& max)
{ 
	mEntry->mExtents[0].load3(min.mV); 
	mEntry->mExtents[1].load3(max.mV);
}

void LLViewerOctreeEntryData::setSpatialExtents(const LLVector4a& min, const LLVector4a& max)
{ 
	mEntry->mExtents[0] = min; 
	mEntry->mExtents[1] = max;
}

void LLViewerOctreeEntryData::setPositionGroup(const LLVector4a& pos)
{
	mEntry->mPositionGroup = pos;
}

const LLVector4a* LLViewerOctreeEntryData::getSpatialExtents() const 
{
	return mEntry->getSpatialExtents();
}

//virtual
void LLViewerOctreeEntryData::setGroup(LLviewerOctreeGroup* group)
{
	mEntry->setGroup(group);
}

void LLViewerOctreeEntryData::shift(const LLVector4a &shift_vector)
{
	mEntry->mExtents[0].add(shift_vector);
	mEntry->mExtents[1].add(shift_vector);
	mEntry->mPositionGroup.add(shift_vector);
}

LLviewerOctreeGroup* LLViewerOctreeEntryData::getGroup()const        
{
	return mEntry.notNull() ? mEntry->mGroup : NULL;
}

const LLVector4a& LLViewerOctreeEntryData::getPositionGroup() const  
{
	return mEntry->getPositionGroup();
}	

//virtual
bool LLViewerOctreeEntryData::isVisible() const
{
	if(mEntry)
	{
		return mEntry->mVisible == sCurVisible;
	}
	return false;
}

//virtual
bool LLViewerOctreeEntryData::isRecentlyVisible() const
{
	if(!mEntry)
	{
		return false;
	}

	if(isVisible())
	{
		return true;
	}
	if(getGroup() && getGroup()->isRecentlyVisible())
	{
		setVisible();
		return true;
	}

	return (sCurVisible - mEntry->mVisible < getMinVisFrameRange());
}

void LLViewerOctreeEntryData::setVisible() const
{
	if(mEntry)
	{
		mEntry->mVisible = sCurVisible;
	}
}

//-----------------------------------------------------------------------------------
//class LLviewerOctreeGroup definitions
//-----------------------------------------------------------------------------------

LLviewerOctreeGroup::~LLviewerOctreeGroup()
{
}

LLviewerOctreeGroup::LLviewerOctreeGroup(OctreeNode* node) :
	mOctreeNode(node),
	mState(CLEAN)
{
	LLVector4a tmp;
	tmp.splat(0.f);
	mExtents[0] = mExtents[1] = mObjectBounds[0] = mObjectBounds[0] = mObjectBounds[1] = 
		mObjectExtents[0] = mObjectExtents[1] = tmp;
	
	mBounds[0] = node->getCenter();
	mBounds[1] = node->getSize();

	mOctreeNode->addListener(this);
}

bool LLviewerOctreeGroup::removeFromGroup(LLViewerOctreeEntryData* data)
{
	return removeFromGroup(data->getEntry());
}

bool LLviewerOctreeGroup::removeFromGroup(LLViewerOctreeEntry* entry)
{
	llassert(entry != NULL);
	llassert(!entry->getGroup());

	unbound();
	if (mOctreeNode)
	{
		if (!mOctreeNode->remove(entry))
		{
			OCT_ERRS << "Could not remove LLVOCacheEntry from LLVOCacheOctreeGroup" << llendl;
			return false;
		}
	}
	setState(OBJECT_DIRTY);

	return true;
}

//virtual 
void LLviewerOctreeGroup::unbound()
{
	if (isDirty())
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
			LLviewerOctreeGroup* group = (LLviewerOctreeGroup*) parent->getListener(0);
			if (!group || group->isDirty())
			{
				return;
			}
			
			group->setState(DIRTY);
			parent = (OctreeNode*) parent->getParent();
		}
	}
}
	
//virtual 
void LLviewerOctreeGroup::rebound()
{
	if (!isDirty())
	{	
		return;
	}
	
	if (mOctreeNode->getChildCount() == 1 && mOctreeNode->getElementCount() == 0)
	{
		LLviewerOctreeGroup* group = (LLviewerOctreeGroup*) mOctreeNode->getChild(0)->getListener(0);
		group->rebound();
		
		//copy single child's bounding box
		mBounds[0] = group->mBounds[0];
		mBounds[1] = group->mBounds[1];
		mExtents[0] = group->mExtents[0];
		mExtents[1] = group->mExtents[1];
		
		group->setState(SKIP_FRUSTUM_CHECK);
	}
	else if (mOctreeNode->isLeaf())
	{ //copy object bounding box if this is a leaf
		boundObjects(TRUE, mExtents[0], mExtents[1]);
		mBounds[0] = mObjectBounds[0];
		mBounds[1] = mObjectBounds[1];
	}
	else
	{
		LLVector4a& newMin = mExtents[0];
		LLVector4a& newMax = mExtents[1];
		LLviewerOctreeGroup* group = (LLviewerOctreeGroup*) mOctreeNode->getChild(0)->getListener(0);
		group->clearState(SKIP_FRUSTUM_CHECK);
		group->rebound();
		//initialize to first child
		newMin = group->mExtents[0];
		newMax = group->mExtents[1];

		//first, rebound children
		for (U32 i = 1; i < mOctreeNode->getChildCount(); i++)
		{
			group = (LLviewerOctreeGroup*) mOctreeNode->getChild(i)->getListener(0);
			group->clearState(SKIP_FRUSTUM_CHECK);
			group->rebound();
			const LLVector4a& max = group->mExtents[1];
			const LLVector4a& min = group->mExtents[0];

			newMax.setMax(newMax, max);
			newMin.setMin(newMin, min);
		}

		boundObjects(FALSE, newMin, newMax);
		
		mBounds[0].setAdd(newMin, newMax);
		mBounds[0].mul(0.5f);
		mBounds[1].setSub(newMax, newMin);
		mBounds[1].mul(0.5f);
	}
	
	clearState(DIRTY);

	return;
}

//virtual 
void LLviewerOctreeGroup::handleInsertion(const TreeNode* node, LLViewerOctreeEntry* obj)
{
	obj->setGroup(this);	
	unbound();
	setState(OBJECT_DIRTY);
}
	
//virtual 
void LLviewerOctreeGroup::handleRemoval(const TreeNode* node, LLViewerOctreeEntry* obj)
{
	obj->setGroup(NULL);
	unbound();
	setState(OBJECT_DIRTY);
}
	
//virtual 
void LLviewerOctreeGroup::handleDestruction(const TreeNode* node)
{
	for (OctreeNode::element_iter i = mOctreeNode->getDataBegin(); i != mOctreeNode->getDataEnd(); ++i)
	{
		LLViewerOctreeEntry* obj = *i;
		if (obj && obj->getGroup() == this)
		{
			obj->nullGroup();
		}
	}
}
	
//virtual 
void LLviewerOctreeGroup::handleStateChange(const TreeNode* node)
{
	//drop bounding box upon state change
	if (mOctreeNode != node)
	{
		mOctreeNode = (OctreeNode*) node;
	}
	unbound();
}
	
//virtual 
void LLviewerOctreeGroup::handleChildAddition(const OctreeNode* parent, OctreeNode* child)
{
	llerrs << "can not access here. It is an abstract class." << llendl;

	//((LLviewerOctreeGroup*)child->getListener(0))->unbound();
}
	
//virtual 
void LLviewerOctreeGroup::handleChildRemoval(const OctreeNode* parent, const OctreeNode* child)
{
	unbound();
}

LLviewerOctreeGroup* LLviewerOctreeGroup::getParent()
{
	if(!mOctreeNode)
	{
		return NULL;
	}
	
	OctreeNode* parent = mOctreeNode->getOctParent();

	if (parent)
	{
		return (LLviewerOctreeGroup*) parent->getListener(0);
	}

	return NULL;
}

//virtual 
bool LLviewerOctreeGroup::boundObjects(BOOL empty, LLVector4a& minOut, LLVector4a& maxOut)
{
	const OctreeNode* node = mOctreeNode;

	if (node->isEmpty())
	{	//don't do anything if there are no objects
		if (empty && mOctreeNode->getParent())
		{	//only root is allowed to be empty
			OCT_ERRS << "Empty leaf found in octree." << llendl;
		}
		return false;
	}

	LLVector4a& newMin = mObjectExtents[0];
	LLVector4a& newMax = mObjectExtents[1];
	
	if (hasState(OBJECT_DIRTY))
	{ //calculate new bounding box
		clearState(OBJECT_DIRTY);

		//initialize bounding box to first element
		OctreeNode::const_element_iter i = node->getDataBegin();
		LLViewerOctreeEntry* entry = *i;
		const LLVector4a* minMax = entry->getSpatialExtents();

		newMin = minMax[0];
		newMax = minMax[1];

		for (++i; i != node->getDataEnd(); ++i)
		{
			entry = *i;
			minMax = entry->getSpatialExtents();
			
			update_min_max(newMin, newMax, minMax[0]);
			update_min_max(newMin, newMax, minMax[1]);
		}
		
		mObjectBounds[0].setAdd(newMin, newMax);
		mObjectBounds[0].mul(0.5f);
		mObjectBounds[1].setSub(newMax, newMin);
		mObjectBounds[1].mul(0.5f);
	}
	
	if (empty)
	{
		minOut = newMin;
		maxOut = newMax;
	}
	else
	{
		minOut.setMin(minOut, newMin);
		maxOut.setMax(maxOut, newMax);
	}
		
	return TRUE;
}

//virtual 
BOOL LLviewerOctreeGroup::isVisible() const
{
	return TRUE;
}

//-----------------------------------------------------------------------------------
//class LLViewerOctreeCull definitions
//-----------------------------------------------------------------------------------

//virtual 
bool LLViewerOctreeCull::earlyFail(LLviewerOctreeGroup* group)
{	
	return false;
}
	
//virtual 
void LLViewerOctreeCull::traverse(const OctreeNode* n)
{
	LLviewerOctreeGroup* group = (LLviewerOctreeGroup*) n->getListener(0);

	if (earlyFail(group))
	{
		return;
	}
		
	if (mRes == 2 || 
		(mRes && group->hasState(LLviewerOctreeGroup::SKIP_FRUSTUM_CHECK)))
	{	//fully in, just add everything
		OctreeTraveler::traverse(n);
	}
	else
	{
		mRes = frustumCheck(group);
				
		if (mRes)
		{ //at least partially in, run on down
			OctreeTraveler::traverse(n);
		}

		mRes = 0;
	}
}
	
S32 LLViewerOctreeCull::AABBInFrustumNoFarClipGroupBounds(const LLviewerOctreeGroup* group)
{
	return mCamera->AABBInFrustumNoFarClip(group->mBounds[0], group->mBounds[1]);
}

S32 LLViewerOctreeCull::AABBSphereIntersectGroupExtents(const LLviewerOctreeGroup* group)
{
	return AABBSphereIntersect(group->mExtents[0], group->mExtents[1], mCamera->getOrigin(), mCamera->mFrustumCornerDist);
}

S32 LLViewerOctreeCull::AABBInFrustumNoFarClipObjectBounds(const LLviewerOctreeGroup* group)
{
	return mCamera->AABBInFrustumNoFarClip(group->mObjectBounds[0], group->mObjectBounds[1]);
}

S32 LLViewerOctreeCull::AABBSphereIntersectObjectExtents(const LLviewerOctreeGroup* group)
{
	return AABBSphereIntersect(group->mObjectExtents[0], group->mObjectExtents[1], mCamera->getOrigin(), mCamera->mFrustumCornerDist);
}

S32 LLViewerOctreeCull::AABBInFrustumGroupBounds(const LLviewerOctreeGroup* group)
{
	return mCamera->AABBInFrustum(group->mBounds[0], group->mBounds[1]);
}

S32 LLViewerOctreeCull::AABBInFrustumObjectBounds(const LLviewerOctreeGroup* group)
{
	return mCamera->AABBInFrustum(group->mObjectBounds[0], group->mObjectBounds[1]);
}

//virtual 
bool LLViewerOctreeCull::checkObjects(const OctreeNode* branch, const LLviewerOctreeGroup* group)
{
	if (branch->getElementCount() == 0) //no elements
	{
		return false;
	}
	else if (branch->getChildCount() == 0) //leaf state, already checked tightest bounding box
	{
		return true;
	}
	else if (mRes == 1 && !frustumCheckObjects(group)) //no objects in frustum
	{
		return false;
	}
		
	return true;
}

//virtual 
void LLViewerOctreeCull::preprocess(LLviewerOctreeGroup* group)
{		
}
	
//virtual 
void LLViewerOctreeCull::processGroup(LLviewerOctreeGroup* group)
{
}
	
//virtual 
void LLViewerOctreeCull::visit(const OctreeNode* branch) 
{	
	LLviewerOctreeGroup* group = (LLviewerOctreeGroup*) branch->getListener(0);

	preprocess(group);
		
	if (checkObjects(branch, group))
	{
		processGroup(group);
	}
}


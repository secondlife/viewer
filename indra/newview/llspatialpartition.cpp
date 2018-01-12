/** 
 * @file llspatialpartition.cpp
 * @brief LLSpatialGroup class implementation and supporting functions
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

#include "llspatialpartition.h"

#include "llappviewer.h"
#include "lltexturecache.h"
#include "lltexturefetch.h"
#include "llimageworker.h"
#include "llviewerwindow.h"
#include "llviewerobjectlist.h"
#include "llvovolume.h"
#include "llvolume.h"
#include "llvolumeoctree.h"
#include "llviewercamera.h"
#include "llface.h"
#include "llfloatertools.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llcamera.h"
#include "pipeline.h"
#include "llmeshrepository.h"
#include "llrender.h"
#include "lloctree.h"
#include "llphysicsshapebuilderutil.h"
#include "llvoavatar.h"
#include "llvolumemgr.h"
#include "lltextureatlas.h"
#include "llviewershadermgr.h"

static LLTrace::BlockTimerStatHandle FTM_FRUSTUM_CULL("Frustum Culling");
static LLTrace::BlockTimerStatHandle FTM_CULL_REBOUND("Cull Rebound Partition");

extern bool gShiftFrame;

static U32 sZombieGroups = 0;
U32 LLSpatialGroup::sNodeCount = 0;

U32 gOctreeMaxCapacity;
F32 gOctreeMinSize;

BOOL LLSpatialGroup::sNoDelete = FALSE;

static F32 sLastMaxTexPriority = 1.f;
static F32 sCurMaxTexPriority = 1.f;

BOOL LLSpatialPartition::sTeleportRequested = FALSE;

//static counter for frame to switch LOD on

void sg_assert(BOOL expr)
{
#if LL_OCTREE_PARANOIA_CHECK
	if (!expr)
	{
		LL_ERRS() << "Octree invalid!" << LL_ENDL;
	}
#endif
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
	/*if (sNoDelete)
	{
		LL_ERRS() << "Illegal deletion of LLSpatialGroup!" << LL_ENDL;
	}*/

	if (gDebugGL)
	{
		gPipeline.checkReferences(this);
	}

	if (hasState(DEAD))
	{
		sZombieGroups--;
	}
	
	sNodeCount--;

	clearDrawMap();
	clearAtlasList() ;
}

BOOL LLSpatialGroup::hasAtlas(LLTextureAtlas* atlasp)
{
	S8 type = atlasp->getComponents() - 1 ;
	for(std::list<LLTextureAtlas*>::iterator iter = mAtlasList[type].begin(); iter != mAtlasList[type].end() ; ++iter)
	{
		if(atlasp == *iter)
		{
			return TRUE ;
		}
	}
	return FALSE ;
}

void LLSpatialGroup::addAtlas(LLTextureAtlas* atlasp, S8 recursive_level) 
{		
	if(!hasAtlas(atlasp))
	{
		mAtlasList[atlasp->getComponents() - 1].push_back(atlasp) ;
		atlasp->addSpatialGroup(this) ;
	}
	
	--recursive_level;
	if(recursive_level)//levels propagating up.
	{
		LLSpatialGroup* parent = getParent() ;
		if(parent)
		{
			parent->addAtlas(atlasp, recursive_level) ;
		}
	}	
}

void LLSpatialGroup::removeAtlas(LLTextureAtlas* atlasp, BOOL remove_group, S8 recursive_level) 
{
	mAtlasList[atlasp->getComponents() - 1].remove(atlasp) ;
	if(remove_group)
	{
		atlasp->removeSpatialGroup(this) ;
	}

	--recursive_level;
	if(recursive_level)//levels propagating up.
	{
		LLSpatialGroup* parent = getParent() ;
		if(parent)
		{
			parent->removeAtlas(atlasp, recursive_level) ;
		}
	}	
}

void LLSpatialGroup::clearAtlasList() 
{
	std::list<LLTextureAtlas*>::iterator iter ;
	for(S8 i = 0 ; i < 4 ; i++)
	{
		if(mAtlasList[i].size() > 0)
		{
			for(iter = mAtlasList[i].begin(); iter != mAtlasList[i].end() ; ++iter)
			{
				((LLTextureAtlas*)*iter)->removeSpatialGroup(this) ;			
			}
			mAtlasList[i].clear() ;
		}
	}
}

LLTextureAtlas* LLSpatialGroup::getAtlas(S8 ncomponents, S8 to_be_reserved, S8 recursive_level)
{
	S8 type = ncomponents - 1 ;
	if(mAtlasList[type].size() > 0)
	{
		for(std::list<LLTextureAtlas*>::iterator iter = mAtlasList[type].begin(); iter != mAtlasList[type].end() ; ++iter)
		{
			if(!((LLTextureAtlas*)*iter)->isFull(to_be_reserved))
			{
				return *iter ;
			}
		}
	}

	--recursive_level;
	if(recursive_level)
	{
		LLSpatialGroup* parent = getParent() ;
		if(parent)
		{
			return parent->getAtlas(ncomponents, to_be_reserved, recursive_level) ;
		}
	}
	return NULL ;
}

void LLSpatialGroup::setCurUpdatingSlot(LLTextureAtlasSlot* slotp) 
{ 
	mCurUpdatingSlotp = slotp;

	//if(!hasAtlas(mCurUpdatingSlotp->getAtlas()))
	//{
	//	addAtlas(mCurUpdatingSlotp->getAtlas()) ;
	//}
}

LLTextureAtlasSlot* LLSpatialGroup::getCurUpdatingSlot(LLViewerTexture* imagep, S8 recursive_level) 
{ 
	if(gFrameCount && mCurUpdatingTime == gFrameCount && mCurUpdatingTexture == imagep)
	{
		return mCurUpdatingSlotp ;
	}

	//--recursive_level ;
	//if(recursive_level)
	//{
	//	LLSpatialGroup* parent = getParent() ;
	//	if(parent)
	//	{
	//		return parent->getCurUpdatingSlot(imagep, recursive_level) ;
	//	}
	//}
	return NULL ;
}

void LLSpatialGroup::clearDrawMap()
{
	mDrawMap.clear();
}

BOOL LLSpatialGroup::isHUDGroup() 
{
	return getSpatialPartition() && getSpatialPartition()->isHUDPartition() ; 
}

void LLSpatialGroup::validate()
{
	ll_assert_aligned(this,64);
#if LL_OCTREE_PARANOIA_CHECK

	sg_assert(!isState(DIRTY));
	sg_assert(!isDead());

	LLVector4a myMin;
	myMin.setSub(mBounds[0], mBounds[1]);
	LLVector4a myMax;
	myMax.setAdd(mBounds[0], mBounds[1]);

	validateDrawMap();

	for (element_iter i = getDataBegin(); i != getDataEnd(); ++i)
	{
		LLDrawable* drawable = *i;
		sg_assert(drawable->getSpatialGroup() == this);
		if (drawable->getSpatialBridge())
		{
			sg_assert(drawable->getSpatialBridge() == getSpatialPartition()->asBridge());
		}

		/*if (drawable->isSpatialBridge())
		{
			LLSpatialPartition* part = drawable->asPartition();
			if (!part)
			{
				LL_ERRS() << "Drawable reports it is a spatial bridge but not a partition." << LL_ENDL;
			}
			LLSpatialGroup* group = (LLSpatialGroup*) part->mOctree->getListener(0);
			group->validate();
		}*/
	}

	for (U32 i = 0; i < mOctreeNode->getChildCount(); ++i)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) mOctreeNode->getChild(i)->getListener(0);

		group->validate();
		
		//ensure all children are enclosed in this node
		LLVector4a center = group->mBounds[0];
		LLVector4a size = group->mBounds[1];
		
		LLVector4a min;
		min.setSub(center, size);
		LLVector4a max;
		max.setAdd(center, size);
		
		for (U32 j = 0; j < 3; j++)
		{
			sg_assert(min[j] >= myMin[j]-0.02f);
			sg_assert(max[j] <= myMax[j]+0.02f);
		}
	}

#endif
}

void LLSpatialGroup::validateDrawMap()
{
#if LL_OCTREE_PARANOIA_CHECK
	for (draw_map_t::iterator i = mDrawMap.begin(); i != mDrawMap.end(); ++i)
	{
		LLSpatialGroup::drawmap_elem_t& draw_vec = i->second;
		for (drawmap_elem_t::iterator j = draw_vec.begin(); j != draw_vec.end(); ++j)
		{
			LLDrawInfo& params = **j;
		
			params.validate();
		}
	}
#endif
}

BOOL LLSpatialGroup::updateInGroup(LLDrawable *drawablep, BOOL immediate)
{
	drawablep->updateSpatialExtents();

	OctreeNode* parent = mOctreeNode->getOctParent();
	
	if (mOctreeNode->isInside(drawablep->getPositionGroup()) && 
		(mOctreeNode->contains(drawablep->getEntry()) ||
		 (drawablep->getBinRadius() > mOctreeNode->getSize()[0] &&
				parent && parent->getElementCount() >= gOctreeMaxCapacity)))
	{
		unbound();
		setState(OBJECT_DIRTY);
		//setState(GEOM_DIRTY);
		return TRUE;
	}
		
	return FALSE;
}


BOOL LLSpatialGroup::addObject(LLDrawable *drawablep)
{
	if(!drawablep)
	{
		return FALSE;
	}
	{
		drawablep->setGroup(this);
		setState(OBJECT_DIRTY | GEOM_DIRTY);
		setOcclusionState(LLSpatialGroup::DISCARD_QUERY, LLSpatialGroup::STATE_MODE_ALL_CAMERAS);
		gPipeline.markRebuild(this, TRUE);
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
	if (!isDead())
	{
		getSpatialPartition()->rebuildGeom(this);

		if (hasState(LLSpatialGroup::MESH_DIRTY))
		{
			gPipeline.markMeshDirty(this);
		}
	}
}

void LLSpatialGroup::rebuildMesh()
{
	if (!isDead())
	{
		getSpatialPartition()->rebuildMesh(this);
	}
}

static LLTrace::BlockTimerStatHandle FTM_REBUILD_VBO("VBO Rebuilt");
static LLTrace::BlockTimerStatHandle FTM_ADD_GEOMETRY_COUNT("Add Geometry");
static LLTrace::BlockTimerStatHandle FTM_CREATE_VB("Create VB");
static LLTrace::BlockTimerStatHandle FTM_GET_GEOMETRY("Get Geometry");

void LLSpatialPartition::rebuildGeom(LLSpatialGroup* group)
{
	if (group->isDead() || !group->hasState(LLSpatialGroup::GEOM_DIRTY))
	{
		return;
	}

	if (group->changeLOD())
	{
		group->mLastUpdateDistance = group->mDistance;
		group->mLastUpdateViewAngle = group->mViewAngle;
	}
	
	LL_RECORD_BLOCK_TIME(FTM_REBUILD_VBO);	

	group->clearDrawMap();
	
	//get geometry count
	U32 index_count = 0;
	U32 vertex_count = 0;

	{
		LL_RECORD_BLOCK_TIME(FTM_ADD_GEOMETRY_COUNT);
		addGeometryCount(group, vertex_count, index_count);
	}

	if (vertex_count > 0 && index_count > 0)
	{ //create vertex buffer containing volume geometry for this node
		{
			LL_RECORD_BLOCK_TIME(FTM_CREATE_VB);
			group->mBuilt = 1.f;
			if (group->mVertexBuffer.isNull() ||
				!group->mVertexBuffer->isWriteable() ||
				(group->mBufferUsage != group->mVertexBuffer->getUsage() && LLVertexBuffer::sEnableVBOs))
			{
				group->mVertexBuffer = createVertexBuffer(mVertexDataMask, group->mBufferUsage);
				if (!group->mVertexBuffer->allocateBuffer(vertex_count, index_count, true))
				{
					LL_WARNS() << "Failed to allocate Vertex Buffer on rebuild to "
						<< vertex_count << " vertices and "
						<< index_count << " indices" << LL_ENDL;
					group->mVertexBuffer = NULL;
					group->mBufferMap.clear();
				}
				stop_glerror();
			}
			else
			{
				if (!group->mVertexBuffer->resizeBuffer(vertex_count, index_count))
				{
					// Is likely to cause a crash. If this gets triggered find a way to avoid it (don't forget to reset face)
					LL_WARNS() << "Failed to resize Vertex Buffer on rebuild to "
						<< vertex_count << " vertices and "
						<< index_count << " indices" << LL_ENDL;
					group->mVertexBuffer = NULL;
					group->mBufferMap.clear();
				}
				stop_glerror();
			}
		}

		if (group->mVertexBuffer)
		{
			LL_RECORD_BLOCK_TIME(FTM_GET_GEOMETRY);
			getGeometry(group);
		}
	}
	else
	{
		group->mVertexBuffer = NULL;
		group->mBufferMap.clear();
	}

	group->mLastUpdateTime = gFrameTimeSeconds;
	group->clearState(LLSpatialGroup::GEOM_DIRTY);
}


void LLSpatialPartition::rebuildMesh(LLSpatialGroup* group)
{

}

LLSpatialGroup* LLSpatialGroup::getParent()
{
	return (LLSpatialGroup*)LLViewerOctreeGroup::getParent();
	}

BOOL LLSpatialGroup::removeObject(LLDrawable *drawablep, BOOL from_octree)
	{
	if(!drawablep)
	{
		return FALSE;
	}

	unbound();
	if (mOctreeNode && !from_octree)
	{
		drawablep->setGroup(NULL);
	}
	else
	{
		drawablep->setGroup(NULL);
		setState(GEOM_DIRTY);
		gPipeline.markRebuild(this, TRUE);

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

		if (getElementCount() == 0)
		{ //delete draw map on last element removal since a rebuild might never happen
			clearDrawMap();
		}
	}
	return TRUE;
}

void LLSpatialGroup::shift(const LLVector4a &offset)
{
	LLVector4a t = mOctreeNode->getCenter();
	t.add(offset);	
	mOctreeNode->setCenter(t);
	mOctreeNode->updateMinMax();
	mBounds[0].add(offset);
	mExtents[0].add(offset);
	mExtents[1].add(offset);
	mObjectBounds[0].add(offset);
	mObjectExtents[0].add(offset);
	mObjectExtents[1].add(offset);

	if (!getSpatialPartition()->mRenderByGroup && 
		getSpatialPartition()->mPartitionType != LLViewerRegion::PARTITION_TREE &&
		getSpatialPartition()->mPartitionType != LLViewerRegion::PARTITION_TERRAIN &&
		getSpatialPartition()->mPartitionType != LLViewerRegion::PARTITION_BRIDGE)
	{
		setState(GEOM_DIRTY);
		gPipeline.markRebuild(this, TRUE);
	}
}

class LLSpatialSetState : public OctreeTraveler
{
public:
	U32 mState;
	LLSpatialSetState(U32 state) : mState(state) { }
	virtual void visit(const OctreeNode* branch) { ((LLSpatialGroup*) branch->getListener(0))->setState(mState); }	
};

class LLSpatialSetStateDiff : public LLSpatialSetState
{
public:
	LLSpatialSetStateDiff(U32 state) : LLSpatialSetState(state) { }

	virtual void traverse(const OctreeNode* n)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) n->getListener(0);
		
		if (!group->hasState(mState))
		{
			OctreeTraveler::traverse(n);
		}
	}
};

void LLSpatialGroup::setState(U32 state, S32 mode) 
{
	llassert(state <= LLSpatialGroup::STATE_MASK);
	
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

class LLSpatialClearState : public OctreeTraveler
{
public:
	U32 mState;
	LLSpatialClearState(U32 state) : mState(state) { }
	virtual void visit(const OctreeNode* branch) { ((LLSpatialGroup*) branch->getListener(0))->clearState(mState); }
};

class LLSpatialClearStateDiff : public LLSpatialClearState
{
public:
	LLSpatialClearStateDiff(U32 state) : LLSpatialClearState(state) { }

	virtual void traverse(const OctreeNode* n)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) n->getListener(0);
		
		if (group->hasState(mState))
		{
			OctreeTraveler::traverse(n);
		}
	}
};

void LLSpatialGroup::clearState(U32 state, S32 mode)
{
	llassert(state <= LLSpatialGroup::STATE_MASK);

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
}

//======================================
//		Octree Listener Implementation
//======================================

LLSpatialGroup::LLSpatialGroup(OctreeNode* node, LLSpatialPartition* part) : LLOcclusionCullingGroup(node, part),
	mObjectBoxSize(1.f),
	mGeometryBytes(0),
	mSurfaceArea(0.f),
	mBuilt(0.f),
	mVertexBuffer(NULL), 
	mBufferUsage(part->mBufferUsage),
	mDistance(0.f),
	mDepth(0.f),
	mLastUpdateDistance(-1.f), 
	mLastUpdateTime(gFrameTimeSeconds),
	mAtlasList(4),
	mCurUpdatingTime(0),
	mCurUpdatingSlotp(NULL),
	mCurUpdatingTexture (NULL)
{
	ll_assert_aligned(this,16);
	
	sNodeCount++;

	mViewAngle.splat(0.f);
	mLastUpdateViewAngle.splat(-1.f);

	sg_assert(mOctreeNode->getListenerCount() == 0);
	setState(SG_INITIAL_STATE_MASK);
	gPipeline.markRebuild(this, TRUE);

	mRadius = 1;
	mPixelArea = 1024.f;
}

void LLSpatialGroup::updateDistance(LLCamera &camera)
{
	if (LLViewerCamera::sCurCameraID != LLViewerCamera::CAMERA_WORLD)
	{
		LL_WARNS() << "Attempted to update distance for camera other than world camera!" << LL_ENDL;
		return;
	}

	if (gShiftFrame)
	{
		return;
	}

#if !LL_RELEASE_FOR_DOWNLOAD
	if (hasState(LLSpatialGroup::OBJECT_DIRTY))
	{
		LL_ERRS() << "Spatial group dirty on distance update." << LL_ENDL;
	}
#endif
	if (!isEmpty())
	{
		mRadius = getSpatialPartition()->mRenderByGroup ? mObjectBounds[1].getLength3().getF32() :
						(F32) mOctreeNode->getSize().getLength3().getF32();
		mDistance = getSpatialPartition()->calcDistance(this, camera);
		mPixelArea = getSpatialPartition()->calcPixelArea(this, camera);
	}
}

F32 LLSpatialPartition::calcDistance(LLSpatialGroup* group, LLCamera& camera)
{
	LLVector4a eye;
	LLVector4a origin;
	origin.load3(camera.getOrigin().mV);

	eye.setSub(group->mObjectBounds[0], origin);

	F32 dist = 0.f;

	if (group->mDrawMap.find(LLRenderPass::PASS_ALPHA) != group->mDrawMap.end())
	{
		LLVector4a v = eye;

		dist = eye.getLength3().getF32();
		eye.normalize3fast();

		if (!group->hasState(LLSpatialGroup::ALPHA_DIRTY))
		{
			if (!group->getSpatialPartition()->isBridge())
			{
				LLVector4a view_angle = eye;

				LLVector4a diff;
				diff.setSub(view_angle, group->mLastUpdateViewAngle);

				if (diff.getLength3().getF32() > 0.64f)
				{
					group->mViewAngle = view_angle;
					group->mLastUpdateViewAngle = view_angle;
					//for occasional alpha sorting within the group
					//NOTE: If there is a trivial way to detect that alpha sorting here would not change the render order,
					//not setting this node to dirty would be a very good thing
					group->setState(LLSpatialGroup::ALPHA_DIRTY);
					gPipeline.markRebuild(group, FALSE);
				}
			}
		}

		//calculate depth of node for alpha sorting

		LLVector3 at = camera.getAtAxis();

		LLVector4a ata;
		ata.load3(at.mV);

		LLVector4a t = ata;
		//front of bounding box
		t.mul(0.25f);
		t.mul(group->mObjectBounds[1]);
		v.sub(t);
		
		group->mDepth = v.dot3(ata).getF32();
	}
	else
	{
		dist = eye.getLength3().getF32();
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

F32 LLSpatialGroup::getUpdateUrgency() const
{
	if (!isVisible())
	{
		return 0.f;
	}
	else
	{
		F32 time = gFrameTimeSeconds-mLastUpdateTime+4.f;
		return time + (mObjectBounds[1].dot3(mObjectBounds[1]).getF32()+1.f)/mDistance;
	}
}

BOOL LLSpatialGroup::changeLOD()
{
	if (hasState(ALPHA_DIRTY | OBJECT_DIRTY))
	{ ///a rebuild is going to happen, update distance and LoD
		return TRUE;
	}

	if (getSpatialPartition()->mSlopRatio > 0.f)
	{
		F32 ratio = (mDistance - mLastUpdateDistance)/(llmax(mLastUpdateDistance, mRadius));

		if (fabsf(ratio) >= getSpatialPartition()->mSlopRatio)
		{
			return TRUE;
		}

		if (mDistance > mRadius*2.f)
		{
			return FALSE;
		}
	}
	
	if (needsUpdate())
	{
		return TRUE;
	}
	
	return FALSE;
}

void LLSpatialGroup::handleInsertion(const TreeNode* node, LLViewerOctreeEntry* entry)
{
	addObject((LLDrawable*)entry->getDrawable());
	unbound();
	setState(OBJECT_DIRTY);
}

void LLSpatialGroup::handleRemoval(const TreeNode* node, LLViewerOctreeEntry* entry)
{
	removeObject((LLDrawable*)entry->getDrawable(), TRUE);
	LLViewerOctreeGroup::handleRemoval(node, entry);
}

void LLSpatialGroup::handleDestruction(const TreeNode* node)
{
	if(isDead())
	{
		return;
	}
	setState(DEAD);	

	for (element_iter i = getDataBegin(); i != getDataEnd(); ++i)
	{
		LLViewerOctreeEntry* entry = *i;

		if (entry->getGroup() == this)
		{
			if(entry->hasDrawable())
			{
				((LLDrawable*)entry->getDrawable())->setGroup(NULL);
			}
		}
	}
	
	//clean up avatar attachment stats
	LLSpatialBridge* bridge = getSpatialPartition()->asBridge();
	if (bridge)
	{
		if (bridge->mAvatar.notNull())
		{
			bridge->mAvatar->subtractAttachmentArea(mSurfaceArea );
		}
	}

	clearDrawMap();
	mVertexBuffer = NULL;
	mBufferMap.clear();
	sZombieGroups++;
	mOctreeNode = NULL;
}

void LLSpatialGroup::handleChildAddition(const OctreeNode* parent, OctreeNode* child) 
{
	if (child->getListenerCount() == 0)
	{
		new LLSpatialGroup(child, getSpatialPartition());
	}
	else
	{
		OCT_ERRS << "LLSpatialGroup redundancy detected." << LL_ENDL;
	}

	unbound();

	assert_states_valid(this);
}

void LLSpatialGroup::destroyGL(bool keep_occlusion) 
{
	setState(LLSpatialGroup::GEOM_DIRTY | LLSpatialGroup::IMAGE_DIRTY);

	if (!keep_occlusion)
	{ //going to need a rebuild
		gPipeline.markRebuild(this, TRUE);
	}

	mLastUpdateTime = gFrameTimeSeconds;
	mVertexBuffer = NULL;
	mBufferMap.clear();

	clearDrawMap();

	if (!keep_occlusion)
	{
		releaseOcclusionQueryObjectNames();
	}


	for (LLSpatialGroup::element_iter i = getDataBegin(); i != getDataEnd(); ++i)
	{
		LLDrawable* drawable = (LLDrawable*)(*i)->getDrawable();
		if(!drawable)
		{
			continue;
		}
		for (S32 j = 0; j < drawable->getNumFaces(); j++)
		{
			LLFace* facep = drawable->getFace(j);
			if (facep)
			{
				facep->clearVertexBuffer();
			}
		}
	}
}

//==============================================

LLSpatialPartition::LLSpatialPartition(U32 data_mask, BOOL render_by_group, U32 buffer_usage, LLViewerRegion* regionp)
: mRenderByGroup(render_by_group), mBridge(NULL)
{
	mRegionp = regionp;		
	mPartitionType = LLViewerRegion::PARTITION_NONE;
	mVertexDataMask = data_mask;
	mBufferUsage = buffer_usage;
	mDepthMask = FALSE;
	mSlopRatio = 0.25f;
	mInfiniteFarClip = FALSE;

	new LLSpatialGroup(mOctree, this);
}


LLSpatialPartition::~LLSpatialPartition()
{
}

LLSpatialGroup *LLSpatialPartition::put(LLDrawable *drawablep, BOOL was_visible)
{
	drawablep->updateSpatialExtents();

	//keep drawable from being garbage collected
	LLPointer<LLDrawable> ptr = drawablep;
		
	if(!drawablep->getGroup())
	{
	assert_octree_valid(mOctree);
		mOctree->insert(drawablep->getEntry());
	assert_octree_valid(mOctree);
	}	
	
	LLSpatialGroup* group = drawablep->getSpatialGroup();
	llassert(group != NULL);

	if (group && was_visible && group->isOcclusionState(LLSpatialGroup::QUERY_PENDING))
	{
		group->setOcclusionState(LLSpatialGroup::DISCARD_QUERY, LLSpatialGroup::STATE_MODE_ALL_CAMERAS);
	}

	return group;
}

BOOL LLSpatialPartition::remove(LLDrawable *drawablep, LLSpatialGroup *curp)
{
	if (!curp->removeObject(drawablep))
	{
		OCT_ERRS << "Failed to remove drawable from octree!" << LL_ENDL;
	}
	else
	{
		drawablep->setGroup(NULL);
	}

	assert_octree_valid(mOctree);
	
	return TRUE;
}

void LLSpatialPartition::move(LLDrawable *drawablep, LLSpatialGroup *curp, BOOL immediate)
{
	// sanity check submitted by open source user bushing Spatula
	// who was seeing crashing here. (See VWR-424 reported by Bunny Mayne)
	if (!drawablep)
	{
		OCT_ERRS << "LLSpatialPartition::move was passed a bad drawable." << LL_ENDL;
		return;
	}
		
	BOOL was_visible = curp ? curp->isVisible() : FALSE;

	if (curp && curp->getSpatialPartition() != this)
	{
		//keep drawable from being garbage collected
		LLPointer<LLDrawable> ptr = drawablep;
		if (curp->getSpatialPartition()->remove(drawablep, curp))
		{
			put(drawablep, was_visible);
			return;
		}
		else
		{
			OCT_ERRS << "Drawable lost between spatial partitions on outbound transition." << LL_ENDL;
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
		OCT_ERRS << "Move couldn't find existing spatial group!" << LL_ENDL;
	}

	put(drawablep, was_visible);
}

class LLSpatialShift : public OctreeTraveler
{
public:
	const LLVector4a& mOffset;

	LLSpatialShift(const LLVector4a& offset) : mOffset(offset) { }
	virtual void visit(const OctreeNode* branch) 
	{ 
		((LLSpatialGroup*) branch->getListener(0))->shift(mOffset); 
	}
};

void LLSpatialPartition::shift(const LLVector4a &offset)
{ //shift octree node bounding boxes by offset
	LLSpatialShift shifter(offset);
	shifter.traverse(mOctree);
}

class LLOctreeCull : public LLViewerOctreeCull
{
public:
	LLOctreeCull(LLCamera* camera) : LLViewerOctreeCull(camera) {}

	virtual bool earlyFail(LLViewerOctreeGroup* base_group)
	{
		LLSpatialGroup* group = (LLSpatialGroup*)base_group;
		group->checkOcclusion();

		if (group->getOctreeNode()->getParent() &&	//never occlusion cull the root node
		  	LLPipeline::sUseOcclusion &&			//ignore occlusion if disabled
			group->isOcclusionState(LLSpatialGroup::OCCLUDED))
		{
			gPipeline.markOccluder(group);
			return true;
		}
		
		return false;
	}
	
	virtual S32 frustumCheck(const LLViewerOctreeGroup* group)
	{
		S32 res = AABBInFrustumNoFarClipGroupBounds(group);
		if (res != 0)
		{
			res = llmin(res, AABBSphereIntersectGroupExtents(group));
		}
		return res;
	}

	virtual S32 frustumCheckObjects(const LLViewerOctreeGroup* group)
	{
		S32 res = AABBInFrustumNoFarClipObjectBounds(group);
		if (res != 0)
		{
			res = llmin(res, AABBSphereIntersectObjectExtents(group));
		}
		return res;
	}

	virtual void processGroup(LLViewerOctreeGroup* base_group)
	{
		LLSpatialGroup* group = (LLSpatialGroup*)base_group;
		if (group->needsUpdate() ||
			group->getVisible(LLViewerCamera::sCurCameraID) < LLDrawable::getCurrentFrame() - 1)
		{
			group->doOcclusion(mCamera);
		}
		gPipeline.markNotCulled(group, *mCamera);
	}
};

class LLOctreeCullNoFarClip : public LLOctreeCull
{
public: 
	LLOctreeCullNoFarClip(LLCamera* camera) 
		: LLOctreeCull(camera) { }

	virtual S32 frustumCheck(const LLViewerOctreeGroup* group)
	{
		return AABBInFrustumNoFarClipGroupBounds(group);
	}

	virtual S32 frustumCheckObjects(const LLViewerOctreeGroup* group)
	{
		S32 res = AABBInFrustumNoFarClipObjectBounds(group);
		return res;
	}
};

class LLOctreeCullShadow : public LLOctreeCull
{
public:
	LLOctreeCullShadow(LLCamera* camera)
		: LLOctreeCull(camera) { }

	virtual S32 frustumCheck(const LLViewerOctreeGroup* group)
	{
		return AABBInFrustumGroupBounds(group);
	}

	virtual S32 frustumCheckObjects(const LLViewerOctreeGroup* group)
	{
		return AABBInFrustumObjectBounds(group);
	}
};

class LLOctreeCullVisExtents: public LLOctreeCullShadow
{
public:
	LLOctreeCullVisExtents(LLCamera* camera, LLVector4a& min, LLVector4a& max)
		: LLOctreeCullShadow(camera), mMin(min), mMax(max), mEmpty(TRUE) { }

	virtual bool earlyFail(LLViewerOctreeGroup* base_group)
	{
		LLSpatialGroup* group = (LLSpatialGroup*)base_group;

		if (group->getOctreeNode()->getParent() &&	//never occlusion cull the root node
			LLPipeline::sUseOcclusion &&			//ignore occlusion if disabled
			group->isOcclusionState(LLSpatialGroup::OCCLUDED))
		{
			return true;
		}
		
		return false;
	}

	virtual void traverse(const OctreeNode* n)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) n->getListener(0);

		if (earlyFail(group))
		{
			return;
		}
		
		if ((mRes && group->hasState(LLSpatialGroup::SKIP_FRUSTUM_CHECK)) ||
			mRes == 2)
		{	//don't need to do frustum check
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

	virtual void processGroup(LLViewerOctreeGroup* base_group)
	{
		LLSpatialGroup* group = (LLSpatialGroup*)base_group;
		
		llassert(!group->hasState(LLSpatialGroup::DIRTY) && !group->isEmpty());
		
		if (mRes < 2)
		{
			if (AABBInFrustumObjectBounds(group) > 0)
			{
				mEmpty = FALSE;
				const LLVector4a* exts = group->getObjectExtents();
				update_min_max(mMin, mMax, exts[0]);
				update_min_max(mMin, mMax, exts[1]);
			}
		}
		else
		{
			mEmpty = FALSE;
			const LLVector4a* exts = group->getExtents();
			update_min_max(mMin, mMax, exts[0]);
			update_min_max(mMin, mMax, exts[1]);
		}
	}

	BOOL mEmpty;
	LLVector4a& mMin;
	LLVector4a& mMax;
};

class LLOctreeCullDetectVisible: public LLOctreeCullShadow
{
public:
	LLOctreeCullDetectVisible(LLCamera* camera)
		: LLOctreeCullShadow(camera), mResult(FALSE) { }

	virtual bool earlyFail(LLViewerOctreeGroup* base_group)
	{
		LLSpatialGroup* group = (LLSpatialGroup*)base_group;

		if (mResult || //already found a node, don't check any more
			(group->getOctreeNode()->getParent() &&	//never occlusion cull the root node
			 LLPipeline::sUseOcclusion &&			//ignore occlusion if disabled
			 group->isOcclusionState(LLSpatialGroup::OCCLUDED)))
		{
			return true;
		}
		
		return false;
	}

	virtual void processGroup(LLViewerOctreeGroup* base_group)
	{
		if (base_group->isVisible())
		{
			mResult = TRUE;
		}
	}

	BOOL mResult;
};

class LLOctreeSelect : public LLOctreeCull
{
public:
	LLOctreeSelect(LLCamera* camera, std::vector<LLDrawable*>* results)
		: LLOctreeCull(camera), mResults(results) { }

	virtual bool earlyFail(LLViewerOctreeGroup* group) { return false; }
	virtual void preprocess(LLViewerOctreeGroup* group) { }

	virtual void processGroup(LLViewerOctreeGroup* base_group)
	{
		LLSpatialGroup* group = (LLSpatialGroup*)base_group;
		OctreeNode* branch = group->getOctreeNode();

		for (OctreeNode::const_element_iter i = branch->getDataBegin(); i != branch->getDataEnd(); ++i)
		{
			LLDrawable* drawable = (LLDrawable*)(*i)->getDrawable();
			if(!drawable)
		{
				continue;
			}
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

void drawBox(const LLVector3& c, const LLVector3& r)
{
	LLVertexBuffer::unbind();

	gGL.begin(LLRender::TRIANGLE_STRIP);
	//left front
	gGL.vertex3fv((c+r.scaledVec(LLVector3(-1,1,-1))).mV);
	gGL.vertex3fv((c+r.scaledVec(LLVector3(-1,1,1))).mV);
	//right front
	gGL.vertex3fv((c+r.scaledVec(LLVector3(1,1,-1))).mV);
	gGL.vertex3fv((c+r.scaledVec(LLVector3(1,1,1))).mV);
	//right back
 	gGL.vertex3fv((c+r.scaledVec(LLVector3(1,-1,-1))).mV);
	gGL.vertex3fv((c+r.scaledVec(LLVector3(1,-1,1))).mV);
	//left back
	gGL.vertex3fv((c+r.scaledVec(LLVector3(-1,-1,-1))).mV);
	gGL.vertex3fv((c+r.scaledVec(LLVector3(-1,-1,1))).mV);
	//left front
	gGL.vertex3fv((c+r.scaledVec(LLVector3(-1,1,-1))).mV);
	gGL.vertex3fv((c+r.scaledVec(LLVector3(-1,1,1))).mV);
	gGL.end();
	
	//bottom
	gGL.begin(LLRender::TRIANGLE_STRIP);
	gGL.vertex3fv((c+r.scaledVec(LLVector3(1,1,-1))).mV);
	gGL.vertex3fv((c+r.scaledVec(LLVector3(1,-1,-1))).mV);
	gGL.vertex3fv((c+r.scaledVec(LLVector3(-1,1,-1))).mV);
	gGL.vertex3fv((c+r.scaledVec(LLVector3(-1,-1,-1))).mV);
	gGL.end();

	//top
	gGL.begin(LLRender::TRIANGLE_STRIP);
	gGL.vertex3fv((c+r.scaledVec(LLVector3(1,1,1))).mV);
	gGL.vertex3fv((c+r.scaledVec(LLVector3(-1,1,1))).mV);
	gGL.vertex3fv((c+r.scaledVec(LLVector3(1,-1,1))).mV);
	gGL.vertex3fv((c+r.scaledVec(LLVector3(-1,-1,1))).mV);
	gGL.end();	
}

void drawBox(const LLVector4a& c, const LLVector4a& r)
{
	drawBox(reinterpret_cast<const LLVector3&>(c), reinterpret_cast<const LLVector3&>(r));
}

void drawBoxOutline(const LLVector3& pos, const LLVector3& size)
{

	llassert(pos.isFinite());
	llassert(size.isFinite());

	llassert(!llisnan(pos.mV[0]));
	llassert(!llisnan(pos.mV[1]));
	llassert(!llisnan(pos.mV[2]));

	llassert(!llisnan(size.mV[0]));
	llassert(!llisnan(size.mV[1]));
	llassert(!llisnan(size.mV[2]));

	LLVector3 v1 = size.scaledVec(LLVector3( 1, 1,1));
	LLVector3 v2 = size.scaledVec(LLVector3(-1, 1,1));
	LLVector3 v3 = size.scaledVec(LLVector3(-1,-1,1));
	LLVector3 v4 = size.scaledVec(LLVector3( 1,-1,1));

	gGL.begin(LLRender::LINES); 
	
	//top
	gGL.vertex3fv((pos+v1).mV);
	gGL.vertex3fv((pos+v2).mV);
	gGL.vertex3fv((pos+v2).mV);
	gGL.vertex3fv((pos+v3).mV);
	gGL.vertex3fv((pos+v3).mV);
	gGL.vertex3fv((pos+v4).mV);
	gGL.vertex3fv((pos+v4).mV);
	gGL.vertex3fv((pos+v1).mV);
	
	//bottom
	gGL.vertex3fv((pos-v1).mV);
	gGL.vertex3fv((pos-v2).mV);
	gGL.vertex3fv((pos-v2).mV);
	gGL.vertex3fv((pos-v3).mV);
	gGL.vertex3fv((pos-v3).mV);
	gGL.vertex3fv((pos-v4).mV);
	gGL.vertex3fv((pos-v4).mV);
	gGL.vertex3fv((pos-v1).mV);
	
	//right
	gGL.vertex3fv((pos+v1).mV);
	gGL.vertex3fv((pos-v3).mV);
			
	gGL.vertex3fv((pos+v4).mV);
	gGL.vertex3fv((pos-v2).mV);

	//left
	gGL.vertex3fv((pos+v2).mV);
	gGL.vertex3fv((pos-v4).mV);

	gGL.vertex3fv((pos+v3).mV);
	gGL.vertex3fv((pos-v1).mV);

	gGL.end();
}

void drawBoxOutline(const LLVector4a& pos, const LLVector4a& size)
{
	drawBoxOutline(reinterpret_cast<const LLVector3&>(pos), reinterpret_cast<const LLVector3&>(size));
}

class LLOctreeDirty : public OctreeTraveler
{
public:
	LLOctreeDirty(bool no_rebuild) : mNoRebuild(no_rebuild){}

	virtual void visit(const OctreeNode* state)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) state->getListener(0);
		group->destroyGL();

		for (LLSpatialGroup::element_iter i = group->getDataBegin(); i != group->getDataEnd(); ++i)
		{
			LLDrawable* drawable = (LLDrawable*)(*i)->getDrawable();
			if(!drawable)
			{
				continue;
			}
			if (!mNoRebuild && drawable->getVObj().notNull() && !group->getSpatialPartition()->mRenderByGroup)
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

private:
	BOOL mNoRebuild;
};

void LLSpatialPartition::restoreGL()
{
}

void LLSpatialPartition::resetVertexBuffers()
{
	LLOctreeDirty dirty(sTeleportRequested);
	dirty.traverse(mOctree);
}

BOOL LLSpatialPartition::getVisibleExtents(LLCamera& camera, LLVector3& visMin, LLVector3& visMax)
{
	LLVector4a visMina, visMaxa;
	visMina.load3(visMin.mV);
	visMaxa.load3(visMax.mV);

	{
		LL_RECORD_BLOCK_TIME(FTM_CULL_REBOUND);		
		LLSpatialGroup* group = (LLSpatialGroup*) mOctree->getListener(0);
		group->rebound();
	}

	LLOctreeCullVisExtents vis(&camera, visMina, visMaxa);
	vis.traverse(mOctree);

	visMin.set(visMina.getF32ptr());
	visMax.set(visMaxa.getF32ptr());
	return vis.mEmpty;
}

BOOL LLSpatialPartition::visibleObjectsInFrustum(LLCamera& camera)
{
	LLOctreeCullDetectVisible vis(&camera);
	vis.traverse(mOctree);
	return vis.mResult;
}

S32 LLSpatialPartition::cull(LLCamera &camera, std::vector<LLDrawable *>* results, BOOL for_select)
{
#if LL_OCTREE_PARANOIA_CHECK
	((LLSpatialGroup*)mOctree->getListener(0))->checkStates();
#endif
	{
		LL_RECORD_BLOCK_TIME(FTM_CULL_REBOUND);		
		LLSpatialGroup* group = (LLSpatialGroup*) mOctree->getListener(0);
		group->rebound();
	}

#if LL_OCTREE_PARANOIA_CHECK
	((LLSpatialGroup*)mOctree->getListener(0))->validate();
#endif

		LLOctreeSelect selecter(&camera, results);
		selecter.traverse(mOctree);
	
	return 0;
	}
	
S32 LLSpatialPartition::cull(LLCamera &camera, bool do_occlusion)
{
#if LL_OCTREE_PARANOIA_CHECK
	((LLSpatialGroup*)mOctree->getListener(0))->checkStates();
#endif
	{
		LL_RECORD_BLOCK_TIME(FTM_CULL_REBOUND);		
		LLSpatialGroup* group = (LLSpatialGroup*) mOctree->getListener(0);
		group->rebound();
	}

#if LL_OCTREE_PARANOIA_CHECK
	((LLSpatialGroup*)mOctree->getListener(0))->validate();
#endif

	if (LLPipeline::sShadowRender)
	{
		LL_RECORD_BLOCK_TIME(FTM_FRUSTUM_CULL);
		LLOctreeCullShadow culler(&camera);
		culler.traverse(mOctree);
	}
	else if (mInfiniteFarClip || !LLPipeline::sUseFarClip)
	{
		LL_RECORD_BLOCK_TIME(FTM_FRUSTUM_CULL);		
		LLOctreeCullNoFarClip culler(&camera);
		culler.traverse(mOctree);
	}
	else
	{
		LL_RECORD_BLOCK_TIME(FTM_FRUSTUM_CULL);		
		LLOctreeCull culler(&camera);
		culler.traverse(mOctree);
	}
	
	return 0;
}

void pushVerts(LLDrawInfo* params, U32 mask)
{
	LLRenderPass::applyModelMatrix(*params);
	params->mVertexBuffer->setBuffer(mask);
	params->mVertexBuffer->drawRange(params->mParticle ? LLRender::POINTS : LLRender::TRIANGLES,
								params->mStart, params->mEnd, params->mCount, params->mOffset);
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

void pushVerts(LLFace* face, U32 mask)
{
	if (face)
	{
		llassert(face->verify());

		LLVertexBuffer* buffer = face->getVertexBuffer();

		if (buffer && (face->getGeomCount() >= 3))
		{
			buffer->setBuffer(mask);
			U16 start = face->getGeomStart();
			U16 end = start + face->getGeomCount()-1;
			U32 count = face->getIndicesCount();
			U16 offset = face->getIndicesStart();
			buffer->drawRange(LLRender::TRIANGLES, start, end, count, offset);
		}
	}
}

void pushVerts(LLDrawable* drawable, U32 mask)
{
	for (S32 i = 0; i < drawable->getNumFaces(); ++i)
	{
		pushVerts(drawable->getFace(i), mask);
	}
}

void pushVerts(LLVolume* volume)
{
	LLVertexBuffer::unbind();
	for (S32 i = 0; i < volume->getNumVolumeFaces(); ++i)
	{
		const LLVolumeFace& face = volume->getVolumeFace(i);
		LLVertexBuffer::drawElements(LLRender::TRIANGLES, face.mPositions, NULL, face.mNumIndices, face.mIndices);
	}
}

void pushBufferVerts(LLVertexBuffer* buffer, U32 mask)
{
	if (buffer)
	{
		buffer->setBuffer(mask);
		buffer->drawRange(LLRender::TRIANGLES, 0, buffer->getNumVerts()-1, buffer->getNumIndices(), 0);
	}
}

void pushBufferVerts(LLSpatialGroup* group, U32 mask, bool push_alpha = true)
{
	if (group->getSpatialPartition()->mRenderByGroup)
	{
		if (!group->mDrawMap.empty())
		{
			LLDrawInfo* params = *(group->mDrawMap.begin()->second.begin());
			LLRenderPass::applyModelMatrix(*params);
		
			if (push_alpha)
			{
				pushBufferVerts(group->mVertexBuffer, mask);
			}

			for (LLSpatialGroup::buffer_map_t::iterator i = group->mBufferMap.begin(); i != group->mBufferMap.end(); ++i)
			{
				for (LLSpatialGroup::buffer_texture_map_t::iterator j = i->second.begin(); j != i->second.end(); ++j)
				{
					for (LLSpatialGroup::buffer_list_t::iterator k = j->second.begin(); k != j->second.end(); ++k)
					{
						pushBufferVerts(*k, mask);
					}
				}
			}
		}
	}
	/*else
	{
		//const LLVector4a* bounds = group->getBounds();
		//drawBox(bounds[0], bounds[1]);
	}*/
}

void pushVertsColorCoded(LLSpatialGroup* group, U32 mask)
{
	LLDrawInfo* params = NULL;

	static const LLColor4 colors[] = {
		LLColor4::green,
		LLColor4::green1,
		LLColor4::green2,
		LLColor4::green3,
		LLColor4::green4,
		LLColor4::green5,
		LLColor4::green6
	};
		
	static const U32 col_count = LL_ARRAY_SIZE(colors);

	U32 col = 0;

	for (LLSpatialGroup::draw_map_t::iterator i = group->mDrawMap.begin(); i != group->mDrawMap.end(); ++i)
	{
		for (LLSpatialGroup::drawmap_elem_t::iterator j = i->second.begin(); j != i->second.end(); ++j) 
		{
			params = *j;
			LLRenderPass::applyModelMatrix(*params);
			gGL.diffuseColor4f(colors[col].mV[0], colors[col].mV[1], colors[col].mV[2], 0.5f);
			params->mVertexBuffer->setBuffer(mask);
			params->mVertexBuffer->drawRange(params->mParticle ? LLRender::POINTS : LLRender::TRIANGLES,
				params->mStart, params->mEnd, params->mCount, params->mOffset);
			col = (col+1)%col_count;
		}
	}
}

void renderOctree(LLSpatialGroup* group)
{
	//render solid object bounding box, color
	//coded by buffer usage and activity
	gGL.setSceneBlendType(LLRender::BT_ADD_WITH_ALPHA);
	LLVector4 col;
	if (group->mBuilt > 0.f)
	{
		group->mBuilt -= 2.f * gFrameIntervalSeconds.value();
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
			LLGLDepthTest gl_depth(FALSE, FALSE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			gGL.diffuseColor4f(1,0,0,group->mBuilt);
			gGL.flush();
			glLineWidth(5.f);

			const LLVector4a* bounds = group->getObjectBounds();
			drawBoxOutline(bounds[0], bounds[1]);
			gGL.flush();
			glLineWidth(1.f);
			gGL.flush();
			for (LLSpatialGroup::element_iter i = group->getDataBegin(); i != group->getDataEnd(); ++i)
			{
				LLDrawable* drawable = (LLDrawable*)(*i)->getDrawable();
				if(!drawable)
				{
					continue;
				}
				if (!group->getSpatialPartition()->isBridge())
				{
					gGL.pushMatrix();
					LLVector3 trans = drawable->getRegion()->getOriginAgent();
					gGL.translatef(trans.mV[0], trans.mV[1], trans.mV[2]);
				}
				
				for (S32 j = 0; j < drawable->getNumFaces(); j++)
				{
					LLFace* face = drawable->getFace(j);
					if (face && face->getVertexBuffer())
					{
						if (gFrameTimeSeconds - face->mLastUpdateTime < 0.5f)
						{
							gGL.diffuseColor4f(0, 1, 0, group->mBuilt);
						}
						else if (gFrameTimeSeconds - face->mLastMoveTime < 0.5f)
						{
							gGL.diffuseColor4f(1, 0, 0, group->mBuilt);
						}
						else
						{
							continue;
						}

						face->getVertexBuffer()->setBuffer(LLVertexBuffer::MAP_VERTEX);
						//drawBox((face->mExtents[0] + face->mExtents[1])*0.5f,
						//		(face->mExtents[1]-face->mExtents[0])*0.5f);
						face->getVertexBuffer()->draw(LLRender::TRIANGLES, face->getIndicesCount(), face->getIndicesStart());
					}
				}

				if (!group->getSpatialPartition()->isBridge())
				{
					gGL.popMatrix();
				}
			}
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			gGL.diffuseColor4f(1,1,1,1);
		}
	}
	else
	{
		if (group->mBufferUsage == GL_STATIC_DRAW_ARB && !group->isEmpty() 
			&& group->getSpatialPartition()->mRenderByGroup)
		{
			col.setVec(0.8f, 0.4f, 0.1f, 0.1f);
		}
		else
		{
			col.setVec(0.1f, 0.1f, 1.f, 0.1f);
		}
	}

	gGL.diffuseColor4fv(col.mV);
	LLVector4a fudge;
	fudge.splat(0.001f);

	//LLVector4a size = group->mObjectBounds[1];
	//size.mul(1.01f);
	//size.add(fudge);

	//{
	//	LLGLDepthTest depth(GL_TRUE, GL_FALSE);
	//	drawBox(group->mObjectBounds[0], fudge);
	//}
	
	gGL.setSceneBlendType(LLRender::BT_ALPHA);

	//if (group->mBuilt <= 0.f)
	{
		//draw opaque outline
		//gGL.diffuseColor4f(col.mV[0], col.mV[1], col.mV[2], 1.f);
		//drawBoxOutline(group->mObjectBounds[0], group->mObjectBounds[1]);

		gGL.diffuseColor4f(0,1,1,1);

		const LLVector4a* bounds = group->getBounds();
		drawBoxOutline(bounds[0], bounds[1]);
		
		//draw bounding box for draw info
		/*if (group->getSpatialPartition()->mRenderByGroup)
		{
			gGL.diffuseColor4f(1.0f, 0.75f, 0.25f, 0.6f);
			for (LLSpatialGroup::draw_map_t::iterator i = group->mDrawMap.begin(); i != group->mDrawMap.end(); ++i)
			{
				for (LLSpatialGroup::drawmap_elem_t::iterator j = i->second.begin(); j != i->second.end(); ++j)
				{
					LLDrawInfo* draw_info = *j;
					LLVector4a center;
					center.setAdd(draw_info->mExtents[1], draw_info->mExtents[0]);
					center.mul(0.5f);
					LLVector4a size;
					size.setSub(draw_info->mExtents[1], draw_info->mExtents[0]);
					size.mul(0.5f);
					drawBoxOutline(center, size);
				}
			}
		}*/
	}
	
//	LLSpatialGroup::OctreeNode* node = group->mOctreeNode;
//	gGL.diffuseColor4f(0,1,0,1);
//	drawBoxOutline(LLVector3(node->getCenter()), LLVector3(node->getSize()));
}

std::set<LLSpatialGroup*> visible_selected_groups;

void renderVisibility(LLSpatialGroup* group, LLCamera* camera)
{
	/*LLGLEnable blend(GL_BLEND);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
	LLGLEnable cull(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);*/

	/*BOOL render_objects = (!LLPipeline::sUseOcclusion || !group->isOcclusionState(LLSpatialGroup::OCCLUDED)) && group->isVisible() &&
							!group->isEmpty();


	if (render_objects)
	{
		LLGLDepthTest depth(GL_TRUE, GL_FALSE);

		LLGLDisable blend(GL_BLEND);
		gGL.diffuseColor4f(0.f, 0.75f, 0.f,0.5f);
		pushBufferVerts(group, LLVertexBuffer::MAP_VERTEX, false);
		
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(4.f);
		gGL.diffuseColor4f(0.f, 0.5f, 0.f, 1.f);
		pushBufferVerts(group, LLVertexBuffer::MAP_VERTEX, false);
		glLineWidth(1.f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		bool selected = false;
		
		for (LLSpatialGroup::element_iter iter = group->getDataBegin(); iter != group->getDataEnd(); ++iter)
		{
			LLDrawable* drawable = *iter;
			if (drawable->getVObj().notNull() && drawable->getVObj()->isSelected())
			{
				selected = true;
				break;
			}
		}
		
		if (selected)
		{ //store for rendering occlusion volume as overlay
			visible_selected_groups.insert(group);
		}
	}*/		

	/*if (render_objects)
	{
		LLGLDepthTest depth_under(GL_TRUE, GL_FALSE, GL_GREATER);
		gGL.diffuseColor4f(0, 0.5f, 0, 0.5f);
		gGL.diffuseColor4f(0, 0.5f, 0, 0.5f);
		pushBufferVerts(group, LLVertexBuffer::MAP_VERTEX);
	}

	{
		LLGLDepthTest depth_over(GL_TRUE, GL_FALSE, GL_LEQUAL);

		if (render_objects)
		{
			gGL.diffuseColor4f(0.f, 0.5f, 0.f,1.f);
			gGL.diffuseColor4f(0.f, 0.5f, 0.f, 1.f);
			pushBufferVerts(group, LLVertexBuffer::MAP_VERTEX);
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		if (render_objects)
		{
			gGL.diffuseColor4f(0.f, 0.75f, 0.f,0.5f);
			gGL.diffuseColor4f(0.f, 0.75f, 0.f, 0.5f);
			pushBufferVerts(group, LLVertexBuffer::MAP_VERTEX);
		
			bool selected = false;
		
			for (LLSpatialGroup::element_iter iter = group->getDataBegin(); iter != group->getDataEnd(); ++iter)
			{
				LLDrawable* drawable = *iter;
				if (drawable->getVObj().notNull() && drawable->getVObj()->isSelected())
				{
					selected = true;
					break;
				}
			}
		
			if (selected)
			{ //store for rendering occlusion volume as overlay
				visible_selected_groups.insert(group);
			}
		}		
	}*/
}

void renderXRay(LLSpatialGroup* group, LLCamera* camera)
{
	BOOL render_objects = (!LLPipeline::sUseOcclusion || !group->isOcclusionState(LLSpatialGroup::OCCLUDED)) && group->isVisible() &&
							!group->isEmpty();
	
	if (render_objects)
	{
		pushBufferVerts(group, LLVertexBuffer::MAP_VERTEX, false);

		bool selected = false;

		for (LLSpatialGroup::element_iter iter = group->getDataBegin(); iter != group->getDataEnd(); ++iter)
		{
			LLDrawable* drawable = (LLDrawable*)(*iter)->getDrawable();
			if (drawable->getVObj().notNull() && drawable->getVObj()->isSelected())
			{
				selected = true;
				break;
			}
		}

		if (selected)
		{ //store for rendering occlusion volume as overlay

			if (!group->getSpatialPartition()->isBridge())
			{
				visible_selected_groups.insert(group);
			}
			else
			{
				visible_selected_groups.insert(group->getSpatialPartition()->asBridge()->getSpatialGroup());
			}
		}
	}
}

void renderCrossHairs(LLVector3 position, F32 size, LLColor4 color)
{
	gGL.color4fv(color.mV);
	gGL.begin(LLRender::LINES);
	{
		gGL.vertex3fv((position - LLVector3(size, 0.f, 0.f)).mV);
		gGL.vertex3fv((position + LLVector3(size, 0.f, 0.f)).mV);
		gGL.vertex3fv((position - LLVector3(0.f, size, 0.f)).mV);
		gGL.vertex3fv((position + LLVector3(0.f, size, 0.f)).mV);
		gGL.vertex3fv((position - LLVector3(0.f, 0.f, size)).mV);
		gGL.vertex3fv((position + LLVector3(0.f, 0.f, size)).mV);
	}
	gGL.end();
}

void renderUpdateType(LLDrawable* drawablep)
{
	LLViewerObject* vobj = drawablep->getVObj();
	if (!vobj || OUT_UNKNOWN == vobj->getLastUpdateType())
	{
		return;
	}
	LLGLEnable blend(GL_BLEND);
	switch (vobj->getLastUpdateType())
	{
	case OUT_FULL:
		gGL.diffuseColor4f(0,1,0,0.5f);
		break;
	case OUT_TERSE_IMPROVED:
		gGL.diffuseColor4f(0,1,1,0.5f);
		break;
	case OUT_FULL_COMPRESSED:
		if (vobj->getLastUpdateCached())
		{
			gGL.diffuseColor4f(1,0,0,0.5f);
		}
		else
		{
			gGL.diffuseColor4f(1,1,0,0.5f);
		}
		break;
	case OUT_FULL_CACHED:
		gGL.diffuseColor4f(0,0,1,0.5f);
		break;
	default:
		LL_WARNS() << "Unknown update_type " << vobj->getLastUpdateType() << LL_ENDL;
		break;
	};
	S32 num_faces = drawablep->getNumFaces();
	if (num_faces)
	{
		for (S32 i = 0; i < num_faces; ++i)
		{
			pushVerts(drawablep->getFace(i), LLVertexBuffer::MAP_VERTEX);
		}
	}
}

void renderComplexityDisplay(LLDrawable* drawablep)
{
	LLViewerObject* vobj = drawablep->getVObj();
	if (!vobj)
	{
		return;
	}

	LLVOVolume *voVol = dynamic_cast<LLVOVolume*>(vobj);

	if (!voVol)
	{
		return;
	}

	if (!voVol->isRoot())
	{
		return;
	}

	LLVOVolume::texture_cost_t textures;
	F32 cost = (F32) voVol->getRenderCost(textures);

	// add any child volumes
	LLViewerObject::const_child_list_t children = voVol->getChildren();
	for (LLViewerObject::const_child_list_t::const_iterator iter = children.begin(); iter != children.end(); ++iter)
	{
		const LLViewerObject *child = *iter;
		const LLVOVolume *child_volume = dynamic_cast<const LLVOVolume*>(child);
		if (child_volume)
		{
			cost += child_volume->getRenderCost(textures);
		}
	}

	// add texture cost
	for (LLVOVolume::texture_cost_t::iterator iter = textures.begin(); iter != textures.end(); ++iter)
	{
		// add the cost of each individual texture in the linkset
		cost += iter->second;
	}

	F32 cost_max = (F32) LLVOVolume::getRenderComplexityMax();



	// allow user to set a static color scale
	if (gSavedSettings.getS32("RenderComplexityStaticMax") > 0)
	{
		cost_max = gSavedSettings.getS32("RenderComplexityStaticMax");
	}

	F32 cost_ratio = cost / cost_max;
	
	// cap cost ratio at 1.0f in case cost_max is at a low threshold
	cost_ratio = cost_ratio > 1.0f ? 1.0f : cost_ratio;
	
	LLGLEnable blend(GL_BLEND);

	LLColor4 color;
	const LLColor4 color_min = gSavedSettings.getColor4("RenderComplexityColorMin");
	const LLColor4 color_mid = gSavedSettings.getColor4("RenderComplexityColorMid");
	const LLColor4 color_max = gSavedSettings.getColor4("RenderComplexityColorMax");

	if (cost_ratio < 0.5f)
	{
		color = color_min * (1 - cost_ratio * 2) + color_mid * (cost_ratio * 2);
	}
	else
	{
		color = color_mid * (1 - (cost_ratio - 0.5) * 2) + color_max * ((cost_ratio - 0.5) * 2);
	}

	LLSD color_val = color.getValue();

	// don't highlight objects below the threshold
	if (cost > gSavedSettings.getS32("RenderComplexityThreshold"))
	{
		glColor4f(color[0],color[1],color[2],0.5f);


		S32 num_faces = drawablep->getNumFaces();
		if (num_faces)
		{
			for (S32 i = 0; i < num_faces; ++i)
			{
				pushVerts(drawablep->getFace(i), LLVertexBuffer::MAP_VERTEX);
			}
		}
		LLViewerObject::const_child_list_t children = voVol->getChildren();
		for (LLViewerObject::const_child_list_t::const_iterator iter = children.begin(); iter != children.end(); ++iter)
		{
			const LLViewerObject *child = *iter;
			if (child)
			{
				num_faces = child->getNumFaces();
				if (num_faces)
				{
					for (S32 i = 0; i < num_faces; ++i)
					{
						pushVerts(child->mDrawable->getFace(i), LLVertexBuffer::MAP_VERTEX);
					}
				}
			}
		}
	}
	
	voVol->setDebugText(llformat("%4.0f", cost));	
}

void renderBoundingBox(LLDrawable* drawable, BOOL set_color = TRUE)
{
	if (set_color)
	{
		if (drawable->isSpatialBridge())
		{
			gGL.diffuseColor4f(1,0.5f,0,1);
		}
		else if (drawable->getVOVolume())
		{
			if (drawable->isRoot())
			{
				gGL.diffuseColor4f(1,1,0,1);
			}
			else
			{
				gGL.diffuseColor4f(0,1,0,1);
			}
		}
		else if (drawable->getVObj())
		{
			switch (drawable->getVObj()->getPCode())
			{
				case LLViewerObject::LL_VO_SURFACE_PATCH:
						gGL.diffuseColor4f(0,1,1,1);
						break;
				case LLViewerObject::LL_VO_CLOUDS:
						// no longer used
						break;
				case LLViewerObject::LL_VO_PART_GROUP:
				case LLViewerObject::LL_VO_HUD_PART_GROUP:
						gGL.diffuseColor4f(0,0,1,1);
						break;
				case LLViewerObject::LL_VO_VOID_WATER:
				case LLViewerObject::LL_VO_WATER:
						gGL.diffuseColor4f(0,0.5f,1,1);
						break;
				case LL_PCODE_LEGACY_TREE:
						gGL.diffuseColor4f(0,0.5f,0,1);
						break;
				default:
						gGL.diffuseColor4f(1,0,1,1);
						break;
			}
		}
		else 
		{
			gGL.diffuseColor4f(1,0,0,1);
		}
	}

	const LLVector4a* ext;
	LLVector4a pos, size;

	if (drawable->getVOVolume())
	{
		//render face bounding boxes
		for (S32 i = 0; i < drawable->getNumFaces(); i++)
		{
			LLFace* facep = drawable->getFace(i);
			if (facep)
			{
				ext = facep->mExtents;

				pos.setAdd(ext[0], ext[1]);
				pos.mul(0.5f);
				size.setSub(ext[1], ext[0]);
				size.mul(0.5f);
		
				drawBoxOutline(pos,size);
			}
		}
	}

	//render drawable bounding box
	ext = drawable->getSpatialExtents();

	pos.setAdd(ext[0], ext[1]);
	pos.mul(0.5f);
	size.setSub(ext[1], ext[0]);
	size.mul(0.5f);
	
	LLViewerObject* vobj = drawable->getVObj();
	if (vobj && vobj->onActiveList())
	{
		gGL.flush();
		glLineWidth(llmax(4.f*sinf(gFrameTimeSeconds*2.f)+1.f, 1.f));
		//glLineWidth(4.f*(sinf(gFrameTimeSeconds*2.f)*0.25f+0.75f));
		stop_glerror();
		drawBoxOutline(pos,size);
		gGL.flush();
		glLineWidth(1.f);
	}
	else
	{
		drawBoxOutline(pos,size);
	}
}

void renderNormals(LLDrawable* drawablep)
{
	LLVertexBuffer::unbind();

	LLVOVolume* vol = drawablep->getVOVolume();
	if (vol)
	{
		LLVolume* volume = vol->getVolume();
		gGL.pushMatrix();
		gGL.multMatrix((F32*) vol->getRelativeXform().mMatrix);
		
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		LLVector4a scale(gSavedSettings.getF32("RenderDebugNormalScale"));

		for (S32 i = 0; i < volume->getNumVolumeFaces(); ++i)
		{
			const LLVolumeFace& face = volume->getVolumeFace(i);

			for (S32 j = 0; j < face.mNumVertices; ++j)
			{
				gGL.begin(LLRender::LINES);
				LLVector4a n,p;
				
				n.setMul(face.mNormals[j], scale);
				p.setAdd(face.mPositions[j], n);
				
				gGL.diffuseColor4f(1,1,1,1);
				gGL.vertex3fv(face.mPositions[j].getF32ptr());
				gGL.vertex3fv(p.getF32ptr());
				
				if (face.mTangents)
				{
					n.setMul(face.mTangents[j], scale);
					p.setAdd(face.mPositions[j], n);
				
					gGL.diffuseColor4f(0,1,1,1);
					gGL.vertex3fv(face.mPositions[j].getF32ptr());
					gGL.vertex3fv(p.getF32ptr());
				}	
				gGL.end();
			}
		}

		gGL.popMatrix();
	}
}

S32 get_physics_detail(const LLVolumeParams& volume_params, const LLVector3& scale)
{
	const S32 DEFAULT_DETAIL = 1;
	const F32 LARGE_THRESHOLD = 5.f;
	const F32 MEGA_THRESHOLD = 25.f;

	S32 detail = DEFAULT_DETAIL;
	F32 avg_scale = (scale[0]+scale[1]+scale[2])/3.f;

	if (avg_scale > LARGE_THRESHOLD)
	{
		detail += 1;
		if (avg_scale > MEGA_THRESHOLD)
		{
			detail += 1;
		}
	}

	return detail;
}

void renderMeshBaseHull(LLVOVolume* volume, U32 data_mask, LLColor4& color, LLColor4& line_color)
{
	LLUUID mesh_id = volume->getVolume()->getParams().getSculptID();
	LLModel::Decomposition* decomp = gMeshRepo.getDecomposition(mesh_id);

	const LLVector3 center(0,0,0);
	const LLVector3 size(0.25f,0.25f,0.25f);

	if (decomp)
	{		
		if (!decomp->mBaseHullMesh.empty())
		{
			gGL.diffuseColor4fv(color.mV);
			LLVertexBuffer::drawArrays(LLRender::TRIANGLES, decomp->mBaseHullMesh.mPositions, decomp->mBaseHullMesh.mNormals);
		}
		else
		{
			gMeshRepo.buildPhysicsMesh(*decomp);
			gGL.diffuseColor4f(0,1,1,1);
			drawBoxOutline(center, size);
		}

	}
	else
	{
		gGL.diffuseColor3f(1,0,1);
		drawBoxOutline(center, size);
	}
}

void render_hull(LLModel::PhysicsMesh& mesh, const LLColor4& color, const LLColor4& line_color)
{
	gGL.diffuseColor4fv(color.mV);
	LLVertexBuffer::drawArrays(LLRender::TRIANGLES, mesh.mPositions, mesh.mNormals);
	LLGLEnable offset(GL_POLYGON_OFFSET_LINE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glPolygonOffset(3.f, 3.f);
	glLineWidth(3.f);
	gGL.diffuseColor4fv(line_color.mV);
	LLVertexBuffer::drawArrays(LLRender::TRIANGLES, mesh.mPositions, mesh.mNormals);
	glLineWidth(1.f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void renderPhysicsShape(LLDrawable* drawable, LLVOVolume* volume)
{
	U8 physics_type = volume->getPhysicsShapeType();

	if (physics_type == LLViewerObject::PHYSICS_SHAPE_NONE || volume->isFlexible())
	{
		return;
	}

	//not allowed to return at this point without rendering *something*

	F32 threshold = gSavedSettings.getF32("ObjectCostHighThreshold");
	F32 cost = volume->getObjectCost();

	LLColor4 low = gSavedSettings.getColor4("ObjectCostLowColor");
	LLColor4 mid = gSavedSettings.getColor4("ObjectCostMidColor");
	LLColor4 high = gSavedSettings.getColor4("ObjectCostHighColor");

	F32 normalizedCost = 1.f - exp( -(cost / threshold) );

	LLColor4 color;
	if ( normalizedCost <= 0.5f )
	{
		color = lerp( low, mid, 2.f * normalizedCost );
	}
	else
	{
		color = lerp( mid, high, 2.f * ( normalizedCost - 0.5f ) );
	}

	LLColor4 line_color = color*0.5f;

	U32 data_mask = LLVertexBuffer::MAP_VERTEX;

	LLVolumeParams volume_params = volume->getVolume()->getParams();

	LLPhysicsVolumeParams physics_params(volume_params, 
		physics_type == LLViewerObject::PHYSICS_SHAPE_CONVEX_HULL); 

	LLPhysicsShapeBuilderUtil::PhysicsShapeSpecification physics_spec;
	LLPhysicsShapeBuilderUtil::determinePhysicsShape(physics_params, volume->getScale(), physics_spec);

	U32 type = physics_spec.getType();

	LLVector3 center(0,0,0);
	LLVector3 size(0.25f,0.25f,0.25f);

	gGL.pushMatrix();
	gGL.multMatrix((F32*) volume->getRelativeXform().mMatrix);
		
	if (type == LLPhysicsShapeBuilderUtil::PhysicsShapeSpecification::USER_MESH)
	{
		LLUUID mesh_id = volume->getVolume()->getParams().getSculptID();
		LLModel::Decomposition* decomp = gMeshRepo.getDecomposition(mesh_id);
			
		if (decomp)
		{ //render a physics based mesh
			
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

			if (!decomp->mHull.empty())
			{ //decomposition exists, use that

				if (decomp->mMesh.empty())
				{
					gMeshRepo.buildPhysicsMesh(*decomp);
				}

				for (U32 i = 0; i < decomp->mMesh.size(); ++i)
				{		
					render_hull(decomp->mMesh[i], color, line_color);
				}
			}
			else if (!decomp->mPhysicsShapeMesh.empty())
			{ 
				//decomp has physics mesh, render that mesh
				gGL.diffuseColor4fv(color.mV);
				LLVertexBuffer::drawArrays(LLRender::TRIANGLES, decomp->mPhysicsShapeMesh.mPositions, decomp->mPhysicsShapeMesh.mNormals);
								
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				gGL.diffuseColor4fv(line_color.mV);
				LLVertexBuffer::drawArrays(LLRender::TRIANGLES, decomp->mPhysicsShapeMesh.mPositions, decomp->mPhysicsShapeMesh.mNormals);
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
			else
			{ //no mesh or decomposition, render base hull
				renderMeshBaseHull(volume, data_mask, color, line_color);

				if (decomp->mPhysicsShapeMesh.empty())
				{
					//attempt to fetch physics shape mesh if available
					gMeshRepo.fetchPhysicsShape(mesh_id);
				}
			}
		}
		else
		{	
			gGL.diffuseColor3f(1,1,0);
			drawBoxOutline(center, size);
		}
	}
	else if (type == LLPhysicsShapeBuilderUtil::PhysicsShapeSpecification::USER_CONVEX ||
		type == LLPhysicsShapeBuilderUtil::PhysicsShapeSpecification::PRIM_CONVEX)
	{
		if (volume->isMesh())
		{
			renderMeshBaseHull(volume, data_mask, color, line_color);
		}
		else
		{
			LLVolumeParams volume_params = volume->getVolume()->getParams();
			S32 detail = get_physics_detail(volume_params, volume->getScale());
			LLVolume* phys_volume = LLPrimitive::sVolumeManager->refVolume(volume_params, detail);

			if (!phys_volume->mHullPoints)
			{ //build convex hull
				std::vector<LLVector3> pos;
				std::vector<U16> index;

				S32 index_offset = 0;

				for (S32 i = 0; i < phys_volume->getNumVolumeFaces(); ++i)
				{
					const LLVolumeFace& face = phys_volume->getVolumeFace(i);
					if (index_offset + face.mNumVertices > 65535)
					{
						continue;
					}

					for (S32 j = 0; j < face.mNumVertices; ++j)
					{
						pos.push_back(LLVector3(face.mPositions[j].getF32ptr()));
					}

					for (S32 j = 0; j < face.mNumIndices; ++j)
					{
						index.push_back(face.mIndices[j]+index_offset);
					}

					index_offset += face.mNumVertices;
				}

				if (!pos.empty() && !index.empty())
				{
					LLCDMeshData mesh;
					mesh.mIndexBase = &index[0];
					mesh.mVertexBase = pos[0].mV;
					mesh.mNumVertices = pos.size();
					mesh.mVertexStrideBytes = 12;
					mesh.mIndexStrideBytes = 6;
					mesh.mIndexType = LLCDMeshData::INT_16;

					mesh.mNumTriangles = index.size()/3;
					
					LLCDMeshData res;

					LLConvexDecomposition::getInstance()->generateSingleHullMeshFromMesh( &mesh, &res );

					//copy res into phys_volume
					phys_volume->mHullPoints = (LLVector4a*) ll_aligned_malloc_16(sizeof(LLVector4a)*res.mNumVertices);
					phys_volume->mNumHullPoints = res.mNumVertices;

					S32 idx_size = (res.mNumTriangles*3*2+0xF) & ~0xF;
					phys_volume->mHullIndices = (U16*) ll_aligned_malloc_16(idx_size);
					phys_volume->mNumHullIndices = res.mNumTriangles*3;

					const F32* v = res.mVertexBase;

					for (S32 i = 0; i < res.mNumVertices; ++i)
					{
						F32* p = (F32*) ((U8*)v+i*res.mVertexStrideBytes);
						phys_volume->mHullPoints[i].load3(p);
					}

					if (res.mIndexType == LLCDMeshData::INT_16)
					{
						for (S32 i = 0; i < res.mNumTriangles; ++i)
						{
							U16* idx = (U16*) (((U8*)res.mIndexBase)+i*res.mIndexStrideBytes);

							phys_volume->mHullIndices[i*3+0] = idx[0];
							phys_volume->mHullIndices[i*3+1] = idx[1];
							phys_volume->mHullIndices[i*3+2] = idx[2];
						}
					}
					else
					{
						for (S32 i = 0; i < res.mNumTriangles; ++i)
						{
							U32* idx = (U32*) (((U8*)res.mIndexBase)+i*res.mIndexStrideBytes);

							phys_volume->mHullIndices[i*3+0] = (U16) idx[0];
							phys_volume->mHullIndices[i*3+1] = (U16) idx[1];
							phys_volume->mHullIndices[i*3+2] = (U16) idx[2];
						}
					}
				}
			}

			if (phys_volume->mHullPoints)
			{
				//render hull
			
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				
				gGL.diffuseColor4fv(line_color.mV);
				LLVertexBuffer::unbind();

				llassert(!LLGLSLShader::sNoFixedFunction || LLGLSLShader::sCurBoundShader != 0);
							
				LLVertexBuffer::drawElements(LLRender::TRIANGLES, phys_volume->mHullPoints, NULL, phys_volume->mNumHullIndices, phys_volume->mHullIndices);
				
				gGL.diffuseColor4fv(color.mV);
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				LLVertexBuffer::drawElements(LLRender::TRIANGLES, phys_volume->mHullPoints, NULL, phys_volume->mNumHullIndices, phys_volume->mHullIndices);
				
			}
			else
			{
				gGL.diffuseColor4f(1,0,1,1);
				drawBoxOutline(center, size);
			}

			LLPrimitive::sVolumeManager->unrefVolume(phys_volume);
		}
	}
	else if (type == LLPhysicsShapeBuilderUtil::PhysicsShapeSpecification::BOX)
	{
		LLVector3 center = physics_spec.getCenter();
		LLVector3 scale = physics_spec.getScale();
		LLVector3 vscale = volume->getScale()*2.f;
		scale.set(scale[0]/vscale[0], scale[1]/vscale[1], scale[2]/vscale[2]);
		
		gGL.diffuseColor4fv(color.mV);
		drawBox(center, scale);
	}
	else if	(type == LLPhysicsShapeBuilderUtil::PhysicsShapeSpecification::SPHERE)
	{
		LLVolumeParams volume_params;
		volume_params.setType( LL_PCODE_PROFILE_CIRCLE_HALF, LL_PCODE_PATH_CIRCLE );
		volume_params.setBeginAndEndS( 0.f, 1.f );
		volume_params.setBeginAndEndT( 0.f, 1.f );
		volume_params.setRatio	( 1, 1 );
		volume_params.setShear	( 0, 0 );
		LLVolume* sphere = LLPrimitive::sVolumeManager->refVolume(volume_params, 3);
		
		gGL.diffuseColor4fv(color.mV);
		pushVerts(sphere);
		LLPrimitive::sVolumeManager->unrefVolume(sphere);
	}
	else if (type == LLPhysicsShapeBuilderUtil::PhysicsShapeSpecification::CYLINDER)
	{
		LLVolumeParams volume_params;
		volume_params.setType( LL_PCODE_PROFILE_CIRCLE, LL_PCODE_PATH_LINE );
		volume_params.setBeginAndEndS( 0.f, 1.f );
		volume_params.setBeginAndEndT( 0.f, 1.f );
		volume_params.setRatio	( 1, 1 );
		volume_params.setShear	( 0, 0 );
		LLVolume* cylinder = LLPrimitive::sVolumeManager->refVolume(volume_params, 3);
		
		gGL.diffuseColor4fv(color.mV);
		pushVerts(cylinder);
		LLPrimitive::sVolumeManager->unrefVolume(cylinder);
	}
	else if (type == LLPhysicsShapeBuilderUtil::PhysicsShapeSpecification::PRIM_MESH)
	{
		LLVolumeParams volume_params = volume->getVolume()->getParams();
		S32 detail = get_physics_detail(volume_params, volume->getScale());

		LLVolume* phys_volume = LLPrimitive::sVolumeManager->refVolume(volume_params, detail);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		
		gGL.diffuseColor4fv(line_color.mV);
		pushVerts(phys_volume);
		
		gGL.diffuseColor4fv(color.mV);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		pushVerts(phys_volume);
		LLPrimitive::sVolumeManager->unrefVolume(phys_volume);
	}
	else if (type == LLPhysicsShapeBuilderUtil::PhysicsShapeSpecification::PRIM_CONVEX)
	{
		LLVolumeParams volume_params = volume->getVolume()->getParams();
		S32 detail = get_physics_detail(volume_params, volume->getScale());

		LLVolume* phys_volume = LLPrimitive::sVolumeManager->refVolume(volume_params, detail);

		if (phys_volume->mHullPoints && phys_volume->mHullIndices)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			llassert(!LLGLSLShader::sNoFixedFunction || LLGLSLShader::sCurBoundShader != 0);
			LLVertexBuffer::unbind();
			glVertexPointer(3, GL_FLOAT, 16, phys_volume->mHullPoints);
			gGL.diffuseColor4fv(line_color.mV);
			gGL.syncMatrices();
			glDrawElements(GL_TRIANGLES, phys_volume->mNumHullIndices, GL_UNSIGNED_SHORT, phys_volume->mHullIndices);
			
			gGL.diffuseColor4fv(color.mV);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDrawElements(GL_TRIANGLES, phys_volume->mNumHullIndices, GL_UNSIGNED_SHORT, phys_volume->mHullIndices);			
		}
		else
		{
			gGL.diffuseColor3f(1,0,1);
			drawBoxOutline(center, size);
			gMeshRepo.buildHull(volume_params, detail);
		}
		LLPrimitive::sVolumeManager->unrefVolume(phys_volume);
	}
	else if (type == LLPhysicsShapeBuilderUtil::PhysicsShapeSpecification::SCULPT)
	{
		//TODO: implement sculpted prim physics display
	}
	else 
	{
		LL_ERRS() << "Unhandled type" << LL_ENDL;
	}

	gGL.popMatrix();
}

void renderPhysicsShapes(LLSpatialGroup* group)
{
	for (OctreeNode::const_element_iter i = group->getDataBegin(); i != group->getDataEnd(); ++i)
	{
		LLDrawable* drawable = (LLDrawable*)(*i)->getDrawable();
		if(!drawable)
	{
			continue;
		}

		if (drawable->isSpatialBridge())
		{
			LLSpatialBridge* bridge = drawable->asPartition()->asBridge();

			if (bridge)
			{
				gGL.pushMatrix();
				gGL.multMatrix((F32*)bridge->mDrawable->getRenderMatrix().mMatrix);
				bridge->renderPhysicsShapes();
				gGL.popMatrix();
			}
		}
		else
		{
			LLVOVolume* volume = drawable->getVOVolume();
			if (volume && !volume->isAttachment() && volume->getPhysicsShapeType() != LLViewerObject::PHYSICS_SHAPE_NONE )
			{
				if (!group->getSpatialPartition()->isBridge())
				{
					gGL.pushMatrix();
					LLVector3 trans = drawable->getRegion()->getOriginAgent();
					gGL.translatef(trans.mV[0], trans.mV[1], trans.mV[2]);
					renderPhysicsShape(drawable, volume);
					gGL.popMatrix();
				}
				else
				{
					renderPhysicsShape(drawable, volume);
				}
			}
			else
			{
				LLViewerObject* object = drawable->getVObj();
				if (object && object->getPCode() == LLViewerObject::LL_VO_SURFACE_PATCH)
				{
					gGL.pushMatrix();
					gGL.multMatrix((F32*) object->getRegion()->mRenderMatrix.mMatrix);
					//push face vertices for terrain
					for (S32 i = 0; i < drawable->getNumFaces(); ++i)
					{
						LLFace* face = drawable->getFace(i);
						if (face)
						{
							LLVertexBuffer* buff = face->getVertexBuffer();
							if (buff)
							{
								glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

								buff->setBuffer(LLVertexBuffer::MAP_VERTEX);
								gGL.diffuseColor3f(0.2f, 0.5f, 0.3f);
								buff->draw(LLRender::TRIANGLES, buff->getNumIndices(), 0);
									
								gGL.diffuseColor3f(0.2f, 1.f, 0.3f);
								glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
								buff->draw(LLRender::TRIANGLES, buff->getNumIndices(), 0);
							}
						}
					}
					gGL.popMatrix();
				}
			}
		}
	}
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
		
		//LLViewerTexture* imagep = facep->getTexture();
		//if (imagep)
		if (facep)
		{
				
			//F32 vsize = imagep->mMaxVirtualSize;
			F32 vsize = facep->getPixelArea();

			if (vsize > sCurMaxTexPriority)
			{
				sCurMaxTexPriority = vsize;
			}
			
			F32 t = vsize/sLastMaxTexPriority;
			
			LLVector4 col = lerp(cold, hot, t);
			gGL.diffuseColor4fv(col.mV);
		}
		//else
		//{
		//	gGL.diffuseColor4f(1,0,1,1);
		//}
		
		LLVector4a center;
		center.setAdd(facep->mExtents[1],facep->mExtents[0]);
		center.mul(0.5f);
		LLVector4a size;
		size.setSub(facep->mExtents[1],facep->mExtents[0]);
		size.mul(0.5f);
		size.add(LLVector4a(0.01f));
		drawBox(center, size);
		
		/*S32 boost = imagep->getBoostLevel();
		if (boost>LLGLTexture::BOOST_NONE)
		{
			F32 t = (F32) boost / (F32) (LLGLTexture::BOOST_MAX_LEVEL-1);
			LLVector4 col = lerp(boost_cold, boost_hot, t);
			LLGLEnable blend_on(GL_BLEND);
			gGL.blendFunc(GL_SRC_ALPHA, GL_ONE);
			gGL.diffuseColor4fv(col.mV);
			drawBox(center, size);
			gGL.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}*/
	}
}

void renderPoints(LLDrawable* drawablep)
{
	LLGLDepthTest depth(GL_FALSE, GL_FALSE);
	if (drawablep->getNumFaces())
	{
		gGL.begin(LLRender::POINTS);
		gGL.diffuseColor3f(1,1,1);
		for (S32 i = 0; i < drawablep->getNumFaces(); i++)
		{
			LLFace * face = drawablep->getFace(i);
			if (face)
			{
				gGL.vertex3fv(face->mCenterLocal.mV);
			}
		}
		gGL.end();
	}
}

void renderTextureAnim(LLDrawInfo* params)
{
	if (!params->mTextureMatrix)
	{
		return;
	}
	
	LLGLEnable blend(GL_BLEND);
	gGL.diffuseColor4f(1,1,0,0.5f);
	pushVerts(params, LLVertexBuffer::MAP_VERTEX);
}

void renderBatchSize(LLDrawInfo* params)
{
	LLGLEnable offset(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1.f, 1.f);
	gGL.diffuseColor4ubv((GLubyte*) &(params->mDebugColor));
	pushVerts(params, LLVertexBuffer::MAP_VERTEX);
}

void renderShadowFrusta(LLDrawInfo* params)
{
	LLGLEnable blend(GL_BLEND);
	gGL.setSceneBlendType(LLRender::BT_ADD);

	LLVector4a center;
	center.setAdd(params->mExtents[1], params->mExtents[0]);
	center.mul(0.5f);
	LLVector4a size;
	size.setSub(params->mExtents[1],params->mExtents[0]);
	size.mul(0.5f);

	if (gPipeline.mShadowCamera[4].AABBInFrustum(center, size))
	{
		gGL.diffuseColor3f(1,0,0);
		pushVerts(params, LLVertexBuffer::MAP_VERTEX);
	}
	if (gPipeline.mShadowCamera[5].AABBInFrustum(center, size))
	{
		gGL.diffuseColor3f(0,1,0);
		pushVerts(params, LLVertexBuffer::MAP_VERTEX);
	}
	if (gPipeline.mShadowCamera[6].AABBInFrustum(center, size))
	{
		gGL.diffuseColor3f(0,0,1);
		pushVerts(params, LLVertexBuffer::MAP_VERTEX);
	}
	if (gPipeline.mShadowCamera[7].AABBInFrustum(center, size))
	{
		gGL.diffuseColor3f(1,0,1);
		pushVerts(params, LLVertexBuffer::MAP_VERTEX);
	}

	gGL.setSceneBlendType(LLRender::BT_ALPHA);
}

void renderTexelDensity(LLDrawable* drawable)
{
	if (LLViewerTexture::sDebugTexelsMode == LLViewerTexture::DEBUG_TEXELS_OFF
		|| LLViewerTexture::sCheckerBoardImagep.isNull())
	{
		return;
	}

	LLGLEnable _(GL_BLEND);
	//gObjectFullbrightProgram.bind();

	LLMatrix4 checkerboard_matrix;
	S32 discard_level = -1;

	for (S32 f = 0; f < drawable->getNumFaces(); f++)
	{
		LLFace* facep = drawable->getFace(f);
		LLVertexBuffer* buffer = facep->getVertexBuffer();
		LLViewerTexture* texturep = facep->getTexture();

		if (texturep == NULL) continue;

		switch(LLViewerTexture::sDebugTexelsMode)
		{
		case LLViewerTexture::DEBUG_TEXELS_CURRENT:
			discard_level = -1;
			break;
		case LLViewerTexture::DEBUG_TEXELS_DESIRED:
			{
				LLViewerFetchedTexture* fetched_texturep = dynamic_cast<LLViewerFetchedTexture*>(texturep);
				discard_level = fetched_texturep ? fetched_texturep->getDesiredDiscardLevel() : -1;
				break;
			}
		default:
		case LLViewerTexture::DEBUG_TEXELS_FULL:
			discard_level = 0;
			break;
		}

		checkerboard_matrix.initScale(LLVector3(texturep->getWidth(discard_level) / 8, texturep->getHeight(discard_level) / 8, 1.f));

		gGL.getTexUnit(0)->bind(LLViewerTexture::sCheckerBoardImagep, TRUE);
		gGL.matrixMode(LLRender::MM_TEXTURE);
		gGL.loadMatrix((GLfloat*)&checkerboard_matrix.mMatrix);

		if (buffer && (facep->getGeomCount() >= 3))
		{
			buffer->setBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0);
			U16 start = facep->getGeomStart();
			U16 end = start + facep->getGeomCount()-1;
			U32 count = facep->getIndicesCount();
			U16 offset = facep->getIndicesStart();
			buffer->drawRange(LLRender::TRIANGLES, start, end, count, offset);
		}

		gGL.loadIdentity();
		gGL.matrixMode(LLRender::MM_MODELVIEW);
	}

	//S32 num_textures = llmax(1, (S32)params->mTextureList.size());

	//for (S32 i = 0; i < num_textures; i++)
	//{
	//	LLViewerTexture* texturep = params->mTextureList.empty() ? params->mTexture.get() : params->mTextureList[i].get();
	//	if (texturep == NULL) continue;

	//	LLMatrix4 checkboard_matrix;
	//	S32 discard_level = -1;
	//	switch(LLViewerTexture::sDebugTexelsMode)
	//	{
	//	case LLViewerTexture::DEBUG_TEXELS_CURRENT:
	//		discard_level = -1;
	//		break;
	//	case LLViewerTexture::DEBUG_TEXELS_DESIRED:
	//		{
	//			LLViewerFetchedTexture* fetched_texturep = dynamic_cast<LLViewerFetchedTexture*>(texturep);
	//			discard_level = fetched_texturep ? fetched_texturep->getDesiredDiscardLevel() : -1;
	//			break;
	//		}
	//	default:
	//	case LLViewerTexture::DEBUG_TEXELS_FULL:
	//		discard_level = 0;
	//		break;
	//	}

	//	checkboard_matrix.initScale(LLVector3(texturep->getWidth(discard_level) / 8, texturep->getHeight(discard_level) / 8, 1.f));
	//	gGL.getTexUnit(i)->activate();

	//	glMatrixMode(GL_TEXTURE);
	//	glPushMatrix();
	//	glLoadIdentity();
	//	//gGL.matrixMode(LLRender::MM_TEXTURE);
	//	glLoadMatrixf((GLfloat*) checkboard_matrix.mMatrix);

	//	gGL.getTexUnit(i)->bind(LLViewerTexture::sCheckerBoardImagep, TRUE);

	//	pushVerts(params, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_COLOR | LLVertexBuffer::MAP_NORMAL );

	//	glPopMatrix();
	//	glMatrixMode(GL_MODELVIEW);
	//	//gGL.matrixMode(LLRender::MM_MODELVIEW);
	//}
}


void renderLights(LLDrawable* drawablep)
{
	if (!drawablep->isLight())
	{
		return;
	}

	if (drawablep->getNumFaces())
	{
		LLGLEnable blend(GL_BLEND);
		gGL.diffuseColor4f(0,1,1,0.5f);

		for (S32 i = 0; i < drawablep->getNumFaces(); i++)
		{
			LLFace * face = drawablep->getFace(i);
			if (face)
			{
				pushVerts(face, LLVertexBuffer::MAP_VERTEX);
			}
		}

		const LLVector4a* ext = drawablep->getSpatialExtents();

		LLVector4a pos;
		pos.setAdd(ext[0], ext[1]);
		pos.mul(0.5f);
		LLVector4a size;
		size.setSub(ext[1], ext[0]);
		size.mul(0.5f);

		{
			LLGLDepthTest depth(GL_FALSE, GL_TRUE);
			gGL.diffuseColor4f(1,1,1,1);
			drawBoxOutline(pos, size);
		}

		gGL.diffuseColor4f(1,1,0,1);
		F32 rad = drawablep->getVOVolume()->getLightRadius();
		drawBoxOutline(pos, LLVector4a(rad));
	}
}

class LLRenderOctreeRaycast : public LLOctreeTriangleRayIntersect
{
public:
	
	
	LLRenderOctreeRaycast(const LLVector4a& start, const LLVector4a& dir, F32* closest_t)
		: LLOctreeTriangleRayIntersect(start, dir, NULL, closest_t, NULL, NULL, NULL, NULL)
	{

	}

	void visit(const LLOctreeNode<LLVolumeTriangle>* branch)
	{
		LLVolumeOctreeListener* vl = (LLVolumeOctreeListener*) branch->getListener(0);

		LLVector3 center, size;
		
		if (branch->isEmpty())
		{
			gGL.diffuseColor3f(1.f,0.2f,0.f);
			center.set(branch->getCenter().getF32ptr());
			size.set(branch->getSize().getF32ptr());
		}
		else
		{
			gGL.diffuseColor3f(0.75f, 1.f, 0.f);
			center.set(vl->mBounds[0].getF32ptr());
			size.set(vl->mBounds[1].getF32ptr());
		}

		drawBoxOutline(center, size);	
		
		for (U32 i = 0; i < 2; i++)
		{
			LLGLDepthTest depth(GL_TRUE, GL_FALSE, i == 1 ? GL_LEQUAL : GL_GREATER);

			if (i == 1)
			{
				gGL.diffuseColor4f(0,1,1,0.5f);
			}
			else
			{
				gGL.diffuseColor4f(0,0.5f,0.5f, 0.25f);
				drawBoxOutline(center, size);
			}
			
			if (i == 1)
			{
				gGL.flush();
				glLineWidth(3.f);
			}

			gGL.begin(LLRender::TRIANGLES);
			for (LLOctreeNode<LLVolumeTriangle>::const_element_iter iter = branch->getDataBegin();
					iter != branch->getDataEnd();
					++iter)
			{
				const LLVolumeTriangle* tri = *iter;
				
				gGL.vertex3fv(tri->mV[0]->getF32ptr());
				gGL.vertex3fv(tri->mV[1]->getF32ptr());
				gGL.vertex3fv(tri->mV[2]->getF32ptr());
			}	
			gGL.end();

			if (i == 1)
			{
				gGL.flush();
				glLineWidth(1.f);
			}
		}
	}
};

void renderRaycast(LLDrawable* drawablep)
{
	if (drawablep->getNumFaces())
	{
		LLGLEnable blend(GL_BLEND);
		gGL.diffuseColor4f(0,1,1,0.5f);

		if (drawablep->getVOVolume())
		{
			//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			//pushVerts(drawablep->getFace(gDebugRaycastFaceHit), LLVertexBuffer::MAP_VERTEX);
			//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			LLVOVolume* vobj = drawablep->getVOVolume();
			LLVolume* volume = vobj->getVolume();

			bool transform = true;
			if (drawablep->isState(LLDrawable::RIGGED))
			{
				volume = vobj->getRiggedVolume();
				transform = false;
			}

			if (volume)
			{
				LLVector3 trans = drawablep->getRegion()->getOriginAgent();
				
				for (S32 i = 0; i < volume->getNumVolumeFaces(); ++i)
				{
					const LLVolumeFace& face = volume->getVolumeFace(i);
					
					gGL.pushMatrix();
					gGL.translatef(trans.mV[0], trans.mV[1], trans.mV[2]);					
					gGL.multMatrix((F32*) vobj->getRelativeXform().mMatrix);

					LLVector4a start, end;
					if (transform)
					{
						LLVector3 v_start(gDebugRaycastStart.getF32ptr());
						LLVector3 v_end(gDebugRaycastEnd.getF32ptr());

						v_start = vobj->agentPositionToVolume(v_start);
						v_end = vobj->agentPositionToVolume(v_end);

						start.load3(v_start.mV);
						end.load3(v_end.mV);
					}
					else
					{
						start = gDebugRaycastStart;
						end = gDebugRaycastEnd;
					}

					LLVector4a dir;
					dir.setSub(end, start);

					gGL.flush();
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);				

					{
						//render face positions
						LLVertexBuffer::unbind();
						gGL.diffuseColor4f(0,1,1,0.5f);
						glVertexPointer(3, GL_FLOAT, sizeof(LLVector4a), face.mPositions);
						gGL.syncMatrices();
						glDrawElements(GL_TRIANGLES, face.mNumIndices, GL_UNSIGNED_SHORT, face.mIndices);
					}
					
					if (!volume->isUnique())
					{
						F32 t = 1.f;

						if (!face.mOctree)
						{
							((LLVolumeFace*) &face)->createOctree(); 
						}

						LLRenderOctreeRaycast render(start, dir, &t);
					
						render.traverse(face.mOctree);
					}

					gGL.popMatrix();		
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				}
			}
		}
		else if (drawablep->isAvatar())
		{
			if (drawablep->getVObj() == gDebugRaycastObject)
			{
				LLGLDepthTest depth(GL_FALSE);
				LLVOAvatar* av = (LLVOAvatar*) drawablep->getVObj().get();
				av->renderCollisionVolumes();
			}
		}

		if (drawablep->getVObj() == gDebugRaycastObject)
		{
			// draw intersection point
			gGL.pushMatrix();
			gGL.loadMatrix(gGLModelView);
			LLVector3 translate(gDebugRaycastIntersection.getF32ptr());
			gGL.translatef(translate.mV[0], translate.mV[1], translate.mV[2]);
			LLCoordFrame orient;
			LLVector4a debug_binormal;
			
			debug_binormal.setCross3(gDebugRaycastNormal, gDebugRaycastTangent);
			debug_binormal.mul(gDebugRaycastTangent.getF32ptr()[3]);

			LLVector3 normal(gDebugRaycastNormal.getF32ptr());
			LLVector3 binormal(debug_binormal.getF32ptr());
						
			orient.lookDir(normal, binormal);
			LLMatrix4 rotation;
			orient.getRotMatrixToParent(rotation);
			gGL.multMatrix((float*)rotation.mMatrix);
			
			gGL.diffuseColor4f(1,0,0,0.5f);
			drawBox(LLVector3(0, 0, 0), LLVector3(0.1f, 0.022f, 0.022f));
			gGL.diffuseColor4f(0,1,0,0.5f);
			drawBox(LLVector3(0, 0, 0), LLVector3(0.021f, 0.1f, 0.021f));
			gGL.diffuseColor4f(0,0,1,0.5f);
			drawBox(LLVector3(0, 0, 0), LLVector3(0.02f, 0.02f, 0.1f));
			gGL.popMatrix();

			// draw bounding box of prim
			const LLVector4a* ext = drawablep->getSpatialExtents();

			LLVector4a pos;
			pos.setAdd(ext[0], ext[1]);
			pos.mul(0.5f);
			LLVector4a size;
			size.setSub(ext[1], ext[0]);
			size.mul(0.5f);

			LLGLDepthTest depth(GL_FALSE, GL_TRUE);
			gGL.diffuseColor4f(0,0.5f,0.5f,1);
			drawBoxOutline(pos, size);		
		}
	}
}


void renderAvatarCollisionVolumes(LLVOAvatar* avatar)
{
	avatar->renderCollisionVolumes();
}

void renderAvatarBones(LLVOAvatar* avatar)
{
	avatar->renderBones();
}

void renderAgentTarget(LLVOAvatar* avatar)
{
	// render these for self only (why, i don't know)
	if (avatar->isSelf())
	{
		renderCrossHairs(avatar->getPositionAgent(), 0.2f, LLColor4(1, 0, 0, 0.8f));
		renderCrossHairs(avatar->mDrawable->getPositionAgent(), 0.2f, LLColor4(0, 1, 0, 0.8f));
		renderCrossHairs(avatar->mRoot->getWorldPosition(), 0.2f, LLColor4(1, 1, 1, 0.8f));
		renderCrossHairs(avatar->mPelvisp->getWorldPosition(), 0.2f, LLColor4(0, 0, 1, 0.8f));
	}
}

class LLOctreeRenderNonOccluded : public OctreeTraveler
{
public:
	LLCamera* mCamera;
	LLOctreeRenderNonOccluded(LLCamera* camera): mCamera(camera) {}
	
	virtual void traverse(const OctreeNode* node)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) node->getListener(0);
		
		const LLVector4a* bounds = group->getBounds();
		if (!mCamera || mCamera->AABBInFrustumNoFarClip(bounds[0], bounds[1]))
		{
			node->accept(this);
			stop_glerror();

			for (U32 i = 0; i < node->getChildCount(); i++)
			{
				traverse(node->getChild(i));
				stop_glerror();
			}
			
			//draw tight fit bounding boxes for spatial group
			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCTREE))
			{	
				group->rebuildGeom();
				group->rebuildMesh();

				renderOctree(group);
				stop_glerror();
			}

			//render visibility wireframe
			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCCLUSION))
			{
				group->rebuildGeom();
				group->rebuildMesh();

				gGL.flush();
				gGL.pushMatrix();
				gGLLastMatrix = NULL;
				gGL.loadMatrix(gGLModelView);
				renderVisibility(group, mCamera);
				stop_glerror();
				gGLLastMatrix = NULL;
				gGL.popMatrix();
				gGL.diffuseColor4f(1,1,1,1);
			}
		}
	}

	virtual void visit(const OctreeNode* branch)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) branch->getListener(0);
		const LLVector4a* bounds = group->getBounds();
		if (group->hasState(LLSpatialGroup::GEOM_DIRTY) || (mCamera && !mCamera->AABBInFrustumNoFarClip(bounds[0], bounds[1])))
		{
			return;
		}

		group->rebuildGeom();
		group->rebuildMesh();

		if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_BBOXES))
		{
			if (!group->isEmpty())
			{
				gGL.diffuseColor3f(0,0,1);
				const LLVector4a* obj_bounds = group->getObjectBounds();
				drawBoxOutline(obj_bounds[0], obj_bounds[1]);
			}
		}

		for (OctreeNode::const_element_iter i = branch->getDataBegin(); i != branch->getDataEnd(); ++i)
		{
			LLDrawable* drawable = (LLDrawable*)(*i)->getDrawable();
			if(!drawable)
		{
				continue;
			}
					
			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_BBOXES))
			{
				renderBoundingBox(drawable);			
			}

			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_NORMALS))
			{
				renderNormals(drawable);
			}
			
			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_BUILD_QUEUE))
			{
				if (drawable->isState(LLDrawable::IN_REBUILD_Q2))
				{
					gGL.diffuseColor4f(0.6f, 0.6f, 0.1f, 1.f);
					const LLVector4a* ext = drawable->getSpatialExtents();
					LLVector4a center;
					center.setAdd(ext[0], ext[1]);
					center.mul(0.5f);
					LLVector4a size;
					size.setSub(ext[1], ext[0]);
					size.mul(0.5f);
					drawBoxOutline(center, size);
				}
			}	

			if (drawable->getVOVolume() && gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY))
			{
				renderTexturePriority(drawable);
			}

			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_POINTS))
			{
				renderPoints(drawable);
			}

			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_LIGHTS))
			{
				renderLights(drawable);
			}

			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_RAYCAST))
			{
				renderRaycast(drawable);
			}
			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_UPDATE_TYPE))
			{
				renderUpdateType(drawable);
			}
			if(gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_RENDER_COMPLEXITY))
			{
				renderComplexityDisplay(drawable);
			}
			if(gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXEL_DENSITY))
			{
				renderTexelDensity(drawable);
			}

			LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(drawable->getVObj().get());
			
			if (avatar && gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_AVATAR_VOLUME))
			{
				renderAvatarCollisionVolumes(avatar);
			}

			if (avatar && gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_AVATAR_JOINTS))
			{
				renderAvatarBones(avatar);
			}

			if (avatar && gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_AGENT_TARGET))
			{
				renderAgentTarget(avatar);
			}
			
			if (gDebugGL)
			{
				for (U32 i = 0; i < drawable->getNumFaces(); ++i)
				{
					LLFace* facep = drawable->getFace(i);
					if (facep)
					{
						U8 index = facep->getTextureIndex();
						if (facep->mDrawInfo)
						{
							if (index < 255)
							{
								if (facep->mDrawInfo->mTextureList.size() <= index)
								{
									LL_ERRS() << "Face texture index out of bounds." << LL_ENDL;
								}
								else if (facep->mDrawInfo->mTextureList[index] != facep->getTexture())
								{
									LL_ERRS() << "Face texture index incorrect." << LL_ENDL;
								}
							}
						}
					}
				}
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
				if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_BATCH_SIZE))
				{
					renderBatchSize(draw_info);
				}
				if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA))
				{
					renderShadowFrusta(draw_info);
				}
			}
		}
	}
};

class LLOctreeRenderXRay : public OctreeTraveler
{
public:
	LLCamera* mCamera;
	LLOctreeRenderXRay(LLCamera* camera): mCamera(camera) {}
	
	virtual void traverse(const OctreeNode* node)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) node->getListener(0);
		
		const LLVector4a* bounds = group->getBounds();
		if (!mCamera || mCamera->AABBInFrustumNoFarClip(bounds[0], bounds[1]))
		{
			node->accept(this);
			stop_glerror();

			for (U32 i = 0; i < node->getChildCount(); i++)
			{
				traverse(node->getChild(i));
				stop_glerror();
			}
			
			//render visibility wireframe
			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCCLUSION))
			{
				group->rebuildGeom();
				group->rebuildMesh();

				gGL.flush();
				gGL.pushMatrix();
				gGLLastMatrix = NULL;
				gGL.loadMatrix(gGLModelView);
				renderXRay(group, mCamera);
				stop_glerror();
				gGLLastMatrix = NULL;
				gGL.popMatrix();
			}
		}
	}

	virtual void visit(const OctreeNode* node) {}

};

class LLOctreeRenderPhysicsShapes : public OctreeTraveler
{
public:
	LLCamera* mCamera;
	LLOctreeRenderPhysicsShapes(LLCamera* camera): mCamera(camera) {}
	
	virtual void traverse(const OctreeNode* node)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) node->getListener(0);
		
		const LLVector4a* bounds = group->getBounds();
		if (!mCamera || mCamera->AABBInFrustumNoFarClip(bounds[0], bounds[1]))
		{
			node->accept(this);
			stop_glerror();

			for (U32 i = 0; i < node->getChildCount(); i++)
			{
				traverse(node->getChild(i));
				stop_glerror();
			}
			
			group->rebuildGeom();
			group->rebuildMesh();

			renderPhysicsShapes(group);
		}
	}

	virtual void visit(const OctreeNode* branch)
	{
		
	}
};

class LLOctreePushBBoxVerts : public OctreeTraveler
{
public:
	LLCamera* mCamera;
	LLOctreePushBBoxVerts(LLCamera* camera): mCamera(camera) {}
	
	virtual void traverse(const OctreeNode* node)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) node->getListener(0);
		
		const LLVector4a* bounds = group->getBounds();
		if (!mCamera || mCamera->AABBInFrustum(bounds[0], bounds[1]))
		{
			node->accept(this);

			for (U32 i = 0; i < node->getChildCount(); i++)
			{
				traverse(node->getChild(i));
			}
		}
	}

	virtual void visit(const OctreeNode* branch)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) branch->getListener(0);

		const LLVector4a* bounds = group->getBounds();
		if (group->hasState(LLSpatialGroup::GEOM_DIRTY) || (mCamera && !mCamera->AABBInFrustumNoFarClip(bounds[0], bounds[1])))
		{
			return;
		}

		for (OctreeNode::const_element_iter i = branch->getDataBegin(); i != branch->getDataEnd(); ++i)
		{
			LLDrawable* drawable = (LLDrawable*)(*i)->getDrawable();
			if(!drawable)
		{
				continue;
			}
			renderBoundingBox(drawable, FALSE);			
		}
	}
};

void LLSpatialPartition::renderIntersectingBBoxes(LLCamera* camera)
{
	LLOctreePushBBoxVerts pusher(camera);
	pusher.traverse(mOctree);
}

class LLOctreeStateCheck : public OctreeTraveler
{
public:
	U32 mInheritedMask[LLViewerCamera::NUM_CAMERAS];

	LLOctreeStateCheck()
	{ 
		for (U32 i = 0; i < LLViewerCamera::NUM_CAMERAS; i++)
		{
			mInheritedMask[i] = 0;
		}
	}

	virtual void traverse(const OctreeNode* node)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) node->getListener(0);
		
		node->accept(this);


		U32 temp[LLViewerCamera::NUM_CAMERAS];

		for (U32 i = 0; i < LLViewerCamera::NUM_CAMERAS; i++)
		{
			temp[i] = mInheritedMask[i];
			mInheritedMask[i] |= group->mOcclusionState[i] & LLSpatialGroup::OCCLUDED; 
		}

		for (U32 i = 0; i < node->getChildCount(); i++)
		{
			traverse(node->getChild(i));
		}

		for (U32 i = 0; i < LLViewerCamera::NUM_CAMERAS; i++)
		{
			mInheritedMask[i] = temp[i];
		}
	}
	

	virtual void visit(const OctreeNode* state)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) state->getListener(0);

		for (U32 i = 0; i < LLViewerCamera::NUM_CAMERAS; i++)
		{
			if (mInheritedMask[i] && !(group->mOcclusionState[i] & mInheritedMask[i]))
			{
				LL_ERRS() << "Spatial group failed inherited mask test." << LL_ENDL;
			}
		}

		if (group->hasState(LLSpatialGroup::DIRTY))
		{
			assert_parent_state(group, LLSpatialGroup::DIRTY);
		}
	}

	void assert_parent_state(LLSpatialGroup* group, U32 state)
	{
		LLSpatialGroup* parent = group->getParent();
		while (parent)
		{
			if (!parent->hasState(state))
			{
				LL_ERRS() << "Spatial group failed parent state check." << LL_ENDL;
			}
			parent = parent->getParent();
		}
	}	
};


void LLSpatialPartition::renderPhysicsShapes()
{
	LLSpatialBridge* bridge = asBridge();
	LLCamera* camera = LLViewerCamera::getInstance();
	
	if (bridge)
	{
		camera = NULL;
	}

	gGL.flush();
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	glLineWidth(3.f);
	LLOctreeRenderPhysicsShapes render_physics(camera);
	render_physics.traverse(mOctree);
	gGL.flush();
	glLineWidth(1.f);
}

void LLSpatialPartition::renderDebug()
{
	if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCTREE |
									  LLPipeline::RENDER_DEBUG_OCCLUSION |
									  LLPipeline::RENDER_DEBUG_LIGHTS |
									  LLPipeline::RENDER_DEBUG_BATCH_SIZE |
									  LLPipeline::RENDER_DEBUG_UPDATE_TYPE |
									  LLPipeline::RENDER_DEBUG_BBOXES |
									  LLPipeline::RENDER_DEBUG_NORMALS |
									  LLPipeline::RENDER_DEBUG_POINTS |
									  LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY |
									  LLPipeline::RENDER_DEBUG_TEXTURE_ANIM |
									  LLPipeline::RENDER_DEBUG_RAYCAST |
									  LLPipeline::RENDER_DEBUG_AVATAR_VOLUME |
									  LLPipeline::RENDER_DEBUG_AVATAR_JOINTS |
									  LLPipeline::RENDER_DEBUG_AGENT_TARGET |
									  //LLPipeline::RENDER_DEBUG_BUILD_QUEUE |
									  LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA |
									  LLPipeline::RENDER_DEBUG_RENDER_COMPLEXITY |
									  LLPipeline::RENDER_DEBUG_TEXEL_DENSITY)) 
	{
		return;
	}
	
	if (LLGLSLShader::sNoFixedFunction)
	{
		gDebugProgram.bind();
	}

	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY))
	{
		//sLastMaxTexPriority = lerp(sLastMaxTexPriority, sCurMaxTexPriority, gFrameIntervalSeconds);
		sLastMaxTexPriority = (F32) LLViewerCamera::getInstance()->getScreenPixelArea();
		sCurMaxTexPriority = 0.f;
	}

	LLGLDisable cullface(GL_CULL_FACE);
	LLGLEnable blend(GL_BLEND);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gPipeline.disableLights();

	LLSpatialBridge* bridge = asBridge();
	LLCamera* camera = LLViewerCamera::getInstance();
	
	if (bridge)
	{
		camera = NULL;
	}

	LLOctreeStateCheck checker;
	checker.traverse(mOctree);

	LLOctreeRenderNonOccluded render_debug(camera);
	render_debug.traverse(mOctree);


	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCCLUSION))
	{
		{
			LLGLEnable cull(GL_CULL_FACE);
			
			LLGLEnable blend(GL_BLEND);
			LLGLDepthTest depth_under(GL_TRUE, GL_FALSE, GL_GREATER);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			gGL.diffuseColor4f(0.5f, 0.0f, 0, 0.25f);

			LLGLEnable offset(GL_POLYGON_OFFSET_LINE);
			glPolygonOffset(-1.f, -1.f);

			LLOctreeRenderXRay xray(camera);
			xray.traverse(mOctree);

			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}
	if (LLGLSLShader::sNoFixedFunction)
	{
		gDebugProgram.unbind();
	}
}

void LLSpatialGroup::drawObjectBox(LLColor4 col)
{
	gGL.diffuseColor4fv(col.mV);
	LLVector4a size;
	size = mObjectBounds[1];
	size.mul(1.01f);
	size.add(LLVector4a(0.001f));
	drawBox(mObjectBounds[0], size);
}

bool LLSpatialPartition::isHUDPartition() 
{ 
	return mPartitionType == LLViewerRegion::PARTITION_HUD ;
} 

BOOL LLSpatialPartition::isVisible(const LLVector3& v)
{
	if (!LLViewerCamera::getInstance()->sphereInFrustum(v, 4.0f))
	{
		return FALSE;
	}

	return TRUE;
}

LL_ALIGN_PREFIX(16)
class LLOctreeIntersect : public LLOctreeTraveler<LLViewerOctreeEntry>
{
public:
	LL_ALIGN_16(LLVector4a mStart);
	LL_ALIGN_16(LLVector4a mEnd);

	S32       *mFaceHit;
	LLVector4a *mIntersection;
	LLVector2 *mTexCoord;
	LLVector4a *mNormal;
	LLVector4a *mTangent;
	LLDrawable* mHit;
	BOOL mPickTransparent;
	BOOL mPickRigged;

	LLOctreeIntersect(const LLVector4a& start, const LLVector4a& end, BOOL pick_transparent, BOOL pick_rigged,
					  S32* face_hit, LLVector4a* intersection, LLVector2* tex_coord, LLVector4a* normal, LLVector4a* tangent)
		: mStart(start),
		  mEnd(end),
		  mFaceHit(face_hit),
		  mIntersection(intersection),
		  mTexCoord(tex_coord),
		  mNormal(normal),
		  mTangent(tangent),
		  mHit(NULL),
		  mPickTransparent(pick_transparent),
		  mPickRigged(pick_rigged)
	{
	}
	
	virtual void visit(const OctreeNode* branch) 
	{	
		for (OctreeNode::const_element_iter i = branch->getDataBegin(); i != branch->getDataEnd(); ++i)
		{
			check(*i);
		}
	}

	virtual LLDrawable* check(const OctreeNode* node)
	{
		node->accept(this);
	
		for (U32 i = 0; i < node->getChildCount(); i++)
		{
			const OctreeNode* child = node->getChild(i);
			LLVector3 res;

			LLSpatialGroup* group = (LLSpatialGroup*) child->getListener(0);
			
			LLVector4a size;
			LLVector4a center;
			
			const LLVector4a* bounds = group->getBounds();
			size = bounds[1];
			center = bounds[0];
			
			LLVector4a local_start = mStart;
			LLVector4a local_end   = mEnd;

			if (group->getSpatialPartition()->isBridge())
			{
				LLMatrix4 local_matrix = group->getSpatialPartition()->asBridge()->mDrawable->getRenderMatrix();
				local_matrix.invert();
				
				LLMatrix4a local_matrix4a;
				local_matrix4a.loadu(local_matrix);

				local_matrix4a.affineTransform(mStart, local_start);
				local_matrix4a.affineTransform(mEnd, local_end);
			}

			if (LLLineSegmentBoxIntersect(local_start, local_end, center, size))
			{
				check(child);
			}
		}	

		return mHit;
	}

	virtual bool check(LLViewerOctreeEntry* entry)
	{	
		LLDrawable* drawable = (LLDrawable*)entry->getDrawable();
	
		if (!drawable || !gPipeline.hasRenderType(drawable->getRenderType()) || !drawable->isVisible())
		{
			return false;
		}

		if (drawable->isSpatialBridge())
		{
			LLSpatialPartition *part = drawable->asPartition();
			LLSpatialBridge* bridge = part->asBridge();
			if (bridge && gPipeline.hasRenderType(bridge->mDrawableType))
			{
				check(part->mOctree);
			}
		}
		else
		{
			LLViewerObject* vobj = drawable->getVObj();

			if (vobj)
			{
				LLVector4a intersection;
				bool skip_check = false;
				if (vobj->isAvatar())
				{
					LLVOAvatar* avatar = (LLVOAvatar*) vobj;
					if ((mPickRigged) || ((avatar->isSelf()) && (LLFloater::isVisible(gFloaterTools))))
					{
						LLViewerObject* hit = avatar->lineSegmentIntersectRiggedAttachments(mStart, mEnd, -1, mPickTransparent, mPickRigged, mFaceHit, &intersection, mTexCoord, mNormal, mTangent);
						if (hit)
						{
							mEnd = intersection;
							if (mIntersection)
							{
								*mIntersection = intersection;
							}
							
							mHit = hit->mDrawable;
							skip_check = true;
						}

					}
				}

				if (!skip_check && vobj->lineSegmentIntersect(mStart, mEnd, -1, mPickTransparent, mPickRigged, mFaceHit, &intersection, mTexCoord, mNormal, mTangent))
				{
					mEnd = intersection;  // shorten ray so we only find CLOSER hits
					if (mIntersection)
					{
						*mIntersection = intersection;
					}
					
					mHit = vobj->mDrawable;
				}
			}
		}
				
		return false;
	}
} LL_ALIGN_POSTFIX(16);

LLDrawable* LLSpatialPartition::lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
													 BOOL pick_transparent,
													 BOOL pick_rigged,
													 S32* face_hit,                   // return the face hit
													 LLVector4a* intersection,         // return the intersection point
													 LLVector2* tex_coord,            // return the texture coordinates of the intersection point
													 LLVector4a* normal,               // return the surface normal at the intersection point
													 LLVector4a* tangent			// return the surface tangent at the intersection point
	)

{
	LLOctreeIntersect intersect(start, end, pick_transparent, pick_rigged, face_hit, intersection, tex_coord, normal, tangent);
	LLDrawable* drawable = intersect.check(mOctree);

	return drawable;
}

LLDrawInfo::LLDrawInfo(U16 start, U16 end, U32 count, U32 offset, 
					   LLViewerTexture* texture, LLVertexBuffer* buffer,
					   BOOL fullbright, U8 bump, BOOL particle, F32 part_size)
:	LLTrace::MemTrackableNonVirtual<LLDrawInfo, 16>("LLDrawInfo"),
	mVertexBuffer(buffer),
	mTexture(texture),
	mTextureMatrix(NULL),
	mModelMatrix(NULL),
	mStart(start),
	mEnd(end),
	mCount(count),
	mOffset(offset), 
	mFullbright(fullbright),
	mBump(bump),
	mParticle(particle),
	mPartSize(part_size),
	mVSize(0.f),
	mGroup(NULL),
	mFace(NULL),
	mDistance(0.f),
	mDrawMode(LLRender::TRIANGLES),
	mMaterial(NULL),
	mShaderMask(0),
	mSpecColor(1.0f, 1.0f, 1.0f, 0.5f),
	mBlendFuncSrc(LLRender::BF_SOURCE_ALPHA),
	mBlendFuncDst(LLRender::BF_ONE_MINUS_SOURCE_ALPHA),
	mHasGlow(FALSE),
	mEnvIntensity(0.0f),
	mAlphaMaskCutoff(0.5f),
	mDiffuseAlphaMode(0)
{
	mVertexBuffer->validateRange(mStart, mEnd, mCount, mOffset);
	
	mDebugColor = (rand() << 16) + rand();
}

LLDrawInfo::~LLDrawInfo()	
{
	/*if (LLSpatialGroup::sNoDelete)
	{
		LL_ERRS() << "LLDrawInfo deleted illegally!" << LL_ENDL;
	}*/

	if (mFace)
	{
		mFace->setDrawInfo(NULL);
	}

	if (gDebugGL)
	{
		gPipeline.checkReferences(this);
	}
}

void LLDrawInfo::validate()
{
	mVertexBuffer->validateRange(mStart, mEnd, mCount, mOffset);
}

LLVertexBuffer* LLGeometryManager::createVertexBuffer(U32 type_mask, U32 usage)
{
	return new LLVertexBuffer(type_mask, usage);
}

LLCullResult::LLCullResult() 
{
	mVisibleGroupsAllocated = 0;
	mAlphaGroupsAllocated = 0;
	mOcclusionGroupsAllocated = 0;
	mDrawableGroupsAllocated = 0;
	mVisibleListAllocated = 0;
	mVisibleBridgeAllocated = 0;

	mVisibleGroups.clear();
	mVisibleGroups.push_back(NULL);
	mVisibleGroupsEnd = &mVisibleGroups[0];
	mAlphaGroups.clear();
	mAlphaGroups.push_back(NULL);
	mAlphaGroupsEnd = &mAlphaGroups[0];
	mOcclusionGroups.clear();
	mOcclusionGroups.push_back(NULL);
	mOcclusionGroupsEnd = &mOcclusionGroups[0];
	mDrawableGroups.clear();
	mDrawableGroups.push_back(NULL);
	mDrawableGroupsEnd = &mDrawableGroups[0];
	mVisibleList.clear();
	mVisibleList.push_back(NULL);
	mVisibleListEnd = &mVisibleList[0];
	mVisibleBridge.clear();
	mVisibleBridge.push_back(NULL);
	mVisibleBridgeEnd = &mVisibleBridge[0];

	for (U32 i = 0; i < LLRenderPass::NUM_RENDER_TYPES; i++)
	{
		mRenderMap[i].clear();
		mRenderMap[i].push_back(NULL);
		mRenderMapEnd[i] = &mRenderMap[i][0];
		mRenderMapAllocated[i] = 0;
	}

	clear();
}

template <class T, class V> 
void LLCullResult::pushBack(T& head, U32& count, V* val)
{
	head[count] = val;
	head.push_back(NULL);
	count++;
}

void LLCullResult::clear()
{
	mVisibleGroupsSize = 0;
	mVisibleGroupsEnd = &mVisibleGroups[0];

	mAlphaGroupsSize = 0;
	mAlphaGroupsEnd = &mAlphaGroups[0];

	mOcclusionGroupsSize = 0;
	mOcclusionGroupsEnd = &mOcclusionGroups[0];

	mDrawableGroupsSize = 0;
	mDrawableGroupsEnd = &mDrawableGroups[0];

	mVisibleListSize = 0;
	mVisibleListEnd = &mVisibleList[0];

	mVisibleBridgeSize = 0;
	mVisibleBridgeEnd = &mVisibleBridge[0];


	for (U32 i = 0; i < LLRenderPass::NUM_RENDER_TYPES; i++)
	{
		for (U32 j = 0; j < mRenderMapSize[i]; j++)
		{
			mRenderMap[i][j] = 0;
		}
		mRenderMapSize[i] = 0;
		mRenderMapEnd[i] = &(mRenderMap[i][0]);
	}
}

LLCullResult::sg_iterator LLCullResult::beginVisibleGroups()
{
	return &mVisibleGroups[0];
}

LLCullResult::sg_iterator LLCullResult::endVisibleGroups()
{
	return mVisibleGroupsEnd;
}

LLCullResult::sg_iterator LLCullResult::beginAlphaGroups()
{
	return &mAlphaGroups[0];
}

LLCullResult::sg_iterator LLCullResult::endAlphaGroups()
{
	return mAlphaGroupsEnd;
}

LLCullResult::sg_iterator LLCullResult::beginOcclusionGroups()
{
	return &mOcclusionGroups[0];
}

LLCullResult::sg_iterator LLCullResult::endOcclusionGroups()
{
	return mOcclusionGroupsEnd;
}

LLCullResult::sg_iterator LLCullResult::beginDrawableGroups()
{
	return &mDrawableGroups[0];
}

LLCullResult::sg_iterator LLCullResult::endDrawableGroups()
{
	return mDrawableGroupsEnd;
}

LLCullResult::drawable_iterator LLCullResult::beginVisibleList()
{
	return &mVisibleList[0];
}

LLCullResult::drawable_iterator LLCullResult::endVisibleList()
{
	return mVisibleListEnd;
}

LLCullResult::bridge_iterator LLCullResult::beginVisibleBridge()
{
	return &mVisibleBridge[0];
}

LLCullResult::bridge_iterator LLCullResult::endVisibleBridge()
{
	return mVisibleBridgeEnd;
}

LLCullResult::drawinfo_iterator LLCullResult::beginRenderMap(U32 type)
{
	return &mRenderMap[type][0];
}

LLCullResult::drawinfo_iterator LLCullResult::endRenderMap(U32 type)
{
	return mRenderMapEnd[type];
}

void LLCullResult::pushVisibleGroup(LLSpatialGroup* group)
{
	if (mVisibleGroupsSize < mVisibleGroupsAllocated)
	{
		mVisibleGroups[mVisibleGroupsSize] = group;
	}
	else
	{
		pushBack(mVisibleGroups, mVisibleGroupsAllocated, group);
	}
	++mVisibleGroupsSize;
	mVisibleGroupsEnd = &mVisibleGroups[mVisibleGroupsSize];
}

void LLCullResult::pushAlphaGroup(LLSpatialGroup* group)
{
	if (mAlphaGroupsSize < mAlphaGroupsAllocated)
	{
		mAlphaGroups[mAlphaGroupsSize] = group;
	}
	else
	{
		pushBack(mAlphaGroups, mAlphaGroupsAllocated, group);
	}
	++mAlphaGroupsSize;
	mAlphaGroupsEnd = &mAlphaGroups[mAlphaGroupsSize];
}

void LLCullResult::pushOcclusionGroup(LLSpatialGroup* group)
{
	if (mOcclusionGroupsSize < mOcclusionGroupsAllocated)
	{
		mOcclusionGroups[mOcclusionGroupsSize] = group;
	}
	else
	{
		pushBack(mOcclusionGroups, mOcclusionGroupsAllocated, group);
	}
	++mOcclusionGroupsSize;
	mOcclusionGroupsEnd = &mOcclusionGroups[mOcclusionGroupsSize];
}

void LLCullResult::pushDrawableGroup(LLSpatialGroup* group)
{
	if (mDrawableGroupsSize < mDrawableGroupsAllocated)
	{
		mDrawableGroups[mDrawableGroupsSize] = group;
	}
	else
	{
		pushBack(mDrawableGroups, mDrawableGroupsAllocated, group);
	}
	++mDrawableGroupsSize;
	mDrawableGroupsEnd = &mDrawableGroups[mDrawableGroupsSize];
}

void LLCullResult::pushDrawable(LLDrawable* drawable)
{
	if (mVisibleListSize < mVisibleListAllocated)
	{
		mVisibleList[mVisibleListSize] = drawable;
	}
	else
	{
		pushBack(mVisibleList, mVisibleListAllocated, drawable);
	}
	++mVisibleListSize;
	mVisibleListEnd = &mVisibleList[mVisibleListSize];
}

void LLCullResult::pushBridge(LLSpatialBridge* bridge)
{
	if (mVisibleBridgeSize < mVisibleBridgeAllocated)
	{
		mVisibleBridge[mVisibleBridgeSize] = bridge;
	}
	else
	{
		pushBack(mVisibleBridge, mVisibleBridgeAllocated, bridge);
	}
	++mVisibleBridgeSize;
	mVisibleBridgeEnd = &mVisibleBridge[mVisibleBridgeSize];
}

void LLCullResult::pushDrawInfo(U32 type, LLDrawInfo* draw_info)
{
	if (mRenderMapSize[type] < mRenderMapAllocated[type])
	{
		mRenderMap[type][mRenderMapSize[type]] = draw_info;
	}
	else
	{
		pushBack(mRenderMap[type], mRenderMapAllocated[type], draw_info);
	}
	++mRenderMapSize[type];
	mRenderMapEnd[type] = &(mRenderMap[type][mRenderMapSize[type]]);
}


void LLCullResult::assertDrawMapsEmpty()
{
	for (U32 i = 0; i < LLRenderPass::NUM_RENDER_TYPES; i++)
	{
		if (mRenderMapSize[i] != 0)
		{
			LL_ERRS() << "Stale LLDrawInfo's in LLCullResult!" << LL_ENDL;
		}
	}
}


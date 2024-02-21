/** 
 * @file llworld.cpp
 * @brief Initial test structure to organize viewer regions
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

#include "llviewerprecompiledheaders.h"

#include "llworld.h"
#include "llrender.h"

#include "indra_constants.h"
#include "llstl.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llviewercontrol.h"
#include "lldrawpool.h"
#include "llglheaders.h"
#include "llhttpnode.h"
#include "llregionhandle.h"
#include "llsky.h"
#include "llsurface.h"
#include "lltrans.h"
#include "llviewercamera.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "llviewernetwork.h"
#include "llviewerobjectlist.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llvlcomposition.h"
#include "llvoavatar.h"
#include "llvocache.h"
#include "llvowater.h"
#include "message.h"
#include "pipeline.h"
#include "llappviewer.h"		// for do_disconnect()
#include "llscenemonitor.h"
#include <deque>
#include <queue>
#include <map>
#include <cstring>


//
// Globals
//
U32			gAgentPauseSerialNum = 0;

//
// Constants
//
const S32 WORLD_PATCH_SIZE = 16;

extern LLColor4U MAX_WATER_COLOR;

const U32 LLWorld::mWidth = 256;

// meters/point, therefore mWidth * mScale = meters per edge
const F32 LLWorld::mScale = 1.f;

const F32 LLWorld::mWidthInMeters = mWidth * mScale;

//
// Functions
//

// allocate the stack
LLWorld::LLWorld() :
	mLandFarClip(DEFAULT_FAR_PLANE),
	mLastPacketsIn(0),
	mLastPacketsOut(0),
	mLastPacketsLost(0),
	mSpaceTimeUSec(0)
{
	for (S32 i = 0; i < EDGE_WATER_OBJECTS_COUNT; i++)
	{
		mEdgeWaterObjects[i] = NULL;
	}

	LLPointer<LLImageRaw> raw = new LLImageRaw(1,1,4);
	U8 *default_texture = raw->getData();
	*(default_texture++) = MAX_WATER_COLOR.mV[0];
	*(default_texture++) = MAX_WATER_COLOR.mV[1];
	*(default_texture++) = MAX_WATER_COLOR.mV[2];
	*(default_texture++) = MAX_WATER_COLOR.mV[3];
	
	mDefaultWaterTexturep = LLViewerTextureManager::getLocalTexture(raw.get(), false);
	gGL.getTexUnit(0)->bind(mDefaultWaterTexturep);
	mDefaultWaterTexturep->setAddressMode(LLTexUnit::TAM_CLAMP);

    LLViewerRegion::sVOCacheCullingEnabled = gSavedSettings.getBOOL("RequestFullRegionCache") && gSavedSettings.getBOOL("ObjectCacheEnabled");
}


void LLWorld::resetClass()
{
	mHoleWaterObjects.clear();
	gObjectList.destroy();
    gSky.cleanup(); // references an object
	for(region_list_t::iterator region_it = mRegionList.begin(); region_it != mRegionList.end(); )
	{
		LLViewerRegion* region_to_delete = *region_it++;
		removeRegion(region_to_delete->getHost());
	}
	
	LLViewerPartSim::getInstance()->destroyClass();

	mDefaultWaterTexturep = NULL ;
	for (S32 i = 0; i < EDGE_WATER_OBJECTS_COUNT; i++)
	{
		mEdgeWaterObjects[i] = NULL;
	}

	//make all visible drawbles invisible.
	LLDrawable::incrementVisible();

	LLSceneMonitor::deleteSingleton();
}


LLViewerRegion* LLWorld::addRegion(const U64 &region_handle, const LLHost &host)
{
	LL_INFOS() << "Add region with handle: " << region_handle << " on host " << host << LL_ENDL;
	LLViewerRegion *regionp = getRegionFromHandle(region_handle);
	std::string seedUrl;
	if (regionp)
	{
		LLHost old_host = regionp->getHost();
		// region already exists!
		if (host == old_host && regionp->isAlive())
		{
			// This is a duplicate for the same host and it's alive, don't bother.
			LL_INFOS() << "Region already exists and is alive, using existing region" << LL_ENDL;
			return regionp;
		}

		if (host != old_host)
		{
			LL_WARNS() << "LLWorld::addRegion exists, but old host " << old_host
					<< " does not match new host " << host 
					<< ", removing old region and creating new" << LL_ENDL;
		}
		if (!regionp->isAlive())
		{
			LL_WARNS() << "LLWorld::addRegion exists, but isn't alive. Removing old region and creating new" << LL_ENDL;
		}

		// Save capabilities seed URL
		seedUrl = regionp->getCapability("Seed");

		// Kill the old host, and then we can continue on and add the new host.  We have to kill even if the host
		// matches, because all the agent state for the new camera is completely different.
		removeRegion(old_host);
	}
	else
	{
		LL_INFOS() << "Region does not exist, creating new one" << LL_ENDL;
	}

	U32 iindex = 0;
	U32 jindex = 0;
	from_region_handle(region_handle, &iindex, &jindex);
	S32 x = (S32)(iindex/mWidth);
	S32 y = (S32)(jindex/mWidth);
	LL_INFOS() << "Adding new region (" << x << ":" << y << ")" 
		<< " on host: " << host << LL_ENDL;

	LLVector3d origin_global;

	origin_global = from_region_handle(region_handle);

	regionp = new LLViewerRegion(region_handle,
								    host,
									mWidth,
									WORLD_PATCH_SIZE,
									getRegionWidthInMeters() );
	if (!regionp)
	{
		LL_ERRS() << "Unable to create new region!" << LL_ENDL;
	}

	if ( !seedUrl.empty() )
	{
		regionp->setCapability("Seed", seedUrl);
	}

	mRegionList.push_back(regionp);
	mActiveRegionList.push_back(regionp);
	mCulledRegionList.push_back(regionp);


	// Find all the adjacent regions, and attach them.
	// Generate handles for all of the adjacent regions, and attach them in the correct way.
	// connect the edges
	F32 adj_x = 0.f;
	F32 adj_y = 0.f;
	F32 region_x = 0.f;
	F32 region_y = 0.f;
	U64 adj_handle = 0;

	F32 width = getRegionWidthInMeters();

	LLViewerRegion *neighborp;
	from_region_handle(region_handle, &region_x, &region_y);

	// Iterate through all directions, and connect neighbors if there.
	S32 dir;
	for (dir = 0; dir < 8; dir++)
	{
		adj_x = region_x + width * gDirAxes[dir][0];
		adj_y = region_y + width * gDirAxes[dir][1];
		to_region_handle(adj_x, adj_y, &adj_handle);

		neighborp = getRegionFromHandle(adj_handle);
		if (neighborp)
		{
			//LL_INFOS() << "Connecting " << region_x << ":" << region_y << " -> " << adj_x << ":" << adj_y << LL_ENDL;
			regionp->connectNeighbor(neighborp, dir);
		}
	}

	updateWaterObjects();

	return regionp;
}


void LLWorld::removeRegion(const LLHost &host)
{
	F32 x, y;

	LLViewerRegion *regionp = getRegion(host);
	if (!regionp)
	{
		LL_WARNS() << "Trying to remove region that doesn't exist!" << LL_ENDL;
		return;
	}
	
	if (regionp == gAgent.getRegion())
	{
		for (region_list_t::iterator iter = mRegionList.begin();
			 iter != mRegionList.end(); ++iter)
		{
			LLViewerRegion* reg = *iter;
			LL_WARNS() << "RegionDump: " << reg->getName()
				<< " " << reg->getHost()
				<< " " << reg->getOriginGlobal()
				<< LL_ENDL;
		}

		LL_WARNS() << "Agent position global " << gAgent.getPositionGlobal() 
			<< " agent " << gAgent.getPositionAgent()
			<< LL_ENDL;

		LL_WARNS() << "Regions visited " << gAgent.getRegionsVisited() << LL_ENDL;

		LL_WARNS() << "gFrameTimeSeconds " << gFrameTimeSeconds << LL_ENDL;

		LL_WARNS() << "Disabling region " << regionp->getName() << " that agent is in!" << LL_ENDL;
		LLAppViewer::instance()->forceDisconnect(LLTrans::getString("YouHaveBeenDisconnected"));

		regionp->saveObjectCache() ; //force to save objects here in case that the object cache is about to be destroyed.
		return;
	}

	from_region_handle(regionp->getHandle(), &x, &y);
	LL_INFOS() << "Removing region " << x << ":" << y << LL_ENDL;

	mRegionList.remove(regionp);
	mActiveRegionList.remove(regionp);
	mCulledRegionList.remove(regionp);
	mVisibleRegionList.remove(regionp);

	mRegionRemovedSignal(regionp);

	updateWaterObjects();

	//double check all objects of this region are removed.
	gObjectList.clearAllMapObjectsInRegion(regionp) ;
	//llassert_always(!gObjectList.hasMapObjectInRegion(regionp)) ;

	delete regionp; // Delete last to prevent use after free
}


LLViewerRegion* LLWorld::getRegion(const LLHost &host)
{
	for (region_list_t::iterator iter = mRegionList.begin();
		 iter != mRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		if (regionp->getHost() == host)
		{
			return regionp;
		}
	}
	return NULL;
}

LLViewerRegion* LLWorld::getRegionFromPosAgent(const LLVector3 &pos)
{
	return getRegionFromPosGlobal(gAgent.getPosGlobalFromAgent(pos));
}

LLViewerRegion* LLWorld::getRegionFromPosGlobal(const LLVector3d &pos)
{
	for (region_list_t::iterator iter = mRegionList.begin();
		 iter != mRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		if (regionp->pointInRegionGlobal(pos))
		{
			return regionp;
		}
	}
	return NULL;
}


LLVector3d	LLWorld::clipToVisibleRegions(const LLVector3d &start_pos, const LLVector3d &end_pos)
{
	if (positionRegionValidGlobal(end_pos))
	{
		return end_pos;
	}

	LLViewerRegion* regionp = getRegionFromPosGlobal(start_pos);
	if (!regionp) 
	{
		return start_pos;
	}

	LLVector3d delta_pos = end_pos - start_pos;
	LLVector3d delta_pos_abs;
	delta_pos_abs.setVec(delta_pos);
	delta_pos_abs.abs();

	LLVector3 region_coord = regionp->getPosRegionFromGlobal(end_pos);
	F64 clip_factor = 1.0;
	F32 region_width = regionp->getWidth();
	if (region_coord.mV[VX] < 0.f)
	{
		if (region_coord.mV[VY] < region_coord.mV[VX])
		{
			// clip along y -
			clip_factor = -(region_coord.mV[VY] / delta_pos_abs.mdV[VY]);
		}
		else
		{
			// clip along x -
			clip_factor = -(region_coord.mV[VX] / delta_pos_abs.mdV[VX]);
		}
	}
	else if (region_coord.mV[VX] > region_width)
	{
		if (region_coord.mV[VY] > region_coord.mV[VX])
		{
			// clip along y +
			clip_factor = (region_coord.mV[VY] - region_width) / delta_pos_abs.mdV[VY];
		}
		else
		{
			//clip along x +
			clip_factor = (region_coord.mV[VX] - region_width) / delta_pos_abs.mdV[VX];
		}
	}
	else if (region_coord.mV[VY] < 0.f)
	{
		// clip along y -
		clip_factor = -(region_coord.mV[VY] / delta_pos_abs.mdV[VY]);
	}
	else if (region_coord.mV[VY] > region_width)
	{ 
		// clip along y +
		clip_factor = (region_coord.mV[VY] - region_width) / delta_pos_abs.mdV[VY];
	}

	// clamp to within region dimensions
	LLVector3d final_region_pos = LLVector3d(region_coord) - (delta_pos * clip_factor);
	final_region_pos.mdV[VX] = llclamp(final_region_pos.mdV[VX], 0.0,
									   (F64)(region_width - F_ALMOST_ZERO));
	final_region_pos.mdV[VY] = llclamp(final_region_pos.mdV[VY], 0.0,
									   (F64)(region_width - F_ALMOST_ZERO));
	final_region_pos.mdV[VZ] = llclamp(final_region_pos.mdV[VZ], 0.0,
									   (F64)(LLWorld::getInstance()->getRegionMaxHeight() - F_ALMOST_ZERO));
	return regionp->getPosGlobalFromRegion(LLVector3(final_region_pos));
}

LLViewerRegion* LLWorld::getRegionFromHandle(const U64 &handle)
{
	for (region_list_t::iterator iter = mRegionList.begin();
		 iter != mRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		if (regionp->getHandle() == handle)
		{
			return regionp;
		}
	}
	return NULL;
}

LLViewerRegion* LLWorld::getRegionFromID(const LLUUID& region_id)
{
	for (region_list_t::iterator iter = mRegionList.begin();
		 iter != mRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		if (regionp->getRegionID() == region_id)
		{
			return regionp;
		}
	}
	return NULL;
}

void LLWorld::updateAgentOffset(const LLVector3d &offset_global)
{
#if 0
	for (region_list_t::iterator iter = mRegionList.begin();
		 iter != mRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		regionp->setAgentOffset(offset_global);
	}
#endif
}


bool LLWorld::positionRegionValidGlobal(const LLVector3d &pos_global)
{
	for (region_list_t::iterator iter = mRegionList.begin();
		 iter != mRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		if (regionp->pointInRegionGlobal(pos_global))
		{
			return true;
		}
	}
	return false;
}


// Allow objects to go up to their radius underground.
F32 LLWorld::getMinAllowedZ(LLViewerObject* object, const LLVector3d &global_pos)
{
	F32 land_height = resolveLandHeightGlobal(global_pos);
	F32 radius = 0.5f * object->getScale().length();
	return land_height - radius;
}



LLViewerRegion* LLWorld::resolveRegionGlobal(LLVector3 &pos_region, const LLVector3d &pos_global)
{
	LLViewerRegion *regionp = getRegionFromPosGlobal(pos_global);

	if (regionp)
	{
		pos_region = regionp->getPosRegionFromGlobal(pos_global);
		return regionp;
	}

	return NULL;
}


LLViewerRegion* LLWorld::resolveRegionAgent(LLVector3 &pos_region, const LLVector3 &pos_agent)
{
	LLVector3d pos_global = gAgent.getPosGlobalFromAgent(pos_agent);
	LLViewerRegion *regionp = getRegionFromPosGlobal(pos_global);

	if (regionp)
	{
		pos_region = regionp->getPosRegionFromGlobal(pos_global);
		return regionp;
	}

	return NULL;
}


F32 LLWorld::resolveLandHeightAgent(const LLVector3 &pos_agent)
{
	LLVector3d pos_global = gAgent.getPosGlobalFromAgent(pos_agent);
	return resolveLandHeightGlobal(pos_global);
}


F32 LLWorld::resolveLandHeightGlobal(const LLVector3d &pos_global)
{
	LLViewerRegion *regionp = getRegionFromPosGlobal(pos_global);
	if (regionp)
	{
		return regionp->getLand().resolveHeightGlobal(pos_global);
	}
	return 0.0f;
}


// Takes a line defined by "point_a" and "point_b" and determines the closest (to point_a) 
// point where the the line intersects an object or the land surface.  Stores the results
// in "intersection" and "intersection_normal" and returns a scalar value that represents
// the normalized distance along the line from "point_a" to "intersection".
//
// Currently assumes point_a and point_b only differ in z-direction, 
// but it may eventually become more general.
F32 LLWorld::resolveStepHeightGlobal(const LLVOAvatar* avatarp, const LLVector3d &point_a, const LLVector3d &point_b, 
							   LLVector3d &intersection, LLVector3 &intersection_normal,
							   LLViewerObject **viewerObjectPtr)
{
	// initialize return value to null
	if (viewerObjectPtr)
	{
		*viewerObjectPtr = NULL;
	}

	LLViewerRegion *regionp = getRegionFromPosGlobal(point_a);
	if (!regionp)
	{
		// We're outside the world 
		intersection = 0.5f * (point_a + point_b);
		intersection_normal.setVec(0.0f, 0.0f, 1.0f);
		return 0.5f;
	}
	
	// calculate the length of the segment
	F32 segment_length = (F32)((point_a - point_b).length());
	if (0.0f == segment_length)
	{
		intersection = point_a;
		intersection_normal.setVec(0.0f, 0.0f, 1.0f);
		return segment_length;
	}

	// get land height	
	// Note: we assume that the line is parallel to z-axis here
	LLVector3d land_intersection = point_a;
	F32 normalized_land_distance;

	land_intersection.mdV[VZ] = regionp->getLand().resolveHeightGlobal(point_a);
	normalized_land_distance = (F32)(point_a.mdV[VZ] - land_intersection.mdV[VZ]) / segment_length;
	intersection = land_intersection;
	intersection_normal = resolveLandNormalGlobal(land_intersection);

	if (avatarp && !avatarp->mFootPlane.isExactlyClear())
	{
		LLVector3 foot_plane_normal(avatarp->mFootPlane.mV);
		LLVector3 start_pt = avatarp->getRegion()->getPosRegionFromGlobal(point_a);
		// added 0.05 meters to compensate for error in foot plane reported by Havok
		F32 norm_dist_from_plane = ((start_pt * foot_plane_normal) - avatarp->mFootPlane.mV[VW]) + 0.05f;
		norm_dist_from_plane = llclamp(norm_dist_from_plane / segment_length, 0.f, 1.f);
		if (norm_dist_from_plane < normalized_land_distance)
		{
			// collided with object before land
			normalized_land_distance = norm_dist_from_plane;
			intersection = point_a;
			intersection.mdV[VZ] -= norm_dist_from_plane * segment_length;
			intersection_normal = foot_plane_normal;
		}
		else
		{
			intersection = land_intersection;
			intersection_normal = resolveLandNormalGlobal(land_intersection);
		}
	}

	return normalized_land_distance;
}


LLSurfacePatch * LLWorld::resolveLandPatchGlobal(const LLVector3d &pos_global)
{
	//  returns a pointer to the patch at this location
	LLViewerRegion *regionp = getRegionFromPosGlobal(pos_global);
	if (!regionp)
	{
		return NULL;
	}

	return regionp->getLand().resolvePatchGlobal(pos_global);
}


LLVector3 LLWorld::resolveLandNormalGlobal(const LLVector3d &pos_global)
{
	LLViewerRegion *regionp = getRegionFromPosGlobal(pos_global);
	if (!regionp)
	{
		return LLVector3::z_axis;
	}

	return regionp->getLand().resolveNormalGlobal(pos_global);
}


void LLWorld::updateVisibilities()
{
	F32 cur_far_clip = LLViewerCamera::getInstance()->getFar();

	// Go through the culled list and check for visible regions (region is visible if land is visible)
	for (region_list_t::iterator iter = mCulledRegionList.begin();
		 iter != mCulledRegionList.end(); )
	{
		region_list_t::iterator curiter = iter++;
		LLViewerRegion* regionp = *curiter;
		
		LLSpatialPartition* part = regionp->getSpatialPartition(LLViewerRegion::PARTITION_TERRAIN);
		if (part)
		{
			LLSpatialGroup* group = (LLSpatialGroup*) part->mOctree->getListener(0);
			const LLVector4a* bounds = group->getBounds();
			if (LLViewerCamera::getInstance()->AABBInFrustum(bounds[0], bounds[1]))
			{
				mCulledRegionList.erase(curiter);
				mVisibleRegionList.push_back(regionp);
			}
		}
	}
	
	// Update all of the visible regions 
	for (region_list_t::iterator iter = mVisibleRegionList.begin();
		 iter != mVisibleRegionList.end(); )
	{
		region_list_t::iterator curiter = iter++;
		LLViewerRegion* regionp = *curiter;
		if (!regionp->getLand().hasZData())
		{
			continue;
		}

		LLSpatialPartition* part = regionp->getSpatialPartition(LLViewerRegion::PARTITION_TERRAIN);
		if (part)
		{
			LLSpatialGroup* group = (LLSpatialGroup*) part->mOctree->getListener(0);
			const LLVector4a* bounds = group->getBounds();
			if (LLViewerCamera::getInstance()->AABBInFrustum(bounds[0], bounds[1]))
			{
				regionp->calculateCameraDistance();
				regionp->getLand().updatePatchVisibilities(gAgent);
			}
			else
			{
				mVisibleRegionList.erase(curiter);
				mCulledRegionList.push_back(regionp);
			}
		}
	}

	// Sort visible regions
	mVisibleRegionList.sort(LLViewerRegion::CompareDistance());
	
	LLViewerCamera::getInstance()->setFar(cur_far_clip);
}

static LLTrace::SampleStatHandle<> sNumActiveCachedObjects("numactivecachedobjects", "Number of objects loaded from cache");

void LLWorld::updateRegions(F32 max_update_time)
{
    LL_PROFILE_ZONE_SCOPED;
	LLTimer update_timer;
	mNumOfActiveCachedObjects = 0;
	
	if(LLViewerCamera::getInstance()->isChanged())
	{
		LLViewerRegion::sLastCameraUpdated = LLViewerOctreeEntryData::getCurrentFrame() + 1;
	}
	LLViewerRegion::calcNewObjectCreationThrottle();
	if(LLViewerRegion::isNewObjectCreationThrottleDisabled())
	{
		max_update_time = llmax(max_update_time, 1.0f); //seconds, loosen the time throttle.
	}

	F32 max_time = llmin((F32)(max_update_time - update_timer.getElapsedTimeF32()), max_update_time * 0.25f);
	//update the self avatar region
	LLViewerRegion* self_regionp = gAgent.getRegion();
	if(self_regionp)
	{		
		self_regionp->idleUpdate(max_time);
	}

	//sort regions by its mLastUpdate
	//smaller mLastUpdate first to make sure every region has chance to get updated.
	LLViewerRegion::region_priority_list_t region_list;
	for (region_list_t::iterator iter = mRegionList.begin();
		 iter != mRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		if(regionp != self_regionp)
		{
			region_list.insert(regionp);
		}
		mNumOfActiveCachedObjects += regionp->getNumOfActiveCachedObjects();
	}

	// Perform idle time updates for the regions (and associated surfaces)
	for (LLViewerRegion::region_priority_list_t::iterator iter = region_list.begin();
		 iter != region_list.end(); ++iter)
	{
		if(max_time > 0.f)
		{
			max_time = llmin((F32)(max_update_time - update_timer.getElapsedTimeF32()), max_update_time * 0.25f);
		}

		if(max_time > 0.f)
		{
			(*iter)->idleUpdate(max_time);
		}
		else
		{
			//perform some necessary but very light updates.
			(*iter)->lightIdleUpdate();
		}
	}

	if(max_time > 0.f)
	{
		max_time = llmin((F32)(max_update_time - update_timer.getElapsedTimeF32()), max_update_time * 0.25f);
	}
	if(max_time > 0.f)
	{
		LLViewerRegion::idleCleanup(max_time);
	}

	sample(sNumActiveCachedObjects, mNumOfActiveCachedObjects);
}

void LLWorld::clearAllVisibleObjects()
{
	for (region_list_t::iterator iter = mRegionList.begin();
		 iter != mRegionList.end(); ++iter)
	{
		//clear all cached visible objects.
		(*iter)->clearCachedVisibleObjects();
	}
    clearHoleWaterObjects();
    clearEdgeWaterObjects();
}

void LLWorld::updateParticles()
{
	LLViewerPartSim::getInstance()->updateSimulation();
}

void LLWorld::renderPropertyLines()
{
	for (region_list_t::iterator iter = mVisibleRegionList.begin();
		 iter != mVisibleRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		regionp->renderPropertyLines();
	}
}


void LLWorld::updateNetStats()
{
	F64Bits bits;

	for (region_list_t::iterator iter = mActiveRegionList.begin();
		 iter != mActiveRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		regionp->updateNetStats();
		bits += regionp->mBitsReceived;
		regionp->mBitsReceived = (F32Bits)0.f;
		regionp->mPacketsReceived = 0.f;
	}

	S32 packets_in = gMessageSystem->mPacketsIn - mLastPacketsIn;
	S32 packets_out = gMessageSystem->mPacketsOut - mLastPacketsOut;
	S32 packets_lost = gMessageSystem->mDroppedPackets - mLastPacketsLost;

	F64Bits actual_in_bits(gMessageSystem->mPacketRing.getAndResetActualInBits());
	F64Bits actual_out_bits(gMessageSystem->mPacketRing.getAndResetActualOutBits());

	add(LLStatViewer::MESSAGE_SYSTEM_DATA_IN, actual_in_bits);
	add(LLStatViewer::MESSAGE_SYSTEM_DATA_OUT, actual_out_bits);
	add(LLStatViewer::ACTIVE_MESSAGE_DATA_RECEIVED, bits);
	add(LLStatViewer::PACKETS_IN, packets_in);
	add(LLStatViewer::PACKETS_OUT, packets_out);
	add(LLStatViewer::PACKETS_LOST, packets_lost);

	F32 total_packets_in = LLViewerStats::instance().getRecording().getSum(LLStatViewer::PACKETS_IN);
	if (total_packets_in > 0)
	{
		F32 total_packets_lost = LLViewerStats::instance().getRecording().getSum(LLStatViewer::PACKETS_LOST);
		sample(LLStatViewer::PACKETS_LOST_PERCENT, LLUnits::Ratio::fromValue((F32)total_packets_lost/(F32)total_packets_in));
	}

	mLastPacketsIn = gMessageSystem->mPacketsIn;
	mLastPacketsOut = gMessageSystem->mPacketsOut;
	mLastPacketsLost = gMessageSystem->mDroppedPackets;
}


void LLWorld::printPacketsLost()
{
	LL_INFOS() << "Simulators:" << LL_ENDL; 
	LL_INFOS() << "----------" << LL_ENDL;

	LLCircuitData *cdp = NULL;
	for (region_list_t::iterator iter = mActiveRegionList.begin();
		 iter != mActiveRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		cdp = gMessageSystem->mCircuitInfo.findCircuit(regionp->getHost());
		if (cdp)
		{
			LLVector3d range = regionp->getCenterGlobal() - gAgent.getPositionGlobal();
				
			LL_INFOS() << regionp->getHost() << ", range: " << range.length()
					<< " packets lost: " << cdp->getPacketsLost() << LL_ENDL;
		}
	}
}

void LLWorld::processCoarseUpdate(LLMessageSystem* msg, void** user_data)
{
	LLViewerRegion* region = LLWorld::getInstance()->getRegion(msg->getSender());
	if( region )
	{
		region->updateCoarseLocations(msg);
	}
}

F32 LLWorld::getLandFarClip() const
{
	return mLandFarClip;
}

void LLWorld::setLandFarClip(const F32 far_clip)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_ENVIRONMENT;
	static S32 const rwidth = (S32)REGION_WIDTH_U32;
	S32 const n1 = (llceil(mLandFarClip) - 1) / rwidth;
	S32 const n2 = (llceil(far_clip) - 1) / rwidth;
	bool need_water_objects_update = n1 != n2;

	mLandFarClip = far_clip;

	if (need_water_objects_update)
	{
		updateWaterObjects();
	}
}

// Some region that we're connected to, but not the one we're in, gave us
// a (possibly) new water height. Update it in our local copy.
void LLWorld::waterHeightRegionInfo(std::string const& sim_name, F32 water_height)
{
	for (region_list_t::iterator iter = mRegionList.begin(); iter != mRegionList.end(); ++iter)
	{
		if ((*iter)->getName() == sim_name)
		{
			(*iter)->setWaterHeight(water_height);
			break;
		}
	}
}

void LLWorld::clearHoleWaterObjects()
{
    for (std::list<LLPointer<LLVOWater> >::iterator iter = mHoleWaterObjects.begin();
        iter != mHoleWaterObjects.end(); ++iter)
    {
        LLVOWater* waterp = (*iter).get();
        gObjectList.killObject(waterp);
    }
    mHoleWaterObjects.clear();
}

void LLWorld::clearEdgeWaterObjects()
{
    for (S32 i = 0; i < EDGE_WATER_OBJECTS_COUNT; i++)
    {
        gObjectList.killObject(mEdgeWaterObjects[i]);
        mEdgeWaterObjects[i] = NULL;
    }
}

void LLWorld::updateWaterObjects()
{
	if (!gAgent.getRegion())
	{
		return;
	}
	if (mRegionList.empty())
	{
		LL_WARNS() << "No regions!" << LL_ENDL;
		return;
	}

	// First, determine the min and max "box" of water objects
	S32 min_x = 0;
	S32 min_y = 0;
	S32 max_x = 0;
	S32 max_y = 0;
	U32 region_x, region_y;

	S32 rwidth = 256;

	// We only want to fill in water for stuff that's near us, say, within 256 or 512m
	S32 range = LLViewerCamera::getInstance()->getFar() > 256.f ? 512 : 256;

	LLViewerRegion* regionp = gAgent.getRegion();
	from_region_handle(regionp->getHandle(), &region_x, &region_y);

	min_x = (S32)region_x - range;
	min_y = (S32)region_y - range;
	max_x = (S32)region_x + range;
	max_y = (S32)region_y + range;

	for (region_list_t::iterator iter = mRegionList.begin();
		 iter != mRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		LLVOWater* waterp = regionp->getLand().getWaterObj();
		if (waterp)
		{
			gObjectList.updateActive(waterp);
		}
	}

    clearHoleWaterObjects();

	// Use the water height of the region we're on for areas where there is no region
	F32 water_height = gAgent.getRegion()->getWaterHeight();

	// Now, get a list of the holes
	S32 x, y;
	for (x = min_x; x <= max_x; x += rwidth)
	{
		for (y = min_y; y <= max_y; y += rwidth)
		{
			U64 region_handle = to_region_handle(x, y);
			if (!getRegionFromHandle(region_handle))
			{	// No region at that area, so make water
				LLVOWater* waterp = (LLVOWater *)gObjectList.createObjectViewer(LLViewerObject::LL_VO_WATER, gAgent.getRegion());
				waterp->setUseTexture(false);
				waterp->setPositionGlobal(LLVector3d(x + rwidth/2,
													 y + rwidth/2,
													 256.f + water_height));
				waterp->setScale(LLVector3((F32)rwidth, (F32)rwidth, 512.f));
				gPipeline.createObject(waterp);
				mHoleWaterObjects.push_back(waterp);
			}
		}
	}

	// Update edge water objects
	S32 wx, wy;
	S32 center_x, center_y;
	wx = (max_x - min_x) + rwidth;
	wy = (max_y - min_y) + rwidth;
	center_x = min_x + (wx >> 1);
	center_y = min_y + (wy >> 1);

	S32 add_boundary[4] = {
		(S32)(512 - (max_x - region_x)),
		(S32)(512 - (max_y - region_y)),
		(S32)(512 - (region_x - min_x)),
		(S32)(512 - (region_y - min_y)) };
		
	S32 dir;
	for (dir = 0; dir < EDGE_WATER_OBJECTS_COUNT; dir++)
	{
		S32 dim[2] = { 0 };
		switch (gDirAxes[dir][0])
		{
		case -1: dim[0] = add_boundary[2]; break;
		case  0: dim[0] = wx; break;
		default: dim[0] = add_boundary[0]; break;
		}
		switch (gDirAxes[dir][1])
		{
		case -1: dim[1] = add_boundary[3]; break;
		case  0: dim[1] = wy; break;
		default: dim[1] = add_boundary[1]; break;
		}

		// Resize and reshape the water objects
		const S32 water_center_x = center_x + ll_round((wx + dim[0]) * 0.5f * gDirAxes[dir][0]);
		const S32 water_center_y = center_y + ll_round((wy + dim[1]) * 0.5f * gDirAxes[dir][1]);
		
		LLVOWater* waterp = mEdgeWaterObjects[dir];
		if (!waterp || waterp->isDead())
		{
			// The edge water objects can be dead because they're attached to the region that the
			// agent was in when they were originally created.
			mEdgeWaterObjects[dir] = (LLVOWater *)gObjectList.createObjectViewer(LLViewerObject::LL_VO_VOID_WATER,
																				 gAgent.getRegion());
			waterp = mEdgeWaterObjects[dir];
			waterp->setUseTexture(false);
			waterp->setIsEdgePatch(true);
			gPipeline.createObject(waterp);
		}

		waterp->setRegion(gAgent.getRegion());
		LLVector3d water_pos(water_center_x, water_center_y, 256.f + water_height) ;
		LLVector3 water_scale((F32) dim[0], (F32) dim[1], 512.f);

		//stretch out to horizon
		water_scale.mV[0] += fabsf(2048.f * gDirAxes[dir][0]);
		water_scale.mV[1] += fabsf(2048.f * gDirAxes[dir][1]);

		water_pos.mdV[0] += 1024.f * gDirAxes[dir][0];
		water_pos.mdV[1] += 1024.f * gDirAxes[dir][1];

		waterp->setPositionGlobal(water_pos);
		waterp->setScale(water_scale);

		gObjectList.updateActive(waterp);
	}
}


void LLWorld::shiftRegions(const LLVector3& offset)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
	for (region_list_t::const_iterator i = getRegionList().begin(); i != getRegionList().end(); ++i)
	{
		LLViewerRegion* region = *i;
		region->updateRenderMatrix();
	}

	LLViewerPartSim::getInstance()->shift(offset);
}

LLViewerTexture* LLWorld::getDefaultWaterTexture()
{
	return mDefaultWaterTexturep;
}

void LLWorld::setSpaceTimeUSec(const U64MicrosecondsImplicit space_time_usec)
{
	mSpaceTimeUSec = space_time_usec;
}

U64MicrosecondsImplicit LLWorld::getSpaceTimeUSec() const
{
	return mSpaceTimeUSec;
}

void LLWorld::requestCacheMisses()
{
	for (region_list_t::iterator iter = mRegionList.begin();
		 iter != mRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		regionp->requestCacheMisses();
	}
}

void LLWorld::getInfo(LLSD& info)
{
	LLSD region_info;
	for (region_list_t::iterator iter = mRegionList.begin();
		 iter != mRegionList.end(); ++iter)
	{	
		LLViewerRegion* regionp = *iter;
		regionp->getInfo(region_info);
		info["World"].append(region_info);
	}
}

void LLWorld::disconnectRegions()
{
	LLMessageSystem* msg = gMessageSystem;
	for (region_list_t::iterator iter = mRegionList.begin();
		 iter != mRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		if (regionp == gAgent.getRegion())
		{
			// Skip the main agent
			continue;
		}

		LL_INFOS() << "Sending AgentQuitCopy to: " << regionp->getHost() << LL_ENDL;
		msg->newMessageFast(_PREHASH_AgentQuitCopy);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_FuseBlock);
		msg->addU32Fast(_PREHASH_ViewerCircuitCode, gMessageSystem->mOurCircuitCode);
		msg->sendMessage(regionp->getHost());
	}
}

void process_enable_simulator(LLMessageSystem *msg, void **user_data)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_NETWORK;
	// enable the appropriate circuit for this simulator and 
	// add its values into the gSimulator structure
	U64		handle;
	U32		ip_u32;
	U16		port;

	msg->getU64Fast(_PREHASH_SimulatorInfo, _PREHASH_Handle, handle);
	msg->getIPAddrFast(_PREHASH_SimulatorInfo, _PREHASH_IP, ip_u32);
	msg->getIPPortFast(_PREHASH_SimulatorInfo, _PREHASH_Port, port);

	// which simulator should we modify?
	LLHost sim(ip_u32, port);

	// Viewer trusts the simulator.
	msg->enableCircuit(sim, true);
	LLWorld::getInstance()->addRegion(handle, sim);

	// give the simulator a message it can use to get ip and port
	LL_INFOS() << "simulator_enable() Enabling " << sim << " with code " << msg->getOurCircuitCode() << LL_ENDL;
	msg->newMessageFast(_PREHASH_UseCircuitCode);
	msg->nextBlockFast(_PREHASH_CircuitCode);
	msg->addU32Fast(_PREHASH_Code, msg->getOurCircuitCode());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_ID, gAgent.getID());
	msg->sendReliable(sim);
}

class LLEstablishAgentCommunication : public LLHTTPNode
{
	LOG_CLASS(LLEstablishAgentCommunication);
public:
 	virtual void describe(Description& desc) const
	{
		desc.shortInfo("seed capability info for a region");
		desc.postAPI();
		desc.input(
			"{ seed-capability: ..., sim-ip: ..., sim-port }");
		desc.source(__FILE__, __LINE__);
	}

	virtual void post(ResponsePtr response, const LLSD& context, const LLSD& input) const
	{
        if (LLApp::isExiting())
        {
            return;
        }

        if (gDisconnected)
        {
            return;
        }

        if (!LLWorld::instanceExists())
        {
            return;
        }

		if (!input["body"].has("agent-id") ||
			!input["body"].has("sim-ip-and-port") ||
			!input["body"].has("seed-capability"))
		{
			LL_WARNS() << "invalid parameters" << LL_ENDL;
            return;
		}

        LLHost sim(input["body"]["sim-ip-and-port"].asString());
        if (sim.isInvalid())
        {
            LL_WARNS() << "Got EstablishAgentCommunication with invalid host" << LL_ENDL;
            return;
        }

		LLViewerRegion* regionp = LLWorld::getInstance()->getRegion(sim);
		if (!regionp)
		{
			LL_WARNS() << "Got EstablishAgentCommunication for unknown region "
					<< sim << LL_ENDL;
			return;
		}
		LL_DEBUGS("CrossingCaps") << "Calling setSeedCapability from LLEstablishAgentCommunication::post. Seed cap == "
				<< input["body"]["seed-capability"] << " for region " << regionp->getRegionID() << LL_ENDL;
		regionp->setSeedCapability(input["body"]["seed-capability"]);
	}
};

// disable the circuit to this simulator
// Called in response to "DisableSimulator" message.
void process_disable_simulator(LLMessageSystem *mesgsys, void **user_data)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_NETWORK;

    LLHost host = mesgsys->getSender();

	//LL_INFOS() << "Disabling simulator with message from " << host << LL_ENDL;
	LLWorld::getInstance()->removeRegion(host);

	mesgsys->disableCircuit(host);
}


void process_region_handshake(LLMessageSystem* msg, void** user_data)
{
	LLHost host = msg->getSender();
	LLViewerRegion* regionp = LLWorld::getInstance()->getRegion(host);
	if (!regionp)
	{
		LL_WARNS() << "Got region handshake for unknown region "
			<< host << LL_ENDL;
		return;
	}

	regionp->unpackRegionHandshake();
}


void send_agent_pause()
{
	// *NOTE:Mani Pausing the mainloop timeout. Otherwise a long modal event may cause
	// the thread monitor to timeout.
	LLAppViewer::instance()->pauseMainloopTimeout();
	
	// Note: used to check for LLWorld initialization before it became a singleton.
	// Rather than just remove this check I'm changing it to assure that the message 
	// system has been initialized. -MG
	if (!gMessageSystem)
	{
		return;
	}
	
	gMessageSystem->newMessageFast(_PREHASH_AgentPause);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgentID);
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgentSessionID);

	gAgentPauseSerialNum++;
	gMessageSystem->addU32Fast(_PREHASH_SerialNum, gAgentPauseSerialNum);

	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
		 iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		gMessageSystem->sendReliable(regionp->getHost());
	}

	gObjectList.mWasPaused = true;
	LLViewerStats::instance().getRecording().stop();
}


void send_agent_resume()
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_NETWORK
	// Note: used to check for LLWorld initialization before it became a singleton.
	// Rather than just remove this check I'm changing it to assure that the message 
	// system has been initialized. -MG
	if (!gMessageSystem)
	{
		return;
	}

	gMessageSystem->newMessageFast(_PREHASH_AgentResume);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgentID);
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgentSessionID);

	gAgentPauseSerialNum++;
	gMessageSystem->addU32Fast(_PREHASH_SerialNum, gAgentPauseSerialNum);
	

	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
		 iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		gMessageSystem->sendReliable(regionp->getHost());
	}

	// Resume data collection to ignore invalid rates
	LLViewerStats::instance().getRecording().resume();

	LLAppViewer::instance()->resumeMainloopTimeout();
}

static LLVector3d unpackLocalToGlobalPosition(U32 compact_local, const LLVector3d& region_origin)
{
	LLVector3d pos_local;

	pos_local.mdV[VZ] = (compact_local & 0xFFU) * 4;
	pos_local.mdV[VY] = (compact_local >> 8) & 0xFFU;
	pos_local.mdV[VX] = (compact_local >> 16) & 0xFFU;

	return region_origin + pos_local;
}

void LLWorld::getAvatars(uuid_vec_t* avatar_ids, std::vector<LLVector3d>* positions, const LLVector3d& relative_to, F32 radius) const
{
	F32 radius_squared = radius * radius;
	
	if(avatar_ids != NULL)
	{
		avatar_ids->clear();
	}
	if(positions != NULL)
	{
		positions->clear();
	}
	// get the list of avatars from the character list first, so distances are correct
	// when agent is above 1020m and other avatars are nearby
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* pVOAvatar = (LLVOAvatar*) *iter;

		if (!pVOAvatar->isDead() && !pVOAvatar->mIsDummy && !pVOAvatar->isOrphaned())
		{
			LLVector3d pos_global = pVOAvatar->getPositionGlobal();
			LLUUID uuid = pVOAvatar->getID();

			if (!uuid.isNull()
				&& dist_vec_squared(pos_global, relative_to) <= radius_squared)
			{
				if(positions != NULL)
				{
					positions->push_back(pos_global);
				}
				if(avatar_ids !=NULL)
				{
					avatar_ids->push_back(uuid);
				}
			}
		}
	}
	// region avatars added for situations where radius is greater than RenderFarClip
	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
		iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		const LLVector3d& origin_global = regionp->getOriginGlobal();
		S32 count = regionp->mMapAvatars.size();
		for (S32 i = 0; i < count; i++)
		{
			LLVector3d pos_global = unpackLocalToGlobalPosition(regionp->mMapAvatars.at(i), origin_global);
			if(dist_vec_squared(pos_global, relative_to) <= radius_squared)
			{
				LLUUID uuid = regionp->mMapAvatarIDs.at(i);
				// if this avatar doesn't already exist in the list, add it
				if(uuid.notNull() && avatar_ids != NULL && std::find(avatar_ids->begin(), avatar_ids->end(), uuid) == avatar_ids->end())
				{
					if (positions != NULL)
					{
						positions->push_back(pos_global);
					}
					avatar_ids->push_back(uuid);
				}
			}
		}
	}
}

F32 LLWorld::getNearbyAvatarsAndMaxGPUTime(std::vector<LLCharacter*> &valid_nearby_avs)
{
    static LLCachedControl<F32> render_far_clip(gSavedSettings, "RenderFarClip", 64);
    F32 nearby_max_complexity = 0;
    F32 radius = render_far_clip * render_far_clip;
    std::vector<LLCharacter*>::iterator char_iter = LLCharacter::sInstances.begin();
    while (char_iter != LLCharacter::sInstances.end())
    {
        LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(*char_iter);
        if (avatar && !avatar->isDead() && !avatar->isControlAvatar())
        {
            if ((dist_vec_squared(avatar->getPositionGlobal(), gAgent.getPositionGlobal()) > radius) &&
                (dist_vec_squared(avatar->getPositionGlobal(), gAgentCamera.getCameraPositionGlobal()) > radius))
            {
                char_iter++;
                continue;
            }

            if (!avatar->isTooSlow())
            {
                gPipeline.profileAvatar(avatar);
            }
            nearby_max_complexity = llmax(nearby_max_complexity, avatar->getGPURenderTime());
            valid_nearby_avs.push_back(*char_iter);
        }
        char_iter++;
    }
    return nearby_max_complexity;
}

bool LLWorld::isRegionListed(const LLViewerRegion* region) const
{
	region_list_t::const_iterator it = find(mRegionList.begin(), mRegionList.end(), region);
	return it != mRegionList.end();
}

boost::signals2::connection LLWorld::setRegionRemovedCallback(const region_remove_signal_t::slot_type& cb)
{
	return mRegionRemovedSignal.connect(cb);
}

LLHTTPRegistration<LLEstablishAgentCommunication>
	gHTTPRegistrationEstablishAgentCommunication(
							"/message/EstablishAgentCommunication");

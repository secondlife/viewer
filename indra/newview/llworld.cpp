/** 
 * @file llworld.cpp
 * @brief Initial test structure to organize viewer regions
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#include "llworld.h"

#include "indra_constants.h"
#include "llstl.h"

#include "llagent.h"
#include "llviewercontrol.h"
#include "lldrawpool.h"
#include "llglheaders.h"
#include "llhttpnode.h"
#include "llregionhandle.h"
#include "llsurface.h"
#include "llviewercamera.h"
#include "llviewerimage.h"
#include "llviewerimagelist.h"
#include "llviewernetwork.h"
#include "llviewerobjectlist.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llvlcomposition.h"
#include "llvoavatar.h"
#include "llvowater.h"
#include "message.h"
#include "pipeline.h"
#include "llappviewer.h"		// for do_disconnect()

//
// Globals
//
LLWorld*	gWorldp = NULL;
U32			gAgentPauseSerialNum = 0;

//
// Constants
//
const S32 MAX_NUMBER_OF_CLOUDS	= 750;
const S32 WORLD_PATCH_SIZE = 16;

extern LLColor4U MAX_WATER_COLOR;

//
// Functions
//

// allocate the stack
LLWorld::LLWorld(const U32 grids_per_region, const F32 meters_per_grid)
:	mWidth(grids_per_region), 
	mScale(meters_per_grid), 
	mWidthInMeters( grids_per_region * meters_per_grid )
{
	mSpaceTimeUSec = 0;
	mLastPacketsIn = 0;
	mLastPacketsOut = 0;
	mLastPacketsLost = 0;
	mLandFarClip = DEFAULT_FAR_PLANE;

	if (gNoRender)
	{
		return;
	}

	for (S32 i = 0; i < 8; i++)
	{
		mEdgeWaterObjects[i] = NULL;
	}

	LLPointer<LLImageRaw> raw = new LLImageRaw(1,1,4);
	U8 *default_texture = raw->getData();
	*(default_texture++) = MAX_WATER_COLOR.mV[0];
	*(default_texture++) = MAX_WATER_COLOR.mV[1];
	*(default_texture++) = MAX_WATER_COLOR.mV[2];
	*(default_texture++) = MAX_WATER_COLOR.mV[3];
	
	mDefaultWaterTexturep = new LLViewerImage(raw, FALSE);
	mDefaultWaterTexturep->bind();
	mDefaultWaterTexturep->setClamp(TRUE, TRUE);

}


LLWorld::~LLWorld()
{
	gObjectList.killAllObjects();
	for_each(mRegionList.begin(), mRegionList.end(), DeletePointer());
}


LLViewerRegion* LLWorld::addRegion(const U64 &region_handle, const LLHost &host)
{
	LLMemType mt(LLMemType::MTYPE_REGIONS);
	
	LLViewerRegion *regionp = getRegionFromHandle(region_handle);
	if (regionp)
	{
		LLHost old_host = regionp->getHost();
		// region already exists!
		if (host == old_host && regionp->mAlive)
		{
			// This is a duplicate for the same host and it's alive, don't bother.
			return regionp;
		}

		if (host != old_host)
		{
			llwarns << "LLWorld::addRegion exists, but old host " << old_host
					<< " does not match new host " << host << llendl;
		}
		if (!regionp->mAlive)
		{
			llwarns << "LLWorld::addRegion exists, but isn't alive" << llendl;
		}

		// Kill the old host, and then we can continue on and add the new host.  We have to kill even if the host
		// matches, because all the agent state for the new camera is completely different.
		removeRegion(old_host);
	}

	U32 iindex = 0;
	U32 jindex = 0;
	from_region_handle(region_handle, &iindex, &jindex);
	S32 x = (S32)(iindex/mWidth);
	S32 y = (S32)(jindex/mWidth);
	llinfos << "Adding new region (" << x << ":" << y << ")" << llendl;
	llinfos << "Host: " << host << llendl;

	LLVector3d origin_global;

	origin_global = from_region_handle(region_handle);

	regionp = new LLViewerRegion(region_handle,
								    host,
									mWidth,
									WORLD_PATCH_SIZE,
									getRegionWidthInMeters() );
	if (!regionp)
	{
		llerrs << "Unable to create new region!" << llendl;
	}

	regionp->mCloudLayer.create(regionp);
	regionp->mCloudLayer.setWidth((F32)mWidth);
	regionp->mCloudLayer.setWindPointer(&regionp->mWind);

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
			//llinfos << "Connecting " << region_x << ":" << region_y << " -> " << adj_x << ":" << adj_y << llendl;
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
		llwarns << "Trying to remove region that doesn't exist!" << llendl;
		return;
	}
	
	if (regionp == gAgent.getRegion())
	{
		for (region_list_t::iterator iter = mRegionList.begin();
			 iter != mRegionList.end(); ++iter)
		{
			LLViewerRegion* reg = *iter;
			llwarns << "RegionDump: " << reg->getName()
				<< " " << reg->getHost()
				<< " " << reg->getOriginGlobal()
				<< llendl;
		}

		llwarns << "Agent position global " << gAgent.getPositionGlobal() 
			<< " agent " << gAgent.getPositionAgent()
			<< llendl;

		llwarns << "Regions visited " << gAgent.getRegionsVisited() << llendl;

		llwarns << "gFrameTimeSeconds " << gFrameTimeSeconds << llendl;

		llwarns << "Disabling region " << regionp->getName() << " that agent is in!" << llendl;
		LLAppViewer::instance()->forceDisconnect("You have been disconnected from the region you were in.");
		return;
	}

	from_region_handle(regionp->getHandle(), &x, &y);
	llinfos << "Removing region " << x << ":" << y << llendl;

	mRegionList.remove(regionp);
	mActiveRegionList.remove(regionp);
	mCulledRegionList.remove(regionp);
	mVisibleRegionList.remove(regionp);
	
	delete regionp;

	updateWaterObjects();
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

	// clamp to < 256 to stay in sim
	LLVector3d final_region_pos = LLVector3d(region_coord) - (delta_pos * clip_factor);
	final_region_pos.clamp(0.0, 255.999);
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


BOOL LLWorld::positionRegionValidGlobal(const LLVector3d &pos_global)
{
	for (region_list_t::iterator iter = mRegionList.begin();
		 iter != mRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		if (regionp->pointInRegionGlobal(pos_global))
		{
			return TRUE;
		}
	}
	return FALSE;
}


// Allow objects to go up to their radius underground.
F32 LLWorld::getMinAllowedZ(LLViewerObject* object)
{
	F32 land_height = resolveLandHeightGlobal(object->getPositionGlobal());
	F32 radius = 0.5f * object->getScale().magVec();
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
	F32 segment_length = (F32)((point_a - point_b).magVec());
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
	F32 cur_far_clip = gCamera->getFar();

	gCamera->setFar(mLandFarClip);

	F32 diagonal_squared = F_SQRT2 * F_SQRT2 * mWidth * mWidth;
	// Go through the culled list and check for visible regions
	for (region_list_t::iterator iter = mCulledRegionList.begin();
		 iter != mCulledRegionList.end(); )
	{
		region_list_t::iterator curiter = iter++;
		LLViewerRegion* regionp = *curiter;
		F32 height = regionp->getLand().getMaxZ() - regionp->getLand().getMinZ();
		F32 radius = 0.5f*fsqrtf(height * height + diagonal_squared);
		if (!regionp->getLand().hasZData()
			|| gCamera->sphereInFrustum(regionp->getCenterAgent(), radius))
		{
			mCulledRegionList.erase(curiter);
			mVisibleRegionList.push_back(regionp);
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

		F32 height = regionp->getLand().getMaxZ() - regionp->getLand().getMinZ();
		F32 radius = 0.5f*fsqrtf(height * height + diagonal_squared);
		if (gCamera->sphereInFrustum(regionp->getCenterAgent(), radius))
		{
			regionp->calculateCameraDistance();
			if (!gNoRender)
			{
				regionp->getLand().updatePatchVisibilities(gAgent);
			}
		}
		else
		{
			mVisibleRegionList.erase(curiter);
			mCulledRegionList.push_back(regionp);
		}
	}

	// Sort visible regions
	mVisibleRegionList.sort(LLViewerRegion::CompareDistance());
	
	gCamera->setFar(cur_far_clip);
}

void LLWorld::updateRegions(F32 max_update_time)
{
	LLTimer update_timer;
	BOOL did_one = FALSE;
	
	// Perform idle time updates for the regions (and associated surfaces)
	for (region_list_t::iterator iter = mRegionList.begin();
		 iter != mRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		F32 max_time = max_update_time - update_timer.getElapsedTimeF32();
		if (did_one && max_time <= 0.f)
			break;
		max_time = llmin(max_time, max_update_time*.1f);
		did_one |= regionp->idleUpdate(max_update_time);
	}
}

void LLWorld::updateParticles()
{
	mPartSim.updateSimulation();
}

void LLWorld::updateClouds(const F32 dt)
{
	if (gSavedSettings.getBOOL("FreezeTime"))
	{
		// don't move clouds in snapshot mode
		return;
	}
	if (mActiveRegionList.size())
	{
		// Update all the cloud puff positions, and timer based stuff
		// such as death decay
		for (region_list_t::iterator iter = mActiveRegionList.begin();
			 iter != mActiveRegionList.end(); ++iter)
		{
			LLViewerRegion* regionp = *iter;
			regionp->mCloudLayer.updatePuffs(dt);
		}

		// Reshuffle who owns which puffs
		for (region_list_t::iterator iter = mActiveRegionList.begin();
			 iter != mActiveRegionList.end(); ++iter)
		{
			LLViewerRegion* regionp = *iter;
			regionp->mCloudLayer.updatePuffOwnership();
		}

		// Add new puffs
		for (region_list_t::iterator iter = mActiveRegionList.begin();
			 iter != mActiveRegionList.end(); ++iter)
		{
			LLViewerRegion* regionp = *iter;
			regionp->mCloudLayer.updatePuffCount();
		}
	}
}

LLCloudGroup* LLWorld::findCloudGroup(const LLCloudPuff &puff)
{
	if (mActiveRegionList.size())
	{
		// Update all the cloud puff positions, and timer based stuff
		// such as death decay
		for (region_list_t::iterator iter = mActiveRegionList.begin();
			 iter != mActiveRegionList.end(); ++iter)
		{
			LLViewerRegion* regionp = *iter;
			LLCloudGroup *groupp = regionp->mCloudLayer.findCloudGroup(puff);
			if (groupp)
			{
				return groupp;
			}
		}
	}
	return NULL;
}


void LLWorld::renderPropertyLines()
{
	S32 region_count = 0;
	S32 vertex_count = 0;

	for (region_list_t::iterator iter = mVisibleRegionList.begin();
		 iter != mVisibleRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		region_count++;
		vertex_count += regionp->renderPropertyLines();
	}
}


void LLWorld::updateNetStats()
{
	F32 bits = 0.f;
	U32 packets = 0;

	for (region_list_t::iterator iter = mActiveRegionList.begin();
		 iter != mActiveRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		regionp->updateNetStats();
		bits += regionp->mBitStat.getCurrent();
		packets += llfloor( regionp->mPacketsStat.getCurrent() );
	}

	S32 packets_in = gMessageSystem->mPacketsIn - mLastPacketsIn;
	S32 packets_out = gMessageSystem->mPacketsOut - mLastPacketsOut;
	S32 packets_lost = gMessageSystem->mDroppedPackets - mLastPacketsLost;

	S32 actual_in_bits = gMessageSystem->mPacketRing.getAndResetActualInBits();
	S32 actual_out_bits = gMessageSystem->mPacketRing.getAndResetActualOutBits();
	gViewerStats->mActualInKBitStat.addValue(actual_in_bits/1024.f);
	gViewerStats->mActualOutKBitStat.addValue(actual_out_bits/1024.f);
	gViewerStats->mKBitStat.addValue(bits/1024.f);
	gViewerStats->mPacketsInStat.addValue(packets_in);
	gViewerStats->mPacketsOutStat.addValue(packets_out);
	gViewerStats->mPacketsLostStat.addValue(gMessageSystem->mDroppedPackets);
	if (packets_in)
	{
		gViewerStats->mPacketsLostPercentStat.addValue(100.f*((F32)packets_lost/(F32)packets_in));
	}
	else
	{
		gViewerStats->mPacketsLostPercentStat.addValue(0.f);
	}

	mLastPacketsIn = gMessageSystem->mPacketsIn;
	mLastPacketsOut = gMessageSystem->mPacketsOut;
	mLastPacketsLost = gMessageSystem->mDroppedPackets;
}


void LLWorld::printPacketsLost()
{
	llinfos << "Simulators:" << llendl;
	llinfos << "----------" << llendl;

	LLCircuitData *cdp = NULL;
	for (region_list_t::iterator iter = mActiveRegionList.begin();
		 iter != mActiveRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		cdp = gMessageSystem->mCircuitInfo.findCircuit(regionp->getHost());
		if (cdp)
		{
			LLVector3d range = regionp->getCenterGlobal() - gAgent.getPositionGlobal();
				
			llinfos << regionp->getHost() << ", range: " << range.magVec()
					<< " packets lost: " << cdp->getPacketsLost() << llendl;
		}
	}
}

void LLWorld::processCoarseUpdate(LLMessageSystem* msg, void** user_data)
{
	LLViewerRegion* region = gWorldp->getRegion(msg->getSender());
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
	mLandFarClip = far_clip;
}


void LLWorld::updateWaterObjects()
{
	//llinfos << "Start water update" << llendl;
	if (!gAgent.getRegion())
	{
		return;
	}
	if (mRegionList.empty())
	{
		llwarns << "No regions!" << llendl;
		return;
	}

	// First, determine the min and max "box" of water objects
	bool first = true;
	S32 min_x = 0;
	S32 min_y = 0;
	S32 max_x = 0;
	S32 max_y = 0;
	U32 region_x, region_y;

	S32 rwidth = llfloor(getRegionWidthInMeters());

	
	for (region_list_t::iterator iter = mRegionList.begin();
		 iter != mRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		from_region_handle(regionp->getHandle(), &region_x, &region_y);
		if (first)
		{
			first = false;
			min_x = max_x = region_x;
			min_y = max_y = region_y;
		}
		else
		{
			min_x = llmin(min_x, (S32)region_x);
			min_y = llmin(min_y, (S32)region_y);
			max_x = llmax(max_x, (S32)region_x);
			max_y = llmax(max_y, (S32)region_y);
		}
		LLVOWater* waterp = regionp->getLand().getWaterObj();
		if (waterp)
		{
			gObjectList.updateActive(waterp);
		}
	}

	for (std::list<LLVOWater*>::iterator iter = mHoleWaterObjects.begin();
		 iter != mHoleWaterObjects.end(); ++ iter)
	{
		LLVOWater* waterp = *iter;
		gObjectList.killObject(waterp);
	}
	mHoleWaterObjects.clear();

	// We only want to fill in holes for stuff that's near us, say, within 512m
	LLViewerRegion* regionp = gAgent.getRegion();
	from_region_handle(regionp->getHandle(), &region_x, &region_y);

	min_x = llmax((S32)region_x - 512, min_x);
	min_y = llmax((S32)region_y - 512, min_y);
	max_x = llmin((S32)region_x + 512, max_x);
	max_y = llmin((S32)region_y + 512, max_y);
	
	// Now, get a list of the holes
	S32 x, y;
	for (x = min_x; x <= max_x; x += rwidth)
	{
		for (y = min_y; y <= max_y; y += rwidth)
		{
			U64 region_handle = to_region_handle(x, y);
			if (!getRegionFromHandle(region_handle))
			{
				LLVOWater* waterp = (LLVOWater *)gObjectList.createObjectViewer(LLViewerObject::LL_VO_WATER, gAgent.getRegion());
				waterp->setUseTexture(FALSE);
				gPipeline.addObject(waterp);
				waterp->setPositionGlobal(LLVector3d(x + rwidth/2,
													 y + rwidth/2,
													 DEFAULT_WATER_HEIGHT));
				waterp->setScale(LLVector3((F32)rwidth, (F32)rwidth, 0.f));
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
		512 - (max_x - region_x),
		512 - (max_y - region_y),
		512 - (region_x - min_x),
		512 - (region_y - min_y) };
		
		
	S32 dir;
	for (dir = 0; dir < 8; dir++)
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

		if (dim[0] == 0 || dim[1] == 0)
		{
			continue;
		}

		// Resize and reshape the water objects
		const S32 water_center_x = center_x + llround((wx + dim[0]) * 0.5f * gDirAxes[dir][0]);
		const S32 water_center_y = center_y + llround((wy + dim[1]) * 0.5f * gDirAxes[dir][1]);
		
		
		LLVOWater* waterp = mEdgeWaterObjects[dir];
		if (!waterp || waterp->isDead())
		{
			// The edge water objects can be dead because they're attached to the region that the
			// agent was in when they were originally created.
			mEdgeWaterObjects[dir] = (LLVOWater *)gObjectList.createObjectViewer(LLViewerObject::LL_VO_WATER,
																				 gAgent.getRegion());
			waterp = mEdgeWaterObjects[dir];
			waterp->setUseTexture(FALSE);
			gPipeline.addObject(waterp);
		}

		waterp->setRegion(gAgent.getRegion());
		LLVector3d water_pos(water_center_x, water_center_y, 
			DEFAULT_WATER_HEIGHT);
		waterp->setPositionGlobal(water_pos);
		waterp->setScale(LLVector3((F32)dim[0], (F32)dim[1], 0.f));
		gObjectList.updateActive(waterp);
		/*if (!gNoRender)
		{
			gPipeline.markMoved(waterp->mDrawable);
		}*/
	}


	//llinfos << "End water update" << llendl;
}

LLViewerImage* LLWorld::getDefaultWaterTexture()
{
	return mDefaultWaterTexturep;
}

void LLWorld::setSpaceTimeUSec(const U64 space_time_usec)
{
	mSpaceTimeUSec = space_time_usec;
}

U64 LLWorld::getSpaceTimeUSec() const
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

LLString LLWorld::getInfoString()
{
	LLString info_string("World Info:\n");
	for (region_list_t::iterator iter = mRegionList.begin();
		 iter != mRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		info_string += regionp->getInfoString();
	}
	return info_string;
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

		llinfos << "Sending AgentQuitCopy to: " << regionp->getHost() << llendl;
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
	msg->enableCircuit(sim, TRUE);
	gWorldp->addRegion(handle, sim);

	// give the simulator a message it can use to get ip and port
	llinfos << "simulator_enable() Enabling " << sim << " with code " << msg->getOurCircuitCode() << llendl;
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
		if (!input["body"].has("agent-id") ||
			!input["body"].has("sim-ip-and-port") ||
			!input["body"].has("seed-capability"))
		{
			llwarns << "invalid parameters" << llendl;
            return;
		}

		LLHost sim(input["body"]["sim-ip-and-port"].asString());
	
		LLViewerRegion* regionp = gWorldp->getRegion(sim);
		if (!regionp)
		{
			llwarns << "Got EstablishAgentCommunication for unknown region "
					<< sim << llendl;
			return;
		}
		regionp->setSeedCapability(input["body"]["seed-capability"]);
	}
};

// disable the circuit to this simulator
// Called in response to "DisableSimulator" message.
void process_disable_simulator(LLMessageSystem *mesgsys, void **user_data)
{	
	LLHost host = mesgsys->getSender();

	//llinfos << "Disabling simulator with message from " << host << llendl;
	gWorldp->removeRegion(host);

	mesgsys->disableCircuit(host);
}


void process_region_handshake(LLMessageSystem* msg, void** user_data)
{
	LLHost host = msg->getSender();
	LLViewerRegion* regionp = gWorldp->getRegion(host);
	if (!regionp)
	{
		llwarns << "Got region handshake for unknown region "
			<< host << llendl;
		return;
	}

	regionp->unpackRegionHandshake();
}


void send_agent_pause()
{
	// world not initialized yet
	if (!gWorldp)
	{
		return;
	}
	
	gMessageSystem->newMessageFast(_PREHASH_AgentPause);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgentID);
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgentSessionID);

	gAgentPauseSerialNum++;
	gMessageSystem->addU32Fast(_PREHASH_SerialNum, gAgentPauseSerialNum);

	for (LLWorld::region_list_t::iterator iter = gWorldp->mActiveRegionList.begin();
		 iter != gWorldp->mActiveRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		gMessageSystem->sendReliable(regionp->getHost());
	}

	gObjectList.mWasPaused = TRUE;
}


void send_agent_resume()
{
	// world not initialized yet
	if (!gWorldp) return;

	gMessageSystem->newMessageFast(_PREHASH_AgentResume);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgentID);
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgentSessionID);

	gAgentPauseSerialNum++;
	gMessageSystem->addU32Fast(_PREHASH_SerialNum, gAgentPauseSerialNum);
	

	for (LLWorld::region_list_t::iterator iter = gWorldp->mActiveRegionList.begin();
		 iter != gWorldp->mActiveRegionList.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		gMessageSystem->sendReliable(regionp->getHost());
	}

	// Reset the FPS counter to avoid an invalid fps
	gViewerStats->mFPSStat.start();
}


LLHTTPRegistration<LLEstablishAgentCommunication>
	gHTTPRegistrationEstablishAgentCommunication(
							"/message/EstablishAgentCommunication");

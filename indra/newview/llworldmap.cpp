/** 
 * @file llworldmap.cpp
 * @brief Underlying data representation for map of the world
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llworldmap.h"

#include "llregionhandle.h"
#include "message.h"

#include "viewer.h"	// for gPacificDaylightTime
#include "llagent.h"
#include "llmapresponders.h"
#include "llviewercontrol.h"
#include "llfloaterworldmap.h"
#include "lltracker.h"
#include "llviewerimagelist.h"
#include "llviewerregion.h"
#include "llregionflags.h"

LLWorldMap* gWorldMap = NULL;

const F32 REQUEST_ITEMS_TIMER =  10.f * 60.f; // 10 minutes

LLItemInfo::LLItemInfo(F32 global_x, F32 global_y,
					   const std::string& name, 
					   LLUUID id,
					   S32 extra, S32 extra2)
:	mName(name),
	mToolTip(""),
	mPosGlobal(global_x, global_y, 40.0),
	mID(id),
	mSelected(FALSE),
	mExtra(extra),
	mExtra2(extra2)
{
	mRegionHandle = to_region_handle(mPosGlobal);
}

LLSimInfo::LLSimInfo()
:	mHandle(0),
	mName(),
	mAgentsUpdateTime(0),
	mAccess(0x0),
	mRegionFlags(0x0),
	mWaterHeight(0.f),
	mAlpha(-1.f)
{
}


LLVector3d LLSimInfo::getGlobalPos(LLVector3 local_pos) const
{
	LLVector3d pos = from_region_handle(mHandle);
	pos.mdV[VX] += local_pos.mV[VX];
	pos.mdV[VY] += local_pos.mV[VY];
	pos.mdV[VZ] += local_pos.mV[VZ];
	return pos;
}


//---------------------------------------------------------------------------
// World Map
//---------------------------------------------------------------------------

LLWorldMap::LLWorldMap() :
	mIsTrackingUnknownLocation( FALSE ),
	mInvalidLocation( FALSE ),
	mIsTrackingDoubleClick( FALSE ),
	mIsTrackingCommit( FALSE ),
	mUnknownLocation( 0, 0, 0 ),
	mRequestLandForSale(true),
	mCurrentMap(0),
	mMinX(U32_MAX),
	mMaxX(U32_MIN),
	mMinY(U32_MAX),
	mMaxY(U32_MIN),
	mNeighborMap(NULL),
	mTelehubCoverageMap(NULL),
	mNeighborMapWidth(0),
	mNeighborMapHeight(0)
{
	for (S32 map=0; map<MAP_SIM_IMAGE_TYPES; ++map)
	{
		mMapLoaded[map] = FALSE;
		mMapBlockLoaded[map] = new BOOL[MAP_BLOCK_RES*MAP_BLOCK_RES];
		for (S32 idx=0; idx<MAP_BLOCK_RES*MAP_BLOCK_RES; ++idx)
		{
			mMapBlockLoaded[map][idx] = FALSE;
		}
	}
}


LLWorldMap::~LLWorldMap()
{
	reset();
	for (S32 map=0; map<MAP_SIM_IMAGE_TYPES; ++map)
	{
		delete[] mMapBlockLoaded[map];
	}
}


void LLWorldMap::reset()
{
	for_each(mSimInfoMap.begin(), mSimInfoMap.end(), DeletePairedPointer());
	mSimInfoMap.clear();

	for (S32 m=0; m<MAP_SIM_IMAGE_TYPES; ++m)
	{
		mMapLoaded[m] = FALSE;
	}

	clearSimFlags();
	
	eraseItems();

	mMinX = U32_MAX;
	mMaxX = U32_MIN;

	mMinY = U32_MAX;
	mMaxY = U32_MIN;

	delete [] mNeighborMap;
	mNeighborMap = NULL;
	delete [] mTelehubCoverageMap;
	mTelehubCoverageMap = NULL;

	mNeighborMapWidth = 0;
	mNeighborMapHeight = 0;
}

void LLWorldMap::eraseItems()
{
	if (mRequestTimer.getElapsedTimeF32() > REQUEST_ITEMS_TIMER)
	{
		mRequestTimer.reset();

		mTelehubs.clear();
		mInfohubs.clear();
		mPGEvents.clear();
		mMatureEvents.clear();
		mPopular.clear();
		mLandForSale.clear();
		mClassifieds.clear();
	}
// 	mAgentLocationsMap.clear(); // persists
// 	mNumAgents.clear(); // persists
}


void LLWorldMap::clearImageRefs()
{
	for (sim_info_map_t::iterator it = mSimInfoMap.begin(); it != mSimInfoMap.end(); ++it)
	{
		LLSimInfo* info = (*it).second;
		if (info->mCurrentImage)
		{
			info->mCurrentImage->setBoostLevel(0);
			info->mCurrentImage = NULL;
		}
		if (info->mOverlayImage)
		{
			info->mOverlayImage->setBoostLevel(0);
			info->mOverlayImage = NULL;
		}
	}
}

// Doesn't clear the already-loaded sim infos, just re-requests them
void LLWorldMap::clearSimFlags()
{
	for (S32 map=0; map<MAP_SIM_IMAGE_TYPES; ++map)
	{
		for (S32 idx=0; idx<MAP_BLOCK_RES*MAP_BLOCK_RES; ++idx)
		{
			mMapBlockLoaded[map][idx] = FALSE;
		}
	}
}

LLSimInfo* LLWorldMap::simInfoFromPosGlobal(const LLVector3d& pos_global)
{
	U64 handle = to_region_handle(pos_global);
	return simInfoFromHandle(handle);
}

LLSimInfo* LLWorldMap::simInfoFromHandle(const U64 handle)
{
	sim_info_map_t::iterator it = mSimInfoMap.find(handle);
	if (it != mSimInfoMap.end())
	{
		LLSimInfo* sim_info = (*it).second;
		if (sim_info)
		{
			return sim_info;
		}
	}
	return NULL;
}


LLSimInfo* LLWorldMap::simInfoFromName(const LLString& sim_name)
{
	LLSimInfo* sim_info = NULL;
	if (!sim_name.empty())
	{
		for (sim_info_map_t::iterator it = mSimInfoMap.begin(); it != mSimInfoMap.end(); ++it)
		{
			sim_info = (*it).second;
			if (sim_info
				&& (0 == LLString::compareInsensitive(sim_name.c_str(), sim_info->mName.c_str())) )
			{
				break;
			}
			sim_info = NULL;
		}
	}
	return sim_info;
}

bool LLWorldMap::simNameFromPosGlobal(const LLVector3d& pos_global, LLString & outSimName )
{
	bool gotSimName = true;

	U64 handle = to_region_handle(pos_global);

	sim_info_map_t::iterator it = mSimInfoMap.find(handle);
	if (it != mSimInfoMap.end())
	{
		LLSimInfo* info = (*it).second;
		outSimName = info->mName.c_str();
	}
	else
	{
		gotSimName = false;
		outSimName = "(unknown region)";
	}

	return gotSimName;
}

void LLWorldMap::setCurrentLayer(S32 layer, bool request_layer)
{
	mCurrentMap = layer;
	if (!mMapLoaded[layer] || request_layer)
	{
		sendMapLayerRequest();
	}

	if (mTelehubs.size() == 0 ||
		mInfohubs.size() == 0)
	{
		// Request for telehubs
		sendItemRequest(MAP_ITEM_TELEHUB);
	}

	if (mPGEvents.size() == 0)
	{
		// Request for events
		sendItemRequest(MAP_ITEM_PG_EVENT);
	}

	if (mMatureEvents.size() == 0)
	{
		// Request for events (mature)
		sendItemRequest(MAP_ITEM_MATURE_EVENT);
	}

	if (mPopular.size() == 0)
	{
		// Request for popular
		sendItemRequest(MAP_ITEM_POPULAR);
	}

	if (mLandForSale.size() == 0)
	{
		// Request for Land For Sale
		sendItemRequest(MAP_ITEM_LAND_FOR_SALE);
	}

	if (mClassifieds.size() == 0)
	{
		sendItemRequest(MAP_ITEM_CLASSIFIED);
	}

	clearImageRefs();
	clearSimFlags();
}

void LLWorldMap::sendItemRequest(U32 type, U64 handle)
{
	LLMessageSystem* msg = gMessageSystem;
	S32 layer = mCurrentMap;

	msg->newMessageFast(_PREHASH_MapItemRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addU32Fast(_PREHASH_Flags, layer);
	msg->addU32Fast(_PREHASH_EstateID, 0); // Filled in on sim
	msg->addBOOLFast(_PREHASH_Godlike, FALSE); // Filled in on sim

	msg->nextBlockFast(_PREHASH_RequestData);
	msg->addU32Fast(_PREHASH_ItemType, type);
	msg->addU64Fast(_PREHASH_RegionHandle, handle); // If zero, filled in on sim

	gAgent.sendReliableMessage();
}

// public
void LLWorldMap::sendMapLayerRequest()
{
	LLSD body;
	body["Flags"] = mCurrentMap;
	std::string url = gAgent.getRegion()->getCapability(
		gAgent.isGodlike() ? "MapLayerGod" : "MapLayer");

	if (!url.empty())
	{
		llinfos << "LLWorldMap::sendMapLayerRequest via capability" << llendl;
		LLHTTPClient::post(url, body, new LLMapLayerResponder());
	}
	else
	{
		llinfos << "LLWorldMap::sendMapLayerRequest via message system" << llendl;
		LLMessageSystem* msg = gMessageSystem;
		S32 layer = mCurrentMap;

		// Request for layer
		msg->newMessageFast(_PREHASH_MapLayerRequest);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->addU32Fast(_PREHASH_Flags, layer);
		msg->addU32Fast(_PREHASH_EstateID, 0); // Filled in on sim
		msg->addBOOLFast(_PREHASH_Godlike, FALSE); // Filled in on sim
		gAgent.sendReliableMessage();

		if (mRequestLandForSale)
		{
			msg->newMessageFast(_PREHASH_MapLayerRequest);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->addU32Fast(_PREHASH_Flags, 2);
			msg->addU32Fast(_PREHASH_EstateID, 0); // Filled in on sim
			msg->addBOOLFast(_PREHASH_Godlike, FALSE); // Filled in on sim
			gAgent.sendReliableMessage();
		}
	}
}

// public
void LLWorldMap::sendNamedRegionRequest(std::string region_name)
{
	LLMessageSystem* msg = gMessageSystem;
	S32 layer = mCurrentMap;

	// Request for layer
	msg->newMessageFast(_PREHASH_MapNameRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addU32Fast(_PREHASH_Flags, layer);
	msg->addU32Fast(_PREHASH_EstateID, 0); // Filled in on sim
	msg->addBOOLFast(_PREHASH_Godlike, FALSE); // Filled in on sim
	msg->nextBlockFast(_PREHASH_NameData);
	msg->addStringFast(_PREHASH_Name, region_name);
	gAgent.sendReliableMessage();
}

// public
void LLWorldMap::sendMapBlockRequest(U16 min_x, U16 min_y, U16 max_x, U16 max_y, bool return_nonexistent)
{
	S32 layer = mCurrentMap;
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MapBlockRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	U32 flags = layer;
	flags |= (return_nonexistent ? 0x10000 : 0);
	msg->addU32Fast(_PREHASH_Flags, flags);
	msg->addU32Fast(_PREHASH_EstateID, 0); // Filled in on sim
	msg->addBOOLFast(_PREHASH_Godlike, FALSE); // Filled in on sim
	msg->nextBlockFast(_PREHASH_PositionData);
	msg->addU16Fast(_PREHASH_MinX, min_x);
	msg->addU16Fast(_PREHASH_MinY, min_y);
	msg->addU16Fast(_PREHASH_MaxX, max_x);
	msg->addU16Fast(_PREHASH_MaxY, max_y);
	gAgent.sendReliableMessage();

	if (mRequestLandForSale)
	{
		msg->newMessageFast(_PREHASH_MapBlockRequest);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->addU32Fast(_PREHASH_Flags, 2);
		msg->addU32Fast(_PREHASH_EstateID, 0); // Filled in on sim
		msg->addBOOLFast(_PREHASH_Godlike, FALSE); // Filled in on sim
		msg->nextBlockFast(_PREHASH_PositionData);
		msg->addU16Fast(_PREHASH_MinX, min_x);
		msg->addU16Fast(_PREHASH_MinY, min_y);
		msg->addU16Fast(_PREHASH_MaxX, max_x);
		msg->addU16Fast(_PREHASH_MaxY, max_y);
		gAgent.sendReliableMessage();
	}
}

// public static
void LLWorldMap::processMapLayerReply(LLMessageSystem* msg, void**)
{
	llinfos << "LLWorldMap::processMapLayerReply from message system" << llendl;

	U32 agent_flags;
	msg->getU32Fast(_PREHASH_AgentData, _PREHASH_Flags, agent_flags);

	if (agent_flags != (U32)gWorldMap->mCurrentMap)
	{
		llwarns << "Invalid or out of date map image type returned!" << llendl;
		return;
	}

	LLUUID image_id;
	//U32 left, right, top, bottom;

	S32 num_blocks = msg->getNumberOfBlocksFast(_PREHASH_LayerData);

	gWorldMap->mMapLayers[agent_flags].clear();

	BOOL adjust = FALSE;
	for (S32 block=0; block<num_blocks; ++block)
	{
		LLWorldMapLayer new_layer;
		new_layer.LayerDefined = TRUE;
		msg->getUUIDFast(_PREHASH_LayerData, _PREHASH_ImageID, new_layer.LayerImageID, block);
		new_layer.LayerImage = gImageList.getImage(new_layer.LayerImageID, MIPMAP_TRUE, FALSE);
		new_layer.LayerImage->bindTexture(0);
		new_layer.LayerImage->setClamp(TRUE, TRUE);
		
		U32 left, right, top, bottom;
		msg->getU32Fast(_PREHASH_LayerData, _PREHASH_Left, left, block);
		msg->getU32Fast(_PREHASH_LayerData, _PREHASH_Right, right, block);
		msg->getU32Fast(_PREHASH_LayerData, _PREHASH_Top, top, block);
		msg->getU32Fast(_PREHASH_LayerData, _PREHASH_Bottom, bottom, block);

		new_layer.LayerExtents.mLeft = left;
		new_layer.LayerExtents.mRight = right;
		new_layer.LayerExtents.mBottom = bottom;
		new_layer.LayerExtents.mTop = top;

		F32 x_meters = F32(left*REGION_WIDTH_UNITS);
		F32 y_meters = F32(bottom*REGION_WIDTH_UNITS);
		adjust = gWorldMap->extendAABB(U32(x_meters), U32(y_meters), 
							   U32(x_meters+REGION_WIDTH_UNITS*new_layer.LayerExtents.getWidth()),
							   U32(y_meters+REGION_WIDTH_UNITS*new_layer.LayerExtents.getHeight())) || adjust;

		gWorldMap->mMapLayers[agent_flags].push_back(new_layer);
	}

	gWorldMap->mMapLoaded[agent_flags] = TRUE;
	if(adjust) gFloaterWorldMap->adjustZoomSliderBounds();
}

// public static
void LLWorldMap::processMapBlockReply(LLMessageSystem* msg, void**)
{
	U32 agent_flags;
	msg->getU32Fast(_PREHASH_AgentData, _PREHASH_Flags, agent_flags);

	if ((S32)agent_flags < 0 || agent_flags >= MAP_SIM_IMAGE_TYPES)
	{
		llwarns << "Invalid map image type returned! " << agent_flags << llendl;
		return;
	}

	S32 num_blocks = msg->getNumberOfBlocksFast(_PREHASH_Data);

	bool found_null_sim = false;

	BOOL adjust = FALSE;
	for (S32 block=0; block<num_blocks; ++block)
	{
		U16 x_regions;
		U16 y_regions;
		char name[MAX_STRING];		/* Flawfinder: ignore */
		U8 access;		/* Flawfinder: ignore */
		U32 region_flags;
		U8 water_height;
		U8 agents;
		LLUUID image_id;
		msg->getU16Fast(_PREHASH_Data, _PREHASH_X, x_regions, block);
		msg->getU16Fast(_PREHASH_Data, _PREHASH_Y, y_regions, block);
		msg->getStringFast(_PREHASH_Data, _PREHASH_Name, MAX_STRING, name, block);
		msg->getU8Fast(_PREHASH_Data, _PREHASH_Access, access, block);		/* Flawfinder: ignore */
		msg->getU32Fast(_PREHASH_Data, _PREHASH_RegionFlags, region_flags, block);
		msg->getU8Fast(_PREHASH_Data, _PREHASH_WaterHeight, water_height, block);
		msg->getU8Fast(_PREHASH_Data, _PREHASH_Agents, agents, block);
		msg->getUUIDFast(_PREHASH_Data, _PREHASH_MapImageID, image_id, block);

		U32 x_meters = x_regions * REGION_WIDTH_UNITS;
		U32 y_meters = y_regions * REGION_WIDTH_UNITS;

		if (access == 255)
		{
			// This region doesn't exist
			if (gWorldMap->mIsTrackingUnknownLocation &&
				gWorldMap->mUnknownLocation.mdV[0] >= x_meters &&
				gWorldMap->mUnknownLocation.mdV[0] < x_meters + 256 &&
				gWorldMap->mUnknownLocation.mdV[1] >= y_meters &&
				gWorldMap->mUnknownLocation.mdV[1] < y_meters + 256)
			{
				// We were tracking this location, but it doesn't exist
				gWorldMap->mInvalidLocation = TRUE;
			}

			found_null_sim = true;
		}
		else
		{
			adjust = gWorldMap->extendAABB(x_meters, 
										y_meters, 
										x_meters+REGION_WIDTH_UNITS,
										y_meters+REGION_WIDTH_UNITS) || adjust;
			U64 handle = to_region_handle(x_meters, y_meters);

// 			llinfos << "Map sim " << name << " image layer " << agent_flags << " ID " << image_id.getString() << llendl;
			
			LLSimInfo* siminfo = new LLSimInfo();
			sim_info_map_t::iterator iter = gWorldMap->mSimInfoMap.find(handle);
			if (iter != gWorldMap->mSimInfoMap.end())
			{
				LLSimInfo* oldinfo = iter->second;
				for (S32 image=0; image<MAP_SIM_IMAGE_TYPES; ++image)
				{
					siminfo->mMapImageID[image] = oldinfo->mMapImageID[image];
				}
				delete oldinfo;
			}
			gWorldMap->mSimInfoMap[handle] = siminfo;

			siminfo->mHandle = handle;
			siminfo->mName.assign( name );
			siminfo->mAccess = access;		/* Flawfinder: ignore */
			siminfo->mRegionFlags = region_flags;
			siminfo->mWaterHeight = (F32) water_height;
			siminfo->mMapImageID[agent_flags] = image_id;
			siminfo->mCurrentImage = gImageList.getImage(siminfo->mMapImageID[gWorldMap->mCurrentMap], MIPMAP_TRUE, FALSE);
			siminfo->mCurrentImage->bindTexture(0);
			siminfo->mCurrentImage->setClamp(TRUE, TRUE);
			
			if (siminfo->mMapImageID[2].notNull())
			{
				siminfo->mOverlayImage = gImageList.getImage(siminfo->mMapImageID[2], MIPMAP_TRUE, FALSE);
			}
			else
			{
				siminfo->mOverlayImage = NULL;
			}

			if (gWorldMap->mIsTrackingUnknownLocation &&
				gWorldMap->mUnknownLocation.mdV[0] >= x_meters &&
				gWorldMap->mUnknownLocation.mdV[0] < x_meters + 256 &&
				gWorldMap->mUnknownLocation.mdV[1] >= y_meters &&
				gWorldMap->mUnknownLocation.mdV[1] < y_meters + 256)
			{
				if (siminfo->mAccess == SIM_ACCESS_DOWN)
				{
					// We were tracking this location, but it doesn't exist
					gWorldMap->mInvalidLocation = true;
				}
				else
				{
					// We were tracking this location, and it does exist
					bool is_tracking_dbl = gWorldMap->mIsTrackingDoubleClick == TRUE;
					gFloaterWorldMap->trackLocation(gWorldMap->mUnknownLocation);
					if (is_tracking_dbl)
					{
						LLVector3d pos_global = LLTracker::getTrackedPositionGlobal();
						gAgent.teleportViaLocation( pos_global );
					}
				}
			}
		}
	}
	if(adjust) gFloaterWorldMap->adjustZoomSliderBounds();
	gFloaterWorldMap->updateSims(found_null_sim);
}

// public static
void LLWorldMap::processMapItemReply(LLMessageSystem* msg, void**)
{
	U32 type;
	msg->getU32Fast(_PREHASH_RequestData, _PREHASH_ItemType, type);

	S32 num_blocks = msg->getNumberOfBlocks("Data");

	for (S32 block=0; block<num_blocks; ++block)
	{
		U32 X, Y;
		char name[MAX_STRING];		/* Flawfinder: ignore */
		S32 extra, extra2;
		LLUUID uuid;
		msg->getU32Fast(_PREHASH_Data, _PREHASH_X, X, block);
		msg->getU32Fast(_PREHASH_Data, _PREHASH_Y, Y, block);
		msg->getStringFast(_PREHASH_Data, _PREHASH_Name, MAX_STRING, name, block);
		msg->getUUIDFast(_PREHASH_Data, _PREHASH_ID, uuid, block);
		msg->getS32Fast(_PREHASH_Data, _PREHASH_Extra, extra, block);
		msg->getS32Fast(_PREHASH_Data, _PREHASH_Extra2, extra2, block);

		F32 world_x = (F32)X;
		X /= REGION_WIDTH_UNITS;
		F32 world_y = (F32)Y;
		Y /= REGION_WIDTH_UNITS;
		
		LLItemInfo new_item(world_x, world_y, name, uuid, extra, extra2);
		LLSimInfo* siminfo = gWorldMap->simInfoFromHandle(new_item.mRegionHandle);

		switch (type)
		{
			case MAP_ITEM_TELEHUB: // telehubs
			{
				// Telehub color, store in extra as 4 U8's
				U8 *color = (U8 *)&new_item.mExtra;

				F32 red = fmod((F32)X * 0.11f, 1.f) * 0.8f;
				F32 green = fmod((F32)Y * 0.11f, 1.f) * 0.8f;
				F32 blue = fmod(1.5f * (F32)(X + Y) * 0.11f, 1.f) * 0.8f;
				F32 add_amt = (X % 2) ? 0.15f : -0.15f;
				add_amt += (Y % 2) ? -0.15f : 0.15f;
				color[0] = U8((red + add_amt) * 255);
				color[1] = U8((green + add_amt) * 255);
				color[2] = U8((blue + add_amt) * 255);
				color[3] = 255;
				
				// extra2 specifies whether this is an infohub or a telehub.
				if (extra2)
				{
					gWorldMap->mInfohubs.push_back(new_item);
				}
				else
				{
					gWorldMap->mTelehubs.push_back(new_item);
				}

				break;
			}
			case MAP_ITEM_PG_EVENT: // events
			case MAP_ITEM_MATURE_EVENT:
			{
				char buffer[32];		/* Flawfinder: ignore */
				struct tm* timep;
				// Convert to Pacific, based on server's opinion of whether
				// it's daylight savings time there.
				timep = utc_to_pacific_time(extra, gPacificDaylightTime);

				S32 display_hour = timep->tm_hour % 12;
				if (display_hour == 0) display_hour = 12;

				snprintf(buffer, sizeof(buffer), "%d:%02d %s",		/* Flawfinder: ignore */
					display_hour,
					timep->tm_min,
					(timep->tm_hour < 12 ? "AM" : "PM") );
				new_item.mToolTip.assign( buffer );

				// HACK: store Z in extra2
				new_item.mPosGlobal.mdV[VZ] = (F64)extra2;
				if (type == MAP_ITEM_PG_EVENT)
				{
					gWorldMap->mPGEvents.push_back(new_item);
				}
				else
				{
					gWorldMap->mMatureEvents.push_back(new_item);
				}
				break;
			}
			case MAP_ITEM_POPULAR: // popular
			{
				new_item.mPosGlobal.mdV[VZ] = (F64)extra2;
				gWorldMap->mPopular.push_back(new_item);
				break;
			}
			case MAP_ITEM_LAND_FOR_SALE: // land for sale
			{
				new_item.mToolTip = llformat("%d sq. m. L$%d", new_item.mExtra, new_item.mExtra2);
				gWorldMap->mLandForSale.push_back(new_item);
				break;
			}
			case MAP_ITEM_CLASSIFIED: // classifieds
			{
				// HACK: Z-height is in Extra2 field.
				new_item.mPosGlobal.mdV[VZ] = (F64)extra2;
				gWorldMap->mClassifieds.push_back(new_item);
				break;
			}
			case MAP_ITEM_AGENT_LOCATIONS: // agent locations
			{
				if (!siminfo)
				{
					llinfos << "siminfo missing for " << new_item.mPosGlobal.mdV[0] << ", " << new_item.mPosGlobal.mdV[1] << llendl;
					break;
				}
// 				llinfos << "New Location " << new_item.mName << llendl;

				item_info_list_t& agentcounts = gWorldMap->mAgentLocationsMap[new_item.mRegionHandle];

				// Find the last item in the list with a different name and erase them
				item_info_list_t::iterator lastiter;
				for (lastiter = agentcounts.begin(); lastiter!=agentcounts.end(); ++lastiter)
				{
					const LLItemInfo& info = *lastiter;
					if (info.mName == new_item.mName)
					{
						break;
					}
				}
				if (lastiter != agentcounts.begin())
				{
					agentcounts.erase(agentcounts.begin(), lastiter);
				}
				// Now append the new location
				if (new_item.mExtra > 0)
				{
					agentcounts.push_back(new_item);
				}
				break;
			}
			default:
				break;
		};
	}
}

void LLWorldMap::dump()
{
	for (sim_info_map_t::iterator it = mSimInfoMap.begin(); it != mSimInfoMap.end(); ++it)
	{
		U64 handle = (*it).first;
		LLSimInfo* info = (*it).second;

		U32 x_pos, y_pos;
		from_region_handle(handle, &x_pos, &y_pos);

		llinfos << x_pos << "," << y_pos
			<< " " << info->mName.c_str() 
			<< " " << (S32)info->mAccess
			<< " " << std::hex << info->mRegionFlags << std::dec
			<< " " << info->mWaterHeight
			//<< " " << info->mTelehubName
			//<< " " << info->mTelehubPosition
			<< llendl;

		if (info->mCurrentImage)
		{
			llinfos << "image discard " << (S32)info->mCurrentImage->getDiscardLevel()
					<< " fullwidth " << info->mCurrentImage->getWidth(0)
					<< " fullheight " << info->mCurrentImage->getHeight(0)
					<< " maxvirt " << info->mCurrentImage->mMaxVirtualSize
					<< " maxdisc " << (S32)info->mCurrentImage->getMaxDiscardLevel()
					<< llendl;
		}
	}
}


BOOL LLWorldMap::extendAABB(U32 min_x, U32 min_y, U32 max_x, U32 max_y)
{
	BOOL rv = FALSE;
	if (min_x < mMinX)
	{
		rv = TRUE;
		mMinX = min_x;
	}
	if (min_y < mMinY)
	{
		rv = TRUE;
		mMinY = min_y;
	}
	if (max_x > mMaxX)
	{
		rv = TRUE;
		mMaxX = max_x;
	}
	if (max_y > mMaxY)
	{
		rv = TRUE;
		mMaxY = max_y;
	}
	lldebugs << "World map aabb: (" << mMinX << ", " << mMinY << "), ("
			 << mMaxX << ", " << mMaxY << ")" << llendl;
	return rv;
}


U32 LLWorldMap::getWorldWidth() const
{
	return mMaxX - mMinX;
}


U32 LLWorldMap::getWorldHeight() const
{
	return mMaxY - mMinY;
}

BOOL LLWorldMap::coveredByTelehub(LLSimInfo* infop)
{
	/*if (!mTelehubCoverageMap)
	{
		return FALSE;
	}
	U32 x_pos, y_pos;
	from_region_handle(infop->mHandle, &x_pos, &y_pos);
	x_pos /= REGION_WIDTH_UNITS;
	y_pos /= REGION_WIDTH_UNITS;

	S32 index = x_pos - (mMinX / REGION_WIDTH_UNITS - 1) + (mNeighborMapWidth * (y_pos - (mMinY / REGION_WIDTH_UNITS - 1)));
	return mTelehubCoverageMap[index] != 0;	*/
	return FALSE;
}

void LLWorldMap::updateTelehubCoverage()
{
	/*S32 neighbor_width = getWorldWidth() / REGION_WIDTH_UNITS + 2;
	S32 neighbor_height = getWorldHeight() / REGION_WIDTH_UNITS + 2;
	if (neighbor_width > mNeighborMapWidth || neighbor_height > mNeighborMapHeight)
	{
		mNeighborMapWidth = neighbor_width;
		mNeighborMapHeight = neighbor_height;
		delete mNeighborMap;
		delete mTelehubCoverageMap;

		mNeighborMap = new U8[mNeighborMapWidth * mNeighborMapHeight];
		mTelehubCoverageMap = new U8[mNeighborMapWidth * mNeighborMapHeight];
	}

	memset(mNeighborMap, 0, mNeighborMapWidth * mNeighborMapHeight * sizeof(U8));
	memset(mTelehubCoverageMap, 0, mNeighborMapWidth * mNeighborMapHeight * sizeof(U8));

	// leave 1 sim border
	S32 min_x = (mMinX / REGION_WIDTH_UNITS) - 1;
	S32 min_y = (mMinY / REGION_WIDTH_UNITS) - 1;

 	std::map<U64, LLSimInfo*>::const_iterator it;
	for (it = mSimInfoMap.begin(); it != mSimInfoMap.end(); ++it)
	{
		U64 handle = (*it).first;
		//LLSimInfo* info = (*it).second;

		U32 x_pos, y_pos;
		from_region_handle(handle, &x_pos, &y_pos);
		x_pos /= REGION_WIDTH_UNITS;
		y_pos /= REGION_WIDTH_UNITS;
		x_pos -= min_x;
		y_pos -= min_y;

		S32 index = x_pos + (mNeighborMapWidth * y_pos);
		mNeighborMap[index - 1]++;
		mNeighborMap[index + 1]++;
		mNeighborMap[index - mNeighborMapWidth]++;
		mNeighborMap[index + mNeighborMapWidth]++;
	}

	for (it = mSimInfoMap.begin(); it != mSimInfoMap.end(); ++it)
	{
		U64 handle = (*it).first;
		LLSimInfo* info = (*it).second;

		U32 x_pos, y_pos;
		from_region_handle(handle, &x_pos, &y_pos);
		x_pos /= REGION_WIDTH_UNITS;
		y_pos /= REGION_WIDTH_UNITS;
		x_pos -= min_x;
		y_pos -= min_y;

		S32 index = x_pos + (mNeighborMapWidth * y_pos);

		if (!info->mTelehubName.empty() && mNeighborMap[index])
		{
			S32 x_start = llmax(0, S32(x_pos - 5));
			S32 x_span = llmin(mNeighborMapWidth - 1, (S32)(x_pos + 5)) - x_start + 1;
			S32 y_start = llmax(0, (S32)y_pos - 5);
			S32 y_end = llmin(mNeighborMapHeight - 1, (S32)(y_pos + 5));
			for (S32 y_index = y_start; y_index <= y_end; y_index++)
			{
				memset(&mTelehubCoverageMap[x_start + y_index * mNeighborMapWidth], 0xff, sizeof(U8) * x_span);
			}
		}
	}

	for (it = mSimInfoMap.begin(); it != mSimInfoMap.end(); ++it)
	{
		U64 handle = (*it).first;
		//LLSimInfo* info = (*it).second;

		U32 x_pos, y_pos;
		from_region_handle(handle, &x_pos, &y_pos);
		x_pos /= REGION_WIDTH_UNITS;
		y_pos /= REGION_WIDTH_UNITS;

		S32 index = x_pos - min_x + (mNeighborMapWidth * (y_pos - min_y));
		mTelehubCoverageMap[index] *= mNeighborMap[index];
	}*/
}

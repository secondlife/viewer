/** 
 * @file llworldmapmessage.cpp
 * @brief Handling of the messages to the DB made by and for the world map.
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

#include "llworldmapmessage.h"
#include "message.h"

#include "llworldmap.h"
#include "llagent.h"
#include "llfloaterworldmap.h"

const U32 LAYER_FLAG = 2;

//---------------------------------------------------------------------------
// World Map Message Handling
//---------------------------------------------------------------------------

LLWorldMapMessage::LLWorldMapMessage() :
	mSLURLRegionName(),
	mSLURLRegionHandle(0),
	mSLURL(),
	mSLURLCallback(0),
	mSLURLTeleport(false)
{
}

LLWorldMapMessage::~LLWorldMapMessage()
{
}

void LLWorldMapMessage::sendItemRequest(U32 type, U64 handle)
{
	//LL_INFOS("World Map") << "Send item request : type = " << type << LL_ENDL;
	LLMessageSystem* msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_MapItemRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addU32Fast(_PREHASH_Flags, LAYER_FLAG);
	msg->addU32Fast(_PREHASH_EstateID, 0); // Filled in on sim
	msg->addBOOLFast(_PREHASH_Godlike, FALSE); // Filled in on sim

	msg->nextBlockFast(_PREHASH_RequestData);
	msg->addU32Fast(_PREHASH_ItemType, type);
	msg->addU64Fast(_PREHASH_RegionHandle, handle); // If zero, filled in on sim

	gAgent.sendReliableMessage();
}

void LLWorldMapMessage::sendNamedRegionRequest(std::string region_name)
{
	//LL_INFOS("World Map") << "LLWorldMap::sendNamedRegionRequest()" << LL_ENDL;
	LLMessageSystem* msg = gMessageSystem;

	// Request for region data
	msg->newMessageFast(_PREHASH_MapNameRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addU32Fast(_PREHASH_Flags, LAYER_FLAG);
	msg->addU32Fast(_PREHASH_EstateID, 0); // Filled in on sim
	msg->addBOOLFast(_PREHASH_Godlike, FALSE); // Filled in on sim
	msg->nextBlockFast(_PREHASH_NameData);
	msg->addStringFast(_PREHASH_Name, region_name);
	gAgent.sendReliableMessage();
}

void LLWorldMapMessage::sendNamedRegionRequest(std::string region_name, 
		url_callback_t callback,
		const std::string& callback_url,
		bool teleport)	// immediately teleport when result returned
{
	//LL_INFOS("World Map") << "LLWorldMap::sendNamedRegionRequest()" << LL_ENDL;
	mSLURLRegionName = region_name;
	mSLURLRegionHandle = 0;
	mSLURL = callback_url;
	mSLURLCallback = callback;
	mSLURLTeleport = teleport;

	sendNamedRegionRequest(region_name);
}

void LLWorldMapMessage::sendHandleRegionRequest(U64 region_handle, 
		url_callback_t callback,
		const std::string& callback_url,
		bool teleport)	// immediately teleport when result returned
{
	//LL_INFOS("World Map") << "LLWorldMap::sendHandleRegionRequest()" << LL_ENDL;
	mSLURLRegionName.clear();
	mSLURLRegionHandle = region_handle;
	mSLURL = callback_url;
	mSLURLCallback = callback;
	mSLURLTeleport = teleport;

	U32 global_x;
	U32 global_y;
	from_region_handle(region_handle, &global_x, &global_y);
	U16 grid_x = (U16)(global_x / REGION_WIDTH_UNITS);
	U16 grid_y = (U16)(global_y / REGION_WIDTH_UNITS);
	
	sendMapBlockRequest(grid_x, grid_y, grid_x, grid_y, true);
}

void LLWorldMapMessage::sendMapBlockRequest(U16 min_x, U16 min_y, U16 max_x, U16 max_y, bool return_nonexistent)
{
	//LL_INFOS("World Map") << "LLWorldMap::sendMapBlockRequest()" << ", min = (" << min_x << ", " << min_y << "), max = (" << max_x << ", " << max_y << "), nonexistent = " << return_nonexistent << LL_ENDL;
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MapBlockRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	U32 flags = LAYER_FLAG;
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
}

// public static
void LLWorldMapMessage::processMapBlockReply(LLMessageSystem* msg, void**)
{
	U32 agent_flags;
	msg->getU32Fast(_PREHASH_AgentData, _PREHASH_Flags, agent_flags);

	// There's only one flag that we ever use here
	if (agent_flags != LAYER_FLAG)
	{
		llwarns << "Invalid map image type returned! layer = " << agent_flags << llendl;
		return;
	}

	S32 num_blocks = msg->getNumberOfBlocksFast(_PREHASH_Data);
	//LL_INFOS("World Map") << "LLWorldMap::processMapBlockReply(), num_blocks = " << num_blocks << LL_ENDL;

	bool found_null_sim = false;

	for (S32 block=0; block<num_blocks; ++block)
	{
		U16 x_regions;
		U16 y_regions;
		std::string name;
		U8 accesscode;
		U32 region_flags;
//		U8 water_height;
//		U8 agents;
		LLUUID image_id;
		msg->getU16Fast(_PREHASH_Data, _PREHASH_X, x_regions, block);
		msg->getU16Fast(_PREHASH_Data, _PREHASH_Y, y_regions, block);
		msg->getStringFast(_PREHASH_Data, _PREHASH_Name, name, block);
		msg->getU8Fast(_PREHASH_Data, _PREHASH_Access, accesscode, block);
		msg->getU32Fast(_PREHASH_Data, _PREHASH_RegionFlags, region_flags, block);
//		msg->getU8Fast(_PREHASH_Data, _PREHASH_WaterHeight, water_height, block);
//		msg->getU8Fast(_PREHASH_Data, _PREHASH_Agents, agents, block);
		msg->getUUIDFast(_PREHASH_Data, _PREHASH_MapImageID, image_id, block);

		U32 x_world = (U32)(x_regions) * REGION_WIDTH_UNITS;
		U32 y_world = (U32)(y_regions) * REGION_WIDTH_UNITS;

		// name shouldn't be empty, see EXT-4568
		llassert(!name.empty());

		// Insert that region in the world map, if failure, flag it as a "null_sim"
		if (!(LLWorldMap::getInstance()->insertRegion(x_world, y_world, name, image_id, (U32)accesscode, region_flags)))
		{
			found_null_sim = true;
		}

		// If we hit a valid tracking location, do what needs to be done app level wise
		if (LLWorldMap::getInstance()->isTrackingValidLocation())
		{
			LLVector3d pos_global = LLWorldMap::getInstance()->getTrackedPositionGlobal();
			if (LLWorldMap::getInstance()->isTrackingDoubleClick())
			{
				// Teleport if the user double clicked
				gAgent.teleportViaLocation(pos_global);
			}
			// Update the "real" tracker information
			gFloaterWorldMap->trackLocation(pos_global);
		}

		// Handle the SLURL callback if any
		url_callback_t callback = LLWorldMapMessage::getInstance()->mSLURLCallback;
		if(callback != NULL)
		{
			U64 handle = to_region_handle(x_world, y_world);
			// Check if we reached the requested region
			if ((LLStringUtil::compareInsensitive(LLWorldMapMessage::getInstance()->mSLURLRegionName, name)==0)
				|| (LLWorldMapMessage::getInstance()->mSLURLRegionHandle == handle))
			{
				LLWorldMapMessage::getInstance()->mSLURLCallback = NULL;
				LLWorldMapMessage::getInstance()->mSLURLRegionName.clear();
				LLWorldMapMessage::getInstance()->mSLURLRegionHandle = 0;

				callback(handle, LLWorldMapMessage::getInstance()->mSLURL, image_id, LLWorldMapMessage::getInstance()->mSLURLTeleport);
			}
		}
	}
	// Tell the UI to update itself
	gFloaterWorldMap->updateSims(found_null_sim);
}

// public static
void LLWorldMapMessage::processMapItemReply(LLMessageSystem* msg, void**)
{
	//LL_INFOS("World Map") << "LLWorldMap::processMapItemReply()" << LL_ENDL;
	U32 type;
	msg->getU32Fast(_PREHASH_RequestData, _PREHASH_ItemType, type);

	S32 num_blocks = msg->getNumberOfBlocks("Data");

	for (S32 block=0; block<num_blocks; ++block)
	{
		U32 X, Y;
		std::string name;
		S32 extra, extra2;
		LLUUID uuid;
		msg->getU32Fast(_PREHASH_Data, _PREHASH_X, X, block);
		msg->getU32Fast(_PREHASH_Data, _PREHASH_Y, Y, block);
		msg->getStringFast(_PREHASH_Data, _PREHASH_Name, name, block);
		msg->getUUIDFast(_PREHASH_Data, _PREHASH_ID, uuid, block);
		msg->getS32Fast(_PREHASH_Data, _PREHASH_Extra, extra, block);
		msg->getS32Fast(_PREHASH_Data, _PREHASH_Extra2, extra2, block);

		LLWorldMap::getInstance()->insertItem(X, Y, name, uuid, type, extra, extra2);
	}
}


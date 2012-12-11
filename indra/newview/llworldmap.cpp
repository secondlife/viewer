/** 
 * @file llworldmap.cpp
 * @brief Underlying data representation for map of the world
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

#include "llworldmap.h"

#include "llworldmapmessage.h"
#include "message.h"
#include "lltracker.h"
#include "lluistring.h"
#include "llviewertexturelist.h"
#include "lltrans.h"

// Timers to temporise database requests
const F32 AGENTS_UPDATE_TIMER = 60.0;			// Seconds between 2 agent requests for a region
const F32 REQUEST_ITEMS_TIMER = 10.f * 60.f;	// Seconds before we consider re-requesting item data for the grid

//---------------------------------------------------------------------------
// LLItemInfo
//---------------------------------------------------------------------------

LLItemInfo::LLItemInfo(F32 global_x, F32 global_y,
					   const std::string& name, 
					   LLUUID id)
:	mName(name),
	mToolTip(""),
	mPosGlobal(global_x, global_y, 40.0),
	mID(id),
	mCount(1)
//	mSelected(false)
//	mColor()
{
}

//---------------------------------------------------------------------------
// LLSimInfo
//---------------------------------------------------------------------------

LLSimInfo::LLSimInfo(U64 handle)
:	mHandle(handle),
	mName(),
	mAgentsUpdateTime(0),
	mAccess(0x0),
	mRegionFlags(0x0),
	mFirstAgentRequest(true)
//	mWaterHeight(0.f)
{
}

void LLSimInfo::setLandForSaleImage (LLUUID image_id) 
{
	mMapImageID = image_id;

	// Fetch the image
	if (mMapImageID.notNull())
	{
		mOverlayImage = LLViewerTextureManager::getFetchedTexture(mMapImageID, MIPMAP_TRUE, LLGLTexture::BOOST_MAP, LLViewerTexture::LOD_TEXTURE);
		mOverlayImage->setAddressMode(LLTexUnit::TAM_CLAMP);
	}
	else
	{
		mOverlayImage = NULL;
	}
}

LLPointer<LLViewerFetchedTexture> LLSimInfo::getLandForSaleImage () 
{
	if (mOverlayImage.isNull() && mMapImageID.notNull())
	{
		// Fetch the image if it hasn't been done yet (unlikely but...)
		mOverlayImage = LLViewerTextureManager::getFetchedTexture(mMapImageID, MIPMAP_TRUE, LLGLTexture::BOOST_MAP, LLViewerTexture::LOD_TEXTURE);
		mOverlayImage->setAddressMode(LLTexUnit::TAM_CLAMP);
	}
	if (!mOverlayImage.isNull())
	{
		// Boost the fetch level when we try to access that image
		mOverlayImage->setBoostLevel(LLGLTexture::BOOST_MAP);
	}
	return mOverlayImage;
}

LLVector3d LLSimInfo::getGlobalPos(const LLVector3& local_pos) const
{
	LLVector3d pos = from_region_handle(mHandle);
	pos.mdV[VX] += local_pos.mV[VX];
	pos.mdV[VY] += local_pos.mV[VY];
	pos.mdV[VZ] += local_pos.mV[VZ];
	return pos;
}

LLVector3d LLSimInfo::getGlobalOrigin() const
{
	return from_region_handle(mHandle);
}
LLVector3 LLSimInfo::getLocalPos(LLVector3d global_pos) const
{
	LLVector3d sim_origin = from_region_handle(mHandle);
	return LLVector3(global_pos - sim_origin);
}

void LLSimInfo::clearImage()
{
	if (!mOverlayImage.isNull())
	{
		mOverlayImage->setBoostLevel(0);
		mOverlayImage = NULL;
	}
}

void LLSimInfo::dropImagePriority()
{
	if (!mOverlayImage.isNull())
	{
		mOverlayImage->setBoostLevel(0);
	}
}

// Update the agent count for that region
void LLSimInfo::updateAgentCount(F64 time)
{
	if ((time - mAgentsUpdateTime > AGENTS_UPDATE_TIMER) || mFirstAgentRequest)
	{
		LLWorldMapMessage::getInstance()->sendItemRequest(MAP_ITEM_AGENT_LOCATIONS, mHandle);
		mAgentsUpdateTime = time;
		mFirstAgentRequest = false;
	}
}

// Get the total agents count
const S32 LLSimInfo::getAgentCount() const
{
	S32 total_agent_count = 0;
	for (LLSimInfo::item_info_list_t::const_iterator it = mAgentLocations.begin(); it != mAgentLocations.end(); ++it)
	{
		total_agent_count += it->getCount();
	}
	return total_agent_count;
}

bool LLSimInfo::isName(const std::string& name) const
{
	return (LLStringUtil::compareInsensitive(name, mName) == 0);
}

void LLSimInfo::dump() const
{
	U32 x_pos, y_pos;
	from_region_handle(mHandle, &x_pos, &y_pos);

	LL_INFOS("World Map") << x_pos << "," << y_pos
		<< " " << mName 
		<< " " << (S32)mAccess
		<< " " << std::hex << mRegionFlags << std::dec
//		<< " " << mWaterHeight
		<< LL_ENDL;
}

void LLSimInfo::clearItems()
{
	mTelehubs.clear();
	mInfohubs.clear();
	mPGEvents.clear();
	mMatureEvents.clear();
	mAdultEvents.clear();
	mLandForSale.clear();
	mLandForSaleAdult.clear();
//  We persist the agent count though as it is updated on a frequent basis
// 	mAgentLocations.clear();
}

void LLSimInfo::insertAgentLocation(const LLItemInfo& item) 
{
	std::string name = item.getName();

	// Find the last item in the list with a different name and erase them
	item_info_list_t::iterator lastiter;
	for (lastiter = mAgentLocations.begin(); lastiter != mAgentLocations.end(); ++lastiter)
	{
		LLItemInfo& info = *lastiter;
		if (info.isName(name))
		{
			break;
		}
	}
	if (lastiter != mAgentLocations.begin())
	{
		mAgentLocations.erase(mAgentLocations.begin(), lastiter);
	}

	// Now append the new location
	mAgentLocations.push_back(item); 
}

//---------------------------------------------------------------------------
// World Map
//---------------------------------------------------------------------------

LLWorldMap::LLWorldMap() :
	mIsTrackingLocation( false ),
	mIsTrackingFound( false ),
	mIsInvalidLocation( false ),
	mIsTrackingDoubleClick( false ),
	mIsTrackingCommit( false ),
	mTrackingLocation( 0, 0, 0 ),
	mFirstRequest(true)
{
	//LL_INFOS("World Map") << "Creating the World Map -> LLWorldMap::LLWorldMap()" << LL_ENDL;
	mMapBlockLoaded = new bool[MAP_BLOCK_RES*MAP_BLOCK_RES];
	clearSimFlags();
}


LLWorldMap::~LLWorldMap()
{
	//LL_INFOS("World Map") << "Destroying the World Map -> LLWorldMap::~LLWorldMap()" << LL_ENDL;
	reset();
	delete[] mMapBlockLoaded;
}


void LLWorldMap::reset()
{
	clearItems(true);		// Clear the items lists
	clearImageRefs();		// Clear the world mipmap and the land for sale tiles
	clearSimFlags();		// Clear the block info flags array 

	// Finally, clear the region map itself
	for_each(mSimInfoMap.begin(), mSimInfoMap.end(), DeletePairedPointer());
	mSimInfoMap.clear();
}

// Returns true if the items have been cleared
bool LLWorldMap::clearItems(bool force)
{
	bool clear = false;
	if ((mRequestTimer.getElapsedTimeF32() > REQUEST_ITEMS_TIMER) || mFirstRequest || force)
	{
		mRequestTimer.reset();

		LLSimInfo* sim_info = NULL;
		for (sim_info_map_t::iterator it = mSimInfoMap.begin(); it != mSimInfoMap.end(); ++it)
		{
			sim_info = it->second;
			if (sim_info)
			{
				sim_info->clearItems();
			}
		}
		clear = true;
		mFirstRequest = false;
	}
	return clear;
}

void LLWorldMap::clearImageRefs()
{
	// We clear the reference to the images we're holding.
	// Images hold by the world mipmap first
	mWorldMipmap.reset();

	// Images hold by the region map
	LLSimInfo* sim_info = NULL;
	for (sim_info_map_t::iterator it = mSimInfoMap.begin(); it != mSimInfoMap.end(); ++it)
	{
		sim_info = it->second;
		if (sim_info)
		{
			sim_info->clearImage();
		}
	}
}

// Doesn't clear the already-loaded sim infos, just re-requests them
void LLWorldMap::clearSimFlags()
{
	for (S32 idx=0; idx<MAP_BLOCK_RES*MAP_BLOCK_RES; ++idx)
	{
		mMapBlockLoaded[idx] = false;
	}
}

LLSimInfo* LLWorldMap::createSimInfoFromHandle(const U64 handle)
{
	LLSimInfo* sim_info = new LLSimInfo(handle);
	mSimInfoMap[handle] = sim_info;
	return sim_info;
}

void LLWorldMap::equalizeBoostLevels()
{
	mWorldMipmap.equalizeBoostLevels();
	return;
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
		return it->second;
	}
	return NULL;
}


LLSimInfo* LLWorldMap::simInfoFromName(const std::string& sim_name)
{
	LLSimInfo* sim_info = NULL;
	if (!sim_name.empty())
	{
		// Iterate through the entire sim info map and compare the name
		sim_info_map_t::iterator it;
		for (it = mSimInfoMap.begin(); it != mSimInfoMap.end(); ++it)
		{
			sim_info = it->second;
			if (sim_info && sim_info->isName(sim_name) )
			{
				// break out of loop if success
				break;
			}
		}
		// If we got to the end, we haven't found the sim. Reset the ouput value to NULL.
		if (it == mSimInfoMap.end())
			sim_info = NULL;
	}
	return sim_info;
}

bool LLWorldMap::simNameFromPosGlobal(const LLVector3d& pos_global, std::string & outSimName )
{
	LLSimInfo* sim_info = simInfoFromPosGlobal(pos_global);

	if (sim_info)
	{
		outSimName = sim_info->getName();
	}
	else
	{
		outSimName = "(unknown region)";
	}

	return (sim_info != NULL);
}

void LLWorldMap::reloadItems(bool force)
{
	//LL_INFOS("World Map") << "LLWorldMap::reloadItems()" << LL_ENDL;
	if (clearItems(force))
	{
		LLWorldMapMessage::getInstance()->sendItemRequest(MAP_ITEM_TELEHUB);
		LLWorldMapMessage::getInstance()->sendItemRequest(MAP_ITEM_PG_EVENT);
		LLWorldMapMessage::getInstance()->sendItemRequest(MAP_ITEM_MATURE_EVENT);
		LLWorldMapMessage::getInstance()->sendItemRequest(MAP_ITEM_ADULT_EVENT);
		LLWorldMapMessage::getInstance()->sendItemRequest(MAP_ITEM_LAND_FOR_SALE);
	}
}


// static public
// Insert a region in the region map
// returns true if region inserted, false otherwise
bool LLWorldMap::insertRegion(U32 x_world, U32 y_world, std::string& name, LLUUID& image_id, U32 accesscode, U64 region_flags)
{
	// This region doesn't exist
	if (accesscode == 255)
	{
		// Checks if the track point is in it and invalidates it if it is
		if (LLWorldMap::getInstance()->isTrackingInRectangle( x_world, y_world, x_world + REGION_WIDTH_UNITS, y_world + REGION_WIDTH_UNITS))
		{
			LLWorldMap::getInstance()->setTrackingInvalid();
		}
		// return failure to insert
		return false;
	}
	else
	{
		U64 handle = to_region_handle(x_world, y_world);
 		//LL_INFOS("World Map") << "Map sim : " << name << ", ID : " << image_id.getString() << LL_ENDL;
		// Insert the region in the region map of the world map
		// Loading the LLSimInfo object with what we got and insert it in the map
		LLSimInfo* siminfo = LLWorldMap::getInstance()->simInfoFromHandle(handle);
		if (siminfo == NULL)
		{
			siminfo = LLWorldMap::getInstance()->createSimInfoFromHandle(handle);
		}
		siminfo->setName(name);
		siminfo->setAccess(accesscode);
		siminfo->setRegionFlags(region_flags);
	//	siminfo->setWaterHeight((F32) water_height);
		siminfo->setLandForSaleImage(image_id);

		// Handle the location tracking (for teleport, UI feedback and info display)
		if (LLWorldMap::getInstance()->isTrackingInRectangle( x_world, y_world, x_world + REGION_WIDTH_UNITS, y_world + REGION_WIDTH_UNITS))
		{
			if (siminfo->isDown())
			{
				// We were tracking this location, but it's no available
				LLWorldMap::getInstance()->setTrackingInvalid();
			}
			else
			{
				// We were tracking this location, and it does exist and is available
				LLWorldMap::getInstance()->setTrackingValid();
			}
		}
		// return insert region success
		return true;
	}
}

// static public
// Insert an item in the relevant region map
// returns true if item inserted, false otherwise
bool LLWorldMap::insertItem(U32 x_world, U32 y_world, std::string& name, LLUUID& uuid, U32 type, S32 extra, S32 extra2)
{
	// Create an item record for the received object
	LLItemInfo new_item((F32)x_world, (F32)y_world, name, uuid);

	// Compute a region handle based on the objects coordinates
	LLVector3d	pos((F32)x_world, (F32)y_world, 40.0);
	U64 handle = to_region_handle(pos);

	// Get the region record for that handle or NULL if we haven't browsed it yet
	LLSimInfo* siminfo = LLWorldMap::getInstance()->simInfoFromHandle(handle);
	if (siminfo == NULL)
	{
		siminfo = LLWorldMap::getInstance()->createSimInfoFromHandle(handle);
	}

	//LL_INFOS("World Map") << "Process item : type = " << type << LL_ENDL;
	switch (type)
	{
		case MAP_ITEM_TELEHUB: // telehubs
		{
			/* Merov: we are not using the hub color anymore for display so commenting that out
			// Telehub color
			U32 X = x_world / REGION_WIDTH_UNITS;
			U32 Y = y_world / REGION_WIDTH_UNITS;
			F32 red = fmod((F32)X * 0.11f, 1.f) * 0.8f;
			F32 green = fmod((F32)Y * 0.11f, 1.f) * 0.8f;
			F32 blue = fmod(1.5f * (F32)(X + Y) * 0.11f, 1.f) * 0.8f;
			F32 add_amt = (X % 2) ? 0.15f : -0.15f;
			add_amt += (Y % 2) ? -0.15f : 0.15f;
			LLColor4 color(red + add_amt, green + add_amt, blue + add_amt);
			new_item.setColor(color);
			*/
			
			// extra2 specifies whether this is an infohub or a telehub.
			if (extra2)
			{
				siminfo->insertInfoHub(new_item);
			}
			else
			{
				siminfo->insertTeleHub(new_item);
			}
			break;
		}
		case MAP_ITEM_PG_EVENT: // events
		case MAP_ITEM_MATURE_EVENT:
		case MAP_ITEM_ADULT_EVENT:
		{
			std::string timeStr = "["+ LLTrans::getString ("TimeHour")+"]:["
					                   +LLTrans::getString ("TimeMin")+"] ["
									   +LLTrans::getString ("TimeAMPM")+"]";
			LLSD substitution;
			substitution["datetime"] = (S32) extra;
			LLStringUtil::format (timeStr, substitution);				
			new_item.setTooltip(timeStr);

			// HACK: store Z in extra2
			new_item.setElevation((F64)extra2);
			if (type == MAP_ITEM_PG_EVENT)
			{
				siminfo->insertPGEvent(new_item);
			}
			else if (type == MAP_ITEM_MATURE_EVENT)
			{
				siminfo->insertMatureEvent(new_item);
			}
			else if (type == MAP_ITEM_ADULT_EVENT)
			{
				siminfo->insertAdultEvent(new_item);
			}
			break;
		}
		case MAP_ITEM_LAND_FOR_SALE:		// land for sale
		case MAP_ITEM_LAND_FOR_SALE_ADULT:	// adult land for sale 
		{
			static LLUIString tooltip_fmt = LLTrans::getString("worldmap_item_tooltip_format");

			tooltip_fmt.setArg("[AREA]",  llformat("%d", extra));
			tooltip_fmt.setArg("[PRICE]", llformat("%d", extra2));
			new_item.setTooltip(tooltip_fmt.getString());

			if (type == MAP_ITEM_LAND_FOR_SALE)
			{
				siminfo->insertLandForSale(new_item);
			}
			else if (type == MAP_ITEM_LAND_FOR_SALE_ADULT)
			{
				siminfo->insertLandForSaleAdult(new_item);
			}
			break;
		}
		case MAP_ITEM_CLASSIFIED: // classifieds
		{
			//DEPRECATED: no longer used
			break;
		}
		case MAP_ITEM_AGENT_LOCATIONS: // agent locations
		{
// 				LL_INFOS("World Map") << "New Location " << new_item.mName << LL_ENDL;
			if (extra > 0)
			{
				new_item.setCount(extra);
				siminfo->insertAgentLocation(new_item);
			}
			break;
		}
		default:
			break;
	}
	return true;
}

bool LLWorldMap::isTrackingInRectangle(F64 x0, F64 y0, F64 x1, F64 y1)
{
	if (!mIsTrackingLocation)
		return false;
	return ((mTrackingLocation[0] >= x0) && (mTrackingLocation[0] < x1) && (mTrackingLocation[1] >= y0) && (mTrackingLocation[1] < y1));
}

// Drop priority of all images being fetched by the map
void LLWorldMap::dropImagePriorities()
{
	// Drop the download of tiles priority to nil
	mWorldMipmap.dropBoostLevels();
	// Same for the "land for sale" tiles per region
	for (sim_info_map_t::iterator it = mSimInfoMap.begin(); it != mSimInfoMap.end(); ++it)
	{
		LLSimInfo* info = it->second;
		info->dropImagePriority();
	}
}

// Load all regions in a given rectangle (in region grid coordinates, i.e. world / 256 meters)
void LLWorldMap::updateRegions(S32 x0, S32 y0, S32 x1, S32 y1)
{
	// Convert those boundaries to the corresponding (MAP_BLOCK_SIZE x MAP_BLOCK_SIZE) block coordinates
	x0 = x0 / MAP_BLOCK_SIZE;
	x1 = x1 / MAP_BLOCK_SIZE;
	y0 = y0 / MAP_BLOCK_SIZE;
	y1 = y1 / MAP_BLOCK_SIZE;

	// Load the region info those blocks
	for (S32 block_x = llmax(x0, 0); block_x <= llmin(x1, MAP_BLOCK_RES-1); ++block_x)
	{
		for (S32 block_y = llmax(y0, 0); block_y <= llmin(y1, MAP_BLOCK_RES-1); ++block_y)
		{
			S32 offset = block_x | (block_y * MAP_BLOCK_RES);
			if (!mMapBlockLoaded[offset])
			{
 				//LL_INFOS("World Map") << "Loading Block (" << block_x << "," << block_y << ")" << LL_ENDL;
				LLWorldMapMessage::getInstance()->sendMapBlockRequest(block_x * MAP_BLOCK_SIZE, block_y * MAP_BLOCK_SIZE, (block_x * MAP_BLOCK_SIZE) + MAP_BLOCK_SIZE - 1, (block_y * MAP_BLOCK_SIZE) + MAP_BLOCK_SIZE - 1);
				mMapBlockLoaded[offset] = true;
			}
		}
	}
}

void LLWorldMap::dump()
{
	LL_INFOS("World Map") << "LLWorldMap::dump()" << LL_ENDL;
	for (sim_info_map_t::iterator it = mSimInfoMap.begin(); it != mSimInfoMap.end(); ++it)
	{
		LLSimInfo* info = it->second;
		if (info)
		{
			info->dump();
		}
	}
}


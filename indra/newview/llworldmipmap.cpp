/** 
 * @file llworldmipmap.cpp
 * @brief Data storage for the S3 mipmap of the entire world.
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

#include "llworldmipmap.h"
#include "llviewercontrol.h"		// LLControlGroup

#include "llviewertexturelist.h"
#include "math.h"	// log()

// Turn this on to output tile stats in the standard output
#define DEBUG_TILES_STAT 0

LLWorldMipmap::LLWorldMipmap() :
	mCurrentLevel(0)
{
}

LLWorldMipmap::~LLWorldMipmap()
{
	reset();
}

// Delete all sublevel maps and clean them
void LLWorldMipmap::reset()
{
	for (int level = 0; level < MAP_LEVELS; level++)
	{
		mWorldObjectsMipMap[level].clear();
	}
}

// This method should be called before each use of the mipmap (typically, before each draw), so that to let
// the boost level of unused tiles to drop to 0 (BOOST_NONE).
// Tiles that are accessed have had their boost level pushed to BOOST_MAP_VISIBLE so we can identify them.
// The result of this strategy is that if a tile is not used during 2 consecutive loops, its boost level drops to 0.
void LLWorldMipmap::equalizeBoostLevels()
{
#if DEBUG_TILES_STAT
	S32 nb_missing = 0;
	S32 nb_tiles = 0;
	S32 nb_visible = 0;
#endif // DEBUG_TILES_STAT
	// For each level
	for (S32 level = 0; level < MAP_LEVELS; level++)
	{
		sublevel_tiles_t& level_mipmap = mWorldObjectsMipMap[level];
		// For each tile
		for (sublevel_tiles_t::iterator iter = level_mipmap.begin(); iter != level_mipmap.end(); iter++)
		{
			LLPointer<LLViewerFetchedTexture> img = iter->second;
			S32 current_boost_level = img->getBoostLevel();
			if (current_boost_level == LLGLTexture::BOOST_MAP_VISIBLE)
			{
				// If level was BOOST_MAP_VISIBLE, the tile has been used in the last draw so keep it high
				img->setBoostLevel(LLGLTexture::BOOST_MAP);
			}
			else
			{
				// If level was BOOST_MAP only (or anything else...), the tile wasn't used in the last draw 
				// so we drop its boost level to BOOST_NONE.
				img->setBoostLevel(LLGLTexture::BOOST_NONE);
			}
#if DEBUG_TILES_STAT
			// Increment some stats if compile option on
			nb_tiles++;
			if (current_boost_level == LLGLTexture::BOOST_MAP_VISIBLE)
			{
				nb_visible++;
			}
			if (img->isMissingAsset())
			{
				nb_missing++;
			}
#endif // DEBUG_TILES_STAT
		}
	}
#if DEBUG_TILES_STAT
	LL_INFOS("World Map") << "LLWorldMipmap tile stats : total requested = " << nb_tiles << ", visible = " << nb_visible << ", missing = " << nb_missing << LL_ENDL;
#endif // DEBUG_TILES_STAT
}

// This method should be used when the mipmap is not actively used for a while, e.g., the map UI is hidden
void LLWorldMipmap::dropBoostLevels()
{
	// For each level
	for (S32 level = 0; level < MAP_LEVELS; level++)
	{
		sublevel_tiles_t& level_mipmap = mWorldObjectsMipMap[level];
		// For each tile
		for (sublevel_tiles_t::iterator iter = level_mipmap.begin(); iter != level_mipmap.end(); iter++)
		{
			LLPointer<LLViewerFetchedTexture> img = iter->second;
			img->setBoostLevel(LLGLTexture::BOOST_NONE);
		}
	}
}

LLPointer<LLViewerFetchedTexture> LLWorldMipmap::getObjectsTile(U32 grid_x, U32 grid_y, S32 level, bool load)
{
	// Check the input data
	llassert(level <= MAP_LEVELS);
	llassert(level >= 1);

	// If the *loading* level changed, cleared the new level from "missed" tiles
	// so that we get a chance to reload them
	if (load && (level != mCurrentLevel))
	{
		cleanMissedTilesFromLevel(level);
		mCurrentLevel = level;
	}

	// Build the region handle
	U64 handle = convertGridToHandle(grid_x, grid_y);

	// Check if the image is around already
	sublevel_tiles_t& level_mipmap = mWorldObjectsMipMap[level-1];
	sublevel_tiles_t::iterator found = level_mipmap.find(handle);

	// If not there and load on, go load it
	if (found == level_mipmap.end())
	{
		if (load)
		{
			// Load it 
			LLPointer<LLViewerFetchedTexture> img = loadObjectsTile(grid_x, grid_y, level);
			// Insert the image in the map
			level_mipmap.insert(sublevel_tiles_t::value_type( handle, img ));
			// Find the element again in the map (it's there now...)
			found = level_mipmap.find(handle);
		}
		else
		{
			// Return with NULL if not found and we're not trying to load
			return NULL;
		}
	}

	// Get the image pointer and check if this asset is missing
	LLPointer<LLViewerFetchedTexture> img = found->second;
	if (img->isMissingAsset())
	{
		// Return NULL if asset missing
		return NULL;
	}
	else
	{
		// Boost the tile level so to mark it's in use *if* load on
		if (load)
		{
			img->setBoostLevel(LLGLTexture::BOOST_MAP_VISIBLE);
		}
		return img;
	}
}

LLPointer<LLViewerFetchedTexture> LLWorldMipmap::loadObjectsTile(U32 grid_x, U32 grid_y, S32 level)
{
	// Get the grid coordinates
	std::string imageurl = gSavedSettings.getString("CurrentMapServerURL") + llformat("map-%d-%d-%d-objects.jpg", level, grid_x, grid_y);

	// DO NOT COMMIT!! DEBUG ONLY!!!
	// Use a local jpeg for every tile to test map speed without S3 access
	//imageurl = "file://C:\\Develop\\mapserver-distribute-3\\indra\\build-vc80\\mapserver\\relwithdebinfo\\regions\\00995\\01001\\region-995-1001-prims.jpg";
	// END DEBUG
	//LL_INFOS("World Map") << "LLWorldMipmap::loadObjectsTile(), URL = " << imageurl << LL_ENDL;

	LLPointer<LLViewerFetchedTexture> img = LLViewerTextureManager::getFetchedTextureFromUrl(imageurl, FTT_MAP_TILE, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
	img->setBoostLevel(LLGLTexture::BOOST_MAP);

	// Return the smart pointer
	return img;
}

// This method is used to clean up a level from tiles marked as "missing".
// The idea is to allow tiles that have been improperly marked missing to be reloaded when retraversing the level again.
// When zooming in and out rapidly, some tiles are never properly loaded and, eventually marked missing.
// This creates "blue" areas in a subresolution that never got a chance to reload if we don't clean up the level.
void LLWorldMipmap::cleanMissedTilesFromLevel(S32 level)
{
	// Check the input data
	llassert(level <= MAP_LEVELS);
	llassert(level >= 0);

	// This happens when the object is first initialized
	if (level == 0)
	{
		return;
	}

	// Iterate through the subresolution level and suppress the tiles that are marked as missing
	// Note: erasing in a map while iterating through it is bug prone. Using a postfix increment is mandatory here.
	sublevel_tiles_t& level_mipmap = mWorldObjectsMipMap[level-1];
	sublevel_tiles_t::iterator it = level_mipmap.begin();
	while (it != level_mipmap.end())
	{
		LLPointer<LLViewerFetchedTexture> img = it->second;
		if (img->isMissingAsset())
		{
			level_mipmap.erase(it++);
		}
		else
		{
			++it;
		}
	}
	return;
}

// static methods
// Compute the level in the world mipmap (between 1 and MAP_LEVELS, as in the URL) given the scale (size of a sim in screen pixels)
S32 LLWorldMipmap::scaleToLevel(F32 scale)
{
	// If scale really small, picks up the higest level there is (lowest resolution)
	if (scale <= F32_MIN)
		return MAP_LEVELS;
	// Compute the power of two resolution level knowing the base level
	S32 level = llfloor((log(REGION_WIDTH_METERS/scale)/log(2.0f)) + 1.0f);
	// Check bounds and return the value
	if (level > MAP_LEVELS)
		return MAP_LEVELS;
	else if (level < 1)
		return 1;
	else
		return level;
}

// Convert world coordinates to mipmap grid coordinates at a given level (between 1 and MAP_LEVELS)
void LLWorldMipmap::globalToMipmap(F64 global_x, F64 global_y, S32 level, U32* grid_x, U32* grid_y)
{
	// Check the input data
	llassert(level <= MAP_LEVELS);
	llassert(level >= 1);

	// Convert world coordinates into grid coordinates
	*grid_x = lltrunc(global_x/REGION_WIDTH_METERS);
	*grid_y = lltrunc(global_y/REGION_WIDTH_METERS);
	// Compute the valid grid coordinates at that level of the mipmap
	S32 regions_in_tile = 1 << (level - 1);
	*grid_x = *grid_x - (*grid_x % regions_in_tile);
	*grid_y = *grid_y - (*grid_y % regions_in_tile);
}



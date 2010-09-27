/** 
 * @file llworldmipmap.h
 * @brief Data storage for the S3 mipmap of the entire world.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LL_LLWORLDMIPMAP_H
#define LL_LLWORLDMIPMAP_H

#include <map>

#include "llmemory.h"			// LLPointer
#include "indra_constants.h"	// REGION_WIDTH_UNITS
#include "llregionhandle.h"		// to_region_handle()

class LLViewerFetchedTexture;

// LLWorldMipmap : Mipmap handling of all the tiles used to render the world at any resolution.
// This class provides a clean structured access to the hierarchy of tiles stored in the 
// Amazon S3 repository and abstracts its directory/file structure.
// The interface of this class though still assumes that the caller knows the general level/tiles
// structure (at least, that it exists...) but doesn't requite the caller to know the details of it.
// IOW, you need to know that rendering levels exists as well as grid coordinates for regions, 
// but you can ignore where those tiles are located, how to get them, etc...
// The class API gives you back LLPointer<LLViewerFetchedTexture> per tile.

// See llworldmipmapview.cpp for the implementation of a class who knows how to render an LLWorldMipmap.

// Implementation notes:
// - On the S3 servers, the tiles are rendered in 2 flavors: Objects and Terrain.
// - For the moment, LLWorldMipmap implements access only to the Objects tiles.
class LLWorldMipmap
{
public:
	// Parameters of the mipmap
	static const S32 MAP_LEVELS = 8;		// Number of subresolution levels computed by the mapserver
	static const S32 MAP_TILE_SIZE = 256;	// Width in pixels of the tiles computed by the mapserver

	LLWorldMipmap();
	~LLWorldMipmap();

	// Clear up the maps and release all image handles
	void	reset();
	// Manage the boost levels between loops (typically draw() loops)
	void	equalizeBoostLevels();
	// Drop the boost levels to none (used when hiding the map)
	void	dropBoostLevels();
	// Get the tile smart pointer, does the loading if necessary
	LLPointer<LLViewerFetchedTexture> getObjectsTile(U32 grid_x, U32 grid_y, S32 level, bool load = true);

	// Helper functions: those are here as they depend solely on the topology of the mipmap though they don't access it
	// Convert sim scale (given in sim width in display pixels) into a mipmap level
	static S32  scaleToLevel(F32 scale);
	// Convert world coordinates to mipmap grid coordinates at a given level
	static void globalToMipmap(F64 global_x, F64 global_y, S32 level, U32* grid_x, U32* grid_y);

private:
	// Get a handle (key) from grid coordinates
	U64		convertGridToHandle(U32 grid_x, U32 grid_y) { return to_region_handle(grid_x * REGION_WIDTH_UNITS, grid_y * REGION_WIDTH_UNITS); }
	// Load the relevant tile from S3
	LLPointer<LLViewerFetchedTexture> loadObjectsTile(U32 grid_x, U32 grid_y, S32 level);
	// Clear a level from its "missing" tiles
	void cleanMissedTilesFromLevel(S32 level);

	// The mipmap is organized by resolution level (MAP_LEVELS of them). Each resolution level is an std::map
	// using a region_handle as a key and storing a smart pointer to the image as a value.
	typedef std::map<U64, LLPointer<LLViewerFetchedTexture> > sublevel_tiles_t;
	sublevel_tiles_t mWorldObjectsMipMap[MAP_LEVELS];
//	sublevel_tiles_t mWorldTerrainMipMap[MAP_LEVELS];

	S32 mCurrentLevel;		// The level last accessed by a getObjectsTile()
};

#endif // LL_LLWORLDMIPMAP_H

/** 
 * @file llregionhandle.h
 * @brief Routines for converting positions to/from region handles.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLREGIONHANDLE_H
#define LL_LLREGIONHANDLE_H

#include "indra_constants.h"
#include "v3math.h"
#include "v3dmath.h"

inline U64 to_region_handle(const U32 x_origin, const U32 y_origin)
{
	U64 region_handle;
	region_handle =  ((U64)x_origin) << 32;
	region_handle |= (U64) y_origin;
	return region_handle;
}

inline U64 to_region_handle(const LLVector3d& pos_global)
{
	U32 global_x = (U32)pos_global.mdV[VX];
	global_x -= global_x % 256;

	U32 global_y = (U32)pos_global.mdV[VY];
	global_y -= global_y % 256;

	return to_region_handle(global_x, global_y);
}

inline U64 to_region_handle_global(const F32 x_global, const F32 y_global)
{
	// Round down to the nearest origin
	U32 x_origin = (U32)x_global;
	x_origin -= x_origin % REGION_WIDTH_U32;
	U32 y_origin = (U32)y_global;
	y_origin -= y_origin % REGION_WIDTH_U32;
	U64 region_handle;
	region_handle =  ((U64)x_origin) << 32;
	region_handle |= (U64) y_origin;
	return region_handle;
}

inline BOOL to_region_handle(const F32 x_pos, const F32 y_pos, U64 *region_handle)
{
	U32 x_int, y_int;
	if (x_pos < 0.f)
	{
//		llwarns << "to_region_handle:Clamping negative x position " << x_pos << " to zero!" << llendl;
		return FALSE;
	}
	else
	{
		x_int = (U32)llround(x_pos);
	}
	if (y_pos < 0.f)
	{
//		llwarns << "to_region_handle:Clamping negative y position " << y_pos << " to zero!" << llendl;
		return FALSE;
	}
	else
	{
		y_int = (U32)llround(y_pos);
	}
	*region_handle = to_region_handle(x_int, y_int);
	return TRUE;
}

// stuff the word-frame XY location of sim's SouthWest corner in x_pos, y_pos
inline void from_region_handle(const U64 &region_handle, F32 *x_pos, F32 *y_pos)
{
	*x_pos = (F32)((U32)(region_handle >> 32));
	*y_pos = (F32)((U32)(region_handle & 0xFFFFFFFF));
}

// stuff the word-frame XY location of sim's SouthWest corner in x_pos, y_pos
inline void from_region_handle(const U64 &region_handle, U32 *x_pos, U32 *y_pos)
{
	*x_pos = ((U32)(region_handle >> 32));
	*y_pos = ((U32)(region_handle & 0xFFFFFFFF));
}

// return the word-frame XY location of sim's SouthWest corner in LLVector3d
inline LLVector3d from_region_handle(const U64 &region_handle)
{
	return LLVector3d(((U32)(region_handle >> 32)), (U32)(region_handle & 0xFFFFFFFF), 0.f);
}

// grid-based region handle encoding. pass in a grid position
// (eg: 1000,1000) and this will return the region handle.
inline U64 grid_to_region_handle(U32 grid_x, U32 grid_y)
{
	return to_region_handle(grid_x * REGION_WIDTH_UNITS,
							grid_y * REGION_WIDTH_UNITS);
}

inline void grid_from_region_handle(const U64& region_handle, U32* grid_x, U32* grid_y)
{
	from_region_handle(region_handle, grid_x, grid_y);
	*grid_x /= REGION_WIDTH_UNITS;
	*grid_y /= REGION_WIDTH_UNITS;
}
	
#endif

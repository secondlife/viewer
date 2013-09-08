/** 
 * @file lldefs.h
 * @brief Various generic constant definitions.
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

#ifndef LL_LLDEFS_H
#define LL_LLDEFS_H

#include "stdtypes.h"

// Often used array indices
const U32	VX			= 0;
const U32	VY			= 1;
const U32	VZ			= 2;
const U32	VW			= 3;
const U32	VS			= 3;

const U32	VRED		= 0;
const U32	VGREEN		= 1;
const U32	VBLUE		= 2;
const U32	VALPHA		= 3;

const U32	INVALID_DIRECTION = 0xFFFFFFFF;
const U32	EAST		= 0;
const U32	NORTH		= 1;
const U32	WEST		= 2;
const U32	SOUTH		= 3;

const U32	NORTHEAST	= 4;
const U32	NORTHWEST	= 5;
const U32	SOUTHWEST	= 6;
const U32	SOUTHEAST	= 7;
const U32	MIDDLE		= 8;

const U8	EAST_MASK		= 0x1<<EAST;
const U8	NORTH_MASK		= 0x1<<NORTH;
const U8	WEST_MASK		= 0x1<<WEST;
const U8	SOUTH_MASK		= 0x1<<SOUTH;

const U8	NORTHEAST_MASK	= NORTH_MASK | EAST_MASK;
const U8	NORTHWEST_MASK	= NORTH_MASK | WEST_MASK;
const U8	SOUTHWEST_MASK	= SOUTH_MASK | WEST_MASK;
const U8	SOUTHEAST_MASK	= SOUTH_MASK | EAST_MASK;

const U32 gDirOpposite[8] = {2, 3, 0, 1, 6, 7, 4, 5};
const U32 gDirAdjacent[8][2] =  {
								{4, 7},
								{4, 5},
								{5, 6},
								{6, 7},
								{0, 1},
								{1, 2},
								{2, 3},
								{0, 3}
								};

// Magnitude along the x and y axis
const S32 gDirAxes[8][2] = {
							{ 1, 0}, // east
							{ 0, 1}, // north
							{-1, 0}, // west
							{ 0,-1}, // south
							{ 1, 1}, // ne
							{-1, 1}, // nw
							{-1,-1}, // sw
							{ 1,-1}, // se
							};

const S32 gDirMasks[8] = { 
							EAST_MASK,
							NORTH_MASK,
							WEST_MASK,
							SOUTH_MASK,
							NORTHEAST_MASK,
							NORTHWEST_MASK,
							SOUTHWEST_MASK,
							SOUTHEAST_MASK
							};

// Sides of a box...
//                  . Z      __.Y
//                 /|\        /|       0 = NO_SIDE
//                  |        /         1 = FRONT_SIDE   = +x
//           +------|-----------+      2 = BACK_SIDE    = -x
//          /|      |/     /   /|      3 = LEFT_SIDE    = +y
//         / |     -5-   |/   / |      4 = RIGHT_SIDE   = -y
//        /  |     /|   -3-  /  |      5 = TOP_SIDE     = +z
//       +------------------+   |      6 = BOTTOM_SIDE  = -z
//       |   |      |  /    |   |
//       | |/|      | /     | |/|                   
//       | 2 |    | *-------|-1--------> X           
//       |/| |   -4-        |/| |                  
//       |   +----|---------|---+
//       |  /        /      |  /
//       | /       -6-      | /
//       |/        /        |/ 
//       +------------------+
const U32 NO_SIDE 		= 0;
const U32 FRONT_SIDE 	= 1;
const U32 BACK_SIDE 	= 2;
const U32 LEFT_SIDE 	= 3;
const U32 RIGHT_SIDE 	= 4;
const U32 TOP_SIDE 		= 5;
const U32 BOTTOM_SIDE 	= 6;

const U8 LL_SOUND_FLAG_NONE =         0x0;
const U8 LL_SOUND_FLAG_LOOP =         1<<0;
const U8 LL_SOUND_FLAG_SYNC_MASTER =  1<<1;
const U8 LL_SOUND_FLAG_SYNC_SLAVE =   1<<2;
const U8 LL_SOUND_FLAG_SYNC_PENDING = 1<<3;
const U8 LL_SOUND_FLAG_QUEUE =        1<<4;
const U8 LL_SOUND_FLAG_STOP =         1<<5;
const U8 LL_SOUND_FLAG_SYNC_MASK = LL_SOUND_FLAG_SYNC_MASTER | LL_SOUND_FLAG_SYNC_SLAVE | LL_SOUND_FLAG_SYNC_PENDING;

//
// *NOTE: These values may be used as hard-coded numbers in scanf() variants.
//
// --------------
// DO NOT CHANGE.
// --------------
//
const U32	LL_MAX_PATH		= 1024;		// buffer size of maximum path + filename string length

// For strings we send in messages
const U32	STD_STRING_BUF_SIZE	= 255;	// Buffer size
const U32	STD_STRING_STR_LEN	= 254;	// Length of the string (not including \0)

// *NOTE: This value is used as hard-coded numbers in scanf() variants.
// DO NOT CHANGE.
const U32	MAX_STRING			= STD_STRING_BUF_SIZE;	// Buffer size

const U32	MAXADDRSTR		= 17;		// 123.567.901.345 = 15 chars + \0 + 1 for good luck

// C++ is our friend. . . use template functions to make life easier!

// specific inlines for basic types
//
// defined for all:
//   llmin(a,b)
//   llmax(a,b)
//   llclamp(a,minimum,maximum)
//
// defined for F32, F64:
//   llclampf(a)     // clamps a to [0.0 .. 1.0]
//
// defined for U16, U32, U64, S16, S32, S64, :
//   llclampb(a)     // clamps a to [0 .. 255]
//   				   

template <class LLDATATYPE> 
inline LLDATATYPE llmax(const LLDATATYPE& d1, const LLDATATYPE& d2)
{
	return (d1 > d2) ? d1 : d2;
}

template <class LLDATATYPE> 
inline LLDATATYPE llmax(const LLDATATYPE& d1, const LLDATATYPE& d2, const LLDATATYPE& d3)
{
	LLDATATYPE r = llmax(d1,d2);
	return llmax(r, d3);
}

template <class LLDATATYPE> 
inline LLDATATYPE llmax(const LLDATATYPE& d1, const LLDATATYPE& d2, const LLDATATYPE& d3, const LLDATATYPE& d4)
{
	LLDATATYPE r1 = llmax(d1,d2);
	LLDATATYPE r2 = llmax(d3,d4);
	return llmax(r1, r2);
}

template <class LLDATATYPE> 
inline LLDATATYPE llmin(const LLDATATYPE& d1, const LLDATATYPE& d2)
{
	return (d1 < d2) ? d1 : d2;
}

template <class LLDATATYPE> 
inline LLDATATYPE llmin(const LLDATATYPE& d1, const LLDATATYPE& d2, const LLDATATYPE& d3)
{
	LLDATATYPE r = llmin(d1,d2);
	return (r < d3 ? r : d3);
}

template <class LLDATATYPE> 
inline LLDATATYPE llmin(const LLDATATYPE& d1, const LLDATATYPE& d2, const LLDATATYPE& d3, const LLDATATYPE& d4)
{
	LLDATATYPE r1 = llmin(d1,d2);
	LLDATATYPE r2 = llmin(d3,d4);
	return llmin(r1, r2);
}

template <class LLDATATYPE> 
inline LLDATATYPE llclamp(const LLDATATYPE& a, const LLDATATYPE& minval, const LLDATATYPE& maxval)
{
	if ( a < minval )
	{
		return minval;
	}
	else if ( a > maxval )
	{
		return maxval;
	}
	return a;
}

template <class LLDATATYPE> 
inline LLDATATYPE llclampf(const LLDATATYPE& a)
{
	return llmin(llmax(a, (LLDATATYPE)0), (LLDATATYPE)1);
}

template <class LLDATATYPE> 
inline LLDATATYPE llclampb(const LLDATATYPE& a)
{
	return llmin(llmax(a, (LLDATATYPE)0), (LLDATATYPE)255);
}

template <class LLDATATYPE> 
inline void llswap(LLDATATYPE& lhs, LLDATATYPE& rhs)
{
	LLDATATYPE tmp = lhs;
	lhs = rhs;
	rhs = tmp;
}

#endif // LL_LLDEFS_H


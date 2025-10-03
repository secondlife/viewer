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
#include <type_traits>

// Often used array indices
constexpr U32   VX          = 0;
constexpr U32   VY          = 1;
constexpr U32   VZ          = 2;
constexpr U32   VW          = 3;
constexpr U32   VS          = 3;

constexpr U32   VRED        = 0;
constexpr U32   VGREEN      = 1;
constexpr U32   VBLUE       = 2;
constexpr U32   VALPHA      = 3;

constexpr U32   INVALID_DIRECTION = 0xFFFFFFFF;
constexpr U32   EAST        = 0;
constexpr U32   NORTH       = 1;
constexpr U32   WEST        = 2;
constexpr U32   SOUTH       = 3;

constexpr U32   NORTHEAST   = 4;
constexpr U32   NORTHWEST   = 5;
constexpr U32   SOUTHWEST   = 6;
constexpr U32   SOUTHEAST   = 7;
constexpr U32   MIDDLE      = 8;

constexpr U8    EAST_MASK       = 0x1<<EAST;
constexpr U8    NORTH_MASK      = 0x1<<NORTH;
constexpr U8    WEST_MASK       = 0x1<<WEST;
constexpr U8    SOUTH_MASK      = 0x1<<SOUTH;

constexpr U8    NORTHEAST_MASK  = NORTH_MASK | EAST_MASK;
constexpr U8    NORTHWEST_MASK  = NORTH_MASK | WEST_MASK;
constexpr U8    SOUTHWEST_MASK  = SOUTH_MASK | WEST_MASK;
constexpr U8    SOUTHEAST_MASK  = SOUTH_MASK | EAST_MASK;

constexpr U32 gDirOpposite[8] = {2, 3, 0, 1, 6, 7, 4, 5};
constexpr U32 gDirAdjacent[8][2] =  {
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
constexpr S32 gDirAxes[8][2] = {
                               { 1, 0}, // east
                               { 0, 1}, // north
                               {-1, 0}, // west
                               { 0,-1}, // south
                               { 1, 1}, // ne
                               {-1, 1}, // nw
                               {-1,-1}, // sw
                               { 1,-1}, // se
                               };

constexpr S32 gDirMasks[8] = {
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
constexpr U32 NO_SIDE       = 0;
constexpr U32 FRONT_SIDE    = 1;
constexpr U32 BACK_SIDE     = 2;
constexpr U32 LEFT_SIDE     = 3;
constexpr U32 RIGHT_SIDE    = 4;
constexpr U32 TOP_SIDE      = 5;
constexpr U32 BOTTOM_SIDE   = 6;

constexpr U8 LL_SOUND_FLAG_NONE =         0x0;
constexpr U8 LL_SOUND_FLAG_LOOP =         1<<0;
constexpr U8 LL_SOUND_FLAG_SYNC_MASTER =  1<<1;
constexpr U8 LL_SOUND_FLAG_SYNC_SLAVE =   1<<2;
constexpr U8 LL_SOUND_FLAG_SYNC_PENDING = 1<<3;
constexpr U8 LL_SOUND_FLAG_QUEUE =        1<<4;
constexpr U8 LL_SOUND_FLAG_STOP =         1<<5;
constexpr U8 LL_SOUND_FLAG_SYNC_MASK = LL_SOUND_FLAG_SYNC_MASTER | LL_SOUND_FLAG_SYNC_SLAVE | LL_SOUND_FLAG_SYNC_PENDING;

//
// *NOTE: These values may be used as hard-coded numbers in scanf() variants.
//
// --------------
// DO NOT CHANGE.
// --------------
//
constexpr U32   LL_MAX_PATH     = 1024;     // buffer size of maximum path + filename string length

// For strings we send in messages
constexpr U32   STD_STRING_BUF_SIZE = 255;  // Buffer size
constexpr U32   STD_STRING_STR_LEN  = 254;  // Length of the string (not including \0)

// *NOTE: This value is used as hard-coded numbers in scanf() variants.
// DO NOT CHANGE.
constexpr U32   MAX_STRING          = STD_STRING_BUF_SIZE;  // Buffer size

constexpr U32   MAXADDRSTR      = 17;       // 123.567.901.345 = 15 chars + \0 + 1 for good luck

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

// recursion tail
template <typename T>
constexpr auto llmax(T data)
{
    return data;
}

template <typename T0, typename T1, typename... Ts>
constexpr auto llmax(T0 d0, T1 d1, Ts... rest)
{
    auto maxrest = llmax(d1, rest...);
    return (d0 > maxrest)? d0 : maxrest;
}

// recursion tail
template <typename T>
constexpr auto llmin(T data)
{
    return data;
}

template <typename T0, typename T1, typename... Ts>
constexpr auto llmin(T0 d0, T1 d1, Ts... rest)
{
    auto minrest = llmin(d1, rest...);
    return (d0 < minrest) ? d0 : minrest;
}

template <typename A, typename MIN, typename MAX>
constexpr A llclamp(A a, MIN minval, MAX maxval)
{
    A aminval{ static_cast<A>(minval) }, amaxval{ static_cast<A>(maxval) };
    if ( a < aminval )
    {
        return aminval;
    }
    else if ( a > amaxval )
    {
        return amaxval;
    }
    return a;
}

template <class LLDATATYPE>
constexpr LLDATATYPE llclampf(LLDATATYPE a)
{
    return llmin(llmax(a, LLDATATYPE(0)), LLDATATYPE(1));
}

template <class LLDATATYPE>
constexpr LLDATATYPE llclampb(LLDATATYPE a)
{
    return llmin(llmax(a, LLDATATYPE(0)), LLDATATYPE(255));
}

template <class LLDATATYPE>
inline void llswap(LLDATATYPE& lhs, LLDATATYPE& rhs)
{
    std::swap(lhs, rhs);
}

#endif // LL_LLDEFS_H


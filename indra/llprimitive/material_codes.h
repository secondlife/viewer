/** 
 * @file material_codes.h
 * @brief Material_codes definitions
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_MATERIAL_CODES_H
#define LL_MATERIAL_CODES_H

class LLUUID;

    // material types
const U8    LL_MCODE_STONE   = 0;
const U8    LL_MCODE_METAL   = 1;
const U8    LL_MCODE_GLASS   = 2;
const U8    LL_MCODE_WOOD    = 3;
const U8    LL_MCODE_FLESH   = 4;
const U8    LL_MCODE_PLASTIC = 5;
const U8    LL_MCODE_RUBBER  = 6;
const U8    LL_MCODE_LIGHT   = 7;
const U8    LL_MCODE_END     = 8;
const U8    LL_MCODE_MASK    = 0x0F;

// *NOTE: Define these in .cpp file to reduce duplicate instances
extern const LLUUID LL_DEFAULT_STONE_UUID;
extern const LLUUID LL_DEFAULT_METAL_UUID;
extern const LLUUID LL_DEFAULT_GLASS_UUID;
extern const LLUUID LL_DEFAULT_WOOD_UUID;
extern const LLUUID LL_DEFAULT_FLESH_UUID;
extern const LLUUID LL_DEFAULT_PLASTIC_UUID;
extern const LLUUID LL_DEFAULT_RUBBER_UUID;
extern const LLUUID LL_DEFAULT_LIGHT_UUID;

#endif

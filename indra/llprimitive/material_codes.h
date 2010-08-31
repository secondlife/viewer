/** 
 * @file material_codes.h
 * @brief Material_codes definitions
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#ifndef LL_MATERIAL_CODES_H
#define LL_MATERIAL_CODES_H

class LLUUID;

	// material types
const U8	LL_MCODE_STONE   = 0;
const U8	LL_MCODE_METAL   = 1;
const U8	LL_MCODE_GLASS   = 2;
const U8	LL_MCODE_WOOD    = 3;
const U8	LL_MCODE_FLESH   = 4;
const U8	LL_MCODE_PLASTIC = 5;
const U8	LL_MCODE_RUBBER  = 6;
const U8	LL_MCODE_LIGHT   = 7;
const U8    LL_MCODE_END     = 8;
const U8	LL_MCODE_MASK    = 0x0F;

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

/** 
 * @file material_codes.h
 * @brief Material_codes definitions
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_MATERIAL_CODES_H
#define LL_MATERIAL_CODES_H

#include "lluuid.h"

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

const LLUUID LL_DEFAULT_STONE_UUID("87c5765b-aa26-43eb-b8c6-c09a1ca6208e");
const LLUUID LL_DEFAULT_METAL_UUID("6f3c53e9-ba60-4010-8f3e-30f51a762476");
const LLUUID LL_DEFAULT_GLASS_UUID("b4ba225c-373f-446d-9f7e-6cb7b5cf9b3d");
const LLUUID LL_DEFAULT_WOOD_UUID("89556747-24cb-43ed-920b-47caed15465f");
const LLUUID LL_DEFAULT_FLESH_UUID("80736669-e4b9-450e-8890-d5169f988a50");
const LLUUID LL_DEFAULT_PLASTIC_UUID("304fcb4e-7d33-4339-ba80-76d3d22dc11a");
const LLUUID LL_DEFAULT_RUBBER_UUID("9fae0bc5-666d-477e-9f70-84e8556ec867");
const LLUUID LL_DEFAULT_LIGHT_UUID("00000000-0000-0000-0000-000000000000");

#endif

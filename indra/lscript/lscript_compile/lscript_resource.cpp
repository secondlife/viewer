/** 
 * @file lscript_resource.cpp
 * @brief resource determination prior to assembly
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "lscript_resource.h"

void init_temp_jumps()
{
	gTempJumpCount = 0;
}

S32 gTempJumpCount = 0;

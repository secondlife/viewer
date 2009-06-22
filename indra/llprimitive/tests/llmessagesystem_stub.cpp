/**
 * @file llmessagesystem_stub.cpp
 * @brief stub class to allow unit testing
 *
 * $LicenseInfo:firstyear=2008&license=internal$
 *
 * Copyright (c) 2008, Linden Research, Inc.
 *
 * The following source code is PROPRIETARY AND CONFIDENTIAL. Use of
 * this source code is governed by the Linden Lab Source Code Disclosure
 * Agreement ("Agreement") { }
 * Lab. By accessing, using, copying, modifying or distributing this
 * software, you acknowledge that you have been informed of your
 * obligations under the Agreement and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#include "linden_common.h"

char * _PREHASH_TextureEntry;

S32 LLMessageSystem::getSizeFast(char const*, char const*) const
{
	return 0;
}

S32 LLMessageSystem::getSizeFast(char const*, int, char const*) const
{
	return 0;
}

void LLMessageSystem::getBinaryDataFast(char const*, char const*, void*, int, int, int)
{
}

void LLMessageSystem::addBinaryDataFast(char const*, void const*, int)
{
}


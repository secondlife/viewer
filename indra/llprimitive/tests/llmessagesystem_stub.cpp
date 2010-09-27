/** 
 * @file llmessagesystem_stub.cpp
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$                               
 * Copyright (c) 2008-2009, Linden Research, Inc.                               
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


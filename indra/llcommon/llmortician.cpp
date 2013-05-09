/** 
 * @file llmortician.cpp
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "linden_common.h"
#include "llmortician.h"

#include <list>

std::list<LLMortician*> LLMortician::sGraveyard;

BOOL LLMortician::sDestroyImmediate = FALSE;

LLMortician::~LLMortician() 
{
	sGraveyard.remove(this);
}

void LLMortician::updateClass() 
{
	while (!sGraveyard.empty()) 
	{
		LLMortician* dead = sGraveyard.front();
		delete dead;
	}
}

void LLMortician::die() 
{
	// It is valid to call die() more than once on something that hasn't died yet
	if (sDestroyImmediate)
	{
		// *NOTE: This is a hack to ensure destruction order on shutdown (relative to non-mortician controlled classes).
		mIsDead = TRUE;
		delete this;
		return;
	}
	else if (!mIsDead)
	{
		mIsDead = TRUE;
		sGraveyard.push_back(this);
	}
}

// static
void LLMortician::setZealous(BOOL b)
{
	sDestroyImmediate = b;
}

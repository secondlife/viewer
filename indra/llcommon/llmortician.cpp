/** 
 * @file llmortician.cpp
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

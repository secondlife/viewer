/** 
 * @file llscriptresourcepool.h
 * @brief A collection of LLScriptResources
 *
 * $LicenseInfo:firstyear=2008&license=internal$
 * 
 * Copyright (c) 2008, Linden Research, Inc.
 * 
 * The following source code is PROPRIETARY AND CONFIDENTIAL. Use of
 * this source code is governed by the Linden Lab Source Code Disclosure
 * Agreement ("Agreement") previously entered between you and Linden
 * Lab. By accessing, using, copying, modifying or distributing this
 * software, you acknowledge that you have been informed of your
 * obligations under the Agreement and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLSCRIPTRESOURCEPOOL_H
#define LL_LLSCRIPTRESOURCEPOOL_H

#include "llscriptresource.h"

// This is just a holder for LLSimResources
class LLScriptResourcePool
{
public:	
	LLScriptResourcePool();
	// ~LLSimResourceMgr();

	LLScriptResource& getPublicURLResource();
	const LLScriptResource& getPublicURLResource() const;

	// An empty resource pool.
	static LLScriptResourcePool null;

private:
	LLScriptResource mLSLPublicURLs;
};



#endif

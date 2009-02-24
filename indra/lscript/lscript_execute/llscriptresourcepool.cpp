/** 
 * @file llscriptresourcepool.cpp
 * @brief Collection of limited script resources
 *
 * $LicenseInfo:firstyear=2002&license=internal$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#include "llscriptresourcepool.h"

LLScriptResourcePool LLScriptResourcePool::null;

LLScriptResourcePool::LLScriptResourcePool() 
{ 

}

LLScriptResource& LLScriptResourcePool::getPublicURLResource()
{
	return mLSLPublicURLs;
}

const LLScriptResource& LLScriptResourcePool::getPublicURLResource() const
{
	return mLSLPublicURLs;
}

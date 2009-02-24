/** 
 * @file llscriptresourceconsumer.cpp
 * @brief An interface for a script resource consumer.
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

#include "llscriptresourceconsumer.h"

#include "llscriptresourcepool.h"

LLScriptResourceConsumer::LLScriptResourceConsumer()
	: mScriptResourcePool(&LLScriptResourcePool::null)
{ }

// Get the resource pool this consumer is currently using.
// virtual 
LLScriptResourcePool& LLScriptResourceConsumer::getScriptResourcePool()
{
	return *mScriptResourcePool;
}

// Get the resource pool this consumer is currently using.
// virtual 
const LLScriptResourcePool& LLScriptResourceConsumer::getScriptResourcePool() const
{
	return *mScriptResourcePool;
}

// virtual
void LLScriptResourceConsumer::setScriptResourcePool(LLScriptResourcePool& new_pool)
{
	mScriptResourcePool = &new_pool;
}

bool LLScriptResourceConsumer::switchScriptResourcePools(LLScriptResourcePool& new_pool)
{
	if (&new_pool == &LLScriptResourcePool::null)
	{
		llwarns << "New pool is null" << llendl;
	}

	if (isInPool(new_pool))
	{
		return true;
	}

	if (!canUseScriptResourcePool(new_pool))
	{
		return false;
	}

	S32 used_urls = getUsedPublicURLs();
	
	getScriptResourcePool().getPublicURLResource().release( used_urls );
	setScriptResourcePool(new_pool);
	getScriptResourcePool().getPublicURLResource().request( used_urls );
	
	return true;
}

bool LLScriptResourceConsumer::canUseScriptResourcePool(const LLScriptResourcePool& resource_pool)
{
	if (isInPool(resource_pool))
	{
		return true;
	}

	if (resource_pool.getPublicURLResource().getAvailable() < getUsedPublicURLs())
	{
		return false;
	}
	
	return true;
}

bool LLScriptResourceConsumer::isInPool(const LLScriptResourcePool& resource_pool)
{
	const LLScriptResourcePool& current_pool = getScriptResourcePool();
	if ( &resource_pool == &current_pool )
	{
		// This consumer is already in this pool
		return true;
	}
	return false;
}


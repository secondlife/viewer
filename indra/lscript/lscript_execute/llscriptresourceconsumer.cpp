/** 
 * @file llscriptresourceconsumer.cpp
 * @brief An interface for a script resource consumer.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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


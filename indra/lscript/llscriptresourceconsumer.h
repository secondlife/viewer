/** 
 * @file llscriptresourceconsumer.h
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

#ifndef LL_LLSCRIPTRESOURCECONSUMER_H
#define LL_LLSCRIPTRESOURCECONSUMER_H

#include "linden_common.h"

class LLScriptResourcePool;

// Entities that use limited script resources 
// should implement this interface

class LLScriptResourceConsumer
{
public:	
	LLScriptResourceConsumer();

	virtual ~LLScriptResourceConsumer() { }

	// Get the number of public urls used by this consumer.
	virtual S32 getUsedPublicURLs() const = 0;

	// Get the resource pool this consumer is currently using.
	LLScriptResourcePool& getScriptResourcePool();
	const LLScriptResourcePool& getScriptResourcePool() const;

	bool switchScriptResourcePools(LLScriptResourcePool& new_pool);
	bool canUseScriptResourcePool(const LLScriptResourcePool& resource_pool);
	bool isInPool(const LLScriptResourcePool& resource_pool);

protected:
	virtual void setScriptResourcePool(LLScriptResourcePool& pool);

	LLScriptResourcePool* mScriptResourcePool;
};

#endif // LL_LLSCRIPTRESOURCECONSUMER_H

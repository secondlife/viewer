/** 
 * @file llscriptresourceconsumer.h
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

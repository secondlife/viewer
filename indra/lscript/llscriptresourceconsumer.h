/** 
 * @file llscriptresourceconsumer.h
 * @brief An interface for a script resource consumer.
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

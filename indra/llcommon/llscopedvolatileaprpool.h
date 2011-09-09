/**
 * @file llscopedvolatileaprpool.h
 * @brief Implementation of LLScopedVolatileAPRPool
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_LLSCOPEDVOLATILEAPRPOOL_H
#define LL_LLSCOPEDVOLATILEAPRPOOL_H

#include "llthread.h"

/** Scoped volatile memory pool.
 *
 * As the LLVolatileAPRPool should never keep allocations very
 * long, its most common use is for allocations with a lifetime
 * equal to it's scope.
 *
 * This is a convenience class that makes just a little easier to type.
 */
class LL_COMMON_API LLScopedVolatileAPRPool
{
private:
	LLVolatileAPRPool& mPool;
	apr_pool_t* mScopedAPRpool;		// The use of apr_pool_t is OK here.
public:
	LLScopedVolatileAPRPool() : mPool(LLThreadLocalData::tldata().mVolatileAPRPool), mScopedAPRpool(mPool.getVolatileAPRPool()) { }
	~LLScopedVolatileAPRPool() { mPool.clearVolatileAPRPool(); }
	//! @attention Only use this to pass the underlaying pointer to a libapr-1 function that requires it.
	operator apr_pool_t*() const { return mScopedAPRpool; }		// The use of apr_pool_t is OK here.
};

#endif

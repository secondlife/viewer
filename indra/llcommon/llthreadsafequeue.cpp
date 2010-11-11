/** 
 * @file llthread.cpp
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#include <apr_pools.h>
#include <apr_queue.h>
#include "llthreadsafequeue.h"



// LLThreadSafeQueueImplementation
//-----------------------------------------------------------------------------


LLThreadSafeQueueImplementation::LLThreadSafeQueueImplementation(apr_pool_t * pool, unsigned int capacity):
	mOwnsPool(pool == 0),
	mPool(pool),
	mQueue(0)
{
	if(mOwnsPool) {
		apr_status_t status = apr_pool_create(&mPool, 0);
		if(status != APR_SUCCESS) throw LLThreadSafeQueueError("failed to allocate pool");
	} else {
		; // No op.
	}
	
	apr_status_t status = apr_queue_create(&mQueue, capacity, mPool);
	if(status != APR_SUCCESS) throw LLThreadSafeQueueError("failed to allocate queue");
}


LLThreadSafeQueueImplementation::~LLThreadSafeQueueImplementation()
{
	if(mQueue != 0) {
		if(apr_queue_size(mQueue) != 0) llwarns << 
			"terminating queue which still contains " << apr_queue_size(mQueue) <<
			" elements;" << "memory will be leaked" << LL_ENDL;
		apr_queue_term(mQueue);
	}
	if(mOwnsPool && (mPool != 0)) apr_pool_destroy(mPool);
}


void LLThreadSafeQueueImplementation::pushFront(void * element)
{
	apr_status_t status = apr_queue_push(mQueue, element);
	
	if(status == APR_EINTR) {
		throw LLThreadSafeQueueInterrupt();
	} else if(status != APR_SUCCESS) {
		throw LLThreadSafeQueueError("push failed");
	} else {
		; // Success.
	}
}


bool LLThreadSafeQueueImplementation::tryPushFront(void * element){
	return apr_queue_trypush(mQueue, element) == APR_SUCCESS;
}


void * LLThreadSafeQueueImplementation::popBack(void)
{
	void * element;
	apr_status_t status = apr_queue_pop(mQueue, &element);

	if(status == APR_EINTR) {
		throw LLThreadSafeQueueInterrupt();
	} else if(status != APR_SUCCESS) {
		throw LLThreadSafeQueueError("pop failed");
	} else {
		return element;
	}
}


bool LLThreadSafeQueueImplementation::tryPopBack(void *& element)
{
	return apr_queue_trypop(mQueue, &element) == APR_SUCCESS;
}


size_t LLThreadSafeQueueImplementation::size()
{
	return apr_queue_size(mQueue);
}

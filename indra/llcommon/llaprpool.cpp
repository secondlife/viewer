/**
 * @file llaprpool.cpp
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
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
 *
 * CHANGELOG
 *   and additional copyright holders.
 *
 *   04/04/2010
 *   - Initial version, written by Aleric Inglewood @ SL
 *
 *   10/11/2010
 *   - Added APR_HAS_THREADS #if's to allow creation and destruction
 *     of subpools by threads other than the parent pool owner.
 */

#include "linden_common.h"

#include "llerror.h"
#include "llaprpool.h"
#include "llthread.h"

// Create a subpool from parent.
void LLAPRPool::create(LLAPRPool& parent)
{
	llassert(!mPool);			// Must be non-initialized.
	mParent = &parent;
	if (!mParent)				// Using the default parameter?
	{
		// By default use the root pool of the current thread.
		mParent = &LLThreadLocalData::tldata().mRootPool;
	}
	llassert(mParent->mPool);	// Parent must be initialized.
#if APR_HAS_THREADS
	// As per the documentation of APR (ie http://apr.apache.org/docs/apr/1.4/apr__pools_8h.html):
	//
	// Note that most operations on pools are not thread-safe: a single pool should only be
	// accessed by a single thread at any given time. The one exception to this rule is creating
	// a subpool of a given pool: one or more threads can safely create subpools at the same
	// time that another thread accesses the parent pool.
	//
	// In other words, it's safe for any thread to create a (sub)pool, independent of who
	// owns the parent pool.
	mOwner = apr_os_thread_current();
#else
	mOwner = mParent->mOwner;
	llassert(apr_os_thread_equal(mOwner, apr_os_thread_current()));
#endif
	apr_status_t const apr_pool_create_status = apr_pool_create(&mPool, mParent->mPool);
	llassert_always(apr_pool_create_status == APR_SUCCESS);
	llassert(mPool);			// Initialized.
	apr_pool_cleanup_register(mPool, this, &s_plain_cleanup, &apr_pool_cleanup_null);
}

// Destroy the (sub)pool, if any.
void LLAPRPool::destroy(void)
{
	// Only do anything if we are not already (being) destroyed.
	if (mPool)
	{
#if !APR_HAS_THREADS
		// If we are a root pool, then every thread may destruct us: in that case
		// we have to assume that no other thread will use this pool concurrently,
		// of course. Otherwise, if we are a subpool, only the thread that owns
		// the parent may destruct us, since that is the pool that is still alive,
		// possibly being used by others and being altered here.
		llassert(!mParent || apr_os_thread_equal(mParent->mOwner, apr_os_thread_current()));
#endif
		apr_pool_t* pool = mPool;	// The use of apr_pool_t is OK here.
									// Temporary store before destroying the pool.
		mPool = NULL;				// Mark that we are BEING destructed.
		apr_pool_cleanup_kill(pool, this, &s_plain_cleanup);
		apr_pool_destroy(pool);
	}
}

bool LLAPRPool::parent_is_being_destructed(void)
{
	return mParent && (!mParent->mPool || mParent->parent_is_being_destructed());
}

LLAPRInitialization::LLAPRInitialization(void)
{
	static bool apr_initialized = false;

	if (!apr_initialized)
	{
		apr_initialize();
	}

	apr_initialized = true;
}

bool LLAPRRootPool::sCountInitialized = false;
apr_uint32_t volatile LLAPRRootPool::sCount;

apr_thread_mutex_t* gLogMutexp;
apr_thread_mutex_t* gCallStacksLogMutexp;

LLAPRRootPool::LLAPRRootPool(void) : LLAPRInitialization(), LLAPRPool(0)
{
	// sCountInitialized don't need locking because when we get here there is still only a single thread.
	if (!sCountInitialized)
	{
		// Initialize the logging mutex
		apr_thread_mutex_create(&gLogMutexp, APR_THREAD_MUTEX_UNNESTED, mPool);
		apr_thread_mutex_create(&gCallStacksLogMutexp, APR_THREAD_MUTEX_UNNESTED, mPool);

		apr_status_t status = apr_atomic_init(mPool);
		llassert_always(status == APR_SUCCESS);
		apr_atomic_set32(&sCount, 1);	// Set to 1 to account for the global root pool.
		sCountInitialized = true;

		// Initialize thread-local APR pool support.
		// Because this recursively calls LLAPRRootPool::LLAPRRootPool(void)
		// it must be done last, so that sCount is already initialized.
		LLThreadLocalData::init();
	}
	apr_atomic_inc32(&sCount);
}

LLAPRRootPool::~LLAPRRootPool()
{
	if (!apr_atomic_dec32(&sCount))
	{
		// The last pool was destructed. Cleanup remainder of APR.
		LL_INFOS("APR") << "Cleaning up APR" << LL_ENDL;

		if (gLogMutexp)
		{
			// Clean up the logging mutex

			// All other threads NEED to be done before we clean up APR, so this is okay.
			apr_thread_mutex_destroy(gLogMutexp);
			gLogMutexp = NULL;
		}
		if (gCallStacksLogMutexp)
		{
			// Clean up the logging mutex

			// All other threads NEED to be done before we clean up APR, so this is okay.
			apr_thread_mutex_destroy(gCallStacksLogMutexp);
			gCallStacksLogMutexp = NULL;
		}

		// Must destroy ALL, and therefore this last LLAPRRootPool, before terminating APR.
		static_cast<LLAPRRootPool*>(this)->destroy();

		apr_terminate();
	}
}

//static
// Return a global root pool that is independent of LLThreadLocalData.
// Normally you should NOT use this. Only use for early initialization
// (before main) and deinitialization (after main).
LLAPRRootPool& LLAPRRootPool::get(void)
{
  static LLAPRRootPool global_APRpool(0);
  return global_APRpool;
}

void LLVolatileAPRPool::clearVolatileAPRPool()
{
	llassert_always(mNumActiveRef > 0);
	if (--mNumActiveRef == 0)
	{
		if (isOld())
		{
			destroy();
			mNumTotalRef = 0 ;
		}
		else
		{
			// This does not actually free the memory,
			// it just allows the pool to re-use this memory for the next allocation.
			clear();
		}
	}

	// Paranoia check if the pool is jammed.
	llassert(mNumTotalRef < (FULL_VOLATILE_APR_POOL << 2)) ;
}

/**
 * @file llcommon.cpp
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llcommon.h"

#include "llmemory.h"
#include "llthread.h"
#include "lltrace.h"
#include "lltracethreadrecorder.h"
#include "llcleanup.h"

thread_local bool gProfilerEnabled = false;

#if (TRACY_ENABLE)
// Override new/delete for tracy memory profiling

void* ll_tracy_new(size_t size)
{
    void* ptr;
    if (gProfilerEnabled)
    {
        //LL_PROFILE_ZONE_SCOPED_CATEGORY_MEMORY;
        ptr = (malloc)(size);
    }
    else
    {
        ptr = (malloc)(size);
    }
    if (!ptr)
    {
        throw std::bad_alloc();
    }
    LL_PROFILE_ALLOC(ptr, size);
    return ptr;
}

void* operator new(size_t size)
{
    return ll_tracy_new(size);
}

void* operator new[](std::size_t count)
{
    return ll_tracy_new(count);
}

void ll_tracy_delete(void* ptr)
{
    LL_PROFILE_FREE(ptr);
    if (gProfilerEnabled)
    {
        //LL_PROFILE_ZONE_SCOPED_CATEGORY_MEMORY;
        (free)(ptr);
    }
    else
    {
        (free)(ptr);
    }
}

void operator delete(void *ptr) noexcept
{
    ll_tracy_delete(ptr);
}

void operator delete[](void* ptr) noexcept
{
    ll_tracy_delete(ptr);
}

// C-style malloc/free can't be so easily overridden, so we define tracy versions and use
// a pre-processor #define in linden_common.h to redirect to them. The parens around the native
// functions below prevents recursive substitution by the preprocessor.
//
// Unaligned mallocs are rare in LL code but hooking them causes problems in 3p lib code (looking at
// you, Havok), so we'll only capture the aligned version.

void *tracy_aligned_malloc(size_t size, size_t alignment)
{
    auto ptr = ll_aligned_malloc_fallback(size, alignment);
    if (ptr) LL_PROFILE_ALLOC(ptr, size);
    return ptr;
}

void tracy_aligned_free(void *memblock)
{
    LL_PROFILE_FREE(memblock);
    ll_aligned_free_fallback(memblock);
}

#endif

//static
bool LLCommon::sAprInitialized = false;

static LLTrace::ThreadRecorder* sMasterThreadRecorder = NULL;

//static
void LLCommon::initClass()
{
    if (!sAprInitialized)
    {
        ll_init_apr();
        sAprInitialized = true;
    }
    LLTimer::initClass();
    assert_main_thread();       // Make sure we record the main thread
    if (!sMasterThreadRecorder)
    {
        sMasterThreadRecorder = new LLTrace::ThreadRecorder();
        LLTrace::set_master_thread_recorder(sMasterThreadRecorder);
    }
}

//static
void LLCommon::cleanupClass()
{
    delete sMasterThreadRecorder;
    sMasterThreadRecorder = NULL;
    LLTrace::set_master_thread_recorder(NULL);
    SUBSYSTEM_CLEANUP_DBG(LLTimer);
    if (sAprInitialized)
    {
        ll_cleanup_apr();
        sAprInitialized = false;
    }
}

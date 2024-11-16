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

#if LL_PROFILER_CONFIGURATION > 1 && TRACY_ENABLE
// Override new/delete for tracy memory profiling

void* ll_tracy_new(size_t size)
{
    void* ptr = (malloc)(size);
    if (!ptr)
    {
        throw std::bad_alloc();
    }
    TracyAlloc(ptr, size);
    return ptr;
}

void* ll_tracy_aligned_new(size_t size, size_t alignment)
{
    void* ptr = ll_aligned_malloc_fallback(size, alignment);
    if (!ptr)
    {
        throw std::bad_alloc();
    }
    TracyAlloc(ptr, size);
    return ptr;
}

void ll_tracy_delete(void* ptr)
{
    TracyFree(ptr);
    (free)(ptr);
}

void ll_tracy_aligned_delete(void* ptr)
{
    TracyFree(ptr);
    ll_aligned_free_fallback(ptr);
}

void* operator new(size_t size)
{
    return ll_tracy_new(size);
}

void* operator new[](std::size_t count)
{
    return ll_tracy_new(count);
}

void* operator new(size_t size, std::align_val_t align)
{
    return ll_tracy_aligned_new(size, (size_t)align);
}

void* operator new[](std::size_t count, std::align_val_t align)
{
    return ll_tracy_aligned_new(count, (size_t)align);
}

void operator delete(void *ptr) noexcept
{
    ll_tracy_delete(ptr);
}

void operator delete[](void* ptr) noexcept
{
    ll_tracy_delete(ptr);
}

void operator delete(void *ptr, std::align_val_t align) noexcept
{
    ll_tracy_aligned_delete(ptr);
}

void operator delete[](void* ptr, std::align_val_t align) noexcept
{
    ll_tracy_aligned_delete(ptr);
}

#endif // TRACY_ENABLE && !LL_PROFILER_ENABLE_TRACY_OPENGL

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

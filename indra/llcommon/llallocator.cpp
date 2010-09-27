/** 
 * @file llallocator.cpp
 * @brief Implementation of the LLAllocator class.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009-2009, Linden Research, Inc.
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

#include "linden_common.h"
#include "llallocator.h"

#if LL_USE_TCMALLOC

#include "google/heap-profiler.h"
#include "google/commandlineflags_public.h"

DECLARE_bool(heap_profile_use_stack_trace);
//DECLARE_double(tcmalloc_release_rate);

// static
void LLAllocator::pushMemType(S32 type)
{
    if(isProfiling())
    {
    	PushMemType(type);
    }
}

// static
S32 LLAllocator::popMemType()
{
    if (isProfiling())
    {
    	return PopMemType();
    }
    else
    {
        return -1;
    }
}

void LLAllocator::setProfilingEnabled(bool should_enable)
{
    // NULL disables dumping to disk
    static char const * const PREFIX = NULL;
    if(should_enable)
    {
		HeapProfilerSetUseStackTrace(false);
        HeapProfilerStart(PREFIX);
    }
    else
    {
        HeapProfilerStop();
    }
}

// static
bool LLAllocator::isProfiling()
{
    return IsHeapProfilerRunning();
}

std::string LLAllocator::getRawProfile()
{
    // *TODO - fix google-perftools to accept an buffer to avoid this
    // malloc-copy-free cycle.
    char * buffer = GetHeapProfile();
    std::string ret = buffer;
    free(buffer);
    return ret;
}

#else // LL_USE_TCMALLOC

//
// stub implementations for when tcmalloc is disabled
//

// static
void LLAllocator::pushMemType(S32 type)
{
}

// static
S32 LLAllocator::popMemType()
{
    return -1;
}

void LLAllocator::setProfilingEnabled(bool should_enable)
{
}

// static
bool LLAllocator::isProfiling()
{
    return false;
}

std::string LLAllocator::getRawProfile()
{
    return std::string();
}

#endif // LL_USE_TCMALLOC

LLAllocatorHeapProfile const & LLAllocator::getProfile()
{
    mProf.mLines.clear();

    // *TODO - avoid making all these extra copies of things...
    std::string prof_text = getRawProfile();
    //std::cout << prof_text << std::endl;
    mProf.parse(prof_text);
    return mProf;
}

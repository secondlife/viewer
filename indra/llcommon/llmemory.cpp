/**
 * @file llmemory.cpp
 * @brief Very special memory allocation/deallocation stuff here
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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


#include "llthread.h"

#if defined(LL_WINDOWS)
# include <psapi.h>
#elif defined(LL_DARWIN)
# include <sys/types.h>
# include <mach/task.h>
# include <mach/mach_init.h>
#include <mach/mach_host.h>
#elif LL_LINUX
# include <unistd.h>
# include <sys/resource.h>
# include <sys/sysinfo.h>
#endif

#include "llmemory.h"

#include "llsys.h"
#include "llframetimer.h"
#include "lltrace.h"
#include "llerror.h"
//----------------------------------------------------------------------------

//static

// most important memory metric for texture streaming
//  On Windows, this should agree with resource monitor -> performance -> memory -> available
//  On OS X, this should be activity monitor -> memory -> (physical memory - memory used)
// NOTE: this number MAY be less than the actual available memory on systems with more than MaxHeapSize64 GB of physical memory (default 16GB)
//  In that case, should report min(available, sMaxHeapSizeInKB-sAllocateMemInKB)
U32Kilobytes LLMemory::sAvailPhysicalMemInKB(U32_MAX);

// Installed physical memory
U32Kilobytes LLMemory::sMaxPhysicalMemInKB(0);

// Maximimum heap size according to the user's settings (default 16GB)
U32Kilobytes LLMemory::sMaxHeapSizeInKB(U32_MAX);

// Current memory usage
U32Kilobytes LLMemory::sAllocatedMemInKB(0);

U32Kilobytes LLMemory::sAllocatedPageSizeInKB(0);


static LLTrace::SampleStatHandle<F64Megabytes> sAllocatedMem("allocated_mem", "active memory in use by application");
static LLTrace::SampleStatHandle<F64Megabytes> sVirtualMem("virtual_mem", "virtual memory assigned to application");

void ll_assert_aligned_func(uintptr_t ptr,U32 alignment)
{
#if defined(LL_WINDOWS) && defined(LL_DEBUG_BUFFER_OVERRUN)
    //do not check
    return;
#else
    #ifdef SHOW_ASSERT
        // Redundant, place to set breakpoints.
        if (ptr%alignment!=0)
        {
            LL_WARNS() << "alignment check failed" << LL_ENDL;
        }
        llassert(ptr%alignment==0);
    #endif
#endif
}

//static
void LLMemory::initMaxHeapSizeGB(F32Gigabytes max_heap_size)
{
    sMaxHeapSizeInKB = U32Kilobytes::convert(max_heap_size);
}

//static
void LLMemory::updateMemoryInfo()
{
    LL_PROFILE_ZONE_SCOPED;

    sMaxPhysicalMemInKB = gSysMemory.getPhysicalMemoryKB();

    U32Kilobytes avail_mem;
    LLMemoryInfo::getAvailableMemoryKB(avail_mem);
    sAvailPhysicalMemInKB = avail_mem;

#if LL_WINDOWS
    PROCESS_MEMORY_COUNTERS counters;

    if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
    {
        LL_WARNS() << "GetProcessMemoryInfo failed" << LL_ENDL;
        return ;
    }

    sAllocatedMemInKB = U32Kilobytes::convert(U64Bytes(counters.WorkingSetSize));
    sAllocatedPageSizeInKB = U32Kilobytes::convert(U64Bytes(counters.PagefileUsage));
    sample(sVirtualMem, sAllocatedPageSizeInKB);

#elif defined(LL_DARWIN)
    task_vm_info info;
    mach_msg_type_number_t  infoCount = TASK_VM_INFO_COUNT;
    // MACH_TASK_BASIC_INFO reports the same resident_size, but does not tell us the reusable bytes or phys_footprint.
    if (task_info(mach_task_self(), TASK_VM_INFO, reinterpret_cast<task_info_t>(&info), &infoCount) == KERN_SUCCESS)
    {
        // Our Windows definition of PagefileUsage is documented by Microsoft as "the total amount of
        // memory that the memory manager has committed for a running process", which is rss.
        sAllocatedPageSizeInKB = U32Kilobytes::convert(U64Bytes(info.resident_size));

        // Activity Monitor => Inspect Process => Real Memory Size appears to report resident_size
        // Activity monitor => main window memory column appears to report phys_footprint, which spot checks as at least 30% less.
        //        I think that is because of compression, which isn't going to give us a consistent measurement. We want uncompressed totals.
        //
        // In between is resident_size - reusable. This is what Chrome source code uses, with source comments saying it is 'the "Real Memory" value
        // reported for the app by the Memory Monitor in Instruments.' It is still about 8% bigger than phys_footprint.
        //
        // (On Windows, we use WorkingSetSize.)
        sAllocatedMemInKB = U32Kilobytes::convert(U64Bytes(info.resident_size - info.reusable));
     }
    else
    {
        LL_WARNS() << "task_info failed" << LL_ENDL;
    }
#elif defined(LL_LINUX)
    // Use sysinfo() to get the total physical memory.
    struct sysinfo info;
    sysinfo(&info);
    sAllocatedMemInKB = U32Kilobytes::convert(U64Bytes(LLMemory::getCurrentRSS())); // represents the RAM allocated by this process only (in line with the windows implementation)
#else
    //not valid for other systems for now.
    LL_WARNS() << "LLMemory::updateMemoryInfo() not implemented for this platform." << LL_ENDL;
    sAllocatedMemInKB = U64Bytes(LLMemory::getCurrentRSS());
#endif
    sample(sAllocatedMem, sAllocatedMemInKB);

    sAvailPhysicalMemInKB = llmin(sAvailPhysicalMemInKB, sMaxHeapSizeInKB - sAllocatedMemInKB);

    return ;
}

//
//this function is to test if there is enough space with the size in the virtual address space.
//it does not do any real allocation
//if success, it returns the address where the memory chunk can fit in;
//otherwise it returns NULL.
//
//static
void* LLMemory::tryToAlloc(void* address, U32 size)
{
#if LL_WINDOWS
    address = VirtualAlloc(address, size, MEM_RESERVE | MEM_TOP_DOWN, PAGE_NOACCESS) ;
    if(address)
    {
        if(!VirtualFree(address, 0, MEM_RELEASE))
        {
            LL_ERRS() << "error happens when free some memory reservation." << LL_ENDL ;
        }
    }
    return address ;
#else
    return (void*)0x01 ; //skip checking
#endif
}

//static
void LLMemory::logMemoryInfo(bool update)
{
    LL_PROFILE_ZONE_SCOPED;
    if(update)
    {
        updateMemoryInfo() ;
    }

    LL_INFOS() << llformat("Current allocated physical memory: %.2f MB", sAllocatedMemInKB / 1024.0) << LL_ENDL;
    LL_INFOS() << llformat("Current allocated page size: %.2f MB", sAllocatedPageSizeInKB / 1024.0) << LL_ENDL;
    LL_INFOS() << llformat("Current available physical memory: %.2f MB", sAvailPhysicalMemInKB / 1024.0) << LL_ENDL;
    LL_INFOS() << llformat("Current max usable memory: %.2f MB", sMaxPhysicalMemInKB / 1024.0) << LL_ENDL;
}

//static
U32Kilobytes LLMemory::getAvailableMemKB()
{
    return sAvailPhysicalMemInKB ;
}

//static
U32Kilobytes LLMemory::getMaxMemKB()
{
    return sMaxPhysicalMemInKB ;
}

//static
U32Kilobytes LLMemory::getAllocatedMemKB()
{
    return sAllocatedMemInKB ;
}

//----------------------------------------------------------------------------

#if defined(LL_WINDOWS)

//static
U64 LLMemory::getCurrentRSS()
{
    PROCESS_MEMORY_COUNTERS counters;

    if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
    {
        LL_WARNS() << "GetProcessMemoryInfo failed" << LL_ENDL;
        return 0;
    }

    return counters.WorkingSetSize;
}

#elif defined(LL_DARWIN)

//  if (sysctl(ctl, 2, &page_size, &size, NULL, 0) == -1)
//  {
//      LL_WARNS() << "Couldn't get page size" << LL_ENDL;
//      return 0;
//  } else {
//      return page_size;
//  }
// }

U64 LLMemory::getCurrentRSS()
{
    U64 residentSize = 0;
    mach_task_basic_info_data_t basicInfo;
    mach_msg_type_number_t  basicInfoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&basicInfo, &basicInfoCount) == KERN_SUCCESS)
    {
        residentSize = basicInfo.resident_size;
        // 64-bit macos apps allocate 32 GB or more at startup, and this is reflected in virtual_size.
        // basicInfo.virtual_size is not what we want.
    }
    else
    {
        LL_WARNS() << "task_info failed" << LL_ENDL;
    }

    return residentSize;
}

#elif defined(LL_LINUX)

U64 LLMemory::getCurrentRSS()
{
    struct rusage usage;

    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        // Error handling code could be here
        return 0;
    }

    // ru_maxrss (since Linux 2.6.32)
    // This is the maximum resident set size used (in kilobytes).
    return usage.ru_maxrss * 1024;
}

#else

U64 LLMemory::getCurrentRSS()
{
    return 0;
}

#endif

//--------------------------------------------------------------------

#if defined(LL_WINDOWS) && defined(LL_DEBUG_BUFFER_OVERRUN)

#include <map>

struct mem_info {
    std::map<void*, void*> memory_info;
    LLMutex mutex;

    static mem_info& get() {
        static mem_info instance;
        return instance;
    }

private:
    mem_info(){}
};

void* ll_aligned_malloc_fallback( size_t size, int align )
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    unsigned int for_alloc = (size/sysinfo.dwPageSize + !!(size%sysinfo.dwPageSize)) * sysinfo.dwPageSize;

    void *p = VirtualAlloc(NULL, for_alloc+sysinfo.dwPageSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if(NULL == p) {
        // call debugger
        __asm int 3;
    }
    DWORD old;
    bool Res = VirtualProtect((void*)((char*)p + for_alloc), sysinfo.dwPageSize, PAGE_NOACCESS, &old);
    if(false == Res) {
        // call debugger
        __asm int 3;
    }

    void* ret = (void*)((char*)p + for_alloc-size);

    {
        LLMutexLock lock(&mem_info::get().mutex);
        mem_info::get().memory_info.insert(std::pair<void*, void*>(ret, p));
    }


    return ret;
}

void ll_aligned_free_fallback( void* ptr )
{
    LLMutexLock lock(&mem_info::get().mutex);
    VirtualFree(mem_info::get().memory_info.find(ptr)->second, 0, MEM_RELEASE);
    mem_info::get().memory_info.erase(ptr);
}

#endif

/** 
 * @file llprocessor.h
 * @brief Code to figure out the processor. Originally by Benjamin Jurke.
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


#ifndef LLPROCESSOR_H
#define LLPROCESSOR_H
#include "llunits.h"

#if LL_MSVC && _M_X64
#      define LL_X86_64 1
#      define LL_X86 1
#elif LL_MSVC && _M_IX86
#      define LL_X86 1
#elif LL_GNUC && ( defined(__amd64__) || defined(__x86_64__) )
#      define LL_X86_64 1
#      define LL_X86 1
#elif LL_GNUC && ( defined(__i386__) )
#      define LL_X86 1
#elif LL_GNUC && ( defined(__powerpc__) || defined(__ppc__) )
#      define LL_PPC 1
#endif

class LLProcessorInfoImpl;

class LL_COMMON_API LLProcessorInfo
{
public:
    LLProcessorInfo(); 
    ~LLProcessorInfo();

    F64MegahertzImplicit getCPUFrequency() const;
    bool hasSSE() const;
    bool hasSSE2() const;
    bool hasSSE3() const;
    bool hasSSE3S() const;
    bool hasSSE41() const;
    bool hasSSE42() const;
    bool hasSSE4a() const;
    bool hasAltivec() const;
    std::string getCPUFamilyName() const;
    std::string getCPUBrandName() const;
    std::string getCPUFeatureDescription() const;
private:
    LLProcessorInfoImpl* mImpl;
};

#endif // LLPROCESSOR_H

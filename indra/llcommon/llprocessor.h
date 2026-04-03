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
#include "llpreprocessor.h"

class LLProcessorInfoImpl;

class LL_COMMON_API LLProcessorInfo
{
public:
    LLProcessorInfo();
    ~LLProcessorInfo() = default;

    [[nodiscard]] F64MegahertzImplicit getCPUFrequency() const;
    [[nodiscard]] bool hasSSE() const;
    [[nodiscard]] bool hasSSE2() const;
    [[nodiscard]] bool hasSSE3() const;
    [[nodiscard]] bool hasSSE3S() const;
    [[nodiscard]] bool hasSSE41() const;
    [[nodiscard]] bool hasSSE42() const;
    [[nodiscard]] bool hasSSE4a() const;
    [[nodiscard]] bool hasAltivec() const;
    [[nodiscard]] std::string getCPUFamilyName() const;
    [[nodiscard]] std::string getCPUBrandName() const;
    [[nodiscard]] std::string getCPUFeatureDescription() const;
private:
    LLProcessorInfoImpl* mImpl;
};

#endif // LLPROCESSOR_H

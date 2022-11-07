/** 
 * @file llprocessor_test.cpp
 * @date 2010-06-01
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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
#include "../test/lltut.h"

#include "../llprocessor.h"


namespace tut
{
    struct processor
    {
    };

    typedef test_group<processor> processor_t;
    typedef processor_t::object processor_object_t;
    tut::processor_t tut_processor("LLProcessor");

    template<> template<>
    void processor_object_t::test<1>()
    {
        set_test_name("LLProcessorInfo regression test");

        LLProcessorInfo pi;
        F64 freq =  pi.getCPUFrequency();
        //bool sse =  pi.hasSSE();
        //bool sse2 = pi.hasSSE2();
        //bool alitvec = pi.hasAltivec();
        std::string family = pi.getCPUFamilyName();
        std::string brand =  pi.getCPUBrandName();
        //std::string steam =  pi.getCPUFeatureDescription();

        ensure_not_equals("Unknown Brand name", brand, "Unknown"); 
        ensure_not_equals("Unknown Family name", family, "Unknown"); 
        ensure("Reasonable CPU Frequency > 100 && < 10000", freq > 100 && freq < 10000);
    }
}

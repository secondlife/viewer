/**
 * @file lldxhardware.h
 * @brief LLDXHardware definition
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLDXHARDWARE_H
#define LL_LLDXHARDWARE_H

#include <map>

#include "stdtypes.h"
#include "llsd.h"


class LLDXHardware
{
public:
    LLDXHardware();

    void cleanup();

    // WMI can return multiple GPU drivers
    // specify which one to output
    typedef enum {
        GPU_INTEL,
        GPU_NVIDIA,
        GPU_AMD,
        GPU_ANY
    } EGPUVendor;
    std::string getDriverVersionWMI(EGPUVendor vendor);

    LLSD getDisplayInfo();
};

extern LLDXHardware gDXHardware;

#endif // LL_LLDXHARDWARE_H

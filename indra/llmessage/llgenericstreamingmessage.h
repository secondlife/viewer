/**
 * @file llgenericstreamingmessage.h
 * @brief Generic Streaming Message helpers.  Shared between viewer and simulator.
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

#pragma once

#include <string>
#include "stdtypes.h"

class LLMessageSystem;

class LLGenericStreamingMessage
{
public:
    enum Method : U16
    {
        METHOD_GLTF_MATERIAL_OVERRIDE = 0x4175,
        METHOD_UNKNOWN = 0xFFFF,
    };

    void send(LLMessageSystem* msg);
    void unpack(LLMessageSystem* msg);

    Method mMethod = METHOD_UNKNOWN;
    std::string mData;
};



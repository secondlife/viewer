/** 
 * @file llsdjson.cpp
 * @brief LLSD flexible data system
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2015, Linden Research, Inc.
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

// Must turn on conditional declarations in header file so definitions end up
// with proper linkage.
#define LLSD_DEBUG_INFO
#include "linden_common.h"

#include "llsdjson.h"

#include "llerror.h"
#include "../llmath/llmath.h"

//=========================================================================
LLSD LlsdFromJson(const Json::Value &val)
{
    LLSD result;

    switch (val.type())
    {
    default:
    case Json::nullValue:
        break;
    case Json::intValue:
        result = LLSD(static_cast<LLSD::Integer>(val.asInt()));
        break;
    case Json::uintValue:
        result = LLSD(static_cast<LLSD::Integer>(val.asUInt()));
        break;
    case Json::realValue:
        result = LLSD(static_cast<LLSD::Real>(val.asDouble()));
        break;
    case Json::stringValue:
        result = LLSD(static_cast<LLSD::String>(val.asString()));
        break;
    case Json::booleanValue:
        result = LLSD(static_cast<LLSD::Boolean>(val.asBool()));
        break;
    case Json::arrayValue:
        result = LLSD::emptyArray();
        for (Json::ValueConstIterator it = val.begin(); it != val.end(); ++it)
        {
            result.append(LlsdFromJson((*it)));
        }
        break;
    case Json::objectValue:
        result = LLSD::emptyMap();
        for (Json::ValueConstIterator it = val.begin(); it != val.end(); ++it)
        {
            result[it.memberName()] = LlsdFromJson((*it));
        }
        break;
    }
    return result;
}

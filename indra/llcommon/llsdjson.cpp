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

#include "llsdutil.h"
#include "llerror.h"
#include "../llmath/llmath.h"

#include <boost/json/src.hpp>

//=========================================================================
LLSD LlsdFromJson(const boost::json::value& val)
{
    LLSD result;

    switch (val.kind())
    {
    default:
    case boost::json::kind::null:
        break;
    case boost::json::kind::int64:
    case boost::json::kind::uint64:
        result = LLSD(val.to_number<int64_t>());
        break;
    case boost::json::kind::double_:
        result = LLSD(val.to_number<double>());
        break;
    case boost::json::kind::string:
        result = LLSD(boost::json::value_to<std::string>(val));
        break;
    case boost::json::kind::bool_:
        result = LLSD(val.as_bool());
        break;
    case boost::json::kind::array:
    {
        result = LLSD::emptyArray();
        const boost::json::array& array = val.as_array();
        size_t size = array.size();
        // allocate elements 0 .. (size() - 1) to avoid incremental allocation
        if (! array.empty())
        {
            result[size - 1] = LLSD();
        }
        for (size_t i = 0; i < size; i++)
        {
            result[i] = (LlsdFromJson(array[i]));
        }
        break;
    }
    case boost::json::kind::object:
        result = LLSD::emptyMap();
        for (const auto& element : val.as_object())
        {
            result[element.key()] = LlsdFromJson(element.value());
        }
        break;
    }
    return result;
}

//=========================================================================
boost::json::value LlsdToJson(const LLSD &val)
{
    boost::json::value result;

    switch (val.type())
    {
    case LLSD::TypeUndefined:
        result = nullptr;
        break;
    case LLSD::TypeBoolean:
        result = val.asBoolean();
        break;
    case LLSD::TypeInteger:
        result = val.asInteger();
        break;
    case LLSD::TypeReal:
        result = val.asReal();
        break;
    case LLSD::TypeURI:
    case LLSD::TypeDate:
    case LLSD::TypeUUID:
    case LLSD::TypeString:
        result = val.asString();
        break;
    case LLSD::TypeMap:
    {
        boost::json::object& obj = result.emplace_object();
        obj.reserve(val.size());
        for (const auto& llsd_dat : llsd::inMap(val))
        {
            obj[llsd_dat.first] = LlsdToJson(llsd_dat.second);
        }
        break;
    }
    case LLSD::TypeArray:
    {
        boost::json::array& json_array = result.emplace_array();
        json_array.reserve(val.size());
        for (const auto& llsd_dat : llsd::inArray(val))
        {
            json_array.push_back(LlsdToJson(llsd_dat));
        }
        break;
    }
    case LLSD::TypeBinary:
    default:
        LL_ERRS("LlsdToJson") << "Unsupported conversion to JSON from LLSD type ("
                              << val.type() << ")." << LL_ENDL;
        break;
    }

    return result;
}

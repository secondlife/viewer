/**
 * @file lljsbridge.cpp
 * @brief See lljsbridge.h.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only
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

#include "llviewerprecompiledheaders.h"

#include "lljsbridge.h"

#include "llsdjson.h"

#include <boost/json.hpp>

LLJSBridge::LLJSBridge()
{
    // Demo/parity handler, not a real Viewer feature: matches llcefbrowser's own
    // example apps' AppJavaScriptBridge::OnQuery exactly (same "op":"add" request
    // shape, same {"sum": ...} response shape), so the existing
    // js-bridge-test.html page's "Send JSON add" button succeeds identically
    // whether it's pointed at llcefbrowser's own example or at the Viewer.
    registerHandler("add", [](const LLSD& request) -> LLSD
    {
        if (!request.has("a") || !request.has("b")) return LLSD();
        LLSD response;
        response["sum"] = request["a"].asReal() + request["b"].asReal();
        return response;
    });
}

void LLJSBridge::registerHandler(const std::string& op, Handler handler)
{
    mHandlers[op] = std::move(handler);
}

LLJSBridge::Result LLJSBridge::dispatch(const std::string& request)
{
    // Plumbing sanity check, independent of the "op" mechanism entirely -- see
    // this method's own comment in the header.
    if (request == "ping")
    {
        return { true, "pong" };
    }

    boost::system::error_code ec;
    const boost::json::value parsed = boost::json::parse(request, ec);
    if (ec.failed() || !parsed.is_object())
    {
        return { false, "request is not a JSON object" };
    }

    const LLSD request_llsd = LlsdFromJson(parsed);
    const std::string op = request_llsd["op"].asString();
    if (op.empty())
    {
        return { false, "no \"op\" field in request" };
    }

    const auto it = mHandlers.find(op);
    if (it == mHandlers.end())
    {
        return { false, "no handler registered for op \"" + op + "\"" };
    }

    const LLSD response_llsd = it->second(request_llsd);
    if (response_llsd.isUndefined())
    {
        return { false, "op \"" + op + "\" failed" };
    }

    return { true, boost::json::serialize(LlsdToJson(response_llsd)) };
}

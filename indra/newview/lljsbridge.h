/**
 * @file lljsbridge.h
 * @brief Dispatches window.cefQuery(...) JSON messages from embedded-browser
 *        media to registered, "op"-keyed handlers.
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

#pragma once

#include "llsingleton.h"
#include "llsd.h"

#include <functional>
#include <map>
#include <string>

// Any web page running inside embedded-browser media (a UI floater or a prim) can
// call window.cefQuery({request: ..., onSuccess: ..., onFailure: ...}) to reach
// native Viewer code -- see LLViewerMediaImpl::updateEmbeddedBrowserEvents()'s
// JSQuery case, which is this class's only caller today. The general contract:
// request is a JSON object with a top-level "op" string field naming which
// registered handler should receive it; the handler's own LLSD return value is
// serialized back as the JSON response handed to the page's onSuccess.
//
// This class itself has no opinion about what any particular op means -- it's
// pure dispatch plumbing. Any Viewer subsystem that wants to expose itself to page
// JS registers its own handler here, independently of this file.
class LLJSBridge : public LLSingleton<LLJSBridge>
{
    LLSINGLETON(LLJSBridge);
    ~LLJSBridge() override = default;

public:
    // request is the full parsed JSON object (including "op" itself, in case a
    // handler wants it -- e.g. one handler registered under multiple op names).
    // Return the LLSD to serialize back as the JSON response on success. Return an
    // undefined LLSD (the default-constructed LLSD()) to signal failure -- the
    // page's onFailure receives a generic error message in that case.
    using Handler = std::function<LLSD(const LLSD& request)>;

    // Last registration for a given op wins, matching this project's other
    // registration-style APIs (e.g. LLUICtrl::CommitCallbackRegistry). Not expected
    // to be called often or from a hot path -- once per subsystem at startup.
    void registerHandler(const std::string& op, Handler handler);

    struct Result
    {
        bool success = false;
        std::string body; // the JSON (or, for the built-in "ping" case, plain-text)
                           // response body -- the page's onSuccess argument if
                           // success, otherwise the error message for onFailure.
    };

    // Parses request as JSON, looks up "op", and calls the registered handler (if
    // any). Deliberately does NOT perform the actual respondToQuery() wire call
    // itself -- the caller already has the queryId/embedded-browser-id this needs,
    // and keeping this class free of any embedded-browser dependency keeps it
    // trivially testable/reusable on its own.
    //
    // The literal string "ping" (not JSON at all) is special-cased to always
    // succeed with body "pong", independent of any registered handler -- a
    // zero-configuration way to prove the whole plumbing (page -> producer ->
    // Viewer -> back to page) works at all, matching llcefbrowser's own example
    // apps' identical "ping"/"pong" contract (see their AppJavaScriptBridge).
    Result dispatch(const std::string& request);

private:
    std::map<std::string, Handler> mHandlers;
};

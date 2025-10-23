/**
 * @file http_fakes.h
 * @date   2025-02-18
 * @brief doctest: unit tests for HTTP test fakes
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

/**
 * @file http_fakes.h
 * @brief Deterministic fakes for llcorehttp doctests (no network/IO); see docs/testing/doctest_quickstart.md.
 */
#pragma once

#include "httpcommon.h"
#include "httpheaders.h"
#include "httphandler.h"
#include "httpresponse.h"
#include "bufferarray.h"

#include <cstdint>
#include <map>
#include <queue>
#include <string>

namespace llcorehttp_test
{

struct FakeResponse
{
    LLCore::HttpStatus status;
    LLCore::HttpHeaders::ptr_t headers;
    std::string body;
    bool follow_redirect{false};

    FakeResponse()
        : status(LLCore::HttpStatus(LLCore::HttpStatus::LLCORE, LLCore::HE_SUCCESS)),
          headers(new LLCore::HttpHeaders())
    {
    }

    static FakeResponse Redirect(const std::string& location);
    static FakeResponse ServerError(int http_code = 500);
    static FakeResponse SuccessPayload(const std::string& payload,
                                       const std::string& content_type = "application/octet-stream");

    void applyToResponse(LLCore::HttpResponse* response) const;
};

class FakeClock
{
public:
    FakeClock() : mNow(0) {}

    std::uint64_t now() const { return mNow; }
    void advance(std::uint64_t delta) { mNow += delta; }

private:
    std::uint64_t mNow;
};

class FakeBufferArray
{
public:
    FakeBufferArray();
    ~FakeBufferArray();

    LLCore::BufferArray* get() const { return mBuffer; }
    void drop();

    void assign(const std::string& data);

private:
    LLCore::BufferArray* mBuffer;
};

class FakeTransport
{
public:
    FakeTransport();

    LLCore::HttpHandle issueNoOp(const LLCore::HttpHandler::ptr_t& handler);
    LLCore::HttpHandle issueWithResponse(const LLCore::HttpHandler::ptr_t& handler,
                                         const FakeResponse& response);

    void queueResponse(LLCore::HttpHandle handle, const FakeResponse& response);
    void cancel(LLCore::HttpHandle handle);

    /// Deliver the next queued response. Returns true when work was performed.
    bool pump();

private:
    struct Pending
    {
        LLCore::HttpHandle handle;
        LLCore::HttpHandler::ptr_t handler;
    };

    struct Scheduled
    {
        FakeResponse response;
        bool cancelled{false};
        bool follow_redirect{false};
    };

    LLCore::HttpHandle nextHandle();

    std::uintptr_t mNextId;
    std::queue<Pending> mPending;
    std::map<LLCore::HttpHandle, std::queue<Scheduled>> mResponses;
};

} // namespace llcorehttp_test

/**
 * @file http_fakes.cpp
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
 * @file http_fakes.cpp
 * @brief Implementation of deterministic llcorehttp doctest fakes; see docs/testing/doctest_quickstart.md.
 */

#include "http_fakes.h"

namespace llcorehttp_test
{

FakeBufferArray::FakeBufferArray()
    : mBuffer(new LLCore::BufferArray())
{
}

FakeBufferArray::~FakeBufferArray()
{
    drop();
}

void FakeBufferArray::drop()
{
    if (mBuffer)
    {
        mBuffer->release();
        mBuffer = nullptr;
    }
}

void FakeBufferArray::assign(const std::string& data)
{
    if (!mBuffer)
    {
        mBuffer = new LLCore::BufferArray();
    }
    mBuffer->append(data.data(), data.size());
}

FakeResponse FakeResponse::Redirect(const std::string& location)
{
    FakeResponse resp;
    resp.status = LLCore::HttpStatus(302, LLCore::HE_SUCCESS);
    resp.headers->append("Location", location);
    resp.follow_redirect = true;
    return resp;
}

FakeResponse FakeResponse::ServerError(int http_code)
{
    FakeResponse resp;
    resp.status = LLCore::HttpStatus(http_code, LLCore::HE_REPLY_ERROR);
    return resp;
}

FakeResponse FakeResponse::SuccessPayload(const std::string& payload,
                                          const std::string& content_type)
{
    FakeResponse resp;
    resp.status = LLCore::HttpStatus(200, LLCore::HE_SUCCESS);
    resp.body = payload;
    if (!content_type.empty())
    {
        resp.headers->append("Content-Type", content_type);
    }
    return resp;
}

void FakeResponse::applyToResponse(LLCore::HttpResponse* response) const
{
    response->setStatus(status);
    if (headers)
    {
        auto copy = headers;
        response->setHeaders(copy);
    }
    if (!body.empty())
    {
        FakeBufferArray buffer;
        buffer.assign(body);
        response->setBody(buffer.get());
        buffer.drop();
    }
}

FakeTransport::FakeTransport()
    : mNextId(0)
{
}

LLCore::HttpHandle FakeTransport::issueNoOp(const LLCore::HttpHandler::ptr_t& handler)
{
    const LLCore::HttpHandle handle = nextHandle();
    queueResponse(handle, FakeResponse());
    mPending.push(Pending{handle, handler});
    return handle;
}

LLCore::HttpHandle FakeTransport::issueWithResponse(const LLCore::HttpHandler::ptr_t& handler,
                                                    const FakeResponse& response)
{
    const LLCore::HttpHandle handle = nextHandle();
    queueResponse(handle, response);
    mPending.push(Pending{handle, handler});
    return handle;
}

void FakeTransport::queueResponse(LLCore::HttpHandle handle, const FakeResponse& response)
{
    mResponses[handle].push(Scheduled{response, false, response.follow_redirect});
}

void FakeTransport::cancel(LLCore::HttpHandle handle)
{
    std::queue<Scheduled> replacement;
    replacement.push(Scheduled{FakeResponse(), true, false});
    mResponses[handle] = std::move(replacement);
}

bool FakeTransport::pump()
{
    if (mPending.empty())
    {
        return false;
    }

    const Pending pending = mPending.front();
    mPending.pop();

    Scheduled scheduled;
    auto found = mResponses.find(pending.handle);
    if (found != mResponses.end())
    {
        if (!found->second.empty())
        {
            scheduled = found->second.front();
            found->second.pop();
            if (found->second.empty())
            {
                mResponses.erase(found);
            }
        }
        else
        {
            mResponses.erase(found);
        }
    }

    LLCore::HttpResponse* http_response = new LLCore::HttpResponse();
    if (scheduled.cancelled)
    {
        http_response->setStatus(LLCore::HttpStatus(LLCore::HttpStatus::LLCORE, LLCore::HE_OP_CANCELED));
    }
    else
    {
        scheduled.response.applyToResponse(http_response);
    }

    pending.handler->onCompleted(pending.handle, http_response);
    http_response->release();
    if (scheduled.follow_redirect && mResponses.count(pending.handle))
    {
        mPending.push(Pending{pending.handle, pending.handler});
    }
    return true;
}

LLCore::HttpHandle FakeTransport::nextHandle()
{
    ++mNextId;
    return reinterpret_cast<LLCore::HttpHandle>(mNextId);
}

} // namespace llcorehttp_test


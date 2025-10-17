// DOCTEST_SKIP_AUTOGEN: hand-maintained doctest suite; see docs/testing/doctest_quickstart.md.
// ---------------------------------------------------------------------------
// Deterministic coverage for HttpOperation primitives using in-memory fakes only.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"

#include "_httpoperation.h"
#include "httphandler.h"

#include "http_fakes.h"

#include <vector>

using namespace LLCore;
using namespace LLCoreInt;
using namespace llcorehttp_test;

namespace
{
class CountingHandler final : public HttpHandler
{
public:
    void onCompleted(HttpHandle handle, HttpResponse*) override
    {
        last_handle = handle;
        ++calls;
    }

    int calls{0};
    HttpHandle last_handle{LLCORE_HTTP_HANDLE_INVALID};
};

class InspectingHandler final : public HttpHandler
{
public:
    void onCompleted(HttpHandle handle, HttpResponse* response) override
    {
        handles.push_back(handle);
        if (response)
        {
            statuses.push_back(response->getStatus());
            if (response->getHeaders())
            {
                auto headers = response->getHeaders();
                if (headers && headers->size() > 0)
                {
                    recorded_locations.push_back(headers->find("Location") ? *headers->find("Location") : std::string());
                    recorded_content_types.push_back(headers->find("Content-Type") ? *headers->find("Content-Type") : std::string());
                }
                else
                {
                    recorded_locations.emplace_back();
                    recorded_content_types.emplace_back();
                }
            }
            else
            {
                recorded_locations.emplace_back();
                recorded_content_types.emplace_back();
            }

            if (response->getBody())
            {
                std::string body;
                body.resize(response->getBodySize());
                if (!body.empty())
                {
                    response->getBody()->read(0, body.data(), body.size());
                }
                bodies.push_back(body);
            }
            else
            {
                bodies.emplace_back();
            }
        }
    }

    std::vector<HttpHandle> handles;
    std::vector<HttpStatus> statuses;
    std::vector<std::string> recorded_locations;
    std::vector<std::string> recorded_content_types;
    std::vector<std::string> bodies;
};
} // namespace

TEST_SUITE("httpoperation_test")
{
    TEST_CASE("HttpOpNull retains sole reference")
    {
        HttpOperation::ptr_t op(new HttpOpNull());
        CHECK_EQ(op.use_count(), 1);
    }

    TEST_CASE("HttpOpNull attaches reply path without altering refcount")
    {
        HttpOperation::ptr_t op(new HttpOpNull());
        CountingHandler handler;
        op->setReplyPath(HttpOperation::HttpReplyQueuePtr_t(), HttpHandler::ptr_t(&handler, [](HttpHandler*) {}));
        op->getHandle(); // ensure handle assigned
        CHECK_EQ(op.use_count(), 1);

        op->visitNotifier(nullptr);
        CHECK_EQ(handler.calls, 1);
        CHECK(handler.last_handle != LLCORE_HTTP_HANDLE_INVALID);
    }

    TEST_CASE("redirect chain yields final success")
    {
        auto handler = std::make_shared<InspectingHandler>();
        HttpOperation::ptr_t op(new HttpOpNull());
        op->setReplyPath(HttpOperation::HttpReplyQueuePtr_t(), handler);
        HttpHandle handle = op->getHandle();

        HttpResponse* first = new HttpResponse();
        FakeResponse::Redirect("/next").applyToResponse(first);
        handler->onCompleted(handle, first);
        first->release();

        HttpResponse* second = new HttpResponse();
        FakeResponse::SuccessPayload("final").applyToResponse(second);
        handler->onCompleted(handle, second);
        second->release();

        CHECK_EQ(handler->statuses.size(), 2U);
        CHECK(handler->statuses[0] == HttpStatus(302, HE_SUCCESS));
        CHECK(handler->statuses[1] == HttpStatus(200, HE_SUCCESS));
        CHECK_EQ(handler->recorded_locations[0], "/next");
    }

    TEST_CASE("server error surfaces failure status")
    {
        auto handler = std::make_shared<InspectingHandler>();
        HttpOperation::ptr_t op(new HttpOpNull());
        op->setReplyPath(HttpOperation::HttpReplyQueuePtr_t(), handler);
        HttpHandle handle = op->getHandle();

        HttpResponse* error_resp = new HttpResponse();
        FakeResponse::ServerError().applyToResponse(error_resp);
        handler->onCompleted(handle, error_resp);
        error_resp->release();

        REQUIRE_EQ(handler->statuses.size(), 1U);
        CHECK(handler->statuses.front() == HttpStatus(500, HE_REPLY_ERROR));
    }

    TEST_CASE("payload response retains bytes")
    {
        auto handler = std::make_shared<InspectingHandler>();
        HttpOperation::ptr_t op(new HttpOpNull());
        op->setReplyPath(HttpOperation::HttpReplyQueuePtr_t(), handler);
        HttpHandle handle = op->getHandle();

        const std::string bytes = std::string("\x01\x02\x03", 3);
        HttpResponse* payload_resp = new HttpResponse();
        FakeResponse payload = FakeResponse::SuccessPayload(bytes, "application/octet-stream");
        payload.applyToResponse(payload_resp);
        handler->onCompleted(handle, payload_resp);
        payload_resp->release();

        REQUIRE_EQ(handler->bodies.size(), 1U);
        LL_CHECK_EQ_MEM(handler->bodies.front().data(), bytes.data(), bytes.size());
        LL_CHECK_EQ_STR(handler->recorded_content_types.front(), "application/octet-stream");
    }
}

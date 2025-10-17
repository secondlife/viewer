// DOCTEST_SKIP_AUTOGEN: hand-maintained doctest suite; see docs/testing/doctest_quickstart.md.
// ---------------------------------------------------------------------------
// Deterministic HttpRequestQueue coverage using in-memory fakes (no sockets or threads).
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"

#include "_httprequestqueue.h"
#include "_httpoperation.h"

#include "http_fakes.h"

#include <map>
#include <string>
#include <vector>

using namespace LLCore;
using namespace llcorehttp_test;

namespace
{
class QueueFixture
{
public:
    QueueFixture()
    {
        HttpRequestQueue::init();
        queue = HttpRequestQueue::instanceOf();
    }

    ~QueueFixture()
    {
        HttpRequestQueue::term();
    }

    HttpRequestQueue* queue;
};

class RecordingHandler final : public HttpHandler
{
public:
    RecordingHandler(std::vector<std::string>& order,
                     std::vector<HttpStatus>& statuses)
        : mOrder(order), mStatuses(statuses) {}

    void registerLabel(HttpHandle handle, const std::string& label)
    {
        mLabels[handle] = label;
    }

    std::string labelFor(HttpHandle handle) const
    {
        auto it = mLabels.find(handle);
        return it != mLabels.end() ? it->second : std::string();
    }

    void onCompleted(HttpHandle handle, HttpResponse* response) override
    {
        mOrder.push_back(labelFor(handle));
        if (response)
        {
            mStatuses.push_back(response->getStatus());
        }
    }

private:
    std::vector<std::string>& mOrder;
    std::vector<HttpStatus>& mStatuses;
    std::map<HttpHandle, std::string> mLabels;
};
} // namespace

TEST_SUITE("httprequestqueue_test")
{
    TEST_CASE("fifo ordering yields matching callbacks")
    {
        QueueFixture fixture;
        std::vector<std::string> completions;
        std::vector<HttpStatus> statuses;
        auto handler = std::make_shared<RecordingHandler>(completions, statuses);

        const std::vector<std::string> labels = {"first", "second", "third"};
        for (const auto& label : labels)
        {
            HttpOperation::ptr_t op(new HttpOpNull());
            const HttpHandle handle = op->getHandle();
            handler->registerLabel(handle, label);
            op->mStatus = HttpStatus(HttpStatus::LLCORE, HE_SUCCESS);
            op->setReplyPath(HttpOperation::HttpReplyQueuePtr_t(), handler);
            fixture.queue->addOp(op);
        }

        HttpRequestQueue::OpContainer fetched;
        fixture.queue->fetchAll(false, fetched);
        CHECK_EQ(fetched.size(), labels.size());
        for (std::size_t i = 0; i < fetched.size(); ++i)
        {
            CAPTURE(i);
            CHECK_EQ(handler->labelFor(fetched[i]->getHandle()), labels[i]);
            fetched[i]->visitNotifier(nullptr);
        }

        CHECK_EQ(completions, labels);
        CHECK_EQ(statuses.size(), labels.size());
    }

    TEST_CASE("cancel reports cancelled then successful")
    {
        QueueFixture fixture;
        FakeTransport transport;
        std::vector<std::string> completions;
        std::vector<HttpStatus> statuses;
        auto handler = std::make_shared<RecordingHandler>(completions, statuses);

        const std::vector<std::string> labels = {"first", "second"};
        for (const auto& label : labels)
        {
            HttpOperation::ptr_t op(new HttpOpNull());
            handler->registerLabel(op->getHandle(), label);
            op->mStatus = HttpStatus(HttpStatus::LLCORE, HE_SUCCESS);
            op->setReplyPath(HttpOperation::HttpReplyQueuePtr_t(), handler);
            fixture.queue->addOp(op);
        }

        HttpRequestQueue::OpContainer fetched;
        fixture.queue->fetchAll(false, fetched);
        CHECK_EQ(fetched.size(), labels.size());

        std::vector<HttpHandle> scheduled;
        for (std::size_t i = 0; i < fetched.size(); ++i)
        {
            const HttpHandle handle = transport.issueWithResponse(handler, FakeResponse());
            handler->registerLabel(handle, labels[i]);
            scheduled.push_back(handle);
        }

        transport.cancel(scheduled.front());
        while (transport.pump()) {}

        CHECK_EQ(completions.size(), 2U);
        CHECK_EQ(completions[0], labels[0]);
        CHECK_EQ(completions[1], labels[1]);
        CHECK_EQ(statuses.size(), 2U);
        LL_CHECK_EQ_STR(statuses.front().toHex(), HttpStatus(HttpStatus::LLCORE, HE_OP_CANCELED).toHex());
        LL_CHECK_EQ_STR(statuses.back().toHex(), HttpStatus(HttpStatus::LLCORE, HE_SUCCESS).toHex());
    }

    TEST_CASE("retry can succeed after failure")
    {
        QueueFixture fixture;
        FakeClock clock;
        std::vector<std::string> completions;
        std::vector<HttpStatus> statuses;
        auto handler = std::make_shared<RecordingHandler>(completions, statuses);

        HttpOperation::ptr_t op(new HttpOpNull());
        handler->registerLabel(op->getHandle(), "retry");
        op->setReplyPath(HttpOperation::HttpReplyQueuePtr_t(), handler);
        fixture.queue->addOp(op);

        HttpRequestQueue::OpContainer fetched;
        fixture.queue->fetchAll(false, fetched);
        REQUIRE_EQ(fetched.size(), 1U);

        op->mStatus = HttpStatus(HttpStatus::LLCORE, HE_REPLY_ERROR);
        fetched.front()->visitNotifier(nullptr);

        CHECK_EQ(statuses.size(), 1U);
        LL_CHECK_EQ_STR(statuses.front().toHex(), HttpStatus(HttpStatus::LLCORE, HE_REPLY_ERROR).toHex());

        clock.advance(200);

        fixture.queue->addOp(op);
        fetched.clear();
        fixture.queue->fetchAll(false, fetched);
        REQUIRE_EQ(fetched.size(), 1U);

        op->mStatus = HttpStatus(HttpStatus::LLCORE, HE_SUCCESS);
        fetched.front()->visitNotifier(nullptr);

        CHECK_EQ(completions.size(), 2U);
        CHECK_EQ(completions[0], "retry");
        CHECK_EQ(completions[1], "retry");
        CHECK_EQ(statuses.size(), 2U);
        LL_CHECK_EQ_STR(statuses.back().toHex(), HttpStatus(HttpStatus::LLCORE, HE_SUCCESS).toHex());
        CHECK_GE(clock.now(), static_cast<std::uint64_t>(200));
    }

    TEST_CASE("empty queue fetch returns immediately")
    {
        QueueFixture fixture;
        CHECK_FALSE(fixture.queue->fetchOp(false));
        HttpRequestQueue::OpContainer fetched;
        fixture.queue->fetchAll(false, fetched);
        CHECK(fetched.empty());
    }
}

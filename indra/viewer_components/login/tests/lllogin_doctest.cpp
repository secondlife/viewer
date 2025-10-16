#include "linden_common.h"

#include "doctest.h"

#include "../lllogin.h"

#include "llevents.h"
#include "lleventcoro.h"
#include "llsd.h"

#include "../../../test/lltestapp.h"

#include <boost/bind.hpp>

#include <chrono>
#include <functional>
#include <sstream>
#include <string>
#include <utility>

namespace
{
class LoginListener : public LLEventTrackable
{
public:
    explicit LoginListener(std::string name)
        : mName(std::move(name))
    {
    }

    bool call(const LLSD& event)
    {
        mLastEvent = event;
        ++mCalls;
        return false;
    }

    LLBoundListener listenTo(LLEventPump& pump)
    {
        return pump.listen(mName, boost::bind(&LoginListener::call, this, _1));
    }

    const LLSD& lastEvent() const { return mLastEvent; }

    size_t getCalls() const { return mCalls; }

    template <typename Predicate>
    LLSD waitFor(const std::string& description, Predicate&& predicate, double timeout_seconds = 2.0) const
    {
        auto start = std::chrono::steady_clock::now();
        while (!predicate())
        {
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (std::chrono::duration<double>(elapsed).count() > timeout_seconds)
            {
                std::ostringstream message;
                message << description << " not observed within " << timeout_seconds << " seconds";
                FAIL_CHECK(message.str().c_str());
                break;
            }
            llcoro::suspend();
        }
        return lastEvent();
    }

    LLSD waitFor(size_t previousCalls, double timeout_seconds) const
    {
        return waitFor(
            "listener to receive new event",
            [this, previousCalls]() { return getCalls() > previousCalls; },
            timeout_seconds);
    }

private:
    std::string mName;
    mutable LLSD mLastEvent;
    mutable size_t mCalls{0};
};

class LLXMLRPCListener : public LLEventTrackable
{
public:
    LLXMLRPCListener(std::string name, bool immediate_response = false, LLSD response = LLSD())
        : mName(std::move(name)),
          mImmediateResponse(immediate_response),
          mResponse(std::move(response))
    {
        if (mResponse.isUndefined())
        {
            mResponse["status"] = "Complete";
            mResponse["errorcode"] = 0;
            mResponse["error"] = "dummy response";
            mResponse["transfer_rate"] = 0;
            mResponse["responses"]["login"] = true;
        }
    }

    void setResponse(const LLSD& response) { mResponse = response; }

    bool handleEvent(const LLSD& event)
    {
        mEvent = event;
        if (mImmediateResponse)
        {
            sendReply();
        }
        return false;
    }

    void sendReply()
    {
        LLEventPumps::instance().obtain(mEvent["reply"]).post(mResponse);
    }

    LLBoundListener listenTo(LLEventPump& pump)
    {
        return pump.listen(mName, boost::bind(&LLXMLRPCListener::handleEvent, this, _1));
    }

private:
    std::string mName;
    bool mImmediateResponse{false};
    LLSD mResponse;
    LLSD mEvent;
};

struct LoginTestEnvironment
{
    LoginTestEnvironment()
        : pumps(LLEventPumps::instance())
    {
    }

    ~LoginTestEnvironment()
    {
        pumps.clear();
    }

    LLEventPumps& pumps;
    LLTestApp testApp;
};
} // namespace

TEST_SUITE("login")
{
    TEST_CASE("connects with immediate xmlrpc response")
    {
        LoginTestEnvironment env;
        LLEventStream xmlrpcPump("LLXMLRPCTransaction");

        const bool respond_immediately = true;
        LLXMLRPCListener dummyXMLRPC("dummy_xmlrpc", respond_immediately);
        LLTempBoundListener conn1 = dummyXMLRPC.listenTo(xmlrpcPump);

        LLLogin login;

        LoginListener listener("test_ear");
        LLTempBoundListener conn2 = listener.listenTo(login.getEventPump());

        LLSD credentials;
        credentials["first"] = "foo";
        credentials["last"] = "bar";
        credentials["passwd"] = "secret";

        login.connect("login.bar.com", credentials);
        LLSD event = listener.waitFor(
            "online state",
            [&listener]() { return listener.lastEvent()["state"].asString() == "online"; });

        CHECK(event["state"].asString() == "online");
    }

    TEST_CASE("failed login transitions offline")
    {
        LoginTestEnvironment env;
        LLEventStream xmlrpcPump("LLXMLRPCTransaction");

        LLXMLRPCListener dummyXMLRPC("dummy_xmlrpc");
        LLTempBoundListener conn1 = dummyXMLRPC.listenTo(xmlrpcPump);

        LLLogin login;
        LoginListener listener("test_ear");
        LLTempBoundListener conn2 = listener.listenTo(login.getEventPump());

        LLSD credentials;
        credentials["first"] = "who";
        credentials["last"] = "what";
        credentials["passwd"] = "badpasswd";

        login.connect("login.bar.com", credentials);
        llcoro::suspend();

        CHECK(listener.lastEvent()["change"].asString() == "authenticating");

        size_t previous = listener.getCalls();

        LLSD data;
        data["status"] = "Complete";
        data["errorcode"] = 0;
        data["error"] = "dummy response";
        data["transfer_rate"] = 0;
        data["responses"]["login"] = "false";
        dummyXMLRPC.setResponse(data);
        dummyXMLRPC.sendReply();

        listener.waitFor(previous, 11.0);

        CHECK(listener.lastEvent()["state"].asString() == "offline");
    }
}

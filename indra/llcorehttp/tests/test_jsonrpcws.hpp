/**
 * @file test_jsonrpcws.hpp
 * @brief unit tests for LLJSONRPCConnection helpers and dispatch behavior
 */

#ifndef TEST_LLCORE_JSONRPCWS_H_
#define TEST_LLCORE_JSONRPCWS_H_

#include "lljsonrpcws.h"
#include "lltimer.h"

namespace
{
class TestJSONRPCConnection : public LLJSONRPCConnection
{
public:
    TestJSONRPCConnection()
        : LLJSONRPCConnection(LLWebsocketMgr::WSServer::ptr_t(), LLWebsocketMgr::connection_h())
    {
    }

    using LLJSONRPCConnection::processMessage;
    using LLJSONRPCConnection::validateMessage;
    using LLJSONRPCConnection::testInjectPendingRequest;
    using LLJSONRPCConnection::testPendingRequestCount;
    using LLJSONRPCConnection::testSweepTimeouts;
};
}

namespace tut
{
    struct JSONRPCWSTestData
    {
    };

    typedef test_group<JSONRPCWSTestData> JSONRPCWSTestGroupType;
    typedef JSONRPCWSTestGroupType::object JSONRPCWSTestObjectType;
    JSONRPCWSTestGroupType JSONRPCWSTestGroup("LLJSONRPCConnection Tests");

    template<> template<>
    void JSONRPCWSTestObjectType::test<1>()
    {
        set_test_name("makeEnvelope notification omits id");

        LLSD params;
        params["value"] = 42;
        LLSD env = LLJSONRPCConnection::makeEnvelope(LLSD(), "runtime.debug", params, LLSD(), LLSD());

        ensure("jsonrpc field should exist", env.has("jsonrpc"));
        ensure_equals("jsonrpc version", env["jsonrpc"].asString(), "2.0");
        ensure("notification should omit id", !env.has("id"));
        ensure_equals("method", env["method"].asString(), "runtime.debug");
        ensure_equals("param round trip", env["params"]["value"].asInteger(), 42);
    }

    template<> template<>
    void JSONRPCWSTestObjectType::test<2>()
    {
        set_test_name("makeEnvelope response keeps id slot");

        LLSD env = LLJSONRPCConnection::makeEnvelope(LLSD(), std::string(), LLSD(), LLSD("ok"), LLSD());

        ensure("response should include id", env.has("id"));
        ensure("response should not include method", !env.has("method"));
        ensure_equals("result", env["result"].asString(), "ok");
    }

    template<> template<>
    void JSONRPCWSTestObjectType::test<3>()
    {
        set_test_name("validateMessage accepts valid request and rejects invalid params");

        TestJSONRPCConnection conn;

        LLSD valid;
        valid["jsonrpc"] = "2.0";
        valid["method"] = "session.ping";
        LLSD params = LLSD::emptyMap();
        params["timestamp"] = 123;
        valid["params"] = params;
        ensure("valid request should pass", conn.validateMessage(valid, true));

        LLSD invalid = valid;
        invalid["params"] = "not-an-array-or-object";
        ensure("invalid params type should fail", !conn.validateMessage(invalid, true));
    }

    template<> template<>
    void JSONRPCWSTestObjectType::test<4>()
    {
        set_test_name("processMessage dispatches notification handler");

        TestJSONRPCConnection conn;

        bool called = false;
        LLSD seen_id;
        LLSD seen_params;
        conn.registerMethod("runtime.debug",
            [&](const std::string& method, const LLSD& id, const LLSD& params) -> LLSD
            {
                called = true;
                ensure_equals("method propagated", method, "runtime.debug");
                seen_id = id;
                seen_params = params;
                return LLSD();
            });

        LLSD params;
        params["message"] = "hello";
        LLSD notification = LLJSONRPCConnection::makeEnvelope(LLSD(), "runtime.debug", params, LLSD(), LLSD());
        conn.processMessage(notification);

        ensure("handler should be called", called);
        ensure("notification id should be undefined", seen_id.isUndefined());
        ensure_equals("payload should be forwarded", seen_params["message"].asString(), "hello");
    }

    template<> template<>
    void JSONRPCWSTestObjectType::test<5>()
    {
        set_test_name("sweepTimeouts expires overdue callbacks");

        TestJSONRPCConnection conn;

        bool callback_called = false;
        conn.testInjectPendingRequest(
            "req_1",
            LLTimer::getTotalSeconds() - 1.0,
            [&](const LLSD& result, const LLSD& error)
            {
                callback_called = true;
                ensure("timed out result should be undefined", result.isUndefined());
                ensure_equals(
                    "timeout code",
                    error["code"].asInteger(),
                    LLJSONRPCConnection::RPCError::REQUEST_TIMEOUT);
            });

        ensure_equals("pending request should be tracked", conn.testPendingRequestCount(), (size_t)1);
        conn.testSweepTimeouts();
        ensure("timeout callback should be called", callback_called);
        ensure_equals("pending request should be removed", conn.testPendingRequestCount(), (size_t)0);
    }
}

#endif
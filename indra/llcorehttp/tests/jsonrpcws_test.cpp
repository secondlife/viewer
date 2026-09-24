/**
 * @file jsonrpcws_test.cpp
 * @brief Security-focused unit tests for LLJSONRPCConnection / LLJSONRPCServer
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

#include "linden_common.h"

#include "lljsonrpcws.h"
#include "llsdjson.h"

#include "../test/lltut.h"

#include <boost/json.hpp>
#include <vector>

namespace
{
    // Exposes the protected test-only members LLJSONRPCConnection provides
    // for exactly this purpose, on a connection with no real transport.
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
        using LLJSONRPCConnection::setAuthenticated;
    };

    // Minimal concrete server, used only to register the built-in
    // system.* methods on a connection via setupConnectionMethods() --
    // LLJSONRPCServer itself is abstract (handleGetVersion is pure virtual).
    // Never started, so port 0 is fine.
    class TestJSONRPCServer : public LLJSONRPCServer
    {
    public:
        TestJSONRPCServer() : LLJSONRPCServer("test_jsonrpcws_server", 0, true) {}

        using LLJSONRPCServer::setupConnectionMethods;

        LLSD handleGetVersion(const LLJSONRPCConnection::ptr_t&, const LLSD&) const override
        {
            return LLSD();
        }
    };

    // Captures what would be sent to the peer, since there is no real
    // transport to observe it on.
    class CapturingJSONRPCConnection : public TestJSONRPCConnection
    {
    public:
        mutable std::vector<std::string> mSent;

        bool sendMessage(const std::string& message) const override
        {
            mSent.push_back(message);
            return true;
        }
    };
}

namespace tut
{
    struct jsonrpcws_data
    {
    };

    typedef test_group<jsonrpcws_data> jsonrpcws_test;
    typedef jsonrpcws_test::object jsonrpcws_object;
    jsonrpcws_test jsonrpcws_testcase("LLJSONRPCConnection");

    template<> template<>
    void jsonrpcws_object::test<1>()
    {
        set_test_name("an unauthenticated connection dispatches neither requests nor notifications");

        TestJSONRPCConnection conn;
        conn.setAuthenticated(false);

        int calls = 0;
        conn.registerMethod("object.list",
            [&](const std::string&, const LLSD&, const LLSD&) -> LLSD
            {
                ++calls;
                return LLSD();
            });

        const LLSD request = LLJSONRPCConnection::makeEnvelope(LLSD("req_1"), "object.list", LLSD::emptyMap(), LLSD(), LLSD());
        conn.processMessage(request);

        const LLSD notification = LLJSONRPCConnection::makeEnvelope(LLSD(), "object.list", LLSD::emptyMap(), LLSD(), LLSD());
        conn.processMessage(notification);

        ensure_equals("neither a request nor a notification reaches the handler", calls, 0);
    }

    template<> template<>
    void jsonrpcws_object::test<2>()
    {
        set_test_name("once authenticated, requests and notifications dispatch normally");

        TestJSONRPCConnection conn;
        conn.setAuthenticated(true);
        ensure("authenticated after the peer proves itself", conn.isAuthenticated());

        int calls = 0;
        conn.registerMethod("object.list",
            [&](const std::string&, const LLSD&, const LLSD&) -> LLSD
            {
                ++calls;
                return LLSD();
            });

        const LLSD request = LLJSONRPCConnection::makeEnvelope(LLSD("req_1"), "object.list", LLSD::emptyMap(), LLSD(), LLSD());
        conn.processMessage(request);
        ensure_equals("the request reaches the handler", calls, 1);

        const LLSD notification = LLJSONRPCConnection::makeEnvelope(LLSD(), "object.list", LLSD::emptyMap(), LLSD(), LLSD());
        conn.processMessage(notification);
        ensure_equals("the notification also reaches the handler", calls, 2);
    }

    template<> template<>
    void jsonrpcws_object::test<3>()
    {
        set_test_name("an unauthenticated connection still receives the answers to its own calls");

        TestJSONRPCConnection conn;
        ensure("unauthenticated by default", !conn.isAuthenticated());

        LLSD answer;
        conn.testInjectPendingRequest(
            "rpc_1",
            LLTimer::getTotalSeconds() + 60.0,
            [&](const LLSD& result, const LLSD& error)
            {
                ensure("no error", error.isUndefined());
                answer = result;
            });

        LLSD result;
        result["challenge_response"] = "secret";
        conn.processMessage(LLJSONRPCConnection::makeEnvelope(LLSD("rpc_1"), std::string(), LLSD(), result, LLSD()));
        ensure_equals("the answer reached the caller", answer["challenge_response"].asString(), std::string("secret"));
        ensure("and it is answered", conn.testPendingRequestCount() == 0);
    }

    template<> template<>
    void jsonrpcws_object::test<4>()
    {
        set_test_name("an unauthenticated request is answered -32002; a notification gets nothing back");

        CapturingJSONRPCConnection conn;
        ensure("unauthenticated by default", !conn.isAuthenticated());

        const LLSD request = LLJSONRPCConnection::makeEnvelope(LLSD("req_1"), "object.list", LLSD::emptyMap(), LLSD(), LLSD());
        conn.processMessage(request);
        ensure_equals("exactly one message sent for the request", conn.mSent.size(), (size_t)1);

        boost::system::error_code ec;
        const LLSD sent = LlsdFromJson(boost::json::parse(conn.mSent[0], ec));
        ensure("parsed without error", !ec);
        ensure_equals("unauthorized error code", sent["error"]["code"].asInteger(), LLJSONRPCConnection::RPCError::UNAUTHORIZED);

        const LLSD notification = LLJSONRPCConnection::makeEnvelope(LLSD(), "object.list", LLSD::emptyMap(), LLSD(), LLSD());
        conn.processMessage(notification);
        ensure_equals("nothing sent for a notification", conn.mSent.size(), (size_t)1);
    }

    template<> template<>
    void jsonrpcws_object::test<5>()
    {
        set_test_name("malformed JSON is answered with a parse error, and the connection survives");

        CapturingJSONRPCConnection conn;
        conn.setAuthenticated(true);

        conn.onMessage("{not valid json");
        ensure_equals("a parse error is sent", conn.mSent.size(), (size_t)1);

        boost::system::error_code ec;
        const LLSD sent = LlsdFromJson(boost::json::parse(conn.mSent[0], ec));
        ensure("parsed without error", !ec);
        ensure_equals("parse error code", sent["error"]["code"].asInteger(), LLJSONRPCConnection::RPCError::PARSE_ERROR);

        // The connection itself is unaffected: valid follow-up traffic still works.
        int calls = 0;
        conn.registerMethod("object.list",
            [&](const std::string&, const LLSD&, const LLSD&) -> LLSD
            {
                ++calls;
                return LLSD();
            });
        conn.onMessage(boost::json::serialize(LlsdToJson(LLJSONRPCConnection::makeEnvelope(LLSD("req_1"), "object.list", LLSD::emptyMap(), LLSD(), LLSD()))));
        ensure_equals("a later valid request still dispatches", calls, 1);
    }

    template<> template<>
    void jsonrpcws_object::test<6>()
    {
        set_test_name("a batch request is rejected outright; batch support is intentionally disabled");

        CapturingJSONRPCConnection conn;
        conn.setAuthenticated(true);

        const LLSD one = LLJSONRPCConnection::makeEnvelope(LLSD("req_1"), "object.list", LLSD::emptyMap(), LLSD(), LLSD());
        LLSD batch = LLSD::emptyArray();
        batch.append(one);
        batch.append(one);

        conn.onMessage(boost::json::serialize(LlsdToJson(batch)));
        ensure_equals("a single error is sent for the whole batch", conn.mSent.size(), (size_t)1);

        boost::system::error_code ec;
        const LLSD sent = LlsdFromJson(boost::json::parse(conn.mSent[0], ec));
        ensure("parsed without error", !ec);
        ensure_equals("invalid request error code", sent["error"]["code"].asInteger(), LLJSONRPCConnection::RPCError::INVALID_REQUEST);
    }

    template<> template<>
    void jsonrpcws_object::test<7>()
    {
        set_test_name("a missing or wrong jsonrpc version is rejected by validateMessage");

        TestJSONRPCConnection conn;

        LLSD missing_version;
        missing_version["method"] = "object.list";
        ensure("missing version fails validation", !conn.validateMessage(missing_version, true));

        LLSD wrong_version = missing_version;
        wrong_version["jsonrpc"] = "1.0";
        ensure("wrong version fails validation", !conn.validateMessage(wrong_version, true));

        LLSD correct_version = missing_version;
        correct_version["jsonrpc"] = "2.0";
        ensure("correct version passes validation", conn.validateMessage(correct_version, true));
    }

    template<> template<>
    void jsonrpcws_object::test<8>()
    {
        set_test_name("params of the wrong type are rejected; the handler is never invoked");

        TestJSONRPCConnection conn;
        conn.setAuthenticated(true);

        LLSD bad_params;
        bad_params["jsonrpc"] = "2.0";
        bad_params["method"] = "object.list";
        bad_params["params"] = "not-an-array-or-object";
        ensure("string params fail validation", !conn.validateMessage(bad_params, true));

        int calls = 0;
        conn.registerMethod("object.list",
            [&](const std::string&, const LLSD&, const LLSD&) -> LLSD
            {
                ++calls;
                return LLSD();
            });

        conn.processMessage(bad_params);
        ensure_equals("the handler is never invoked", calls, 0);

        LLSD good_params = bad_params;
        good_params["params"] = LLSD::emptyMap();
        conn.processMessage(good_params);
        ensure_equals("valid params do dispatch", calls, 1);
    }

    template<> template<>
    void jsonrpcws_object::test<9>()
    {
        set_test_name("a response for an unknown id is silently ignored");

        TestJSONRPCConnection conn;

        bool callback_called = false;
        conn.testInjectPendingRequest(
            "req_1",
            LLTimer::getTotalSeconds() + 60.0,
            [&](const LLSD&, const LLSD&)
            {
                callback_called = true;
            });

        LLSD result;
        result["value"] = "ok";
        conn.processMessage(LLJSONRPCConnection::makeEnvelope(LLSD("unknown_id"), std::string(), LLSD(), result, LLSD()));

        ensure("no callback fired for an unrelated id", !callback_called);
        ensure_equals("the real pending request is still tracked", conn.testPendingRequestCount(), (size_t)1);

        conn.processMessage(LLJSONRPCConnection::makeEnvelope(LLSD("req_1"), std::string(), LLSD(), result, LLSD()));
        ensure("the callback fires for its own id", callback_called);
        ensure_equals("and it is removed once answered", conn.testPendingRequestCount(), (size_t)0);
    }

    template<> template<>
    void jsonrpcws_object::test<10>()
    {
        set_test_name("a response with both result and error, or with neither, is rejected as invalid");

        TestJSONRPCConnection conn;

        LLSD both;
        both["jsonrpc"] = "2.0";
        both["id"] = "req_1";
        both["result"] = "ok";
        both["error"]["code"] = -1;
        both["error"]["message"] = "bad";
        ensure("result and error together fail validation", !conn.validateMessage(both, false));

        LLSD neither;
        neither["jsonrpc"] = "2.0";
        neither["id"] = "req_1";
        ensure("neither result nor error fails validation", !conn.validateMessage(neither, false));

        LLSD result_only;
        result_only["jsonrpc"] = "2.0";
        result_only["id"] = "req_1";
        result_only["result"] = "ok";
        ensure("result alone passes validation", conn.validateMessage(result_only, false));

        bool callback_called = false;
        conn.testInjectPendingRequest(
            "req_1",
            LLTimer::getTotalSeconds() + 60.0,
            [&](const LLSD&, const LLSD&)
            {
                callback_called = true;
            });

        conn.processMessage(both);
        ensure("an invalid response never reaches the callback", !callback_called);
        ensure_equals("the pending request is still tracked", conn.testPendingRequestCount(), (size_t)1);
    }

    template<> template<>
    void jsonrpcws_object::test<11>()
    {
        set_test_name("an unauthenticated peer is told unauthorized, never that the method doesn't exist");

        CapturingJSONRPCConnection conn;

        const LLSD request = LLJSONRPCConnection::makeEnvelope(LLSD("req_1"), "no.such.method", LLSD::emptyMap(), LLSD(), LLSD());
        conn.processMessage(request);
        ensure_equals("one message sent", conn.mSent.size(), (size_t)1);

        boost::system::error_code ec;
        LLSD sent = LlsdFromJson(boost::json::parse(conn.mSent[0], ec));
        ensure("parsed without error", !ec);
        ensure_equals("unauthorized, not method-not-found", sent["error"]["code"].asInteger(), LLJSONRPCConnection::RPCError::UNAUTHORIZED);

        conn.setAuthenticated(true);
        conn.processMessage(request);
        ensure_equals("a second message sent once authenticated", conn.mSent.size(), (size_t)2);

        sent = LlsdFromJson(boost::json::parse(conn.mSent[1], ec));
        ensure("parsed without error", !ec);
        ensure_equals("now it is method-not-found", sent["error"]["code"].asInteger(), LLJSONRPCConnection::RPCError::METHOD_NOT_FOUND);
    }

    template<> template<>
    void jsonrpcws_object::test<12>()
    {
        set_test_name("unregisterMethod removes both sync and async handlers");

        TestJSONRPCConnection conn;

        conn.registerMethod("sync.method",
            [](const std::string&, const LLSD&, const LLSD&) -> LLSD
            {
                return LLSD();
            });
        conn.registerAsyncMethod("async.method",
            [](const std::string&, const LLSD&, const LLSD&) -> LLSD
            {
                return LLSD();
            });

        ensure_equals("sync method registered", conn.getMethods().count("sync.method"), (size_t)1);
        ensure_equals("async method registered", conn.getMethods().count("async.method"), (size_t)1);

        conn.unregisterMethod("sync.method");
        conn.unregisterMethod("async.method");

        ensure_equals("sync method gone", conn.getMethods().count("sync.method"), (size_t)0);
        ensure_equals("async method gone", conn.getMethods().count("async.method"), (size_t)0);
    }

    template<> template<>
    void jsonrpcws_object::test<13>()
    {
        set_test_name("a handler throwing a plain std::exception is converted to a generic internal error");

        CapturingJSONRPCConnection conn;
        conn.setAuthenticated(true);

        conn.registerMethod("boom",
            [](const std::string&, const LLSD&, const LLSD&) -> LLSD
            {
                throw std::runtime_error("boom detail");
            });

        conn.processMessage(LLJSONRPCConnection::makeEnvelope(LLSD("req_1"), "boom", LLSD::emptyMap(), LLSD(), LLSD()));
        ensure_equals("one message sent", conn.mSent.size(), (size_t)1);

        boost::system::error_code ec;
        const LLSD sent = LlsdFromJson(boost::json::parse(conn.mSent[0], ec));
        ensure("parsed without error", !ec);
        ensure_equals("internal error code", sent["error"]["code"].asInteger(), LLJSONRPCConnection::RPCError::INTERNAL_ERROR);

        // The exception's own message must never reach the client -- it can
        // carry paths, connection strings, or other internal detail.
        ensure("the exception's own message is not forwarded to the client",
               sent["error"]["message"].asString().find("boom detail") == std::string::npos);
    }

    template<> template<>
    void jsonrpcws_object::test<14>()
    {
        set_test_name("call() with no way to send returns undefined and leaves nothing pending");

        TestJSONRPCConnection conn;

        bool callback_called = false;
        const LLSD id = conn.call("object.list", LLSD::emptyMap(),
            [&](const LLSD&, const LLSD&)
            {
                callback_called = true;
            });

        ensure("call() reports it could not send", id.isUndefined());
        ensure_equals("nothing left pending", conn.testPendingRequestCount(), (size_t)0);
        ensure("the callback is never invoked for an unsent call", !callback_called);
    }

    template<> template<>
    void jsonrpcws_object::test<15>()
    {
        set_test_name("the built-in system.* methods are gated by authentication like any other method");

        TestJSONRPCServer server;
        auto conn = std::make_shared<CapturingJSONRPCConnection>();
        server.setupConnectionMethods(conn);

        ensure_equals("system.listMethods is registered", conn->getMethods().count("system.listMethods"), (size_t)1);
        ensure_equals("system.getStats is registered", conn->getMethods().count("system.getStats"), (size_t)1);

        conn->processMessage(LLJSONRPCConnection::makeEnvelope(LLSD("req_1"), "system.listMethods", LLSD::emptyMap(), LLSD(), LLSD()));
        ensure_equals("one message sent", conn->mSent.size(), (size_t)1);

        boost::system::error_code ec;
        const LLSD sent = LlsdFromJson(boost::json::parse(conn->mSent[0], ec));
        ensure("parsed without error", !ec);
        ensure_equals("unauthenticated, even a built-in method is refused", sent["error"]["code"].asInteger(), LLJSONRPCConnection::RPCError::UNAUTHORIZED);
    }

    template<> template<>
    void jsonrpcws_object::test<16>()
    {
        set_test_name("makeEnvelope includes id and method by the documented rules");

        // Notification: id undefined, method non-empty -> id omitted.
        const LLSD notification = LLJSONRPCConnection::makeEnvelope(LLSD(), "object.list", LLSD::emptyMap(), LLSD(), LLSD());
        ensure("notification omits id", !notification.has("id"));
        ensure_equals("notification keeps method", notification["method"].asString(), std::string("object.list"));

        // Request: id defined, method non-empty -> both kept.
        const LLSD request = LLJSONRPCConnection::makeEnvelope(LLSD("req_1"), "object.list", LLSD::emptyMap(), LLSD(), LLSD());
        ensure("request keeps id", request.has("id"));
        ensure_equals("request keeps method", request["method"].asString(), std::string("object.list"));

        // Success response: id defined, method empty -> id kept, method omitted.
        const LLSD response = LLJSONRPCConnection::makeEnvelope(LLSD("req_1"), std::string(), LLSD(), LLSD("ok"), LLSD());
        ensure("response keeps id", response.has("id"));
        ensure("response omits method", !response.has("method"));
        ensure_equals("response keeps result", response["result"].asString(), std::string("ok"));

        // Error response with an undefined id (e.g. a parse error): method
        // is empty, so id is kept even though undefined -- serializes as null.
        LLSD error_obj;
        error_obj["code"] = -32700;
        error_obj["message"] = "Parse error";
        const LLSD error_response = LLJSONRPCConnection::makeEnvelope(LLSD(), std::string(), LLSD(), LLSD(), error_obj);
        ensure("error response keeps id even when undefined", error_response.has("id"));
        ensure("that id is null", error_response["id"].isUndefined());
        ensure("error response omits method", !error_response.has("method"));
    }

    template<> template<>
    void jsonrpcws_object::test<17>()
    {
        set_test_name("onClose fails every pending request with CONNECTION_CLOSED, exactly once");

        TestJSONRPCConnection conn;

        int callback_count_a = 0;
        int callback_count_b = 0;
        conn.testInjectPendingRequest(
            "req_a",
            LLTimer::getTotalSeconds() + 60.0,
            [&](const LLSD& result, const LLSD& error)
            {
                ++callback_count_a;
                ensure("no result on connection close", result.isUndefined());
                ensure_equals("connection closed error code", error["code"].asInteger(), LLJSONRPCConnection::RPCError::CONNECTION_CLOSED);
            });
        conn.testInjectPendingRequest(
            "req_b",
            LLTimer::getTotalSeconds() + 60.0,
            [&](const LLSD&, const LLSD& error)
            {
                ++callback_count_b;
                ensure_equals("connection closed error code", error["code"].asInteger(), LLJSONRPCConnection::RPCError::CONNECTION_CLOSED);
            });
        ensure_equals("two requests pending", conn.testPendingRequestCount(), (size_t)2);

        conn.onClose();

        ensure_equals("first callback fires exactly once", callback_count_a, 1);
        ensure_equals("second callback fires exactly once", callback_count_b, 1);
        ensure_equals("nothing left pending", conn.testPendingRequestCount(), (size_t)0);
    }
}

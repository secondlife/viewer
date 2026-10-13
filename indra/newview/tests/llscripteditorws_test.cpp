/**
 * @file llscripteditorws_test.cpp
 * @brief Headless test fixture for LLScriptEditorWSServer with fake connection
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

#include "../llviewerprecompiledheaders.h"
#include "../llscripteditorws.h"
#include "../test/lltut.h"

namespace
{
class FakeScriptEditorWSConnection : public LLScriptEditorWSConnection
{
public:
    explicit FakeScriptEditorWSConnection(const LLWebsocketMgr::WSServer::ptr_t& server)
        : LLScriptEditorWSConnection(server, LLWebsocketMgr::connection_h())
    {
    }

    using LLJSONRPCConnection::processMessage;
};

class HeadlessScriptEditorWSServer : public LLScriptEditorWSServer
{
public:
    HeadlessScriptEditorWSServer()
        : LLScriptEditorWSServer("script_editor_ws_test_server", 0, true)
    {
    }

    using LLScriptEditorWSServer::setupConnectionMethods;
};
}

namespace tut
{
    struct script_editor_ws_test_data
    {
        std::shared_ptr<HeadlessScriptEditorWSServer> mServer;
        std::shared_ptr<FakeScriptEditorWSConnection> mConnection;

        script_editor_ws_test_data()
        {
            mServer = std::make_shared<HeadlessScriptEditorWSServer>();
            mConnection = std::make_shared<FakeScriptEditorWSConnection>(mServer);
            mServer->setupConnectionMethods(mConnection);
        }
    };

    typedef test_group<script_editor_ws_test_data> script_editor_ws_test_group;
    typedef script_editor_ws_test_group::object script_editor_ws_test_object;
    script_editor_ws_test_group script_editor_ws_tests("LLScriptEditorWSServer");

    template<> template<>
    void script_editor_ws_test_object::test<1>()
    {
        set_test_name("headless fixture constructs server and fake connection");
        ensure("server should be created", mServer != nullptr);
        ensure("fake connection should be created", mConnection != nullptr);
    }

    template<> template<>
    void script_editor_ws_test_object::test<2>()
    {
        set_test_name("fake connection can drive JSON-RPC notification dispatch");

        bool called = false;
        mConnection->registerMethod(
            "fixture.echo",
            [&](const std::string& method, const LLSD& id, const LLSD& params) -> LLSD
            {
                called = true;
                ensure_equals("method should match", method, "fixture.echo");
                ensure("notification id should be undefined", id.isUndefined());
                ensure_equals("payload should be forwarded", params["value"].asString(), "ok");
                return LLSD();
            });

        LLSD params;
        params["value"] = "ok";
        LLSD notification = LLJSONRPCConnection::makeEnvelope(
            LLSD(),
            "fixture.echo",
            params,
            LLSD(),
            LLSD());

        mConnection->processMessage(notification);
        ensure("fixture handler should be invoked", called);
    }

    template<> template<>
    void script_editor_ws_test_object::test<3>()
    {
        set_test_name("fixture includes default session.ping handler");

        LLSD params;
        params["timestamp"] = 123;
        LLSD notification = LLJSONRPCConnection::makeEnvelope(
            LLSD(),
            "system.ping",
            params,
            LLSD(),
            LLSD());

        mConnection->processMessage(notification);
        ensure("session.ping dispatch should complete in headless mode", true);
    }
}

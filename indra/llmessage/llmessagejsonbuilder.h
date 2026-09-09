/**
 * @file llmessagejsonbuilder.h
 * @brief DEBUG/TESTING ONLY: build a message, or simulate receiving one,
 * from a JSON description.
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

#ifndef LL_LLMESSAGEJSONBUILDER_H
#define LL_LLMESSAGEJSONBUILDER_H

#include <string>
#include <boost/json/fwd.hpp>

class LLMessageSystem;
class LLHost;

// DEBUG/TESTING ONLY -- not used in production.
//
// Lets test/QA tooling fabricate an arbitrary message from a JSON
// description instead of one built by real client code, so client-side
// handling of a message (or a message the simulator understands but the
// viewer's UI has no sender for yet) can be exercised ahead of real support
// existing on the other end. Driven by the "#PKT<"/"#PKT>" chat convention
// process_chat_from_simulator() recognizes on llOwnerSay in
// indra/newview/llviewermessage.cpp -- see there for the trigger, and the
// .cpp here for the JSON body format and per-field type-conversion rules.
class LLMessageJsonBuilder
{
public:
    // Builds a message of template `msg_name` into `msg` (calls
    // msg->newMessageFast() itself) from `body`, a JSON object whose "Name"
    // key (if present) is ignored -- the caller already used it to choose
    // msg_name -- and whose other keys must each name a block in that
    // template: an object for a single block, or an array of objects for a
    // multiple/variable block, repeating the block once per array entry.
    // Field values are plain JSON with no explicit wire-type tag; the type
    // is looked up from the message template and used to convert the value
    // automatically. Returns false, after logging why, if the template, a
    // named block, or a named field can't be resolved, or a value can't be
    // converted to its field's wire type.
    static bool build(LLMessageSystem* msg, const std::string& msg_name,
                       const boost::json::value& body);

    // As build(), then loops the built message back through the local
    // decode path and invokes msg_name's already-registered handler
    // directly, exactly as if it had just been received from `host`.
    static bool simulateReceived(LLMessageSystem* msg, const std::string& msg_name,
                                  const boost::json::value& body, const LLHost& host);
};

#endif // LL_LLMESSAGEJSONBUILDER_H

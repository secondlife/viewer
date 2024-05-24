/**
 * @file llremoteparcelrequest_test.cpp
 * @author Brad Kittenbrink <brad@lindenlab.com>
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "../test/lltut.h"
#if 0

#include "../llremoteparcelrequest.h"

#include "../llagent.h"
#include "message.h"
#include "llurlentry.h"
#include "llpounceable.h"

namespace {
    const LLUUID TEST_PARCEL_ID("11111111-1111-1111-1111-111111111111");
}

LLCurl::Responder::Responder() { }
LLCurl::Responder::~Responder() { }
void LLCurl::Responder::httpFailure() { }
void LLCurl::Responder::httpSuccess() { }
void LLCurl::Responder::httpCompleted() { }
void LLCurl::Responder::failureResult(S32 status, const std::string& reason, const LLSD& content) { }
void LLCurl::Responder::successResult(const LLSD& content) { }
void LLCurl::Responder::completeResult(S32 status, const std::string& reason, const LLSD& content) { }
std::string LLCurl::Responder::dumpResponse() const { return "(failure)"; }
void LLCurl::Responder::completedRaw(LLChannelDescriptors const &,std::shared_ptr<LLBufferArray> const &) { }
void LLMessageSystem::getF32(char const *,char const *,F32 &,S32) { }
void LLMessageSystem::getU8(char const *,char const *,U8 &,S32) { }
void LLMessageSystem::getS32(char const *,char const *,S32 &,S32) { }
void LLMessageSystem::getString(char const *,char const *, std::string &,S32) { }
void LLMessageSystem::getUUID(char const *,char const *, LLUUID & out_id,S32)
{
    out_id = TEST_PARCEL_ID;
}
void LLMessageSystem::nextBlock(char const *) { }
void LLMessageSystem::addUUID(char const *,LLUUID const &) { }
void LLMessageSystem::addUUIDFast(char const *,LLUUID const &) { }
void LLMessageSystem::nextBlockFast(char const *) { }
void LLMessageSystem::newMessage(char const *) { }
LLPounceable<LLMessageSystem*, LLPounceableStatic> gMessageSystem;
char const* const _PREHASH_AgentID = 0;   // never dereferenced during this test
char const* const _PREHASH_AgentData = 0; // never dereferenced during this test
LLAgent gAgent;
LLAgent::LLAgent() : mAgentAccess(NULL) { }
LLAgent::~LLAgent() { }
void LLAgent::sendReliableMessage(void) { }
LLUUID gAgentSessionID;
LLUUID gAgentID;
LLUIColor::LLUIColor(void) { }
LLControlGroup::LLControlGroup(std::string const & name) : LLInstanceTracker<LLControlGroup, std::string>(name) { }
LLControlGroup::~LLControlGroup(void) { }
void LLUrlEntryParcel::processParcelInfo(const LLUrlEntryParcel::LLParcelData& parcel_data) { }

namespace tut
{
    struct TestObserver : public LLRemoteParcelInfoObserver {
        TestObserver() : mProcessed(false) { }

        virtual void processParcelInfo(const LLParcelData& parcel_data)
        {
            mProcessed = true;
        }

        virtual void setParcelID(const LLUUID& parcel_id) { }

        virtual void setErrorStatus(S32 status, const std::string& reason) { }

        bool mProcessed;
    };

    struct RemoteParcelRequestData
    {
        RemoteParcelRequestData()
        {
        }
    };

    typedef test_group<RemoteParcelRequestData> remoteparcelrequest_t;
    typedef remoteparcelrequest_t::object remoteparcelrequest_object_t;
    tut::remoteparcelrequest_t tut_remoteparcelrequest("LLRemoteParcelRequest");

    template<> template<>
    void remoteparcelrequest_object_t::test<1>()
    {
        set_test_name("observer pointer");

        std::unique_ptr<TestObserver> observer(new TestObserver());

        LLRemoteParcelInfoProcessor & processor = LLRemoteParcelInfoProcessor::instance();
        processor.addObserver(LLUUID(TEST_PARCEL_ID), observer.get());

        processor.processParcelInfoReply(gMessageSystem, NULL);

        ensure(observer->mProcessed);
    }

    template<> template<>
    void remoteparcelrequest_object_t::test<2>()
    {
        set_test_name("CHOP-220: dangling observer pointer");

        LLRemoteParcelInfoObserver * observer = new TestObserver();

        LLRemoteParcelInfoProcessor & processor = LLRemoteParcelInfoProcessor::instance();
        processor.addObserver(LLUUID(TEST_PARCEL_ID), observer);

        delete observer;
        observer = NULL;

        processor.processParcelInfoReply(gMessageSystem, NULL);
    }
}
#endif

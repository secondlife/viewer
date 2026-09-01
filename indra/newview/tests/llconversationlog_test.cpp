/**
 * @file llconversationlog_test.cpp
 * @brief Conversation Log discovery tests.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 * $/LicenseInfo$
 */
#include "../llviewerprecompiledheaders.h"
#include "llsingleton.h"
#include "llcontrol.h"
#include "../test/lltut.h"
#include "../llconversationlog.h"

LLControlGroup gSavedSettings("test settings");
LLControlGroup gSavedPerAccountSettings("test per-account settings");
LLUUID gAgentID("00000000-0000-0000-0000-000000000001");

// The fixture needs only the no-session branches of the viewer services reached
// by LLConversationLog; these doubles keep that boundary isolated.
LLIMModel::LLIMModel()
{
}

LLIMModel::LLIMSession* LLIMModel::findIMSession(const LLUUID&) const
{
    return NULL;
}

bool LLIMModel::LLIMSession::isOutgoingAdHoc() const
{
    return false;
}

LLUUID LLIMModel::LLIMSession::generateOutgoingAdHocHash() const
{
    return mSessionID;
}

LLIMMgr::LLIMMgr()
{
}

void LLIMMgr::addSessionObserver(LLIMSessionObserver*)
{
}

void LLIMMgr::removeSessionObserver(LLIMSessionObserver*)
{
}

LLAvatarTracker LLAvatarTracker::sInstance;

LLAvatarTracker::LLAvatarTracker()
:   mTrackingData(NULL),
    mTrackedAgentValid(false),
    mModifyMask(0),
    mIsNotifyObservers(false)
{
}

LLAvatarTracker::~LLAvatarTracker()
{
}

void LLAvatarTracker::addObserver(LLFriendObserver*)
{
}

void LLAvatarTracker::removeObserver(LLFriendObserver*)
{
}

void LLLogChat::renameLogFile(const std::string&, const std::string&)
{
}

LLFloaterIMSession* LLFloaterIMSession::findInstance(const LLUUID&)
{
    return NULL;
}

boost::signals2::connection LLFloaterIMSession::setIMFloaterShowedCallback(
    const floater_showed_signal_t::slot_type&)
{
    return boost::signals2::connection();
}

// Keep private access to fixture state; production initialization binds global services.
struct LLConversationLogTestAccess
{
    static void reset(bool logging_enabled)
    {
        LLConversationLog& log = LLConversationLog::instance();
        log.mConversations.clear();
        log.mLoggingEnabled = logging_enabled;
    }

    static void setOfflineMessages(const LLUUID& session_id, bool has_offline_messages)
    {
        LLConversationLog& log = LLConversationLog::instance();
        for (LLConversation& conversation : log.mConversations)
        {
            if (conversation.getSessionID() == session_id)
            {
                conversation.setOfflineMessages(has_offline_messages);
                return;
            }
        }
    }
};

// Capture both observer contracts so timestamp updates stay targeted to one row.
struct ConversationLogObserver : public LLConversationLogObserver
{
    void changed() override
    {
        ++mRefreshes;
    }

    void changed(const LLUUID& session_id, U32 mask) override
    {
        ++mParticularChanges;
        mSessionID = session_id;
        mMask = mask;
    }

    S32 mRefreshes = 0;
    S32 mParticularChanges = 0;
    LLUUID mSessionID;
    U32 mMask = 0;
};

namespace tut
{
struct conversation_log
{
    conversation_log()
    {
        // A different device starts without locally created conversation metadata.
        LLConversationLogTestAccess::reset(true);
    }

    ~conversation_log()
    {
        LLConversationLogTestAccess::reset(false);
    }
};

typedef test_group<conversation_log> group_t;
typedef group_t::object object_t;
group_t group("LLConversationLog");

template<> template<> void object_t::test<1>()
{
    const LLUUID resident_id("00000000-0000-0000-0000-000000000002");
    const LLUUID session_id = gAgentID ^ resident_id;
    const U64Seconds archived_time(LLUnits::Seconds::fromValue(1787756400.0));

    // A durable service archive has resident metadata but no live IM session.
    LLConversationLog& log = LLConversationLog::instance();
    log.addServiceConversation(
        session_id,
        "Bridie Linden",
        "bridie.linden",
        resident_id,
        archived_time);

    // Repeated archive publication must not duplicate the Conversation Log row.
    log.addServiceConversation(
        session_id,
        "Bridie Linden",
        "bridie.linden",
        resident_id,
        archived_time);

    const LLConversation* discovered = log.getConversation(session_id);
    ensure("remote direct conversation is discoverable", discovered != NULL);
    ensure_equals("remote conversation is unique", log.getConversations().size(), size_t(1));
    ensure("remote conversation is P2P",
           discovered->getConversationType() == LLIMModel::LLIMSession::P2P_SESSION);
    ensure("remote archive time retained", discovered->getTime() == archived_time);
    ensure_equals("remote participant retained", discovered->getParticipantID(), resident_id);
    ensure_equals("remote name retained", discovered->getConversationName(),
                  std::string("Bridie Linden"));
    ensure_equals("remote transcript stem retained", discovered->getHistoryFileName(),
                  std::string("bridie.linden"));
}

template<> template<> void object_t::test<2>()
{
    const LLUUID resident_id("00000000-0000-0000-0000-000000000002");
    const LLUUID changed_resident_id("00000000-0000-0000-0000-000000000003");
    const LLUUID session_id = gAgentID ^ resident_id;
    const U64Seconds original_time(LLUnits::Seconds::fromValue(1787756400.0));
    const U64Seconds newer_time(LLUnits::Seconds::fromValue(1787929200.0));
    const U64Seconds older_time(LLUnits::Seconds::fromValue(1787666400.0));

    LLConversationLog& log = LLConversationLog::instance();
    log.addServiceConversation(
        session_id,
        "Bridie Linden",
        "bridie.linden",
        resident_id,
        original_time);
    LLConversationLogTestAccess::setOfflineMessages(session_id, true);

    // A newer archive advances only the timestamp. Equal and older publications
    // remain silent so stale service data cannot regress or duplicate the row.
    ConversationLogObserver observer;
    log.addObserver(&observer);
    log.addServiceConversation(
        session_id,
        "Changed Name",
        "changed.filename",
        changed_resident_id,
        newer_time);
    log.addServiceConversation(
        session_id,
        "Changed Again",
        "changed.again",
        changed_resident_id,
        newer_time);
    log.addServiceConversation(
        session_id,
        "Older Name",
        "older.filename",
        changed_resident_id,
        older_time);
    log.removeObserver(&observer);

    const LLConversation* updated = log.getConversation(session_id);
    ensure("updated conversation remains present", updated != NULL);
    ensure_equals("updated conversation remains unique", log.getConversations().size(), size_t(1));
    ensure("newer archive time applied", updated->getTime() == newer_time);
    ensure_equals("formatted archive time refreshed", updated->getTimestamp(),
                  LLConversation::createTimestamp(newer_time));
    ensure_equals("existing participant preserved", updated->getParticipantID(), resident_id);
    ensure_equals("existing name preserved", updated->getConversationName(),
                  std::string("Bridie Linden"));
    ensure_equals("existing transcript stem preserved", updated->getHistoryFileName(),
                  std::string("bridie.linden"));
    ensure("existing offline state preserved", updated->hasOfflineMessages());
    ensure_equals("timestamp update avoids full refresh", observer.mRefreshes, S32(0));
    ensure_equals("only newer time notifies", observer.mParticularChanges, S32(1));
    ensure_equals("timestamp update identifies session", observer.mSessionID, session_id);
    ensure_equals("timestamp update identifies change", observer.mMask,
                  U32(LLConversationLogObserver::CHANGED_TIME));
}

template<> template<> void object_t::test<3>()
{
    LLConversationLogTestAccess::reset(false);

    // Service history must not create metadata when Conversation Log privacy is disabled.
    const LLUUID resident_id("00000000-0000-0000-0000-000000000002");
    LLConversationLog& log = LLConversationLog::instance();
    log.addServiceConversation(
        gAgentID ^ resident_id,
        "Bridie Linden",
        "bridie.linden",
        resident_id,
        U64Seconds(LLUnits::Seconds::fromValue(1787756400.0)));

    ensure("disabled Conversation Log remains empty", log.isLogEmpty());
}
}

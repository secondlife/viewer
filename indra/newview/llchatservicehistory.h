/**
 * @file llchatservicehistory.h
 * @brief ChatService direct-IM history synchronization and stitched reads.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 * $/LicenseInfo$
 */
#ifndef LL_LLCHATSERVICEHISTORY_H
#define LL_LLCHATSERVICEHISTORY_H

#include "llavatarname.h"
#include "llchatservicehistorycore.h"
#include "llhandle.h"
#include "llinstantmessage.h"
#include "llsd.h"
#include "lluuid.h"

#include <boost/function.hpp>
#include <boost/signals2/connection.hpp>
#include <list>
#include <vector>

namespace LLChatServiceHistory
{
    // Resident snapshots are published on the main thread and contain every value
    // direct-IM and Preview consumers need to render or reload history.
    struct Snapshot
    {
        // Remote, metadata, or storage work is still active for this resident.
        bool service_work_active = false;

        // Service rows may be presented under the current account-wide gates.
        bool service_presentation_allowed = false;

        // Consumers reject local reads captured before the latest archive mutation.
        U32 archive_serial = 0;

        // Shared name metadata is ready for blocking and legacy-path resolution.
        bool metadata_resolved = false;
        LLAvatarName metadata;

        // The validated first page is presentation-only until durable publication.
        std::vector<LLChatServiceHistoryCore::Row> head_preview;
    };

    // An asynchronous stitched read carries the account and archive generations
    // needed to reject results that became stale while filesystem work ran.
    struct HistoryResult
    {
        std::list<LLSD> messages;
        U32 account_epoch = 0;
        U32 archive_serial = 0;
        bool included_service = false;

        // The reader classified a canonical archive that manager-side maintenance must revisit.
        bool maintenance_needed = false;
    };

    typedef boost::function<void(const HistoryResult&)> history_callback_t;
    typedef boost::function<void(bool)> delete_callback_t;
    typedef boost::function<void(const LLUUID&, const Snapshot&)> snapshot_callback_t;

    // The synchronizer has one lifecycle per logged-in account.
    void start();
    void stop();
    void regionChanged();

    // Account-wide gates keep all historical sources fail-closed during unsafe state
    // recovery or transcript deletion.
    U32 accountEpoch();
    bool historySuppressed();
    bool servicePresentationAllowed();
    bool localHistoryExists();
    bool localHistoryExists(const LLUUID& resident_id);

    // Opens and due direct-message activity share one account-scoped priority queue.
    bool isPersistedDirectDialog(EInstantMessage dialog);
    void prioritizeResident(const LLUUID& resident_id, bool follow_active_request = false);
    // Incoming and outgoing IMs reset one quiet deadline per resident.
    void noteDirectMessageActivity(const LLUUID& resident_id);

    // Views connect first and then query so they cannot miss an active-work transition.
    Snapshot getSnapshot(const LLUUID& resident_id);
    boost::signals2::connection setSnapshotChanged(const snapshot_callback_t& callback);
    using Messages = LLChatServiceHistoryCore::Messages;

    // A view owns its subscription and current read. Callbacks run on the main loop;
    // the handle and request token reject results after destruction or reload.
    class History : public LLHandleProvider<History>
    {
    public:
        using callback_t = boost::function<void(const Messages&)>;
        void load(const LLUUID& resident_id, const std::string& legacy_stem, U32 limit,
                  const callback_t& callback, const LLUUID& session_id = LLUUID::null);
        void clear();
        void stop();
        bool isLoading() const { return mLoading; }

    private:
        void reload();
        void onSnapshot(const Snapshot& snapshot);
        void publish(const Snapshot& snapshot);

        LLUUID mResidentID;
        LLUUID mSessionID;
        std::string mLegacyStem;
        U32 mLimit = 0;
        U64 mToken = 0;
        U32 mArchiveSerial = 0;
        bool mLoading = false;
        bool mServiceAllowed = false;
        LLChatServiceHistoryCore::History mHistory;
        callback_t mCallback;
        boost::signals2::scoped_connection mConnection;
    };

    bool replaceHistory(Messages& current, const Messages& history, bool direct);

    // Core IM delivery carries one opaque context through translation and logging.
    LLSD prepareLiveMessage(const std::string& text, U32& timestamp, const LLSD& context);
    void recordLiveMessage(LLSD& message, const LLSD& context);

    // The callback runs on the main queue after legacy and service storage are read.
    bool loadStitchedHistory(const LLUUID& resident_id, const std::string& legacy_stem,
                             U32 limit, const history_callback_t& callback);

    // Deletion records its durable cutoff before clearing views or sweeping files.
    bool deleteTranscriptsAsync(const delete_callback_t& callback);
}

#endif

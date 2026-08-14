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
    bool enabledForLogin();
    U32 accountEpoch();
    bool historySuppressed();
    bool servicePresentationAllowed();
    bool localHistoryExists();
    bool localHistoryExists(const LLUUID& resident_id);

    // Opens and accepted-inbound activity share one account-scoped priority queue.
    // A first outbound send may confirm discovery for an as-yet unlisted resident.
    bool isPersistedDirectDialog(EInstantMessage dialog);
    void prioritizeResident(const LLUUID& resident_id, bool inbound = false);
    void noteOutboundDirectMessage(const LLUUID& resident_id);

    // Views connect first and then query so they cannot miss an active-work transition.
    Snapshot getSnapshot(const LLUUID& resident_id);
    boost::signals2::connection setSnapshotChanged(const snapshot_callback_t& callback);
    std::list<LLSD> mergeHeadPreview(const std::list<LLSD>& loaded,
                                     const Snapshot& snapshot, U32 limit);

    // Remove one historical occurrence for each exact same-minute live occurrence.
    std::list<LLSD> filterLiveDuplicates(const std::list<LLSD>& history,
                                         const std::list<LLSD>& live);

    // The callback runs on the main queue after legacy and service storage are read.
    bool loadStitchedHistory(const LLUUID& resident_id, const std::string& legacy_stem,
                             U32 limit, const history_callback_t& callback);

    // Deletion records its durable cutoff before clearing views or sweeping files.
    bool deleteTranscriptsAsync(const delete_callback_t& callback);
}

#endif

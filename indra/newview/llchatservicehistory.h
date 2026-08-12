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
    struct Snapshot
    {
        bool service_work_active = false;
        bool service_presentation_allowed = false;
        U32 archive_serial = 0;
        bool metadata_resolved = false;
        LLAvatarName metadata;
        std::vector<LLChatServiceHistoryCore::Row> head_preview;
    };

    struct HistoryResult
    {
        std::list<LLSD> messages;
        U32 account_epoch = 0;
        U32 archive_serial = 0;
        bool included_service = false;
        bool maintenance_needed = false;
    };

    typedef boost::function<void(const HistoryResult&)> history_callback_t;
    typedef boost::function<void(bool)> delete_callback_t;
    typedef boost::function<void(const LLUUID&, const Snapshot&)> snapshot_callback_t;

    void start();
    void stop();
    void regionChanged();

    bool enabledForLogin();
    U32 accountEpoch();
    bool historySuppressed();
    bool servicePresentationAllowed();
    bool localHistoryExists();
    bool localHistoryExists(const LLUUID& resident_id);

    bool isPersistedDirectDialog(EInstantMessage dialog);
    void prioritizeResident(const LLUUID& resident_id, bool inbound = false);
    Snapshot getSnapshot(const LLUUID& resident_id);
    boost::signals2::connection setSnapshotChanged(const snapshot_callback_t& callback);
    std::list<LLSD> mergeHeadPreview(const std::list<LLSD>& loaded,
                                     const Snapshot& snapshot, U32 limit);

    bool loadStitchedHistory(const LLUUID& resident_id, const std::string& legacy_stem,
                             U32 limit, const history_callback_t& callback);
    bool deleteTranscriptsAsync(const delete_callback_t& callback);
}

#endif

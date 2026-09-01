/**
 * @file llchatservicehistorycore.h
 * @brief Strict ChatService wire, TimeUUID, and CSV primitives.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 * $/LicenseInfo$
 */
#ifndef LL_LLCHATSERVICEHISTORYCORE_H
#define LL_LLCHATSERVICEHISTORYCORE_H

#include "llsd.h"
#include "lluuid.h"

#include <array>
#include <deque>
#include <iosfwd>
#include <string>
#include <vector>

namespace LLChatServiceHistoryCore
{
    extern const char* const CSV_HEADER;
    extern const char* const INDEX_HEADER;

    // Cassandra compares UUIDv1 timestamps first and the final eight bytes as signed
    // values when timestamps tie.
    struct TimeUuidKey
    {
        U64 ticks = 0;
        std::array<S8, 8> tail{};

        bool operator==(const TimeUuidKey& rhs) const;
        bool operator<(const TimeUuidKey& rhs) const;
        bool operator>(const TimeUuidKey& rhs) const
        {
            return rhs < *this;
        }
        bool operator<=(const TimeUuidKey& rhs) const
        {
            return !(rhs < *this);
        }
        bool operator>=(const TimeUuidKey& rhs) const
        {
            return !(*this < rhs);
        }
    };

    // Row is the single validated representation shared by wire pages and CSV files.
    struct Row
    {
        std::string conversation_id;
        std::string msg_id;
        LLUUID from_id;
        std::string from_name;
        std::string message;
        S32 dialog = 0;
        std::string created_at;
        TimeUuidKey key;
    };

    // Direct entries are extracted only after the complete discovery list validates.
    struct ListEntry
    {
        LLUUID resident_id;
        std::string conversation_id;
        std::string last_msg_id;
    };

    // Pages retain rows newer than the account deletion cutoff and expose the next
    // validated cursor only while older paging remains necessary.
    struct Page
    {
        std::vector<Row> rows;
        std::string next_cursor;
        bool terminal = false;
        bool cutoff_reached = false;
    };

    // Archive states distinguish repairable torn tails from complete corrupt records.
    enum EArchiveState
    {
        ARCHIVE_ABSENT,
        ARCHIVE_VALID,
        ARCHIVE_TORN,
        ARCHIVE_CORRUPT,
        ARCHIVE_FAILED
    };

    // A scan folds the canonical file into a bounded display window and a compact
    // summary without materializing the complete archive.
    struct ArchiveScan
    {
        EArchiveState state = ARCHIVE_ABSENT;
        U32 row_count = 0;
        bool has_oldest = false;
        TimeUuidKey oldest;
        TimeUuidKey newest;
        U64 file_size = 0;
        S64 file_mtime = 0;
        std::streamoff valid_prefix_bytes = 0;
        std::vector<Row> display_rows;
    };

    std::string directConversationId(const LLUUID& agent_id, const LLUUID& resident_id);
    bool parseCanonicalUuid(const std::string& text, LLUUID& id);
    bool parseTimeUuid(const std::string& text, TimeUuidKey& key);
    bool persistedDirectDialog(S32 dialog);
    bool parseCreatedAt(const std::string& text, std::string& normalized);
    bool sameDirectSenderName(const std::string& left, const std::string& right);

    // Legacy SLT wall times carry no UTC offset; UTC-7 is their earliest possible
    // interpretation at the durable service boundary.
    bool legacyWallMayPrecedeService(F64 wall_epoch, F64 service_epoch);

    // Both wire validators are all-or-nothing and clear their output before parsing.
    bool validateConversationList(const LLSD& value, const LLUUID& agent_id,
                                  std::vector<ListEntry>& entries);
    bool validateHistoryPage(const LLSD& value, const LLUUID& agent_id,
                             const LLUUID& resident_id,
                             const std::string& conversation_id,
                             const std::string& requested_cursor,
                             U64 deleted_before_ticks, Page& page);

    std::string quoteCsv(const std::string& value);
    void writeCsvRow(std::ostream& output, const Row& row);

    // scanArchive never repairs storage; callers decide whether a classified archive
    // may be displayed, repaired, or quarantined.
    bool scanArchive(const std::string& path, const LLUUID& agent_id,
                     const LLUUID& resident_id, U64 deleted_before_ticks,
                     U32 display_cap, ArchiveScan& scan);

    // Capture the regular file's physical identity for guarded repair and publication.
    bool archiveStamp(const std::string& path, U64& file_size, S64& file_mtime);
}

#endif

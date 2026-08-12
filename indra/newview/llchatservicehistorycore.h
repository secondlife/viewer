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

    struct TimeUuidKey
    {
        U64 ticks = 0;
        std::array<S8, 8> tail{};

        bool operator==(const TimeUuidKey& rhs) const;
        bool operator<(const TimeUuidKey& rhs) const;
        bool operator>(const TimeUuidKey& rhs) const { return rhs < *this; }
        bool operator<=(const TimeUuidKey& rhs) const { return !(rhs < *this); }
        bool operator>=(const TimeUuidKey& rhs) const { return !(*this < rhs); }
    };

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

    struct ListEntry
    {
        LLUUID resident_id;
        std::string conversation_id;
        std::string last_msg_id;
    };

    struct Page
    {
        std::vector<Row> rows;
        std::string next_cursor;
        bool terminal = false;
        bool cutoff_reached = false;
    };

    enum EArchiveState
    {
        ARCHIVE_ABSENT,
        ARCHIVE_VALID,
        ARCHIVE_TORN,
        ARCHIVE_CORRUPT,
        ARCHIVE_FAILED
    };

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

    bool validateConversationList(const LLSD& value, const LLUUID& agent_id,
                                  std::vector<ListEntry>& entries);
    bool validateHistoryPage(const LLSD& value, const LLUUID& agent_id,
                             const LLUUID& resident_id,
                             const std::string& conversation_id,
                             const std::string& requested_cursor,
                             U64 deleted_before_ticks, Page& page);

    std::string quoteCsv(const std::string& value);
    void writeCsvRow(std::ostream& output, const Row& row);
    bool scanArchive(const std::string& path, const LLUUID& agent_id,
                     const LLUUID& resident_id, U64 deleted_before_ticks,
                     U32 display_cap, ArchiveScan& scan);

    bool archiveStamp(const std::string& path, U64& file_size, S64& file_mtime);
}

#endif

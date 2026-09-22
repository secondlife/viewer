/**
 * @file llchatservicehistorycore.h
 * @brief ChatService wire/storage primitives and direct history composition.
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
#include <list>
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

    // Compare service rows against original bodies and plaintext against logged bodies/times.
    bool sameDirectHistoryOccurrence(const LLSD& history, const LLSD& timed);

    // Reserve known send times before receipt intervals and local-clock minute matches.
    std::list<LLSD> filterDirectHistoryDuplicates(const std::list<LLSD>& history,
                                                 const std::list<LLSD>& live);

    using Messages = std::list<LLSD>;

    // Union historical seams by service ID and plaintext occurrence, consuming overlaps one-for-one.
    Messages mergeDirectHistory(const Messages& loaded, const Messages& service);

    // Preview publications start from the loaded value; visible context is retained separately.
    class History
    {
    public:
        void setLoaded(const Messages& messages);
        void clear();
        void clearService();
        Messages compose(const Messages& preview, const Messages& live, U32 limit,
                         bool retain_context = false);

    private:
        Messages mLoaded;
        Messages mVisible;
    };

    // Preserve live order and notification rows; replace history and reindex only on change.
    bool replaceHistory(Messages& current, const Messages& history, bool direct);

    // Capture timing before translation and keep its provenance with the original wire body.
    LLSD incomingContext(const LLSD& original_text, bool online);
    LLSD captureLiveMessage(const std::string& text, U32& timestamp, const LLSD& context, U32 now);
    void recordLiveMessage(LLSD& message, const LLSD& context, U32 logged_at);

    // Both inputs are newest-first; interleave timed history without reordering live rows.
    std::list<LLSD> interleaveDirectHistory(const std::list<LLSD>& history,
                                          const std::list<LLSD>& live);

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

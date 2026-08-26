/**
 * @file llchatservicehistorycore.cpp
 * @brief Strict ChatService wire, TimeUUID, and CSV primitives.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"

#include "llchatservicehistorycore.h"

#include "llcachename.h"
#include "lldate.h"
#include "llfile.h"
#include "llstring.h"
#include "fsyspath.h"

#if LL_WINDOWS
#include "llwin32headers.h"
#else
#include <sys/stat.h>
#endif

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <limits>
#include <set>

namespace LLChatServiceHistoryCore
{
const char* const CSV_HEADER =
    "conversation_id,msg_id,from_id,from_name,message,dialog,created_at\n";
const char* const INDEX_HEADER = "filename,agent_id,agent_name,display_name\n";

namespace
{
    const U32 COLUMN_COUNT = 7;

    bool cleanText(const std::string& value, size_t limit)
    {
        // Wire and CSV text must remain bounded, NUL-free, and canonical UTF-8.
        return value.size() <= limit && value.find('\0') == std::string::npos &&
               wstring_to_utf8str(utf8str_to_wstring(value)) == value;
    }

    bool decimalDialog(const std::string& text, S32& dialog)
    {
        if (text.empty() || (text.size() > 1 && text[0] == '0') ||
            text.find_first_not_of("0123456789") != std::string::npos)
        {
            return false;
        }

        return LLStringUtil::convertToS32(text, dialog) && persistedDirectDialog(dialog);
    }

    bool rowFromFields(const std::vector<std::string>& fields,
                       const LLUUID& agent_id, const LLUUID& resident_id,
                       Row& row)
    {
        // A stored row is trusted only after every field and cross-field identity
        // constraint matches the current account and resident.
        if (fields.size() != COLUMN_COUNT ||
            fields[0] != directConversationId(agent_id, resident_id) ||
            !parseTimeUuid(fields[1], row.key) ||
            !parseCanonicalUuid(fields[2], row.from_id) ||
            (row.from_id != agent_id && row.from_id != resident_id) ||
            !cleanText(fields[3], 256) || !cleanText(fields[4], 1024) ||
            !decimalDialog(fields[5], row.dialog) ||
            !parseCreatedAt(fields[6], row.created_at))
        {
            return false;
        }

        row.conversation_id = fields[0];
        row.msg_id = fields[1];
        row.from_name = fields[3];
        row.message = fields[4];
        return true;
    }

    // Decode RFC-4180 records incrementally so validation and torn-tail recovery
    // classify the same byte stream with one parser.
    bool readRecord(std::istream& input, std::vector<std::string>& fields,
                    bool& eof_torn, bool& had_record)
    {
        fields.clear();
        eof_torn = false;
        had_record = false;
        std::string field;
        bool quoted = false;
        bool quote_closed = false;
        bool started = false;

        for (;;)
        {
            const int raw = input.get();
            if (raw == EOF)
            {
                // EOF after a complete unquoted field is a valid final record. An
                // open quote or too few columns marks only the final record as torn.
                had_record = started;
                eof_torn = started && (quoted || fields.size() + 1 < COLUMN_COUNT);
                if (started && !eof_torn && !quoted)
                {
                    fields.push_back(field);
                }
                return !input.bad();
            }
            started = true;
            const char ch = static_cast<char>(raw);

            // Quoted fields retain delimiters and newlines; doubled quotes decode
            // to one literal quote.
            if (quoted)
            {
                if (ch != '"')
                {
                    field.push_back(ch);
                    continue;
                }
                if (input.peek() == '"')
                {
                    input.get();
                    field.push_back('"');
                    continue;
                }
                quoted = false;
                quote_closed = true;
                continue;
            }

            // Outside quotes, delimiters advance columns and either newline style
            // terminates the record. Bytes after a closing quote are invalid.
            if (ch == '"')
            {
                if (!field.empty() || quote_closed)
                {
                    return false;
                }
                quoted = true;
            }
            else if (ch == ',')
            {
                fields.push_back(field);
                field.clear();
                quote_closed = false;
            }
            else if (ch == '\n' || ch == '\r')
            {
                if (ch == '\r' && input.peek() == '\n')
                {
                    input.get();
                }
                fields.push_back(field);
                had_record = true;
                return true;
            }
            else if (quote_closed)
            {
                return false;
            }
            else
            {
                field.push_back(ch);
            }
        }
    }

    bool headerMatches(const std::vector<std::string>& fields)
    {
        static const std::vector<std::string> header{
            "conversation_id", "msg_id", "from_id", "from_name",
            "message", "dialog", "created_at"};
        return fields == header;
    }

    bool llsdString(const LLSD& map, const char* name, std::string& value)
    {
        const LLSD& field = map[name];
        if (!field.isString())
        {
            return false;
        }
        value = field.asString();
        return true;
    }

    bool inspectRegularFile(const std::string& path, bool& exists)
    {
        exists = false;
#if LL_WINDOWS
        const DWORD attributes = GetFileAttributesW(ll_convert<std::wstring>(path).c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES)
        {
            return GetLastError() == ERROR_FILE_NOT_FOUND ||
                   GetLastError() == ERROR_PATH_NOT_FOUND;
        }
        exists = true;
        return !(attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
#else
        struct stat status;
        if (::lstat(path.c_str(), &status) != 0)
        {
            return errno == ENOENT;
        }
        exists = true;
        return S_ISREG(status.st_mode);
#endif
    }
}

bool TimeUuidKey::operator==(const TimeUuidKey& rhs) const
{
    return ticks == rhs.ticks && tail == rhs.tail;
}

bool TimeUuidKey::operator<(const TimeUuidKey& rhs) const
{
    // Cassandra orders equal-time UUIDs by signed bytes 8-15, not UUID text.
    return ticks != rhs.ticks ? ticks < rhs.ticks : tail < rhs.tail;
}

std::string directConversationId(const LLUUID& agent_id, const LLUUID& resident_id)
{
    return agent_id < resident_id
        ? agent_id.asString() + "_" + resident_id.asString()
        : resident_id.asString() + "_" + agent_id.asString();
}

bool parseCanonicalUuid(const std::string& text, LLUUID& id)
{
    if (text.size() != 36 || text != utf8str_tolower(text) || !LLUUID::validate(text))
    {
        return false;
    }

    id.set(text, false);
    return id.asString() == text;
}

bool parseTimeUuid(const std::string& text, TimeUuidKey& key)
{
    LLUUID id;
    if (!parseCanonicalUuid(text, id) || (id.mData[6] >> 4) != 1 ||
        (id.mData[8] & 0xc0) != 0x80)
    {
        return false;
    }

    // UUIDv1 stores the 60-bit timestamp across three network-order fields.
    const U64 time_low = (U64(id.mData[0]) << 24) | (U64(id.mData[1]) << 16) |
                         (U64(id.mData[2]) << 8) | U64(id.mData[3]);
    const U64 time_mid = (U64(id.mData[4]) << 8) | U64(id.mData[5]);
    const U64 time_high = (U64(id.mData[6] & 0x0f) << 8) | U64(id.mData[7]);
    key.ticks = (time_high << 48) | (time_mid << 32) | time_low;

    for (U32 pos = 0; pos < key.tail.size(); ++pos)
    {
        key.tail[pos] = static_cast<S8>(id.mData[pos + 8]);
    }

    return true;
}

bool persistedDirectDialog(S32 dialog)
{
    switch (dialog)
    {
        case 0:
        case 3:
        case 4:
        case 20:
        case 22:
        case 26:
        case 38:
            return true;

        default:
            return false;
    }
}

bool sameDirectSenderName(const std::string& left, const std::string& right)
{
    if (left == right)
    {
        return true;
    }

    // Compare resident usernames even when the chat service supplies a legacy name.
    const std::string left_resident = LLCacheName::buildUsername(left);
    const std::string right_resident = LLCacheName::buildUsername(right);
    return !left_resident.empty() && left_resident == right_resident;
}

bool parseCreatedAt(const std::string& text, std::string& normalized)
{
    // Validate the complete wire shape before LLDate performs calendar conversion.
    static const U32 DIGIT_POSITIONS[] = {
        0, 1, 2, 3, 5, 6, 8, 9, 11, 12, 14, 15, 17, 18
    };
    if (text.size() < 19 || text[4] != '-' || text[7] != '-' || text[10] != 'T' ||
        text[13] != ':' || text[16] != ':')
    {
        return false;
    }

    // Reject signed, variable-width, and otherwise non-decimal date fields before
    // converting the fixed calendar components.
    for (U32 position : DIGIT_POSITIONS)
    {
        if (text[position] < '0' || text[position] > '9')
        {
            return false;
        }
    }

    const U32 year = std::stoi(text.substr(0, 4));
    const U32 month = std::stoi(text.substr(5, 2));
    const U32 day = std::stoi(text.substr(8, 2));
    const U32 hour = std::stoi(text.substr(11, 2));
    const U32 minute = std::stoi(text.substr(14, 2));
    const U32 second = std::stoi(text.substr(17, 2));
    static const U32 DAYS_PER_MONTH[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    if (!year || month < 1 || month > 12 || hour > 23 || minute > 59 || second > 59)
    {
        return false;
    }

    // Validate the day against the parsed month, including Gregorian leap years.
    U32 month_days = DAYS_PER_MONTH[month - 1];
    if (month == 2 && (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)))
    {
        ++month_days;
    }
    if (day < 1 || day > month_days)
    {
        return false;
    }

    // Fractional seconds are optional but must contain at least one digit.
    size_t position = 19;
    if (position < text.size() && text[position] == '.')
    {
        const size_t fraction = ++position;
        while (position < text.size() && text[position] >= '0' && text[position] <= '9')
        {
            ++position;
        }
        if (position == fraction)
        {
            return false;
        }
    }

    // Offset-less service timestamps are normalized to UTC. Explicit zones must
    // consume the complete suffix and remain within clock bounds.
    std::string parsed = text;
    if (position == text.size())
    {
        parsed += 'Z';
    }
    else if (text[position] == 'Z')
    {
        if (position + 1 != text.size())
        {
            return false;
        }
    }
    else if (text[position] == '+' || text[position] == '-')
    {
        if (position + 6 != text.size() || text[position + 3] != ':' ||
            !std::isdigit(static_cast<unsigned char>(text[position + 1])) ||
            !std::isdigit(static_cast<unsigned char>(text[position + 2])) ||
            !std::isdigit(static_cast<unsigned char>(text[position + 4])) ||
            !std::isdigit(static_cast<unsigned char>(text[position + 5])))
        {
            return false;
        }
        const U32 hours = (text[position + 1] - '0') * 10 + text[position + 2] - '0';
        const U32 minutes = (text[position + 4] - '0') * 10 + text[position + 5] - '0';
        if (hours > 23 || minutes > 59)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    LLDate date;
    if (!date.fromString(parsed))
    {
        return false;
    }

    normalized = parsed;
    return true;
}

bool validateConversationList(const LLSD& value, const LLUUID& agent_id,
                              std::vector<ListEntry>& entries)
{
    entries.clear();

    // Reject the complete discovery response before extracting any direct entries.
    if (!value.isArray() || value.size() > 10000)
    {
        return false;
    }

    std::set<LLUUID> residents;
    std::set<std::string> conversations;
    for (LLSD::array_const_iterator it = value.beginArray(); it != value.endArray(); ++it)
    {
        // Every list row must have a known type and bounded canonical identifiers,
        // even when non-direct rows are ignored by this feature.
        std::string type;
        std::string conversation;
        if (!it->isMap() || !llsdString(*it, "conversation_type", type) ||
            !llsdString(*it, "conversation_id", conversation) || conversation.empty() ||
            conversation.size() > 512 || !cleanText(conversation, 512) ||
            (type != "direct" && type != "group" && type != "adhoc"))
        {
            return false;
        }
        if (type != "direct")
        {
            continue;
        }

        // Direct entries must identify one unique peer and use the deterministic
        // account/resident conversation ID with a valid latest-message token.
        std::string resident_text;
        std::string token;
        LLUUID resident;
        TimeUuidKey token_key;
        if (!llsdString(*it, "other_participant_id", resident_text) ||
            !llsdString(*it, "last_msg_id", token) ||
            !parseCanonicalUuid(resident_text, resident) || resident.isNull() ||
            resident == agent_id ||
            !parseTimeUuid(token, token_key) ||
            conversation != directConversationId(agent_id, resident) ||
            !residents.insert(resident).second ||
            !conversations.insert(conversation).second)
        {
            return false;
        }

        entries.push_back({resident, conversation, token});
    }

    return true;
}

bool validateHistoryPage(const LLSD& value, const LLUUID& agent_id,
                         const LLUUID& resident_id,
                         const std::string& conversation_id,
                         const std::string& requested_cursor,
                         U64 deleted_before_ticks, Page& page)
{
    page = Page();

    // The service echoes the exact conversation and fixed page size. Coercible LLSD
    // types are rejected so paging state cannot drift across malformed responses.
    if (!value.isMap() || !value["conversation_id"].isString() ||
        value["conversation_id"].asString() != conversation_id ||
        !value["limit"].isInteger() || value["limit"].asInteger() != 100 ||
        !value["messages"].isArray() || value["messages"].size() > 100)
    {
        return false;
    }

    // Cursor echoes are exact: the head omits one, while older pages repeat the
    // requested UUID unchanged.
    const LLSD& echoed = value["before_msg_id"];
    if ((requested_cursor.empty() && !echoed.isUndefined()) ||
        (!requested_cursor.empty() &&
         (!echoed.isString() || echoed.asString() != requested_cursor)))
    {
        return false;
    }

    TimeUuidKey cursor_key;
    if (!requested_cursor.empty() && !parseTimeUuid(requested_cursor, cursor_key))
    {
        return false;
    }

    std::set<std::string> ids;
    TimeUuidKey previous;
    bool have_previous = false;

    // Validate every row before the page can affect scheduler or archive state.
    // UUIDv1 keys must be unique, strictly descending, and below the request cursor.
    for (LLSD::array_const_iterator it = value["messages"].beginArray();
         it != value["messages"].endArray(); ++it)
    {
        Row row;
        std::string from_text;
        if (!it->isMap() || !llsdString(*it, "conversation_id", row.conversation_id) ||
            !llsdString(*it, "msg_id", row.msg_id) ||
            !llsdString(*it, "from_id", from_text) ||
            !llsdString(*it, "from_name", row.from_name) ||
            !llsdString(*it, "message", row.message) ||
            !(*it)["dialog"].isInteger() ||
            !llsdString(*it, "created_at", row.created_at) ||
            row.conversation_id != conversation_id ||
            !parseTimeUuid(row.msg_id, row.key) || !ids.insert(row.msg_id).second ||
            !parseCanonicalUuid(from_text, row.from_id) ||
            (row.from_id != agent_id && row.from_id != resident_id) ||
            !cleanText(row.from_name, 256) || !cleanText(row.message, 1024) ||
            !persistedDirectDialog(row.dialog = (*it)["dialog"].asInteger()) ||
            !parseCreatedAt(row.created_at, row.created_at) ||
            (have_previous && !(row.key < previous)) ||
            (!requested_cursor.empty() && !(row.key < cursor_key)))
        {
            return false;
        }

        have_previous = true;
        previous = row.key;
        page.next_cursor = row.msg_id;

        // The account-wide cutoff is inclusive. Once reached, older rows from the
        // same validated page remain suppressed and older paging terminates.
        if (row.key.ticks <= deleted_before_ticks)
        {
            page.cutoff_reached = true;
        }
        else if (!page.cutoff_reached)
        {
            page.rows.push_back(row);
        }
    }

    page.terminal = value["messages"].size() == 0 || page.cutoff_reached;
    if (page.terminal)
    {
        page.next_cursor.clear();
    }

    return true;
}

std::string quoteCsv(const std::string& value)
{
    if (value.find_first_of(",\"\r\n") == std::string::npos)
    {
        return value;
    }

    std::string result("\"");
    for (char ch : value)
    {
        result.push_back(ch);
        if (ch == '"')
        {
            result.push_back('"');
        }
    }

    return result + '"';
}

void writeCsvRow(std::ostream& output, const Row& row)
{
    output << quoteCsv(row.conversation_id) << ',' << quoteCsv(row.msg_id) << ','
           << quoteCsv(row.from_id.asString()) << ',' << quoteCsv(row.from_name) << ','
           << quoteCsv(row.message) << ',' << row.dialog << ','
           << quoteCsv(row.created_at) << '\n';
}

bool archiveStamp(const std::string& path, U64& file_size, S64& file_mtime)
{
    bool exists = false;
    if (!inspectRegularFile(path, exists) || !exists)
    {
        return false;
    }

    std::error_code error;
    const std::filesystem::path native_path = fsyspath(path);
    const uintmax_t size = std::filesystem::file_size(native_path, error);
    if (error || size > std::numeric_limits<U64>::max())
    {
        return false;
    }

    const std::filesystem::file_time_type write_time =
        std::filesystem::last_write_time(native_path, error);
    if (error)
    {
        return false;
    }

    file_size = static_cast<U64>(size);
    file_mtime = static_cast<S64>(write_time.time_since_epoch().count());
    return true;
}

bool scanArchive(const std::string& path, const LLUUID& agent_id,
                 const LLUUID& resident_id, U64 deleted_before_ticks,
                 U32 display_cap, ArchiveScan& scan)
{
    scan = ArchiveScan();

    // Only a regular canonical path may participate in archive reads.
    bool exists = false;
    if (!inspectRegularFile(path, exists))
    {
        scan.state = ARCHIVE_FAILED;
        return false;
    }
    if (!exists)
    {
        scan.state = ARCHIVE_ABSENT;
        return true;
    }
    if (!archiveStamp(path, scan.file_size, scan.file_mtime))
    {
        scan.state = ARCHIVE_FAILED;
        return false;
    }
    llifstream input(path.c_str(), std::ios::binary);
    if (!input.is_open())
    {
        scan.state = ARCHIVE_FAILED;
        return false;
    }

    // The exact header is the first committed record and is never repairable data.
    std::vector<std::string> fields;
    bool torn = false;
    bool present = false;
    if (!readRecord(input, fields, torn, present) || !present || torn || !headerMatches(fields))
    {
        scan.state = ARCHIVE_CORRUPT;
        return false;
    }

    scan.valid_prefix_bytes = input.tellg();
    TimeUuidKey previous;
    bool have_previous = false;
    std::deque<Row> newest;

    // Fold records in ascending TimeUUID order while retaining only the newest
    // display_cap rows. Complete malformed records are corrupt; incomplete EOF is torn.
    for (;;)
    {
        if (!readRecord(input, fields, torn, present))
        {
            scan.state = ARCHIVE_CORRUPT;
            return false;
        }
        if (!present)
        {
            scan.state = torn ? ARCHIVE_TORN : ARCHIVE_VALID;
            break;
        }
        if (torn)
        {
            scan.state = ARCHIVE_TORN;
            break;
        }

        Row row;
        if (!rowFromFields(fields, agent_id, resident_id, row) ||
            (have_previous && !(previous < row.key)))
        {
            scan.state = ARCHIVE_CORRUPT;
            return false;
        }

        have_previous = true;
        previous = row.key;
        const std::streamoff position = input.tellg();
        scan.valid_prefix_bytes = position >= 0 ? position : scan.file_size;

        // Deleted rows still participate in structural and ordering validation but
        // do not enter the visible summary.
        if (row.key.ticks <= deleted_before_ticks)
        {
            continue;
        }

        if (!scan.has_oldest)
        {
            scan.has_oldest = true;
            scan.oldest = row.key;
        }
        scan.newest = row.key;
        ++scan.row_count;

        if (display_cap)
        {
            newest.push_back(row);
            if (newest.size() > display_cap)
            {
                newest.pop_front();
            }
        }
    }

    scan.display_rows.assign(newest.begin(), newest.end());
    return scan.state == ARCHIVE_VALID;
}
}

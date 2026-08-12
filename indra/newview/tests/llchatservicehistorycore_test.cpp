/**
 * @file llchatservicehistorycore_test.cpp
 * @brief Strict ChatService wire, TimeUUID, and archive tests.
 */
#include "linden_common.h"
#include "../test/lltut.h"
#include "../test/namedtempfile.h"
#include "../llchatservicehistorycore.h"

namespace tut
{
using namespace LLChatServiceHistoryCore;

struct chat_service_history_core {};
typedef test_group<chat_service_history_core> group_t;
typedef group_t::object object_t;
group_t group("LLChatServiceHistoryCore");

const LLUUID AGENT("00000000-0000-0000-0000-000000000001");
const LLUUID RESIDENT("00000000-0000-0000-0000-000000000002");
const char* const FIRST = "00000001-0000-1000-8000-000000000000";
const char* const SECOND = "00000002-0000-1000-8000-000000000000";

LLSD wireRow(const std::string& id, const LLUUID& from = RESIDENT)
{
    LLSD row;
    row["conversation_id"] = directConversationId(AGENT, RESIDENT);
    row["msg_id"] = id;
    row["from_id"] = from.asString();
    row["from_name"] = "Resident";
    row["message"] = "hello";
    row["dialog"] = 0;
    row["created_at"] = "2026-08-12T12:34:56.123Z";
    return row;
}

LLSD wirePage(const LLSD& rows, const std::string& cursor = std::string())
{
    LLSD page;
    page["conversation_id"] = directConversationId(AGENT, RESIDENT);
    page["limit"] = 100;
    page["messages"] = rows;
    if (!cursor.empty()) page["before_msg_id"] = cursor;
    return page;
}

template<> template<> void object_t::test<1>()
{
    ensure_equals("deterministic direct id", directConversationId(RESIDENT, AGENT),
                  AGENT.asString() + "_" + RESIDENT.asString());
}

template<> template<> void object_t::test<2>()
{
    TimeUuidKey first, second;
    ensure("UUIDv1 parses", parseTimeUuid(FIRST, first));
    ensure("timestamp ordering", parseTimeUuid(SECOND, second) && first < second);
    ensure("UUIDv4 rejected", !parseTimeUuid(
        "00000001-0000-4000-8000-000000000000", first));
    ensure("noncanonical UUID rejected", !parseTimeUuid(
        "00000001-0000-1000-8000-00000000000A", first));
}

template<> template<> void object_t::test<3>()
{
    TimeUuidKey low, high;
    parseTimeUuid(FIRST, low);
    parseTimeUuid("00000001-0000-1000-bfff-000000000000", high);
    ensure("Cassandra signed tail ordering", low < high);
}

template<> template<> void object_t::test<4>()
{
    LLSD rows = LLSD::emptyArray();
    rows.append(wireRow(SECOND));
    rows.append(wireRow(FIRST, AGENT));
    Page page;
    ensure("strict descending page", validateHistoryPage(
        wirePage(rows), AGENT, RESIDENT, directConversationId(AGENT, RESIDENT),
        "", 0, page));
    ensure_equals("all rows retained", page.rows.size(), size_t(2));
    ensure_equals("oldest cursor", page.next_cursor, std::string(FIRST));
}

template<> template<> void object_t::test<5>()
{
    LLSD rows = LLSD::emptyArray();
    rows.append(wireRow(FIRST));
    rows.append(wireRow(SECOND));
    Page page;
    ensure("ascending page rejected", !validateHistoryPage(
        wirePage(rows), AGENT, RESIDENT, directConversationId(AGENT, RESIDENT),
        "", 0, page));
    LLSD wrong = wirePage(LLSD::emptyArray());
    wrong["limit"] = "100";
    ensure("coercible type rejected", !validateHistoryPage(
        wrong, AGENT, RESIDENT, directConversationId(AGENT, RESIDENT), "", 0, page));
}

template<> template<> void object_t::test<6>()
{
    LLSD list = LLSD::emptyArray();
    LLSD entry;
    entry["conversation_type"] = "direct";
    entry["conversation_id"] = directConversationId(AGENT, RESIDENT);
    entry["other_participant_id"] = RESIDENT.asString();
    entry["last_msg_id"] = SECOND;
    list.append(entry);
    std::vector<ListEntry> entries;
    ensure("strict list accepted", validateConversationList(list, AGENT, entries));
    list.append(entry);
    ensure("duplicate resident rejected", !validateConversationList(list, AGENT, entries));
    LLSD null_list = LLSD::emptyArray();
    entry["other_participant_id"] = LLUUID::null.asString();
    entry["conversation_id"] = directConversationId(AGENT, LLUUID::null);
    null_list.append(entry);
    ensure("null resident rejected", !validateConversationList(null_list, AGENT, entries));
}

template<> template<> void object_t::test<7>()
{
    std::string normalized;
    ensure("UTC timestamp", parseCreatedAt("2026-08-12T12:34:56Z", normalized));
    ensure("offset timestamp", parseCreatedAt("2026-08-12T12:34:56+02:30", normalized));
    ensure("offset-less timestamp", parseCreatedAt("2026-08-12T12:34:56", normalized));
    ensure("fractional timestamp", parseCreatedAt("2026-08-12T12:34:56.123Z", normalized));
    ensure("reject trailing timestamp data",
           !parseCreatedAt("2026-08-12T12:34:56Z123", normalized));
    ensure("reject invalid offset", !parseCreatedAt("2026-08-12T12:34:56+99:99", normalized));
    ensure("reject signed date field", !parseCreatedAt("2026-+1-12T12:34:56Z", normalized));
    ensure("reject empty fraction", !parseCreatedAt("2026-08-12T12:34:56.Z", normalized));
    ensure("reject month", !parseCreatedAt("2026-13-12T12:34:56Z", normalized));
    ensure("reject day", !parseCreatedAt("2026-08-35T12:34:56Z", normalized));
    ensure("reject nonleap day", !parseCreatedAt("2026-02-29T12:34:56Z", normalized));
    ensure("accept leap day", parseCreatedAt("2024-02-29T12:34:56Z", normalized));
    ensure("reject hour", !parseCreatedAt("2026-08-12T24:34:56Z", normalized));
    ensure("reject minute", !parseCreatedAt("2026-08-12T12:60:56Z", normalized));
    ensure("reject second", !parseCreatedAt("2026-08-12T12:34:60Z", normalized));
    ensure("bad width", !parseCreatedAt("2026-8-12T12:34:56Z", normalized));
    ensure("dialog allowlist", persistedDirectDialog(38) && !persistedDirectDialog(99));
}

template<> template<> void object_t::test<8>()
{
    ensure_equals("RFC4180 quoting", quoteCsv("comma, quote \" and\nnewline"),
                  "\"comma, quote \"\" and\nnewline\"");
}

std::string validArchive()
{
    Row first, second;
    first.conversation_id = second.conversation_id = directConversationId(AGENT, RESIDENT);
    first.msg_id = FIRST;
    second.msg_id = SECOND;
    first.from_id = AGENT;
    second.from_id = RESIDENT;
    first.from_name = second.from_name = "Resident";
    first.message = "first";
    second.message = "second, quoted";
    first.created_at = second.created_at = "2026-08-12T12:34:56Z";
    parseTimeUuid(first.msg_id, first.key);
    parseTimeUuid(second.msg_id, second.key);
    std::ostringstream output;
    output << CSV_HEADER;
    writeCsvRow(output, first);
    writeCsvRow(output, second);
    return output.str();
}

template<> template<> void object_t::test<9>()
{
    NamedTempFile file("chatservice", validArchive());
    ArchiveScan scan;
    ensure("valid archive", scanArchive(file.getPath().string(), AGENT, RESIDENT, 0, 1, scan));
    ensure_equals("two summarized", scan.row_count, U32(2));
    ensure_equals("newest display cap", scan.display_rows.size(), size_t(1));
    ensure_equals("newest retained", scan.display_rows.front().msg_id, std::string(SECOND));
}

template<> template<> void object_t::test<10>()
{
    NamedTempFile file("chatservice", validArchive() + "\"unterminated");
    ArchiveScan scan;
    ensure("torn EOF rejected", !scanArchive(
        file.getPath().string(), AGENT, RESIDENT, 0, 0, scan));
    ensure_equals("torn distinguished", scan.state, ARCHIVE_TORN);
}

template<> template<> void object_t::test<11>()
{
    std::string content = validArchive();
    const std::string needle = ",0,2026-08-12T12:34:56Z\n";
    const size_t last = content.rfind(needle);
    content.replace(last, needle.size(), ",99,2026-08-12T12:34:56Z\n");
    NamedTempFile file("chatservice", content);
    ArchiveScan scan;
    ensure("complete malformed row rejected", !scanArchive(
        file.getPath().string(), AGENT, RESIDENT, 0, 0, scan));
    ensure_equals("corrupt distinguished", scan.state, ARCHIVE_CORRUPT);
}
}

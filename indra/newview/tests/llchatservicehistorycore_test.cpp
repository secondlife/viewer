/**
 * @file llchatservicehistorycore_test.cpp
 * @brief Strict ChatService wire, TimeUUID, and archive tests.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 * $/LicenseInfo$
 */
#include "linden_common.h"
#include "llsingleton.h"
#include "../test/lltut.h"
#include "../test/namedtempfile.h"
#include "../llchatservicehistorycore.h"
#include "../llmutelist.h"
#include <array>
#include "llsdutil.h"

struct LLMuteListTestAccess
{
    static bool server()
    {
        return LLMuteList::isServerAuthoritativeSource(LLMuteList::MLS_SERVER);
    }
    static bool serverEmpty()
    {
        return LLMuteList::isServerAuthoritativeSource(LLMuteList::MLS_SERVER_EMPTY);
    }
    static bool serverCache()
    {
        return LLMuteList::isServerAuthoritativeSource(LLMuteList::MLS_SERVER_CACHE);
    }
    static bool fallbackCache()
    {
        return LLMuteList::isServerAuthoritativeSource(LLMuteList::MLS_FALLBACK_CACHE);
    }
};

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
    if (!cursor.empty())
    {
        page["before_msg_id"] = cursor;
    }

    return page;
}

template<> template<> void object_t::test<1>()
{
    ensure_equals("deterministic direct id", directConversationId(RESIDENT, AGENT),
                  AGENT.asString() + "_" + RESIDENT.asString());
}

template<> template<> void object_t::test<2>()
{
    // UUID parsing accepts only canonical version-1 identifiers and preserves their
    // timestamp order.
    TimeUuidKey first;
    TimeUuidKey second;
    ensure("UUIDv1 parses", parseTimeUuid(FIRST, first));
    ensure("timestamp ordering", parseTimeUuid(SECOND, second) && first < second);

    ensure("UUIDv4 rejected", !parseTimeUuid(
        "00000001-0000-4000-8000-000000000000", first));
    ensure("noncanonical UUID rejected", !parseTimeUuid(
        "00000001-0000-1000-8000-00000000000A", first));
}

template<> template<> void object_t::test<3>()
{
    // Equal-timestamp tails follow Cassandra's signed-byte comparison.
    TimeUuidKey low;
    TimeUuidKey high;
    parseTimeUuid(FIRST, low);
    parseTimeUuid("00000001-0000-1000-bfff-000000000000", high);
    ensure("Cassandra signed tail ordering", low < high);
}

template<> template<> void object_t::test<4>()
{
    // A valid page is strictly descending and exposes its oldest row as the cursor.
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
    // Ordering and exact LLSD types are page-wide validation requirements.
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
    // Discovery rejects duplicate or null direct peers after accepting one canonical row.
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

    // Accept the supported UTC, explicit-offset, offset-less, and fractional forms.
    ensure("UTC timestamp", parseCreatedAt("2026-08-12T12:34:56Z", normalized));
    ensure("offset timestamp", parseCreatedAt("2026-08-12T12:34:56+02:30", normalized));
    ensure("offset-less timestamp", parseCreatedAt("2026-08-12T12:34:56", normalized));
    ensure("fractional timestamp", parseCreatedAt("2026-08-12T12:34:56.123Z", normalized));

    // Reject malformed suffixes and noncanonical field shapes.
    ensure("reject trailing timestamp data",
           !parseCreatedAt("2026-08-12T12:34:56Z123", normalized));
    ensure("reject invalid offset", !parseCreatedAt("2026-08-12T12:34:56+99:99", normalized));
    ensure("reject signed date field", !parseCreatedAt("2026-+1-12T12:34:56Z", normalized));
    ensure("reject empty fraction", !parseCreatedAt("2026-08-12T12:34:56.Z", normalized));

    // Enforce calendar and clock ranges after the fixed-width shape validates.
    ensure("reject month", !parseCreatedAt("2026-13-12T12:34:56Z", normalized));
    ensure("reject day", !parseCreatedAt("2026-08-35T12:34:56Z", normalized));
    ensure("reject nonleap day", !parseCreatedAt("2026-02-29T12:34:56Z", normalized));
    ensure("accept leap day", parseCreatedAt("2024-02-29T12:34:56Z", normalized));
    ensure("reject hour", !parseCreatedAt("2026-08-12T24:34:56Z", normalized));
    ensure("reject minute", !parseCreatedAt("2026-08-12T12:60:56Z", normalized));
    ensure("reject second", !parseCreatedAt("2026-08-12T12:34:60Z", normalized));
    ensure("bad width", !parseCreatedAt("2026-8-12T12:34:56Z", normalized));

    // Persisted history uses an explicit IM-dialog allowlist.
    ensure("dialog allowlist", persistedDirectDialog(38) && !persistedDirectDialog(99));
}

template<> template<> void object_t::test<8>()
{
    ensure_equals("RFC4180 quoting", quoteCsv("comma, quote \" and\nnewline"),
                  "\"comma, quote \"\" and\nnewline\"");
}

std::string validArchive()
{
    // Build a minimal ascending archive used by valid, torn, and corrupt scan cases.
    Row first;
    Row second;
    first.conversation_id = directConversationId(AGENT, RESIDENT);
    second.conversation_id = first.conversation_id;
    first.msg_id = FIRST;
    second.msg_id = SECOND;
    first.from_id = AGENT;
    second.from_id = RESIDENT;
    first.from_name = "Resident";
    second.from_name = first.from_name;
    first.message = "first";
    second.message = "second, quoted";
    first.created_at = "2026-08-12T12:34:56Z";
    second.created_at = first.created_at;
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
    TimeUuidKey oldest;
    parseTimeUuid(FIRST, oldest);

    ensure("valid archive", scanArchive(file.getPath().string(), AGENT, RESIDENT, 0, 1, scan));
    ensure_equals("two summarized", scan.row_count, U32(2));
    ensure("display cap preserves durable oldest", scan.oldest == oldest);
    ensure_equals("newest display cap", scan.display_rows.size(), size_t(1));
    ensure_equals("newest retained", scan.display_rows.front().msg_id, std::string(SECOND));
}

template<> template<> void object_t::test<10>()
{
    // An incomplete final quoted record is repairable torn-tail state.
    NamedTempFile file("chatservice", validArchive() + "\"unterminated");
    ArchiveScan scan;

    ensure("torn EOF rejected", !scanArchive(
        file.getPath().string(), AGENT, RESIDENT, 0, 0, scan));
    ensure_equals("torn distinguished", scan.state, ARCHIVE_TORN);
}

template<> template<> void object_t::test<11>()
{
    // A newline-terminated row with an unsupported dialog is complete corruption,
    // not a repairable EOF fragment.
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

template<> template<> void object_t::test<12>()
{
    // A full server response on the first login and a server-validated cache on
    // the next login must satisfy the same ChatService network prerequisite.
    ensure("full server response is authoritative", LLMuteListTestAccess::server());
    ensure("empty server response is authoritative", LLMuteListTestAccess::serverEmpty());
    ensure("warm-cache relog remains authoritative", LLMuteListTestAccess::serverCache());

    // A timeout fallback has not been validated by the server and remains closed.
    ensure("fallback cache remains degraded", !LLMuteListTestAccess::fallbackCache());
}

template<> template<> void object_t::test<13>()
{
    // Chat service legacy names and viewer resident names identify the same direct sender.
    ensure("legacy and resident names match",
           sameDirectSenderName("Bridie Linden", "bridie.linden"));
    ensure("different residents stay distinct",
           !sameDirectSenderName("bridie.linden", "beanie.tester"));
}

template<> template<> void object_t::test<14>()
{
    const F64 wall = 1000.0;
    const F64 daylight_utc = wall + 7.0 * 3600.0;

    // The earlier UTC-7 interpretation controls conservative seam admission.
    ensure("possible pre-boundary row retained",
           legacyWallMayPrecedeService(wall, daylight_utc + 1.0));
    ensure("row at boundary excluded",
           !legacyWallMayPrecedeService(wall, daylight_utc));
    ensure("post-boundary row excluded",
           !legacyWallMayPrecedeService(wall, daylight_utc - 1.0));

    // A boundary between the UTC-7 and UTC-8 interpretations remains ambiguous.
    ensure("ambiguous DST row retained",
           legacyWallMayPrecedeService(wall, daylight_utc + 30.0 * 60.0));
}

template<> template<> void object_t::test<15>()
{
    // Complete transcript names carry the resident identity inside parentheses.
    ensure("complete name matches legacy resident name",
           sameDirectSenderName("Bridie (bridie.linden)", "Bridie Linden"));
    ensure("complete name matches dotted resident name",
           sameDirectSenderName("Bridie (bridie.linden)", "bridie.linden"));
    ensure("single-name resident matches Resident suffix",
           sameDirectSenderName("beanie lin alt (danyanka)", "danyanka Resident"));
    ensure("resident names compare without case",
           sameDirectSenderName("BRIDIE.LINDEN", "Bridie Linden"));

    // Display-name changes and collisions must not change the compared identity.
    ensure("different display names for one resident match",
           sameDirectSenderName("Old Name (bridie.linden)", "New Name (bridie.linden)"));
    ensure("same display name for different residents stays distinct",
           !sameDirectSenderName("Bridie (bridie.linden)", "Bridie (danyanka)"));
    ensure("display name alone does not identify the resident",
           !sameDirectSenderName("Bridie", "Bridie (bridie.linden)"));
    ensure("missing names do not identify a resident", !sameDirectSenderName("", ""));
}

template<> template<> void object_t::test<16>()
{
    LLSD history;
    history["from_id"] = RESIDENT;
    history["message"] = "hello 1";
    history["timestamp"] = 1789005928;
    history["chat_service_msg_id"] = FIRST;

    // Offline delivery carries its original body independently of the localized notice.
    LLSD delivered = history;
    delivered.erase("chat_service_msg_id");
    delivered["message"] = "(Saved Wed Sep 09 22:05:28 2026)hello 1";
    delivered["chat_service_original_text"] = "hello 1";
    ensure("offline service copy matches delivered original", sameDirectHistoryOccurrence(history, delivered));
    delivered["message"] = "(Enregistre le mercredi)hello 1";
    ensure("offline notice localization does not affect identity", sameDirectHistoryOccurrence(history, delivered));
    delivered["message"] = "hello 1 (bonjour 1)";
    ensure("translation does not affect identity", sameDirectHistoryOccurrence(history, delivered));

    // Never infer original text by stripping something the sender may have typed.
    delivered["message"] = "(Saved Wed Sep 09 22:05:28 2026)hello 1";
    delivered["chat_service_original_text"] = delivered["message"];
    ensure("literal saved prefix remains message content", !sameDirectHistoryOccurrence(history, delivered));
    delivered.erase("chat_service_original_text");
    ensure("unidentified decorated text is not guessed", !sameDirectHistoryOccurrence(history, delivered));
    delivered["message"] = "hello 1";
    ensure("plain live delivery still matches", sameDirectHistoryOccurrence(history, delivered));
    delivered["from_id"] = AGENT;
    ensure("another sender stays distinct", !sameDirectHistoryOccurrence(history, delivered));
    delivered["from_id"] = RESIDENT;
    delivered["timestamp"] = 1789005988;
    ensure("another send minute stays distinct", !sameDirectHistoryOccurrence(history, delivered));
}

template<> template<> void object_t::test<17>()
{
    LLSD delivered;
    delivered["from_id"] = RESIDENT;
    delivered["from"] = "peppertest Resident";
    delivered["message"] = "(Saved Wed Sep 09 22:05:28 2026)hello 1";
    delivered["chat_service_original_text"] = "hello 1";
    delivered["timestamp"] = 1789005928;
    delivered["chat_service_log_timestamp"] = 1789092365;

    // An offline message delivered the next day has a plaintext receipt minute
    // that differs from its service send minute; both copies identify the same delivery.
    LLSD legacy;
    legacy["from"] = "peppertest";
    legacy["message"] = delivered["message"];
    const F64 logged_minute = (1789092365 / 60) * 60;
    legacy["chat_service_legacy_wall_time"] = logged_minute - 7.0 * 3600.0;
    ensure("plaintext matches the delivery minute", sameDirectHistoryOccurrence(legacy, delivered));
    legacy["chat_service_legacy_wall_time"] = logged_minute - 8.0 * 3600.0;
    ensure("plaintext accepts the standard-time SLT offset", sameDirectHistoryOccurrence(legacy, delivered));
    legacy["message"] = "hello 1";
    ensure("plaintext matches its logged body exactly", !sameDirectHistoryOccurrence(legacy, delivered));
    legacy["message"] = delivered["message"];
    legacy.erase("chat_service_legacy_wall_time");
    ensure("undated plaintext is not guessed", !sameDirectHistoryOccurrence(legacy, delivered));
}

template<> template<> void object_t::test<18>()
{
    auto row = [](const std::string& text, S32 timestamp)
    {
        return LLSD().with("message", text).with("timestamp", timestamp);
    };
    const std::list<LLSD> live{
        row("hello 3", 1789005932), row("hello 2", 1789005930), row("hello 1", 1789005928)
    };
    const std::list<LLSD> history{row("hello", 1789005955), row("older", 1789005800)};
    auto merged = interleaveDirectHistory(history, live);
    auto it = merged.begin();
    ensure_equals("fourth message follows the offline deliveries", (*it++)["message"].asString(), "hello");
    ensure_equals("third delivery keeps its place", (*it++)["message"].asString(), "hello 3");
    ensure_equals("second delivery keeps its place", (*it++)["message"].asString(), "hello 2");
    ensure_equals("first delivery keeps its place", (*it++)["message"].asString(), "hello 1");
    ensure_equals("older history precedes the live conversation", (*it++)["message"].asString(), "older");
    ensure("all occurrences retained", it == merged.end());

    // Timestamp ties keep the existing live row later; undated history stays at the start.
    merged = interleaveDirectHistory({row("tie", 1789005930), row("undated", 0)}, live);
    it = merged.begin();
    ensure_equals("newest live stays newest", (*it++)["message"].asString(), "hello 3");
    ensure_equals("live wins timestamp tie", (*it++)["message"].asString(), "hello 2");
    ensure_equals("equal-time history retains its occurrence", (*it++)["message"].asString(), "tie");
    ensure_equals("oldest live stays in delivery order", (*it++)["message"].asString(), "hello 1");
    ensure_equals("undated history remains before live rows", (*it++)["message"].asString(), "undated");
    ensure("interleaving leaves source lists unchanged", live.size() == 3 && history.size() == 2);
}

template<> template<> void object_t::test<19>()
{
    // A retained service row at :05 is a different occurrence from a new send at
    // :35, even when its sender, text, and minute are identical.
    LLSD history;
    history["from_id"] = RESIDENT;
    history["message"] = "ok";
    history["timestamp"] = 1789005905;
    history["chat_service_msg_id"] = FIRST;

    LLSD delivered = history;
    delivered.erase("chat_service_msg_id");
    delivered["timestamp"] = 1789005935;
    ensure("a later repeat in the same minute stays distinct",
           !sameDirectHistoryOccurrence(history, delivered));
    delivered["timestamp"] = 1789005904;
    ensure("a preceding send in the same minute stays distinct",
           !sameDirectHistoryOccurrence(history, delivered));
    delivered["timestamp"] = 1789005905;
    ensure("the matching service second reconciles",
           sameDirectHistoryOccurrence(history, delivered));

    // Missing precision provides no evidence for consuming a service occurrence.
    delivered["timestamp"] = 0;
    ensure("an untimed delivery stays distinct", !sameDirectHistoryOccurrence(history, delivered));
}

template<> template<> void object_t::test<20>()
{
    LLSD history;
    history["from_id"] = AGENT;
    history["message"] = "hello";
    history["timestamp"] = 1789005905;
    history["chat_service_msg_id"] = FIRST;

    // Outgoing echoes use a local clock; server persistence may occur in another second.
    LLSD echo = history;
    echo.erase("chat_service_msg_id");
    echo["chat_service_time_source"] = "local";
    echo["timestamp"] = 1789005904;
    ensure("local send before persistence reconciles", sameDirectHistoryOccurrence(history, echo));
    echo["timestamp"] = 1789005906;
    ensure("local clock ahead of the server reconciles", sameDirectHistoryOccurrence(history, echo));

    // Timestamp tolerance does not relax sender/body identity or extend past the minute.
    echo["message"] = "hello (bonjour)";
    echo["chat_service_original_text"] = "hello";
    ensure("translated local echo reconciles its original body", sameDirectHistoryOccurrence(history, echo));
    echo["chat_service_original_text"] = "another message";
    ensure("different local body stays distinct", !sameDirectHistoryOccurrence(history, echo));
    echo["chat_service_original_text"] = "hello";
    echo["from_id"] = RESIDENT;
    ensure("another sender stays distinct for local time", !sameDirectHistoryOccurrence(history, echo));
    echo["from_id"] = AGENT;
    echo["timestamp"] = 1789005965;
    ensure("local time in another minute stays distinct", !sameDirectHistoryOccurrence(history, echo));

    // Supplied timestamps use exact seconds even for the current agent's messages.
    echo["timestamp"] = 1789005906;
    echo["chat_service_time_source"] = "sent";
    ensure("supplied outgoing timestamp requires exact seconds", !sameDirectHistoryOccurrence(history, echo));
    echo["timestamp"] = 1789005905;
    ensure("supplied outgoing timestamp matches its service second", sameDirectHistoryOccurrence(history, echo));
    echo["chat_service_time_source"] = "local";
    history["timestamp"] = 0;
    ensure("local provenance does not make an untimed service row match", !sameDirectHistoryOccurrence(history, echo));
}

// Repeated incoming bodies share a plaintext minute but have distinct supplied seconds.
LLSD repeatedDelivery(S32 second)
{
    LLSD message;
    message["from_id"] = RESIDENT;
    message["from"] = "Example Resident";
    message["message"] = "ok";
    message["timestamp"] = 1789005900 + second;
    message["chat_service_log_timestamp"] = message["timestamp"];
    return message;
}

LLSD serviceCopy(const LLSD& delivery, const std::string& id)
{
    LLSD message = delivery;
    message["chat_service_msg_id"] = id;
    return message;
}

LLSD plaintextCopy(const LLSD& delivery)
{
    LLSD message;
    message["from"] = delivery["from"];
    message["message"] = delivery["message"];
    message["chat_service_legacy_wall_time"] =
        F64(delivery["timestamp"].asInteger() / 60 * 60) - 7.0 * 3600.0;
    return message;
}

template<> template<> void object_t::test<21>()
{
    const LLSD earlier = repeatedDelivery(5);
    const LLSD later = repeatedDelivery(35);
    const LLSD legacy = plaintextCopy(earlier);
    const LLSD service = serviceCopy(later, FIRST);

    // Partial coverage leaves plaintext before the only service row, which must
    // reserve the :35 delivery before the plaintext minute can consume it.
    for (bool history_reversed : {false, true})
    {
        for (bool live_reversed : {false, true})
        {
            std::list<LLSD> history{legacy, service};
            std::list<LLSD> live{later, earlier};
            if (history_reversed) history.reverse();
            if (live_reversed) live.reverse();
            ensure("partial service coverage reconciles both deliveries in either order",
                   filterDirectHistoryDuplicates(history, live).empty());
        }
    }
}

template<> template<> void object_t::test<22>()
{
    const LLSD earlier = repeatedDelivery(5);
    const LLSD later = repeatedDelivery(35);
    const LLSD legacy = plaintextCopy(earlier);
    LLSD keep_before = legacy;
    keep_before["message"] = "before";
    LLSD keep_after = legacy;
    keep_after["message"] = "after";
    const std::list<LLSD> history{legacy, keep_before, serviceCopy(later, FIRST), legacy, keep_after};
    const std::list<LLSD> live{later, earlier};

    // One exact reservation leaves only one occurrence for plaintext. Extra
    // identical history and unrelated rows remain in their original positions.
    const auto filtered = filterDirectHistoryDuplicates(history, live);
    ensure_equals("each live occurrence is consumed once", filtered.size(), size_t(3));
    auto row = filtered.begin();
    ensure_equals("first unmatched row keeps its position", (*row++)["message"].asString(), "before");
    ensure("extra repeated occurrence retains its plaintext source", !row->has("chat_service_msg_id"));
    ensure_equals("extra repeated occurrence remains", (*row++)["message"].asString(), "ok");
    ensure_equals("last unmatched row keeps its position", (*row++)["message"].asString(), "after");
    ensure_equals("history input is unchanged", history.size(), size_t(5));
    ensure_equals("live input is unchanged", live.size(), size_t(2));
    ensure_equals("no live rows leaves history intact", filterDirectHistoryDuplicates(history, {}).size(), history.size());
    ensure("no history leaves an empty result", filterDirectHistoryDuplicates({}, live).empty());
}

template<> template<> void object_t::test<23>()
{
    const LLSD supplied = repeatedDelivery(5);
    LLSD local = repeatedDelivery(34);
    local["chat_service_time_source"] = "local";
    const LLSD service_earlier = serviceCopy(supplied, FIRST);
    const LLSD service_later = serviceCopy(repeatedDelivery(35), SECOND);

    // A local-minute fallback must not take an exact supplied match's place,
    // regardless of the order of the service rows or live deliveries.
    for (bool history_reversed : {false, true})
    {
        for (bool live_reversed : {false, true})
        {
            std::list<LLSD> history{service_earlier, service_later};
            std::list<LLSD> live{local, supplied};
            if (history_reversed) history.reverse();
            if (live_reversed) live.reverse();
            ensure("exact service matches precede local-minute fallback",
                   filterDirectHistoryDuplicates(history, live).empty());
        }
    }
}

template<> template<> void object_t::test<24>()
{
    LLSD earlier = repeatedDelivery(5);
    LLSD later = repeatedDelivery(35);
    const LLSD service_earlier = serviceCopy(earlier, FIRST);
    const LLSD service_later = serviceCopy(later, SECOND);
    for (LLSD* delivery : {&earlier, &later})
    {
        (*delivery)["chat_service_original_text"] = "ok";
        (*delivery)["message"] = "ok (d'accord)";
    }
    const LLSD legacy = plaintextCopy(earlier);
    std::list<LLSD> history{legacy, service_later, legacy, service_earlier};
    const std::list<LLSD> live{later, earlier};

    // Translated or saved delivery can cover both its decorated plaintext and
    // raw service representation, with separate one-for-one consumption.
    ensure("decorated plaintext and raw service copies both reconcile",
           filterDirectHistoryDuplicates(history, live).empty());
    history.push_front(legacy);
    history.push_back(service_earlier);
    const auto filtered = filterDirectHistoryDuplicates(history, live);
    ensure_equals("extra occurrences in each representation remain", filtered.size(), size_t(2));
    ensure("extra decorated plaintext retains its source", !filtered.front().has("chat_service_msg_id"));
    ensure_equals("extra service retains its source", filtered.back()["chat_service_msg_id"].asString(), std::string(FIRST));
}

template<> template<> void object_t::test<25>()
{
    // Service creation can precede online delivery across a second boundary.
    LLSD history = serviceCopy(repeatedDelivery(0), FIRST);
    history["message"] = "123";
    history["datetime"] = "2026-09-10T17:17:00.904000Z";
    history["timestamp"] = S32(LLDate(history["datetime"].asString()).secondsSinceEpoch());
    LLSD delivered = history;
    delivered.erase("chat_service_msg_id");
    delivered["timestamp"] = 1789060621;
    delivered["chat_service_time_source"] = "receipt";
    ensure("online delivery reconciles the preceding service second",
           filterDirectHistoryDuplicates({history}, {delivered}).empty());

    // Only online delivery admits this delay; offline send times retain their precision.
    delivered["chat_service_time_source"] = "sent";
    ensure("offline send time stays exact", !sameDirectHistoryOccurrence(history, delivered));
    delivered["chat_service_time_source"] = "receipt";
    delivered["timestamp"] = 1789060619;
    ensure("delivery cannot precede service creation", !sameDirectHistoryOccurrence(history, delivered));
    delivered["timestamp"] = 1789060681;
    ensure("delivery outside the rolling window stays distinct", !sameDirectHistoryOccurrence(history, delivered));

    // Elapsed delay crosses minute boundaries, with exact sender and body checks.
    history["timestamp"] = 1789060619;
    delivered["timestamp"] = 1789060620;
    ensure("delivery delay crosses a minute boundary", sameDirectHistoryOccurrence(history, delivered));
    delivered["from_id"] = AGENT;
    ensure("delivery tolerance retains sender identity", !sameDirectHistoryOccurrence(history, delivered));
    delivered["from_id"] = RESIDENT;
    delivered["message"] = "1234";
    ensure("delivery tolerance retains body identity", !sameDirectHistoryOccurrence(history, delivered));
}

template<> template<> void object_t::test<26>()
{
    const LLSD earlier = serviceCopy(repeatedDelivery(0), FIRST);
    const LLSD later = serviceCopy(repeatedDelivery(1), SECOND);
    LLSD delivered = repeatedDelivery(1);
    delivered["chat_service_time_source"] = "receipt";

    // Receipt times do not identify a particular repeated send. Allocate the earliest
    // eligible occurrence, preserving the other occurrence even at the same second.
    for (bool reversed : {false, true})
    {
        std::list<LLSD> history{earlier, later};
        if (reversed) history.reverse();
        const auto filtered = filterDirectHistoryDuplicates(history, {delivered});
        ensure_equals("one live delivery consumes one service occurrence", filtered.size(), size_t(1));
        ensure_equals("the later repeated send survives", filtered.front()["chat_service_msg_id"].asString(), std::string(SECOND));
    }
}

template<> template<> void object_t::test<27>()
{
    const LLSD earlier = serviceCopy(repeatedDelivery(0), FIRST);
    const LLSD later = serviceCopy(repeatedDelivery(1), SECOND);
    LLSD first_delivery = repeatedDelivery(1);
    first_delivery["chat_service_time_source"] = "receipt";
    LLSD second_delivery = first_delivery;

    // Two deliveries in one second remain separate occurrences even when one
    // service creation crossed the boundary.
    for (bool reversed : {false, true})
    {
        std::list<LLSD> history{earlier, later};
        if (reversed) history.reverse();
        ensure("both adjacent service occurrences reconcile one-for-one",
               filterDirectHistoryDuplicates(history, {first_delivery, second_delivery}).empty());
    }

    // Delivery matches reserve their occurrence before local clocks and plaintext minutes.
    LLSD local = repeatedDelivery(0);
    local["chat_service_time_source"] = "local";
    const LLSD much_later = serviceCopy(repeatedDelivery(35), SECOND);
    ensure("delivery matching precedes local-minute matching",
           filterDirectHistoryDuplicates({earlier, much_later}, {local, first_delivery}).empty());
    const LLSD legacy = plaintextCopy(first_delivery);
    ensure("delivery matching precedes plaintext matching",
           filterDirectHistoryDuplicates({legacy, earlier}, {first_delivery, repeatedDelivery(35)}).empty());
}

template<> template<> void object_t::test<28>()
{
    // Receipt windows use elapsed UTC seconds across hour and day boundaries.
    for (const char* created : {"2026-09-10T12:59:59Z", "2026-09-10T23:59:59Z"})
    {
        LLSD history = serviceCopy(repeatedDelivery(0), FIRST);
        const S32 timestamp = S32(LLDate(created).secondsSinceEpoch());
        history["timestamp"] = timestamp;
        LLSD delivered = repeatedDelivery(0);
        delivered["chat_service_time_source"] = "receipt";
        for (S32 delay : {0, 2, 5, 30, 60})
        {
            delivered["timestamp"] = timestamp + delay;
            ensure("receipt inside the rolling window reconciles",
                   filterDirectHistoryDuplicates({history}, {delivered}).empty());
        }
        for (S32 delay : {-1, 61})
        {
            delivered["timestamp"] = timestamp + delay;
            ensure_equals("receipt outside the rolling window remains",
                          filterDirectHistoryDuplicates({history}, {delivered}).size(), size_t(1));
        }
    }
}

template<> template<> void object_t::test<29>()
{
    // Both sides can contain repeated bodies with different delays. Pair in time
    // order so a later delivery does not consume an older delivery's only candidate.
    for (const auto times : {std::array<S32, 4>{0, 15, 5, 30}, {0, 15, 30, 75}, {0, 10, 10, 65}})
    {
        const LLSD first = serviceCopy(repeatedDelivery(times[0]), FIRST);
        const LLSD second = serviceCopy(repeatedDelivery(times[1]), SECOND);
        LLSD first_delivery = repeatedDelivery(times[2]);
        LLSD second_delivery = repeatedDelivery(times[3]);
        first_delivery["chat_service_time_source"] = "receipt";
        second_delivery["chat_service_time_source"] = "receipt";
        for (bool history_reversed : {false, true})
        {
            for (bool live_reversed : {false, true})
            {
                std::list<LLSD> history{first, second};
                std::list<LLSD> live{first_delivery, second_delivery};
                if (history_reversed) history.reverse();
                if (live_reversed) live.reverse();
                ensure("delayed repeats reconcile independently of source order",
                       filterDirectHistoryDuplicates(history, live).empty());
            }
        }
    }
}

// Stable service IDs let the view retain context without guessing plaintext identity.
LLSD contextRow(U8 ordinal)
{
    LLUUID id(FIRST);
    id.mData[3] = ordinal;
    LLSD row = serviceCopy(repeatedDelivery(ordinal * 10), id.asString().c_str());
    row["message"] = std::to_string(ordinal);
    return row;
}

LLSD liveCopy(LLSD row)
{
    row.erase("chat_service_msg_id");
    row["is_history"] = false;
    return row;
}

template<> template<> void object_t::test<30>()
{
    const LLSD first = serviceCopy(repeatedDelivery(5), FIRST);
    const LLSD second = serviceCopy(repeatedDelivery(35), SECOND);
    const LLSD legacy = plaintextCopy(first);
    History history;
    history.setLoaded(mergeDirectHistory({legacy, legacy, legacy}, {first}));

    // Reapplying a preview must not consume the next identical plaintext occurrence.
    const Messages expected{legacy, first, second};
    for (int publication = 0; publication < 3; ++publication)
    {
        const auto result = history.compose({second, first}, {}, 10);
        ensure_equals("preview keeps all three occurrences", result.size(), expected.size());
        ensure("preview is idempotent and chronological",
               std::equal(result.begin(), result.end(), expected.begin(),
                          [](const LLSD& a, const LLSD& b) { return llsd_equals(a, b); }));
    }

    // Canonical ID identity wins over a stale preview, and limits follow reconciliation.
    LLSD stale = first;
    stale["message"] = "stale preview";
    const auto limited = history.compose({second, stale, second}, {}, 2);
    ensure_equals("duplicate IDs do not evict other history", limited.size(), size_t(2));
    ensure_equals("canonical value wins", limited.front()["message"].asString(), std::string("ok"));
    ensure_equals("service tail stays in TimeUUID order", limited.back()["chat_service_msg_id"].asString(), std::string(SECOND));
}

template<> template<> void object_t::test<31>()
{
    const Messages initial{contextRow(1), contextRow(2), contextRow(3)};
    const Messages bounded{contextRow(2), contextRow(3), contextRow(4)};
    const LLSD delivered = liveCopy(contextRow(4));
    History history;
    history.setLoaded(initial);
    history.compose({}, {}, 3, true);
    history.setLoaded(bounded);
    auto result = history.compose({}, {delivered}, 3, true);
    ensure_equals("live copy does not evict older visible context", result.size(), size_t(3));
    ensure_equals("oldest context survives rollover", result.front()["message"].asString(), std::string("1"));

    // A late delivery may replace any retained row, including the newest one.
    for (U8 ordinal : {2, 3})
    {
        history.clear();
        history.setLoaded(initial);
        history.compose({}, {}, 3, true);
        history.setLoaded(bounded);
        history.compose({}, {delivered}, 3, true);
        for (int publication = 0; publication < 3; ++publication)
        {
            result = history.compose({}, {liveCopy(contextRow(ordinal)), delivered}, 3, true);
            ensure_equals("late live delivery replaces one retained occurrence", result.size(), size_t(2));
            ensure_equals("unrelated context remains", result.front()["message"].asString(), std::string("1"));
        }
    }

    history.clear();
    history.setLoaded(initial);
    history.compose({}, {}, 3, true);
    history.setLoaded({contextRow(4), contextRow(5), contextRow(6)});
    const Messages live{liveCopy(contextRow(6)), liveCopy(contextRow(5)), delivered};
    ensure_equals("an all-live source retains open history", history.compose({}, live, 3, true).size(), size_t(3));
    ensure_equals("repeated all-live publication is stable", history.compose({}, live, 3, true).size(), size_t(3));
    ensure("hidden view releases old context", history.compose({}, live, 3).empty());
    history.setLoaded(initial);
    history.compose({}, {}, 3, true);
    history.setLoaded({});
    ensure("empty load clears retained context", history.compose({}, {}, 3, true).empty());

    // Revoking service consent leaves plaintext, while deletion clears both sources.
    const LLSD legacy = plaintextCopy(repeatedDelivery(5));
    history.setLoaded({legacy, contextRow(1)});
    history.compose({}, {}, 3, true);
    history.clearService();
    ensure_equals("service revocation keeps legacy", history.compose({}, {}, 3, true).size(), size_t(1));
    history.clear();
    ensure("clear removes every historical source", history.compose({}, {}, 3, true).empty());
}

template<> template<> void object_t::test<32>()
{
    LLSD offer;
    offer["notification_id"] = AGENT;
    offer["timestamp"] = 0;
    offer["is_history"] = false;
    LLSD first = liveCopy(contextRow(4));
    LLSD second = first;
    first["message"] = "test";
    second["message"] = "test";
    second["timestamp"] = first["timestamp"].asInteger() + 1;
    Messages current{second, first, offer};
    ensure("initial history changes the model", replaceHistory(current, {contextRow(1)}, true));
    ensure_equals("two live tests and the offer survive", current.size(), size_t(4));
    auto row = current.begin();
    ensure_equals("latest test stays live", (*row++)["message"].asString(), std::string("test"));
    ensure_equals("first test stays live", (*row++)["message"].asString(), std::string("test"));
    ensure_equals("inline offer identity survives", (*row)["notification_id"].asUUID(), AGENT);
    ensure("identical publication avoids replay", !replaceHistory(current, {contextRow(1)}, true));
    ensure("clearing history changes the model", replaceHistory(current, {}, true));
    ensure_equals("clearing history retains both live sends and offer", current.size(), size_t(3));
    S32 index = 3;
    for (const LLSD& message : current)
    {
        ensure_equals("indexes remain contiguous", message["index"].asInteger(), --index);
    }
}

template<> template<> void object_t::test<33>()
{
    constexpr U32 received = 1789060621;
    for (bool supplied : {false, true})
    {
        U32 timestamp = supplied ? received : 0;
        LLSD captured = captureLiveMessage("123", timestamp, incomingContext("123", true), received);
        ensure_equals("supplied and synthesized receipt times agree", timestamp, received);
        captured = captureLiveMessage("123 (translated)", timestamp, captured, received + 90);
        LLSD row = repeatedDelivery(0);
        row["timestamp"] = static_cast<S32>(timestamp);
        row["message"] = "123 (translated)";
        recordLiveMessage(row, captured, received + 90);
        ensure_equals("translation preserves original body", row["chat_service_original_text"].asString(), std::string("123"));
        ensure_equals("translation preserves receipt provenance", row["chat_service_time_source"].asString(), std::string("receipt"));
        ensure_equals("logging time follows translation completion", row["chat_service_log_timestamp"].asInteger(), S32(received + 90));
        LLSD service = serviceCopy(repeatedDelivery(0), FIRST);
        service["message"] = "123";
        service["timestamp"] = S32(received - 1);
        ensure("translation delay does not affect service matching", sameDirectHistoryOccurrence(service, row));
    }

    U32 timestamp = received - 86400;
    LLSD offline = captureLiveMessage("(Saved yesterday)123", timestamp, incomingContext("123", false), received);
    ensure_equals("offline delivery keeps the send time", timestamp, received - 86400);
    ensure_equals("offline time is authoritative", offline["chat_service_time_source"].asString(), std::string("sent"));
    timestamp = 0;
    offline = captureLiveMessage("(Saved)123", timestamp, incomingContext("123", false), received);
    ensure_equals("missing send time cannot become authoritative", offline["chat_service_time_source"].asString(), std::string("local"));
    timestamp = 0;
    LLSD echo = captureLiveMessage("123", timestamp, LLSD(), received);
    ensure_equals("outgoing echo captures its local clock", echo["chat_service_time_source"].asString(), std::string("local"));
    ensure_equals("outgoing echo captures before translation", timestamp, received);
}

template<> template<> void object_t::test<34>()
{
    const LLSD old = contextRow(1);
    const LLSD legacy = plaintextCopy(old);
    const LLSD live = liveCopy(contextRow(4));
    History history;
    history.setLoaded({legacy, contextRow(2), contextRow(3)});
    history.compose({}, {}, 3, true);
    history.setLoaded({contextRow(2), contextRow(3), contextRow(4)});
    for (int publication = 0; publication < 3; ++publication)
    {
        const auto result = history.compose({}, {live}, 3, true);
        ensure_equals("capped service rollover retains older plaintext", result.size(), size_t(3));
        ensure("retained plaintext keeps its source", !result.front().has("chat_service_msg_id"));
    }
    history.setLoaded({old, contextRow(2), contextRow(3)});
    auto result = history.compose({}, {live}, 3, true);
    ensure_equals("new archive row replaces retained plaintext", result.size(), size_t(3));
    ensure_equals("replaced plaintext acquires the canonical ID", result.front()["chat_service_msg_id"].asString(), old["chat_service_msg_id"].asString());

    // Each window may already have consumed a different member of repeated plaintext.
    const LLSD first = serviceCopy(repeatedDelivery(5), FIRST);
    const LLSD second = serviceCopy(repeatedDelivery(35), SECOND);
    const LLSD repeat = plaintextCopy(first);
    const Messages left = mergeDirectHistory({repeat, repeat, repeat}, {first});
    const Messages right = mergeDirectHistory({repeat, repeat, repeat}, {second});
    result = mergeDirectHistory(left, right);
    ensure_equals("overlapping windows preserve three original occurrences", result.size(), size_t(3));
    ensure_equals("repeated seam publication is stable", mergeDirectHistory(result, right).size(), size_t(3));
    ensure_equals("source window order preserves counts", mergeDirectHistory(right, left).size(), size_t(3));
}

}

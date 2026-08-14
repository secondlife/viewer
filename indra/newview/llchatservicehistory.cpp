/**
 * @file llchatservicehistory.cpp
 * @brief ChatService direct-IM history synchronization and stitched reads.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"

#include "llchatservicehistory.h"

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llcorehttputil.h"
#include "llcoros.h"
#include "lldate.h"
#include "lldir.h"
#include "lldiriterator.h"
#include "lleventcoro.h"
#include "llevents.h"
#include "llfile.h"
#include "llfloaterconversationpreview.h"
#include "llfloaterreg.h"
#include "llfloaterimsessiontab.h"
#include "llimview.h"
#include "lllogchat.h"
#include "llmutelist.h"
#include "llnotificationsutil.h"
#include "llsdserialize.h"
#include "lltimer.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "workqueue.h"

#if LL_WINDOWS
#include "llwin32headers.h"
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <deque>
#include <map>
#include <set>
#include <sstream>

// Keep exact-path legacy parsing behind LLLogChat's narrow stitching bridge.
struct LLChatServiceHistoryAccess
{
    static void loadLegacy(const std::string& path, std::list<LLSD>& messages,
                           const LLSD& parameters)
    {
        LLLogChat::loadChatHistoryExactUnchecked(path, messages, parameters);
    }
};

namespace
{
using namespace LLChatServiceHistoryCore;

const char* const LIST_CAP = "PersistentChatConversationsRequest";
const char* const HISTORY_CAP = "PersistentChatHistoryRequest";
const char* const ENABLED_SETTING = "ChatServiceEnabled";
const char* const INDEX_NAME = "chat_service_index.csv";
const char* const STATE_NAME = "chat_service_state.xml";
const char* const PENDING_NAME = "(name pending)";
const char* const LEGACY_WALL_TIME = "chat_service_legacy_wall_time";
const F64 REQUEST_SPACING = 2.1;
const F64 RETRY_DELAY = 5.0;
const F64 OUTBOUND_REFRESH_DELAY = 15.0;
const F64 RATE_LIMIT_DELAY = 120.0;
const F64 LIST_INTERVAL = 3600.0;
const F64 NAME_TIMEOUT = 30.0;
const U64 UUID_EPOCH = 122192928000000000ULL;
const U64 UUID_TICK_LIMIT = (1ULL << 60);
const U32 MAX_PASS_ROWS = 50000;
const size_t MAX_PASS_BYTES = 64 * 1024 * 1024;

enum EStateSafety
{
    STATE_UNKNOWN,
    STATE_SAFE,
    STATE_UNSAFE
};

enum EMetadata
{
    META_UNREQUESTED,
    META_PENDING,
    META_RESOLVED,
    META_FAILED
};

enum ESummary
{
    SUMMARY_ABSENT,
    SUMMARY_UNPREPARED,
    SUMMARY_VALID
};

enum EOutboundDiscovery
{
    OUTBOUND_NONE,
    OUTBOUND_PENDING,
    OUTBOUND_CONFIRMING
};

struct CapabilityContext
{
    LLUUID region_id;
    std::string list_url;
    std::string history_url;

    bool complete() const
    {
        return region_id.notNull() && !list_url.empty() && !history_url.empty();
    }

    bool operator==(const CapabilityContext& rhs) const
    {
        return region_id == rhs.region_id && list_url == rhs.list_url &&
               history_url == rhs.history_url;
    }

    bool operator!=(const CapabilityContext& rhs) const
    {
        return !(*this == rhs);
    }
};

struct Summary
{
    // Logical validity and retained-row count.
    ESummary state = SUMMARY_UNPREPARED;
    U32 rows = 0;
    bool has_rows = false;

    // Physical identity captured by the last successful scan or publication.
    bool file_exists = false;
    U64 file_size = 0;
    S64 file_mtime = 0;

    // Validated TimeUUID bounds for seam and append decisions.
    TimeUuidKey oldest;
    TimeUuidKey newest;
};

struct Metadata
{
    EMetadata state = META_UNREQUESTED;
    U32 attempt = 0;
    F64 deadline = 0.0;
    LLAvatarName name;
    boost::signals2::connection connection;
};

struct Resident
{
    // Discovery identity and the durable service range already covered.
    std::string conversation_id;
    std::string advertised_token;
    std::string covered_token;
    U32 covered_serial = 0;
    U32 archive_serial = 0;

    Summary summary;
    Metadata metadata;

    // Coalesced scheduler state for this resident's current or next pass.
    bool listed = false;
    bool force_head = false;
    bool forced_followup = false;
    bool first_request_started = false;
    bool retry_used = false;
    bool metadata_waiting = false;
    bool priority_waiting = false;

    // Outbound bursts share one quiet deadline; unknown conversations also retain
    // one bounded discovery confirmation.
    F64 outbound_refresh_due = 0.0;
    EOutboundDiscovery outbound_discovery = OUTBOUND_NONE;

    LLChatServiceHistory::Snapshot snapshot;
};

struct StateResult
{
    EStateSafety safety = STATE_UNSAFE;
    U64 boundary = 0;
    bool cleanup_pending = false;
};

struct HttpResult
{
    S32 status = 0;
    LLSD body;
};

struct Runtime
{
    // Account lifecycle and manager wake state.
    U32 epoch = 0;
    bool running = false;
    bool rollout = false;
    bool wake_pending = false;

    // Discovery and archive-maintenance state.
    bool initialized_archives = false;
    bool list_valid = false;
    bool list_needed = true;
    bool list_retry_used = false;

    // Deletion command and storage-maintenance state.
    bool delete_requested = false;
    bool delete_active = false;
    bool index_dirty = false;
    bool local_content_exists = false;

    // Durable deletion and privacy gates.
    U64 delete_click_ticks = 0;
    U64 deleted_before_ticks = 0;
    EStateSafety state_safety = STATE_UNKNOWN;
    bool cleanup_pending = false;

    // Manager-wide network deadlines.
    F64 network_not_before = 0.0;
    F64 next_list = 0.0;

    // Login-scoped capability, storage, and identity inputs.
    CapabilityContext context;
    std::string account_dir;
    std::string delimiter;
    LLUUID agent_id;

    // One active resident plus a coalesced priority/background queue.
    LLUUID active_resident;
    LLUUID priority_resident;
    std::deque<LLUUID> queue;
    std::map<LLUUID, Resident> residents;

    // Account-scoped completions and settings observers.
    LLChatServiceHistory::delete_callback_t delete_callback;
    boost::signals2::connection consent_connection;
    boost::signals2::connection show_history_connection;
};

// The main-thread manager owns all mutable scheduling and resident state for one
// login. Epoch checks fence coroutine and callback work across account changes.
Runtime sRuntime;

// General-queue filesystem jobs serialize with view scans and transcript deletion.
LLMutex sStorageMutex;
std::unique_ptr<LLEventMailDrop> sWake;
boost::signals2::signal<void(const LLUUID&, const LLChatServiceHistory::Snapshot&)> sSnapshotSignal;
boost::signals2::connection sRegionConnection;

bool ownsRuntime(U32 epoch)
{
    return sRuntime.running && sRuntime.epoch == epoch;
}

std::string accountPath(const std::string& filename)
{
    return gDirUtilp ? gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, filename)
                     : std::string();
}

std::string childPath(const std::string& directory, const std::string& delimiter,
                      const std::string& filename)
{
    if (directory.empty() || filename.empty())
    {
        return directory + filename;
    }
    if (directory.back() == '/' || directory.back() == '\\')
    {
        return directory + filename;
    }

    return directory + delimiter + filename;
}

std::string archiveName(const LLUUID& resident_id)
{
    return "chat_service_" + resident_id.asString() + ".csv";
}

std::string archivePath(const std::string& directory, const std::string& delimiter,
                        const LLUUID& resident_id)
{
    return childPath(directory, delimiter, archiveName(resident_id));
}

bool transcriptConsent()
{
    return gSavedPerAccountSettings.getS32("KeepConversationLogTranscripts") > 1;
}

CapabilityContext sampleContext()
{
    CapabilityContext result;
    LLViewerRegion* region = gAgent.getRegion();
    if (region && region->capabilitiesReceived())
    {
        result.region_id = region->getRegionID();
        result.list_url = region->getCapability(LIST_CAP);
        result.history_url = region->getCapability(HISTORY_CAP);
    }
    return result;
}

void wakeManager()
{
    if (sRuntime.running && sWake && !sRuntime.wake_pending)
    {
        sRuntime.wake_pending = true;
        sWake->post(LLSD(true));
    }
}

void publishSnapshot(const LLUUID& id, Resident& resident)
{
    // Recompute account-wide presentation permission at publication time so views
    // cannot retain service rows after consent, state, or deletion gates change.
    resident.snapshot.archive_serial = resident.archive_serial;
    resident.snapshot.metadata_resolved = resident.metadata.state == META_RESOLVED;
    if (resident.snapshot.metadata_resolved)
    {
        resident.snapshot.metadata = resident.metadata.name;
    }
    resident.snapshot.service_presentation_allowed =
        sRuntime.rollout && transcriptConsent() &&
        sRuntime.state_safety == STATE_SAFE && !sRuntime.cleanup_pending &&
        !sRuntime.delete_requested;
    sSnapshotSignal(id, resident.snapshot);
}

void setWorkActive(const LLUUID& id, bool active)
{
    Resident& resident = sRuntime.residents[id];
    if (resident.snapshot.service_work_active != active)
    {
        resident.snapshot.service_work_active = active;
        publishSnapshot(id, resident);
    }
}

void queueResident(const LLUUID& id, bool priority)
{
    // One resident has at most one queued occurrence. A fresh priority trigger moves
    // that occurrence to the front without bypassing global request pacing.
    sRuntime.queue.erase(std::remove(sRuntime.queue.begin(), sRuntime.queue.end(), id),
                         sRuntime.queue.end());
    if (priority)
    {
        sRuntime.queue.push_front(id);
    }
    else
    {
        sRuntime.queue.push_back(id);
    }
}

void clearPriority(const LLUUID& id)
{
    if (sRuntime.priority_resident == id)
    {
        sRuntime.priority_resident.setNull();
    }
}

void failResidentPass(const LLUUID& id, Resident& resident)
{
    resident.snapshot.head_preview.clear();
    resident.snapshot.service_work_active = false;
    publishSnapshot(id, resident);
    clearPriority(id);
}

bool parseDecimalTicks(const std::string& text, U64& value)
{
    // Privacy cutoffs use one canonical unsigned decimal form bounded to UUIDv1's
    // 60-bit timestamp field.
    if (text.empty() || (text.size() > 1 && text[0] == '0') ||
        text.find_first_not_of("0123456789") != std::string::npos)
    {
        return false;
    }
    U64 parsed = 0;
    for (char ch : text)
    {
        const U64 digit = static_cast<U64>(ch - '0');
        if (parsed > (UUID_TICK_LIMIT - 1 - digit) / 10)
        {
            return false;
        }
        parsed = parsed * 10 + digit;
    }
    value = parsed;
    return true;
}

bool inspectRegular(const std::string& path, bool& exists)
{
    // Owned artifacts must be regular files; directories and reparse/symlink paths
    // fail inspection rather than being followed.
    exists = false;
#if LL_WINDOWS
    const std::wstring wide = ll_convert<std::wstring>(path);
    const DWORD attributes = GetFileAttributesW(wide.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        return GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND;
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

bool syncDirectory(const std::string& path)
{
#if LL_WINDOWS
    return true;
#else
    const std::string::size_type separator = path.find_last_of("/\\");
    if (separator == std::string::npos)
    {
        return false;
    }

    const std::string directory = path.substr(0, separator);
    const int descriptor = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY);
    if (descriptor < 0)
    {
        return false;
    }
    const bool success = ::fsync(descriptor) == 0;
    ::close(descriptor);
    return success;
#endif
}

bool syncFile(const std::string& path)
{
#if LL_WINDOWS
    HANDLE handle = CreateFileW(ll_convert<std::wstring>(path).c_str(), GENERIC_WRITE,
                                FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE || !FlushFileBuffers(handle))
    {
        if (handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(handle);
        }
        return false;
    }
    CloseHandle(handle);
    return true;
#else
    const int descriptor = ::open(path.c_str(), O_RDONLY);
    if (descriptor < 0 || ::fsync(descriptor) != 0)
    {
        if (descriptor >= 0)
        {
            ::close(descriptor);
        }
        return false;
    }
    ::close(descriptor);
    return true;
#endif
}

bool writeReplace(const std::string& destination, const std::string& contents, bool durable)
{
    // Write a complete sibling temporary before one atomic replacement. Privacy
    // state additionally flushes file and directory metadata before returning.
    const std::string temporary = destination + ".tmp";
    bool exists = false;
    if (!inspectRegular(destination, exists) || !inspectRegular(temporary, exists))
    {
        return false;
    }
    llofstream output(temporary.c_str(), std::ios::binary | std::ios::trunc);
    if (!output.is_open())
    {
        return false;
    }
    output.write(contents.data(), contents.size());
    output.flush();
    output.close();
    if (output.fail())
    {
        return false;
    }

    // Privacy state is flushed before replacement; its directory entry is committed afterward.
    if (durable)
    {
        if (!syncFile(temporary))
        {
            return false;
        }
    }
#if LL_WINDOWS
    const std::wstring from = ll_convert<std::wstring>(temporary);
    const std::wstring to = ll_convert<std::wstring>(destination);
    if (!MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING |
                                               (durable ? MOVEFILE_WRITE_THROUGH : 0)))
    {
        return false;
    }
#else
    if (::rename(temporary.c_str(), destination.c_str()) != 0)
    {
        return false;
    }
#endif
    return !durable || syncDirectory(destination);
}

bool replacePrefix(const std::string& path, std::streamoff bytes)
{
    // Torn-tail repair copies only the last parser-confirmed prefix, then replaces
    // the canonical without interpreting or rewriting valid records.
    const std::string temporary = path + ".tmp";
    bool exists = false;
    if (bytes < 0 || !inspectRegular(path, exists) || !exists ||
        !inspectRegular(temporary, exists))
    {
        return false;
    }

    llifstream input(path.c_str(), std::ios::binary);
    llofstream output(temporary.c_str(), std::ios::binary | std::ios::trunc);
    std::array<char, 64 * 1024> buffer;
    std::streamoff remaining = bytes;
    while (input.is_open() && output.is_open() && remaining > 0)
    {
        const std::streamsize count = static_cast<std::streamsize>(
            llmin<std::streamoff>(remaining, buffer.size()));
        input.read(buffer.data(), count);
        if (input.gcount() != count)
        {
            return false;
        }
        output.write(buffer.data(), count);
        remaining -= count;
    }
    output.flush();
    output.close();
    const bool input_ok = input.is_open() && !input.bad();
    input.close();
    if (!input_ok || output.fail() || remaining)
    {
        return false;
    }
#if LL_WINDOWS
    const std::wstring from = ll_convert<std::wstring>(temporary);
    const std::wstring to = ll_convert<std::wstring>(path);
    return MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
#else
    return ::rename(temporary.c_str(), path.c_str()) == 0;
#endif
}

bool readStateFile(const std::string& path, StateResult& state)
{
    bool exists = false;
    if (!inspectRegular(path, exists) || !exists)
    {
        return false;
    }
    llifstream input(path.c_str(), std::ios::binary);
    LLSD data;
    if (!input.is_open())
    {
        return false;
    }

    // Privacy state accepts exactly the two canonical fields and no trailing bytes.
    const S32 parsed = LLSDSerialize::fromXMLDocument(data, input, false);
    input.clear();
    input >> std::ws;
    if (parsed == LLSDParser::PARSE_FAILURE || input.peek() != EOF ||
        !data.isMap() || data.size() != 2 ||
        !data["deleted_before_uuid_ticks"].isString() ||
        !data["cleanup_pending"].isBoolean() ||
        !parseDecimalTicks(data["deleted_before_uuid_ticks"].asString(), state.boundary))
    {
        return false;
    }
    state.safety = STATE_SAFE;
    state.cleanup_pending = data["cleanup_pending"].asBoolean();
    return true;
}

StateResult loadState(const std::string& path)
{
    // A valid canonical state is authoritative. Any ambiguous canonical/temporary
    // combination fails closed until deletion recovery publishes a new state.
    StateResult result;
    const std::string temporary = path + ".tmp";
    bool canonical_exists = false;
    bool temporary_exists = false;
    const bool canonical_safe = inspectRegular(path, canonical_exists);
    const bool temporary_safe = inspectRegular(temporary, temporary_exists);
    if (canonical_safe && canonical_exists && readStateFile(path, result))
    {
        // A committed canonical is authoritative; discard any interrupted replacement.
        if ((!temporary_safe || temporary_exists) &&
            ((LLFile::remove(temporary, ENOENT) != 0 && errno != ENOENT) ||
             !syncDirectory(path)))
        {
            result.safety = STATE_UNSAFE;
        }
        return result;
    }
    if (canonical_safe && temporary_safe && !canonical_exists && !temporary_exists)
    {
        result.safety = STATE_SAFE;
        return result;
    }
    result.safety = STATE_UNSAFE;
    return result;
}

U64 recoverBoundaryCandidate(const std::string& path)
{
    bool exists = false;
    if (!inspectRegular(path, exists) || !exists)
    {
        return 0;
    }

    // Recovery salvages only a canonical prior cutoff; malformed surrounding state
    // cannot weaken the newer click-time boundary.
    llifstream input(path.c_str(), std::ios::binary);
    LLSD data;
    U64 boundary = 0;
    if (input.is_open() &&
        LLSDSerialize::fromXML(data, input, false) != LLSDParser::PARSE_FAILURE &&
        data.isMap() && data["deleted_before_uuid_ticks"].isString())
    {
        parseDecimalTicks(data["deleted_before_uuid_ticks"].asString(), boundary);
    }
    return boundary;
}

bool recoverStateForDelete(const std::string& path, U64 boundary)
{
    // Build a durable pending state through an integration-owned recovery path so
    // an unsafe canonical remains present until its replacement is ready.
    const std::string temporary = path + ".tmp";
    const std::string::size_type separator = path.find_last_of("/\\");
    const std::string recovery = path.substr(0, separator + 1) + INDEX_NAME + ".tmp";
    LLSD data;
    data["deleted_before_uuid_ticks"] = std::to_string(boundary);
    data["cleanup_pending"] = true;
    std::ostringstream serialized;
    if (!LLSDSerialize::toPrettyXML(data, serialized))
    {
        return false;
    }

    // The index temporary is an owned scratch name that deletion already knows how
    // to sweep; it cannot be mistaken for authoritative privacy state.
    if (LLFile::remove(recovery, ENOENT) != 0 && errno != ENOENT)
    {
        return false;
    }

    llofstream output(recovery.c_str(), std::ios::binary | std::ios::trunc);
    if (!output.is_open())
    {
        return false;
    }
    output << serialized.str();
    output.flush();
    output.close();
    if (output.fail())
    {
        return false;
    }
    if (!syncFile(recovery))
    {
        return false;
    }
#if LL_WINDOWS
    if (!MoveFileExW(ll_convert<std::wstring>(recovery).c_str(),
                     ll_convert<std::wstring>(temporary).c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        return false;
    }
#else
    if (::rename(recovery.c_str(), temporary.c_str()) != 0 ||
        !syncDirectory(temporary))
    {
        return false;
    }
#endif
    if (LLFile::remove(path, ENOENT) != 0 && errno != ENOENT)
    {
        return false;
    }
#if LL_WINDOWS
    if (!MoveFileExW(ll_convert<std::wstring>(temporary).c_str(),
                     ll_convert<std::wstring>(path).c_str(), MOVEFILE_WRITE_THROUGH))
    {
        return false;
    }
#else
    if (::rename(temporary.c_str(), path.c_str()) != 0 || !syncDirectory(path))
    {
        return false;
    }
#endif
    return true;
}

bool writeState(const std::string& path, U64 boundary, bool pending)
{
    LLSD data;
    data["deleted_before_uuid_ticks"] = std::to_string(boundary);
    data["cleanup_pending"] = pending;
    std::ostringstream serialized;
    if (!LLSDSerialize::toPrettyXML(data, serialized))
    {
        return false;
    }
    return writeReplace(path, serialized.str(), true);
}

bool clearUnsafeStateTemporary(const std::string& path)
{
    const std::string temporary = path + ".tmp";
    bool exists = false;
    if (inspectRegular(temporary, exists))
    {
        return true;
    }

    // A nonregular temporary is never authoritative and must be removed before a
    // normal durable replacement can reuse its exact owned path.
    if (LLFile::remove(temporary, ENOENT) != 0 && errno != ENOENT)
    {
        return false;
    }
    if (!inspectRegular(temporary, exists) || exists)
    {
        return false;
    }

    return syncDirectory(path);
}

bool canonicalArchiveName(const std::string& name, LLUUID& resident_id)
{
    const std::string prefix = "chat_service_";
    const std::string suffix = ".csv";
    if (name.size() != prefix.size() + 36 + suffix.size() ||
        name.compare(0, prefix.size(), prefix) ||
        name.compare(name.size() - suffix.size(), suffix.size(), suffix))
    {
        return false;
    }
    return parseCanonicalUuid(name.substr(prefix.size(), 36), resident_id);
}

bool ownedArtifactName(const std::string& name)
{
    if (name == INDEX_NAME || name == std::string(INDEX_NAME) + ".tmp" ||
        name == STATE_NAME || name == std::string(STATE_NAME) + ".tmp")
    {
        return true;
    }
    const std::string prefix = "chat_service_";
    if (name.size() <= prefix.size() + 36 || name.compare(0, prefix.size(), prefix))
    {
        return false;
    }
    LLUUID id;
    if (!parseCanonicalUuid(name.substr(prefix.size(), 36), id))
    {
        return false;
    }
    const std::string suffix = name.substr(prefix.size() + 36);
    return suffix == ".csv" || suffix == ".csv.tmp" || suffix == ".csv.corrupt";
}

Summary summaryFromScan(const ArchiveScan& scan)
{
    Summary result;
    result.state = scan.state == ARCHIVE_ABSENT ? SUMMARY_ABSENT : SUMMARY_VALID;
    result.rows = scan.row_count;
    result.has_rows = scan.has_oldest;
    result.file_exists = scan.state != ARCHIVE_ABSENT;
    result.file_size = scan.file_size;
    result.file_mtime = scan.file_mtime;
    result.oldest = scan.oldest;
    result.newest = scan.newest;
    return result;
}

bool prepareArchive(const std::string& path, const LLUUID& resident_id, const LLUUID& agent_id,
                    U64 boundary, Summary& summary, bool& bytes_changed)
{
    LLMutexLock lock(&sStorageMutex);

    // Classify the canonical under the same lock used by mutation and view scans.
    bool canonical_exists = false;
    if (!inspectRegular(path, canonical_exists))
    {
        return false;
    }

    ArchiveScan scan;
    if (scanArchive(path, agent_id, resident_id, boundary, 0, scan))
    {
        summary = summaryFromScan(scan);
        return true;
    }
    if (scan.state == ARCHIVE_TORN)
    {
        // Repair only when the private physical stamp still matches the classified
        // file, then rescan the replacement before publishing its summary.
        U64 current_size = 0;
        S64 current_mtime = 0;
        if (!archiveStamp(path, current_size, current_mtime) ||
            current_size != scan.file_size || current_mtime != scan.file_mtime ||
            !replacePrefix(path, scan.valid_prefix_bytes) ||
            !scanArchive(path, agent_id, resident_id, boundary, 0, scan))
        {
            summary.state = SUMMARY_UNPREPARED;
            return false;
        }
        bytes_changed = true;
        summary = summaryFromScan(scan);
        return true;
    }
    if (scan.state != ARCHIVE_CORRUPT)
    {
        summary.state = SUMMARY_UNPREPARED;
        return false;
    }

    // Preserve one structurally unusable canonical before a later full-window rebuild.
    const std::string corrupt = path + ".corrupt";
    bool corrupt_exists = false;
    if (!inspectRegular(corrupt, corrupt_exists))
    {
        if (LLFile::remove(corrupt, ENOENT) != 0 && errno != ENOENT)
        {
            return false;
        }
        if (!inspectRegular(corrupt, corrupt_exists) || corrupt_exists)
        {
            return false;
        }
    }
    if (LLFile::remove(corrupt, ENOENT) != 0 && errno != ENOENT)
    {
        summary.state = SUMMARY_UNPREPARED;
        return false;
    }
    U64 current_size = 0;
    S64 current_mtime = 0;
    if (!archiveStamp(path, current_size, current_mtime) ||
        current_size != scan.file_size || current_mtime != scan.file_mtime ||
        LLFile::rename(path, corrupt) != 0)
    {
        summary.state = SUMMARY_UNPREPARED;
        return false;
    }
    bytes_changed = true;
    summary = Summary();
    summary.state = SUMMARY_ABSENT;
    return true;
}

bool publishRows(const std::string& path, const std::vector<Row>& rows,
                 bool append, Summary& resulting)
{
    LLMutexLock lock(&sStorageMutex);
    if (rows.empty())
    {
        return true;
    }

    // The captured summary is a private physical identity check. External changes
    // abort publication instead of reconciling unknown bytes.
    bool exists = false;
    if (!inspectRegular(path, exists) || exists != resulting.file_exists)
    {
        return false;
    }
    if (exists)
    {
        U64 file_size = 0;
        S64 file_mtime = 0;
        if (!archiveStamp(path, file_size, file_mtime) ||
            file_size != resulting.file_size || file_mtime != resulting.file_mtime)
        {
            return false;
        }
    }

    // New and rebuilt archives publish one complete ascending CSV. Existing valid
    // archives append only the fully validated newer stage.
    if (!append)
    {
        std::ostringstream output;
        output << CSV_HEADER;
        for (const Row& row : rows)
        {
            writeCsvRow(output, row);
        }
        if (!writeReplace(path, output.str(), false))
        {
            return false;
        }
    }
    else
    {
        llstat status;
        bool needs_separator = false;
        if (LLFile::stat(path, &status) != 0 || !LLFile::isfile(path))
        {
            return false;
        }
        if (status.st_size)
        {
            llifstream input(path.c_str(), std::ios::binary);
            input.seekg(-1, std::ios::end);
            needs_separator = input.get() != '\n';
        }
        llofstream output(path.c_str(), std::ios::binary | std::ios::app);
        if (!output.is_open())
        {
            return false;
        }
        if (needs_separator)
        {
            output << '\n';
        }
        for (const Row& row : rows)
        {
            writeCsvRow(output, row);
        }
        output.flush();
        output.close();
        if (output.fail())
        {
            return false;
        }
    }

    // Advance the in-memory summary only after the filesystem mutation succeeds.
    resulting.state = SUMMARY_VALID;
    resulting.rows += static_cast<U32>(rows.size());
    if (!resulting.has_rows)
    {
        resulting.oldest = rows.front().key;
        resulting.has_rows = true;
    }
    resulting.newest = rows.back().key;
    resulting.file_exists = true;
    return archiveStamp(path, resulting.file_size, resulting.file_mtime);
}

std::vector<LLUUID> enumerateArchives(const std::string& directory,
                                      const std::string& delimiter)
{
    std::vector<LLUUID> result;
    LLDirIterator iterator(directory, "chat_service_*.csv");
    std::string name;
    while (iterator.next(name))
    {
        LLUUID id;
        if (canonicalArchiveName(name, id))
        {
            result.push_back(id);
        }
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

bool hasOwnedContent(const std::string& directory)
{
    LLDirIterator iterator(directory, "chat_service_*");
    std::string name;
    while (iterator.next(name))
    {
        if (ownedArtifactName(name) && name != INDEX_NAME &&
            name != std::string(INDEX_NAME) + ".tmp" && name != STATE_NAME &&
            name != std::string(STATE_NAME) + ".tmp")
        {
            return true;
        }
    }
    return false;
}

bool regenerateIndex(const std::vector<LLUUID>& ids,
                     const std::map<LLUUID, LLAvatarName>& names,
                     const std::string& directory, const std::string& delimiter)
{
    LLMutexLock lock(&sStorageMutex);

    // Remove unsafe index paths before rebuilding the non-authoritative manifest.
    const std::string index = childPath(directory, delimiter, INDEX_NAME);
    const std::string temporary = index + ".tmp";
    for (const std::string& path : { index, temporary })
    {
        bool exists = false;
        const bool regular = inspectRegular(path, exists);
        if (!regular || (path == temporary && exists))
        {
            if (LLFile::remove(path, ENOENT) != 0 && errno != ENOENT)
            {
                return false;
            }
            if (!inspectRegular(path, exists) || exists)
            {
                return false;
            }
        }
    }

    // Include exactly the valid canonical archives still present at regeneration.
    std::vector<std::pair<std::string, LLUUID>> archives;
    for (const LLUUID& id : ids)
    {
        bool exists = false;
        if (inspectRegular(archivePath(directory, delimiter, id), exists) && exists)
        {
            archives.emplace_back(archiveName(id), id);
        }
    }
    if (archives.empty())
    {
        const int removed = LLFile::remove(index, ENOENT);
        return removed == 0 || errno == ENOENT;
    }

    // Pending metadata remains explicit and is replaced on a later regeneration.
    std::ostringstream output;
    output << INDEX_HEADER;
    for (const auto& archive : archives)
    {
        const auto found = names.find(archive.second);
        const bool resolved = found != names.end();
        output << quoteCsv(archive.first) << ',' << archive.second.asString() << ','
               << quoteCsv(resolved ? found->second.getAccountName() : PENDING_NAME) << ','
               << quoteCsv(resolved ? found->second.getDisplayName() : PENDING_NAME) << '\n';
    }
    return writeReplace(index, output.str(), false);
}

HttpResult request(const std::string& url, const LLSD* post)
{
    HttpResult result;

    // Every request advances one manager-wide deadline before suspension, so opens
    // and retries cannot bypass the 2.1-second service spacing.
    sRuntime.network_not_before = llmax(sRuntime.network_not_before,
                                        F64(LLTimer::getTotalSeconds()) + REQUEST_SPACING);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter =
        std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>(
            "ChatServiceHistory", LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCore::HttpRequest::ptr_t http_request = std::make_shared<LLCore::HttpRequest>();
    LLCore::HttpOptions::ptr_t options = std::make_shared<LLCore::HttpOptions>();
    options->setRetries(0);
    options->setTimeout(30);
    LLSD response = post ? adapter->postAndSuspend(http_request, url, *post, options)
                         : adapter->getAndSuspend(http_request, url, options);
    const LLCore::HttpStatus status =
        LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(
            response[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS]);
    result.status = status.getType();
    if (status)
    {
        // Capability adapters may place the response body either in the explicit
        // content field or at the response root.
        result.body = response.has(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_CONTENT)
            ? response[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_CONTENT]
            : response;
        if (result.body.isMap())
        {
            result.body.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
        }
    }
    return result;
}

bool retryable(S32 status)
{
    return status < 100 || status == 408 || status >= 500;
}

bool nameBlocked(const LLUUID& id, const Resident& resident)
{
    // Blocking fails closed until the authoritative mute list and the shared name
    // record can evaluate both UUID and legacy username forms.
    LLMuteList* mute = LLMuteList::getInstance();
    if (!mute || !mute->isLoadedFromServer() || mute->isMuted(id))
    {
        return true;
    }
    return resident.metadata.state != META_RESOLVED ||
           mute->isMuted(id, resident.metadata.name.getUserName());
}

bool uuidBlocked(const LLUUID& id)
{
    LLMuteList* mute = LLMuteList::getInstance();
    return !mute || !mute->isLoadedFromServer() || mute->isMuted(id);
}

U32 pendingMetadataCount()
{
    U32 count = 0;
    for (const auto& pair : sRuntime.residents)
    {
        count += pair.second.metadata.state == META_PENDING;
    }
    return count;
}

bool ensureMetadata(const LLUUID& id, Resident& resident)
{
    // A resolved record is stable for this login. Republishing it from a view load
    // would synchronously re-enter that load through the resident snapshot signal.
    if (resident.metadata.state == META_RESOLVED)
    {
        return true;
    }

    LLAvatarName cached;
    if (LLAvatarNameCache::get(id, &cached))
    {
        resident.metadata.state = META_RESOLVED;
        resident.metadata.name = cached;
        sRuntime.index_dirty = true;
        publishSnapshot(id, resident);
        return true;
    }
    if (resident.metadata.state == META_PENDING || resident.metadata.state == META_FAILED ||
        pendingMetadataCount() >= 8)
    {
        return false;
    }

    // Mark the attempt first because a cache hit may deliver synchronously from get().
    resident.metadata.state = META_PENDING;
    resident.metadata.deadline = F64(LLTimer::getTotalSeconds()) + NAME_TIMEOUT;
    const U32 attempt = ++resident.metadata.attempt;
    const U32 epoch = sRuntime.epoch;
    boost::signals2::connection connection = LLAvatarNameCache::get(
        id, [id, epoch, attempt](const LLUUID&, const LLAvatarName& name)
        {
            // Epoch and attempt checks discard callbacks from a prior account or a
            // timed-out lookup before they mutate resident state.
            if (!sRuntime.running || sRuntime.epoch != epoch)
            {
                return;
            }
            auto found = sRuntime.residents.find(id);
            if (found == sRuntime.residents.end() ||
                found->second.metadata.state != META_PENDING ||
                found->second.metadata.attempt != attempt)
            {
                return;
            }
            found->second.metadata.connection.disconnect();
            found->second.metadata.state = META_RESOLVED;
            found->second.metadata.name = name;
            sRuntime.index_dirty = true;
            if (found->second.metadata_waiting)
            {
                queueResident(id, found->second.priority_waiting);
                found->second.metadata_waiting = false;
                found->second.priority_waiting = false;
            }
            publishSnapshot(id, found->second);
            wakeManager();
        });
    if (resident.metadata.state == META_PENDING && resident.metadata.attempt == attempt)
    {
        resident.metadata.connection = connection;
    }
    else
    {
        connection.disconnect();
    }
    return resident.metadata.state == META_RESOLVED;
}

bool baseNetworkEligible(const CapabilityContext& context)
{
    // The shared gate is re-evaluated before every request and publication phase.
    return sRuntime.running && sRuntime.rollout && transcriptConsent() &&
           sRuntime.state_safety == STATE_SAFE && !sRuntime.cleanup_pending &&
           !sRuntime.delete_requested && context.complete() &&
           LLMuteList::getInstance() && LLMuteList::getInstance()->isLoadedFromServer();
}

bool networkEligible(const LLUUID& id, const Resident& resident,
                     const CapabilityContext& context)
{
    return baseNetworkEligible(context) && id.notNull() && !nameBlocked(id, resident);
}

void expireMetadata();
void activateDueOutboundRefreshes();

bool waitForWake(F64 seconds, U32 epoch)
{
    if (!ownsRuntime(epoch) || !sWake)
    {
        return false;
    }

    // Coalesce queued wake events, then sleep until either state changes or the
    // nearest pacing, list, metadata, or outbound deadline arrives.
    sRuntime.wake_pending = false;
    sWake->discard();
    llcoro::suspendUntilEventOnWithTimeout(*sWake, static_cast<F32>(llmax(0.01, seconds)),
                                           LLSDMap("timeout", true));
    if (!ownsRuntime(epoch))
    {
        return false;
    }
    sRuntime.wake_pending = false;
    return true;
}

bool pace(const CapabilityContext& context, U32 epoch,
          const LLUUID& resident_id = LLUUID::null)
{
    // Pacing remains interruptible so lifecycle, capability, mute, metadata, and
    // deletion changes can cancel a queued request before network I/O begins.
    while (ownsRuntime(epoch))
    {
        if (sampleContext() != context || sRuntime.delete_requested)
        {
            return false;
        }
        if (resident_id.notNull())
        {
            auto found = sRuntime.residents.find(resident_id);
            if (found == sRuntime.residents.end() ||
                !networkEligible(resident_id, found->second, context))
            {
                return false;
            }
        }

        // Record quiet-expiry priority during a global cooldown without allowing it
        // to bypass that cooldown or create a separate request path.
        activateDueOutboundRefreshes();
        const F64 now = F64(LLTimer::getTotalSeconds());
        const F64 remaining = sRuntime.network_not_before - now;
        if (remaining <= 0.0)
        {
            return true;
        }
        F64 wait = remaining;
        for (const auto& pair : sRuntime.residents)
        {
            if (pair.second.metadata.state == META_PENDING)
            {
                wait = llmin(wait, llmax(0.01, pair.second.metadata.deadline - now));
            }
            if (pair.second.outbound_refresh_due > now)
            {
                wait = llmin(wait, pair.second.outbound_refresh_due - now);
            }
        }
        const U32 pending_before = pendingMetadataCount();
        if (!waitForWake(wait, epoch))
        {
            return false;
        }
        expireMetadata();
        if (pendingMetadataCount() < pending_before && pendingMetadataCount() < 8)
        {
            // Give the manager a turn to fill a newly-open name slot before cooldown ends.
            if (resident_id.notNull())
            {
                queueResident(resident_id, sRuntime.priority_resident == resident_id);
            }
            return false;
        }
    }
    return false;
}

void handleRequestFailure(const LLUUID& id, Resident& resident, S32 status)
{
    if (status == 429)
    {
        // One manager-wide cooldown survives repeated throttles and every priority trigger.
        sRuntime.network_not_before = llmax(sRuntime.network_not_before,
            F64(LLTimer::getTotalSeconds()) + RATE_LIMIT_DELAY);
        queueResident(id, sRuntime.priority_resident == id);
        setWorkActive(id, true);
        return;
    }
    if (retryable(status) && !resident.retry_used)
    {
        // Transport, timeout, and server failures receive one delayed retry for this
        // resident pass; later ordinary triggers may start a fresh pass.
        resident.retry_used = true;
        sRuntime.network_not_before = llmax(sRuntime.network_not_before,
            F64(LLTimer::getTotalSeconds()) + RETRY_DELAY);
        queueResident(id, sRuntime.priority_resident == id);
    }
    else
    {
        setWorkActive(id, false);
        clearPriority(id);
    }
}

bool processList(const LLSD& body, bool& confirm_outbound_discovery)
{
    confirm_outbound_discovery = false;
    std::vector<ListEntry> entries;
    if (!validateConversationList(body, sRuntime.agent_id, entries))
    {
        return false;
    }
    std::set<LLUUID> listed;
    for (const ListEntry& entry : entries)
    {
        listed.insert(entry.resident_id);
    }

    // Validate the complete list before preserving priority placeholders and
    // appending background work. The first miss after an outbound hint retains its
    // placeholder for one delayed confirmation; the next valid miss retires it.
    for (auto& pair : sRuntime.residents)
    {
        pair.second.listed = listed.count(pair.first) != 0;
        if (!pair.second.listed)
        {
            if (pair.second.outbound_discovery == OUTBOUND_PENDING)
            {
                pair.second.outbound_discovery = OUTBOUND_CONFIRMING;
                confirm_outbound_discovery = true;
                continue;
            }

            pair.second.outbound_discovery = OUTBOUND_NONE;
            sRuntime.queue.erase(std::remove(sRuntime.queue.begin(), sRuntime.queue.end(), pair.first),
                                 sRuntime.queue.end());
            if (sRuntime.priority_resident == pair.first)
            {
                sRuntime.priority_resident.setNull();
            }
            pair.second.metadata_waiting = false;
            pair.second.priority_waiting = false;
            setWorkActive(pair.first, false);
        }
    }
    const bool first_list = !sRuntime.list_valid;
    for (const ListEntry& entry : entries)
    {
        Resident& resident = sRuntime.residents[entry.resident_id];
        const bool placeholder = std::find(sRuntime.queue.begin(), sRuntime.queue.end(),
                                           entry.resident_id) != sRuntime.queue.end();
        const bool changed = resident.advertised_token != entry.last_msg_id;
        resident.conversation_id = entry.conversation_id;
        resident.advertised_token = entry.last_msg_id;
        resident.listed = true;
        resident.outbound_discovery = OUTBOUND_NONE;
        if (resident.metadata.state == META_FAILED)
        {
            resident.metadata.state = META_UNREQUESTED;
        }

        // Queue only when discovery or archive coverage says the head may contain
        // unseen rows; token equality is not used as a TimeUUID ordering claim.
        if (!placeholder && (first_list || changed || resident.force_head ||
            resident.covered_token != resident.advertised_token ||
            resident.covered_serial != resident.archive_serial))
        {
            resident.retry_used = false;
            queueResident(entry.resident_id, false);
        }
    }
    sRuntime.list_valid = true;
    return true;
}

void requestList(const CapabilityContext& context, U32 epoch)
{
    if (!pace(context, epoch))
    {
        return;
    }
    if (!baseNetworkEligible(context))
    {
        return;
    }

    // Discovery mutates resident scheduling state only after the complete response
    // passes strict validation. A pending outbound hint remains visible across the
    // HTTP suspension and is consumed by processList rather than this request latch.
    const HttpResult response = request(context.list_url, NULL);
    if (!ownsRuntime(epoch) || sampleContext() != context || sRuntime.delete_requested ||
        !baseNetworkEligible(context))
    {
        return;
    }
    if (response.status == 429)
    {
        sRuntime.network_not_before = llmax(sRuntime.network_not_before,
            F64(LLTimer::getTotalSeconds()) + RATE_LIMIT_DELAY);
        sRuntime.list_needed = true;
        return;
    }
    if (!response.body.isUndefined() && response.status >= 200 && response.status < 300)
    {
        bool confirm_outbound_discovery = false;
        if (processList(response.body, confirm_outbound_discovery))
        {
            const F64 now = F64(LLTimer::getTotalSeconds());
            sRuntime.list_needed = confirm_outbound_discovery;
            sRuntime.list_retry_used = false;
            sRuntime.next_list = now + LIST_INTERVAL;
            if (confirm_outbound_discovery)
            {
                sRuntime.network_not_before = llmax(sRuntime.network_not_before,
                                                     now + RETRY_DELAY);
            }
            return;
        }
    }
    if (retryable(response.status) && !sRuntime.list_retry_used)
    {
        sRuntime.list_retry_used = true;
        sRuntime.network_not_before = llmax(sRuntime.network_not_before,
            F64(LLTimer::getTotalSeconds()) + RETRY_DELAY);
        sRuntime.list_needed = true;
    }
    else
    {
        sRuntime.list_needed = false;
        sRuntime.next_list = F64(LLTimer::getTotalSeconds()) + LIST_INTERVAL;
    }
}

void syncResident(const LLUUID& id, const CapabilityContext& context, U32 epoch)
{
    auto found = sRuntime.residents.find(id);
    if (found == sRuntime.residents.end())
    {
        return;
    }

    // Establish discovery, blocking, metadata, and archive-summary prerequisites
    // before this resident becomes the manager's active network pass.
    bool needs_prepare = false;
    bool had_boundary = false;
    TimeUuidKey stored_newest;
    {
        Resident& resident = found->second;
        if (!resident.listed || resident.conversation_id.empty())
        {
            // A failed discovery remains dormant until a fresh open/list/region or
            // due outbound trigger.
            resident.outbound_discovery = OUTBOUND_NONE;
            resident.snapshot.head_preview.clear();
            setWorkActive(id, false);
            clearPriority(id);
            return;
        }
        if (uuidBlocked(id))
        {
            setWorkActive(id, false);
            clearPriority(id);
            return;
        }
        if (!ensureMetadata(id, resident))
        {
            resident.metadata_waiting = true;
            resident.priority_waiting = resident.priority_waiting || sRuntime.priority_resident == id;
            return;
        }
        resident.metadata_waiting = false;
        resident.priority_waiting = false;
        if (!networkEligible(id, resident, context))
        {
            setWorkActive(id, false);
            clearPriority(id);
            return;
        }
        setWorkActive(id, true);
        sRuntime.active_resident = id;
        resident.first_request_started = false;
        needs_prepare = resident.summary.state == SUMMARY_UNPREPARED;
        had_boundary = !needs_prepare && resident.summary.state == SUMMARY_VALID &&
                       resident.summary.has_rows;
        stored_newest = resident.summary.newest;
    }

    // Stage a complete bounded pass in memory. No canonical bytes change until every
    // traversed page validates and reaches a terminal condition or durable seam.
    std::vector<Row> staged;
    std::set<std::string> pass_ids;
    size_t staged_bytes = 0;
    U32 returned_rows = 0;
    std::string cursor;
    bool complete = false;
    bool exposed_preview = false;
    while (!complete)
    {
        if (!pace(context, epoch, id))
        {
            if (!ownsRuntime(epoch))
            {
                return;
            }
            found = sRuntime.residents.find(id);
            if (found == sRuntime.residents.end())
            {
                return;
            }

            // Temporary account-wide ineligibility retains one occurrence; permanent
            // resident ineligibility clears priority and leaves the pass dormant.
            Resident& suspended = found->second;
            suspended.snapshot.head_preview.clear();
            setWorkActive(id, false);
            sRuntime.active_resident.setNull();
            LLMuteList* mute = LLMuteList::getInstance();
            const bool already_queued =
                std::find(sRuntime.queue.begin(), sRuntime.queue.end(), id) !=
                sRuntime.queue.end();
            const bool temporarily_ineligible = !sRuntime.delete_requested &&
                (sampleContext() != context || !baseNetworkEligible(context) ||
                 !mute || !mute->isLoadedFromServer());
            if (!already_queued)
            {
                if (temporarily_ineligible)
                {
                    queueResident(id, sRuntime.priority_resident == id);
                }
                else
                {
                    clearPriority(id);
                }
            }
            return;
        }

        // Capture the validated conversation and cursor immediately before issuing
        // this page request.
        LLSD post;
        {
            found = sRuntime.residents.find(id);
            if (!ownsRuntime(epoch) || found == sRuntime.residents.end())
            {
                return;
            }
            Resident& current = found->second;
            if (sRuntime.priority_resident.notNull() && sRuntime.priority_resident != id)
            {
                queueResident(id, false);
                staged.clear();
                break;
            }
            // If the first head request begins just after quiet expiry, it already
            // covers that burst. Cursor pages never consume a newer deadline.
            if (!current.first_request_started && cursor.empty() &&
                current.outbound_refresh_due > 0.0 &&
                current.outbound_refresh_due <= F64(LLTimer::getTotalSeconds()))
            {
                current.outbound_refresh_due = 0.0;
            }
            current.first_request_started = true;
            post["conversation_id"] = current.conversation_id;
            post["limit"] = 100;
            if (!cursor.empty())
            {
                post["before_msg_id"] = cursor;
            }
        }

        const HttpResult response = request(context.history_url, &post);
        Page page;
        found = sRuntime.residents.find(id);
        if (!ownsRuntime(epoch) || found == sRuntime.residents.end())
        {
            return;
        }
        {
            // Recheck account and resident eligibility after suspension, then validate
            // the complete response before exposing preview or staging rows.
            Resident& after_request = found->second;
            if (sRuntime.delete_requested || sampleContext() != context ||
                !networkEligible(id, after_request, context))
            {
                break;
            }
            if (response.status < 200 || response.status >= 300)
            {
                handleRequestFailure(id, after_request, response.status);
                after_request.snapshot.head_preview.clear();
                publishSnapshot(id, after_request);
                sRuntime.active_resident.setNull();
                return;
            }
            if (!validateHistoryPage(response.body, sRuntime.agent_id, id,
                                     after_request.conversation_id, cursor,
                                     sRuntime.deleted_before_ticks, page))
            {
                failResidentPass(id, after_request);
                break;
            }

            // A validated response is the yield boundary for outbound deadlines
            // that matured while this request was in flight.
            activateDueOutboundRefreshes();
            if (!ownsRuntime(epoch))
            {
                return;
            }

            // A different priority takes effect only after this response validates.
            if (sRuntime.priority_resident.notNull() && sRuntime.priority_resident != id)
            {
                queueResident(id, false);
                staged.clear();
                break;
            }
            if (!exposed_preview && !page.rows.empty())
            {
                if (!networkEligible(id, after_request, context))
                {
                    break;
                }
                after_request.snapshot.head_preview = page.rows;
                exposed_preview = true;
                publishSnapshot(id, after_request);
            }
        }

        // The first validated page is visible before a potentially years-long archive scan.
        if (needs_prepare)
        {
            bool bytes_changed = false;
            Summary summary;
            const std::string path = archivePath(sRuntime.account_dir, sRuntime.delimiter, id);
            const LLUUID agent_id = sRuntime.agent_id;
            const U64 boundary = sRuntime.deleted_before_ticks;
            LL::WorkQueue::ptr_t general = LL::WorkQueue::getInstance("General");
            const bool prepared = general && general->waitForResult(
                [path, id, agent_id, boundary, &summary, &bytes_changed]()
                {
                    return prepareArchive(path, id, agent_id, boundary, summary, bytes_changed);
                });
            found = sRuntime.residents.find(id);
            if (!ownsRuntime(epoch) || found == sRuntime.residents.end())
            {
                return;
            }
            {
                Resident& after_prepare = found->second;
                if (!prepared)
                {
                    failResidentPass(id, after_prepare);
                    sRuntime.active_resident.setNull();
                    return;
                }
                needs_prepare = false;
                after_prepare.summary = summary;
                sRuntime.index_dirty = true;
                had_boundary = summary.state == SUMMARY_VALID && summary.has_rows;
                stored_newest = summary.newest;
                if (bytes_changed)
                {
                    ++after_prepare.archive_serial;
                    after_prepare.covered_token.clear();
                    sRuntime.index_dirty = true;
                    sRuntime.local_content_exists = true;
                    publishSnapshot(id, after_prepare);
                }

                // Archive inspection is also an awaited page boundary; apply any
                // outbound priority that matured while it ran before staging rows.
                activateDueOutboundRefreshes();
                if (sRuntime.priority_resident.notNull() && sRuntime.priority_resident != id)
                {
                    queueResident(id, false);
                    staged.clear();
                    break;
                }
            }
        }

        // Archive preparation can suspend across logout; never retain its old Resident reference.
        found = sRuntime.residents.find(id);
        if (!ownsRuntime(epoch) || found == sRuntime.residents.end())
        {
            return;
        }
        Resident& page_resident = found->second;
        bool reached_stored_newest = false;
        for (const Row& row : page.rows)
        {
            if (!pass_ids.insert(row.msg_id).second)
            {
                staged.clear();
                complete = false;
                failResidentPass(id, page_resident);
                sRuntime.active_resident.setNull();
                return;
            }
            ++returned_rows;
            if (returned_rows > MAX_PASS_ROWS)
            {
                staged.clear();
                failResidentPass(id, page_resident);
                sRuntime.active_resident.setNull();
                return;
            }

            // The durable newest key is the accumulation boundary. Reaching it ends
            // this pass without re-reading or reconciling older canonical rows.
            if (had_boundary && row.key <= stored_newest)
            {
                reached_stored_newest = true;
                break;
            }

            // Count the serialized field payload against one pass-wide memory bound
            // before retaining the row.
            const size_t bytes = row.conversation_id.size() + row.msg_id.size() + 36 +
                row.from_name.size() + row.message.size() + std::to_string(row.dialog).size() +
                row.created_at.size();
            if (staged_bytes + bytes > MAX_PASS_BYTES)
            {
                staged.clear();
                failResidentPass(id, page_resident);
                sRuntime.active_resident.setNull();
                return;
            }
            staged_bytes += bytes;
            staged.push_back(row);
        }
        complete = page.terminal || reached_stored_newest;
        if (!complete)
        {
            if (page.next_cursor.empty())
            {
                staged.clear();
                break;
            }
            cursor = page.next_cursor;
        }
    }

    if (!complete)
    {
        found = sRuntime.residents.find(id);
        if (!ownsRuntime(epoch) || found == sRuntime.residents.end())
        {
            return;
        }
        Resident& final_resident = found->second;
        final_resident.snapshot.head_preview.clear();
        if (std::find(sRuntime.queue.begin(), sRuntime.queue.end(), id) == sRuntime.queue.end())
        {
            failResidentPass(id, final_resident);
        }
        else
        {
            publishSnapshot(id, final_resident);
        }
        sRuntime.active_resident.setNull();
        return;
    }

    // The service pages arrive newest-first; canonical publication is oldest-first.
    std::sort(staged.begin(), staged.end(), [](const Row& left, const Row& right)
    {
        return left.key < right.key;
    });

    found = sRuntime.residents.find(id);
    if (!ownsRuntime(epoch) || found == sRuntime.residents.end())
    {
        return;
    }

    Resident& final_resident = found->second;
    if (!networkEligible(id, final_resident, context) || sRuntime.delete_requested ||
        sampleContext() != context)
    {
        final_resident.snapshot.head_preview.clear();
        setWorkActive(id, false);
        clearPriority(id);
        sRuntime.active_resident.setNull();
        return;
    }

    bool changed = false;
    bool publication_failed = false;
    if (!staged.empty())
    {
        Summary resulting = final_resident.summary;
        const bool append = final_resident.summary.state == SUMMARY_VALID &&
                            final_resident.summary.has_rows;
        const std::string account_dir = sRuntime.account_dir;
        const std::string path = archivePath(account_dir, sRuntime.delimiter, id);

        // Re-sample account, capability, deletion, and blocking gates immediately
        // before dispatching one bounded mutation. A later Delete sweep owns any
        // artifact that lands after this final dispatch boundary.
        if (!ownsRuntime(epoch) || sRuntime.account_dir != account_dir ||
            sampleContext() != context || sRuntime.delete_requested ||
            !networkEligible(id, final_resident, context))
        {
            return;
        }

        LL::WorkQueue::ptr_t general = LL::WorkQueue::getInstance("General");
        changed = general && general->waitForResult(
            [path, staged, append, &resulting]() mutable
            {
                return publishRows(path, staged, append, resulting);
            });

        found = sRuntime.residents.find(id);
        if (!ownsRuntime(epoch) || sRuntime.account_dir != account_dir ||
            found == sRuntime.residents.end())
        {
            return;
        }

        Resident& after_publish = found->second;
        if (changed)
        {
            after_publish.summary = resulting;
            ++after_publish.archive_serial;
            sRuntime.index_dirty = true;
            sRuntime.local_content_exists = true;
            LLLogChat::notifyTranscriptCreated();
        }
        else
        {
            // Append/replace failure may have changed bytes; invalidate every reader token.
            ++after_publish.archive_serial;
            after_publish.summary.state = SUMMARY_UNPREPARED;
            after_publish.covered_token.clear();
            sRuntime.index_dirty = true;
            sRuntime.local_content_exists = true;
            LLLogChat::notifyTranscriptCreated();
            publication_failed = true;
        }
    }

    found = sRuntime.residents.find(id);
    if (!ownsRuntime(epoch) || found == sRuntime.residents.end())
    {
        return;
    }

    // Publish the new archive generation, clear the transient preview, and retain at
    // most one qualifying follow-up that arrived after this pass began.
    Resident& applied = found->second;

    if (sRuntime.delete_requested)
    {
        applied.snapshot.head_preview.clear();
        sRuntime.active_resident.setNull();
        return;
    }

    if (changed || staged.empty())
    {
        applied.covered_token = applied.advertised_token;
        applied.covered_serial = applied.archive_serial;
    }

    if (!publication_failed)
    {
        applied.retry_used = false;
        applied.force_head = false;
    }

    applied.snapshot.head_preview.clear();
    publishSnapshot(id, applied);

    if (applied.forced_followup && networkEligible(id, applied, context))
    {
        applied.forced_followup = false;
        queueResident(id, true);
    }
    else
    {
        setWorkActive(id, false);
    }

    if (sRuntime.priority_resident == id)
    {
        sRuntime.priority_resident.setNull();
    }
    sRuntime.active_resident.setNull();
}

bool sweepServiceArtifacts(const std::string& directory, const std::string& delimiter,
                           const std::string& state_path)
{
    LLMutexLock lock(&sStorageMutex);

    // Snapshot only integration-owned names while holding the storage boundary;
    // canonical privacy state is deliberately excluded from the sweep.
    LLDirIterator iterator(directory, "chat_service_*");
    std::string name;
    std::vector<std::string> targets;
    while (iterator.next(name))
    {
        if (ownedArtifactName(name) && name != STATE_NAME)
        {
            targets.push_back(childPath(directory, delimiter, name));
        }
    }

    // State temporary is non-authoritative once pending has committed and is swept
    // explicitly.
    targets.push_back(state_path + ".tmp");

    bool success = true;
    for (const std::string& path : targets)
    {
        if (LLFile::remove(path, ENOENT) != 0 && errno != ENOENT)
        {
            success = false;
        }

        bool exists = false;
        if (!inspectRegular(path, exists) || exists)
        {
            success = false;
        }
    }

    if (!syncDirectory(state_path))
    {
        success = false;
    }
    return success;
}

void finishDelete(bool success)
{
    LLChatServiceHistory::delete_callback_t callback = sRuntime.delete_callback;
    sRuntime.delete_callback.clear();
    sRuntime.delete_active = false;

    if (success)
    {
        sRuntime.delete_requested = false;
        sRuntime.cleanup_pending = false;
        sRuntime.state_safety = STATE_SAFE;
        sRuntime.list_needed = true;
    }
    else
    {
        sRuntime.delete_requested = true;
        LLNotificationsUtil::add("ChatServiceHistoryDeleteFailed");
    }

    if (callback)
    {
        callback(success);
    }
    LLLogChat::notifyTranscriptCreated();
}

void runDelete(U32 epoch)
{
    const std::string state_path = childPath(sRuntime.account_dir, sRuntime.delimiter, STATE_NAME);
    U64 boundary = sRuntime.delete_click_ticks;

    // Publish the inclusive cutoff as pending before invalidating views or removing
    // content. This durable ordering keeps every historical source fail-closed after
    // a crash at any later point in the sweep.
    {
        LLMutexLock lock(&sStorageMutex);
        boundary = llmax(boundary, llmax(recoverBoundaryCandidate(state_path),
                                        recoverBoundaryCandidate(state_path + ".tmp")));
        StateResult existing;
        const bool canonical_valid = readStateFile(state_path, existing);
        if ((canonical_valid &&
             (!clearUnsafeStateTemporary(state_path) ||
              !writeState(state_path, boundary, true))) ||
            (!canonical_valid && !recoverStateForDelete(state_path, boundary)))
        {
            sRuntime.state_safety = STATE_UNSAFE;
            finishDelete(false);
            return;
        }
    }

    sRuntime.deleted_before_ticks = boundary;
    sRuntime.cleanup_pending = true;

    // Cancel queued synchronization and clear every displayed historical owner only
    // after pending state is durable.
    sRuntime.queue.clear();
    sRuntime.active_resident.setNull();
    sRuntime.priority_resident.setNull();
    for (auto& pair : sRuntime.residents)
    {
        pair.second.metadata.connection.disconnect();
        pair.second.snapshot.head_preview.clear();
        pair.second.snapshot.service_work_active = false;
        publishSnapshot(pair.first, pair.second);
    }

    for (auto& pair : LLIMModel::instance().mId2SessionMap)
    {
        pair.second->clearForHistoryDeletion();
    }

    const LLFloaterReg::const_instance_list_t& previews =
        LLFloaterReg::getFloaterList("preview_conversation");
    for (LLFloater* floater : previews)
    {
        if (LLFloaterConversationPreview* preview =
                dynamic_cast<LLFloaterConversationPreview*>(floater))
        {
            preview->invalidateHistory();
            preview->closeFloater();
        }
    }

    LLFloaterIMSessionTab::processChatHistoryStyleUpdate(true);

    // Sweep legacy transcripts first, then service-owned artifacts, through the
    // shared filesystem mutation boundaries on the General queue.
    LL::WorkQueue::ptr_t general = LL::WorkQueue::getInstance("General");
    const std::string chat_logs_dir =
        gDirUtilp ? gDirUtilp->getPerAccountChatLogsDir() : std::string();
    const std::string account_dir = sRuntime.account_dir;
    const std::string delimiter = sRuntime.delimiter;
    const bool legacy = general && general->waitForResult([chat_logs_dir]()
    {
        return !chat_logs_dir.empty() && LLLogChat::deleteTranscriptContent(chat_logs_dir);
    });
    if (!ownsRuntime(epoch))
    {
        return;
    }
    const bool service = legacy && general->waitForResult(
        [account_dir, delimiter, state_path]()
        {
            return sweepServiceArtifacts(account_dir, delimiter, state_path);
        });
    if (!ownsRuntime(epoch))
    {
        return;
    }
    if (!service)
    {
        finishDelete(false);
        return;
    }

    // Clear pending only after both sweeps and their required directory syncs succeed.
    if (!ownsRuntime(epoch))
    {
        return;
    }
    {
        LLMutexLock lock(&sStorageMutex);
        if (!writeState(state_path, boundary, false))
        {
            sRuntime.state_safety = STATE_UNSAFE;
            finishDelete(false);
            return;
        }
    }
    if (!ownsRuntime(epoch))
    {
        return;
    }

    // Reset cached summaries last so subsequent discovery starts from empty storage.
    sRuntime.residents.clear();
    sRuntime.initialized_archives = true;
    sRuntime.local_content_exists = false;
    finishDelete(true);
}

bool initializeArchives(U32 epoch)
{
    // Discover account-local artifacts off the main thread; individual archives are
    // prepared incrementally by the manager so startup remains bounded.
    LL::WorkQueue::ptr_t general = LL::WorkQueue::getInstance("General");
    const std::string directory = sRuntime.account_dir;
    const std::string delimiter = sRuntime.delimiter;
    typedef std::pair<std::vector<LLUUID>, bool> initial_artifacts_t;
    const initial_artifacts_t artifacts = general
        ? general->waitForResult([directory, delimiter]()
          {
              return initial_artifacts_t(enumerateArchives(directory, delimiter),
                                         hasOwnedContent(directory));
          })
        : initial_artifacts_t();
    if (!ownsRuntime(epoch))
    {
        return false;
    }
    sRuntime.local_content_exists = artifacts.second;
    const std::vector<LLUUID>& ids = artifacts.first;
    for (const LLUUID& id : ids)
    {
        sRuntime.residents[id].summary.state = SUMMARY_UNPREPARED;
    }
    sRuntime.initialized_archives = true;
    sRuntime.index_dirty = true;
    return true;
}

bool prepareOneArchive(U32 epoch)
{
    for (auto& pair : sRuntime.residents)
    {
        Resident& resident = pair.second;
        if (resident.summary.state != SUMMARY_UNPREPARED)
        {
            continue;
        }

        // Prepare one archive per manager turn so network priority and lifecycle
        // events can interleave with storage maintenance.
        const LLUUID id = pair.first;
        const std::string path = archivePath(sRuntime.account_dir, sRuntime.delimiter, id);
        const LLUUID agent_id = sRuntime.agent_id;
        const U64 boundary = sRuntime.deleted_before_ticks;
        bool changed = false;
        Summary summary;
        LL::WorkQueue::ptr_t general = LL::WorkQueue::getInstance("General");
        const bool prepared = general && general->waitForResult(
            [path, id, agent_id, boundary, &summary, &changed]()
            {
                return prepareArchive(path, id, agent_id, boundary, summary, changed);
            });
        if (!ownsRuntime(epoch))
        {
            return false;
        }
        auto found = sRuntime.residents.find(id);
        if (found == sRuntime.residents.end())
        {
            return false;
        }
        Resident& current = found->second;
        if (prepared)
        {
            current.summary = summary;
            // Prepared validity changes index membership even when no bytes required repair.
            sRuntime.index_dirty = true;
            if (changed)
            {
                ++current.archive_serial;
            }
            if (summary.has_rows)
            {
                ensureMetadata(id, current);
            }
            publishSnapshot(id, current);
            LLLogChat::notifyTranscriptCreated();
        }
        return prepared;
    }
    return false;
}

void fillMetadataSlots()
{
    while (pendingMetadataCount() < 8)
    {
        auto found = std::find_if(sRuntime.residents.begin(), sRuntime.residents.end(),
            [](const auto& pair)
            {
                return pair.second.priority_waiting &&
                       pair.second.metadata.state == META_UNREQUESTED;
            });
        if (found == sRuntime.residents.end())
        {
            found = std::find_if(sRuntime.residents.begin(), sRuntime.residents.end(),
                [](const auto& pair)
                {
                    return (pair.second.metadata_waiting ||
                            pair.second.summary.state == SUMMARY_VALID) &&
                           pair.second.metadata.state == META_UNREQUESTED;
                });
        }
        if (found == sRuntime.residents.end())
        {
            return;
        }
        if (ensureMetadata(found->first, found->second))
        {
            if (found->second.metadata_waiting)
            {
                queueResident(found->first, found->second.priority_waiting);
            }
            found->second.metadata_waiting = false;
            found->second.priority_waiting = false;
        }
    }
}

bool updateIndex(U32 epoch)
{
    // Capture one stable manifest input, regenerate it under the storage mutex, then
    // detect mutations that arrived while the General-queue job was suspended.
    std::vector<LLUUID> archives;
    std::map<LLUUID, LLAvatarName> names;
    for (const auto& pair : sRuntime.residents)
    {
        if (pair.second.summary.state == SUMMARY_VALID &&
            pair.second.summary.file_exists)
        {
            archives.push_back(pair.first);
        }
        if (pair.second.metadata.state == META_RESOLVED)
        {
            names[pair.first] = pair.second.metadata.name;
        }
    }
    LL::WorkQueue::ptr_t general = LL::WorkQueue::getInstance("General");
    const bool updated = general && general->waitForResult(
        [archives, names, directory = sRuntime.account_dir,
         delimiter = sRuntime.delimiter]()
        {
            return regenerateIndex(archives, names, directory, delimiter);
        });
    if (!ownsRuntime(epoch))
    {
        return false;
    }
    if (updated)
    {
        std::vector<LLUUID> current_archives;
        std::vector<LLUUID> current_names;
        for (const auto& pair : sRuntime.residents)
        {
            if (pair.second.summary.state == SUMMARY_VALID &&
                pair.second.summary.file_exists)
            {
                current_archives.push_back(pair.first);
            }
            if (pair.second.metadata.state == META_RESOLVED)
            {
                current_names.push_back(pair.first);
            }
        }
        std::vector<LLUUID> indexed_names;
        for (const auto& pair : names)
        {
            indexed_names.push_back(pair.first);
        }
        sRuntime.index_dirty = archives != current_archives || indexed_names != current_names;
        return true;
    }
    return false;
}

void expireMetadata()
{
    // A timed-out shared lookup releases its bounded slot and leaves the resident
    // dormant until a later ordinary trigger resets failed metadata.
    const F64 now = LLTimer::getTotalSeconds();
    for (auto& pair : sRuntime.residents)
    {
        Metadata& metadata = pair.second.metadata;
        if (metadata.state == META_PENDING && metadata.deadline <= now)
        {
            metadata.connection.disconnect();
            metadata.state = META_FAILED;
            pair.second.metadata_waiting = false;
            pair.second.priority_waiting = false;
            setWorkActive(pair.first, false);
            clearPriority(pair.first);
        }
    }
}

void activateDueOutboundRefreshes()
{
    // Revoked account gates retire pending outbound activity before it can remain
    // a wake source or schedule service work.
    if (!sRuntime.rollout || !transcriptConsent() ||
        sRuntime.state_safety != STATE_SAFE || sRuntime.cleanup_pending ||
        sRuntime.delete_requested)
    {
        for (auto& pair : sRuntime.residents)
        {
            pair.second.outbound_refresh_due = 0.0;
        }
        return;
    }

    const F64 now = LLTimer::getTotalSeconds();
    for (auto& pair : sRuntime.residents)
    {
        Resident& resident = pair.second;
        if (resident.outbound_refresh_due <= 0.0 || resident.outbound_refresh_due > now)
        {
            continue;
        }

        // Consume the deadline before reusing the existing priority/discovery path
        // so an expired value cannot create a rapid manager wake loop.
        resident.outbound_refresh_due = 0.0;
        LLMuteList* mute = LLMuteList::getInstance();
        if (mute && mute->isLoadedFromServer() &&
            (mute->isMuted(pair.first) ||
             (resident.metadata.state == META_RESOLVED &&
              mute->isMuted(pair.first, resident.metadata.name.getUserName()))))
        {
            continue;
        }
        if (resident.listed)
        {
            LLChatServiceHistory::prioritizeResident(pair.first, true);
        }
        else if (resident.outbound_discovery == OUTBOUND_NONE)
        {
            resident.outbound_discovery = OUTBOUND_PENDING;
            LLChatServiceHistory::prioritizeResident(pair.first);
        }
    }
}

F64 nearestWait()
{
    // Sleep until the earliest list, pacing, metadata, or outbound quiet deadline;
    // explicit wake events interrupt this deadline when state changes sooner.
    const F64 now = LLTimer::getTotalSeconds();
    F64 deadline = sRuntime.next_list > now ? sRuntime.next_list : now + LIST_INTERVAL;
    if (sRuntime.network_not_before > now)
    {
        deadline = llmin(deadline, sRuntime.network_not_before);
    }
    for (const auto& pair : sRuntime.residents)
    {
        if (pair.second.metadata.state == META_PENDING)
        {
            deadline = llmin(deadline, pair.second.metadata.deadline);
        }
        if (!sRuntime.delete_requested && !sRuntime.cleanup_pending &&
            sRuntime.state_safety == STATE_SAFE && sRuntime.rollout && transcriptConsent() &&
            pair.second.outbound_refresh_due > 0.0)
        {
            deadline = llmin(deadline, pair.second.outbound_refresh_due);
        }
    }
    return llmax(0.01, deadline - now);
}

void manager(U32 epoch)
{
    // One account-scoped coroutine advances deletion, initialization, network work,
    // and storage maintenance in priority order.
    while (sRuntime.running && sRuntime.epoch == epoch)
    {
        if (sRuntime.delete_active)
        {
            runDelete(epoch);
            if (!ownsRuntime(epoch))
            {
                return;
            }
            continue;
        }
        if (sRuntime.state_safety == STATE_UNKNOWN)
        {
            // Load privacy state before archives, views, or network work become eligible.
            LL::WorkQueue::ptr_t general = LL::WorkQueue::getInstance("General");
            const std::string state_path = childPath(sRuntime.account_dir,
                                                     sRuntime.delimiter, STATE_NAME);
            const StateResult state = general
                ? general->waitForResult([state_path]()
                  {
                      LLMutexLock lock(&sStorageMutex);
                      return loadState(state_path);
                  })
                : StateResult();
            if (!sRuntime.running || sRuntime.epoch != epoch)
            {
                return;
            }
            sRuntime.state_safety = state.safety;
            sRuntime.deleted_before_ticks = state.boundary;
            sRuntime.cleanup_pending = state.cleanup_pending;
            LLLogChat::notifyTranscriptCreated();
            if (state.safety == STATE_SAFE && !state.cleanup_pending)
            {
                LLFloaterIMSessionTab::processChatHistoryStyleUpdate(true);
            }
            if (state.safety == STATE_SAFE && state.cleanup_pending)
            {
                sRuntime.delete_active = true;
                sRuntime.delete_requested = true;
                sRuntime.delete_click_ticks = state.boundary;
                continue;
            }
        }
        if (sRuntime.state_safety == STATE_SAFE && !sRuntime.initialized_archives)
        {
            if (!initializeArchives(epoch))
            {
                return;
            }
            continue;
        }

        // Expire timers, activate due outbound bursts, and fill metadata slots before
        // choosing the next network occurrence.
        expireMetadata();
        activateDueOutboundRefreshes();
        fillMetadataSlots();

        const CapabilityContext context = sampleContext();
        if (context != sRuntime.context)
        {
            // Capability context changes restart discovery but retain valid archive
            // coverage and resident priority state for this account.
            sRuntime.context = context;
            if (context.complete())
            {
                sRuntime.list_needed = true;
                sRuntime.list_retry_used = false;
                for (auto& pair : sRuntime.residents)
                {
                    if (pair.second.metadata.state == META_FAILED)
                    {
                        pair.second.metadata.state = META_UNREQUESTED;
                    }
                }
            }
            else
            {
                for (auto& pair : sRuntime.residents)
                {
                    setWorkActive(pair.first, false);
                }
            }
        }

        // Discovery precedes resident paging; the queue itself preserves open,
        // inbound, and due outbound priority without creating a second worker.
        const bool base_network = baseNetworkEligible(context);
        if (base_network)
        {
            if (F64(LLTimer::getTotalSeconds()) >= sRuntime.next_list)
            {
                sRuntime.list_needed = true;
                sRuntime.list_retry_used = false;
            }
            if (sRuntime.list_needed)
            {
                requestList(context, epoch);
                if (!ownsRuntime(epoch))
                {
                    return;
                }
                continue;
            }
            if (!sRuntime.queue.empty())
            {
                const LLUUID id = sRuntime.queue.front();
                sRuntime.queue.pop_front();
                syncResident(id, context, epoch);
                if (!ownsRuntime(epoch))
                {
                    return;
                }
                continue;
            }

            // Existing archives share the same bounded resolver used by blocking and views.
            for (auto& pair : sRuntime.residents)
            {
                if (pair.second.summary.has_rows && pair.second.metadata.state == META_UNREQUESTED &&
                    pendingMetadataCount() < 8)
                {
                    ensureMetadata(pair.first, pair.second);
                }
            }
        }
        else
        {
            sRuntime.active_resident.setNull();
            for (auto& pair : sRuntime.residents)
            {
                setWorkActive(pair.first, false);
            }
        }

        // Storage maintenance yields to every eligible priority/network occurrence.
        if (sRuntime.state_safety == STATE_SAFE && !sRuntime.cleanup_pending &&
            prepareOneArchive(epoch))
        {
            continue;
        }
        if (!ownsRuntime(epoch))
        {
            return;
        }
        if (sRuntime.index_dirty && sRuntime.state_safety == STATE_SAFE &&
            !sRuntime.cleanup_pending)
        {
            if (!ownsRuntime(epoch))
            {
                return;
            }
            if (updateIndex(epoch))
            {
                continue;
            }
            if (!ownsRuntime(epoch))
            {
                return;
            }
        }
        if (!waitForWake(nearestWait(), epoch))
        {
            return;
        }
    }
}

bool clickTicks(U64& ticks)
{
    // Convert the click-time microsecond clock into the inclusive final UUIDv1 tick
    // for that microsecond using checked integer arithmetic only.
    const U64 sampled = LLTimer::getTotalTime();
    if (sampled > static_cast<U64>(LLONG_MAX))
    {
        return false;
    }
    const S64 offset = static_cast<S64>(gUTCOffset) * 1000000LL;
    S64 corrected = static_cast<S64>(sampled);
    if ((offset > 0 && corrected > LLONG_MAX - offset) ||
        (offset < 0 && corrected < LLONG_MIN - offset))
    {
        return false;
    }
    corrected += offset;
    if (corrected < 0 || static_cast<U64>(corrected) >
        (UUID_TICK_LIMIT - 1 - UUID_EPOCH - 9) / 10)
    {
        return false;
    }
    ticks = UUID_EPOCH + static_cast<U64>(corrected) * 10 + 9;
    return ticks < UUID_TICK_LIMIT;
}

bool legacyWallEpoch(const std::string& text, F64& epoch)
{
    if (text.size() < 15)
    {
        return false;
    }

    // Legacy transcripts may use either a 12-hour suffix or a 24-hour wall clock.
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    char suffix[3] = {};
    int consumed = 0;
    if (std::sscanf(text.c_str(), "%d/%d/%d %d:%d %2s%n", &year, &month, &day,
                    &hour, &minute, suffix, &consumed) == 6)
    {
        if (consumed != static_cast<int>(text.size()) ||
            (strcmp(suffix, "AM") && strcmp(suffix, "PM")) || hour < 1 || hour > 12)
        {
            return false;
        }
        if (!strcmp(suffix, "AM"))
        {
            hour %= 12;
        }
        else if (hour != 12)
        {
            hour += 12;
        }
    }
    else
    {
        consumed = 0;
        if (std::sscanf(text.c_str(), "%d/%d/%d %d:%d%n", &year, &month, &day,
                        &hour, &minute, &consumed) != 5 ||
            consumed != static_cast<int>(text.size()) || hour < 0 || hour > 23)
        {
            return false;
        }
    }
    if (minute < 0 || minute > 59)
    {
        return false;
    }
    try
    {
        const boost::posix_time::ptime value(
            boost::gregorian::date(year, month, day),
            boost::posix_time::hours(hour) + boost::posix_time::minutes(minute));
        static const boost::posix_time::ptime unix_epoch(boost::gregorian::date(1970, 1, 1));
        epoch = static_cast<F64>((value - unix_epoch).total_seconds());
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool sameSenderAndText(const LLSD& left, const LLSD& right)
{
    if (left[LL_IM_TEXT].asString() != right[LL_IM_TEXT].asString())
    {
        return false;
    }

    // Prefer exact resident identity when both sources carry it; otherwise require
    // the transcript names to agree exactly.
    const bool left_has_id = left[LL_IM_FROM_ID].isDefined();
    const bool right_has_id = right[LL_IM_FROM_ID].isDefined();
    return left_has_id && right_has_id
        ? left[LL_IM_FROM_ID].asUUID() == right[LL_IM_FROM_ID].asUUID()
        : left[LL_IM_FROM].asString() == right[LL_IM_FROM].asString();
}

bool sameSenderAndText(const LLSD& legacy, const Row& service)
{
    if (legacy[LL_IM_TEXT].asString() != service.message)
    {
        return false;
    }

    return legacy[LL_IM_FROM_ID].isDefined()
        ? legacy[LL_IM_FROM_ID].asUUID() == service.from_id
        : legacy[LL_IM_FROM].asString() == service.from_name;
}

bool sameLegacyMinute(F64 wall_epoch, F64 utc_epoch)
{
    // Legacy transcript minutes are SLT but do not encode whether UTC-7 or UTC-8
    // applied. Either exact minute may identify the authoritative service row.
    for (const F64 offset : { 7.0 * 3600.0, 8.0 * 3600.0 })
    {
        const F64 minute = wall_epoch + offset;
        if (utc_epoch >= minute && utc_epoch < minute + 60.0)
        {
            return true;
        }
    }
    return false;
}

S64 utcMinute(F64 epoch)
{
    return static_cast<S64>(epoch) / 60;
}

bool sameHistoryLiveOccurrence(const LLSD& history, const LLSD& live)
{
    if (!sameSenderAndText(history, live) ||
        !live["timestamp"].isInteger() || live["timestamp"].asInteger() <= 0)
    {
        return false;
    }

    const F64 live_epoch = static_cast<U32>(live["timestamp"].asInteger());
    if (history["chat_service_msg_id"].isString() &&
        history["timestamp"].isInteger() && history["timestamp"].asInteger() > 0)
    {
        const U32 history_epoch = static_cast<U32>(history["timestamp"].asInteger());
        return history_epoch / 60 == static_cast<U32>(live_epoch) / 60;
    }

    return history[LEGACY_WALL_TIME].isReal() &&
           sameLegacyMinute(history[LEGACY_WALL_TIME].asReal(), live_epoch);
}

LLSD serviceMessage(const Row& row)
{
    LLSD message;

    // Keep the validated wire value as metadata while rendering through the viewer's
    // SLT transcript format.
    const U32 timestamp = static_cast<U32>(LLDate(row.created_at).secondsSinceEpoch());
    message[LL_IM_TIME] = LLLogChat::timestamp2LogString(timestamp, true);
    message[LL_IM_DATE_TIME] = row.created_at;
    message[LL_IM_FROM] = row.from_name;
    message[LL_IM_FROM_ID] = row.from_id;
    message[LL_IM_TEXT] = row.message;
    message["timestamp"] = static_cast<S32>(timestamp);
    message["is_history"] = true;
    message["chat_service_msg_id"] = row.msg_id;
    return message;
}

LLChatServiceHistory::HistoryResult readStitched(
    const LLUUID& id, const LLUUID& agent_id, const std::string& archive_path,
    const std::vector<std::string>& legacy_paths, U32 limit, U32 epoch, U32 serial,
    U64 boundary, bool include_service, std::vector<Row> preview)
{
    LLChatServiceHistory::HistoryResult result;
    result.account_epoch = epoch;
    result.archive_serial = serial;
    result.included_service = include_service;
    ArchiveScan archive;
    if (include_service)
    {
        // View reads serialize with publication but never repair or quarantine storage.
        LLMutexLock lock(&sStorageMutex);
        if (!scanArchive(archive_path, agent_id, id, boundary, limit, archive) &&
            archive.state != ARCHIVE_ABSENT)
        {
            result.maintenance_needed = true;
        }
        if (archive.state != ARCHIVE_VALID)
        {
            archive = ArchiveScan();
        }
    }

    // Resolve the ordinary transcript and monthly shards in their canonical order.
    std::list<LLSD> legacy;
    LLSD parameters;
    parameters["load_all_history"] = true;
    parameters["cut_off_todays_date"] = false;
    for (const std::string& path : legacy_paths)
    {
        LLChatServiceHistoryAccess::loadLegacy(path, legacy, parameters);
    }

    // Canonical rows win exact identity over preview rows, then all service rows
    // share one stable TimeUUID order for seam reconciliation and final display.
    std::set<std::string> canonical_ids;
    std::vector<Row> service = archive.display_rows;
    for (const Row& row : service)
    {
        canonical_ids.insert(row.msg_id);
    }
    for (const Row& row : preview)
    {
        if (!canonical_ids.count(row.msg_id))
        {
            service.push_back(row);
        }
    }
    std::sort(service.begin(), service.end(), [](const Row& left, const Row& right)
    {
        return left.key < right.key;
    });
    std::vector<bool> service_placed(service.size(), false);
    std::multimap<S64, size_t> canonical_minutes;
    for (size_t pos = 0; pos < service.size(); ++pos)
    {
        if (canonical_ids.count(service[pos].msg_id))
        {
            canonical_minutes.emplace(
                utcMinute(LLDate(service[pos].created_at).secondsSinceEpoch()), pos);
        }
    }

    const F64 service_epoch = archive.has_oldest
        ? static_cast<F64>(archive.oldest.ticks - UUID_EPOCH) / 10000000.0 : 0.0;
    std::vector<std::pair<F64, LLSD>> dated;
    std::list<LLSD> undated;

    // Only the durable archive's oldest key controls the legacy/service seam. Keep
    // ambiguous rows conservatively and require dated legacy rows to precede the
    // service boundary by the existing seven-hour tolerance.
    for (const LLSD& message : legacy)
    {
        F64 wall = 0.0;
        if (!legacyWallEpoch(message[LL_IM_DATE_TIME].asString(), wall))
        {
            undated.push_back(message);
        }
        else if (!archive.has_oldest || wall + 7.0 * 3600.0 < service_epoch)
        {
            LLSD stitched = message;
            stitched[LEGACY_WALL_TIME] = wall;

            // Replace an exact legacy occurrence in place with its canonical row.
            // This keeps local ordering around same-minute system messages while
            // consuming only one occurrence from each source.
            size_t match = service.size();
            for (const F64 offset : { 7.0 * 3600.0, 8.0 * 3600.0 })
            {
                const auto range = canonical_minutes.equal_range(utcMinute(wall + offset));
                for (auto candidate = range.first; candidate != range.second; ++candidate)
                {
                    const size_t pos = candidate->second;
                    if (!service_placed[pos] && sameSenderAndText(message, service[pos]))
                    {
                        match = pos;
                        break;
                    }
                }
                if (match != service.size())
                {
                    break;
                }
            }
            if (match != service.size())
            {
                stitched = serviceMessage(service[match]);
                service_placed[match] = true;
            }
            dated.emplace_back(wall, stitched);
        }
    }
    std::stable_sort(dated.begin(), dated.end(),
        [](const auto& left, const auto& right)
        {
            return left.first < right.first;
        });

    for (const auto& item : dated)
    {
        result.messages.push_back(item.second);
    }

    for (const LLSD& message : undated)
    {
        result.messages.push_back(message);
    }

    // Append service rows that did not replace their exact legacy occurrence.
    for (size_t pos = 0; pos < service.size(); ++pos)
    {
        if (!service_placed[pos])
        {
            result.messages.push_back(serviceMessage(service[pos]));
        }
    }

    while (limit && result.messages.size() > limit)
    {
        result.messages.pop_front();
    }

    return result;
}
}

void LLChatServiceHistory::start()
{
    // Reset any prior account first, then capture all per-login paths, IDs, gates,
    // and signal connections before launching the manager coroutine.
    stop();

    ++sRuntime.epoch;
    sRuntime.running = true;
    sRuntime.rollout = gSavedSettings.getBOOL(ENABLED_SETTING);
    sRuntime.account_dir = accountPath("");
    sRuntime.delimiter = gDirUtilp ? gDirUtilp->getDirDelimiter() : std::string("/");
    sRuntime.agent_id = gAgentID;
    sRuntime.state_safety = STATE_UNKNOWN;
    sRuntime.next_list = 0.0;
    sRuntime.list_needed = true;

    if (!sWake)
    {
        sWake.reset(new LLEventMailDrop("ChatServiceHistoryWake", true));
    }
    sWake->discard();

    sRegionConnection = gAgent.addRegionChangedCallback([]()
    {
        LLChatServiceHistory::regionChanged();
    });

    // Consent changes revoke transient presentation immediately and wake the manager
    // to re-evaluate every resident under the new account-wide gate.
    if (gSavedPerAccountSettings.controlExists("KeepConversationLogTranscripts"))
    {
        sRuntime.consent_connection = gSavedPerAccountSettings
            .getControl("KeepConversationLogTranscripts")->getSignal()->connect(
                [](LLControlVariable*, const LLSD&, const LLSD&)
                {
                    const bool allowed = transcriptConsent();
                    for (auto& pair : sRuntime.residents)
                    {
                        if (!allowed)
                        {
                            pair.second.outbound_refresh_due = 0.0;
                            pair.second.snapshot.head_preview.clear();
                            pair.second.snapshot.service_work_active = false;
                        }
                        publishSnapshot(pair.first, pair.second);
                    }
                    wakeManager();
                });
    }

    // Presentation changes reload model-owned sessions without creating another
    // history scheduler.
    if (gSavedPerAccountSettings.controlExists("LogShowHistory"))
    {
        sRuntime.show_history_connection = gSavedPerAccountSettings
            .getControl("LogShowHistory")->getSignal()->connect(
                [](LLControlVariable*, const LLSD&, const LLSD&)
                {
                    LLFloaterIMSessionTab::processChatHistoryStyleUpdate(true);
                });
    }

    regionChanged();
    const U32 epoch = sRuntime.epoch;
    LLCoros::instance().launch("ChatServiceHistory", [epoch]()
        {
            manager(epoch);
        });
}

void LLChatServiceHistory::stop()
{
    if (sRuntime.running)
    {
        sRuntime.running = false;
        ++sRuntime.epoch;

        for (auto& pair : sRuntime.residents)
        {
            pair.second.metadata.connection.disconnect();
        }

        if (sWake)
        {
            sWake->post(LLSD(true));
        }
    }

    sRegionConnection.disconnect();
    sRuntime.consent_connection.disconnect();
    sRuntime.show_history_connection.disconnect();

    const U32 epoch = sRuntime.epoch;
    sRuntime = Runtime();
    sRuntime.epoch = epoch;
}

void LLChatServiceHistory::regionChanged()
{
    LLViewerRegion* region = gAgent.getRegion();
    if (region && !region->capabilitiesReceived())
    {
        region->setCapabilitiesReceivedCallback([](const LLUUID&, LLViewerRegion*)
        {
            wakeManager();
        });
    }

    wakeManager();
}

bool LLChatServiceHistory::enabledForLogin()
{
    return sRuntime.rollout;
}

U32 LLChatServiceHistory::accountEpoch()
{
    return sRuntime.epoch;
}

bool LLChatServiceHistory::historySuppressed()
{
    return sRuntime.state_safety != STATE_SAFE || sRuntime.cleanup_pending ||
           sRuntime.delete_requested;
}

bool LLChatServiceHistory::servicePresentationAllowed()
{
    return sRuntime.rollout && transcriptConsent() && !historySuppressed();
}

bool LLChatServiceHistory::localHistoryExists()
{
    if (sRuntime.delete_active)
    {
        return false;
    }

    if (sRuntime.delete_requested || sRuntime.cleanup_pending ||
        sRuntime.state_safety == STATE_UNSAFE)
    {
        return true;
    }

    for (const auto& pair : sRuntime.residents)
    {
        if (pair.second.summary.has_rows)
        {
            return true;
        }
    }

    return sRuntime.local_content_exists;
}

bool LLChatServiceHistory::localHistoryExists(const LLUUID& resident_id)
{
    if (!servicePresentationAllowed())
    {
        return false;
    }

    const auto found = sRuntime.residents.find(resident_id);
    if (found == sRuntime.residents.end() || found->second.summary.state == SUMMARY_UNPREPARED)
    {
        wakeManager();
        return false;
    }

    return found->second.summary.has_rows;
}

bool LLChatServiceHistory::isPersistedDirectDialog(EInstantMessage dialog)
{
    return persistedDirectDialog(static_cast<S32>(dialog));
}

void LLChatServiceHistory::prioritizeResident(const LLUUID& id, bool follow_active_request)
{
    if (!sRuntime.running || id.isNull())
    {
        return;
    }

    // Coalesce priority triggers into one front occurrence. A qualifying trigger
    // after the active request began records at most one follow-up pass.
    Resident& resident = sRuntime.residents[id];
    resident.force_head = true;
    resident.retry_used = false;
    if (resident.metadata.state == META_FAILED)
    {
        resident.metadata.state = META_UNREQUESTED;
    }

    if (sRuntime.active_resident == id)
    {
        if (follow_active_request && resident.first_request_started)
        {
            resident.forced_followup = true;
        }
    }
    else
    {
        queueResident(id, true);
        sRuntime.priority_resident = id;
    }

    if (!resident.listed)
    {
        sRuntime.list_needed = true;
        sRuntime.list_retry_used = false;
    }

    const CapabilityContext context = sampleContext();
    const bool potentially_active = sRuntime.rollout && transcriptConsent() &&
        sRuntime.state_safety == STATE_SAFE && !sRuntime.cleanup_pending &&
        !sRuntime.delete_requested && context.complete() && !uuidBlocked(id);
    setWorkActive(id, potentially_active);
    wakeManager();
}

void LLChatServiceHistory::noteOutboundDirectMessage(const LLUUID& id)
{
    if (!sRuntime.running || !sRuntime.rollout || !transcriptConsent() ||
        sRuntime.delete_requested || id.isNull() || id == sRuntime.agent_id)
    {
        return;
    }

    // Every send replaces one quiet deadline, while first contact keeps its
    // immediate bounded discovery path.
    Resident& resident = sRuntime.residents[id];
    resident.outbound_refresh_due =
        F64(LLTimer::getTotalSeconds()) + OUTBOUND_REFRESH_DELAY;
    if (!resident.listed && resident.outbound_discovery == OUTBOUND_NONE)
    {
        resident.outbound_discovery = OUTBOUND_PENDING;
        prioritizeResident(id);
    }
    else
    {
        wakeManager();
    }
}

LLChatServiceHistory::Snapshot LLChatServiceHistory::getSnapshot(const LLUUID& id)
{
    const auto found = sRuntime.residents.find(id);
    if (found == sRuntime.residents.end())
    {
        Snapshot snapshot;
        snapshot.service_presentation_allowed = servicePresentationAllowed();
        return snapshot;
    }
    return found->second.snapshot;
}

boost::signals2::connection LLChatServiceHistory::setSnapshotChanged(
    const snapshot_callback_t& callback)
{
    return sSnapshotSignal.connect(callback);
}

std::list<LLSD> LLChatServiceHistory::filterLiveDuplicates(
    const std::list<LLSD>& history, const std::list<LLSD>& live)
{
    std::vector<const LLSD*> live_rows;
    for (const LLSD& message : live)
    {
        live_rows.push_back(&message);
    }
    std::vector<bool> consumed(live_rows.size(), false);

    // Matching is occurrence-aware: repeated identical messages consume repeated
    // rows one-for-one instead of collapsing to one value.
    std::list<LLSD> filtered;
    for (const LLSD& message : history)
    {
        bool duplicate = false;
        for (size_t pos = 0; pos < live_rows.size(); ++pos)
        {
            if (!consumed[pos] && sameHistoryLiveOccurrence(message, *live_rows[pos]))
            {
                consumed[pos] = true;
                duplicate = true;
                break;
            }
        }
        if (!duplicate)
        {
            filtered.push_back(message);
        }
    }
    return filtered;
}

std::list<LLSD> LLChatServiceHistory::mergeHeadPreview(
    const std::list<LLSD>& loaded, const Snapshot& snapshot, U32 limit)
{
    // Deduplicate transient preview rows against the loaded archive by exact message
    // ID, then apply the consumer's ordinary newest-row limit.
    std::list<LLSD> result = loaded;
    std::set<std::string> ids;
    for (const LLSD& message : result)
    {
        if (message["chat_service_msg_id"].isString())
        {
            ids.insert(message["chat_service_msg_id"].asString());
        }
    }

    std::vector<Row> additions;
    for (const Row& row : snapshot.head_preview)
    {
        if (!ids.count(row.msg_id))
        {
            additions.push_back(row);
        }
    }

    std::sort(additions.begin(), additions.end(), [](const Row& left, const Row& right)
    {
        return left.key < right.key;
    });

    for (const Row& row : additions)
    {
        result.push_back(serviceMessage(row));
    }

    while (limit && result.size() > limit)
    {
        result.pop_front();
    }

    return result;
}

bool LLChatServiceHistory::loadStitchedHistory(
    const LLUUID& id, const std::string& legacy_stem, U32 limit,
    const history_callback_t& callback)
{
    if (!callback || id.isNull() || historySuppressed())
    {
        return false;
    }

    // Capture account and archive generations with the filesystem inputs so the
    // main-queue completion can detect any intervening lifecycle or publication.
    Resident& resident = sRuntime.residents[id];
    const bool include_service = servicePresentationAllowed();
    if (include_service)
    {
        ensureMetadata(id, resident);
    }

    const U32 epoch = sRuntime.epoch;
    const U32 serial = resident.archive_serial;
    const U64 boundary = sRuntime.deleted_before_ticks;
    const std::vector<Row> preview = include_service
        ? resident.snapshot.head_preview : std::vector<Row>();
    const LLUUID agent_id = sRuntime.agent_id;
    const std::string archive_path = archivePath(sRuntime.account_dir, sRuntime.delimiter, id);
    std::vector<std::string> legacy_paths;
    LLLogChat::getTranscriptFamily(legacy_stem, legacy_paths);

    LL::WorkQueue::ptr_t main = LL::WorkQueue::getInstance("mainloop");
    LL::WorkQueue::ptr_t general = LL::WorkQueue::getInstance("General");
    if (!main || !general)
    {
        return false;
    }

    // Read and stitch off the main thread; maintenance requests are applied only if
    // the captured archive serial is still current.
    return main->postTo(general,
        [id, agent_id, archive_path, legacy_paths, limit, epoch, serial, boundary,
         include_service, preview]()
        {
            return readStitched(id, agent_id, archive_path, legacy_paths, limit, epoch,
                                serial, boundary, include_service, preview);
        },
        [id, epoch, serial, callback](HistoryResult result)
        {
            if (result.maintenance_needed && sRuntime.epoch == epoch)
            {
                auto found = sRuntime.residents.find(id);
                if (found != sRuntime.residents.end() &&
                    found->second.archive_serial == serial)
                {
                    found->second.summary.state = SUMMARY_UNPREPARED;
                    found->second.covered_token.clear();
                    wakeManager();
                }
            }

            callback(result);
        });
}

bool LLChatServiceHistory::deleteTranscriptsAsync(const delete_callback_t& callback)
{
    if (!sRuntime.running || sRuntime.delete_active || !callback)
    {
        return false;
    }

    U64 ticks = 0;
    if (!clickTicks(ticks))
    {
        return false;
    }

    // Latch deletion and retire outbound deadlines so no new request or view read can
    // start, and no expired wake source remains before pending privacy state is durable.
    sRuntime.delete_click_ticks = ticks;
    sRuntime.delete_callback = callback;
    sRuntime.delete_active = true;
    sRuntime.delete_requested = true;
    for (auto& pair : sRuntime.residents)
    {
        pair.second.outbound_refresh_due = 0.0;
    }

    wakeManager();
    return true;
}

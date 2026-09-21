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
#include "llcachename.h"
#include "llcallbacklist.h"
#include "llconversationlog.h"
#include "llcorehttputil.h"
#include "llcoros.h"
#include "lldate.h"
#include "lldir.h"
#include "lldiriterator.h"
#include "lleventcoro.h"
#include "llevents.h"
#include "llfile.h"
#include "fsyspath.h"
#include "llfloaterconversationpreview.h"
#include "llfloaterreg.h"
#include "llfloaterimsessiontab.h"
#include "llfloaterimsession.h"
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
#include <boost/scope/scope_exit.hpp>
#include <climits>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <sstream>

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
const F64 ACTIVITY_REFRESH_DELAY = 15.0;
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

enum EActivityDiscovery
{
    DISCOVERY_NONE,
    DISCOVERY_PENDING,
    DISCOVERY_CONFIRMING
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

struct Metadata
{
    EMetadata state = META_UNREQUESTED;
    F64 deadline = 0.0;
    LLAvatarName name;
    boost::signals2::connection connection;
};

struct Resident
{
    // Discovery identity and the durable service range already covered.
    std::string advertised_token;
    std::string covered_token;
    U32 archive_serial = 0;

    // Failed or unprepared archives require a scan before reuse.
    ArchiveScan summary{ARCHIVE_FAILED};
    Metadata metadata;

    // Coalesced scheduler state for this resident's current or next pass.
    bool listed = false;
    bool forced_followup = false;
    bool first_request_started = false;
    bool retry_used = false;
    bool metadata_waiting = false;

    // Incoming and outgoing IM bursts share one quiet deadline; unknown
    // conversations retain one bounded discovery confirmation.
    F64 activity_refresh_due = 0.0;
    EActivityDiscovery activity_discovery = DISCOVERY_NONE;

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

void requireRuntime(U32 epoch)
{
    if (!ownsRuntime(epoch))
    {
        LLTHROW(LLCoros::Stopped("Chat Service account ended"));
    }
}

template <typename Work>
auto runStorage(U32 epoch, Work work) -> decltype(work())
{
    // A storage wait may span logout. Unwind the manager before it can touch the
    // replacement account or a resident reference from the previous login.
    LL::WorkQueue::ptr_t general = LL::WorkQueue::getInstance("General");
    auto result = general ? general->waitForResult(work) : decltype(work())();
    requireRuntime(epoch);
    return result;
}

void postPresentation(const nullary_func_t& work)
{
    // All publishers run on the main thread. Idle registration cannot suspend;
    // presentation waits until the main-loop UI operation has unwound.
    const U32 epoch = sRuntime.epoch;
    doOnIdleOneTime([epoch, work]()
    {
        // Queued UI work belongs only to the login that scheduled it.
        if (ownsRuntime(epoch))
        {
            work();
        }
    });
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
        LLChatServiceHistory::servicePresentationAllowed();
    postPresentation([id]()
    {
        // Deliver current state, not a captured preview that may have been revoked
        // or replaced while this notification waited for the main loop.
        auto found = sRuntime.residents.find(id);
        if (found != sRuntime.residents.end())
        {
            const auto snapshot = found->second.snapshot;
            sSnapshotSignal(id, snapshot);
        }
    });
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
    if (handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    const bool success = FlushFileBuffers(handle) != 0;
    CloseHandle(handle);
#else
    const int descriptor = ::open(path.c_str(), O_RDONLY);
    if (descriptor < 0)
    {
        return false;
    }
    const bool success = ::fsync(descriptor) == 0;
    ::close(descriptor);
#endif
    return success;
}

bool writeFile(const std::string& path, const std::string& contents, bool durable)
{
    // Closing flushes the stream; privacy state also reaches stable storage before
    // any directory entry can make these bytes authoritative.
    llofstream output(path.c_str(), std::ios::binary | std::ios::trunc);
    output.write(contents.data(), contents.size());
    output.close();
    return !output.fail() && (!durable || syncFile(path));
}

bool replaceFile(const std::string& from, const std::string& to, bool durable)
{
    // Publish complete bytes atomically, then commit the directory update for privacy state.
#if LL_WINDOWS
    if (!MoveFileExW(ll_convert<std::wstring>(from).c_str(), ll_convert<std::wstring>(to).c_str(),
                     MOVEFILE_REPLACE_EXISTING | (durable ? MOVEFILE_WRITE_THROUGH : 0)))
#else
    if (::rename(from.c_str(), to.c_str()) != 0)
#endif
    {
        return false;
    }
    return !durable || syncDirectory(to);
}

bool writeReplace(const std::string& destination, const std::string& contents, bool durable)
{
    // A complete sibling temporary precedes replacement; neither path may alias
    // another file or directory.
    const std::string temporary = destination + ".tmp";
    bool exists = false;
    return inspectRegular(destination, exists) && inspectRegular(temporary, exists) &&
           writeFile(temporary, contents, durable) && replaceFile(temporary, destination, durable);
}

bool truncateArchive(const std::string& path, std::streamoff bytes)
{
    // The parser and file stamp identify the valid prefix. Truncation removes only
    // the incomplete suffix without copying or rewriting accumulated messages.
    bool exists = false;
    if (bytes < 0 || !inspectRegular(path, exists) || !exists)
    {
        return false;
    }
    std::error_code error;
    std::filesystem::resize_file(fsyspath(path), bytes, error);
    return !error;
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
            (LLFile::remove(temporary) != 0 || !syncDirectory(path)))
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

std::string stateContents(U64 boundary, bool pending)
{
    LLSD data;
    data["deleted_before_uuid_ticks"] = std::to_string(boundary);
    data["cleanup_pending"] = pending;
    std::ostringstream serialized;
    LLSDSerialize::toPrettyXML(data, serialized);
    return serialized.str();
}

bool recoverStateForDelete(const std::string& path, U64 boundary)
{
    // Stage through the owned index scratch path. An unsafe canonical remains in
    // place until a durable pending cutoff has reached the state temporary.
    const std::string temporary = path + ".tmp";
    const std::string::size_type separator = path.find_last_of("/\\");
    const std::string recovery = path.substr(0, separator + 1) + INDEX_NAME + ".tmp";
    if (LLFile::remove(recovery) != 0 ||
        !writeFile(recovery, stateContents(boundary, true), true) ||
        !replaceFile(recovery, temporary, true))
    {
        return false;
    }
    if (LLFile::remove(path) != 0)
    {
        return false;
    }
    return replaceFile(temporary, path, true);
}

bool writeState(const std::string& path, U64 boundary, bool pending)
{
    return writeReplace(path, stateContents(boundary, pending), true);
}

bool clearUnsafeStateTemporary(const std::string& path)
{
    const std::string temporary = path + ".tmp";
    bool exists = false;
    if (inspectRegular(temporary, exists))
    {
        return true;
    }

    // A nonregular temporary must be removed before durable replacement reuses it.
    // LLFile reports absence as success; its return value is authoritative.
    return LLFile::remove(temporary) == 0 && syncDirectory(path);
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

bool prepareArchive(const std::string& path, const LLUUID& resident_id, const LLUUID& agent_id,
                    U64 boundary, ArchiveScan& summary, bool& bytes_changed)
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
        summary = scan;
        return true;
    }
    if (scan.state == ARCHIVE_TORN)
    {
        // Repair only when the private physical stamp still matches the classified
        // file, then rescan the repaired file before publishing its summary.
        U64 current_size = 0;
        S64 current_mtime = 0;
        if (!archiveStamp(path, current_size, current_mtime) ||
            current_size != scan.file_size || current_mtime != scan.file_mtime ||
            !truncateArchive(path, scan.valid_prefix_bytes) ||
            !scanArchive(path, agent_id, resident_id, boundary, 0, scan))
        {
            summary.state = ARCHIVE_FAILED;
            return false;
        }
        bytes_changed = true;
        summary = scan;
        return true;
    }
    if (scan.state != ARCHIVE_CORRUPT)
    {
        summary.state = ARCHIVE_FAILED;
        return false;
    }

    // Preserve one structurally unusable canonical before a later full-window rebuild.
    const std::string corrupt = path + ".corrupt";
    if (LLFile::remove(corrupt) != 0)
    {
        summary.state = ARCHIVE_FAILED;
        return false;
    }
    U64 current_size = 0;
    S64 current_mtime = 0;
    if (!archiveStamp(path, current_size, current_mtime) ||
        current_size != scan.file_size || current_mtime != scan.file_mtime ||
        LLFile::rename(path, corrupt) != 0)
    {
        summary.state = ARCHIVE_FAILED;
        return false;
    }
    bytes_changed = true;
    summary = ArchiveScan();
    return true;
}

bool publishRows(const std::string& path, const std::vector<Row>& rows,
                 bool append, ArchiveScan& resulting)
{
    LLMutexLock lock(&sStorageMutex);
    if (rows.empty())
    {
        return true;
    }

    // The captured summary is a private physical identity check. External changes
    // abort publication instead of reconciling unknown bytes.
    bool exists = false;
    if (!inspectRegular(path, exists) || exists != (resulting.state != ARCHIVE_ABSENT))
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
    resulting.state = ARCHIVE_VALID;
    if (!resulting.has_oldest)
    {
        resulting.has_oldest = true;
        resulting.oldest = rows.front().key;
    }
    resulting.row_count += static_cast<U32>(rows.size());
    resulting.newest = rows.back().key;
    if (!archiveStamp(path, resulting.file_size, resulting.file_mtime))
    {
        return false;
    }
    resulting.valid_prefix_bytes = resulting.file_size;
    return true;
}

typedef std::pair<std::vector<LLUUID>, bool> initial_artifacts_t;

initial_artifacts_t enumerateArchives(const std::string& directory)
{
    // Discover canonical archives and deletable remnants in one directory walk.
    // The resident map supplies ordering and uniqueness when these IDs are applied.
    initial_artifacts_t result;
    LLDirIterator iterator(directory, "chat_service_*");
    std::string name;
    while (iterator.next(name))
    {
        LLUUID id;
        if (canonicalArchiveName(name, id))
        {
            result.first.push_back(id);
        }
        if (ownedArtifactName(name) && name != INDEX_NAME &&
            name != std::string(INDEX_NAME) + ".tmp" && name != STATE_NAME &&
            name != std::string(STATE_NAME) + ".tmp")
        {
            result.second = true;
        }
    }
    return result;
}

using index_entries_t = std::map<LLUUID, std::optional<LLAvatarName>>;

bool regenerateIndex(const index_entries_t& archives,
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
            if (LLFile::remove(path) != 0)
            {
                return false;
            }
        }
    }

    // The captured entries already identify valid archives. Keep only files still
    // present, spelling out names that remain unresolved at this publication.
    std::ostringstream output;
    output << INDEX_HEADER;
    bool has_entries = false;
    for (const auto& [id, name] : archives)
    {
        bool exists = false;
        if (!inspectRegular(archivePath(directory, delimiter, id), exists) || !exists)
        {
            continue;
        }
        has_entries = true;
        output << quoteCsv(archiveName(id)) << ',' << id.asString() << ','
               << quoteCsv(name ? name->getAccountName() : PENDING_NAME) << ','
               << quoteCsv(name ? name->getDisplayName() : PENDING_NAME) << '\n';
    }
    if (!has_entries)
    {
        return LLFile::remove(index) == 0;
    }
    return writeReplace(index, output.str(), false);
}

HttpResult request(const std::string& url, const LLSD* post, U32 epoch)
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
    requireRuntime(epoch);
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

bool ensureMetadata(const LLUUID& id, Resident& resident)
{
    // A resolved record is stable for this login; view loads need no new publication.
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
    if (resident.metadata.state == META_PENDING || resident.metadata.state == META_FAILED)
    {
        return false;
    }

    // The name cache owns batching and request deduplication. Keep one subscription
    // per resident and a presentation timeout; cache hits may complete synchronously.
    resident.metadata.state = META_PENDING;
    resident.metadata.deadline = F64(LLTimer::getTotalSeconds()) + NAME_TIMEOUT;
    const U32 epoch = sRuntime.epoch;
    resident.metadata.connection = LLAvatarNameCache::get(
        id, [id, epoch](const LLUUID&, const LLAvatarName& name)
        {
            // Cache callbacks run on the main loop. Timeout disconnects the slot;
            // the epoch also fences logout and account replacement.
            if (!sRuntime.running || sRuntime.epoch != epoch)
            {
                return;
            }
            auto found = sRuntime.residents.find(id);
            if (found == sRuntime.residents.end() ||
                found->second.metadata.state != META_PENDING)
            {
                return;
            }
            found->second.metadata.connection.disconnect();
            found->second.metadata.state = META_RESOLVED;
            found->second.metadata.name = name;
            sRuntime.index_dirty = true;
            if (found->second.metadata_waiting)
            {
                queueResident(id, sRuntime.priority_resident == id);
                found->second.metadata_waiting = false;
            }
            publishSnapshot(id, found->second);
            wakeManager();
        });
    return resident.metadata.state == META_RESOLVED;
}

bool baseNetworkEligible(const CapabilityContext& context)
{
    // The shared gate is re-evaluated before every request and publication phase.
    return sRuntime.running && LLChatServiceHistory::servicePresentationAllowed() &&
           context.complete() &&
           LLMuteList::getInstance() && LLMuteList::getInstance()->isLoadedFromServer();
}

bool networkEligible(const LLUUID& id, const Resident& resident,
                     const CapabilityContext& context)
{
    return baseNetworkEligible(context) && id.notNull() && !nameBlocked(id, resident);
}

void expireMetadata();
void activateDueActivityRefreshes();

void waitForWake(F64 seconds, U32 epoch)
{
    // The login creates the wake pump before launching the manager. Every wait
    // ends the old coroutine on logout before it can resume account work.
    requireRuntime(epoch);
    sRuntime.wake_pending = false;
    sWake->discard();
    llcoro::suspendUntilEventOnWithTimeout(*sWake, static_cast<F32>(llmax(0.01, seconds)),
                                           LLSDMap("timeout", true));
    requireRuntime(epoch);
    sRuntime.wake_pending = false;
}

bool pace(const CapabilityContext& context, U32 epoch,
          const LLUUID& resident_id = LLUUID::null)
{
    // Pacing remains interruptible so lifecycle, capability, mute, metadata, and
    // deletion changes can cancel a queued request before network I/O begins.
    requireRuntime(epoch);
    for (;;)
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
        activateDueActivityRefreshes();
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
            if (pair.second.activity_refresh_due > now)
            {
                wait = llmin(wait, pair.second.activity_refresh_due - now);
            }
        }
        waitForWake(wait, epoch);
        expireMetadata();
    }
}

void handleRequestFailure(const LLUUID& id, Resident& resident, S32 status)
{
    if (status == 429)
    {
        // One manager-wide cooldown survives repeated throttles and every priority trigger.
        sRuntime.network_not_before = llmax(sRuntime.network_not_before,
            F64(LLTimer::getTotalSeconds()) + RATE_LIMIT_DELAY);
        queueResident(id, sRuntime.priority_resident == id);
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
}

bool processList(const LLSD& body, bool& confirm_activity_discovery)
{
    confirm_activity_discovery = false;
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
    // appending background work. The first miss after an activity hint retains its
    // placeholder for one delayed confirmation; the next valid miss retires it.
    for (auto& pair : sRuntime.residents)
    {
        pair.second.listed = listed.count(pair.first) != 0;
        if (!pair.second.listed)
        {
            if (pair.second.activity_discovery == DISCOVERY_PENDING)
            {
                pair.second.activity_discovery = DISCOVERY_CONFIRMING;
                confirm_activity_discovery = true;
                continue;
            }

            pair.second.activity_discovery = DISCOVERY_NONE;
            sRuntime.queue.erase(std::remove(sRuntime.queue.begin(), sRuntime.queue.end(), pair.first),
                                 sRuntime.queue.end());
            if (sRuntime.priority_resident == pair.first)
            {
                sRuntime.priority_resident.setNull();
            }
            pair.second.metadata_waiting = false;
            setWorkActive(pair.first, false);
        }
    }
    for (const ListEntry& entry : entries)
    {
        Resident& resident = sRuntime.residents[entry.resident_id];
        const bool placeholder = std::find(sRuntime.queue.begin(), sRuntime.queue.end(),
                                           entry.resident_id) != sRuntime.queue.end();
        const bool changed = resident.advertised_token != entry.last_msg_id;
        resident.advertised_token = entry.last_msg_id;
        resident.listed = true;
        resident.activity_discovery = DISCOVERY_NONE;
        if (resident.metadata.state == META_FAILED)
        {
            resident.metadata.state = META_UNREQUESTED;
        }

        // Empty coverage queues first discovery. Later lists queue changed or
        // uncovered heads; token equality is not a TimeUUID ordering claim.
        if (!placeholder && (changed || resident.covered_token != resident.advertised_token))
        {
            resident.retry_used = false;
            queueResident(entry.resident_id, false);
        }
    }
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
    // passes strict validation. A pending activity hint remains visible across the
    // HTTP suspension and is consumed by processList rather than this request latch.
    const HttpResult response = request(context.list_url, NULL, epoch);
    LL_INFOS("ChatServiceHistory")
        << "Conversation-list response: status=" << response.status << LL_ENDL;
    if (sampleContext() != context || sRuntime.delete_requested ||
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
        bool confirm_activity_discovery = false;
        if (processList(response.body, confirm_activity_discovery))
        {
            const F64 now = F64(LLTimer::getTotalSeconds());
            sRuntime.list_needed = confirm_activity_discovery;
            sRuntime.list_retry_used = false;
            sRuntime.next_list = now + LIST_INTERVAL;
            if (confirm_activity_discovery)
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

bool prepareResidentArchive(const LLUUID& id, U32 epoch)
{
    const std::string path = archivePath(sRuntime.account_dir, sRuntime.delimiter, id);
    const LLUUID agent_id = sRuntime.agent_id;
    const U64 boundary = sRuntime.deleted_before_ticks;
    bool changed = false;
    ArchiveScan summary;
    const bool prepared = runStorage(epoch,
        [path, id, agent_id, boundary, &summary, &changed]()
        {
            return prepareArchive(path, id, agent_id, boundary, summary, changed);
        });
    if (!prepared)
    {
        return false;
    }

    // Only this manager removes residents, so the epoch check preserves their
    // identity across the worker wait. Repair invalidates both views and coverage.
    Resident& current = sRuntime.residents.at(id);
    current.summary = summary;
    sRuntime.index_dirty = true;
    if (changed)
    {
        ++current.archive_serial;
        current.covered_token.clear();
        sRuntime.local_content_exists = true;
    }
    publishSnapshot(id, current);
    return true;
}

void syncResident(const LLUUID& id, const CapabilityContext& context, U32 epoch)
{
    auto found = sRuntime.residents.find(id);
    if (found == sRuntime.residents.end())
    {
        return;
    }
    Resident& resident = found->second;

    // Resolve prerequisites before starting a pass. Only the manager erases
    // residents; its checked waits terminate before a reference can become stale.
    if (!resident.listed)
    {
        resident.activity_discovery = DISCOVERY_NONE;
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
        return;
    }
    resident.metadata_waiting = false;
    if (!networkEligible(id, resident, context))
    {
        setWorkActive(id, false);
        clearPriority(id);
        return;
    }
    setWorkActive(id, true);
    sRuntime.active_resident = id;
    resident.first_request_started = false;

    // Every exit retires the transient head and active pass. Queued retries keep
    // their work indicator; logout leaves the replacement account untouched.
    boost::scope::scope_exit finish([epoch, id, &resident]()
    {
        if (!ownsRuntime(epoch))
        {
            return;
        }
        resident.snapshot.head_preview.clear();
        if (std::find(sRuntime.queue.begin(), sRuntime.queue.end(), id) == sRuntime.queue.end())
        {
            resident.snapshot.service_work_active = false;
            clearPriority(id);
        }
        sRuntime.active_resident.setNull();
        publishSnapshot(id, resident);
    });

    bool needs_prepare = resident.summary.state == ARCHIVE_FAILED;
    bool had_boundary = !needs_prepare && resident.summary.state == ARCHIVE_VALID &&
                        resident.summary.has_oldest;
    TimeUuidKey stored_newest = resident.summary.newest;
    const std::string conversation_id = directConversationId(sRuntime.agent_id, id);
    std::vector<Row> staged;
    size_t staged_bytes = 0;
    U32 returned_rows = 0;
    std::string cursor;
    bool complete = false;
    bool exposed_preview = false;

    // Stage a complete bounded pass before changing canonical bytes. Pacing and
    // each awaited page remain cancellation/priority boundaries.
    while (!complete)
    {
        if (!pace(context, epoch, id))
        {
            // Capability or account-wide gates retain one queued occurrence;
            // a blocked resident stays dormant until a fresh ordinary trigger.
            setWorkActive(id, false);
            LLMuteList* mute = LLMuteList::getInstance();
            const bool temporarily_ineligible = !sRuntime.delete_requested &&
                (sampleContext() != context || !baseNetworkEligible(context) ||
                 !mute || !mute->isLoadedFromServer());
            if (temporarily_ineligible &&
                std::find(sRuntime.queue.begin(), sRuntime.queue.end(), id) == sRuntime.queue.end())
            {
                queueResident(id, sRuntime.priority_resident == id);
            }
            return;
        }
        if (sRuntime.priority_resident.notNull() && sRuntime.priority_resident != id)
        {
            queueResident(id, false);
            return;
        }

        // The first head request covers a burst whose quiet deadline has elapsed;
        // continuation pages never consume a newer activity deadline.
        if (!resident.first_request_started && cursor.empty() &&
            resident.activity_refresh_due > 0.0 &&
            resident.activity_refresh_due <= F64(LLTimer::getTotalSeconds()))
        {
            resident.activity_refresh_due = 0.0;
        }
        resident.first_request_started = true;
        LLSD post;
        post["conversation_id"] = conversation_id;
        post["limit"] = 100;
        if (!cursor.empty())
        {
            post["before_msg_id"] = cursor;
        }

        const HttpResult response = request(context.history_url, &post, epoch);
        LL_INFOS("ChatServiceHistory")
            << "History response: resident_id=" << id << ", status=" << response.status
            << ", cursor=" << (cursor.empty() ? "head" : "continuation") << LL_ENDL;
        if (sRuntime.delete_requested || sampleContext() != context ||
            !networkEligible(id, resident, context))
        {
            return;
        }
        if (response.status < 200 || response.status >= 300)
        {
            handleRequestFailure(id, resident, response.status);
            return;
        }
        Page page;
        if (!validateHistoryPage(response.body, sRuntime.agent_id, id, cursor,
                                 sRuntime.deleted_before_ticks, page))
        {
            return;
        }

        // Apply priority changes only after the complete response validates.
        activateDueActivityRefreshes();
        if (sRuntime.priority_resident.notNull() && sRuntime.priority_resident != id)
        {
            queueResident(id, false);
            return;
        }
        if (!exposed_preview && !page.rows.empty())
        {
            resident.snapshot.head_preview = page.rows;
            exposed_preview = true;
            publishSnapshot(id, resident);
        }

        // Present the first page before scanning a potentially years-long archive.
        if (needs_prepare)
        {
            if (!prepareResidentArchive(id, epoch))
            {
                return;
            }
            needs_prepare = false;
            had_boundary = resident.summary.state == ARCHIVE_VALID && resident.summary.has_oldest;
            stored_newest = resident.summary.newest;
            activateDueActivityRefreshes();
            if (sRuntime.priority_resident.notNull() && sRuntime.priority_resident != id)
            {
                queueResident(id, false);
                return;
            }
        }

        // Strictly descending pages accumulate only above the durable newest key.
        // Count every returned row and retained payload against the pass bounds.
        for (const Row& row : page.rows)
        {
            if (++returned_rows > MAX_PASS_ROWS)
            {
                return;
            }
            if (had_boundary && row.key <= stored_newest)
            {
                complete = true;
                break;
            }
            const size_t bytes = row.conversation_id.size() + row.msg_id.size() + 36 +
                row.from_name.size() + row.message.size() + std::to_string(row.dialog).size() +
                row.created_at.size();
            if (staged_bytes + bytes > MAX_PASS_BYTES)
            {
                return;
            }
            staged_bytes += bytes;
            staged.push_back(row);
        }
        complete = complete || page.terminal;
        cursor = page.next_cursor;
    }

    // Recheck dispatch gates after the final page/scan, then publish oldest-first.
    if (!networkEligible(id, resident, context) || sRuntime.delete_requested ||
        sampleContext() != context)
    {
        return;
    }
    std::reverse(staged.begin(), staged.end());
    bool changed = false;
    if (!staged.empty())
    {
        ArchiveScan resulting = resident.summary;
        const bool append = resident.summary.state == ARCHIVE_VALID && resident.summary.has_oldest;
        const std::string path = archivePath(sRuntime.account_dir, sRuntime.delimiter, id);
        changed = runStorage(epoch, [path, staged, append, &resulting]()
        {
            return publishRows(path, staged, append, resulting);
        });
        LL_INFOS("ChatServiceHistory")
            << "Archive publication: resident_id=" << id << ", rows=" << staged.size()
            << ", mode=" << (append ? "append" : "replace")
            << ", success=" << changed << LL_ENDL;

        // Success and partial-write failure both invalidate readers and the manifest.
        ++resident.archive_serial;
        sRuntime.index_dirty = true;
        sRuntime.local_content_exists = true;
        if (changed)
        {
            resident.summary = resulting;
            if (resident.metadata.state == META_RESOLVED)
            {
                const F64 archived_seconds = resident.summary.newest.ticks >= UUID_EPOCH
                    ? static_cast<F64>(resident.summary.newest.ticks - UUID_EPOCH) / 10000000.0
                    : static_cast<F64>(time_corrected());
                const LLAvatarName name = resident.metadata.name;
                postPresentation([id, name, archived_seconds]()
                {
                    if (!LLChatServiceHistory::historySuppressed())
                    {
                        LLConversationLog::instance().addServiceConversation(
                            LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, id),
                            name.getCompleteName(),
                            LLCacheName::buildUsername(name.getUserName()),
                            id,
                            U64Seconds(LLUnits::Seconds::fromValue(archived_seconds)));
                    }
                });
            }
        }
        else
        {
            // A partial write needs a fresh scan before another pass can claim coverage.
            resident.summary.state = ARCHIVE_FAILED;
            resident.covered_token.clear();
        }
        postPresentation(LLLogChat::notifyTranscriptCreated);
    }

    // Only a complete pass claims coverage. At most one activity-triggered follow-up
    // survives; the scope exit handles preview and work-status publication.
    if (sRuntime.delete_requested)
    {
        return;
    }
    if (changed || staged.empty())
    {
        resident.covered_token = resident.advertised_token;
        resident.retry_used = false;
    }
    if (resident.forced_followup && networkEligible(id, resident, context))
    {
        resident.forced_followup = false;
        queueResident(id, true);
    }
    else
    {
        resident.snapshot.service_work_active = false;
    }
    clearPriority(id);
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
    // Attempt every owned path under the storage lock. Missing paths are already
    // successful removals; any reported failure keeps cleanup pending.
    for (const std::string& path : targets)
    {
        if (LLFile::remove(path) != 0)
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
    }

    postPresentation([success, callback]()
    {
        if (!success)
        {
            LLNotificationsUtil::add("ChatServiceHistoryDeleteFailed");
        }
        if (callback)
        {
            callback(success);
        }
        LLLogChat::notifyTranscriptCreated();
    });
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

    // Await invalidation on the main loop before sweeping files or accepting new
    // history. A late clear must not discard messages received after deletion.
    LL::WorkQueue::ptr_t main = LL::WorkQueue::getInstance("mainloop");
    const bool cleared = main && main->waitForResult([epoch]()
    {
        if (!ownsRuntime(epoch))
        {
            return false;
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
        return true;
    });
    requireRuntime(epoch);
    if (!cleared)
    {
        finishDelete(false);
        return;
    }

    // Sweep legacy transcripts first, then service-owned artifacts, through the
    // shared filesystem mutation boundaries on the General queue.
    const std::string chat_logs_dir =
        gDirUtilp ? gDirUtilp->getPerAccountChatLogsDir() : std::string();
    const bool swept = runStorage(epoch,
        [chat_logs_dir, directory = sRuntime.account_dir,
         delimiter = sRuntime.delimiter, state_path]()
        {
            return !chat_logs_dir.empty() && LLLogChat::deleteTranscriptContent(chat_logs_dir) &&
                   sweepServiceArtifacts(directory, delimiter, state_path);
        });
    if (!swept)
    {
        finishDelete(false);
        return;
    }

    // Clear pending only after both sweeps and their required directory syncs succeed.
    {
        LLMutexLock lock(&sStorageMutex);
        if (!writeState(state_path, boundary, false))
        {
            sRuntime.state_safety = STATE_UNSAFE;
            finishDelete(false);
            return;
        }
    }

    // Reset cached summaries last so subsequent discovery starts from empty storage.
    sRuntime.residents.clear();
    sRuntime.initialized_archives = true;
    sRuntime.local_content_exists = false;
    finishDelete(true);
}

void initializeArchives(U32 epoch)
{
    // Discover account-local artifacts off the main thread; individual archives are
    // prepared incrementally by the manager so startup remains bounded.
    const auto artifacts = runStorage(epoch, [directory = sRuntime.account_dir]()
    {
        return enumerateArchives(directory);
    });
    sRuntime.local_content_exists = artifacts.second;
    for (const LLUUID& id : artifacts.first)
    {
        sRuntime.residents[id].summary.state = ARCHIVE_FAILED;
    }
    sRuntime.initialized_archives = true;
    sRuntime.index_dirty = true;
}

bool prepareOneArchive(U32 epoch)
{
    for (auto& pair : sRuntime.residents)
    {
        Resident& resident = pair.second;
        if (resident.summary.state != ARCHIVE_FAILED)
        {
            continue;
        }

        // Prepare one archive per manager turn so network priority and lifecycle
        // events can interleave with storage maintenance.
        const LLUUID id = pair.first;
        if (!prepareResidentArchive(id, epoch))
        {
            return false;
        }
        Resident& current = sRuntime.residents.at(id);
        if (current.summary.has_oldest)
        {
            ensureMetadata(id, current);
        }
        postPresentation(LLLogChat::notifyTranscriptCreated);
        return true;
    }
    return false;
}

bool updateIndex(U32 epoch)
{
    // Clear the dirty latch before suspension so metadata callbacks can request
    // another pass while this captured manifest is being written.
    sRuntime.index_dirty = false;
    index_entries_t archives;
    for (const auto& pair : sRuntime.residents)
    {
        if (pair.second.summary.state == ARCHIVE_VALID)
        {
            archives[pair.first] = pair.second.metadata.state == META_RESOLVED
                ? std::make_optional(pair.second.metadata.name) : std::nullopt;
        }
    }
    const bool updated = runStorage(epoch,
        [archives, directory = sRuntime.account_dir,
         delimiter = sRuntime.delimiter]()
        {
            return regenerateIndex(archives, directory, delimiter);
        });
    sRuntime.index_dirty |= !updated;
    return updated;
}

void expireMetadata()
{
    // A timed-out lookup leaves the resident dormant until a later ordinary trigger.
    const F64 now = LLTimer::getTotalSeconds();
    for (auto& pair : sRuntime.residents)
    {
        Metadata& metadata = pair.second.metadata;
        if (metadata.state == META_PENDING && metadata.deadline <= now)
        {
            metadata.connection.disconnect();
            metadata.state = META_FAILED;
            pair.second.metadata_waiting = false;
            setWorkActive(pair.first, false);
            clearPriority(pair.first);
        }
    }
}

void activateDueActivityRefreshes()
{
    // Revoked account gates retire pending activity before it can remain
    // a wake source or schedule service work.
    if (!LLChatServiceHistory::servicePresentationAllowed())
    {
        for (auto& pair : sRuntime.residents)
        {
            pair.second.activity_refresh_due = 0.0;
        }
        return;
    }

    const F64 now = LLTimer::getTotalSeconds();
    for (auto& pair : sRuntime.residents)
    {
        Resident& resident = pair.second;
        if (resident.activity_refresh_due <= 0.0 || resident.activity_refresh_due > now)
        {
            continue;
        }

        // Consume the deadline before reusing the existing priority/discovery path
        // so an expired value cannot create a rapid manager wake loop.
        resident.activity_refresh_due = 0.0;
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
        else if (resident.activity_discovery == DISCOVERY_NONE)
        {
            resident.activity_discovery = DISCOVERY_PENDING;
            LLChatServiceHistory::prioritizeResident(pair.first);
        }
    }
}

F64 nearestWait()
{
    // Sleep until the earliest list, pacing, metadata, or activity quiet deadline;
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
        if (LLChatServiceHistory::servicePresentationAllowed() &&
            pair.second.activity_refresh_due > 0.0)
        {
            deadline = llmin(deadline, pair.second.activity_refresh_due);
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
            continue;
        }
        if (sRuntime.state_safety == STATE_UNKNOWN)
        {
            // Load privacy state before archives, views, or network work become eligible.
            const std::string state_path = childPath(sRuntime.account_dir,
                                                     sRuntime.delimiter, STATE_NAME);
            const StateResult state = runStorage(epoch, [state_path]()
            {
                LLMutexLock lock(&sStorageMutex);
                return loadState(state_path);
            });
            sRuntime.state_safety = state.safety;
            sRuntime.deleted_before_ticks = state.boundary;
            sRuntime.cleanup_pending = state.cleanup_pending;
            postPresentation([]()
            {
                LLLogChat::notifyTranscriptCreated();
                if (!LLChatServiceHistory::historySuppressed())
                {
                    LLFloaterIMSessionTab::processChatHistoryStyleUpdate(true);
                }
            });
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
            initializeArchives(epoch);
            continue;
        }

        // Expire timers and activate due activity bursts before choosing network work.
        expireMetadata();
        activateDueActivityRefreshes();

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

        // Discovery precedes resident paging; opens and due activity share the
        // resident priority queue.
        const bool base_network = baseNetworkEligible(context);
        LLMuteList* mute = LLMuteList::getInstance();
        // Once all external inputs are available, record why synchronization stayed gated.
        if (!base_network && context.complete() && mute && mute->isLoaded())
        {
            LL_INFOS_ONCE("ChatServiceHistory")
                << "Network sync blocked: rollout=" << sRuntime.rollout
                << ", consent=" << transcriptConsent()
                << ", state_safe=" << (sRuntime.state_safety == STATE_SAFE)
                << ", cleanup_pending=" << sRuntime.cleanup_pending
                << ", delete_requested=" << sRuntime.delete_requested
                << ", mute_source=" << mute->getLoadSourceName()
                << ", mute_authoritative=" << mute->isLoadedFromServer()
                << LL_ENDL;
        }
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
                continue;
            }
            if (!sRuntime.queue.empty())
            {
                const LLUUID id = sRuntime.queue.front();
                sRuntime.queue.pop_front();
                syncResident(id, context, epoch);
                continue;
            }

            // Existing archives use the same cached names as blocking and views.
            for (auto& pair : sRuntime.residents)
            {
                if (pair.second.summary.has_oldest && pair.second.metadata.state == META_UNREQUESTED)
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
        if (sRuntime.index_dirty && sRuntime.state_safety == STATE_SAFE &&
            !sRuntime.cleanup_pending)
        {
            if (updateIndex(epoch))
            {
                continue;
            }
        }
        waitForWake(nearestWait(), epoch);
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
    U64 boundary, bool include_service, const std::vector<Row>& preview)
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
        LLLogChat::loadChatHistoryExact(path, legacy, parameters);
    }

    const F64 service_epoch = archive.has_oldest
        ? static_cast<F64>(archive.oldest.ticks - UUID_EPOCH) / 10000000.0 : 0.0;

    // Admit only the legacy prefix that may predate the durable service boundary.
    // Offset-less SLT timestamps use their earliest UTC interpretation (UTC-7).
    for (const LLSD& message : legacy)
    {
        F64 wall = 0.0;
        if (!legacyWallEpoch(message[LL_IM_DATE_TIME].asString(), wall))
        {
            result.messages.push_back(message);
        }
        else if (!archive.has_oldest || legacyWallMayPrecedeService(wall, service_epoch))
        {
            LLSD stitched = message;
            stitched[LEGACY_WALL_TIME] = wall;
            result.messages.push_back(stitched);
        }
    }
    // Storage reads and preview publications share the same occurrence-aware seam.
    std::list<LLSD> service;
    for (const Row& row : archive.display_rows)
    {
        service.push_back(serviceMessage(row));
    }
    for (const Row& row : preview)
    {
        service.push_back(serviceMessage(row));
    }
    result.messages = mergeDirectHistory(result.messages, service);

    // Apply the consumer limit to the complete seam order, retaining the service tail.
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
                            pair.second.activity_refresh_due = 0.0;
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
                    postPresentation([]()
                    {
                        LLFloaterIMSessionTab::processChatHistoryStyleUpdate(true);
                    });
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
        if (pair.second.summary.has_oldest)
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
    if (found == sRuntime.residents.end() || found->second.summary.state == ARCHIVE_FAILED)
    {
        wakeManager();
        return false;
    }

    return found->second.summary.has_oldest;
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
    const bool potentially_active = servicePresentationAllowed() &&
        context.complete() && !uuidBlocked(id);
    setWorkActive(id, potentially_active);
    wakeManager();
}

void LLChatServiceHistory::noteDirectMessageActivity(const LLUUID& id)
{
    if (!sRuntime.running || !sRuntime.rollout || !transcriptConsent() ||
        sRuntime.delete_requested || id.isNull() || id == sRuntime.agent_id)
    {
        return;
    }

    // Each incoming or outgoing IM resets the quiet deadline. First contact
    // also starts bounded discovery immediately.
    Resident& resident = sRuntime.residents[id];
    resident.activity_refresh_due =
        F64(LLTimer::getTotalSeconds()) + ACTIVITY_REFRESH_DELAY;
    if (!resident.listed && resident.activity_discovery == DISCOVERY_NONE)
    {
        resident.activity_discovery = DISCOVERY_PENDING;
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

static LLChatServiceHistory::Messages composeHistory(
    LLChatServiceHistoryCore::History& history,
    const LLChatServiceHistory::Snapshot& snapshot, U32 limit, const LLUUID& session_id)
{
    Messages preview;
    if (snapshot.service_presentation_allowed)
    {
        for (const Row& row : snapshot.head_preview)
        {
            preview.push_back(serviceMessage(row));
        }
    }
    else
    {
        history.clearService();
    }

    // Preview has no live rows. An IM session supplies only its append-only deliveries.
    Messages live;
    LLFloaterIMSession* floater = nullptr;
    if (session_id.notNull())
    {
        floater = LLFloaterIMSession::findInstance(session_id);
        if (const auto* session = LLIMModel::instance().findIMSession(session_id))
        {
            for (const LLSD& message : session->mMsgs)
            {
                if (!message["is_history"].asBoolean())
                {
                    live.push_back(message);
                }
            }
        }
    }
    return history.compose(preview, live, limit, floater && floater->isInVisibleChain());
}

bool LLChatServiceHistory::replaceHistory(Messages& current, const Messages& history, bool direct)
{
    Messages resolved = history;
    // Legacy names can acquire a UUID from local caches without starting a lookup.
    // Leave unknown IDs absent so reconciliation can still use the resident name.
    for (LLSD& message : resolved)
    {
        const LLSD& source = message;
        if (!source[LL_IM_FROM_ID].isDefined())
        {
            const LLUUID id = LLAvatarNameCache::getInstance()->findIdByName(
                LLCacheName::buildLegacyName(source[LL_IM_FROM].asString()));
            if (id.notNull())
            {
                message[LL_IM_FROM_ID] = id;
            }
        }
    }
    return LLChatServiceHistoryCore::replaceHistory(current, resolved, direct);
}

LLSD LLChatServiceHistory::prepareLiveMessage(
    const std::string& text, U32& timestamp, const LLSD& context)
{
    return captureLiveMessage(text, timestamp, context, static_cast<U32>(time_corrected()));
}

void LLChatServiceHistory::recordLiveMessage(LLSD& message, const LLSD& context)
{
    LLChatServiceHistoryCore::recordLiveMessage(message, context, static_cast<U32>(time_corrected()));
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
                    found->second.summary.state = ARCHIVE_FAILED;
                    found->second.covered_token.clear();
                    wakeManager();
                }
            }

            callback(result);
        });
}

void LLChatServiceHistory::History::load(
    const LLUUID& resident_id, const std::string& legacy_stem, U32 limit,
    const callback_t& callback, const LLUUID& session_id)
{
    // Replacing a request retains visible context but retires its completion.
    const bool first_load = !mConnection.connected();
    ++mToken;
    mLoading = false;
    mResidentID = resident_id;
    mSessionID = session_id;
    mLegacyStem = legacy_stem;
    mLimit = limit;
    mCallback = callback;
    mConnection = setSnapshotChanged([this](const LLUUID& changed, const Snapshot& snapshot)
    {
        if (changed == mResidentID)
        {
            onSnapshot(snapshot);
        }
    });

    // Subscribe before querying; a cold name cache may supply the plaintext stem
    // later, and the first service page can be displayed while disk work runs.
    const Snapshot snapshot = getSnapshot(mResidentID);
    mArchiveSerial = snapshot.archive_serial;
    if (first_load)
    {
        mServiceAllowed = snapshot.service_presentation_allowed;
    }
    if (historySuppressed())
    {
        clear();
        mCallback({});
        return;
    }
    onSnapshot(snapshot);
    if (!mLoading)
    {
        reload();
    }
}

void LLChatServiceHistory::History::clear()
{
    // Deletion invalidates pending reads before owners clear their displayed history.
    ++mToken;
    mLoading = false;
    mArchiveSerial = 0;
    mServiceAllowed = false;
    mHistory.clear();
}

void LLChatServiceHistory::History::stop()
{
    mConnection.disconnect();
    clear();
}

void LLChatServiceHistory::History::publish(const Snapshot& snapshot)
{
    mCallback(composeHistory(mHistory, snapshot, mLimit, mSessionID));
}

void LLChatServiceHistory::History::reload()
{
    const U64 token = ++mToken;
    const std::string legacy_stem = mLegacyStem;
    mLoading = loadStitchedHistory(mResidentID, legacy_stem, mLimit,
        [handle = getHandle(), token, legacy_stem](const HistoryResult& result)
        {
            History* self = handle.get();
            if (!self || token != self->mToken)
            {
                return;
            }
            self->mLoading = false;

            // Account changes and deletion discard the read. Archive publication,
            // consent, or newly resolved names require one read of the current inputs.
            if (result.account_epoch != accountEpoch() || historySuppressed())
            {
                return;
            }
            const Snapshot snapshot = getSnapshot(self->mResidentID);
            if (result.archive_serial != snapshot.archive_serial ||
                result.included_service != snapshot.service_presentation_allowed ||
                legacy_stem != self->mLegacyStem)
            {
                self->reload();
                return;
            }

            self->mArchiveSerial = result.archive_serial;
            self->mHistory.setLoaded(result.messages);
            self->publish(snapshot);
        });
    // Open IMs fall back to plaintext when the queues are unavailable; Preview
    // stays nonblocking.
    if (!mLoading && mSessionID.notNull() && !historySuppressed())
    {
        Messages legacy;
        LLLogChat::loadChatHistory(mLegacyStem, legacy);
        mHistory.setLoaded(legacy);
        mCallback(legacy);
    }
}

void LLChatServiceHistory::History::onSnapshot(const Snapshot& snapshot)
{
    if (historySuppressed())
    {
        return;
    }
    const bool presentation_changed = mServiceAllowed != snapshot.service_presentation_allowed;
    mServiceAllowed = snapshot.service_presentation_allowed;

    // A service-only conversation gains its plaintext history when its name resolves.
    const bool name_resolved = mLegacyStem.empty() && snapshot.metadata_resolved;
    if (name_resolved)
    {
        mLegacyStem = LLCacheName::buildUsername(snapshot.metadata.getUserName());
    }
    // Apply consent changes and validated previews while the disk read catches up.
    if (presentation_changed || !snapshot.head_preview.empty())
    {
        publish(snapshot);
    }

    // In-flight reads compare their captured inputs on completion, coalescing any
    // number of snapshot changes into one follow-up read.
    if (!mLoading && (presentation_changed || name_resolved ||
                      snapshot.archive_serial != mArchiveSerial))
    {
        reload();
    }
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

    // Latch deletion and retire activity deadlines so no new request or view read can
    // start, and no expired wake source remains before pending privacy state is durable.
    sRuntime.delete_click_ticks = ticks;
    sRuntime.delete_callback = callback;
    sRuntime.delete_active = true;
    sRuntime.delete_requested = true;
    for (auto& pair : sRuntime.residents)
    {
        pair.second.activity_refresh_due = 0.0;
    }

    wakeManager();
    return true;
}

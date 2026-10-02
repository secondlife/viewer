/**
 *
 * @file llvlcproducer.cpp
 * @brief SLVlcProducer: hosts real LibVLC media players on demand for two different
 *        viewer-side consumers over llshmframe -- llembeddedbrowser's own RTSP/RTMP/MMS
 *        prim media (schemes CEF cannot play at all; a normal, visual slot), and
 *        LLStreamingAudio_LibVLC's parcel/streaming-music audio (a frame-less,
 *        audio-only slot -- see Slot::isAudioOnly below). A standalone process, separate
 *        from SLCefProducer, split from the former combined SLMediaProducer 2026-09-30 for
 *        licensing reasons: the vendored libvlc package is declared GPL v2 in its own
 *        package metadata, while this project is LGPL v2.1 -- confining every use of
 *        libvlc to this one small process, reached only via IPC, keeps it out of both
 *        the main Viewer binary and any process that also links CEF.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

// llvlcproducer.cpp
//
// Same control-channel + on-demand-allocate + idle-teardown skeleton as the sibling
// SLCefProducer (indra/llcefproducer/llcefproducer.cpp, itself adapted from
// llcefshm-example's own src/cefshm_producer.cpp), same wire protocol (cefshm_protocol.h,
// kept in lockstep by hand across all four copies: this one, llcefproducer's own, the
// viewer's llembeddedbrowser one, and llcefshm-example's own) -- but LibVLC only, no CEF
// at all. Much simpler than SLCefProducer in one real way: libvlc needs no process-wide
// Initialize()/Shutdown() dance, no subprocess re-exec model, no native message pump --
// LibVlcTabManager's own constructor/destructor already own the one shared
// libvlc_instance_t's whole lifetime.
//
// Channel names: llvlcshm_view_0 .. llvlcshm_view_<slot_count - 1>.

#include <shmframe/llshmframe.h>
#include "cefshm_protocol.h"
#include "libvlctabmanager.h"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <windows.h> // GetModuleFileNameA, AllocConsole, WinMain
#elif defined(__APPLE__)
#include <climits>       // PATH_MAX
#include <mach-o/dyld.h> // _NSGetExecutablePath
#include <unistd.h>      // isatty
#else // Linux
#include <climits>  // PATH_MAX
#include <unistd.h> // readlink, isatty
#endif

using namespace cefshm_demo;

namespace {

volatile std::sig_atomic_t g_run = 1;
void on_signal(int) { g_run = 0; }

// Set once a console is actually attached (see show_debug_console()) --
// gates whether log_*() below emit ANSI color codes at all, so output
// stays plain if it's ever redirected somewhere colors don't make sense
// (a log file, say) rather than filling it with raw escape sequences.
bool g_console_enabled = false;

// Plain-text file for these same messages -- std::cout alone goes nowhere unless
// --console is also on (a windowless process's stdio has no console to write to by
// default), so without this, slot connects/disconnects/etc. would only ever be
// visible during a session where a console happened to be up. Opened once in
// run_producer(), right after exe_dir is known. Kept separate from libvlc's own much
// more verbose internal network/demux/decode log (see LibVlcTabManager's own
// log_file_path constructor argument) so this stays easy to skim.
std::ofstream g_log_file;

// A small, deliberately ad hoc set of colored loggers -- info/connect/
// disconnect today, more as needed later. Not a general logging
// framework; just enough structure that adding another call site is a
// one-line thing rather than reinventing formatting each time.
void log_line(const char* color, const std::string& msg)
{
    if (g_console_enabled) std::cout << color << msg << "\x1b[0m\n";
    else                    std::cout << msg << "\n";
    if (g_log_file) g_log_file << msg << std::endl;
}
void log_info(const std::string& msg)       { log_line("\x1b[38;5;103m", msg); } // blue
void log_connect(const std::string& msg)    { log_line("\x1b[38;5;120m", msg); } // green
void log_disconnect(const std::string& msg) { log_line("\x1b[38;5;221m", msg); } // amber
void log_error(const std::string& msg)      { log_line("\x1b[38;5;124m", msg); } // red
void log_priority(const std::string& msg)   { log_line("\x1b[38;5;141m", msg); } // purple -- distance/priority render-rate changes (kSetRenderRate)

// How long a slot may sit with nobody attached before its player is
// destroyed and the index freed for reuse. Deliberately longer, and a
// separate concern, from LLPublisher::command_owner_stale()'s ~2s window:
// that one is "the previous owner almost certainly crashed," this one is
// "nobody wants this right now" -- a slot whose owner crashed is reclaimed
// immediately (see the main loop) rather than waiting out this grace period.
constexpr auto kIdleGracePeriod = std::chrono::seconds(5);

// A slot this producer itself just freed can still briefly look
// "already exists" to a fresh create() for the same name: on Windows, a
// named segment only actually disappears once every process's handle to
// it is released (see LLSegment::unlink()'s own comment in llshmframe) --
// if the departing consumer's own mapping hasn't quite let go yet, this
// producer's own create()/reclaim (which already correctly sees the
// clean-shutdown marker it wrote) can still lose the race against that
// lingering handle. Retried at the kRequestSlot handler's level (one full
// pass over every free index per attempt), not per-index inside
// allocate_slot() itself -- see the sibling SLCefProducer's own identical
// comment on this same pattern for the full reasoning.
constexpr int kAllocateSlotRetries = 10;
constexpr auto kAllocateSlotRetryInterval = std::chrono::milliseconds(10);

// The default 512-byte LLConfig::max_command_bytes was sized for llshmframe's
// own demo's 3-5 byte color-name tokens; a real URL needs more headroom.
// send()/send_text() silently returns false on overflow rather than
// truncating, so this must be generous rather than exact.
constexpr std::uint32_t kMaxCommandBytes = 4096;

struct Slot
{
    std::unique_ptr<LLPublisher> pub; // null <=> this index is free
    VlcTabHandle                 vlcHandle;
    // True for a parcel/streaming-music audio slot (LLStreamingAudio_LibVLC's own IPC
    // client -- see llstreamingaudio_libvlc.cpp), false for a normal RTSP/RTMP video
    // tab. Set once at allocate_slot() time, never changes afterwards (same lifetime
    // rule as a tab's isUI/backend on the viewer side). Gates kResize (a no-op) and the
    // per-tick publish tail (always heartbeat(), never CopyLatestFrame()/publish()) --
    // see their own comments below.
    bool                          isAudioOnly = false;
    std::vector<std::uint8_t>    frameBuf; // reused across ticks -- CopyLatestFrame leaves it untouched when there's nothing new
    std::uint32_t                width  = kDefaultWidth;
    std::uint32_t                height = kDefaultHeight;
    // The actual ceiling this slot's shared-memory segment was sized to (see
    // kRequestSlot's own comment) -- may be smaller than kMaxWidth/kMaxHeight if
    // the consumer's own EmbeddedBrowserMaxWidth/Height was lower. kResize must
    // clamp against these, not the global constants, or a resize request bigger
    // than what this slot's segment actually holds would write past its end.
    std::uint32_t                max_width  = kMaxWidth;
    std::uint32_t                max_height = kMaxHeight;
    bool                          had_subscriber = false; // edge-detects a new consumer claiming this slot

    // Seeded when the slot is allocated and refreshed every tick a
    // subscriber is attached; drives kIdleGracePeriod teardown. Deliberately
    // NOT edge-based -- a slot that is allocated but never actually attached
    // to (the requesting consumer crashed, or gave up after a reply
    // timeout) has no true->false edge to time from, but does have an
    // allocation time to time from.
    std::chrono::steady_clock::time_point last_active;

    // Distance/priority-based publish throttle -- see kSetRenderRate's own
    // comment in cefshm_protocol.h. 0 = unthrottled. Unlike SLCefProducer,
    // there's nothing to "ask" libvlc to render on demand -- it decodes and
    // calls its own display callback on its own clock (see
    // libvlctabmanager.cpp); this just gates how often an already-decoded
    // frame gets drained and published, not decode itself. last_publish
    // default-constructs to the epoch, so a freshly throttled slot's very
    // first check always finds itself due rather than waiting a full
    // interval first.
    std::uint32_t                 target_fps = 0;
    std::chrono::steady_clock::time_point last_publish;
};

// How many slots are currently allocated (pub != null), out of the fixed
// total -- appended to every connect/disconnect log line so a human watching
// the console doesn't have to count colored lines by eye to see how close
// this producer is to its concurrent-instance ceiling.
std::string active_slot_suffix(const std::vector<Slot>& slots)
{
    int active = 0;
    for (const auto& s : slots)
    {
        if (s.pub) ++active;
    }
    return " (" + std::to_string(active) + "/" + std::to_string(slots.size()) + " active)";
}

// Labels for kSetRenderRate's priorityTier byte -- purely for log_priority()'s
// own output, see cefshm_protocol.h's own comment on kSetRenderRate.
std::string priority_tier_label(std::uint8_t tier)
{
    switch (tier) {
        case 1:  return "LOW";
        case 2:  return "SLIDESHOW";
        case 3:  return "HIDDEN";
        default: return "NORMAL";
    }
}

// Spins up this slot's real instance: a LibVLC tab plus its llshmframe
// segment. Leaves s untouched on failure, cleaning up whichever half of the
// pair already succeeded. No media/URL yet -- the slot exists and has a
// sized frame buffer immediately, playback only starts once kSetUrl arrives
// (see the per-slot dispatch loop).
bool allocate_slot(Slot& s, int index, LLConfig cfg, LibVlcTabManager& vlcMgr,
                    std::chrono::steady_clock::time_point now, const std::vector<Slot>& slots,
                    bool audio_only)
{
    cfg.name              = kChannelPrefix + std::to_string(index);
    cfg.max_command_bytes = kMaxCommandBytes;
    if (audio_only)
    {
        // Never publishes a frame -- same 1x1 geometry the control channel itself
        // already uses (see its own "never publishes a frame, only exchanges commands"
        // comment below), so no multi-MB shared-memory segment is ever committed for a
        // pure audio stream.
        cfg.max_width  = 1;
        cfg.max_height = 1;
    }

    LLStatus st{};
    auto pub = LLPublisher::create(cfg, &st);
    if (!pub) {
        log_error("slot " + std::to_string(index) + " (" + cfg.name + "): " + to_string(st));
        return false;
    }

    VlcTabHandle vlcHandle = audio_only
        ? vlcMgr.CreateAudioTrack()
        : vlcMgr.CreateTab(int(kDefaultWidth), int(kDefaultHeight), int(cfg.max_width), int(cfg.max_height));
    if (!vlcHandle.IsValid()) {
        log_error("slot " + std::to_string(index) + ": LibVlcTabManager::" +
                  (audio_only ? "CreateAudioTrack" : "CreateTab") + " failed");
        return false; // pub destructs here, cleanly unlinking the segment we just made
    }

    s.pub            = std::move(pub);
    s.vlcHandle      = vlcHandle;
    s.isAudioOnly    = audio_only;
    s.width          = kDefaultWidth;
    s.height         = kDefaultHeight;
    s.max_width      = cfg.max_width;
    s.max_height     = cfg.max_height;
    s.had_subscriber = false;
    s.last_active    = now;

    log_connect("slot " + std::to_string(index) + " connected" + (audio_only ? " (audio-only)" : "") +
                ", ceiling " + std::to_string(cfg.max_width) + "x" + std::to_string(cfg.max_height) +
                active_slot_suffix(slots));
    return true;
}

// Tears down this slot's real instance: the player first, then discards the
// Slot, which is what actually releases the llshmframe segment. Unlike
// SLCefProducer's own free_slot(), there's no async close handshake to wait
// out -- libvlc_media_player_stop() (inside DestroyTab()) is synchronous.
void free_slot(Slot& s, int index, LibVlcTabManager& vlcMgr,
                const std::string& reason, std::vector<Slot>& slots)
{
    vlcMgr.DestroyTab(s.vlcHandle);
    s = Slot{};
    // Logged after clearing s (which is slots[index]) so the count already
    // reflects this slot's release, matching allocate_slot()'s own log.
    log_disconnect("slot " + std::to_string(index) + " disconnected (" + reason + ")" + active_slot_suffix(slots));
}

// action == GLFW_RELEASE (0) means button-up; anything else (GLFW_PRESS=1,
// GLFW_REPEAT=2) means button-down, matching how the consumer's own input
// packing sends GLFW's raw values straight through.
bool is_mouse_up(std::uint8_t action) { return action == 0; }

#if defined(_WIN32)
// Attaches a new console to this (normally windowless) process and
// redirects stdio to it, so the existing std::cout/std::cerr diagnostics
// become visible. Opt-in only -- see run_producer()'s --console handling.
void show_debug_console()
{
    AllocConsole();
    FILE* fp = nullptr;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$",  "r", stdin);

    // A freshly allocated Windows console does not interpret ANSI escape
    // codes by default, even on a VT100-capable build of Windows -- has to
    // be turned on explicitly per console.
    //
    // Can't use GetStdHandle(STD_OUTPUT_HANDLE) here: this process is
    // launched by the Viewer via LLProcess with stdout/stderr explicitly
    // redirected to a pipe (STARTF_USESTDHANDLES), and AllocConsole() does
    // not retarget an explicitly-provided standard handle to the new
    // console -- it stays pointing at that pipe, so GetConsoleMode on it
    // fails with ERROR_ACCESS_DENIED. Open the console device by name
    // instead, same as the freopen_s calls above do for stdio.
    HANDLE out = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_EXISTING, 0, nullptr);
    DWORD mode = 0;
    if (out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode))
    {
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
    if (out != INVALID_HANDLE_VALUE) CloseHandle(out);

    g_console_enabled = true;
}
#else
// POSIX has no equivalent to a GUI-subsystem process starting with no
// console at all -- this process's stdio always goes wherever LLProcess
// wired it (a pipe, same as Windows' own default case), with no separate
// "attach a console" step needed. --console still means what it always
// meant: turn on ANSI color codes for a human actually watching this run
// interactively (see log_line()), just without the Windows-specific
// ceremony above to get there.
void show_debug_console()
{
    g_console_enabled = true;
}
#endif

#if defined(_WIN32)
std::filesystem::path get_exe_path()
{
    char buf[MAX_PATH + 1];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return std::filesystem::path(buf);
}
#elif defined(__APPLE__)
std::filesystem::path get_exe_path()
{
    char buf[PATH_MAX];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0) return {};
    // _NSGetExecutablePath itself may return a symlink (e.g. via a launcher
    // script) -- resolve it the same way GetModuleFileNameA's own result is
    // already a real, resolved path.
    std::error_code ec;
    const auto resolved = std::filesystem::canonical(buf, ec);
    return ec ? std::filesystem::path(buf) : resolved;
}
#else // Linux
std::filesystem::path get_exe_path()
{
    char buf[PATH_MAX];
    const ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return {};
    buf[n] = '\0';
    return std::filesystem::path(buf);
}
#endif

} // namespace

// The real entry point, taking real argc/argv regardless of which OS entry
// point below actually got called.
int run_producer(int argc, char** argv)
{
    int slot_count = kSlotCount;
    bool show_console = false;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--console") { show_console = true; continue; }
        slot_count = std::atoi(argv[i]);
    }
    if (slot_count <= 0) slot_count = 1;

    if (show_console) show_debug_console();

    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    const std::filesystem::path exe_dir = get_exe_path().parent_path();

    // Truncates on each launch rather than appending -- this producer runs for the
    // whole Viewer session, so one run's worth of slot connects/disconnects is already
    // plenty; nobody wants this growing unbounded across every session forever.
    g_log_file.open(exe_dir / "slvlcproducer_log.txt", std::ios::trunc);

    log_info("SLVlcProducer: starting");

#if defined(__APPLE__)
    // Windows' libvlc.dll auto-discovers its own sibling "plugins\" folder via the
    // OS's standard DLL search-path convention (the directory a DLL loaded from is
    // searched first) -- confirmed working there with no extra configuration. The
    // vendored libvlc.dylib/libvlccore.dylib we bundle for macOS makes no such
    // assumption on its own: without VLC_PLUGIN_PATH set, libvlc_new() can't locate
    // any of its plugins/*.dylib modules at all, which silently cripples it entirely
    // (no demux/access/logger module ever loads -- confirmed via real testing,
    // 2026-10-02: even a minimal, argument-safe libvlc_new() call with no custom
    // options at all still failed outright on macOS). Must be set before libvlc_new()
    // is ever called -- see LibVlcTabManager's own constructor just below. Not needed
    // on Linux: that platform links the SYSTEM's own installed libvlc (no bundled
    // copy, no plugins/ directory sitting next to this executable at all -- see
    // viewer_manifest.py's own Linux SLVlcProducer block), which already has its own
    // correct default plugin path compiled in; pointing VLC_PLUGIN_PATH at a
    // nonexistent local directory there would make things worse, not better.
    setenv("VLC_PLUGIN_PATH", (exe_dir / "plugins").string().c_str(), 1);
#endif

    // Owns the one shared libvlc_instance_t for the whole process (created in its
    // constructor, released in its destructor) -- no separate process-wide
    // Initialize()/Shutdown() dance the way CEF needs, and no subprocess re-exec model
    // either (libvlc has no equivalent concept). Its own log file, separate from
    // slvlcproducer_log.txt above, captures libvlc's own internal network/demux/decode
    // diagnostics -- the actual detail behind "why didn't this play," which nothing
    // else here surfaces.
    LibVlcTabManager vlcMgr((exe_dir / "libvlc_log.txt").string());
    if (vlcMgr.IsReady())
    {
        log_info("SLVlcProducer: libvlc ready");
    }
    else
    {
        // Every CreateTab()/CreateAudioTrack() call will keep silently failing for this
        // process's entire lifetime if this prints -- no RTSP/RTMP prim media and no
        // parcel audio will ever play, regardless of what the rest of this banner says.
        // See LibVlcTabManager's own constructor comment for a real case this caught.
        log_error("SLVlcProducer: libvlc_new() failed -- no LibVLC media of any kind will work this session");
    }

    LLConfig view_cfg; // template for whichever index gets allocated on demand
    view_cfg.max_width  = kMaxWidth;
    view_cfg.max_height = kMaxHeight;
    const std::uint64_t worst_case_bytes = segment_bytes(view_cfg) * std::uint64_t(slot_count);

    std::vector<Slot> slots(static_cast<std::size_t>(slot_count)); // all start unallocated (pub == nullptr)

    LLConfig control_cfg;
    control_cfg.name       = kControlChannelName;
    control_cfg.max_width  = 1; // never publishes a frame, only exchanges commands
    control_cfg.max_height = 1;

    LLStatus st{};
    auto control = LLPublisher::create(control_cfg, &st);
    if (!control) {
        std::cerr << "control channel (" << control_cfg.name << "): " << to_string(st) << "\n";
        return 1;
    }

    {
        std::ostringstream banner;
        banner << "SLVlcProducer: control channel ready, up to " << slot_count
               << " concurrent view(s) (" << kChannelPrefix << "0.." << (slot_count - 1) << "), "
               << (worst_case_bytes / (1024 * 1024)) << " MiB ceiling if all " << slot_count
               << " were active at once at " << kMaxWidth << "x" << kMaxHeight << " each -- "
               << "0 committed until requested";
        log_info(banner.str());
    }

    LLShmCommand cmd;

    while (g_run)
    {
        const auto now = std::chrono::steady_clock::now();

        // Service slot requests first so a freshly-allocated slot gets a
        // chance to publish within this same tick.
        while (control->receive(cmd))
        {
            if (cmd.type == kShutdownProducer) { g_run = 0; continue; }
            if (cmd.type != kRequestSlot) continue;

            // Defaults match the old, isUI-only payload's fallback (see
            // unpack_request_slot's own comment) -- untouched if the consumer sent
            // a short/old-format payload, or clamp-adjusted below if it sent a real
            // ceiling. Clamped to this producer's own absolute maximum (never more)
            // and its default player size (never less -- CreateTab() below always
            // starts a fresh tab at kDefaultWidth/kDefaultHeight regardless of what
            // was requested, so a segment smaller than that would never fit even the
            // first frame). isUI and the trailing backend byte are both unpacked
            // (the wire format is shared with SLCefProducer) but unused here -- isUI
            // only ever selected between CEF's two CefRequestContexts, which libvlc
            // has no equivalent of at all; backend is vestigial since the 2026-09-30
            // producer split (see kRequestSlot's own comment in cefshm_protocol.h).
            bool isUI = true;
            std::uint32_t requested_max_width = kMaxWidth;
            std::uint32_t requested_max_height = kMaxHeight;
            std::uint8_t backend_byte = 0;
            bool audio_only = false;
            unpack_request_slot(cmd.data.data(), cmd.data.size(), isUI, requested_max_width,
                                 requested_max_height, backend_byte, audio_only);
            // An audio-only request's max_width/max_height are irrelevant -- allocate_slot()
            // always overrides them to 1x1 for that case -- so skip the clamp entirely rather
            // than clamping numbers that are about to be thrown away anyway.
            LLConfig slot_cfg = view_cfg;
            if (!audio_only)
            {
                slot_cfg.max_width  = std::clamp(requested_max_width,  kDefaultWidth,  kMaxWidth);
                slot_cfg.max_height = std::clamp(requested_max_height, kDefaultHeight, kMaxHeight);
            }

            // Try every currently-free index, not just the lowest one -- see the
            // sibling SLCefProducer's own identical comment on this same pattern for
            // the full reasoning.
            int free_index = -1;
            for (int outer = 0; outer < kAllocateSlotRetries && free_index < 0; ++outer)
            {
                bool any_free = false;
                for (int i = 0; i < slot_count; ++i)
                {
                    if (slots[std::size_t(i)].pub) continue; // not free
                    any_free = true;
                    if (allocate_slot(slots[std::size_t(i)], i, slot_cfg, vlcMgr, now, slots, audio_only))
                    {
                        free_index = i;
                        break;
                    }
                }
                if (free_index < 0)
                {
                    if (!any_free) break; // no free index at all right now -- retrying won't help
                    std::this_thread::sleep_for(kAllocateSlotRetryInterval);
                }
            }

            if (free_index < 0)
            {
                control->send(kSlotUnavailable, nullptr, 0, cmd.id);
                continue;
            }

            // Reply only now that the segment and the player both demonstrably
            // exist -- see the sibling SLCefProducer's own identical comment on the
            // memory-ordering reasoning for why this is safe without an explicit
            // fence.
            std::uint8_t payload[4];
            pack_u32(payload, std::uint32_t(free_index));
            control->send(kSlotAssigned, payload, 4, cmd.id);
        }

        for (std::size_t i = 0; i < slots.size(); ++i)
        {
            Slot& s = slots[i];
            if (!s.pub) continue;

            const bool has_sub = s.pub->has_subscriber();

            if (has_sub && s.pub->command_owner_stale())
            {
                // Almost certainly a crashed consumer, not a merely-idle
                // one: reclaim now rather than waiting out the softer idle
                // grace period below.
                free_slot(s, int(i), vlcMgr, "crashed consumer", slots);
                continue;
            }

            if (has_sub)
            {
                s.had_subscriber = true;
                s.last_active    = now;
            }
            else
            {
                if (s.had_subscriber)
                {
                    // Edge-triggered, logged the moment the viewer's own subscriber
                    // cleanly detaches -- separate from (and well before) the actual
                    // teardown below, which deliberately waits out kIdleGracePeriod
                    // in case the same consumer reconnects shortly.
                    log_disconnect("slot " + std::to_string(i) + " viewer detached (grace period running)" + active_slot_suffix(slots));
                    s.had_subscriber = false;
                }

                if (now - s.last_active >= kIdleGracePeriod)
                {
                    free_slot(s, int(i), vlcMgr, "idle timeout", slots);
                    continue;
                }
            }

            while (s.pub->receive(cmd))
            {
                if (cmd.type == kSetRenderRate)
                {
                    // Same opcode SLCefProducer also honours for its own slots (see
                    // cefshm_protocol.h) -- just sets Slot's own target_fps field here,
                    // read back by the publish tail below.
                    std::uint32_t fps;
                    std::uint8_t tier;
                    std::string url;
                    if (unpack_render_rate(cmd.data.data(), cmd.data.size(), fps, tier, url)) {
                        s.target_fps = fps;
                        log_priority("slot " + std::to_string(i) + " render rate: " + priority_tier_label(tier) +
                                     " (" + (fps == 0 ? std::string("unthrottled") : (std::to_string(fps) + "fps")) +
                                     ") - " + url + active_slot_suffix(slots));
                    }
                    continue;
                }

                switch (cmd.type)
                {
                case kSetUrl: {
                    const std::string url(cmd.text());
                    log_connect("slot " + std::to_string(i) + " -> " + url + active_slot_suffix(slots));
                    vlcMgr.Open(s.vlcHandle, url);
                    break;
                }
                case kResize: {
                    if (s.isAudioOnly) break; // no video geometry -- never sent by the audio client, defensive no-op if it ever is
                    std::uint32_t w, h;
                    if (unpack_size(cmd.data.data(), cmd.data.size(), w, h) && w && h) {
                        // s.max_width/max_height, not kMaxWidth/kMaxHeight -- this slot's own
                        // segment may have been sized smaller (see kRequestSlot's own comment);
                        // clamping against the global constants here would let a resize request
                        // write past the end of what was actually allocated for it.
                        w = std::min(w, s.max_width);
                        h = std::min(h, s.max_height);
                        if (w != s.width || h != s.height) {
                            s.width  = w;
                            s.height = h;
                            vlcMgr.Resize(s.vlcHandle, int(w), int(h));
                        }
                    }
                    break;
                }
                case kSetVolume: {
                    if (!cmd.data.empty()) vlcMgr.SetVolume(s.vlcHandle, int(cmd.data[0]));
                    break;
                }
                case kSetMuted: {
                    // Maps to volume 0/100 -- see kSetMuted's own comment in
                    // cefshm_protocol.h on why this is honoured here too.
                    if (!cmd.data.empty()) vlcMgr.SetVolume(s.vlcHandle, cmd.data[0] != 0 ? 0 : 100);
                    break;
                }
                case kMouseButton: {
                    // Click-to-pause/resume: the consumer sends this uniformly regardless of
                    // backend (see LLViewerMediaImpl::mouseDown/mouseUp), so this just picks
                    // out the one gesture that's meaningful here -- a completed left-button
                    // click -- and ignores the rest (move/drag, other buttons).
                    std::int32_t x, y; std::uint8_t button, action, click_count;
                    if (unpack_mouse_button(cmd.data.data(), cmd.data.size(), x, y, button, action, click_count) &&
                        button == 0 && is_mouse_up(action))
                    {
                        vlcMgr.TogglePlayPause(s.vlcHandle);
                    }
                    break;
                }
                case kSetPlaybackAction: {
                    // Explicit Play/Pause/Stop from LLPanelPrimMediaControls' own buttons --
                    // see kSetPlaybackAction's own comment in cefshm_protocol.h for why this
                    // is separate from kMouseButton's click-to-pause toggle above.
                    if (!cmd.data.empty()) {
                        switch (cmd.data[0]) {
                        case 0: vlcMgr.Play(s.vlcHandle); break;
                        case 1: vlcMgr.Pause(s.vlcHandle); break;
                        case 2: vlcMgr.Stop(s.vlcHandle); break;
                        default: break;
                        }
                    }
                    break;
                }
                default:
                    // No LibVLC equivalent: kExecuteJavaScript, kMouseMove, kScrollWheel,
                    // kKeyEvent, kSetFocus, kCut/Copy/Paste, kGoBack/Forward, kStopLoad,
                    // kReload, kSetPageZoom, kFileDialogResponse -- silent no-op, not an
                    // error, since a caller may send these uniformly regardless of backend.
                    break;
                }
            }

            // No SendExternalBeginFrame equivalent -- nothing to "ask" libvlc to render
            // on demand, it decodes and calls its own display callback on its own clock
            // (see libvlctabmanager.cpp). The throttle point is "how often do we bother
            // draining and publishing whatever's already been decoded" -- decode itself
            // is unthrottled on libvlc's own thread either way, an explicit, accepted
            // tradeoff (CPU cost of decoding isn't reduced, only shm-publish/network cost
            // is). A throttled slot naturally drops undelivered intermediate frames
            // rather than queuing them, since the display callback just overwrites the
            // one dirty-flag/buffer.
            if (s.isAudioOnly) {
                // Never publishes a frame -- same pattern the control channel itself
                // already uses (see its own 1x1 config above) -- just keep the
                // heartbeat alive so this slot isn't mistaken for idle/crashed.
                s.pub->heartbeat();
            } else {
                const bool publish_due = (s.target_fps == 0) ||
                    (now - s.last_publish >= std::chrono::duration<double>(1.0 / s.target_fps));
                if (publish_due) {
                    s.last_publish = now;
                    int fw = 0, fh = 0;
                    if (vlcMgr.CopyLatestFrame(s.vlcHandle, s.frameBuf, fw, fh)) {
                        s.pub->publish(s.frameBuf.data(), std::uint32_t(fw), std::uint32_t(fh));
                    } else {
                        s.pub->heartbeat();
                    }
                } else {
                    s.pub->heartbeat();
                }
            }

            // Mirrors kEventLoadStart/kEventLoadEnd's own CEF call sites in the sibling
            // SLCefProducer -- both opcodes already exist and already work, reused as-is
            // here rather than inventing new wire surface. Drained once per tick rather
            // than pushed synchronously from libvlc's own event-manager thread -- see
            // libvlctabmanager.h's own comment on why.
            if (vlcMgr.ConsumeLoadStart(s.vlcHandle)) {
                s.pub->send(kEventLoadStart);
            }
            int load_end_status = 0;
            if (vlcMgr.ConsumeLoadEnd(s.vlcHandle, load_end_status)) {
                std::uint8_t payload[4];
                pack_u32(payload, std::uint32_t(load_end_status));
                s.pub->send(kEventLoadEnd, payload, 4);
            }
            bool now_playing = false;
            if (vlcMgr.ConsumePlaybackStateChange(s.vlcHandle, now_playing)) {
                std::uint8_t payload[1] = { now_playing ? std::uint8_t(1) : std::uint8_t(0) };
                s.pub->send(kEventPlaybackStateChanged, payload, 1);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    log_info("SLVlcProducer: shutting down");
    vlcMgr.DestroyAll();
    return 0;
}

#if defined(_WIN32)
// Windowless by default (this process is launched/killed automatically by the Viewer
// every session), matching the sibling SLCefProducer's own identical convention.
// WinMain's own lpCmdLine is a single unsplit ANSI string missing argv[0], so instead
// forward the MSVC CRT's __argc/__argv (declared by <cstdlib>, already included above)
// -- these are populated identically to what a console main() would receive, regardless
// of which entry point the linker actually used, since they come from the same CRT
// startup code either way.
int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return run_producer(__argc, __argv);
}
#else
int main(int argc, char** argv)
{
    return run_producer(argc, argv);
}
#endif

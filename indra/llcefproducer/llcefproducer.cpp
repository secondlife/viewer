/**
 *
 * @file llcefproducer.cpp
 * @brief SLCefProducer: hosts real CEF browser instances on demand for the viewer's own
 *        llembeddedbrowser consumer, over llshmframe. CEF only -- see indra/llvlcproducer/
 *        llvlcproducer.cpp for the separate LibVLC-only producer this was split from
 *        2026-09-30, for licensing reasons (the vendored libvlc package is GPL v2, this
 *        project is LGPL v2.1).
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

// llcefproducer.cpp
//
// Adapted directly from llcefshm-example's own src/cefshm_producer.cpp --
// same control-channel + on-demand-allocate + idle-teardown skeleton, same
// wire protocol (cefshm_protocol.h, kept in lockstep by hand across all
// four copies: this one, the viewer's llembeddedbrowser one, the separate
// LibVLC-only llvlcproducer's own, and llcefshm-example's own). What's
// different here is the entry point (this process is launched/monitored/
// killed by the Viewer itself via LLProcess, not double-clicked by hand)
// and the optional runtime debug console -- everything else, including the
// main loop's cadence and shutdown sequence, is unchanged from the
// reference implementation.
//
// Channel names: llcefshm_view_0 .. llcefshm_view_<slot_count - 1>.

#include <shmframe/llshmframe.h>
#include <llCefBrowserJavaScriptBridge.h>
#include <llCefBrowserLib.h>
#include <llCefBrowserManager.h>
#include <llCefBrowserVersion.h>
#include "cefshm_protocol.h"

#include <algorithm>
#include <chrono>
#include <cmath>
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

// --audio-test-tone: grant audio capture (see kSlotFlagAudioCapture) to any slot that asks
// for it, and fill its audio channel with a generated stereo tone instead of real CEF
// audio. Scaffolding for bringing up the Viewer's OpenAL playback path ahead of
// llCefBrowser's own audio-capture interface -- off by default, so until that interface is
// wired in, capture is always declined and CEF keeps playing its own audio as before.
bool g_audio_test_tone = false;

// Set once a console is actually attached (see show_debug_console()) --
// gates whether log_*() below emit ANSI color codes at all, so output
// stays plain if it's ever redirected somewhere colors don't make sense
// (a log file, say) rather than filling it with raw escape sequences.
bool g_console_enabled = false;

// Plain-text file for these same messages -- std::cout alone goes nowhere unless
// --console is also on (a windowless process's stdio has no console to write to by
// default), so without this, slot connects/disconnects/etc. would only ever be
// visible during a session where a console happened to be up. Opened once in
// run_producer(), right after exe_dir is known. Kept separate from CEF's own much
// larger, much more verbose cefshm_producer_log.txt so this stays easy to skim.
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

// How long a slot may sit with nobody attached before its browser is
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
// allocate_slot() itself: a per-index retry loop pays its own wait cost
// for every stuck index encountered along the way even when some other
// index is actually free right now, which can itself add up past a
// caller's own patience (e.g. LLViewerMediaImpl's disconnect-alert grace
// period) when more than one index happens to be stuck at once.
constexpr int kAllocateSlotRetries = 10;
constexpr auto kAllocateSlotRetryInterval = std::chrono::milliseconds(10);

// The default 512-byte LLConfig::max_command_bytes was sized for llshmframe's
// own demo's 3-5 byte color-name tokens; a real URL needs more headroom.
// send()/send_text() silently returns false on overflow rather than
// truncating, so this must be generous rather than exact.
constexpr std::uint32_t kMaxCommandBytes = 4096;

// The audio format every kAudioPacket from this producer carries -- 10ms packets of 48kHz
// stereo float32, matching what the Viewer's OpenAL path is tuned for. The format is
// self-describing on the wire (see kAudioPacket), so this is a producer-side choice, not a
// protocol constant.
constexpr std::uint32_t kAudioSampleRate      = 48000;
constexpr std::uint8_t  kAudioChannels        = 2;
constexpr std::uint16_t kAudioFramesPerPacket = 480;

// If the main loop stalls for longer than this, the test tone skips ahead (leaving a gap
// in seq) rather than bursting the whole backlog into the audio ring at once.
constexpr std::uint64_t kAudioMaxCatchUpFrames = std::uint64_t(kAudioFramesPerPacket) * 10;

struct Slot
{
    std::unique_ptr<LLPublisher> pub; // null <=> this index is free
    llCefBrowserHandle           cefHandle;
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

    // Distance/priority-based render throttle -- see kSetRenderRate's own
    // comment in cefshm_protocol.h. 0 = unthrottled: SendExternalBeginFrame()
    // fires every tick, same as before this existed. last_begin_frame default-
    // constructs to the epoch, so a freshly throttled slot's very first check
    // always finds itself due rather than waiting a full interval first.
    std::uint32_t                 target_fps = 0;
    std::chrono::steady_clock::time_point last_begin_frame;

    // This slot's audio channel (see kAudioChannelPrefix) -- null unless the consumer asked
    // for audio capture and this producer granted it. Only ever sent on from the main loop,
    // which keeps its command ring single-writer as llshmframe requires.
    std::unique_ptr<LLPublisher> audioPub;
    std::vector<std::uint8_t>    audioBuf;      // one packed kAudioPacket, reused across sends
    std::vector<float>           audioSamples;  // interleaved, reused across sends
    std::uint32_t                audio_seq = 0;
    std::uint64_t                audio_frames = 0;  // frames generated since audio_start
    std::uint64_t                audio_dropped = 0; // packets lost to a full ring or a stall
    bool                         audio_streaming = false;
    std::chrono::steady_clock::time_point audio_start;
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

// Bridges window.cefQuery() calls (from any CEF-backed slot's page JS) back to the
// consumer over kEventJSQuery -- see that opcode's own comment in cefshm_protocol.h.
// llCefBrowserLib::SetJavaScriptBridge() registers exactly one of these, process-wide
// (there's no per-handle registration for the full bridge, unlike the simpler
// observation-only SetOnQueryCallback()), so OnQuery() has to find which slot a given
// query actually arrived on itself, via its own `handle` parameter.
class JsBridge : public llCefBrowserJavaScriptBridge
{
public:
    explicit JsBridge(std::vector<Slot>& slots) : mSlots(slots) {}

    bool OnQuery(llCefBrowserHandle handle, std::int64_t queryId, const std::string& request, bool persistent) override
    {
        Slot* slot = find_slot(handle);
        // No connected consumer to forward this to -- return false so CEF reports
        // failure straight back to the page immediately, rather than leaving its
        // onSuccess/onFailure hanging forever waiting for a response that can never
        // arrive.
        if (!slot || !slot->pub) return false;

        std::vector<std::uint8_t> payload(9 + request.size());
        const std::uint32_t n = pack_js_query(payload.data(), queryId, persistent, request);
        slot->pub->send(kEventJSQuery, payload.data(), n);
        return true;
    }

private:
    // Linear scan: this fires rarely compared to painting/input, and slot_count tops
    // out in the dozens, so there's no need for a real handle->slot index.
    Slot* find_slot(llCefBrowserHandle handle)
    {
        for (auto& s : mSlots)
        {
            if (s.pub && s.cefHandle == handle) return &s;
        }
        return nullptr;
    }

    std::vector<Slot>& mSlots;
};

// Spins up this slot's real instance: a CEF browser plus its llshmframe
// segment. Leaves s untouched on failure, cleaning up whichever half of the
// pair already succeeded.
// Creates this slot's audio channel (see kAudioChannelPrefix). Returns null on failure --
// the caller then just declines audio capture for this slot rather than failing it outright.
std::unique_ptr<LLPublisher> create_audio_publisher(int index)
{
    LLConfig cfg;
    cfg.name              = kAudioChannelPrefix + std::to_string(index);
    cfg.max_width         = 1; // never publishes a frame, only exchanges commands
    cfg.max_height        = 1;
    cfg.command_slots     = kAudioCommandSlots;
    cfg.max_command_bytes = kAudioMaxCommandBytes;

    LLStatus st{};
    auto pub = LLPublisher::create(cfg, &st);
    if (!pub) {
        log_error("slot " + std::to_string(index) + " (" + cfg.name + "): " + to_string(st) +
                  " -- declining audio capture");
    }
    return pub;
}

// Test-tone source for --audio-test-tone (see g_audio_test_tone). Called once per main-loop
// tick: paced against the wall clock from the moment a subscriber attaches, sending however
// many whole 10ms packets have come due since the last call. A distinct pitch per slot (and
// a fifth higher on the right channel) makes several concurrent slots, and a swapped
// left/right, easy to tell apart by ear.
void pump_test_tone(Slot& s, int index, std::chrono::steady_clock::time_point now)
{
    s.audioPub->heartbeat(); // never commits a frame, so this is its only liveness signal

    if (!s.audioPub->has_subscriber()) {
        s.audio_streaming = false; // restart the clock cleanly when someone attaches again
        return;
    }
    if (!s.audio_streaming) {
        s.audio_streaming = true;
        s.audio_start     = now;
        s.audio_frames    = 0;
    }

    const std::uint64_t elapsed_us = std::uint64_t(
        std::chrono::duration_cast<std::chrono::microseconds>(now - s.audio_start).count());
    const std::uint64_t due_frames = elapsed_us * kAudioSampleRate / 1000000;

    if (due_frames > s.audio_frames + kAudioMaxCatchUpFrames) {
        const std::uint64_t skipped = (due_frames - s.audio_frames) / kAudioFramesPerPacket;
        s.audio_frames  += skipped * kAudioFramesPerPacket;
        s.audio_seq     += std::uint32_t(skipped);
        s.audio_dropped += skipped;
    }

    // Integral frequencies, so phase repeats exactly every kAudioSampleRate frames and
    // (frame % kAudioSampleRate) keeps the sin() argument small however long this runs.
    const int left_hz  = 440 + 110 * (index % 4);
    const int right_hz = left_hz * 3 / 2;
    constexpr double kTwoPi     = 6.283185307179586;
    constexpr float  kAmplitude = 0.2f;

    s.audioSamples.resize(std::size_t(kAudioFramesPerPacket) * kAudioChannels);
    s.audioBuf.resize(kAudioPacketHeaderBytes + s.audioSamples.size() * sizeof(float));

    while (s.audio_frames + kAudioFramesPerPacket <= due_frames) {
        for (std::uint32_t f = 0; f < kAudioFramesPerPacket; ++f) {
            const double t = double((s.audio_frames + f) % kAudioSampleRate) / kAudioSampleRate;
            s.audioSamples[f * 2 + 0] = kAmplitude * float(std::sin(kTwoPi * left_hz * t));
            s.audioSamples[f * 2 + 1] = kAmplitude * float(std::sin(kTwoPi * right_hz * t));
        }
        const std::int64_t pts_us = std::int64_t(s.audio_frames * 1000000 / kAudioSampleRate);
        const std::uint32_t n = pack_audio_packet(s.audioBuf.data(), s.audio_seq, pts_us, kAudioSampleRate,
                                                  kAudioChannels, kAudioFramesPerPacket, s.audioSamples.data());
        if (!s.audioPub->send(kAudioPacket, s.audioBuf.data(), n)) {
            ++s.audio_dropped; // ring full -- the consumer is behind; seq still advances
        }
        ++s.audio_seq;
        s.audio_frames += kAudioFramesPerPacket;
    }
}

bool allocate_slot(Slot& s, int index, LLConfig cfg, llCefBrowserManager& manager,
                    std::chrono::steady_clock::time_point now,
                    bool isUI, bool audioCapture, const std::vector<Slot>& slots)
{
    cfg.name              = kChannelPrefix + std::to_string(index);
    cfg.max_command_bytes = kMaxCommandBytes;

    LLStatus st{};
    auto pub = LLPublisher::create(cfg, &st);
    if (!pub) {
        log_error("slot " + std::to_string(index) + " (" + cfg.name + "): " + to_string(st));
        return false;
    }

    llCefBrowserHandle handle = manager.CreateBrowser("about:blank", int(kDefaultWidth), int(kDefaultHeight), isUI);
    if (!handle.IsValid()) {
        log_error("slot " + std::to_string(index) + ": CreateBrowser failed");
        return false; // pub destructs here, cleanly unlinking the segment we just made
    }

    s.pub            = std::move(pub);
    s.cefHandle      = handle;
    s.width          = kDefaultWidth;
    s.height         = kDefaultHeight;
    s.max_width      = cfg.max_width;
    s.max_height     = cfg.max_height;
    s.had_subscriber = false;
    s.last_active    = now;

    // Only granted under --audio-test-tone for now -- see g_audio_test_tone.
    if (audioCapture && g_audio_test_tone) {
        s.audioPub = create_audio_publisher(index);
    }

    log_connect("slot " + std::to_string(index) + " connected, ceiling " + std::to_string(cfg.max_width) +
                "x" + std::to_string(cfg.max_height) + (s.audioPub ? ", audio capture (test tone)" : "") +
                active_slot_suffix(slots));

    // One-shot, sent before any frames: lets the consumer show which
    // llCefBrowser/CEF/Chromium build is actually in play without needing to
    // link llcefbrowser itself just to read its version header. Formatted to
    // match how the Viewer's own About-box embedded-browser block reads
    // (see llappviewer.cpp).
    {
        const std::string version_str =
            std::to_string(LLCEFBROWSER_VERSION_MAJOR) + "." + std::to_string(LLCEFBROWSER_VERSION_MINOR) +
            " (" + LLCEFBROWSER_VERSION_GITHASH + ")\n"
            "  CEF: " + std::to_string(CEF_VERSION_MAJOR) + "." + std::to_string(CEF_VERSION_MINOR) + "." +
            std::to_string(CEF_VERSION_PATCH) + "\n"
            "  Chromium: " + std::to_string(CHROME_VERSION_MAJOR) + "." + std::to_string(CHROME_VERSION_MINOR) + "." +
            std::to_string(CHROME_VERSION_BUILD) + "." + std::to_string(CHROME_VERSION_PATCH);
        s.pub->send_text(kEventVersionInfo, version_str);
    }

    // Forward a subset of llCefBrowserManager's own event callbacks to this
    // slot's consumer as kEvent* commands. `slot` is a stable pointer into
    // the (never-resized) slots vector in run_producer() -- it outlives any
    // async callback still in flight during teardown -- but s.pub itself
    // goes null the instant free_slot() runs, so every callback re-checks it
    // rather than capturing the LLPublisher directly.
    Slot* slot = &s;
    auto send_nav_state = [slot, &manager, handle]() {
        if (slot->pub) {
            std::uint8_t payload[2] = { manager.CanGoBack(handle)    ? std::uint8_t(1) : std::uint8_t(0),
                                         manager.CanGoForward(handle) ? std::uint8_t(1) : std::uint8_t(0) };
            slot->pub->send(kEventNavStateChanged, payload, 2);
        }
    };
    manager.SetOnLoadStartCallback(handle, [slot, send_nav_state]() {
        if (slot->pub) slot->pub->send(kEventLoadStart);
        send_nav_state();
    });
    manager.SetOnLoadEndCallback(handle, [slot, send_nav_state](int httpStatusCode) {
        if (slot->pub) {
            std::uint8_t payload[4];
            pack_u32(payload, std::uint32_t(httpStatusCode));
            slot->pub->send(kEventLoadEnd, payload, 4);
        }
        send_nav_state();
    });
    manager.SetOnTitleChangeCallback(handle, [slot](const std::string& title) {
        if (slot->pub) slot->pub->send_text(kEventTitleChanged, title);
    });
    manager.SetOnAddressChangeCallback(handle, [slot](const std::string& url) {
        if (slot->pub) slot->pub->send_text(kEventAddressChanged, url);
    });
    manager.SetOnStatusMessageCallback(handle, [slot](const std::string& value) {
        if (slot->pub) slot->pub->send_text(kEventStatusTextChanged, value);
    });
    manager.SetOnConsoleMessageCallback(handle, [slot](const std::string& message, const std::string& source, int line) {
        if (slot->pub) {
            std::vector<std::uint8_t> payload(8 + message.size() + source.size());
            const std::uint32_t n = pack_console_message(payload.data(), message, source, line);
            slot->pub->send(kEventConsoleMessage, payload.data(), n);
        }
    });
    manager.SetOnCursorChangedCallback(handle, [slot](llCefCursorType cursorType) {
        if (slot->pub) {
            std::uint8_t payload[4];
            pack_u32(payload, static_cast<std::uint32_t>(cursorType));
            slot->pub->send(kEventCursorChanged, payload, 4);
        }
    });
    manager.SetOnOpenPopupCallback(handle, [slot](const std::string& targetUrl, const std::string& targetFrameName) {
        if (slot->pub) {
            std::vector<std::uint8_t> payload(4 + targetUrl.size() + targetFrameName.size());
            const std::uint32_t n = pack_click_href(payload.data(), targetUrl, targetFrameName);
            slot->pub->send(kEventClickLinkHref, payload.data(), n);
        }
    });
    manager.SetOnCustomSchemeURLCallback(handle, [slot](const std::string& url, bool userGesture, bool isRedirect) {
        if (slot->pub) {
            std::vector<std::uint8_t> payload(1 + url.size());
            const std::uint32_t n = pack_click_nofollow(payload.data(), url, userGesture, isRedirect);
            slot->pub->send(kEventClickLinkNoFollow, payload.data(), n);
        }
    });
    manager.SetOnLoadErrorCallback(handle, [slot](int errorCode, const std::string& /*errorText*/, const std::string& failedUrl) {
        if (slot->pub) {
            std::vector<std::uint8_t> payload(4 + failedUrl.size());
            const std::uint32_t n = pack_load_error(payload.data(), std::uint32_t(errorCode), failedUrl);
            slot->pub->send(kEventLoadError, payload.data(), n);
        }
    });
    manager.SetOnFileDialogCallback(handle, [slot](std::int64_t dialogId, llCefFileDialogMode mode,
                                                    const std::string& /*title*/, const std::string& defaultFilePath,
                                                    const std::vector<std::string>& /*acceptFilters*/) {
        if (slot->pub) {
            std::vector<std::uint8_t> payload(12 + defaultFilePath.size());
            const std::uint32_t n = pack_file_dialog_request(payload.data(), dialogId,
                                                              static_cast<std::uint32_t>(mode), defaultFilePath);
            slot->pub->send(kEventFileDialogRequest, payload.data(), n);
        }
    });

    return true;
}

// Tears down this slot's real instance: the CEF browser first (DestroyBrowser
// is async -- RequestClose() now, actual close completes on a later
// DoMessageLoopWork() pump -- but safe to call and move on from immediately),
// then discards the Slot, which is what actually releases the llshmframe
// segment. Not the other order: clearing the Slot first would lose the
// handle needed to destroy the browser at all.
void free_slot(Slot& s, int index, llCefBrowserManager& manager,
                const std::string& reason, std::vector<Slot>& slots)
{
    manager.DestroyBrowser(s.cefHandle);
    const std::string audio_stats = s.audioPub
        ? ", " + std::to_string(s.audio_seq) + " audio packets, " + std::to_string(s.audio_dropped) + " dropped"
        : std::string();
    s = Slot{};
    // Logged after clearing s (which is slots[index]) so the count already
    // reflects this slot's release, matching allocate_slot()'s own log.
    log_disconnect("slot " + std::to_string(index) + " disconnected (" + reason + audio_stats + ")" +
                   active_slot_suffix(slots));
}

// action == GLFW_RELEASE (0) means button-up; anything else (GLFW_PRESS=1,
// GLFW_REPEAT=2) means button-down, matching how the consumer's own input
// packing sends GLFW's raw values straight through.
bool cef_mouse_up(std::uint8_t action) { return action == 0; }

// GLFW's own numbering: GLFW_MOUSE_BUTTON_LEFT=0, RIGHT=1, MIDDLE=2.
bool map_mouse_button(std::uint8_t glfw_button, llCefMouseButton& out)
{
    switch (glfw_button) {
        case 0: out = llCefMouseButton::Left;   return true;
        case 1: out = llCefMouseButton::Right;  return true;
        case 2: out = llCefMouseButton::Middle; return true;
        default: return false;
    }
}

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
// point below actually got called -- llCefBrowserLib::ExecuteSubProcess()
// needs them to recognize CEF's own re-exec'd helper (renderer/GPU/utility)
// subprocesses.
int run_producer(int argc, char** argv)
{
    // Must be first, before ExecuteSubProcess() or anything else in
    // llCefBrowserLib -- a no-op on Windows/Linux, but mandatory on macOS
    // (see LoadCefLibrary()'s own comment). Missed here originally because this
    // file had never actually been run on macOS until now; the same fix was
    // already in place for llcefbrowser's own example apps and for
    // llCefBrowserHost.cpp (the macOS helper sub-process entry point).
    if (! llCefBrowserLib::LoadCefLibrary())
    {
        return 1;
    }

    int slot_count = kSlotCount;
    bool show_console = false;
    std::string cache_dir_arg;
    int remote_debugging_port = 0;
    const std::string kCacheDirPrefix = "--cache-dir=";
    const std::string kRemoteDebuggingPortPrefix = "--remote-debugging-port=";
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--console") { show_console = true; continue; }
        if (arg == "--audio-test-tone") { g_audio_test_tone = true; continue; }
        if (arg.rfind(kCacheDirPrefix, 0) == 0) { cache_dir_arg = arg.substr(kCacheDirPrefix.size()); continue; }
        if (arg.rfind(kRemoteDebuggingPortPrefix, 0) == 0) { remote_debugging_port = std::atoi(arg.c_str() + kRemoteDebuggingPortPrefix.size()); continue; }
        slot_count = std::atoi(argv[i]);
    }
    if (slot_count <= 0) slot_count = 1;

    // Must be first: CEF re-execs this same binary for its helper
    // (renderer/GPU/utility) subprocesses, distinguished by command-line
    // flags this call recognizes. A non-negative return means this process
    // IS one of those helpers -- return immediately, do not fall through to
    // any of the producer logic below (including the console/signal setup
    // just below, which a helper subprocess has no use for).
    const int subprocess_exit = llCefBrowserLib::ExecuteSubProcess(argc, argv);
    if (subprocess_exit >= 0) return subprocess_exit;

    if (show_console) show_debug_console();

    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    const std::filesystem::path exe_dir = get_exe_path().parent_path();

    // Truncates on each launch rather than appending -- this producer runs for the
    // whole Viewer session, so one run's worth of slot connects/disconnects is already
    // plenty; nobody wants this growing unbounded across every session forever.
    g_log_file.open(exe_dir / "slcefproducer_log.txt", std::ios::trunc);

    // The Viewer passes --cache-dir (see LLEmbeddedBrowser::launchProducer()) pointing
    // at the same per-user cache location the legacy CEF plugin uses, since this
    // process has no gDirUtilp of its own to compute that itself. Falls back to the
    // old exe-relative location only for a manual/dev launch with no such argument.
    const std::filesystem::path cache_dir = cache_dir_arg.empty() ? (exe_dir / "cef_profile")
                                                                   : std::filesystem::path(cache_dir_arg);

    llCefBrowserLibInitOptions init_options;
    init_options.rootCachePath       = cache_dir.string();
    init_options.logFile             = (exe_dir / "cefshm_producer_log.txt").string();
    init_options.userAgentProduct    = "SLCefProducer/1.0";
    init_options.remoteDebuggingPort = remote_debugging_port;
#if defined(__APPLE__)
    // SLCefProducer isn't a real .app bundle (see viewer_manifest.py's own
    // comment on this) -- CEF's bundle-relative auto-detection for
    // resources/framework/sub-process path doesn't apply, so these are set
    // explicitly, same as llcefbrowser's own example apps do. Deploy layout
    // (see Darwin_x86_64_Manifest.package_finish() in viewer_manifest.py):
    //   Contents/Resources/SLCefProducer/SLCefProducer   (exe_dir, this file)
    //   Contents/Resources/Frameworks/Chromium Embedded Framework.framework
    const std::filesystem::path framework_dir =
        exe_dir.parent_path() / "Frameworks" / "Chromium Embedded Framework.framework";
    init_options.frameworkDirPath      = framework_dir.string();
    // The framework bundle ships its own resources.pak/icudtl.dat/locales
    // under Resources -- nothing loose gets deployed alongside SLCefProducer
    // itself (see viewer_manifest.py's own comment on this).
    init_options.resourcesDirPath      = (framework_dir / "Resources").string();
    init_options.mainBundlePath        = exe_dir.string();
    // Empty means "look for a bundled Helper.app" on macOS specifically
    // (unlike Windows/Linux, where empty already means "re-exec myself") --
    // SLCefProducer isn't bundled that way, so every CEF sub-process
    // (GPU/renderer/network) fails to launch at all without this set.
    init_options.browserSubprocessPath = get_exe_path().string();
#endif
    if (!llCefBrowserLib::Initialize(init_options)) {
        std::cerr << "llCefBrowserLib::Initialize failed\n";
        return 1;
    }

    if (remote_debugging_port > 0)
    {
        // Deliberately not an in-process DevTools popup (CefBrowserHost::ShowDevTools) --
        // that opens a real, GPU-composited native window, which on at least one real
        // machine tested reliably crashed/hung the renderer as soon as anything (e.g. a
        // mouse move) needed the CEF UI thread to resolve its state, taking the whole
        // producer down with it. Chrome's remote-debugging protocol serves the exact
        // same DevTools UI over HTTP instead, with no native window in this process at
        // all -- open the URL below in any desktop browser.
        log_info("SLCefProducer: remote debugging on http://localhost:" + std::to_string(remote_debugging_port));
    }

    log_info("SLCefProducer: llCefBrowser " + llCefBrowserLib::GetVersion() +
             ", CEF " + llCefBrowserLib::GetCefVersion() +
             ", Chromium " + llCefBrowserLib::GetChromiumVersion());

    // One shared browser-context cache for every browser this process
    // creates -- a subdirectory of rootCachePath, matching llCefBrowser's
    // own examples. A unique_ptr, not a plain stack object: it must be
    // explicitly destroyed (see the shutdown sequence below) strictly
    // before llCefBrowserLib::Shutdown() runs, not merely by the time
    // run_producer() returns -- a plain local's destructor would run too
    // late, after Shutdown() rather than before it.
    // Two separate cache/cookie contexts -- "Default" for 2D floater/UI media, "Prim"
    // for in-world/prim media -- so a cookie set on one (see kSetOpenIDCookie) is never
    // visible to the other. See llCefBrowserManager::CreateBrowser()'s own isUI param.
    auto manager = std::make_unique<llCefBrowserManager>((cache_dir / "Default").string(),
                                                          (cache_dir / "Prim").string());

    LLConfig view_cfg; // template for whichever index gets allocated on demand
    view_cfg.max_width  = kMaxWidth;
    view_cfg.max_height = kMaxHeight;
    const std::uint64_t worst_case_bytes = segment_bytes(view_cfg) * std::uint64_t(slot_count);

    std::vector<Slot> slots(static_cast<std::size_t>(slot_count)); // all start unallocated (pub == nullptr)

    // See JsBridge's own comment. Registered once, process-wide, for the whole
    // lifetime of the producer -- unregistered explicitly before Shutdown() below.
    JsBridge jsBridge(slots);
    llCefBrowserLib::SetJavaScriptBridge(&jsBridge);

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
        banner << "SLCefProducer: control channel ready, up to " << slot_count
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
        // chance to paint/publish within this same tick.
        while (control->receive(cmd))
        {
            if (cmd.type == kShutdownProducer) { g_run = 0; continue; }
            if (cmd.type == kSetOpenIDCookie)
            {
                std::string url, name, value, domain, path;
                bool httpOnly, secure, alsoPrimContext;
                if (unpack_openid_cookie(cmd.data.data(), cmd.data.size(), url, name, value, domain, path,
                                         httpOnly, secure, alsoPrimContext))
                {
                    manager->SetCookie(url, name, value, domain, path, httpOnly, secure, nullptr, alsoPrimContext);
                }
                continue;
            }
            if (cmd.type != kRequestSlot) continue;

            // Defaults match the old, isUI-only payload's fallback (see
            // unpack_request_slot's own comment) -- untouched if the consumer sent
            // a short/old-format payload, or clamp-adjusted below if it sent a real
            // ceiling. Clamped to this producer's own absolute maximum (never more)
            // and its default browser size (never less -- CreateBrowser() below
            // always starts a fresh view at kDefaultWidth/kDefaultHeight regardless
            // of what was requested, so a segment smaller than that would never fit
            // even the first frame).
            bool isUI = true;
            std::uint32_t requested_max_width = kMaxWidth;
            std::uint32_t requested_max_height = kMaxHeight;
            // The payload's trailing backend and audioOnly bytes are both vestigial here
            // since the 2026-09-30 producer split -- this producer only ever talks to
            // CEF-backed consumers (see kRequestSlot's own comment in cefshm_protocol.h),
            // so both are unpacked and then ignored, not validated. CEF has no audio-only
            // slot concept at all (that's SLVlcProducer-only, see
            // llstreamingaudio_libvlc.cpp). The slot's real use is trusted as sent, not
            // re-validated against whatever URL kSetUrl later brings, since this producer
            // already extends the identical trust to kSetUrl itself (navigates to
            // whatever string arrives, zero validation).
            std::uint8_t backend_byte = 0;
            bool audio_only = false;
            bool audio_capture = false;
            unpack_request_slot(cmd.data.data(), cmd.data.size(), isUI, requested_max_width,
                                 requested_max_height, backend_byte, audio_only, audio_capture);
            LLConfig slot_cfg = view_cfg;
            slot_cfg.max_width  = std::clamp(requested_max_width,  kDefaultWidth,  kMaxWidth);
            slot_cfg.max_height = std::clamp(requested_max_height, kDefaultHeight, kMaxHeight);

            // Try every currently-free index, not just the lowest one: allocate_slot()
            // can fail for a specific index (e.g. its just-reclaimed segment hasn't
            // actually released its OS-level handle yet) without any other free index
            // being similarly stuck. Only trying the lowest free index and giving up
            // would let one bad slot jam every subsequent request in the whole
            // producer, not just requests that happen to land on that exact index.
            //
            // One full pass over every free index per attempt, not a per-index retry
            // loop: a stuck index should cost nothing beyond moving on to the next one
            // within the same pass. Only retry the whole pass, with a brief wait, if
            // every free index failed on it -- kAllocateSlotRetries/RetryInterval's own
            // comment explains why this is at this level rather than inside
            // allocate_slot() itself.
            int free_index = -1;
            for (int outer = 0; outer < kAllocateSlotRetries && free_index < 0; ++outer)
            {
                bool any_free = false;
                for (int i = 0; i < slot_count; ++i)
                {
                    if (slots[std::size_t(i)].pub) continue; // not free
                    any_free = true;
                    if (allocate_slot(slots[std::size_t(i)], i, slot_cfg, *manager, now, isUI, audio_capture, slots))
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

            // Reply only now that the segment and the browser both
            // demonstrably exist: this process is single-threaded, so this
            // command's own release store (below, inside send()) is
            // ordered after every write allocate_slot() just made,
            // including the new segment's own "release the magic last"
            // store -- the requesting consumer's acquire-load of this
            // reply therefore guarantees it will see a fully-initialised
            // header once it opens that segment by name.
            // The flags byte is what tells the consumer whether audio capture is actually
            // on -- it may have asked and been declined (see allocate_slot()).
            std::uint8_t payload[5];
            const std::uint8_t flags = slots[std::size_t(free_index)].audioPub ? kSlotFlagAudioCapture : 0;
            const std::uint32_t payload_len = pack_slot_assigned(payload, std::uint32_t(free_index), flags);
            control->send(kSlotAssigned, payload, payload_len, cmd.id);
        }

#if defined(_WIN32)
        // CefDoMessageLoopWork() below only pumps CEF's own internal scheduled work -
        // it does NOT service the native Win32 message queue for any real (non-
        // offscreen) window CEF creates, e.g. ShowDevTools()'s popup. Every one of our
        // own browsers is windowless/OSR, so this was never needed until DevTools
        // existed: without it, that popup's queued messages (WM_PAINT, WM_MOUSEMOVE,
        // etc.) are never dispatched, and CEF's own window-creation/compositor
        // bookkeeping for it can end up waiting on state that never arrives - which
        // wedges the single CEF UI thread the next time anything else needs it (e.g.
        // dispatching a mouse-move to a completely unrelated browser), stalling this
        // whole loop, including the shm heartbeat, long enough for the consumer to
        // conclude the connection died. Windows-only: DevTools ships as Chrome
        // remote-debugging instead now (see run_producer()'s own comment on
        // --remote-debugging-port), so no real window is ever actually created any
        // more in practice, on any platform -- this is defensive scaffolding for a
        // feature this process doesn't currently use, not a live requirement. Not
        // worth inventing a Cocoa/X11 equivalent (a fundamentally different native
        // event-loop model on each) for a code path nothing exercises today.
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
#endif

        // Pumps every live browser and the resize-confirmation watchdog.
        llCefBrowserLib::DoMessageLoopWork();
        manager->Tick();

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
                free_slot(s, int(i), *manager, "crashed consumer", slots);
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
                    // in case the same consumer reconnects shortly (closing and
                    // reopening the same floater quickly reuses this browser instead
                    // of forcing a fresh navigate/reload).
                    log_disconnect("slot " + std::to_string(i) + " viewer detached (grace period running)" + active_slot_suffix(slots));
                    s.had_subscriber = false;
                }

                if (now - s.last_active >= kIdleGracePeriod)
                {
                    free_slot(s, int(i), *manager, "idle timeout", slots);
                    continue;
                }
            }

            while (s.pub->receive(cmd))
            {
                if (cmd.type == kSetRenderRate)
                {
                    // Same opcode SLVlcProducer also honours for its own slots (see
                    // cefshm_protocol.h) -- just sets Slot's own target_fps field here,
                    // read back by the render/publish tail below.
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
                    manager->Navigate(s.cefHandle, url);
                    break;
                }

                case kExecuteJavaScript:
                    manager->ExecuteJavaScript(s.cefHandle, std::string(cmd.text()));
                    break;


                case kMouseMove: {
                    std::int32_t x, y;
                    if (unpack_i32x2(cmd.data.data(), cmd.data.size(), x, y))
                        manager->SendMouseMoveEvent(s.cefHandle, int(x), int(y));
                    break;
                }
                case kMouseButton: {
                    std::int32_t x, y; std::uint8_t button, action, click_count;
                    llCefMouseButton mapped;
                    if (unpack_mouse_button(cmd.data.data(), cmd.data.size(), x, y, button, action, click_count) &&
                        map_mouse_button(button, mapped))
                        manager->SendMouseClickEvent(s.cefHandle, int(x), int(y), mapped, cef_mouse_up(action), int(click_count));
                    break;
                }
                case kResize: {
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
                            manager->ResizeBrowser(s.cefHandle, int(w), int(h));
                        }
                    }
                    break;
                }
                case kScrollWheel: {
                    std::int32_t x, y, deltaY;
                    if (unpack_scroll(cmd.data.data(), cmd.data.size(), x, y, deltaY)) {
                        manager->SendMouseWheelEvent(s.cefHandle, int(x), int(y), int(deltaY));
                    }
                    break;
                }
                case kKeyEvent: {
                    KeyEventType type;
                    std::uint32_t modifiers, character, unmodified_character;
                    std::int32_t windows_key_code, native_key_code;
                    bool is_system_key;
                    if (unpack_key_event(cmd.data.data(), cmd.data.size(), type, modifiers, windows_key_code,
                                          native_key_code, character, unmodified_character, is_system_key))
                    {
                        // type/modifiers are value-compatible with llCefKeyEventType/
                        // llCefKeyModifier by convention (see cefshm_protocol.h's own
                        // KeyEventType/KeyEventModifier comment) -- no per-bit translation
                        // needed here, only CEF's own enums (inside SendKeyEvent itself)
                        // ever need to change if a future CEF version renumbers them.
                        manager->SendKeyEvent(s.cefHandle, static_cast<llCefKeyEventType>(type), modifiers,
                                              windows_key_code, native_key_code, character,
                                              unmodified_character, is_system_key);
                    }
                    break;
                }
                case kSetFocus: {
                    if (!cmd.data.empty()) {
                        manager->SetFocus(s.cefHandle, cmd.data[0] != 0);
                    }
                    break;
                }
                case kSetMuted: {
                    if (!cmd.data.empty()) {
                        manager->SetAudioMuted(s.cefHandle, cmd.data[0] != 0);
                    }
                    break;
                }
                case kSetVolume: {
                    // Collapsed to CEF's existing binary capability -- see kSetVolume's
                    // own comment in cefshm_protocol.h. Continuous volume for CEF
                    // (JS-injection) is separate, not-yet-scoped work.
                    if (!cmd.data.empty()) {
                        manager->SetAudioMuted(s.cefHandle, cmd.data[0] == 0);
                    }
                    break;
                }
                case kSetPageZoom: {
                    float zoom;
                    if (unpack_f32(cmd.data.data(), cmd.data.size(), zoom)) {
                        manager->SetPageZoom(s.cefHandle, zoom);
                    }
                    break;
                }
                case kCut:
                    manager->Cut(s.cefHandle);
                    break;
                case kCopy:
                    manager->Copy(s.cefHandle);
                    break;
                case kPaste:
                    manager->Paste(s.cefHandle);
                    break;
                case kGoBack:
                    manager->GoBack(s.cefHandle);
                    break;
                case kGoForward:
                    manager->GoForward(s.cefHandle);
                    break;
                case kStopLoad:
                    manager->StopLoad(s.cefHandle);
                    break;
                case kReload: {
                    if (!cmd.data.empty()) {
                        manager->Reload(s.cefHandle, cmd.data[0] != 0);
                    }
                    break;
                }
                case kFileDialogResponse: {
                    std::int64_t dialogId;
                    std::vector<std::string> filePaths;
                    if (unpack_file_dialog_response(cmd.data.data(), cmd.data.size(), dialogId, filePaths)) {
                        manager->RespondToFileDialog(s.cefHandle, dialogId, filePaths);
                    }
                    break;
                }
                case kRespondToQuery: {
                    std::int64_t queryId; bool success; std::int32_t errorCode; std::string response;
                    if (unpack_query_response(cmd.data.data(), cmd.data.size(), queryId, success, errorCode, response)) {
                        llCefBrowserLib::RespondToQuery(queryId, success, response, errorCode);
                    }
                    break;
                }
                default:
                    break;
                }
            }

            // Request a fresh composite right now, reflecting whatever input this
            // slot just received above -- rather than waiting on CEF's own
            // windowless_frame_rate-paced internal timer (see the
            // external_begin_frame_enabled comment in llCefBrowserManagerImpl::
            // CreateBrowser()). Cheap to call even when nothing changed: Chromium's
            // own compositor already skips real work if there's nothing new to paint.
            // Throttled to s.target_fps when set (see kSetRenderRate) -- distance/
            // priority-based media (far away, out of view, etc.) doesn't need this
            // slot's CEF instance painting at full rate; UI/parcel media never sets
            // target_fps at all, so it's unaffected.
            const bool begin_frame_due = (s.target_fps == 0) ||
                (now - s.last_begin_frame >= std::chrono::duration<double>(1.0 / s.target_fps));
            if (begin_frame_due) {
                s.last_begin_frame = now;
                manager->SendExternalBeginFrame(s.cefHandle);
            }

            int fw = 0, fh = 0;
            if (manager->CopyLatestFrame(s.cefHandle, s.frameBuf, fw, fh)) {
                s.pub->publish(s.frameBuf.data(), std::uint32_t(fw), std::uint32_t(fh));
            } else {
                // publish() itself keeps the heartbeat fresh as a side effect (see
                // LLPublisher::publish()), but a mostly-static page can go several
                // seconds between real repaints -- heartbeat() is the independent signal
                // llshmframe actually intends for exactly that case, so a subscriber's
                // LLSubscriber::poll()/producer_responsive() check doesn't mistake "the
                // page just isn't repainting right now" for "the producer process died."
                s.pub->heartbeat();
            }

            if (s.audioPub) {
                pump_test_tone(s, int(i), now);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    log_info("SLCefProducer: shutting down");

    manager->DestroyAll();
    // Pump a few more turns so each async close handshake finishes before
    // the manager (and its shared CefRequestContext reference) is
    // destroyed, matching llCefBrowser's own examples.
    for (int i = 0; i < 30; ++i)
        llCefBrowserLib::DoMessageLoopWork();

    manager.reset(); // must be destroyed before Shutdown(), not merely by the time run_producer() returns
    llCefBrowserLib::SetJavaScriptBridge(nullptr); // jsBridge is about to go out of scope
    llCefBrowserLib::Shutdown();
    return 0;
}

#if defined(_WIN32)
// Windowless by default (this process is launched/killed automatically by
// the Viewer every session -- a flashing console for it would be a
// regression from the legacy Dullahan/SLPlugin path, which is windowless
// too), unlike llcefshm-example's own console-mode main(). WinMain's own
// lpCmdLine is a single unsplit ANSI string missing argv[0], so instead
// forward the MSVC CRT's __argc/__argv (declared by <cstdlib>, already
// included above) -- these are populated identically to what a console
// main() would receive, regardless of which entry point the linker
// actually used, since they come from the same CRT startup code either
// way. This also means every CEF-spawned helper subprocess (which
// re-execs this exact binary on Windows/Linux) inherits the same
// windowless subsystem for free, since that's a static property of the PE
// file, not something set per-process. On macOS, CEF's sub-process model
// is different (a separate, real .app bundle per sub-process type -- see
// llcefbrowser's own src/host/llCefBrowserHost.cpp) -- this same
// executable IS the browser process there, always, and never gets
// re-exec'd for anything, so there's no windowless-subsystem concept to
// preserve at all; a plain main() below already has no console of its own
// unless one is already attached (e.g. launched from a terminal).
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

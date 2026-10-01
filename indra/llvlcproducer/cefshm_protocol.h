/**
 *
 * @file cefshm_protocol.h
 * @brief Application-level protocol shared with the viewer's llembeddedbrowser consumer, including the control channel
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

// A deliberate byte-compatible copy of llcefshm-example's own
// src/cefshm_protocol.h (which is itself kept in lockstep with the viewer's
// indra/llembeddedbrowser/cefshm_protocol.h, and indra/llcefproducer/
// cefshm_protocol.h) -- not a shared include, so this component stays
// self-contained. Keep all four in lockstep by hand.
//
// This is the LibVLC-only producer's copy (SLVlcProducer, split out from the
// former combined SLMediaProducer 2026-09-30) -- keeps the full Opcode enum
// word-for-word identical to every other copy (so a diff across copies is
// meaningful), but only the pack_*/unpack_* helpers llvlcproducer.cpp
// actually calls. See indra/llcefproducer/cefshm_protocol.h for the much
// larger CEF-only helper subset.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace cefshm_demo
{
    inline constexpr int           kSlotCount     = 32;

    // CEF and LibVLC now run as two entirely separate processes (SLCefProducer/
    // SLVlcProducer, split 2026-09-30 for licensing reasons -- see
    // indra/llcefproducer/cefshm_protocol.h's own kControlChannelName comment)
    // and so need distinct channel names: two independent OS processes cannot
    // both bind a channel named "llcefshm_control", and per-view channel names
    // built from a small producer-local slot index (0..31) would otherwise
    // collide between them. The consumer's own copy of this header
    // (indra/llembeddedbrowser/cefshm_protocol.h) carries both this producer's
    // names and llcefproducer's, under the distinguishing kVlc* spelling; this
    // producer's own code just uses these plain names directly, since within
    // one executable there is only ever one active set.
    inline constexpr char          kChannelPrefix[] = "llvlcshm_view_";
    inline constexpr char          kControlChannelName[] = "llvlcshm_control";

    inline constexpr std::uint32_t kDefaultWidth  = 960;
    inline constexpr std::uint32_t kDefaultHeight = 540;

    // Absolute sanity ceiling for any one slot's SHM segment/buffer -- see
    // indra/llcefproducer/cefshm_protocol.h's own much longer comment on this
    // same constant for the full history; identical value and meaning here.
    inline constexpr std::uint32_t kMaxWidth      = 4096;
    inline constexpr std::uint32_t kMaxHeight     = 4096;

    enum Opcode : std::uint32_t
    {
        // consumer -> producer, per-view channel
        kSetUrl      = 1, // text payload: a URL, e.g. "https://example.com"
        kMouseMove   = 2, // data = {int32 x, int32 y}, canvas-space, little-endian
        kMouseButton = 3, // data = {int32 x, int32 y, uint8 button, uint8 action, uint8 click_count}
                          // click_count matches CEF's own SendMouseClickEvent() semantics (1 for a
                          // normal click, 2 for the down half of a double-click) -- CEF is windowless
                          // here, so it has no real OS window to infer a double-click's timing from
                          // on its own; the embedder (us) must say so explicitly on every call.
        kResize      = 4, // data = {uint32 width, uint32 height}
        kScrollWheel = 8, // data = {int32 x, int32 y, int32 deltaY} -- deltaY in CEF's own wheel-delta
                          // units (a multiple of ~30-120 per notch), see SendMouseWheelEvent
        kKeyEvent    = 9, // data = pack_key_event(...) -- a platform-neutral, CEF-shaped key
                          // event, translated by the consumer's own per-platform LLWindow
                          // subclass from its native event. No LibVLC equivalent.
        kSetFocus    = 17, // data = {uint8 focus} -- CEF only, no LibVLC equivalent.
        kExecuteJavaScript = 21, // text payload -- CEF only, no LibVLC equivalent.
        kSetPageZoom = 24, // data = {float32 zoomFactor} -- CEF only, no LibVLC equivalent.
        kCut   = 27, // CEF only, no LibVLC equivalent.
        kCopy  = 28, // CEF only, no LibVLC equivalent.
        kPaste = 29, // CEF only, no LibVLC equivalent.
        kSetMuted = 30, // data = {uint8 muted} -- honoured here (maps to volume 0/100 -- see
                          // kSetVolume), so teardown-time silencing works identically for both
                          // producers regardless of which opcode a given call site uses.
        kGoBack    = 31, // CEF only, no LibVLC equivalent.
        kGoForward = 32, // CEF only, no LibVLC equivalent.
        kStopLoad  = 33, // CEF only, no LibVLC equivalent.
        kReload    = 36, // CEF only, no LibVLC equivalent.
        kSetVolume = 37, // data = {uint8 volume} -- 0-100, matching libvlc_audio_set_volume()'s own
                          // native range directly, given the real distance-rolloff curve
                          // kSetMuted above can't. See indra/llcefproducer/cefshm_protocol.h's
                          // own longer comment on this opcode's shared meaning across producers.
        kSetRenderRate = 35, // data = {uint32 targetFps, uint8 priorityTier, url bytes (remainder)}
                          // -- caps how often this producer drains/publishes an already-decoded
                          // frame for this handle (0 = unthrottled, the default) -- see
                          // indra/llcefproducer/cefshm_protocol.h's own longer comment; handled
                          // identically here, just gating publish cadence rather than
                          // SendExternalBeginFrame (libvlc decodes on its own clock regardless).

        // consumer -> producer, control channel only
        kRequestSlot     = 5, // data = {uint8 isUI, uint32 maxWidth, uint32 maxHeight, uint8 backend}
                          // -- see indra/llcefproducer/cefshm_protocol.h's own much longer comment.
                          // backend is vestigial since the 2026-09-30 producer split (which producer
                          // you're talking to already fixes the backend) -- this producer ignores it
                          // and always treats every request as its own, LibVlc.
        kSetOpenIDCookie = 26, // CEF only -- LibVLC has no cookie-store concept at all. Never sent
                          // here; kept in the enum only so it stays word-for-word identical to
                          // every other copy of this header.
        kShutdownProducer = 25, // empty payload -- asks this producer to exit its main loop and run
                          // its own graceful shutdown instead of being killed outright.

        // producer -> consumer, control channel only; reply_to = request id
        kSlotAssigned    = 6, // data = {uint32 slot index}
        kSlotUnavailable = 7, // empty payload -- no free slot right now

        // producer -> consumer, per-view channel
        kEventLoadStart      = 10, // empty payload
        kEventLoadEnd        = 11, // data = {uint32 httpStatusCode} -- a pseudo-status here (see
                                   // libvlctabmanager.h's own ConsumeLoadEnd comment), not a real
                                   // HTTP code the way CEF's own is.
        kEventTitleChanged   = 12, // CEF only, no LibVLC equivalent.
        kEventAddressChanged = 13, // CEF only, no LibVLC equivalent.
        kEventCursorChanged  = 14, // CEF only, no LibVLC equivalent.
        kEventClickLinkHref     = 15, // CEF only, no LibVLC equivalent.
        kEventClickLinkNoFollow = 16, // CEF only, no LibVLC equivalent.
        kEventFileDialogRequest = 18, // CEF only, no LibVLC equivalent.

        // consumer -> producer, per-view channel
        kFileDialogResponse = 19, // CEF only, no LibVLC equivalent.

        // producer -> consumer, per-view channel
        kEventStatusTextChanged = 20, // CEF only, no LibVLC equivalent.
        kEventConsoleMessage = 22, // CEF only, no LibVLC equivalent.
        kEventVersionInfo = 23, // CEF only, no LibVLC equivalent -- this producer never sends it.
        kEventNavStateChanged = 34, // CEF only, no LibVLC equivalent.
        kEventLoadError = 38, // CEF only, no LibVLC equivalent -- a failed libvlc open surfaces
                                  // only as kEventLoadEnd's own pseudo-status, not this.

        // consumer -> producer, per-view channel -- handled here; a CEF-backed slot silently
        // ignores this (see indra/llcefproducer/cefshm_protocol.h's own comment).
        kSetPlaybackAction = 39, // data = {uint8 action} -- 0=Play, 1=Pause, 2=Stop. See
                                  // indra/llcefproducer/cefshm_protocol.h's own longer comment.
        // producer -> consumer, per-view channel -- sent here; never sent by SLCefProducer.
        kEventPlaybackStateChanged = 40, // data = {uint8 playing}

        // CEF-only (JS bridge) -- no LibVLC equivalent at all; never sent or handled here.
        kEventJSQuery = 41,
        kRespondToQuery = 42,
    };

    inline std::uint32_t pack_u32(std::uint8_t* d, std::uint32_t v)
    {
        d[0]=std::uint8_t(v); d[1]=std::uint8_t(v>>8); d[2]=std::uint8_t(v>>16); d[3]=std::uint8_t(v>>24);
        return 4;
    }

    inline bool unpack_u32(const std::uint8_t* d, std::size_t n, std::uint32_t& v)
    {
        if (n < 4) return false;
        v = std::uint32_t(d[0]) | (std::uint32_t(d[1])<<8) | (std::uint32_t(d[2])<<16) | (std::uint32_t(d[3])<<24);
        return true;
    }

    // Only the unpack half is needed here -- this producer never sends a resize
    // request, only receives one.
    inline bool unpack_size(const std::uint8_t* d, std::size_t n,
                            std::uint32_t& w, std::uint32_t& h)
    {
        if (n < 8) return false;
        w = std::uint32_t(d[0]) | (std::uint32_t(d[1])<<8) | (std::uint32_t(d[2])<<16) | (std::uint32_t(d[3])<<24);
        h = std::uint32_t(d[4]) | (std::uint32_t(d[5])<<8) | (std::uint32_t(d[6])<<16) | (std::uint32_t(d[7])<<24);
        return true;
    }

    inline bool unpack_i32x2(const std::uint8_t* d, std::size_t n,
                              std::int32_t& x, std::int32_t& y)
    {
        if (n < 8) return false;
        auto get = [&](int off) {
            return std::int32_t(std::uint32_t(d[off]) | (std::uint32_t(d[off + 1]) << 8) |
                                (std::uint32_t(d[off + 2]) << 16) | (std::uint32_t(d[off + 3]) << 24));
        };
        x = get(0); y = get(4);
        return true;
    }

    // Only the unpack half is needed here -- this producer never sends a mouse event,
    // only receives one (the click-to-pause gesture), matching this file's own
    // convention of keeping only the direction each side actually uses.
    inline bool unpack_mouse_button(const std::uint8_t* d, std::size_t n,
                                    std::int32_t& x, std::int32_t& y,
                                    std::uint8_t& button, std::uint8_t& action,
                                    std::uint8_t& click_count)
    {
        if (n < 11 || !unpack_i32x2(d, n, x, y)) return false;
        button = d[8]; action = d[9]; click_count = d[10];
        return true;
    }

    inline std::uint32_t pack_request_slot(std::uint8_t* d, bool isUI, std::uint32_t maxWidth,
                                            std::uint32_t maxHeight, std::uint8_t backend)
    {
        d[0] = isUI ? 1 : 0;
        std::uint32_t n = 1 + pack_u32(d + 1, maxWidth);
        n += pack_u32(d + n, maxHeight);
        d[n++] = backend;
        return n;
    }

    // false (only isUI populated) for the old, isUI-only payload -- see kRequestSlot's
    // own comment on why that's a safe, deliberate fallback rather than an error. backend
    // defaults to 0 (Cef) for any payload shorter than 10 bytes, for the same reason --
    // assigned before the n<9 early return, so that fallback still leaves it initialized
    // (this producer ignores the value either way -- see kRequestSlot's own comment above).
    inline bool unpack_request_slot(const std::uint8_t* d, std::size_t n, bool& isUI,
                                     std::uint32_t& maxWidth, std::uint32_t& maxHeight,
                                     std::uint8_t& backend)
    {
        isUI = (n == 0) || (d[0] != 0);
        backend = (n >= 10) ? d[9] : 0;
        if (n < 9) return false;
        return unpack_u32(d + 1, n - 1, maxWidth) && unpack_u32(d + 5, n - 5, maxHeight);
    }

    inline std::uint32_t pack_render_rate(std::uint8_t* d, std::uint32_t targetFps, std::uint8_t priorityTier,
                                           const std::string& url)
    {
        std::uint32_t n = pack_u32(d, targetFps);
        d[n++] = priorityTier;
        std::memcpy(d + n, url.data(), url.size());
        n += std::uint32_t(url.size());
        return n;
    }

    inline bool unpack_render_rate(const std::uint8_t* d, std::size_t n, std::uint32_t& targetFps,
                                    std::uint8_t& priorityTier, std::string& url)
    {
        if (n < 5 || !unpack_u32(d, n, targetFps)) return false;
        priorityTier = d[4];
        url.assign(reinterpret_cast<const char*>(d + 5), n - 5);
        return true;
    }
}

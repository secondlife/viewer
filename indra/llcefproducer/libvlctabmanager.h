/**
 *
 * @file libvlctabmanager.h
 * @brief LibVlcTabManager: hosts LibVLC-backed media tabs (RTSP/RTMP/MMS -- media CEF
 *        cannot play) inside SLCefProducer, alongside llCefBrowserManager's CEF tabs.
 *        Publishes frames through the exact same llshmframe path CEF tabs already use --
 *        see llcefproducer.cpp's per-slot dispatch loop for how the two are picked between.
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

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// Deliberately no <vlc/vlc.h> here -- only libvlctabmanager.cpp includes it, the same
// pimpl spirit as llCefBrowserManager's own wrapper around CEF's own headers.

// Opaque, stable handle to one LibVLC tab -- mirrors llCefBrowserHandle's own shape
// (an index plus a generation counter, so a stale handle from a destroyed-and-reused
// slot is detectable rather than silently operating on the wrong tab).
struct VlcTabHandle
{
    std::uint32_t index = 0xFFFFFFFFu;
    std::uint32_t generation = 0;

    bool IsValid() const { return index != 0xFFFFFFFFu; }
    static VlcTabHandle Invalid() { return VlcTabHandle{}; }
};

// One shared libvlc_instance_t for the whole process (created in the constructor,
// released in the destructor) -- not one per tab. Only the small slice of
// llCefBrowserManager's own API shape a pure AV stream actually needs: no JS, no
// cut/copy/paste, no navigation history, no cursor/tooltip/auth/file-dialog callbacks --
// none of that has a LibVLC equivalent.
class LibVlcTabManager
{
public:
    // log_file_path: if non-empty, libvlc's own internal diagnostic log (network/demux/
    // decode errors -- the actual detail behind "why didn't this play") is written there
    // via libvlc_log_set_file(), the same way this producer's other logs
    // (slcefproducer_log.txt, cefshm_producer_log.txt) already work. Empty is a valid,
    // silent no-op for callers that don't need it (e.g. a future standalone test).
    explicit LibVlcTabManager(const std::string& log_file_path = {});
    ~LibVlcTabManager();

    LibVlcTabManager(const LibVlcTabManager&) = delete;
    LibVlcTabManager& operator=(const LibVlcTabManager&) = delete;

    // No media/player yet -- mirrors llCefBrowserManager::CreateBrowser("about:blank", ...):
    // the slot exists and has a sized frame buffer immediately, playback only starts once
    // Open() is called (see kSetUrl in llcefproducer.cpp). width/height are this tab's
    // starting/current buffer size -- a kResize that arrives before Open() (legitimate;
    // CEF always has a live "about:blank" browser to resize immediately, libvlc doesn't)
    // is remembered here and read back by Open() when it actually creates the player.
    //
    // maxWidth/maxHeight are this slot's negotiated ceiling (kRequestSlot's own
    // maxWidth/maxHeight, fixed for the slot's whole lifetime, already used to size the
    // llshmframe segment itself) -- the pixel buffer is allocated at THIS size once and
    // never reallocated smaller by Resize(). Confirmed necessary by a real crash: a
    // picture already in flight through libvlc's own decode/scale pipeline at the old
    // dimensions can still land in the lock/unlock/display callbacks after Resize() has
    // already shrunk the buffer to the new, smaller size, overrunning it in libvlc's own
    // picture_CopyPixels(). A buffer sized to the ceiling up front is large enough for
    // every width/height Resize() will ever be asked for (llcefproducer.cpp already
    // clamps every resize request to this same ceiling), so no in-flight picture at any
    // prior size can ever overrun it.
    VlcTabHandle CreateTab(int width, int height, int maxWidth, int maxHeight);
    void DestroyTab(VlcTabHandle handle);
    void DestroyAll();
    bool IsValid(VlcTabHandle handle) const;

    // (Re)creates the media+player for this tab at its current width/height and starts
    // playback immediately -- the LibVLC equivalent of Navigate(), and also this tab's
    // only "play" trigger: there is no separate play step, matching how CEF starts
    // rendering as soon as it's navigated. Safe to call again on an already-open tab
    // (e.g. a script changing the stream URL) -- tears down the previous player first.
    void Open(VlcTabHandle handle, const std::string& url);

    // Records a new target size; not applied immediately (a rapid sequence of resize
    // requests, e.g. dragging a 2D floater's resize handle, is coalesced into a single
    // application once requests stop arriving for a short settle period -- see the
    // .cpp's own comments). Once applied, this closes and reopens the player at the new
    // size -- a brief stop/restart, not a live libvlc_video_set_format() reconfigure of
    // the running player. An earlier version of this design tried exactly that (no
    // stop/recreate), and a disposable standalone test against screen:// (a synthetic,
    // non-seekable source) seemed to confirm it worked -- but that test only checked
    // for a crash or an interruption in playback, never actual pixel-level correctness
    // at the new size. Confirmed via extensive real-world testing against a real RTSP
    // stream: reconfiguring an already-running player produces severe, non-recovering
    // pixel corruption at the new size, regardless of timing (coalescing the requests,
    // a settle delay, or a grace period afterward all made no difference) -- closing
    // and reopening the identical URL at the identical size always rendered correctly.
    // See llcefproducer.cpp's kResize handling.
    void Resize(VlcTabHandle handle, int width, int height);

    // volume0to100: matches libvlc_audio_set_volume()'s own native range directly, no
    // scaling. See kSetVolume/kSetMuted in cefshm_protocol.h.
    void SetVolume(VlcTabHandle handle, int volume0to100);

    // Play/pause toggle -- there's no separate "play" trigger for a LibVLC tab (Open()
    // already starts playback immediately, see its own comment), so this is the only
    // transport control needed for a basic click-to-pause/resume. A no-op before the
    // player exists. See kMouseButton's LibVLC handling in llcefproducer.cpp.
    void TogglePlayPause(VlcTabHandle handle);

    // Same contract as llCefBrowserManager::CopyLatestFrame: returns false (dst
    // untouched) if there's no new frame since the last call. Thread-safe against the
    // decode-thread-driven video callbacks internally -- see libvlctabmanager.cpp.
    bool CopyLatestFrame(VlcTabHandle handle, std::vector<std::uint8_t>& dst, int& w, int& h);

    // Coalesced, edge-triggered load-state signals -- consumed (and cleared) once by
    // the main loop each tick, mirroring kEventLoadStart/kEventLoadEnd (both opcodes
    // already exist and already work for CEF tabs -- reused as-is here, not new wire
    // surface). NOT pushed synchronously from libvlc's own event-manager thread; see
    // libvlctabmanager.cpp for why a synchronous send from there would be unsafe.
    bool ConsumeLoadStart(VlcTabHandle handle);
    // pseudoHttpStatus: 200 on libvlc_MediaPlayerPlaying, 0 on
    // libvlc_MediaPlayerEncounteredError -- mirrors the "0 means network/load failure"
    // convention kEventLoadEnd's HTTP-status payload already carries for CEF tabs.
    bool ConsumeLoadEnd(VlcTabHandle handle, int& pseudoHttpStatus);

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

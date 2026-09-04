/**
 *
 * @file libvlctabmanager.cpp
 * @brief LibVlcTabManager implementation -- see libvlctabmanager.h.
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

#include "libvlctabmanager.h"

#include <vlc/vlc.h>

#include <atomic>
#include <chrono>
#include <mutex>

namespace
{
    // libvlc's own pixel format for the lock/unlock/display callback path -- tightly
    // packed 32-bit BGRA, matching CEF's own OnPaint() convention, so the shm frame
    // this produces needs no special-casing anywhere downstream of CopyLatestFrame().
    constexpr const char* kChroma = "RV32";
    constexpr unsigned kBytesPerPixel = 4;

    // The WIDTH (not height) actually configured on libvlc via libvlc_video_set_format()
    // is rounded up to this boundary before use -- confirmed necessary via real testing:
    // resizing a 2D floater's HEIGHT never corrupted the display, but resizing its WIDTH
    // reliably did, at essentially any width dragged to. That's the specific signature
    // of a row-stride/alignment requirement (height only changes how many whole rows
    // there are; width changes the byte layout of every single row) -- almost certainly
    // coming from libvlc's own hardware color-converter chain (d3d9_filters, seen
    // actively in real libvlc logs), which very plausibly requires/prefers an aligned
    // row pitch regardless of what pitch we explicitly request. 32 is a conservative,
    // widely-safe alignment for GPU/codec row strides. The tradeoff is a handful of
    // extra, unused pixel columns on the right edge of the published frame whenever the
    // requested width wasn't already aligned -- far preferable to the alternative
    // (severe, non-recovering pixel corruption for any non-aligned width).
    constexpr std::uint32_t kWidthAlignment = 32;
    std::uint32_t alignWidth(std::uint32_t width)
    {
        return (width + (kWidthAlignment - 1)) / kWidthAlignment * kWidthAlignment;
    }
    // Rounds up to kWidthAlignment, but never past maxWidth (t->pixels is sized to
    // maxWidth*maxHeight and must never be overrun) -- in the rare case maxWidth itself
    // isn't 32-aligned, this clamps down to it exactly rather than overrunning, which
    // may leave the effective width not perfectly aligned right at the ceiling. Safe
    // either way; only matters when resizing to within 32px of the slot's own max.
    std::uint32_t clampAndAlignWidth(std::uint32_t requestedWidth, std::uint32_t maxWidth)
    {
        return std::min(alignWidth(requestedWidth), maxWidth);
    }

    // How long a resize request must go unsuperseded before it's actually applied to
    // libvlc (a real libvlc_video_set_format() call) -- see Resize()'s own comment for
    // why applying one on every single resize request, rather than coalescing a rapid
    // sequence of them, actively corrupts the display rather than merely wasting work.
    // Chosen generously relative to what a filter-chain rebuild looked like in real
    // libvlc logs (well under 100ms); a rapid floater-resize drag keeps deferring this
    // on every subsequent request anyway, so the real-world cost is only ever paid
    // once, right after the user stops dragging.
    constexpr std::chrono::milliseconds kResizeSettleDelay{ 250 };

    // How long AFTER a resize is actually applied (a real libvlc_video_set_format()
    // call, in MaybeApplyPendingResize()) before a frame is trusted/published at the
    // new size. That call returning does not mean libvlc's own filter-chain rebuild
    // (running on ITS OWN decode thread, not synchronously inside the call) has
    // actually finished -- confirmed necessary via real testing: coalescing the
    // requests alone (kResizeSettleDelay above) fixed corruption for a small resize,
    // but a larger one -- presumably because a bigger jump takes the filter chain
    // longer to rebuild -- still corrupted. This is a second, independent grace period
    // layered on top of that one, not a replacement for it.
    constexpr std::chrono::milliseconds kPostApplyGraceDelay{ 400 };

    // One heap-allocated Tab per slot, addressed via VlcTabHandle::index -- never moved
    // or reallocated once created (see LibVlcTabManager::Impl::mTabs' own comment):
    // lock_cb/unlock_cb/display_cb/event_cb all receive a raw Tab* as their
    // opaque/user_data pointer, and libvlc's own decode and event-manager threads can
    // hold onto that pointer for as long as a player exists, well outside any call into
    // this class -- a container that could relocate the Tab object out from under a
    // callback still in flight would be a real use-after-free.
    struct Tab
    {
        std::uint32_t generation = 0;
        bool live = false;

        libvlc_media_player_t* player = nullptr;
        std::string currentUrl; // see Open()'s same-URL guard

        // Frame buffer -- guarded by mutex. Written by lock_cb (held across
        // lock_cb..unlock_cb, the same span libvlc itself always calls them in, on its
        // own decode thread) and by Resize() (the producer's main thread); read by
        // CopyLatestFrame() (main thread). This is the actual cross-thread handoff:
        // nothing from a libvlc callback may ever call into llshmframe's publish()
        // directly, since the producer's whole main loop assumes it owns that call
        // exclusively (see llmediaproducer.cpp's own "single-threaded" comment) --
        // CopyLatestFrame() is polled from there instead.
        std::mutex mutex;
        std::vector<std::uint8_t> pixels; // sized to maxWidth*maxHeight*kBytesPerPixel once, in CreateTab() -- never reallocated by Resize(), see CreateTab()'s own comment in the header
        std::uint32_t width = 0, height = 0;
        std::uint32_t maxWidth = 0, maxHeight = 0;
        bool frameDirty = false; // no lock needed -- a single bool, written only by display_cb

        // The latest requested size that hasn't been applied to libvlc yet, and when it
        // was last (re)requested -- see Resize()'s own comment for why applying a resize
        // is deliberately coalesced/delayed rather than happening immediately on every
        // call. Both Resize() and MaybeApplyPendingResize() only ever run on the
        // producer's single main thread, so no extra locking is needed for these fields
        // specifically (unlike pixels/width/height, which are also touched from
        // libvlc's own decode thread).
        std::uint32_t pendingWidth = 0, pendingHeight = 0;
        bool hasPendingResize = false;
        std::chrono::steady_clock::time_point lastResizeRequestTime{};

        // When width/height (the format actually applied to libvlc) last changed --
        // see CopyLatestFrame()'s own comment for why a frame is refused for a short
        // grace period after that, even though the coalescing above already guarantees
        // libvlc is only ever asked to reconfigure once per settled resize, not
        // repeatedly. The libvlc_video_set_format() call returning doesn't mean the
        // filter-chain rebuild it triggers (on libvlc's own decode thread) has actually
        // finished yet.
        std::chrono::steady_clock::time_point lastAppliedResizeTime{};

        // Status events -- coalesced, edge-triggered, atomic (single writer thread at a
        // time -- libvlc's own event-manager thread -- single reader -- the main loop
        // draining these once per tick, same reasoning as the frame buffer above).
        std::atomic<bool> loadStartPending{ false };
        std::atomic<bool> loadEndPending{ false };
        std::atomic<int>  loadEndStatus{ 0 };

        // Same coalesced/edge-triggered/atomic shape as the load-state pair above, but
        // for play/pause/stop transitions -- see kEventPlaybackStateChanged's own
        // comment in cefshm_protocol.h for why this needs to be fed back to the
        // consumer at all, rather than the consumer just trusting its own last-sent
        // Play/Pause/Stop command.
        std::atomic<bool> playStateChangedPending{ false };
        std::atomic<bool> playStateIsPlaying{ false };
    };

    void* lock_cb(void* opaque, void** planes)
    {
        Tab* t = static_cast<Tab*>(opaque);
        t->mutex.lock();
        *planes = t->pixels.data();
        return nullptr; // single-buffer, like the legacy plugin -- no separate picture id needed
    }
    void unlock_cb(void* opaque, void* /*picture*/, void* const* /*planes*/)
    {
        static_cast<Tab*>(opaque)->mutex.unlock();
    }
    void display_cb(void* opaque, void* /*picture*/)
    {
        static_cast<Tab*>(opaque)->frameDirty = true;
    }

    void event_cb(const libvlc_event_t* event, void* user_data)
    {
        Tab* t = static_cast<Tab*>(user_data);
        switch (event->type)
        {
            case libvlc_MediaPlayerOpening:
                t->loadStartPending = true;
                break;
            case libvlc_MediaPlayerPlaying:
                t->loadEndPending = true;
                t->loadEndStatus = 200;
                t->playStateChangedPending = true;
                t->playStateIsPlaying = true;
                break;
            case libvlc_MediaPlayerEncounteredError:
                t->loadEndPending = true;
                t->loadEndStatus = 0; // mirrors kEventLoadEnd's own "0 = network/load failure" convention
                t->playStateChangedPending = true;
                t->playStateIsPlaying = false;
                break;
            case libvlc_MediaPlayerPaused:
            case libvlc_MediaPlayerStopped:
            case libvlc_MediaPlayerEndReached:
                t->playStateChangedPending = true;
                t->playStateIsPlaying = false;
                break;
            default:
                break;
        }
    }
}

class LibVlcTabManager::Impl
{
public:
    explicit Impl(const std::string& log_file_path)
    {
        // Audio only for playback control -- this class never renders anything without
        // video callbacks explicitly wired up per-tab (see Open()), so no video output
        // subsystem needs to exist at the libvlc_instance_t level at all.
        // --verbose=2: libvlc's default log level is too low to emit anything useful,
        // even for an outright playback failure -- found the hard way debugging a
        // "just shows black" report with an otherwise completely empty log file. 2
        // (info) is deliberately not the max (4/debug), which is noisy enough to bury
        // the one line that actually matters.
        //
        // Logging goes through libvlc's own --file-logging/--logfile= command-line
        // options (the "logger" module -- see plugins/logger/libfile_logger_plugin.dll),
        // NOT libvlc_log_set_file(). That C API looked like the right tool and is
        // documented for exactly this, but proved to emit nothing at all in this
        // vendored build even for a confirmed, real libvlc_MediaPlayerEncounteredError
        // (verified: the event fired, ConsumeLoadEnd() saw status=0, and still zero
        // bytes ever reached the FILE*). --file-logging is the mechanism the real vlc.exe
        // CLI itself uses and is far better exercised in the wild -- switched to it
        // instead of chasing why the lower-level API is silent here.
        std::string logfile_arg = "--logfile=" + log_file_path;
        std::vector<char const*> vlc_argv = { "--no-video-title-show", "--verbose=2" };
        if (!log_file_path.empty())
        {
            vlc_argv.push_back("--file-logging");
            vlc_argv.push_back(logfile_arg.c_str());
        }
        mLibVLC = libvlc_new(int(vlc_argv.size()), vlc_argv.data());
    }

    ~Impl()
    {
        DestroyAll();
        if (mLibVLC)
        {
            libvlc_release(mLibVLC);
            mLibVLC = nullptr;
        }
    }

    VlcTabHandle CreateTab(int width, int height, int maxWidth, int maxHeight)
    {
        if (!mLibVLC)
        {
            return VlcTabHandle::Invalid();
        }

        std::uint32_t index = 0;
        for (; index < mTabs.size(); ++index)
        {
            if (!mTabs[index]->live) break;
        }
        if (index == mTabs.size())
        {
            mTabs.push_back(std::make_unique<Tab>());
        }

        Tab& t = *mTabs[index];
        t.live = true;
        t.generation++;
        t.player = nullptr;
        t.frameDirty = false;
        t.loadStartPending = false;
        t.loadEndPending = false;
        t.loadEndStatus = 0;
        t.playStateChangedPending = false;
        t.playStateIsPlaying = false;
        // See kWidthAlignment's own comment -- the width libvlc is actually told to
        // decode into is rounded up to a safe row-stride alignment; height is not.
        t.width = clampAndAlignWidth(std::uint32_t(width), std::uint32_t(maxWidth));
        t.height = std::uint32_t(height);
        t.pendingWidth = t.width;
        t.pendingHeight = t.height;
        t.hasPendingResize = false;
        t.lastResizeRequestTime = std::chrono::steady_clock::time_point{};
        t.lastAppliedResizeTime = std::chrono::steady_clock::time_point{}; // already "settled" -- no grace period needed before the very first frame
        t.maxWidth = std::uint32_t(maxWidth);
        t.maxHeight = std::uint32_t(maxHeight);
        {
            std::lock_guard<std::mutex> lock(t.mutex);
            // Sized to the ceiling, not the starting width/height -- see this class's
            // own header comment on why Resize() must never reallocate this smaller.
            t.pixels.assign(std::size_t(maxWidth) * std::size_t(maxHeight) * kBytesPerPixel, 0);
        }

        VlcTabHandle h;
        h.index = index;
        h.generation = t.generation;
        return h;
    }

    void DestroyTab(VlcTabHandle handle)
    {
        Tab* t = find(handle);
        if (!t) return;
        closePlayer(*t);
        t->live = false;
        t->pixels.clear();
        t->width = t->height = 0;
        t->pendingWidth = t->pendingHeight = 0;
        t->hasPendingResize = false;
        t->maxWidth = t->maxHeight = 0;
    }

    void DestroyAll()
    {
        for (auto& tab : mTabs)
        {
            if (tab->live)
            {
                closePlayer(*tab);
                tab->live = false;
            }
        }
    }

    bool IsValid(VlcTabHandle handle) const
    {
        return find(handle) != nullptr;
    }

    // force_reopen: bypasses the same-URL no-op guard below -- used only by
    // MaybeApplyPendingResize() to actually recreate the player at a new size (seeing
    // the identical URL is exactly what a resize-triggered reopen looks like, and is
    // NOT the redundant-kSetUrl case that guard exists for). Real kSetUrl handling
    // (llmediaproducer.cpp) always calls with the default false.
    void Open(VlcTabHandle handle, const std::string& url, bool force_reopen = false)
    {
        Tab* t = find(handle);
        if (!t || !mLibVLC) return;

        if (!force_reopen && t->player && t->currentUrl == url)
        {
            const libvlc_state_t state = libvlc_media_player_get_state(t->player);
            if (state == libvlc_Opening || state == libvlc_Buffering ||
                state == libvlc_Playing || state == libvlc_Paused)
            {
                // Redundant re-navigate to the same URL a still-live player is already
                // handling -- a no-op, not a reload. The Viewer resends kSetUrl for
                // reasons that make sense for a web page (e.g. periodic media-param
                // resync from the sim) but are actively destructive for a stream:
                // unconditionally tearing down and reconnecting from scratch on every
                // resend meant a short-lived live source could be restarted before it
                // ever produced a displayable frame. Confirmed via a real MediaMTX RTMP
                // test: the log showed a successful TCP connect + H264 decode start,
                // then an immediate second kSetUrl for the identical URL tearing it
                // down again. Ended/Stopped/Error states fall through to a real reopen
                // below, so an explicit retry after a genuine failure still works.
                return;
            }
        }

        closePlayer(*t); // safe to call again on an already-open tab -- tears down the previous player first
        t->currentUrl = url;

        libvlc_media_t* media = libvlc_media_new_location(mLibVLC, url.c_str());
        if (!media) return;

        t->player = libvlc_media_player_new_from_media(media);
        libvlc_media_release(media); // player takes its own reference -- safe to release ours now
        if (!t->player) return;

        libvlc_video_set_callbacks(t->player, &lock_cb, &unlock_cb, &display_cb, t);
        libvlc_video_set_format(t->player, kChroma, t->width, t->height, t->width * kBytesPerPixel);
        // No pending resize to coalesce here -- closePlayer() just above already
        // synchronously tore down any previous player (libvlc_media_player_stop() is
        // documented synchronous), and this new one hasn't decoded a single frame yet,
        // so applying t->width/height directly, right now, is safe -- see Resize()'s
        // own comment on why a MID-STREAM resize can't do the same.
        t->pendingWidth = t->width;
        t->pendingHeight = t->height;
        t->hasPendingResize = false;
        // Reset for the same reason -- Open() can run again on an already-live tab (a
        // URL change), and a stale timestamp from that tab's PREVIOUS lifetime must not
        // delay this new player's very first frame.
        t->lastAppliedResizeTime = std::chrono::steady_clock::time_point{};

        libvlc_event_manager_t* em = libvlc_media_player_event_manager(t->player);
        if (em)
        {
            libvlc_event_attach(em, libvlc_MediaPlayerOpening, &event_cb, t);
            libvlc_event_attach(em, libvlc_MediaPlayerPlaying, &event_cb, t);
            libvlc_event_attach(em, libvlc_MediaPlayerEncounteredError, &event_cb, t);
            // Playback-state feedback only -- see kEventPlaybackStateChanged's own comment.
            libvlc_event_attach(em, libvlc_MediaPlayerPaused, &event_cb, t);
            libvlc_event_attach(em, libvlc_MediaPlayerStopped, &event_cb, t);
            libvlc_event_attach(em, libvlc_MediaPlayerEndReached, &event_cb, t);
        }

        libvlc_media_player_play(t->player);
    }

    void Resize(VlcTabHandle handle, int width, int height)
    {
        Tab* t = find(handle);
        if (!t) return;

        // width/height are already clamped to this slot's ceiling by the caller
        // (llmediaproducer.cpp's kResize handler), but clamp again here too -- t->pixels'
        // fixed size (see CreateTab()) depends on it absolutely, not just on every
        // caller remembering to. Width is additionally rounded up to kWidthAlignment --
        // see its own comment for why -- applied here, at request time, rather than
        // later when actually applying the resize, so a run of rapid requests that all
        // round to the same aligned width get recognized as duplicates by the dedupe
        // check just below, rather than each one resetting the settle timer.
        std::uint32_t aligned_width = clampAndAlignWidth(std::uint32_t(width), t->maxWidth);
        height = int(std::min(std::uint32_t(height), t->maxHeight));

        if (!t->player)
        {
            // kResize can legitimately arrive before Open() -- CEF always has a live
            // "about:blank" browser to resize immediately on slot creation, libvlc has
            // no equivalent placeholder. Just remember the size; Open() reads it when
            // it actually creates the player.
            t->width = aligned_width;
            t->height = std::uint32_t(height);
            t->pendingWidth = t->width;
            t->pendingHeight = t->height;
            t->hasPendingResize = false;
            return;
        }

        // Deliberately NOT calling libvlc_video_set_format() here -- only recording the
        // request. See MaybeApplyPendingResize() (polled once per tick from
        // CopyLatestFrame(), the same way this whole class polls everything else) for
        // why: a rapid sequence of resize requests (dragging a 2D floater's resize
        // handle fires many per second) previously called libvlc_video_set_format()
        // once per request, meaning libvlc's own decode/filter-chain rebuild was
        // continuously restarted mid-reconfiguration for the ENTIRE drag, never
        // settling on any one size -- the buffer content was in constant flux the whole
        // time, not just briefly mid-transition. Confirmed via real testing: severe,
        // non-recovering pixel corruption that persisted even long after the drag
        // stopped, which a purely output-side debounce (an earlier, incomplete fix)
        // couldn't touch, since it only delayed what got reported/copied out, not what
        // was actually being written into the buffer moment to moment. Coalescing the
        // request itself means libvlc is only ever asked to reconfigure once, to the
        // final settled size, giving it a real, uninterrupted chance to complete that
        // one reconfiguration before a frame at that size is ever read or published.
        if (aligned_width == t->pendingWidth && std::uint32_t(height) == t->pendingHeight)
        {
            return; // identical to the already-pending request -- don't reset the settle timer for no reason
        }
        t->pendingWidth = aligned_width;
        t->pendingHeight = std::uint32_t(height);
        t->hasPendingResize = (t->pendingWidth != t->width || t->pendingHeight != t->height);
        t->lastResizeRequestTime = std::chrono::steady_clock::now();
    }

    // Polled once per tick from CopyLatestFrame() -- actually applies a pending resize,
    // but only once it's gone unsuperseded for kResizeSettleDelay. See Resize()'s own
    // comment for why coalescing this way, rather than applying every request
    // immediately, matters regardless of the mechanism used to actually apply it.
    //
    // Applies via a full close+reopen (Open() again, at the new size) rather than
    // calling libvlc_video_set_format() on the already-running player. That
    // mid-stream-reconfigure approach was this project's original design specifically
    // to avoid a stop/restart on every resize (see the old media_plugin_libvlc.cpp
    // comment this replaces), and an early disposable test against screen:// seemed to
    // confirm it worked -- but that test only checked "does it crash or stop playing,"
    // never actual pixel-level correctness at the new size. Confirmed via extensive
    // real testing against a real RTSP stream: mid-stream libvlc_video_set_format()
    // produces severe, non-recovering pixel corruption at the new size, independent of
    // timing -- neither coalescing the requests, nor a settle delay before applying,
    // nor an additional grace period afterward before trusting a frame, made any
    // difference. Closing and reopening the SAME URL at the SAME (corrupted) size
    // always rendered correctly, proving this is inherent to reconfiguring an
    // already-running player, not a race we can wait out. A brief stop/restart
    // interruption is the accepted tradeoff for a resize actually working correctly.
    void MaybeApplyPendingResize(VlcTabHandle handle)
    {
        Tab* t = find(handle);
        if (!t || !t->hasPendingResize || !t->player) return;
        if (std::chrono::steady_clock::now() - t->lastResizeRequestTime < kResizeSettleDelay) return;

        t->width = t->pendingWidth;
        t->height = t->pendingHeight;
        t->hasPendingResize = false;
        // A COPY, not a reference to t->currentUrl -- Open() calls closePlayer()
        // internally, which clears t->currentUrl; passing the member directly would
        // alias it, so by the time Open() reads its own `url` parameter it's already
        // been cleared out from under it, and libvlc ends up opening an empty MRL.
        const std::string url = t->currentUrl;
        // Captured BEFORE Open()'s closePlayer() call releases t->player -- restored
        // just below, once the new player exists, so a resize doesn't restart a
        // seekable file (a real video, as opposed to a live RTSP/RTMP stream) from the
        // beginning every time. libvlc_media_player_set_time() is documented as a safe,
        // harmless no-op for a live/non-seekable source ("no effect if no media is
        // being played... not all formats and protocols support this"), so this is
        // unconditional rather than only for known-seekable backends.
        const libvlc_time_t resume_time_ms = libvlc_media_player_get_time(t->player);
        // force_reopen=true -- the same-URL guard in Open() exists for a DIFFERENT
        // case (a redundant kSetUrl resend for the same stream) and would otherwise
        // make this whole call a no-op, since url is (deliberately) the same URL the
        // tab is already playing.
        Open(handle, url, true); // closes the current player and reopens at the new t->width/height
        if (resume_time_ms >= 0 && t->player)
        {
            libvlc_media_player_set_time(t->player, resume_time_ms);
        }
        t->lastAppliedResizeTime = std::chrono::steady_clock::now();
    }

    void SetVolume(VlcTabHandle handle, int volume0to100)
    {
        Tab* t = find(handle);
        if (!t || !t->player) return;
        libvlc_audio_set_volume(t->player, volume0to100);
    }

    // Explicit set-pause rather than libvlc_media_player_pause() (whose own toggle
    // semantics have drifted across libvlc versions) -- read the actual current state
    // and flip it, so this is correct regardless of which libvlc build this links against.
    void TogglePlayPause(VlcTabHandle handle)
    {
        Tab* t = find(handle);
        if (!t || !t->player) return;
        const bool playing = libvlc_media_player_is_playing(t->player) != 0;
        libvlc_media_player_set_pause(t->player, playing ? 1 : 0);
    }

    // See this method's own comment in the header -- mirrors media_plugin_libvlc.cpp's
    // "start" message handling exactly, including the Ended-state special case.
    void Play(VlcTabHandle handle)
    {
        Tab* t = find(handle);
        if (!t || !t->player) return;
        if (libvlc_media_player_get_state(t->player) == libvlc_Ended &&
            !libvlc_media_player_is_playing(t->player))
        {
            libvlc_media_player_stop(t->player);
        }
        libvlc_media_player_play(t->player);
    }

    void Pause(VlcTabHandle handle)
    {
        Tab* t = find(handle);
        if (!t || !t->player) return;
        libvlc_media_player_set_pause(t->player, 1);
    }

    void Stop(VlcTabHandle handle)
    {
        Tab* t = find(handle);
        if (!t || !t->player) return;
        libvlc_media_player_stop(t->player);
    }

    bool CopyLatestFrame(VlcTabHandle handle, std::vector<std::uint8_t>& dst, int& w, int& h)
    {
        Tab* t = find(handle);
        if (!t) return false;

        // Must run BEFORE the frameDirty check below -- if it just reopened the player
        // (a real stop+recreate, see its own comment), there's no dirty frame yet.
        MaybeApplyPendingResize(handle);

        // See kPostApplyGraceDelay's own comment: a resize having been applied doesn't
        // mean libvlc's filter-chain rebuild has actually finished, so no frame is
        // trusted/published until some time has passed since that application -- even
        // though the coalescing above already guarantees libvlc is only ever asked to
        // reconfigure once per settled resize, not repeatedly.
        if (std::chrono::steady_clock::now() - t->lastAppliedResizeTime < kPostApplyGraceDelay)
        {
            return false;
        }

        std::lock_guard<std::mutex> lock(t->mutex);
        if (!t->frameDirty) return false;
        // Copy only the active width*height, not the whole (ceiling-sized, since the
        // fixed-buffer-overrun fix) t->pixels -- this copy runs under the same mutex
        // libvlc's own decode thread needs for lock_cb/unlock_cb, so copying the full
        // ceiling buffer (e.g. up to ~8 MiB at 1920x1080) here on every call, regardless
        // of how much smaller the tab's actual current resolution is, meaningfully grew
        // how long this lock is held -- a real contributor to a genuine
        // "buffer deadlock prevented" seen from libvlc during real playback testing.
        const std::size_t bytes = std::size_t(t->width) * std::size_t(t->height) * kBytesPerPixel;
        dst.assign(t->pixels.begin(), t->pixels.begin() + std::ptrdiff_t(bytes));
        w = int(t->width);
        h = int(t->height);
        t->frameDirty = false;
        return true;
    }

    bool ConsumeLoadStart(VlcTabHandle handle)
    {
        Tab* t = find(handle);
        if (!t) return false;
        return t->loadStartPending.exchange(false);
    }

    bool ConsumeLoadEnd(VlcTabHandle handle, int& pseudoHttpStatus)
    {
        Tab* t = find(handle);
        if (!t) return false;
        if (!t->loadEndPending.exchange(false)) return false;
        pseudoHttpStatus = t->loadEndStatus.load();
        return true;
    }

    bool ConsumePlaybackStateChange(VlcTabHandle handle, bool& out_playing)
    {
        Tab* t = find(handle);
        if (!t) return false;
        if (!t->playStateChangedPending.exchange(false)) return false;
        out_playing = t->playStateIsPlaying.load();
        return true;
    }

private:
    Tab* find(VlcTabHandle handle)
    {
        if (!handle.IsValid() || handle.index >= mTabs.size()) return nullptr;
        Tab& t = *mTabs[handle.index];
        if (!t.live || t.generation != handle.generation) return nullptr;
        return &t;
    }
    const Tab* find(VlcTabHandle handle) const
    {
        return const_cast<Impl*>(this)->find(handle);
    }

    // libvlc_media_player_stop() is documented as synchronous -- no lock/unlock
    // callback can still be in flight once it returns, so the stop/release sequence
    // itself needs no extra locking (matches the reference plugin's own resetVLC()).
    // The frameDirty reset at the end does still take t.mutex -- see its own comment.
    void closePlayer(Tab& t)
    {
        if (t.player)
        {
            libvlc_media_player_stop(t.player);
            libvlc_media_player_release(t.player);
            t.player = nullptr;
            t.currentUrl.clear();
        }
        t.loadStartPending = false;
        t.loadEndPending = false;
        t.playStateChangedPending = false;
        t.playStateIsPlaying = false;
        // A frame already marked dirty from the closed player's last picture must not
        // get published under the NEW player's (possibly different, post-resize-reopen)
        // dimensions -- t.pixels itself is untouched here (still the old player's last
        // content), but nothing should trust it as "new" once there's no player using it.
        {
            std::lock_guard<std::mutex> lock(t.mutex);
            t.frameDirty = false;
        }
    }

    libvlc_instance_t* mLibVLC = nullptr;

    // unique_ptr elements, not plain Tab values -- see Tab's own comment on why its
    // address must never move. Never shrinks; a freed slot is marked !live and reused
    // by a later CreateTab() via the free-index scan above, the same index-reuse
    // pattern llCefBrowserHandle-style handles already use elsewhere in this project.
    std::vector<std::unique_ptr<Tab>> mTabs;
};

LibVlcTabManager::LibVlcTabManager(const std::string& log_file_path)
    : mImpl(std::make_unique<Impl>(log_file_path))
{
}
LibVlcTabManager::~LibVlcTabManager() = default;

VlcTabHandle LibVlcTabManager::CreateTab(int width, int height, int maxWidth, int maxHeight) { return mImpl->CreateTab(width, height, maxWidth, maxHeight); }
void LibVlcTabManager::DestroyTab(VlcTabHandle handle) { mImpl->DestroyTab(handle); }
void LibVlcTabManager::DestroyAll() { mImpl->DestroyAll(); }
bool LibVlcTabManager::IsValid(VlcTabHandle handle) const { return mImpl->IsValid(handle); }
void LibVlcTabManager::Open(VlcTabHandle handle, const std::string& url) { mImpl->Open(handle, url); }
void LibVlcTabManager::Resize(VlcTabHandle handle, int width, int height) { mImpl->Resize(handle, width, height); }
void LibVlcTabManager::SetVolume(VlcTabHandle handle, int volume0to100) { mImpl->SetVolume(handle, volume0to100); }
void LibVlcTabManager::TogglePlayPause(VlcTabHandle handle) { mImpl->TogglePlayPause(handle); }
void LibVlcTabManager::Play(VlcTabHandle handle) { mImpl->Play(handle); }
void LibVlcTabManager::Pause(VlcTabHandle handle) { mImpl->Pause(handle); }
void LibVlcTabManager::Stop(VlcTabHandle handle) { mImpl->Stop(handle); }
bool LibVlcTabManager::CopyLatestFrame(VlcTabHandle handle, std::vector<std::uint8_t>& dst, int& w, int& h)
{
    return mImpl->CopyLatestFrame(handle, dst, w, h);
}
bool LibVlcTabManager::ConsumeLoadStart(VlcTabHandle handle) { return mImpl->ConsumeLoadStart(handle); }
bool LibVlcTabManager::ConsumeLoadEnd(VlcTabHandle handle, int& pseudoHttpStatus)
{
    return mImpl->ConsumeLoadEnd(handle, pseudoHttpStatus);
}
bool LibVlcTabManager::ConsumePlaybackStateChange(VlcTabHandle handle, bool& out_playing)
{
    return mImpl->ConsumePlaybackStateChange(handle, out_playing);
}

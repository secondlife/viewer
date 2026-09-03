/**
 *
 * @file cefshm_protocol.h
 * @brief Application-level protocol shared with cefshm_producer, including the control channel
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
// src/cefshm_protocol.h, not a shared include -- this PoC talks to an
// unmodified cefshm_producer.exe over the same wire protocol, so this file
// must stay in lockstep with that repo's copy by hand.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace cefshm_demo
{
    inline constexpr char          kChannelPrefix[] = "llcefshm_view_";
    inline constexpr std::uint32_t kDefaultWidth  = 960;
    inline constexpr std::uint32_t kDefaultHeight = 540;

    // Always-on, cheap (1x1 frame geometry -- it never publishes a frame,
    // only exchanges commands) channel a consumer uses to ask the producer
    // for one of the real per-view channels above, which the producer only
    // creates (a real CEF browser instance, plus its llshmframe segment)
    // once actually requested.
    inline constexpr char kControlChannelName[] = "llcefshm_control";

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
        kKeyEvent    = 9, // data = {uint32 msg, uint32 wParam, uint32 lParam} -- a raw Win32
                          // keyboard message triple, straight from LLWindowWin32::getNativeKeyData()
                          // on the consumer side, straight into llCefBrowserManager::SendKeyEvent()
                          // on the producer side. Windows-only, matching SendKeyEvent itself.
        kSetFocus    = 17, // data = {uint8 focus} -- straight into llCefBrowserManager::SetFocus();
                          // drives caret blink and focus/blur page JS, independent of key/mouse events
        kExecuteJavaScript = 21, // text payload: JS source, straight into
                          // llCefBrowserManager::ExecuteJavaScript() -- fire-and-forget, no result
                          // is returned (matching LLPluginClassMedia::executeJavaScript() itself)
        kSetPageZoom = 24, // data = {float32 zoomFactor} -- a plain 1.0-centered scale factor,
                          // straight into llCefBrowserManager::SetPageZoom(). Mirrors the legacy
                          // plugin's set_page_zoom_factor: zooms the page's rendered content
                          // without changing the pixel buffer size (see LLMediaCtrl::reshape()'s
                          // embedded-browser special case, which keeps the requested width/height
                          // unscaled and routes LLUI::getScaleFactor() through here instead).
        kCut   = 27, // empty payload -- straight into llCefBrowserManager::Cut(). Fire-and-forget,
                          // matching kExecuteJavaScript; there's no completion/result to report.
        kCopy  = 28, // empty payload -- straight into llCefBrowserManager::Copy().
        kPaste = 29, // empty payload -- straight into llCefBrowserManager::Paste().
        kSetMuted = 30, // data = {uint8 muted} -- straight into llCefBrowserManager::SetAudioMuted().
                          // Binary on/off only: CEF's public API has no continuous per-browser
                          // volume level (audio mixing happens inside Chromium's own audio
                          // service), so this can't replicate the legacy plugin's smooth
                          // distance-rolloff curve -- see LLViewerMediaImpl::updateVolume(),
                          // which collapses that same computed volume to a mute/unmute decision
                          // for embedded-browser media instead of a graded multiplier. Also
                          // honoured for a LibVLC-backed slot (maps to volume 0/100 -- see
                          // kSetVolume), so teardown-time silencing works identically for both
                          // backends regardless of which opcode a given call site uses.
        kGoBack    = 31, // empty payload -- straight into llCefBrowserManager::GoBack().
        kGoForward = 32, // empty payload -- straight into llCefBrowserManager::GoForward().
        kStopLoad  = 33, // empty payload -- straight into llCefBrowserManager::StopLoad().
        kReload    = 36, // data = {uint8 ignoreCache} -- straight into llCefBrowserManager::Reload().
                          // ignoreCache true matches the legacy plugin's own browse_reload(true)
                          // (a hard refresh, bypassing HTTP cache), which every reload call site
                          // in the Viewer already requests.
        kSetVolume = 37, // data = {uint8 volume} -- 0-100, matching libvlc_audio_set_volume()'s own
                          // native range directly. ONE opcode, both backends (see kRequestSlot's
                          // backend byte): a LibVLC-backed slot calls libvlc_audio_set_volume() with
                          // the value as-is, giving it the real distance-rolloff curve kSetMuted
                          // above can't. A CEF-backed slot collapses it to CEF's existing binary
                          // capability (0 -> SetAudioMuted(true), >0 -> SetAudioMuted(false)) --
                          // does not replace kSetMuted, which remains the explicit "silence
                          // immediately" signal used at teardown, independent of slider position.
        kSetRenderRate = 35, // data = {uint32 targetFps, uint8 priorityTier, url bytes (remainder)}
                          // -- caps how often the producer calls SendExternalBeginFrame() for
                          // this handle (0 = unthrottled/full rate, the default). Distance/
                          // priority-based render throttling, the embedded-browser equivalent
                          // of the legacy plugin's own setPriority()/setLowPrioritySizeLimit()
                          // -- see LLViewerMediaImpl::setPriority()'s own EMBEDDED_BROWSER_FPS_*
                          // constants for the tiers this is actually driven from. Never sent as
                          // anything but 0/tier-0 for UI/parcel media -- see that same comment.
                          // priorityTier (0=Normal/High, 1=Low, 2=Slideshow, 3=Hidden) and url
                          // are for the producer's own console/log output only (see
                          // log_priority() in llcefproducer.cpp) -- purely diagnostic, nothing
                          // on the producer side branches on either.

        // consumer -> producer, control channel only
        kRequestSlot     = 5, // data = {uint8 isUI, uint32 maxWidth, uint32 maxHeight, uint8 backend}
                          // -- isUI selects which of the producer's two CefRequestContexts (and
                          // therefore which cookie store) the new browser is created in: true
                          // for 2D floater/UI media, false for in-world/prim media. See
                          // llCefBrowserManager::CreateBrowser()'s own isUI parameter and
                          // kSetOpenIDCookie below. maxWidth/maxHeight are the consumer's own
                          // current ceiling (EmbeddedBrowserMaxWidth/Height) -- the producer
                          // clamps them to its own absolute maximum and sizes this slot's
                          // shared-memory segment to the result, rather than always reserving
                          // its absolute maximum for every slot regardless of what the consumer
                          // will ever actually request. A payload shorter than 9 bytes (the old,
                          // isUI-only format) falls back to the producer's own absolute maximum,
                          // for safety. backend (0=Cef, 1=LibVlc, appended as a 10th byte) picks
                          // which producer-side implementation renders this slot -- chosen once,
                          // consumer-side, from the URL's scheme (see
                          // LLViewerMediaImpl::createMediaSource()'s chooseEmbeddedBrowserBackend()),
                          // and fixed for the slot's whole lifetime: the producer must commit to a
                          // backend here, before it has ever seen a URL at all (kSetUrl is a later,
                          // separate command). A payload shorter than 10 bytes defaults to 0/Cef,
                          // for the same backward-compatibility reason as the 9-byte fallback above.
        kSetOpenIDCookie = 26, // data = {5x (uint32 len, bytes): url, name, value, domain, path;
                          // uint8 httpOnly; uint8 secure; uint8 alsoPrimContext} -- straight
                          // into llCefBrowserManager::SetCookie(), which always targets the UI
                          // context (see CreateBrowser's isUI) and mirrors into the prim
                          // context too if alsoPrimContext is set. This carries the Viewer's
                          // OpenID login cookie (see LLViewerMedia::getOpenIDCookieCoro()) --
                          // alsoPrimContext is that call site's own static policy switch for
                          // whether prim-hosted content should get it too.
        kShutdownProducer = 25, // empty payload -- asks the producer to exit its main loop and
                          // run its own graceful shutdown (llCefBrowserLib::Shutdown(), which
                          // flushes CEF's on-disk cookie/history/etc. stores) instead of being
                          // killed outright. See LLEmbeddedBrowser::reset(): a hard
                          // TerminateProcess() (what LLProcess::kill() does on Windows) gives CEF
                          // no chance to run any cleanup at all, which can lose cookies set only
                          // moments earlier -- this is sent first, with kill() as a fallback only
                          // if the producer doesn't exit on its own within a short grace period.

        // producer -> consumer, control channel only; reply_to = request id
        kSlotAssigned    = 6, // data = {uint32 slot index}
        kSlotUnavailable = 7, // empty payload -- no free slot right now

        // producer -> consumer, per-view channel -- mirrors a subset of
        // LLPluginClassMediaOwner::EMediaEvent (see the viewer's
        // llpluginclassmediaowner.h), driven by llCefBrowserManager's own
        // SetOnLoadStart/LoadEnd/TitleChange/AddressChange/CursorChanged
        // callbacks. Not every plugin event has an equivalent here yet --
        // this is deliberately the subset needed for load-state and
        // title/location/cursor feedback, not full parity.
        kEventLoadStart      = 10, // empty payload
        kEventLoadEnd        = 11, // data = {uint32 httpStatusCode}
        kEventTitleChanged   = 12, // text payload: the new page title
        kEventAddressChanged = 13, // text payload: the new URL
        kEventCursorChanged  = 14, // data = {uint32 cursorType} -- an llCefCursorType value
                                   // (see llCefBrowserHandle.h), opaque to this protocol layer
        kEventClickLinkHref     = 15, // data = {uint32 urlLen, url bytes, target bytes (remainder)} --
                                       // a link wants to open in a new window/tab (target="_blank",
                                       // window.open(), etc.), see llCefBrowserManager::SetOnOpenPopupCallback
        kEventClickLinkNoFollow = 16, // data = {uint8 flags (bit0=userGesture, bit1=isRedirect), url bytes
                                       // (remainder)} -- navigation to a recognized custom URL scheme (e.g.
                                       // "secondlife://"), see llCefBrowserManager::SetOnCustomSchemeURLCallback
        kEventFileDialogRequest = 18, // data = {int64 dialogId, uint32 mode (an llCefFileDialogMode ordinal --
                                       // Open=0, OpenMultiple=1, OpenFolder=2, Save=3), defaultFilePath bytes
                                       // (remainder)} -- see llCefBrowserManager::SetOnFileDialogCallback.
                                       // title/acceptFilters aren't forwarded: nothing on the consumer side
                                       // uses them today (see llmediactrl.cpp's own filter-guessing-from-
                                       // filename logic for MEDIA_EVENT_FILE_DOWNLOAD).

        // consumer -> producer, per-view channel
        kFileDialogResponse = 19, // data = {int64 dialogId, uint32 count, count * (uint32 len, bytes)} --
                                   // the file(s) the user picked, echoing the dialogId from
                                   // kEventFileDialogRequest; empty count means canceled. See
                                   // llCefBrowserManager::RespondToFileDialog.

        // producer -> consumer, per-view channel
        kEventStatusTextChanged = 20, // text payload: the new status-bar text (e.g. a hovered
                                       // link's URL), see llCefBrowserManager::SetOnStatusMessageCallback
        kEventConsoleMessage = 22, // data = {int32 line, uint32 messageLen, message bytes, source bytes
                                    // (remainder)} -- a console.log/warn/error call from page JS, see
                                    // llCefBrowserManager::SetOnConsoleMessageCallback
        kEventVersionInfo = 23, // text payload: llCefBrowser's own version plus the CEF/Chromium
                                  // build it was built against, multi-line -- e.g.
                                  // "0.15 (9f3f886)\n  CEF: 150.0.11\n  Chromium: 150.0.7871.115" --
                                  // sent once per slot right after it's allocated, before any
                                  // frames. See llCefBrowserVersion.h on the producer side.
        kEventNavStateChanged = 34, // data = {uint8 canGoBack, uint8 canGoForward} -- llCefBrowserManager's
                                  // CanGoBack()/CanGoForward(), sampled and re-sent alongside
                                  // kEventLoadStart/kEventLoadEnd, since back/forward availability
                                  // only ever changes as a result of navigation.
        kEventLoadError = 38, // data = {uint32 errorCode, url bytes (remainder)} -- the main frame's
                                  // navigation failed, see llCefBrowserManager::SetOnLoadErrorCallback.
                                  // Distinct from kEventClickLinkNoFollow: that one only fires for a
                                  // small, hardcoded "custom scheme" set (effectively just
                                  // "secondlife://"), so a scheme CEF's own URL parser recognizes but
                                  // has no protocol handler for (e.g. "rtsp://") is NOT caught by it --
                                  // CEF just attempts (and fails) a normal navigation, landing here
                                  // instead, with its own built-in error page already rendered in the
                                  // browser. This is purely a signal for the consumer to react to (e.g.
                                  // recognizing a stream-only scheme in the failed URL and switching
                                  // this slot's backend via a fresh navigate) -- it does not suppress or
                                  // replace CEF's own error page.
    };

    inline std::uint32_t pack_i32x2(std::uint8_t* d, std::int32_t x, std::int32_t y)
    {
        auto put = [&](int off, std::int32_t v) {
            d[off + 0] = std::uint8_t(v);       d[off + 1] = std::uint8_t(v >> 8);
            d[off + 2] = std::uint8_t(v >> 16);  d[off + 3] = std::uint8_t(v >> 24);
        };
        put(0, x); put(4, y);
        return 8;
    }

    inline std::uint32_t pack_mouse_button(std::uint8_t* d, std::int32_t x, std::int32_t y,
                                           std::uint8_t button, std::uint8_t action,
                                           std::uint8_t click_count)
    {
        const std::uint32_t n = pack_i32x2(d, x, y);
        d[n + 0] = button;
        d[n + 1] = action;
        d[n + 2] = click_count;
        return n + 3;
    }

    inline std::uint32_t pack_size(std::uint8_t* d, std::uint32_t w, std::uint32_t h)
    {
        d[0]=std::uint8_t(w); d[1]=std::uint8_t(w>>8); d[2]=std::uint8_t(w>>16); d[3]=std::uint8_t(w>>24);
        d[4]=std::uint8_t(h); d[5]=std::uint8_t(h>>8); d[6]=std::uint8_t(h>>16); d[7]=std::uint8_t(h>>24);
        return 8;
    }

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

    inline std::uint32_t pack_f32(std::uint8_t* d, float v)
    {
        std::uint32_t bits;
        std::memcpy(&bits, &v, sizeof(bits));
        return pack_u32(d, bits);
    }

    inline bool unpack_f32(const std::uint8_t* d, std::size_t n, float& v)
    {
        std::uint32_t bits;
        if (!unpack_u32(d, n, bits)) return false;
        std::memcpy(&v, &bits, sizeof(v));
        return true;
    }

    inline std::uint32_t pack_scroll(std::uint8_t* d, std::int32_t x, std::int32_t y, std::int32_t deltaY)
    {
        const std::uint32_t n = pack_i32x2(d, x, y);
        d[n+0]=std::uint8_t(deltaY); d[n+1]=std::uint8_t(deltaY>>8);
        d[n+2]=std::uint8_t(deltaY>>16); d[n+3]=std::uint8_t(deltaY>>24);
        return n + 4;
    }

    // msg/wParam/lParam straight from LLWindowWin32::getNativeKeyData()'s "msg"/"w_param"/
    // "l_param" fields -- all three are stored there as U32 (see ll_sd_from_U32), even though
    // Win32's own WPARAM/LPARAM are wider on 64-bit Windows, so uint32 round-trips them exactly.
    inline std::uint32_t pack_key_event(std::uint8_t* d, std::uint32_t msg, std::uint32_t wParam, std::uint32_t lParam)
    {
        std::uint32_t n = pack_u32(d, msg);
        n += pack_u32(d + n, wParam);
        n += pack_u32(d + n, lParam);
        return n;
    }

    inline bool unpack_click_href(const std::uint8_t* d, std::size_t n,
                                  std::string& url, std::string& target)
    {
        std::uint32_t url_len;
        if (!unpack_u32(d, n, url_len) || n < 4 + std::size_t(url_len)) return false;
        url.assign(reinterpret_cast<const char*>(d + 4), url_len);
        target.assign(reinterpret_cast<const char*>(d + 4 + url_len), n - 4 - url_len);
        return true;
    }

    inline bool unpack_click_nofollow(const std::uint8_t* d, std::size_t n, std::string& url,
                                      bool& userGesture, bool& isRedirect)
    {
        if (n < 1) return false;
        userGesture = (d[0] & 1) != 0;
        isRedirect  = (d[0] & 2) != 0;
        url.assign(reinterpret_cast<const char*>(d + 1), n - 1);
        return true;
    }

    inline std::uint32_t pack_load_error(std::uint8_t* d, std::uint32_t errorCode, const std::string& failedUrl)
    {
        std::uint32_t n = pack_u32(d, errorCode);
        std::memcpy(d + n, failedUrl.data(), failedUrl.size());
        return n + std::uint32_t(failedUrl.size());
    }

    inline bool unpack_load_error(const std::uint8_t* d, std::size_t n, std::uint32_t& errorCode, std::string& failedUrl)
    {
        if (!unpack_u32(d, n, errorCode)) return false;
        failedUrl.assign(reinterpret_cast<const char*>(d + 4), n - 4);
        return true;
    }

    inline std::uint32_t pack_i64(std::uint8_t* d, std::int64_t v)
    {
        for (int i = 0; i < 8; ++i) d[i] = std::uint8_t(std::uint64_t(v) >> (8 * i));
        return 8;
    }

    inline bool unpack_i64(const std::uint8_t* d, std::size_t n, std::int64_t& v)
    {
        if (n < 8) return false;
        std::uint64_t u = 0;
        for (int i = 0; i < 8; ++i) u |= std::uint64_t(d[i]) << (8 * i);
        v = std::int64_t(u);
        return true;
    }

    inline bool unpack_file_dialog_request(const std::uint8_t* d, std::size_t n, std::int64_t& dialogId,
                                           std::uint32_t& mode, std::string& defaultFilePath)
    {
        if (n < 12 || !unpack_i64(d, n, dialogId) || !unpack_u32(d + 8, n - 8, mode)) return false;
        defaultFilePath.assign(reinterpret_cast<const char*>(d + 12), n - 12);
        return true;
    }

    inline std::uint32_t pack_file_dialog_response(std::uint8_t* d, std::int64_t dialogId,
                                                    const std::vector<std::string>& filePaths)
    {
        std::uint32_t n = pack_i64(d, dialogId);
        n += pack_u32(d + n, std::uint32_t(filePaths.size()));
        for (const auto& path : filePaths)
        {
            n += pack_u32(d + n, std::uint32_t(path.size()));
            std::memcpy(d + n, path.data(), path.size());
            n += std::uint32_t(path.size());
        }
        return n;
    }

    inline bool unpack_console_message(const std::uint8_t* d, std::size_t n, std::string& message,
                                       std::string& source, std::int32_t& line)
    {
        std::uint32_t line_u, msg_len;
        if (n < 8 || !unpack_u32(d, n, line_u) || !unpack_u32(d + 4, n - 4, msg_len) ||
            n < 8 + std::size_t(msg_len))
        {
            return false;
        }
        line = std::int32_t(line_u);
        message.assign(reinterpret_cast<const char*>(d + 8), msg_len);
        source.assign(reinterpret_cast<const char*>(d + 8 + msg_len), n - 8 - msg_len);
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
    // assigned before the n<9 early return, so that fallback still leaves it initialized.
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

    inline std::uint32_t pack_openid_cookie(std::uint8_t* d, const std::string& url, const std::string& name,
                                             const std::string& value, const std::string& domain,
                                             const std::string& path, bool httpOnly, bool secure,
                                             bool alsoPrimContext)
    {
        std::uint32_t n = 0;
        for (const std::string* s : {&url, &name, &value, &domain, &path})
        {
            n += pack_u32(d + n, std::uint32_t(s->size()));
            std::memcpy(d + n, s->data(), s->size());
            n += std::uint32_t(s->size());
        }
        d[n++] = httpOnly ? 1 : 0;
        d[n++] = secure ? 1 : 0;
        d[n++] = alsoPrimContext ? 1 : 0;
        return n;
    }

    inline bool unpack_openid_cookie(const std::uint8_t* d, std::size_t n, std::string& url, std::string& name,
                                      std::string& value, std::string& domain, std::string& path,
                                      bool& httpOnly, bool& secure, bool& alsoPrimContext)
    {
        std::size_t off = 0;
        for (std::string* s : {&url, &name, &value, &domain, &path})
        {
            std::uint32_t len;
            if (off + 4 > n || !unpack_u32(d + off, n - off, len)) return false;
            off += 4;
            if (off + len > n) return false;
            s->assign(reinterpret_cast<const char*>(d + off), len);
            off += len;
        }
        if (off + 3 > n) return false;
        httpOnly        = d[off] != 0;
        secure          = d[off + 1] != 0;
        alsoPrimContext = d[off + 2] != 0;
        return true;
    }
}

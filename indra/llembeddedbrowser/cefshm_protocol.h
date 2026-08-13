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
        kMouseButton = 3, // data = {int32 x, int32 y, uint8 button, uint8 action}
        kResize      = 4, // data = {uint32 width, uint32 height}
        kScrollWheel = 8, // data = {int32 x, int32 y, int32 deltaY} -- deltaY in CEF's own wheel-delta
                          // units (a multiple of ~30-120 per notch), see SendMouseWheelEvent
        kKeyEvent    = 9, // data = {uint32 msg, uint32 wParam, uint32 lParam} -- a raw Win32
                          // keyboard message triple, straight from LLWindowWin32::getNativeKeyData()
                          // on the consumer side, straight into llCefBrowserManager::SendKeyEvent()
                          // on the producer side. Windows-only, matching SendKeyEvent itself.

        // consumer -> producer, control channel only
        kRequestSlot     = 5, // empty payload

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
                                           std::uint8_t button, std::uint8_t action)
    {
        const std::uint32_t n = pack_i32x2(d, x, y);
        d[n + 0] = button;
        d[n + 1] = action;
        return n + 2;
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
}

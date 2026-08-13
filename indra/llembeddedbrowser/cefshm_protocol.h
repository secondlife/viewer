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
// must stay in lockstep with that repo's copy by hand. Only the pieces this
// consumer actually uses are kept (no mouse/keyboard packing yet -- see
// LLEmbeddedBrowserTab, which doesn't forward input in this first pass).
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

        // consumer -> producer, control channel only
        kRequestSlot     = 5, // empty payload

        // producer -> consumer, control channel only; reply_to = request id
        kSlotAssigned    = 6, // data = {uint32 slot index}
        kSlotUnavailable = 7, // empty payload -- no free slot right now
    };

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
}

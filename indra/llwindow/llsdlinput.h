/**
 * @file llsdlinput.h
 * @brief Service SDL input while waiting for a frame deadline.
 *
 * Copyright (c) 2026, The Phoenix Firestorm Project, Inc.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef LL_LLSDLINPUT_H
#define LL_LLSDLINPUT_H

#include <SDL3/SDL_events.h>

#include <algorithm>
#include <chrono>

namespace LLSDL
{
// Main thread only. Dispatch through the window's normal event handler; do not
// invoke the SDL application iteration callback recursively. In particular, X11
// input methods can need several native event exchanges before a key is ready.
template<typename Dispatch, typename KeepWaiting>
void waitForInput(std::chrono::steady_clock::duration duration,
                  Dispatch dispatch, KeepWaiting keep_waiting)
{
    using Clock = std::chrono::steady_clock;
    using Milliseconds = std::chrono::milliseconds;
    constexpr Milliseconds slice(2);
    const auto deadline = Clock::now() + duration;

    while (keep_waiting())
    {
        const auto remaining = deadline - Clock::now();
        if (remaining <= Clock::duration::zero())
        {
            break;
        }

        SDL_Event event;
        // Wait for native input instead of spinning, with a short timeout so
        // input-method progress and quit requests do not depend on the FPS cap.
        const auto timeout = std::min(slice, std::chrono::ceil<Milliseconds>(remaining));
        if (SDL_WaitEventTimeout(&event, static_cast<int>(timeout.count())))
        {
            dispatch(event);
        }

        // Drain a burst, but do not let a continuous event stream extend the
        // frame's deadline indefinitely. The outer loop rechecks shutdown too.
        const auto drain_deadline = std::min(deadline, Clock::now() + slice);
        while (keep_waiting() && Clock::now() < drain_deadline && SDL_PollEvent(&event))
        {
            dispatch(event);
        }
    }
}
}

#endif // LL_LLSDLINPUT_H

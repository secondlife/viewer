/**
 * @file llgamecontrol.h
 * @brief GameController detection and management
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
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

#include <vector>


#include "llerror.h"
#include "llsingleton.h"
#include "stdtypes.h"


// LLGameControl is a singleton with pure static public interface
class LLGameControl : public LLSingleton<LLGameControl>
{
	LLSINGLETON_EMPTY_CTOR(LLGameControl);
	virtual ~LLGameControl();
    LOG_CLASS(LLGameControl);

public:
    // State is a minimal class for storing axes and buttons values
    class State
    {
    public:
        State();
        bool onButton(U8 button, bool pressed);
        std::vector<S16> mAxes; // [ -32768, 32767 ]
        std::vector<S16> mPrevAxes; // value in last outgoing packet
        U32 mButtons;
    };

    static bool isInitialized();
	static void init();
	static void terminate();

    static void addKeyButtonMap(U16 key, U8 button);
    static void removeKeyButtonMap(U16 key);
    static void addKeyAxisMap(U16 key, U8 axis, bool positive);
    static void removeKeyAxisMap(U16 key);

    static void onKeyDown(U16 key, U32 mask);
    static void onKeyUp(U16 key, U32 mask);

    // returns 'true' if GameControlInput message needs to go out,
    // which will be the case for new data or resend. Call this right
    // before deciding to put a GameControlInput packet on the wire
    // or not.
    static bool computeFinalInputAndCheckForChanges();

    static void clearAllInput();
    static void clearAllKeys();

    static void processEvents(bool app_has_focus = true);
    static const State& getState();

    static void setIncludeKeyboardButtons(bool include);
    static bool getIncludeKeyboardButtons();

    // call this after putting a GameControlInput packet on the wire
    static void updateResendPeriod();
};


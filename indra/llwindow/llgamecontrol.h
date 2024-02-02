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
    enum KeyboardAxis
    {
        AXIS_LEFTX_NEG = 0,
        AXIS_LEFTX_POS,
        AXIS_LEFTY_NEG,
        AXIS_LEFTY_POS,
        AXIS_RIGHTX_NEG,
        AXIS_RIGHTX_POS,
        AXIS_RIGHTY_NEG,
        AXIS_RIGHTY_POS,
        AXIS_TRIGGERLEFT_NEG,
        AXIS_TRIGGERLEFT_POS,
        AXIS_TRIGGERRIGHT_NEG,
        AXIS_TRIGGERRIGHT_POS,
        AXIS_LAST
    };

    enum Button
    {
        BUTTON_A = 0,
        BUTTON_B,
        BUTTON_X,
        BUTTON_Y,
        BUTTON_BACK,
        BUTTON_GUIDE,
        BUTTON_START,
        BUTTON_LEFTSTICK,
        BUTTON_RIGHTSTICK,
        BUTTON_LEFTSHOULDER,
        BUTTON_RIGHTSHOULDER,
        BUTTON_DPAD_UP,
        BUTTON_DPAD_DOWN,
        BUTTON_DPAD_LEFT,
        BUTTON_DPAD_RIGHT,
        BUTTON_MISC1,
        BUTTON_PADDLE1,
        BUTTON_PADDLE2,
        BUTTON_PADDLE3,
        BUTTON_PADDLE4,
        BUTTON_TOUCHPAD,
        BUTTON_21,
        BUTTON_22,
        BUTTON_23,
        BUTTON_24,
        BUTTON_25,
        BUTTON_26,
        BUTTON_27,
        BUTTON_28,
        BUTTON_29,
        BUTTON_30,
        BUTTON_31
    };

    class InputChannel
    {
    public:
        enum Type
        {
            TYPE_AXIS,
            TYPE_BUTTON,
            TYPE_NONE
        };

        static void initChannelMap();

        InputChannel() {}
        InputChannel(Type type, U8 index) : mType(type), mIndex(index) {}

        // these methods for readability
        bool isAxis() const { return mType == TYPE_AXIS; }
        bool isButton() const { return mType == TYPE_BUTTON; }
        bool isNone() const { return mType == TYPE_NONE; }

        bool isNegAxis() const;

        std::string getLocalName() const; // AXIS_0-, AXIS_0+, BUTTON_0, etc
        std::string getRemoteName() const; // GAME_CONTROL_AXIS_LEFTX, GAME_CONTROL_BUTTON_A, etc

        Type mType { TYPE_NONE };
        U8 mIndex { 255 };
    };

    // State is a minimal class for storing axes and buttons values
    class State
    {
    public:
        State();
        void clear();
        bool onButton(U8 button, bool pressed);
        std::vector<S16> mAxes; // [ -32768, 32767 ]
        std::vector<S16> mPrevAxes; // value in last outgoing packet
        U32 mButtons;
    };

    static bool isInitialized();
	static void init();
	static void terminate();

    // returns 'true' if GameControlInput message needs to go out,
    // which will be the case for new data or resend. Call this right
    // before deciding to put a GameControlInput packet on the wire
    // or not.
    static bool computeFinalStateAndCheckForChanges();

    static void clearAllState();

    static void processEvents(bool app_has_focus = true);
    static const State& getState();

    // these methods for accepting input from keyboard
    static void setInterpretControlActionsAsGameControl(bool include);
    static bool getInterpretControlActionsAsGameControl();

    // "Action" refers to avatar motion actions (e.g. push_forward, slide_left, etc)
    // this is a roundabout way to convert keystrokes to GameControl input.
    static LLGameControl::InputChannel getChannelByName(const std::string& name);
    static LLGameControl::InputChannel getChannelByActionName(const std::string& name);
    static void addActionMapping(const std::string& name,  LLGameControl::InputChannel channel);


    static U32 computeInternalActionFlags();
    static void setExternalActionFlags(U32 action_flags);

    // Keyboard presses produce action_flags which can be translated into State
    // and game_control devices produce State which can be translated into action_flags.
    // This method exchanges the two varieties of action flags in one call
    // so the twain do not touch.
    //static U32 exchangeActionFlags(U32 action_flags);
    //static void getActionFlags();                 // LLGameControl --> LLAgent
    //static void setActionFlags(U32 action_flags); // LLAgent --> LLGameControl

    // call this after putting a GameControlInput packet on the wire
    static void updateResendPeriod();
};


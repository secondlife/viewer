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
#include "SDL2/SDL_events.h"

// For reference, here are the RAW indices of the various input channels
// of a standard XBox controller.  Button (N) is numbered in parentheses,
// whereas axisN has N+ and N- labels.
//
//                 leftpaddle                          rightpaddle
//                 _______                               _______
//                /   4+  '-.                         .-'  5+   \.
// leftshoulder _(9)_________'-.____           ____.-'_________(10) rightshoulder
//             /  _________         \_________/                   \.
//            /  /    1-   \                               (3)     \.
//            | |           |     (4)   (5)   (6)           Y      |
//            | |0-  (7)  0+|               _________  (2)X   B(1) |
//            | |           |              /    3-   \      A      |
//            | |     1+    |             |           |    (0)     |
//            |  \_________/              |2-  (8)  2+|            |
//            |  leftstick     (11)       |           |            |
//            |             (13)  (14)    |     3+    |            |
//            |                (12)        \_________/             |
//            |               d-pad         rightstick             |
//            |                ____________________                |
//            |              /                      \              |
//            |             /                        \             |
//            |            /                          \            |
//             \__________/                            \__________/
//
// Note: the analog joysticks provide NEGATIVE X,Y values for LEFT,FORWARD
// whereas those directions are actually POSITIVE in SL's local right-handed
// reference frame.  This is why we implicitly negate those axes the moment
// they are extracted from SDL, before being used anywhere.  See the
// implementation in LLGameControllerManager::onAxis().

// LLGameControl is a singleton with pure static public interface
class LLGameControl : public LLSingleton<LLGameControl>
{
    LLSINGLETON_EMPTY_CTOR(LLGameControl);
    virtual ~LLGameControl();
    LOG_CLASS(LLGameControl);

public:
    enum AgentControlMode
    {
        CONTROL_MODE_AVATAR,
        CONTROL_MODE_FLYCAM,
        CONTROL_MODE_NONE
    };

    enum ActionNameType
    {
        ACTION_NAME_UNKNOWN,
        ACTION_NAME_ANALOG,     // E.g., "push"
        ACTION_NAME_ANALOG_POS, // E.g., "push+"
        ACTION_NAME_ANALOG_NEG, // E.g., "push-"
        ACTION_NAME_BINARY,     // E.g., "stop"
        ACTION_NAME_FLYCAM      // E.g., "zoom"
    };

    enum KeyboardAxis : U8
    {
        AXIS_LEFTX,
        AXIS_LEFTY,
        AXIS_RIGHTX,
        AXIS_RIGHTY,
        AXIS_TRIGGERLEFT,
        AXIS_TRIGGERRIGHT,
        NUM_AXES
    };

    enum Button
    {
        BUTTON_A,
        BUTTON_B,
        BUTTON_X,
        BUTTON_Y,
        BUTTON_BACK,
        BUTTON_GUIDE,
        BUTTON_START,
        BUTTON_LEFTSTICK,
        BUTTON_RIGHTSTICK,
        BUTTON_LEFTSHOULDER,
        BUTTON_RIGHTSHOULDER, // 10
        BUTTON_DPAD_UP,
        BUTTON_DPAD_DOWN,
        BUTTON_DPAD_LEFT,
        BUTTON_DPAD_RIGHT,
        BUTTON_MISC1,
        BUTTON_PADDLE1,
        BUTTON_PADDLE2,
        BUTTON_PADDLE3,
        BUTTON_PADDLE4,
        BUTTON_TOUCHPAD, // 20
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
        BUTTON_31,
        NUM_BUTTONS
    };

    static const U16 MAX_AXIS_DEAD_ZONE = 16384;
    static const U16 MAX_AXIS_OFFSET = 16384;

    class InputChannel
    {
    public:
        enum Type
        {
            TYPE_NONE,
            TYPE_AXIS,
            TYPE_BUTTON
        };

        InputChannel() {}
        InputChannel(Type type, U8 index) : mType(type), mIndex(index) {}
        InputChannel(Type type, U8 index, S32 sign) : mType(type), mSign(sign), mIndex(index) {}

        // these methods for readability
        bool isNone() const { return mType == TYPE_NONE; }
        bool isAxis() const { return mType == TYPE_AXIS; }
        bool isButton() const { return mType == TYPE_BUTTON; }

        bool isEqual(const InputChannel& other)
        {
            return mType == other.mType && mSign == other.mSign && mIndex == other.mIndex;
        }

        std::string getLocalName() const; // AXIS_0-, AXIS_0+, BUTTON_0, NONE etc.
        std::string getRemoteName() const; // GAME_CONTROL_AXIS_LEFTX, GAME_CONTROL_BUTTON_A, etc

        Type mType { TYPE_NONE };
        S32 mSign { 0 };
        U8 mIndex { 255 };
    };

    // Options is a data structure for storing device-specific settings
    class Options
    {
    public:
        struct AxisOptions
        {
            S32 mMultiplier = 1;
            U16 mDeadZone { 0 };
            S16 mOffset { 0 };

            void resetToDefaults()
            {
                mMultiplier = 1;
                mDeadZone = 0;
                mOffset = 0;
            }

            S16 computeModifiedValue(S16 raw_value) const
            {
                S32 new_value = ((S32)raw_value + S32(mOffset)) * mMultiplier;
                return (S16)(std::clamp(new_value, -32768, 32767));
            }

            std::string saveToString() const;
            void loadFromString(std::string options);
        };

        Options();

        void resetToDefaults();

        U8 mapAxis(U8 axis) const;
        U8 mapButton(U8 button) const;

        S16 fixAxisValue(U8 axis, S16 value) const;

        std::string saveToString(const std::string& name, bool force_empty = false) const;
        bool loadFromString(std::string& name, std::string options);
        bool loadFromString(std::string options);

        const std::vector<AxisOptions>& getAxisOptions() const { return mAxisOptions; }
        std::vector<AxisOptions>& getAxisOptions() { return mAxisOptions; }
        const std::vector<U8>& getAxisMap() const { return mAxisMap; }
        std::vector<U8>& getAxisMap() { return mAxisMap; }
        const std::vector<U8>& getButtonMap() const { return mButtonMap; }
        std::vector<U8>& getButtonMap() { return mButtonMap; }

    private:
        std::vector<AxisOptions> mAxisOptions;
        std::vector<U8> mAxisMap;
        std::vector<U8> mButtonMap;
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

    // Device is a data structure for describing any detected controller
    class Device
    {
        const int mJoystickID { -1 };
        const std::string mGUID;
        const std::string mName;
        Options mOptions;
        State mState;

    public:
        Device(int joystickID, const std::string& guid, const std::string& name);
        int getJoystickID() const { return mJoystickID; }
        std::string getGUID() const { return mGUID; }
        std::string getName() const { return mName; }
        const Options& getOptions() const { return mOptions; }
        const State& getState() const { return mState; }

        void resetOptionsToDefaults() { mOptions.resetToDefaults(); }
        std::string saveOptionsToString(bool force_empty = false) const { return mOptions.saveToString(mName, force_empty); }
        void loadOptionsFromString(const std::string& options) { mOptions.loadFromString(options); }

        friend class LLGameControllerManager;
    };

    static bool isEnabled();
    static void setEnabled(bool enabled);

    static bool isInitialized();
    static void init(const std::string& gamecontrollerdb_path,
        std::function<bool(const std::string&)> loadBoolean,
        std::function<void(const std::string&, bool)> saveBoolean,
        std::function<std::string(const std::string&)> loadString,
        std::function<void(const std::string&, const std::string&)> saveString,
        std::function<LLSD(const std::string&)> loadObject,
        std::function<void(const std::string&, const LLSD&)> saveObject,
        std::function<void()> updateUI);
    static void terminate();

    static const std::list<LLGameControl::Device>& getDevices();
    static const std::map<std::string, std::string>& getDeviceOptions();

    // returns 'true' if GameControlInput message needs to go out,
    // which will be the case for new data or resend. Call this right
    // before deciding to put a GameControlInput packet on the wire
    // or not.
    static bool computeFinalStateAndCheckForChanges();

    static void clearAllStates();

    static void processEvents(bool app_has_focus = true);
    static void handleEvent(const SDL_Event& event, bool app_has_focus);
    static const State& getState();
    static InputChannel getActiveInputChannel();
    static void getFlycamInputs(std::vector<F32>& inputs_out);

    // these methods for accepting input from keyboard
    static void setSendToServer(bool enable);
    static void setControlAgent(bool enable);
    static void setTranslateAgentActions(bool enable);
    static void setAgentControlMode(AgentControlMode mode);

    static bool getSendToServer();
    static bool getControlAgent();
    static bool getTranslateAgentActions();
    static AgentControlMode getAgentControlMode();
    static ActionNameType getActionNameType(const std::string& action);

    static bool willControlAvatar();

    // Given a name like "AXIS_1-" or "BUTTON_5" returns the corresponding InputChannel
    // If the axis name lacks the +/- postfix it assumes '+' postfix.
    static LLGameControl::InputChannel getChannelByName(const std::string& name);

    // action_name = push+, strafe-, etc
    static LLGameControl::InputChannel getChannelByAction(const std::string& action);

    static bool updateActionMap(const std::string& action_name,  LLGameControl::InputChannel channel);

    // Keyboard presses produce action_flags which can be translated into State
    // and game_control devices produce State which can be translated into action_flags.
    // These methods help exchange such translations.
    static U32 computeInternalActionFlags();
    static void setExternalInput(U32 action_flags, U32 buttons_from_keys);

    // call this after putting a GameControlInput packet on the wire
    static void updateResendPeriod();

    using getChannel_t = std::function<LLGameControl::InputChannel(const std::string& action)>;
    static std::string stringifyAnalogMappings(getChannel_t getChannel);
    static std::string stringifyBinaryMappings(getChannel_t getChannel);
    static std::string stringifyFlycamMappings(getChannel_t getChannel);
    static void getDefaultMappings(std::vector<std::pair<std::string, LLGameControl::InputChannel>>& mappings);

    static bool parseDeviceOptions(const std::string& options, std::string& name,
        std::vector<LLGameControl::Options::AxisOptions>& axis_options,
        std::vector<U8>& axis_map, std::vector<U8>& button_map);
    static std::string stringifyDeviceOptions(const std::string& name,
        const std::vector<LLGameControl::Options::AxisOptions>& axis_options,
        const std::vector<U8>& axis_map, const std::vector<U8>& button_map,
        bool force_empty = false);

    static void initByDefault();
    static void loadFromSettings();
    static void saveToSettings();
    static void setDeviceOptions(const std::string& guid, const Options& options);
};


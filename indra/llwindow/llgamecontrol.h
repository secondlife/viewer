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
#include "SDL3/SDL_events.h"

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
//            | |           |     (4)   (5)   (6)           N      |
//            | |0-  (7)  0+|               _________  (2)W   E(1) |
//            | |           |              /    3-   \      S    |
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

// Interface to get controller binding from assigned command
class LLGameControllerBindingToStringHandler
{
public:
    virtual std::string getBindingAsString(const std::string& control) const = 0;
    virtual bool hasHandlingDevice() const = 0;
};

// LLGameControl is a singleton with pure static public interface
class LLGameControl : public LLSingleton<LLGameControl>, public LLGameControllerBindingToStringHandler
{
    LLSINGLETON_EMPTY_CTOR(LLGameControl);
    virtual ~LLGameControl();
    LOG_CLASS(LLGameControl);

public:
    enum AgentControlMode
    {
        CONTROL_MODE_AVATAR,
        CONTROL_MODE_FLYCAM,
        CONTROL_MODE_CAPTIVE, // Avatar is sitting, or controls have been taken
        CONTROL_MODE_NONE
    };

    enum ActionType: U8
    {
        DOF,
        BUTTON,
        NONE = 255,
    };

    enum MovementDirection: U8
    {
        MOVE_DIR_STRAFE_LEFT,
        MOVE_DIR_STRAFE_RIGHT,
        MOVE_DIR_PUSH_FORWARD,
        MOVE_DIR_PUSH_BACKWARD,
        MOVE_DIR_TURN_LEFT,
        MOVE_DIR_TURN_RIGHT,
        MOVE_DIR_LOOK_UP,
        MOVE_DIR_LOOK_DOWN,
        MOVE_DIR_RISE_UP,
        MOVE_DIR_DROP_DOWN,
        MOVE_DIR_ROLL_LEFT,
        MOVE_DIR_ROLL_RIGHT,
        NUM_MOVE_DIRS,
    };

    enum ActionNameType
    {
        ACTION_NAME_UNKNOWN,
        ACTION_NAME_ANALOG,     // avatar movement, e.g. "push"
        ACTION_NAME_BINARY,     // maps to button, e.g. "stop"
        ACTION_NAME_FLYCAM      // E.g., "zoom"
    };

    enum KeyboardAxis : U8
    {
        AXIS_LEFTX,
        AXIS_LEFTY,
        AXIS_RIGHTX,
        AXIS_RIGHTY,
        AXIS_LEFT_TRIGGER,
        AXIS_RIGHT_TRIGGER,
        NUM_AXES
    };

    enum Button : U8
    {
        BUTTON_SOUTH,
        BUTTON_EAST,
        BUTTON_WEST,
        BUTTON_NORTH,
        BUTTON_SELECT, // BUTTON_BACK in SDL
        BUTTON_HOME,   // BUTTON_GUIDE in SDL
        BUTTON_START,
        BUTTON_LEFT_STICK,
        BUTTON_RIGHT_STICK,
        BUTTON_LEFT_SHOULDER,
        BUTTON_RIGHT_SHOULDER, // 10
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

    // Order in which flycam channel values are packed by getFlycamInputs() and
    // consumed by LLAgent::updateFlycam().
    enum FlycamChannel : U8
    {
        FLYCAM_TRUCK = 0, // strafe left/right
        FLYCAM_DOLLY,     // advance forward/back
        FLYCAM_PAN,       // yaw left/right
        FLYCAM_TILT,      // pitch up/down
        FLYCAM_BOOM,      // rise up/down
        FLYCAM_ROLL,
        FLYCAM_ZOOM,
        FLYCAM_NUM_CHANNELS
    };

    // Axis-map output codes stored in Options::mAxisMap (one per physical axis):
    //   0 .. NUM_AXES-1          maps 1:1 to that canonical axis
    //   AXIS_OUTPUT_NONE         physical axis is unmapped / disabled
    //   AXIS_OUTPUT_TRIGGER_PAIR physical axis fans out to the canonical trigger
    //                            pair: its negative half drives the LEFT_TRIGGER
    //                            channel, its positive half the RIGHT_TRIGGER channel.
    // The trigger pair is a virtual bidirectional axis (left = negative side,
    // right = positive side) shared with the Actions-tab "Triggers left/right" input.
    static constexpr U8 AXIS_OUTPUT_NONE = NUM_AXES;
    static constexpr U8 AXIS_OUTPUT_TRIGGER_PAIR = NUM_AXES + 1;
    static constexpr U8 NUM_AXIS_OUTPUTS = NUM_AXES + 2;

    static const U16 MAX_AXIS_DEAD_ZONE = 16384;
    static const U16 MAX_AXIS_OFFSET = 16384;

    // Default dead zone applied to a freshly-constructed / reset axis
    // is nonzero because most controllers have a bit of hysteresis or offset.
    static constexpr U16 DEFAULT_DEAD_ZONE = 1024;

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

        std::string getLocalName() const; // AXIS_0, AXIS_1, BUTTON_0, NONE etc.
        std::string getSignedLocalName() const; // AXIS_0-, AXIS_0+, BUTTON_0, NONE etc.
        std::string getRemoteName() const; // AXIS_LEFTX, BUTTON_SOUTH, etc

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
            U16 mDeadZone { DEFAULT_DEAD_ZONE };
            S16 mOffset { 0 };

            void resetToDefaults();

            S16 computeModifiedValue(S16 raw_value) const;

            std::string saveToString() const;
            void loadFromString(std::string options);
        };

        Options();

        void resetToDefaults();

        U8 mapAxis(U8 axis) const;
        U8 mapButton(U8 button) const;

        U8 unmapAxis(U8 axis) const;
        U8 unmapButton(U8 button) const;

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
        void storePrevious();
        bool onButton(U8 button, bool pressed);
        std::vector<U16> mAxes; // [ 0 , 32767 ] post-fix (dead zone/offset/invert applied), split into +/- half-axes
        std::vector<U16> mRawAxes; // [ 0 , 32767 ] pre-fix values, same +/- half-axis layout as mAxes

        // Pre-map (physical-axis-indexed) copies of the per-axis values, kept only for
        // the preferences Device-State tab.  onAxis() re-keys mAxes/mRawAxes by canonical
        // output axis (mapAxis) for the input pipeline, discarding the physical index; the
        // tab's rows are physical-axis-indexed (their Invert/Offset/Dead-Zone options are),
        // so it reads these to show Raw/Adjusted on the matching row.  Signed [ -32768,
        // 32767 ], one entry per physical axis (NUM_AXES), not split into half-axes.
        std::vector<S16> mPhysicalRawAxes;   // pre-fix, indexed by physical axis
        std::vector<S16> mPhysicalFixedAxes; // post-fix, indexed by physical axis

        U32 mButtons;

        // Pre-map button state, keyed by physical button index (same reason as the
        // mPhysical*Axes above): onButton() re-keys mButtons by canonical button
        // (mapButton) for the input pipeline, and the Device-State tab's rows are
        // physical buttons, so it reads this to show each row's own pressed state.
        U32 mPhysicalButtons;

        std::vector<U16> mPrevAxes;
        U32 mPrevButtons;
    };

    class ServerState
    {
    public:
        ServerState();
        void clear();
        std::vector<S16> mAxes; // [ -32768, 32767 ]
        std::vector<S16> mPrevAxes; // value in last outgoing packet
        U32 mButtons;
        U32 mPrevButtons;
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

    static bool actionFromString(const std::string& string, ActionType& actionType, U8& action);
    static std::string stringFromAction(const ActionType actionType, U8 action);
    static std::string controllerInputStringFromAction(const ActionType actionType, U8 action);

    static F32 getControllerHeldTime(ActionType actionType, U8 action);
    static S32 getControllerHeldFrames(ActionType actionType, U8 action);

    static bool isInitialized();

    // Bulk settings I/O is delegated to the host so this library does not
    // need to know about LLControlGroup at compile time.
    //   loadSettings(keys)        -- returns an LLSD map of the requested keys.
    //                                Keys absent from the host's store are
    //                                omitted from the returned map.
    //   saveSettings(key_values)  -- writes every entry of the LLSD map.
    using LoadSettingsFn = std::function<LLSD(const std::vector<std::string>&)>;
    using SaveSettingsFn = std::function<void(const LLSD&)>;

    static void init(const std::string& gamecontrollerdb_path,
        LoadSettingsFn loadSettings,
        SaveSettingsFn saveSettings,
        std::function<void()> updateUI);
    static void terminate();

    static const std::list<LLGameControl::Device>& getDevices();
    static const std::map<std::string, std::string>& getDeviceOptions();

    // returns 'true' if GameControlInput message needs to go out,
    // which will be the case for new data or resend. Call this right
    // before deciding to put a GameControlInput packet on the wire
    // or not.
    static bool computeFinalStateAndCheckForChanges();
    static void computeFinalState();

    static void clearAllStates();

    static void processEvents(bool app_has_focus = true);
    static void handleEvent(const SDL_Event& event, bool app_has_focus);
    static const ServerState& getServerState();
    static const State& getState();
    static InputChannel getActiveInputChannel();
    static void getFlycamInputs(std::vector<F32>& inputs_out);

    // these methods for accepting input from keyboard
    static void setSendToServer(bool enable);
    static void setAgentControlMode(AgentControlMode mode);

    // Master runtime on/off for the whole feature: gates all game-control ->
    // action logic (via isEnabled()) and the sending of GameControlData.
    static void setGameControlEnabled(bool enabled);
    static bool getGameControlEnabled();

    static bool sendToServer();
    static AgentControlMode getAgentControlMode();

    // Runtime gating for the local-control path.  "Enabled" now means simply
    // "a controller is connected" -- the feature no longer has a separate on/off
    // toggle (see the "GameControl always enabled" change).  willControlAvatar()
    // and willControlFlycam() additionally gate on the active AgentControlMode.
    static bool isEnabled();        // a controller is connected
    static bool willControlAvatar();// enabled AND mode is Avatar/Captive
    static bool willControlFlycam();// enabled AND mode is FlyCam
    static ActionNameType getActionNameType(const std::string& action);

    // Given a name like "AXIS_1-" or "BUTTON_5" returns the corresponding InputChannel
    // If the axis name lacks the +/- postfix it assumes '+' postfix.
    static LLGameControl::InputChannel getChannelByName(const std::string& name);

    // Translate between an axis-map output code and its symbolic name, as used by the
    // Devices-tab output selector: "AXIS_LEFTX".."AXIS_RIGHT_TRIGGER" for the canonical
    // axes, "AXIS_TRIGGERS" for the fan-out pair, and "AXIS_NONE"/unrecognized for None.
    static U8 axisOutputFromName(const std::string& name);
    static std::string axisOutputName(U8 code);

    // Actions bound to a button/axis that don't correspond to an AGENT_CONTROL_*
    // bit (e.g. they toggle viewer-side state rather than a movement flag).
    // These are plain small integers, not bitmasks, so they cannot be OR'd into
    // a flags word; they are carried separately in AgentActions::mMiscActions.
    static constexpr U32 ACTION_TOGGLE_FLY = 33;
    static constexpr U32 ACTION_TOGGLE_SIT = 34;
    static constexpr U32 ACTION_TOGGLE_SPEAK = 35;
    static constexpr U32 ACTION_TOGGLE_FLYCAM = 36;
    static constexpr U32 ACTION_TOGGLE_MOUSELOOK = 37;
    static constexpr U32 ACTION_TOGGLE_3RD_PERSON = 38;

    // Result of translating controller/keyboard State into agent actions:
    // mControlFlags holds the AGENT_CONTROL_* bits (OR-combinable), mMiscActions
    // holds any ACTION_TOGGLE_* (or future non-flag) actions in the order they
    // were detected, and mIsRunning is the final walk/run decision -- true when
    // either the "Toggle run" button is currently engaged or the movement axes
    // are pushed hard enough (see LLGameControllerManager::computeAgentActions()).
    struct AgentActions
    {
        std::vector<U32> mMiscActions;
        U32 mControlFlags { 0 };
        bool mIsRunning { false };
    };

    // Keyboard presses produce action_flags which can be translated into State
    // and game_control devices produce State which can be translated into action_flags.
    // These methods help exchange such translations.
    static AgentActions computeAgentActions();

    // is_running mirrors gAgent::getRunning() (the same walk/run state
    // computeAgentActions()/RUN_ENGAGE_FRACTION derive from an analog controller
    // axis): translated movement axes are set to full deflection when running,
    // half deflection when walking -- approximating the walk/run threshold as a
    // 50% stick tilt, per LLGameControllerManager::computeAgentActions().
    static void setExternalInput(U32 action_flags, U32 buttons_from_keys, bool is_running);

    // call this after putting a GameControlInput packet on the wire
    static void updateResendPeriod();

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

    // Settings serialization helpers used by loadFromSettings/saveToSettings
    // and exposed for callers that need to snapshot or restore module state
    // (e.g. preference panel cancel-restore).
    //   getSettingKeys()                 -- the host setting names this module reads/writes.
    //   getSettingsAsLLSD()              -- snapshot of current in-memory state, keyed by the same names.
    //   applySettingsFromLLSD(settings)  -- apply 'settings' to in-memory state.
    //                                       Keys not present in 'settings' are left unchanged.
    static const std::vector<std::string>& getSettingKeys();
    static LLSD getSettingsAsLLSD();
    static void applySettingsFromLLSD(const LLSD& settings);

    // GameControl settings, stored under the single "GameControl" setting key:
    //   ModeMappings/<Mode>/Axes|Buttons  -- GLOBAL action -> canonical-input maps
    //   Devices/<guid>/Config             -- per-device serialized hardware options
    // 'mode' is "Avatar"/"FlyCam"/"Captive"; 'kind' is "Axes"/"Buttons".
    static LLSD getDefaultModeMappings();        // { <Mode> : { Axes, Buttons } }
    static LLSD getDefaultGameControlSettings(); // full default GameControl map
    static const LLSD& getGameControlSettings();
    static void setGameControlSettings(const LLSD& settings);

    // Global per-mode action mapping accessors.  getModeMapping falls back to the
    // built-in defaults when an entry is missing.
    static LLSD getModeMapping(const std::string& mode, const std::string& kind);
    static void setModeMapping(const std::string& mode, const std::string& kind,
        const LLSD& mapping);
    static void updateModeMapping(const std::string& mode, const std::string& kind,
        const std::string& action, const std::string& input);

    // Per-mode enable flag: when false, game-control input is not converted to the
    // mode's actions and its mappings are treated as locked.  Defaults to true when
    // the flag is absent.  'mode' is "Avatar"/"FlyCam"/"Captive".
    static bool isModeEnabled(const std::string& mode);
    static void setModeEnabled(const std::string& mode, bool enabled);
    static std::string getDeviceConfig(const std::string& guid);
    static void setDeviceConfig(const std::string& guid, const std::string& config);

    // "Avatar"/"FlyCam"/"Captive" for the given mode (empty for CONTROL_MODE_NONE).
    static std::string getModeName(AgentControlMode mode);

    static void setDeviceOptions(const std::string& guid, const Options& options);

    // inherited from LLGameControllerBindingToStringHandler
    virtual std::string getBindingAsString(const std::string& control) const override;
    virtual bool hasHandlingDevice() const override;
};

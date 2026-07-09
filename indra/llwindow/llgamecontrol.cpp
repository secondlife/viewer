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

#include "llgamecontrol.h"

#include <algorithm>
#include <chrono>
#include <unordered_map>

#include "SDL3/SDL.h"
#include "SDL3/SDL_gamepad.h"
#include "SDL3/SDL_joystick.h"

#include "indra_constants.h"
#include "llfile.h"
#include "llgamecontroltranslator.h"
#include "llsd.h"
#include "llsdl.h"

namespace std
{
    string to_string(const SDL_GUID& guid)
    {
        char buffer[33] = { 0 };
        SDL_GUIDToString(guid, buffer, sizeof(guid));
        return buffer;
    }

    string to_string(SDL_JoystickType type)
    {
        switch (type)
        {
        case SDL_JOYSTICK_TYPE_GAMEPAD:
            return "GAMECONTROLLER";
        case SDL_JOYSTICK_TYPE_WHEEL:
            return "WHEEL";
        case SDL_JOYSTICK_TYPE_ARCADE_STICK:
            return "ARCADE_STICK";
        case SDL_JOYSTICK_TYPE_FLIGHT_STICK:
            return "FLIGHT_STICK";
        case SDL_JOYSTICK_TYPE_DANCE_PAD:
            return "DANCE_PAD";
        case SDL_JOYSTICK_TYPE_GUITAR:
            return "GUITAR";
        case SDL_JOYSTICK_TYPE_DRUM_KIT:
            return "DRUM_KIT";
        case SDL_JOYSTICK_TYPE_ARCADE_PAD:
            return "ARCADE_PAD";
        case SDL_JOYSTICK_TYPE_THROTTLE:
            return "THROTTLE";
        default:;
        }
        return "UNKNOWN";
    }

    string to_string(SDL_GamepadType type)
    {
        switch (type)
        {
        case SDL_GAMEPAD_TYPE_STANDARD:
            return "STANDARD";
        case SDL_GAMEPAD_TYPE_XBOX360:
            return "XBOX360";
        case SDL_GAMEPAD_TYPE_XBOXONE:
            return "XBOXONE";
        case SDL_GAMEPAD_TYPE_PS3:
            return "PS3";
        case SDL_GAMEPAD_TYPE_PS4:
            return "PS4";
        case SDL_GAMEPAD_TYPE_PS5:
            return "PS5";
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
            return "NINTENDO_SWITCH_PRO";
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
            return "NINTENDO_SWITCH_JOYCON_LEFT";
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
            return "NINTENDO_SWITCH_JOYCON_RIGHT";
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
            return "NINTENDO_SWITCH_JOYCON_PAIR";
        default:;
        }
        return "UNKNOWN";
    }

    string to_string(SDL_GamepadButton button)
    {
        return SDL_GetGamepadStringForButton(button);
    }

    string to_string(SDL_GamepadAxis axis)
    {
        return SDL_GetGamepadStringForAxis(axis);
    }

    string to_string(SDL_GamepadButtonLabel label)
    {
        switch (label)
        {
        case SDL_GAMEPAD_BUTTON_LABEL_A:
            return "a";
        case SDL_GAMEPAD_BUTTON_LABEL_B:
            return "b";
        case SDL_GAMEPAD_BUTTON_LABEL_X:
            return "x";
        case SDL_GAMEPAD_BUTTON_LABEL_Y:
            return "y";
        case SDL_GAMEPAD_BUTTON_LABEL_CROSS:
            return "cross";
        case SDL_GAMEPAD_BUTTON_LABEL_CIRCLE:
            return "circle";
        case SDL_GAMEPAD_BUTTON_LABEL_SQUARE:
            return "square";
        case SDL_GAMEPAD_BUTTON_LABEL_TRIANGLE:
            return "triangle";
        default:
            return "UNKOWN";
        }
    }
}

// Util for dumping SDL_JoystickGUID info
std::ostream& operator<<(std::ostream& out, SDL_GUID& guid)
{
    return out << std::to_string(guid);
}

// Util for dumping SDL_JoystickType type name
std::ostream& operator<<(std::ostream& out, SDL_JoystickType type)
{
    return out << std::to_string(type);
}

// Util for dumping SDL_GameControllerType type name
std::ostream& operator<<(std::ostream& out, SDL_GamepadType type)
{
    return out << std::to_string(type);
}

namespace std
{
    string to_string(SDL_Joystick* joystick)
    {
        if (!joystick)
        {
            return "nullptr";
        }

        std::stringstream ss;

        ss << "{id:" << SDL_GetJoystickID(joystick);
        SDL_GUID guid = SDL_GetJoystickGUID(joystick);
        ss << ",guid:'" << guid << "'";
        ss << ",type:'" << SDL_GetJoystickType(joystick) << "'";
        ss << ",name:'" << ll_safe_string(SDL_GetJoystickName(joystick)) << "'";
        ss << ",vendor:" << SDL_GetJoystickVendor(joystick);
        ss << ",product:" << SDL_GetJoystickProduct(joystick);
        if (U16 version = SDL_GetJoystickProductVersion(joystick))
        {
            ss << ",version:" << version;
        }
        if (const char* serial = SDL_GetJoystickSerial(joystick))
        {
            ss << ",serial:'" << serial << "'";
        }
        ss << ",num_axes:" << SDL_GetNumJoystickAxes(joystick);
        ss << ",num_balls:" << SDL_GetNumJoystickBalls(joystick);
        ss << ",num_hats:" << SDL_GetNumJoystickHats(joystick);
        ss << ",num_buttons:" << SDL_GetNumJoystickButtons(joystick);
        ss << "}";

        return ss.str();
    }

    string to_string(SDL_Gamepad* controller)
    {
        if (!controller)
        {
            return "nullptr";
        }

        stringstream ss;

        ss << "{type:'" << SDL_GetGamepadType(controller) << "'";
        ss << ",name:'" << ll_safe_string(SDL_GetGamepadName(controller)) << "'";
        ss << ",vendor:" << SDL_GetGamepadVendor(controller);
        ss << ",product:" << SDL_GetGamepadProduct(controller);
        if (U16 version = SDL_GetGamepadProductVersion(controller))
        {
            ss << ",version:" << version;
        }
        if (const char* serial = SDL_GetGamepadSerial(controller))
        {
            ss << ",serial:'" << serial << "'";
        }
        ss << "}";

        return ss.str();
    }
}

// Util for dumping SDL_Joystick info
std::ostream& operator<<(std::ostream& out, SDL_Joystick* joystick)
{
    return out << std::to_string(joystick);
}

// Util for dumping SDL_GameController info
std::ostream& operator<<(std::ostream& out, SDL_Gamepad* controller)
{
    return out << std::to_string(controller);
}

std::string LLGameControl::InputChannel::getLocalName() const
{
    // HACK: we hard-code English channel names, but
    // they should be loaded from localized XML config files.

    if (isAxis() && mIndex < NUM_AXES)
    {
        return "AXIS_" + std::to_string((U32)mIndex);
    }

    if (isButton() && mIndex < NUM_BUTTONS)
    {
        return "BUTTON_" + std::to_string((U32)mIndex);
    }

    return "NONE";
}

std::string LLGameControl::InputChannel::getSignedLocalName() const
{
    std::string name = getLocalName();
    if (isAxis() && mIndex < NUM_AXES)
    {
        name.append(mSign < 0 ? "-" : mSign > 0 ? "+" : "");
    }
    return name;
}

std::string LLGameControl::InputChannel::getRemoteName() const
{
    // HACK: we hard-code English channel names, but
    // they should be loaded from localized XML config files.
    std::string name = " ";
    // AXIS_LEFTX, BUTTON_SOUTH, etc
    if (isAxis())
    {
        switch (mIndex)
        {
            case 0:
                name = "AXIS_LEFTX";
                break;
            case 1:
                name = "AXIS_LEFTY";
                break;
            case 2:
                name = "AXIS_RIGHTX";
                break;
            case 3:
                name = "AXIS_RIGHTY";
                break;
            case 4:
                name = "AXIS_LEFT_TRIGGER";
                break;
            case 5:
                name = "AXIS_RIGHT_TRIGGER";
                break;
            default:
                break;
        }
    }
    else if (isButton())
    {
        switch(mIndex)
        {
            case 0:
                name = "BUTTON_SOUTH";
                break;
            case 1:
                name = "BUTTON_EAST";
                break;
            case 2:
                name = "BUTTON_WEST";
                break;
            case 3:
                name = "BUTTON_NORTH";
                break;
            case 4:
                name = "BUTTON_BACK";
                break;
            case 5:
                name = "BUTTON_GUIDE";
                break;
            case 6:
                name = "BUTTON_START";
                break;
            case 7:
                name = "BUTTON_LEFT_STICK";
                break;
            case 8:
                name = "BUTTON_RIGHT_STICK";
                break;
            case 9:
                name = "BUTTON_LEFT_SHOULDER";
                break;
            case 10:
                name = "BUTTON_RIGHT_SHOULDER";
                break;
            case 11:
                name = "BUTTON_DPAD_UP";
                break;
            case 12:
                name = "BUTTON_DPAD_DOWN";
                break;
            case 13:
                name = "BUTTON_DPAD_LEFT";
                break;
            case 14:
                name = "BUTTON_DPAD_RIGHT";
                break;
            case 15:
                name = "BUTTON_MISC1";
                break;
            case 16:
                name = "BUTTON_PADDLE1";
                break;
            case 17:
                name = "BUTTON_PADDLE2";
                break;
            case 18:
                name = "BUTTON_PADDLE3";
                break;
            case 19:
                name = "BUTTON_PADDLE4";
                break;
            case 20:
                name = "BUTTON_TOUCHPAD";
                break;
            default:
                break;
        }
    }
    return name;
}


// internal class for managing list of controllers and per-controller state
class LLGameControllerManager
{
public:
    using ActionToChannelMap = std::map< std::string, LLGameControl::InputChannel >;
    LLGameControllerManager();

    void resetDeviceOptionsToDefaults();
    void applyRememberedDeviceOptions();
    void rememberDeviceOptions() const;
    void setDeviceOptions(const std::string& guid, const LLGameControl::Options& options);

    void addController(SDL_JoystickID id, const std::string& guid, const std::string& name);
    void removeController(SDL_JoystickID id);

    const LLGameControl::Device* getLastActiveDevice() const;

    void onAxis(SDL_JoystickID id, U8 axis, S16 value);
    void onButton(SDL_JoystickID id, U8 button, bool pressed);

    void clearAllStates();

    void accumulateInternalState();
    void computeFinalState();

    LLGameControl::ActionNameType getActionNameType(const std::string& action) const;

    U32 computeInternalActionFlags();
    void getFlycamInputs(std::vector<F32>& inputs_out);
    void setExternalInput(U32 action_flags, U32 buttons);

    // Rebuild mAxisActionLabels/mButtonActionLabels from the global ModeMappings
    // for the currently-active AgentControlMode.  Cheap; called on settings change
    // and on mode change.  Safe to call every frame (early-outs when up to date).
    void rebuildActionLookup(bool force = false);

    void clear();

private:
    std::list<LLGameControl::Device> mDevices; // all connected devices
    using device_it = std::list<LLGameControl::Device>::iterator;
    device_it findDevice(SDL_JoystickID id)
    {
        return std::find_if(mDevices.begin(), mDevices.end(),
            [id](LLGameControl::Device& device)
            {
                return device.getJoystickID() == id;
            });
    }

    LLGameControl::State mExternalState;
    LLGameControlTranslator mActionTranslator;
    std::map<std::string, LLGameControl::ActionNameType> mActions;

    // Runtime action lookup for the active AgentControlMode, rebuilt from the
    // global ModeMappings whenever settings or the active mode change.  Each entry
    // holds the UI action label (e.g. "Move forward/back") bound to that canonical
    // input, or "" if unbound.  mAxisActionLabels is indexed by physical axis
    // (KeyboardAxis, NUM_AXES); mButtonActionLabels by Button (NUM_BUTTONS).
    std::vector<std::string> mAxisActionLabels;
    std::vector<std::string> mButtonActionLabels;
    LLGameControl::AgentControlMode mLookupMode { LLGameControl::CONTROL_MODE_NONE };

    // std::vector<S16> mAxesAccumulator;
    // std::vector<S16> mAxesMappedAccumulator;
    // LLGameControl::State mAccumulatedState;
    // LLGameControl::State mMappedAccumulatedState;
    U32 mButtonAccumulator { 0 };
    U32 mLastActiveFlags { 0 };
    U32 mLastFlycamActionFlags { 0 };
    SDL_JoystickID mlastActiveControllerID { 0 };

    friend class LLGameControl;
};

// local globals
namespace
{
    LLGameControl* g_gameControl = nullptr;
    LLGameControllerManager g_manager;

    // The GameControlInput message is sent via UDP which is lossy.
    // Since we send the only the list of pressed buttons the receiving
    // side can compute the difference between subsequent states to
    // find button-down/button-up events.
    //
    // To reduce the likelihood of buttons being stuck "pressed" forever
    // on the receiving side (for lost final packet) we resend the last
    // data state. However, to keep the ambient resend bandwidth low we
    // expand the resend period at a geometric rate.
    //
    constexpr U64 MSEC_PER_NSEC = 1000000;
    constexpr U64 FIRST_RESEND_PERIOD = 100 * MSEC_PER_NSEC;
    constexpr U64 RESEND_EXPANSION_RATE = 10;

    LLGameControl::State g_innerState; // state from all game controllers
    LLGameControl::State g_mappedState; // state after user mapping is applied
    LLGameControl::State g_flycamMappedState; // state for flycam after user mapping is applied
    LLGameControl::ServerState g_finalState; // sum of inner and outer

    LLTimer g_buttonLevelTimer[LLGameControl::Button::NUM_BUTTONS];
    LLTimer g_axisHeldTimer[LLGameControl::MovementDirection::NUM_MOVE_DIRS];
    S32 g_buttonLevelFrames[LLGameControl::Button::NUM_BUTTONS];
    S32 g_axisHeldFrames[LLGameControl::MovementDirection::NUM_MOVE_DIRS];

    U64 g_lastSend = 0;
    U64 g_nextResendPeriod = FIRST_RESEND_PERIOD;

    bool g_sendToServer = false;
    LLGameControl::AgentControlMode g_agentControlMode = LLGameControl::CONTROL_MODE_AVATAR;

    // g_gameControlSettings is the nested GameControl structure stored under the
    // single "GameControl" setting key: global per-mode action mappings plus
    // per-device hardware config.  See buildDefaultGameControlSettings() below.
    LLSD g_gameControlSettings;

    // g_deviceOptions is a map of [guid,deviceOptions] pairs for known devices
    // its values are expected to agree with connected device
    std::map<std::string, std::string> g_deviceOptions;

    LLGameControl::LoadSettingsFn s_loadSettings;
    LLGameControl::SaveSettingsFn s_saveSettings;
    std::function<void()> s_updateUI;

    std::string SETTING_GAMECONTROL("GameControl");

    // Keys used within the nested GameControl LLSD structure
    // (see buildDefaultGameControlSettings()).
    const std::string GC_COMMENT("Comment");
    const std::string GC_SENDTOSERVER("SendDataToServer");
    const std::string GC_DEVICES("Devices");
    const std::string GC_DEFAULT_DEVICE("Default");
    const std::string GC_CONFIG("Config");
    const std::string GC_MODEMAPPINGS("ModeMappings");
    const std::string GC_AXES("Axes");
    const std::string GC_BUTTONS("Buttons");
    const std::string GC_MODE_AVATAR("Avatar");
    const std::string GC_MODE_FLYCAM("FlyCam");
    const std::string GC_MODE_CAPTIVE("Captive");

#ifdef TEMPORARILY_DISABLED
    std::string ENUM_AGENTCONTROLMODE_FLYCAM("flycam");
    std::string ENUM_AGENTCONTROLMODE_NONE("none");

    LLGameControl::AgentControlMode convertStringToAgentControlMode(const std::string& mode)
    {
        if (mode == ENUM_AGENTCONTROLMODE_NONE)
            return LLGameControl::CONTROL_MODE_NONE;
        if (mode == ENUM_AGENTCONTROLMODE_FLYCAM)
            return LLGameControl::CONTROL_MODE_FLYCAM;
        // All values except NONE and FLYCAM are treated as default (AVATAR)
        return LLGameControl::CONTROL_MODE_AVATAR;
    }

    const std::string& convertAgentControlModeToString(LLGameControl::AgentControlMode mode)
    {
        if (mode == LLGameControl::CONTROL_MODE_NONE)
            return ENUM_AGENTCONTROLMODE_NONE;
        if (mode == LLGameControl::CONTROL_MODE_FLYCAM)
            return ENUM_AGENTCONTROLMODE_FLYCAM;
        // All values except NONE and FLYCAM are treated as default (AVATAR)
        return LLStringUtil::null;
    }
#endif // TEMPORARILY_DISABLED

    const std::string& getDeviceOptionsString(const std::string& guid)
    {
        const auto& it = g_deviceOptions.find(guid);
        return it == g_deviceOptions.end() ? LLStringUtil::null : it->second;
    }

    // Map an AgentControlMode to the mode key used in the GameControl LLSD.
    // Returns empty for modes without a mapping (e.g. CONTROL_MODE_NONE).
    const std::string& modeToString(LLGameControl::AgentControlMode mode)
    {
        switch (mode)
        {
            case LLGameControl::CONTROL_MODE_AVATAR:  return GC_MODE_AVATAR;
            case LLGameControl::CONTROL_MODE_FLYCAM:  return GC_MODE_FLYCAM;
            case LLGameControl::CONTROL_MODE_CAPTIVE: return GC_MODE_CAPTIVE;
            default:                                  return LLStringUtil::null;
        }
    }

    // Build the default global per-mode mapping structure.  Layout:
    //   { <Mode> : { "Axes": {action:input...}, "Buttons": {action:input...} } }
    // Action keys match the combo-box labels in panel_preferences_game_control.xml;
    // input values match the axis_input_selector / button_input_selector values.
    LLSD buildDefaultModeMappings()
    {
        // Axes: action label -> axis input
        LLSD avatar_axes;
        avatar_axes["Strafe left/right"] = "AXIS_LEFTX";
        avatar_axes["Move forward/back"] = "AXIS_LEFTY";
        avatar_axes["Turn left/right"]   = "AXIS_RIGHTX";
        avatar_axes["Look up/down"]      = "AXIS_RIGHTY";
        avatar_axes["Rise up"]           = "AXIS_LEFT_TRIGGER";
        avatar_axes["Drop down"]         = "AXIS_RIGHT_TRIGGER";

        // Buttons: action label -> button input
        LLSD avatar_buttons;
        avatar_buttons["Jump"]                   = "BUTTON_SOUTH";
        avatar_buttons["Crouch"]                 = "BUTTON_EAST";
        avatar_buttons["Toggle sit"]             = "BUTTON_WEST";
        avatar_buttons["Interact"]               = "BUTTON_NORTH";
        avatar_buttons["Toggle 3rd person view"] = "BUTTON_BACK";
        avatar_buttons["Toggle menu"]            = "BUTTON_GUIDE";
        avatar_buttons["Toggle mouselook"]       = "BUTTON_START";
        avatar_buttons["Toggle fly"]             = "BUTTON_LEFT_STICK";
        avatar_buttons["Toggle flycam"]          = "BUTTON_RIGHT_STICK";
        avatar_buttons["Mouse click left"]       = "BUTTON_LEFT_SHOULDER";
        avatar_buttons["Mouse click right"]      = "BUTTON_RIGHT_SHOULDER";
        avatar_buttons["Move forward"]           = "BUTTON_DPAD_UP";
        avatar_buttons["Move back"]              = "BUTTON_DPAD_DOWN";
        avatar_buttons["Strafe left"]            = "BUTTON_DPAD_LEFT";
        avatar_buttons["Strafe right"]           = "BUTTON_DPAD_RIGHT";

        // FlyCam adds a Roll axis and uses a distinct button set.
        LLSD flycam_axes = avatar_axes;
        flycam_axes["Roll left/right"] = "AXIS_NONE";

        LLSD flycam_buttons;
        flycam_buttons["Select"]         = "BUTTON_SOUTH";
        flycam_buttons["Toggle AltZoom"] = "BUTTON_EAST";
        flycam_buttons["Interact"]       = "BUTTON_NORTH";
        flycam_buttons["Toggle menu"]    = "BUTTON_GUIDE";
        flycam_buttons["Toggle flycam"]  = "BUTTON_RIGHT_STICK";
        flycam_buttons["Roll left"]      = "BUTTON_LEFT_SHOULDER";
        flycam_buttons["Roll right"]     = "BUTTON_RIGHT_SHOULDER";
        flycam_buttons["Move forward"]   = "BUTTON_DPAD_UP";
        flycam_buttons["Move back"]      = "BUTTON_DPAD_DOWN";
        flycam_buttons["Strafe left"]    = "BUTTON_DPAD_LEFT";
        flycam_buttons["Strafe right"]   = "BUTTON_DPAD_RIGHT";

        auto makeMode = [](const LLSD& axes, const LLSD& buttons)
        {
            LLSD mode;
            mode[GC_AXES]    = axes;
            mode[GC_BUTTONS] = buttons;
            return mode;
        };

        LLSD mappings;
        mappings[GC_MODE_AVATAR]  = makeMode(avatar_axes, avatar_buttons);
        mappings[GC_MODE_FLYCAM]  = makeMode(flycam_axes, flycam_buttons);
        mappings[GC_MODE_CAPTIVE] = makeMode(avatar_axes, avatar_buttons);
        return mappings;
    }

    // Build the full default GameControl structure: global per-mode action mappings
    // plus a single empty "Default" device template.  ModeMappings are global (not
    // per-device): the Devices layer normalizes hardware to canonical inputs, while
    // the global ModeMappings layer binds canonical inputs to logical actions per mode.
    LLSD buildDefaultGameControlSettings()
    {
        LLSD device;
        device[GC_CONFIG] = "";

        LLSD settings;
        settings[GC_COMMENT] = "GameControl settings";
        settings[GC_SENDTOSERVER] = false;
        settings[GC_MODEMAPPINGS] = buildDefaultModeMappings();
        settings[GC_DEVICES][GC_DEFAULT_DEVICE] = device;
        return settings;
    }

    // Ensure g_gameControlSettings holds a usable structure; seed from defaults
    // if it is empty or malformed.  The global ModeMappings block is seeded
    // independently so an older config that lacks it still gets sane defaults.
    void ensureGameControlSettings()
    {
        if (!g_gameControlSettings.isMap() || !g_gameControlSettings.has(GC_DEVICES))
        {
            g_gameControlSettings = buildDefaultGameControlSettings();
        }
        if (!g_gameControlSettings[GC_MODEMAPPINGS].isMap())
        {
            g_gameControlSettings[GC_MODEMAPPINGS] = buildDefaultModeMappings();
        }
    }

    // Ensure a per-device entry exists before mutating it.  Devices now hold only
    // their hardware Config; the action mappings live in the global ModeMappings.
    void ensureDeviceEntry(const std::string& guid)
    {
        ensureGameControlSettings();
        LLSD& devices = g_gameControlSettings[GC_DEVICES];
        if (!devices.has(guid))
        {
            LLSD device;
            device[GC_CONFIG] = "";
            devices[guid] = device;
        }
    }
}

LLGameControl::~LLGameControl()
{
    terminate();
}

LLGameControl::State::State()
    :
    mButtons(0),
    mPrevButtons(0)

{
    mAxes.resize(NUM_MOVE_DIRS, 0);
    mRawAxes.resize(NUM_MOVE_DIRS, 0);
    mPrevAxes.resize(NUM_MOVE_DIRS, 0);
}

void LLGameControl::State::clear()
{
    std::fill(mAxes.begin(), mAxes.end(), 0);
    std::fill(mRawAxes.begin(), mRawAxes.end(), 0);
    mButtons = 0;

    // // DO NOT clear mPrevAxes because those are managed by external logic.
    // std::fill(mPrevAxes.begin(), mPrevAxes.end(), 0);
    // mPrevButtons = 0;
}

void LLGameControl::State::storePrevious()
{
    mPrevButtons = mButtons;
    for(size_t i = 0; i < mAxes.size(); i++)
    {
        mPrevAxes[i] = mAxes[i];
    }
}

bool LLGameControl::State::onButton(U8 button, bool pressed)
{
    U32 old_buttons = mButtons;
    if (button < NUM_BUTTONS)
    {
        if (pressed)
        {
            mButtons |= (0x01 << button);
        }
        else
        {
            mButtons &= ~(0x01 << button);
        }
    }
    return mButtons != old_buttons;
}

LLGameControl::ServerState::ServerState()
: mButtons(0)
, mPrevButtons(0)
{
    mAxes.resize(NUM_AXES,0);
    mPrevAxes.resize(NUM_AXES,0);
}

void LLGameControl::ServerState::clear()
{
    std::fill(mAxes.begin(), mAxes.end(), 0);
    mButtons = 0;

    // // DO NOT clear mPrevAxes because those are managed by external logic.
    // std::fill(mPrevAxes.begin(), mPrevAxes.end(), 0);
    // mPrevButtons = 0;
}

LLGameControl::Device::Device(int joystickID, const std::string& guid, const std::string& name)
: mJoystickID(joystickID)
, mGUID(guid)
, mName(name)
{
}

S16 LLGameControl::Options::AxisOptions::computeModifiedValue(S16 raw_value) const
{
    S16 new_value = (S16)(std::clamp(((S32)raw_value + S32(mOffset)) * mMultiplier, -32768, 32767));
    if (abs(new_value) < mDeadZone)
    {
        new_value = 0;
    }
    return new_value;
}

void LLGameControl::Options::AxisOptions::resetToDefaults()
{
    mMultiplier = 1;
    mDeadZone = DEFAULT_DEAD_ZONE;
    mOffset = 0;
}

LLGameControl::Options::Options()
{
    mAxisOptions.resize(NUM_AXES);
    mAxisMap.resize(NUM_AXES);
    mButtonMap.resize(NUM_BUTTONS);
    resetToDefaults();
}

void LLGameControl::Options::resetToDefaults()
{
    for (size_t i = 0; i < NUM_AXES; ++i)
    {
        mAxisOptions[i].resetToDefaults();
        mAxisMap[i] = (U8)i;
    }
    for (size_t i = 0; i < NUM_BUTTONS; ++i)
    {
        mButtonMap[i] = (U8)i;
    }
}

U8 LLGameControl::Options::mapAxis(U8 axis) const
{
    if (axis >= NUM_AXES)
    {
        LL_WARNS("SDL3") << "Invalid input axis: " << axis << LL_ENDL;
        return axis;
    }
    return mAxisMap[axis];
}

U8 LLGameControl::Options::mapButton(U8 button) const
{
    if (button >= NUM_BUTTONS)
    {
        LL_WARNS("SDL3") << "Invalid input button: " << button << LL_ENDL;
        return button;
    }
    return mButtonMap[button];
}

U8 LLGameControl::Options::unmapAxis(U8 axis) const
{
    if (axis >= NUM_AXES)
    {
        LL_WARNS("SDL3") << "Invalid unmap axis: " << axis << LL_ENDL;
        return axis;
    }
    for(size_t i=0; i < mAxisMap.size(); i++)
    {
        if(mAxisMap[i] == axis)
        {
            return (U8)i;
        }
    }
    return axis;
}

U8 LLGameControl::Options::unmapButton(U8 button) const
{

    if (button >= NUM_BUTTONS)
    {
        LL_WARNS("SDL3") << "Invalid unmap button: " << button << LL_ENDL;
        return button;
    }

    for(size_t i=0; i < mButtonMap.size(); i++)
    {
        if(mButtonMap[i] == button)
        {
            return (U8)i;
        }
    }
    return button;
}

S16 LLGameControl::Options::fixAxisValue(U8 axis, S16 value) const
{
    if (axis >= NUM_AXES)
    {
        LL_WARNS("SDL3") << "Invalid input axis: " << axis << LL_ENDL;
    }
    else
    {
        value = mAxisOptions[axis].computeModifiedValue(value);
    }
    return value;
}

std::string LLGameControl::Options::AxisOptions::saveToString() const
{
    std::list<std::string> options;

    if (mMultiplier == -1)
    {
        options.push_back("invert:1");
    }
    if (mDeadZone)
    {
        options.push_back(llformat("dead_zone:%u", mDeadZone));
    }
    if (mOffset)
    {
        options.push_back(llformat("offset:%d", mOffset));
    }

    std::string result = LLStringUtil::join(options);

    return result.empty() ? result : "{" + result + "}";
}

// Parse string "{key:value,key:{key:value,key:value}}" and fill the map
static bool parse(std::map<std::string, std::string>& result, std::string source)
{
    result.clear();

    LLStringUtil::trim(source);
    if (source.empty())
        return true;

    if (source.front() != '{' || source.back() != '}')
        return false;

    source = source.substr(1, source.size() - 2);

    LLStringUtil::trim(source);
    if (source.empty())
        return true;

    // Split the string "key:value" and add the pair to the map
    auto split = [&](const std::string& pair) -> bool
    {
        size_t pos = pair.find(':');
        if (!pos || pos == std::string::npos)
            return false;
        std::string key = pair.substr(0, pos);
        std::string value = pair.substr(pos + 1);
        LLStringUtil::trim(key);
        LLStringUtil::trim(value);
        if (key.empty() || value.empty())
            return false;
        result[key] = value;
        return true;
    };

    U32 depth = 0;
    size_t offset = 0;
    while (true)
    {
        size_t pos = source.find_first_of(depth ? "{}" : ",{}", offset);
        if (pos == std::string::npos)
        {
            return !depth && split(source);
        }
        if (source[pos] == ',')
        {
            if (!split(source.substr(0, pos)))
                return false;
            source = source.substr(pos + 1);
            offset = 0;
        }
        else if (source[pos] == '{')
        {
            depth++;
            offset = pos + 1;
        }
        else if (depth) // Assume '}' here
        {
            depth--;
            offset = pos + 1;
        }
        else
        {
            return false; // Extra '}' found
        }
    }

    return true;
}

void LLGameControl::Options::AxisOptions::loadFromString(std::string options)
{
    resetToDefaults();

    if (options.empty())
        return;

    std::map<std::string, std::string> pairs;
    if (!parse(pairs, options))
    {
        LL_WARNS("SDL3") << "Invalid axis options: '" << options << "'" << LL_ENDL;
    }

    mMultiplier = 1;
    std::string invert = pairs["invert"];
    if (!invert.empty())
    {
        if (invert == "1")
        {
            mMultiplier = -1;
        }
        else
        {
            LL_WARNS("SDL3") << "Invalid invert value: '" << invert << "'" << LL_ENDL;
        }
    }

    std::string dead_zone = pairs["dead_zone"];
    if (!dead_zone.empty())
    {
        size_t number = std::stoull(dead_zone);
        if (number <= MAX_AXIS_DEAD_ZONE && std::to_string(number) == dead_zone)
        {
            mDeadZone = (U16)number;
        }
        else
        {
            LL_WARNS("SDL3") << "Invalid dead_zone value: '" << dead_zone << "'" << LL_ENDL;
        }
    }

    std::string offset = pairs["offset"];
    if (!offset.empty())
    {
        S32 number = std::stoi(offset);
        if (abs(number) > MAX_AXIS_OFFSET || std::to_string(number) != offset)
        {
            LL_WARNS("SDL3") << "Invalid offset value: '" << offset << "'" << LL_ENDL;
        }
        else
        {
            mOffset = (S16)number;
        }
    }
}

std::string LLGameControl::Options::saveToString(const std::string& name, bool force_empty) const
{
    return stringifyDeviceOptions(name, mAxisOptions, mAxisMap, mButtonMap, force_empty);
}

bool LLGameControl::Options::loadFromString(std::string& name, std::string options)
{
    resetToDefaults();
    return LLGameControl::parseDeviceOptions(options, name, mAxisOptions, mAxisMap, mButtonMap);
}

bool LLGameControl::Options::loadFromString(std::string options)
{
    resetToDefaults();
    std::string dummy_name;
    return LLGameControl::parseDeviceOptions(options, dummy_name, mAxisOptions, mAxisMap, mButtonMap);
}

LLGameControllerManager::LLGameControllerManager()
{
}

void LLGameControllerManager::resetDeviceOptionsToDefaults()
{
    for (LLGameControl::Device& device : mDevices)
    {
        device.resetOptionsToDefaults();
    }
}

void LLGameControllerManager::applyRememberedDeviceOptions()
{
    for (LLGameControl::Device& device : mDevices)
    {
        device.loadOptionsFromString(getDeviceOptionsString(device.getGUID()));
    }
}

void LLGameControllerManager::rememberDeviceOptions() const
{
    for (const LLGameControl::Device& device : mDevices)
    {
        std::string options = device.saveOptionsToString();
        if (options.empty())
        {
            g_deviceOptions.erase(device.getGUID());
        }
        else
        {
            g_deviceOptions[device.getGUID()] = options;
        }
    }
}

void LLGameControllerManager::setDeviceOptions(const std::string& guid, const LLGameControl::Options& options)
{
    for (LLGameControl::Device& device : mDevices)
    {
        if (device.getGUID() == guid)
        {
            device.mOptions = options;

            // remember the options
            std::string options_str = device.saveOptionsToString(true);
            auto itr = g_deviceOptions.find(guid);
            if (itr == g_deviceOptions.end())
            {
                g_deviceOptions.insert({guid, options_str});
            }
            else
            {
                itr->second = options_str;
            }
            return;
        }
    }
}

void LLGameControllerManager::addController(SDL_JoystickID id, const std::string& guid, const std::string& name)
{
    llassert(id >= 0);

    for (const LLGameControl::Device& device :  mDevices)
    {
        if (device.getJoystickID() == id)
        {
            LL_WARNS("SDL3") << "device with id=" << id << " was already added"
                << ", guid: '" << device.getGUID() << "'"
                << ", name: '" << device.getName() << "'"
                << LL_ENDL;
            return;
        }
    }

    mDevices.emplace_back(id, guid, name).loadOptionsFromString(getDeviceOptionsString(guid));
}

void LLGameControllerManager::removeController(SDL_JoystickID id)
{
    LL_INFOS("SDL3") << "joystick id: " << id << LL_ENDL;

    mDevices.remove_if([id](LLGameControl::Device& device)
        {
            return device.getJoystickID() == id;
        });
}

const LLGameControl::Device* LLGameControllerManager::getLastActiveDevice() const
{
    if (mDevices.empty())
    {
        return nullptr;
    }

    if (mlastActiveControllerID != 0)
    {
        auto it = std::find_if(mDevices.begin(), mDevices.end(),
            [this](const LLGameControl::Device& device)
            {
                return device.getJoystickID() == mlastActiveControllerID;
            });
        if (it != mDevices.end())
        {
            return &(*it);
        }
    }

    return &mDevices.front();
}

// Negates a signed axis value, accounting for the asymmetric S16 range
// [-32768, 32767] by shifting one during negation.
static S16 negateAxisValue(S16 value)
{
    if (value < 0)
    {
        return (S16)(-(value + 1));
    }
    if (value > 0)
    {
        return (S16)((-value) - 1);
    }
    return 0;
}

// Splits a signed axis value into the +/- half-axis pair used by State::mAxes /
// State::mRawAxes: the positive magnitude lands in half_axes[base], the negative
// magnitude in half_axes[base + 1].
static void storeHalfAxes(std::vector<U16>& half_axes, U8 base, S16 value)
{
    half_axes[base] = 0;
    half_axes[base + 1] = 0;
    if (value > 0)
    {
        half_axes[base] = value;
    }
    else
    {
        half_axes[base + 1] = (U16)abs(value);
    }
}

void LLGameControllerManager::onAxis(SDL_JoystickID id, U8 axis, S16 value)
{
    device_it it = findDevice(id);
    if (it == mDevices.end())
    {
        LL_WARNS("SDL3") << "Unknown device: joystick=0x" << std::hex << id << std::dec
            << " axis=" << (S32)axis
            << " value=" << (S32)value << LL_ENDL;
        return;
    }

    // Map axis using device-specific settings
    // or leave the value unchanged
    U8 mapped_axis = it->mOptions.mapAxis(axis);
    if (mapped_axis != axis)
    {
        LL_DEBUGS("SDL3") << "Axis mapped: joystick=0x" << std::hex << id << std::dec
            << " input axis i=" << (S32)axis
            << " mapped axis i=" << (S32)mapped_axis << LL_ENDL;
        axis = mapped_axis;
    }

    if (axis >= LLGameControl::NUM_AXES)
    {
        LL_WARNS("SDL3") << "Unknown axis: joystick=0x" << std::hex << id << std::dec
            << " axis=" << (S32)(axis)
            << " value=" << (S32)(value) << LL_ENDL;
        return;
    }

    // Keep the pre-fix (raw) value so the UI can display it alongside the
    // post-fix value; see LLGameControl::Device::State::mRawAxes.
    S16 raw_value = value;

    // Fix value using device-specific settings
    // or leave the value unchanged
    S16 fixed_value = it->mOptions.fixAxisValue(axis, value);
    if (fixed_value != value)
    {
        LL_DEBUGS("SDL3") << "Value fixed: joystick=0x" << std::hex << id << std::dec
            << " axis i=" << (S32)axis
            << " input value=" << (S32)value
            << " fixed value=" << (S32)fixed_value << LL_ENDL;
        value = fixed_value;
    }

    // Note: the RAW analog joysticks provide NEGATIVE X,Y values for LEFT,FORWARD
    // whereas those directions are actually POSITIVE in SL's local right-handed
    // reference frame.  Therefore we implicitly negate those axes here where
    // they are extracted from SDL, before being used anywhere.  The raw value is
    // negated the same way so both share one sign convention and differ only by
    // the fix transform (dead zone / offset / invert).
    if (axis < SDL_GAMEPAD_AXIS_LEFT_TRIGGER)
    {
        value = negateAxisValue(value);
        raw_value = negateAxisValue(raw_value);
    }

    LL_DEBUGS("SDL3") << "joystick=0x" << std::hex << id << std::dec
        << " axis=" << (S32)(axis)
        << " value=" << (S32)(value) << LL_ENDL;

    U8 base = axis * 2;
    storeHalfAxes(it->mState.mAxes, base, value);
    storeHalfAxes(it->mState.mRawAxes, base, raw_value);
}

void LLGameControllerManager::onButton(SDL_JoystickID id, U8 button, bool pressed)
{
    device_it it = findDevice(id);
    if (it == mDevices.end())
    {
        LL_WARNS("SDL3") << "Unknown device: joystick=0x" << std::hex << id << std::dec
            << " button i=" << (S32)button << LL_ENDL;
        return;
    }

    mlastActiveControllerID = id;

    // Map button using device-specific settings
    // or leave the value unchanged
    U8 mapped_button = it->mOptions.mapButton(button);
    if (mapped_button != button)
    {
        LL_DEBUGS("SDL3") << "Button mapped: joystick=0x" << std::hex << id << std::dec
            << " input button i=" << (S32)button
            << " mapped button i=" << (S32)mapped_button << LL_ENDL;
        button = mapped_button;
    }

    if (button >= LLGameControl::NUM_BUTTONS)
    {
        LL_WARNS("SDL3") << "Unknown button: joystick=0x" << std::hex << id << std::dec
            << " button i=" << (S32)button << LL_ENDL;
        return;
    }

    if (it->mState.onButton(button, pressed))
    {
        LL_DEBUGS("SDL3") << "joystick=0x" << std::hex << id << std::dec
            << " button i=" << (S32)button
            << " pressed=" << pressed << LL_ENDL;
    }
}

void LLGameControllerManager::clearAllStates()
{
    for (auto& device : mDevices)
    {
        device.mState.clear();
    }
    mExternalState.clear();
    mLastActiveFlags = 0;
    mLastFlycamActionFlags = 0;
}

void LLGameControllerManager::accumulateInternalState()
{
    g_innerState.storePrevious();
    // clear the old state
    g_innerState.clear();


    // accumulate the controllers
    for (const auto& device : mDevices)
    {
        g_innerState.mButtons |= device.mState.mButtons;
        for(U8 i = 0; i < LLGameControl::Button::NUM_BUTTONS; i++)
        {
            U32 button = 1 << i;
            if(g_innerState.mButtons & button && ~(g_innerState.mPrevButtons) & button)
            {
                g_buttonLevelTimer[i].reset();
                g_buttonLevelFrames[i] = 0;
            }
            else
            {
                g_buttonLevelFrames[i]++;
            }
        }
        for (size_t i = 0; i < LLGameControl::NUM_MOVE_DIRS; ++i)
        {
            // Note: we don't bother to clamp the axes yet
            // because at this stage we haven't yet accumulated the "inner" state.
            if (device.mState.mAxes[i] > g_innerState.mAxes[i])
            {
                g_innerState.mAxes[i] = (S32)device.mState.mAxes[i];
                if(g_innerState.mPrevAxes[i] < 1)
                {
                    g_axisHeldTimer[i].reset();
                    g_axisHeldFrames[i] = 0;
                }
                else
                {
                    g_axisHeldFrames[i]++;
                }
            }
        }
    }
}

F32 LLGameControl::getControllerHeldTime(ActionType actionType, U8 action)
{
    switch (actionType)
    {
        case ActionType::DOF:
            return g_axisHeldTimer[action].getElapsedTimeF32();
        case ActionType::BUTTON:
            return g_buttonLevelTimer[action].getElapsedTimeF32();
        default:
            return 0.0;
    }
}

S32 LLGameControl::getControllerHeldFrames(ActionType actionType, U8 action)
{
    switch (actionType)
    {
        case ActionType::DOF:
            return g_axisHeldFrames[action];
        case ActionType::BUTTON:
            return g_buttonLevelFrames[action];
        default:
            return 0.0;
    }
}

void LLGameControllerManager::computeFinalState()
{
    static const LLGameControlTranslator::ControllerMappings axis_mappings = {
        // Axes
        {{LLGameControl::ActionType::DOF, LLGameControl::MovementDirection::MOVE_DIR_STRAFE_LEFT},          LLGameControl::MovementDirection::MOVE_DIR_STRAFE_LEFT},
        {{LLGameControl::ActionType::DOF, LLGameControl::MovementDirection::MOVE_DIR_STRAFE_RIGHT},         LLGameControl::MovementDirection::MOVE_DIR_STRAFE_RIGHT},
        {{LLGameControl::ActionType::DOF, LLGameControl::MovementDirection::MOVE_DIR_PUSH_FORWARD},           LLGameControl::MovementDirection::MOVE_DIR_PUSH_FORWARD},
        {{LLGameControl::ActionType::DOF, LLGameControl::MovementDirection::MOVE_DIR_PUSH_BACKWARD},          LLGameControl::MovementDirection::MOVE_DIR_PUSH_BACKWARD},
        {{LLGameControl::ActionType::DOF, LLGameControl::MovementDirection::MOVE_DIR_TURN_LEFT},     LLGameControl::MovementDirection::MOVE_DIR_TURN_LEFT},
        {{LLGameControl::ActionType::DOF, LLGameControl::MovementDirection::MOVE_DIR_TURN_RIGHT},    LLGameControl::MovementDirection::MOVE_DIR_TURN_RIGHT},
        {{LLGameControl::ActionType::DOF, LLGameControl::MovementDirection::MOVE_DIR_LOOK_UP},       LLGameControl::MovementDirection::MOVE_DIR_LOOK_UP},
        {{LLGameControl::ActionType::DOF, LLGameControl::MovementDirection::MOVE_DIR_LOOK_DOWN},     LLGameControl::MovementDirection::MOVE_DIR_LOOK_DOWN},
        {{LLGameControl::ActionType::DOF, LLGameControl::MovementDirection::MOVE_DIR_RISE_UP},            LLGameControl::MovementDirection::MOVE_DIR_RISE_UP},
        {{LLGameControl::ActionType::DOF, LLGameControl::MovementDirection::MOVE_DIR_DROP_DOWN},          LLGameControl::MovementDirection::MOVE_DIR_DROP_DOWN},
        {{LLGameControl::ActionType::DOF, LLGameControl::MovementDirection::MOVE_DIR_ROLL_LEFT},     LLGameControl::MovementDirection::MOVE_DIR_ROLL_LEFT},
        {{LLGameControl::ActionType::DOF, LLGameControl::MovementDirection::MOVE_DIR_ROLL_RIGHT},    LLGameControl::MovementDirection::MOVE_DIR_ROLL_RIGHT}
    };

    static const LLGameControlTranslator::ControllerMappings button_mappings = {
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_SOUTH},              (U32)1 << LLGameControl::Button::BUTTON_SOUTH},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_EAST},              (U32)1 << LLGameControl::Button::BUTTON_EAST},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_WEST},              (U32)1 << LLGameControl::Button::BUTTON_WEST},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_NORTH},              (U32)1 << LLGameControl::Button::BUTTON_NORTH},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_BACK},           (U32)1 << LLGameControl::Button::BUTTON_BACK},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_GUIDE},          (U32)1 << LLGameControl::Button::BUTTON_GUIDE},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_START},          (U32)1 << LLGameControl::Button::BUTTON_START},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_LEFT_STICK},      (U32)1 << LLGameControl::Button::BUTTON_LEFT_STICK},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_RIGHT_STICK},     (U32)1 << LLGameControl::Button::BUTTON_RIGHT_STICK},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_LEFT_SHOULDER},   (U32)1 << LLGameControl::Button::BUTTON_LEFT_SHOULDER},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_RIGHT_SHOULDER},  (U32)1 << LLGameControl::Button::BUTTON_RIGHT_SHOULDER},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_DPAD_UP},        (U32)1 << LLGameControl::Button::BUTTON_DPAD_UP},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_DPAD_DOWN},      (U32)1 << LLGameControl::Button::BUTTON_DPAD_DOWN},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_DPAD_LEFT},      (U32)1 << LLGameControl::Button::BUTTON_DPAD_LEFT},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_DPAD_RIGHT},     (U32)1 << LLGameControl::Button::BUTTON_DPAD_RIGHT},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_MISC1},          (U32)1 << LLGameControl::Button::BUTTON_MISC1},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_PADDLE1},        (U32)1 << LLGameControl::Button::BUTTON_PADDLE1},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_PADDLE2},        (U32)1 << LLGameControl::Button::BUTTON_PADDLE2},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_PADDLE3},        (U32)1 << LLGameControl::Button::BUTTON_PADDLE3},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_PADDLE4},        (U32)1 << LLGameControl::Button::BUTTON_PADDLE4},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_TOUCHPAD},       (U32)1 << LLGameControl::Button::BUTTON_TOUCHPAD},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_21},             (U32)1 << LLGameControl::Button::BUTTON_21},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_22},             (U32)1 << LLGameControl::Button::BUTTON_22},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_23},             (U32)1 << LLGameControl::Button::BUTTON_23},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_24},             (U32)1 << LLGameControl::Button::BUTTON_24},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_25},             (U32)1 << LLGameControl::Button::BUTTON_25},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_26},             (U32)1 << LLGameControl::Button::BUTTON_26},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_27},             (U32)1 << LLGameControl::Button::BUTTON_27},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_28},             (U32)1 << LLGameControl::Button::BUTTON_28},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_29},             (U32)1 << LLGameControl::Button::BUTTON_29},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_30},             (U32)1 << LLGameControl::Button::BUTTON_30},
        {{LLGameControl::ActionType::BUTTON, LLGameControl::Button::BUTTON_31},             (U32)1 << LLGameControl::Button::BUTTON_31}
    };

    // We assume accumulateInternalState() has already been called and we will
    // finish by accumulating "external" state (if enabled)
    U32 old_buttons = g_finalState.mButtons;
    g_mappedState.clear();
    g_mappedState.mButtons = mActionTranslator.calculateTranslatedButtons(button_mappings, g_innerState);

    mActionTranslator.calculateTranslatedAxes(axis_mappings, g_innerState, g_mappedState.mAxes);

    g_finalState.mPrevButtons = g_finalState.mButtons;
    g_finalState.mButtons = g_mappedState.mButtons;

    // TODO: accumulate from mExternalState in a different way
    g_finalState.mButtons |= mExternalState.mButtons;

    if (old_buttons != g_finalState.mButtons)
    {
        g_nextResendPeriod = 0; // packet needs to go out ASAP
    }

    size_t final_i = 0;
    // clamp the accumulated axes
    for(size_t i = 0; i < LLGameControl::NUM_MOVE_DIRS; i+=2)
    {
        S16 axis_pos = g_mappedState.mAxes[i];
        S16 axis_neg = g_mappedState.mAxes[i+1];
        if (true)
        {
            // Note: we accumulate mExternalState onto local 'axis' variable
            // rather than onto mAxisAccumulator[i] because the internal
            // accumulated value is also used to drive the Flycam, and
            // we don't want any external state leaking into that value.
            // TODO: FIX TO greater not sum
            if(mExternalState.mAxes[i] > axis_pos)
            {
                axis_pos = mExternalState.mAxes[i];
            }
            if(mExternalState.mAxes[i+1] > axis_neg)
            {
                axis_neg = mExternalState.mAxes[i+1];
            }
        }
        // axis = (S16)std::min(std::max(axis, -32768), 32767);
        S16 axis = axis_pos + (axis_neg * -1);
        // check for change
        // Note: g_mappedState uses NUM_MOVE_DIRS split half-axes (indexed by 'i'),
        // while g_finalState uses NUM_AXES combined signed axes (indexed by 'final_i').
        if (g_finalState.mAxes[final_i] != axis)
        {
            // When axis changes we explicitly update the corresponding prevAxis
            // prior to storing axis.  The only other place where prevAxis
            // is updated in updateResendPeriod() which is explicitly called after
            // a packet is sent.  The result is: unchanged axes are included in
            // first resend but not later ones.
            g_finalState.mPrevAxes[final_i] = g_finalState.mAxes[final_i];
            g_finalState.mAxes[final_i] = axis;
            g_nextResendPeriod = 0; // packet needs to go out ASAP
        }
        ++final_i;
    }
}

LLGameControl::ActionNameType LLGameControllerManager::getActionNameType(const std::string& action) const
{
    auto it = mActions.find(action);
    return it == mActions.end() ? LLGameControl::ACTION_NAME_UNKNOWN : it->second;
}

namespace
{
    // Resolve a symbolic input name as stored in ModeMappings ("AXIS_LEFTX",
    // "BUTTON_SOUTH", "AXIS_NONE", ...) to a canonical InputChannel.  This is the
    // inverse of InputChannel::getRemoteName(); axis channels are returned with
    // sign 0 (bidirectional) because ModeMappings do not store a sign.  Anything
    // unrecognized (including the "none" sentinels) yields an isNone() channel.
    LLGameControl::InputChannel channelFromInputName(const std::string& name)
    {
        static const std::map<std::string, U8> s_axis_names = {
            { "AXIS_LEFTX",         LLGameControl::AXIS_LEFTX },
            { "AXIS_LEFTY",         LLGameControl::AXIS_LEFTY },
            { "AXIS_RIGHTX",        LLGameControl::AXIS_RIGHTX },
            { "AXIS_RIGHTY",        LLGameControl::AXIS_RIGHTY },
            { "AXIS_LEFT_TRIGGER",  LLGameControl::AXIS_LEFT_TRIGGER },
            { "AXIS_RIGHT_TRIGGER", LLGameControl::AXIS_RIGHT_TRIGGER },
        };
        static const std::map<std::string, U8> s_button_names = {
            { "BUTTON_SOUTH",          LLGameControl::BUTTON_SOUTH },
            { "BUTTON_EAST",           LLGameControl::BUTTON_EAST },
            { "BUTTON_WEST",           LLGameControl::BUTTON_WEST },
            { "BUTTON_NORTH",          LLGameControl::BUTTON_NORTH },
            { "BUTTON_BACK",           LLGameControl::BUTTON_BACK },
            { "BUTTON_GUIDE",          LLGameControl::BUTTON_GUIDE },
            { "BUTTON_START",          LLGameControl::BUTTON_START },
            { "BUTTON_LEFT_STICK",     LLGameControl::BUTTON_LEFT_STICK },
            { "BUTTON_RIGHT_STICK",    LLGameControl::BUTTON_RIGHT_STICK },
            { "BUTTON_LEFT_SHOULDER",  LLGameControl::BUTTON_LEFT_SHOULDER },
            { "BUTTON_RIGHT_SHOULDER", LLGameControl::BUTTON_RIGHT_SHOULDER },
            { "BUTTON_DPAD_UP",        LLGameControl::BUTTON_DPAD_UP },
            { "BUTTON_DPAD_DOWN",      LLGameControl::BUTTON_DPAD_DOWN },
            { "BUTTON_DPAD_LEFT",      LLGameControl::BUTTON_DPAD_LEFT },
            { "BUTTON_DPAD_RIGHT",     LLGameControl::BUTTON_DPAD_RIGHT },
            { "BUTTON_MISC1",          LLGameControl::BUTTON_MISC1 },
            { "BUTTON_PADDLE1",        LLGameControl::BUTTON_PADDLE1 },
            { "BUTTON_PADDLE2",        LLGameControl::BUTTON_PADDLE2 },
            { "BUTTON_PADDLE3",        LLGameControl::BUTTON_PADDLE3 },
            { "BUTTON_PADDLE4",        LLGameControl::BUTTON_PADDLE4 },
            { "BUTTON_TOUCHPAD",       LLGameControl::BUTTON_TOUCHPAD },
        };

        auto ait = s_axis_names.find(name);
        if (ait != s_axis_names.end())
        {
            return LLGameControl::InputChannel(LLGameControl::InputChannel::TYPE_AXIS, ait->second, 0);
        }
        auto bit = s_button_names.find(name);
        if (bit != s_button_names.end())
        {
            return LLGameControl::InputChannel(LLGameControl::InputChannel::TYPE_BUTTON, bit->second);
        }
        // Numeric fallback for the higher buttons (e.g. "BUTTON_21").
        if (LLStringUtil::startsWith(name, "BUTTON_"))
        {
            S32 index = atoi(name.substr(7).c_str());
            if (index > 0 && index < LLGameControl::NUM_BUTTONS)
            {
                return LLGameControl::InputChannel(LLGameControl::InputChannel::TYPE_BUTTON, (U8)index);
            }
        }
        return LLGameControl::InputChannel(); // isNone()
    }

    // Step-0 bridge (approach A): UI action label -> engine effect for the
    // Avatar/Captive modes.  Analog axis labels expand to a positive-half and
    // negative-half AGENT_CONTROL bit; unidirectional trigger labels ("Rise up",
    // "Drop down") use only the positive half.  Button labels expand to a single
    // AGENT_CONTROL bit (the fly/flycam entries reuse the HACK bits consumed by
    // LLAgent::applyExternalActionFlags).  Labels that map to viewer *commands*
    // rather than movement bits (Interact, Toggle sit/menu/mouselook/3rd-person,
    // Mouse clicks) are intentionally absent here and are a separate follow-up.
    struct AxisActionEffect { U32 posFlag; U32 negFlag; };

    const std::map<std::string, AxisActionEffect>& avatarAxisBridge()
    {
        // Note: g_innerState half-axes are pre-negated so LEFT/FORWARD/UP land in
        // the positive half (axis*2); see LLGameControllerManager::onAxis().
        static const std::map<std::string, AxisActionEffect> bridge = {
            { "Strafe left/right", { AGENT_CONTROL_LEFT_POS,  AGENT_CONTROL_LEFT_NEG } },
            { "Move forward/back", { AGENT_CONTROL_AT_POS,    AGENT_CONTROL_AT_NEG } },
            { "Turn left/right",   { AGENT_CONTROL_YAW_POS,   AGENT_CONTROL_YAW_NEG } },
            { "Look up/down",      { AGENT_CONTROL_PITCH_POS, AGENT_CONTROL_PITCH_NEG } },
            { "Rise up",           { AGENT_CONTROL_UP_POS,    0 } },
            { "Drop down",         { AGENT_CONTROL_UP_NEG,    0 } },
        };
        return bridge;
    }

    const std::map<std::string, U32>& avatarButtonBridge()
    {
        static const std::map<std::string, U32> bridge = {
            { "Jump",          AGENT_CONTROL_UP_POS },
            { "Crouch",        AGENT_CONTROL_UP_NEG },
            { "Move forward",  AGENT_CONTROL_AT_POS },
            { "Move back",     AGENT_CONTROL_AT_NEG },
            { "Strafe left",   AGENT_CONTROL_LEFT_POS },
            { "Strafe right",  AGENT_CONTROL_LEFT_NEG },
            // HACK bits consumed by LLAgent::applyExternalActionFlags for toggles.
            { "Toggle fly",    AGENT_CONTROL_FLY },          // HACK
            { "Toggle flycam", AGENT_CONTROL_NUDGE_AT_NEG }, // HACK
        };
        return bridge;
    }
}

void LLGameControllerManager::rebuildActionLookup(bool force)
{
    LLGameControl::AgentControlMode mode = g_agentControlMode;
    if (!force && mode == mLookupMode && !mAxisActionLabels.empty())
    {
        return; // already current
    }
    mLookupMode = mode;
    mAxisActionLabels.assign(LLGameControl::NUM_AXES, std::string());
    mButtonActionLabels.assign(LLGameControl::NUM_BUTTONS, std::string());

    const std::string& mode_name = modeToString(mode);
    if (mode_name.empty())
    {
        return; // CONTROL_MODE_NONE has no mappings
    }

    // Invert ModeMappings[mode][Axes|Buttons] (action label -> input name) into
    // canonical input index -> action label.  Last binding wins on collision.
    LLSD axes = LLGameControl::getModeMapping(mode_name, GC_AXES);
    for (auto it = axes.beginMap(); it != axes.endMap(); ++it)
    {
        LLGameControl::InputChannel channel = channelFromInputName(it->second.asString());
        if (channel.isAxis() && channel.mIndex < LLGameControl::NUM_AXES)
        {
            mAxisActionLabels[channel.mIndex] = it->first;
        }
    }
    LLSD buttons = LLGameControl::getModeMapping(mode_name, GC_BUTTONS);
    for (auto it = buttons.beginMap(); it != buttons.endMap(); ++it)
    {
        LLGameControl::InputChannel channel = channelFromInputName(it->second.asString());
        if (channel.isButton() && channel.mIndex < LLGameControl::NUM_BUTTONS)
        {
            mButtonActionLabels[channel.mIndex] = it->first;
        }
    }
}

U32 LLGameControllerManager::computeInternalActionFlags()
{
    // Ensure the lookup matches the active mode (cheap no-op when up to date).
    rebuildActionLookup();

    // Threshold matching LLGameControlTranslator's ON/OFF zone for analog->digital.
    constexpr U16 AXIS_THRESHOLD = 32768 / 8;

    U32 flags = 0;

    // Axes: each physical axis' two half-magnitudes (positive at axis*2, negative
    // at axis*2+1 in g_innerState) drive the bound label's positive/negative bits.
    const auto& axis_bridge = avatarAxisBridge();
    for (U8 axis = 0; axis < LLGameControl::NUM_AXES; ++axis)
    {
        const std::string& label = mAxisActionLabels[axis];
        if (label.empty())
        {
            continue;
        }
        auto it = axis_bridge.find(label);
        if (it == axis_bridge.end())
        {
            continue;
        }
        if (g_innerState.mAxes[axis * 2] > AXIS_THRESHOLD)
        {
            flags |= it->second.posFlag;
        }
        if (g_innerState.mAxes[axis * 2 + 1] > AXIS_THRESHOLD)
        {
            flags |= it->second.negFlag;
        }
    }

    // Buttons: each pressed button contributes its bound label's bit.
    const auto& button_bridge = avatarButtonBridge();
    for (U8 btn = 0; btn < LLGameControl::NUM_BUTTONS; ++btn)
    {
        if (!(g_innerState.mButtons & (1U << btn)))
        {
            continue;
        }
        const std::string& label = mButtonActionLabels[btn];
        if (label.empty())
        {
            continue;
        }
        auto it = button_bridge.find(label);
        if (it != button_bridge.end())
        {
            flags |= it->second;
        }
    }

    return flags;
}

namespace
{
    // Flycam channel packing order is defined by LLGameControl::FlycamChannel
    // (shared with LLAgent::updateFlycam()).
    struct FlycamAxisEffect { U8 channel; F32 polarity; };

    // FlyCam-mode axis label -> flycam channel + polarity applied to the axis'
    // signed value (positive half minus negative half).  "Rise up"/"Drop down"
    // are two labels on the paired triggers that accumulate into RISE with
    // opposite sign, giving the tied-trigger behavior for free.
    const std::map<std::string, FlycamAxisEffect>& flycamAxisBridge()
    {
        static const std::map<std::string, FlycamAxisEffect> bridge = {
            { "Move forward/back", { LLGameControl::FLYCAM_ADVANCE,  1.f } },
            { "Strafe left/right", { LLGameControl::FLYCAM_PAN,      1.f } },
            { "Rise up",           { LLGameControl::FLYCAM_RISE,     1.f } },
            { "Drop down",         { LLGameControl::FLYCAM_RISE,    -1.f } },
            { "Look up/down",      { LLGameControl::FLYCAM_PITCH,    1.f } },
            { "Turn left/right",   { LLGameControl::FLYCAM_YAW,      1.f } },
            { "Roll left/right",   { LLGameControl::FLYCAM_ROLL,     1.f } },
        };
        return bridge;
    }

    // FlyCam-mode button label -> flycam channel + full-deflection contribution.
    // A gamepad has no free axis for roll (all six are used for translate/look),
    // so roll is driven by the shoulder buttons by default; the dpad likewise
    // provides digital advance/pan.  Command-type buttons (Select, Interact,
    // Toggle AltZoom/menu) and the flycam on/off toggle are handled elsewhere.
    const std::map<std::string, FlycamAxisEffect>& flycamButtonBridge()
    {
        static const std::map<std::string, FlycamAxisEffect> bridge = {
            { "Roll left",    { LLGameControl::FLYCAM_ROLL,     1.f } },
            { "Roll right",   { LLGameControl::FLYCAM_ROLL,    -1.f } },
            { "Move forward", { LLGameControl::FLYCAM_ADVANCE,  1.f } },
            { "Move back",    { LLGameControl::FLYCAM_ADVANCE, -1.f } },
            { "Strafe left",  { LLGameControl::FLYCAM_PAN,      1.f } },
            { "Strafe right", { LLGameControl::FLYCAM_PAN,     -1.f } },
        };
        return bridge;
    }
}

void LLGameControllerManager::getFlycamInputs(std::vector<F32>& inputs)
{
    // Ensure the runtime lookup matches the active (FlyCam) mode.
    rebuildActionLookup();

    // Accumulate each flycam channel from the bound FlyCam-mode axes, reading the
    // canonical device state.  Values are normalized to [-1, 1] and packed in the
    // fixed order consumed by LLAgent::updateFlycam().
    std::vector<F32> dof(LLGameControl::FLYCAM_NUM_CHANNELS, 0.f);
    const auto& bridge = flycamAxisBridge();
    for (U8 axis = 0; axis < LLGameControl::NUM_AXES; ++axis)
    {
        const std::string& label = mAxisActionLabels[axis];
        if (label.empty())
        {
            continue;
        }
        auto it = bridge.find(label);
        if (it == bridge.end())
        {
            continue;
        }
        // g_innerState half-axes: positive magnitude at axis*2, negative at axis*2+1.
        F32 pos = (F32)g_innerState.mAxes[axis * 2]     / 32767.f;
        F32 neg = (F32)g_innerState.mAxes[axis * 2 + 1] / 32767.f;
        dof[it->second.channel] += it->second.polarity * (pos - neg);
    }

    // Button-driven flycam motion (roll on the shoulders, dpad advance/pan):
    // each pressed button contributes full deflection to its channel.
    const auto& button_bridge = flycamButtonBridge();
    for (U8 btn = 0; btn < LLGameControl::NUM_BUTTONS; ++btn)
    {
        if (!(g_innerState.mButtons & (1U << btn)))
        {
            continue;
        }
        const std::string& label = mButtonActionLabels[btn];
        if (label.empty())
        {
            continue;
        }
        auto it = button_bridge.find(label);
        if (it != button_bridge.end())
        {
            dof[it->second.channel] += it->second.polarity;
        }
    }

    for (F32& v : dof)
    {
        v = std::clamp(v, -1.f, 1.f);
    }
    inputs = dof;
}

void LLGameControllerManager::setExternalInput(U32 action_flags, U32 buttons)
{
    return;
    if (true)
    {
        // HACK: these are the bits we can safely translate from control flags to GameControl
        // Extracting LLGameControl::InputChannels that are mapped to other bits is a WIP.
        // TODO: translate other bits to GameControl, which might require measure of gAgent
        // state changes (e.g. sitting <--> standing, flying <--> not-flying, etc)
        const U32 BITS_OF_INTEREST =
            AGENT_CONTROL_AT_POS | AGENT_CONTROL_AT_NEG
            | AGENT_CONTROL_LEFT_POS | AGENT_CONTROL_LEFT_NEG
            | AGENT_CONTROL_UP_POS | AGENT_CONTROL_UP_NEG
            | AGENT_CONTROL_YAW_POS | AGENT_CONTROL_YAW_NEG
            | AGENT_CONTROL_PITCH_POS | AGENT_CONTROL_PITCH_NEG
            | AGENT_CONTROL_STOP
            | AGENT_CONTROL_FAST_AT
            | AGENT_CONTROL_FAST_LEFT
            | AGENT_CONTROL_FAST_UP;
        action_flags &= BITS_OF_INTEREST;

        U32 active_flags = action_flags & mActionTranslator.getMappedFlags();
        if (active_flags != mLastActiveFlags)
        {
            mLastActiveFlags = active_flags;
            mExternalState = mActionTranslator.computeStateFromFlags(action_flags);
            mExternalState.mButtons |= buttons;
        }
        else
        {
            mExternalState.mButtons = buttons;
        }
    }
    else
    {
        mExternalState.mButtons = buttons;
    }
}

void LLGameControllerManager::clear()
{
    mDevices.clear();
}

U64 get_now_nsec()
{
    std::chrono::time_point<std::chrono::steady_clock> t0;
    return (std::chrono::steady_clock::now() - t0).count();
}

void onJoystickDeviceAdded(const SDL_Event& event)
{
    std::string guid(std::to_string(SDL_GetJoystickGUIDForID(event.jdevice.which)));
    SDL_JoystickType type(SDL_GetJoystickTypeForID(event.jdevice.which));
    std::string name(ll_safe_string(SDL_GetJoystickNameForID(event.jdevice.which)));

    LL_INFOS("SDL3") << "joystick {id:" << event.jdevice.which
        << ",guid:'" << guid << "'"
        << ",type:'" << type << "'"
        << ",name:'" << name << "'"
        << "}" << LL_ENDL;

    if (SDL_Joystick* joystick = SDL_OpenJoystick(event.jdevice.which))
    {
        LL_INFOS("SDL3") << "joystick " << joystick << LL_ENDL;
        SDL_CloseJoystick(joystick);
    }
    else
    {
        LL_WARNS("SDL3") << "Can't open joystick: " << SDL_GetError() << LL_ENDL;
    }
}

void onJoystickDeviceRemoved(const SDL_Event& event)
{
    LL_INFOS("SDL3") << "joystick id: " << event.jdevice.which << LL_ENDL;
}

void onControllerDeviceAdded(const SDL_Event& event)
{
    std::string guid(std::to_string(SDL_GetGamepadGUIDForID(event.gdevice.which)));
    SDL_GamepadType type(SDL_GetGamepadTypeForID(event.gdevice.which));
    std::string name(ll_safe_string(SDL_GetGamepadNameForID(event.gdevice.which)));

    LL_INFOS("SDL3") << "controller {id:" << event.gdevice.which
        << ",guid:'" << guid << "'"
        << ",type:'" << type << "'"
        << ",name:'" << name << "'"
        << "}" << LL_ENDL;

    SDL_Gamepad* controller = SDL_OpenGamepad(event.gdevice.which);
    if (!controller)
    {
        LL_WARNS("SDL3") << "Can't open game controller: " << SDL_GetError() << LL_ENDL;
        return;
    }

    g_manager.addController(event.gdevice.which, guid, name);

    // this event could happen while the preferences UI is open
    // in which case we need to force it to update
    s_updateUI();
}

void onControllerDeviceRemoved(const SDL_Event& event)
{
    LL_INFOS("SDL3") << "joystick id=" << event.gdevice.which << LL_ENDL;

    SDL_JoystickID id = event.gdevice.which;
    g_manager.removeController(id);

    // this event could happen while the preferences UI is open
    // in which case we need to force it to update
    s_updateUI();
}

void onControllerButton(const SDL_Event& event)
{
    g_manager.onButton(event.gbutton.which, event.gbutton.button, event.gbutton.down);
}

void onControllerAxis(const SDL_Event& event)
{
    LL_DEBUGS("SDL3") << "joystick=0x" << std::hex << event.gaxis.which << std::dec
        << " axis=" << (S32)(event.gaxis.axis)
        << " value=" << (S32)(event.gaxis.value) << LL_ENDL;
    g_manager.onAxis(event.gaxis.which, event.gaxis.axis, event.gaxis.value);
}

// static
bool LLGameControl::actionFromString(const std::string& string, ActionType& actionType, U8& action)
{
    actionType = ActionType::NONE;
    if(LLStringUtil::startsWith(string, "axis_")) {
        actionType = ActionType::DOF;
        if(string == "axis_left")
        {
            action = MOVE_DIR_STRAFE_LEFT;
        }
        else if(string == "axis_right")
        {
            action = MOVE_DIR_STRAFE_RIGHT;
        }
        else if(string == "axis_forward")
        {
            action = MOVE_DIR_PUSH_FORWARD;
        }
        else if(string == "axis_backward")
        {
            action = MOVE_DIR_PUSH_BACKWARD;
        }
        else if(string == "axis_turn_left")
        {
            action = MOVE_DIR_TURN_LEFT;
        }
        else if(string == "axis_turn_right")
        {
            action = MOVE_DIR_TURN_RIGHT;
        }
        else if(string == "axis_look_up")
        {
            action = MOVE_DIR_LOOK_UP;
        }
        else if(string == "axis_look_down")
        {
            action = MOVE_DIR_LOOK_DOWN;
        }
        else if(string == "axis_up")
        {
            action = MOVE_DIR_RISE_UP;
        }
        else if(string == "axis_down")
        {
            action = MOVE_DIR_DROP_DOWN;
        }
        else if(string == "axis_roll_left")
        {
            action = MOVE_DIR_ROLL_LEFT;
        }
        else if(string == "axis_roll_right")
        {
            action = MOVE_DIR_ROLL_RIGHT;
        }
        else
        {
            actionType = ActionType::NONE;
            return false;
        }
        return true;
    }
    else if(LLStringUtil::startsWith(string, "button_")) {
        actionType = ActionType::BUTTON;
        if(string == "button_south")
        {
            action = BUTTON_SOUTH;
        }
        else if(string == "button_east")
        {
            action = BUTTON_EAST;
        }
        else if(string == "button_west")
        {
            action = BUTTON_WEST;
        }
        else if(string == "button_north")
        {
            action = BUTTON_NORTH;
        }
        else if(string == "button_back")
        {
            action = BUTTON_BACK;
        }
        else if(string == "button_start")
        {
            action = BUTTON_START;
        }
        else if(string == "button_guide")
        {
            action = BUTTON_GUIDE;
        }
        else if(string == "button_leftstick")
        {
            action = BUTTON_LEFT_STICK;
        }
        else if(string == "button_rightstick")
        {
            action = BUTTON_RIGHT_STICK;
        }
        else if(string == "button_leftshoulder")
        {
            action = BUTTON_LEFT_SHOULDER;
        }
        else if(string == "button_rightshoulder")
        {
            action = BUTTON_RIGHT_SHOULDER;
        }
        else if(string == "button_dpad_up")
        {
            action = BUTTON_DPAD_UP;
        }
        else if(string == "button_dpad_down")
        {
            action = BUTTON_DPAD_DOWN;
        }
        else if(string == "button_dpad_left")
        {
            action = BUTTON_DPAD_LEFT;
        }
        else if(string == "button_dpad_right")
        {
            action = BUTTON_DPAD_RIGHT;
        }
        else if(string == "button_misc1")
        {
            action = BUTTON_MISC1;
        }
        else if(string == "button_paddle1")
        {
            action = BUTTON_PADDLE1;
        }
        else if(string == "button_paddle2")
        {
            action = BUTTON_PADDLE2;
        }
        else if(string == "button_paddle3")
        {
            action = BUTTON_PADDLE3;
        }
        else if(string == "button_paddle4")
        {
            action = BUTTON_PADDLE4;
        }
        else if(string == "button_touchpad")
        {
            action = BUTTON_TOUCHPAD;
        }
        else
        {
            actionType = ActionType::NONE;
            return false;
        }
        return true;
    }
    return false;
}

std::string LLGameControl::stringFromAction(const ActionType actionType, const U8 action)
{
    switch (actionType)
    {
        case ActionType::DOF:
        {
            switch(action)
            {
                case MovementDirection::MOVE_DIR_STRAFE_LEFT:        return "axis_left";
                case MovementDirection::MOVE_DIR_STRAFE_RIGHT:       return "axis_right";
                case MovementDirection::MOVE_DIR_PUSH_FORWARD:     return "axis_forward";
                case MovementDirection::MOVE_DIR_PUSH_BACKWARD:    return "axis_backward";
                case MovementDirection::MOVE_DIR_TURN_LEFT:   return "axis_turn_left";
                case MovementDirection::MOVE_DIR_TURN_RIGHT:  return "axis_turn_right";
                case MovementDirection::MOVE_DIR_LOOK_UP:     return "axis_look_up";
                case MovementDirection::MOVE_DIR_LOOK_DOWN:   return "axis_look_down";
                case MovementDirection::MOVE_DIR_RISE_UP:          return "axis_up";
                case MovementDirection::MOVE_DIR_DROP_DOWN:        return "axis_down";
                case MovementDirection::MOVE_DIR_ROLL_LEFT:   return "axis_roll_left";
                case MovementDirection::MOVE_DIR_ROLL_RIGHT:  return "axis_roll_right";
            }
        }
        case ActionType::BUTTON:
        {
            switch(action)
            {
                case BUTTON_SOUTH:     return "button_south";
                case BUTTON_EAST:      return "button_east";
                case BUTTON_WEST:      return "button_west";
                case BUTTON_NORTH:     return "button_north";
                case BUTTON_BACK:           return "button_back";
                case BUTTON_START:          return "button_start";
                case BUTTON_GUIDE:          return "button_guide";
                case BUTTON_LEFT_STICK:      return "button_leftstick";
                case BUTTON_RIGHT_STICK:     return "button_rightstick";
                case BUTTON_LEFT_SHOULDER:   return "button_leftshoulder";
                case BUTTON_RIGHT_SHOULDER:  return "button_rightshoulder";
                case BUTTON_DPAD_UP:        return "button_dpad_up";
                case BUTTON_DPAD_DOWN:      return "button_dpad_down";
                case BUTTON_DPAD_LEFT:      return "button_dpad_left";
                case BUTTON_DPAD_RIGHT:     return "button_dpad_right";
                case BUTTON_MISC1:          return "button_misc1";
                case BUTTON_PADDLE1:        return "button_paddle1";
                case BUTTON_PADDLE2:        return "button_paddle2";
                case BUTTON_PADDLE3:        return "button_paddle3";
                case BUTTON_PADDLE4:        return "button_paddle4";
                case BUTTON_TOUCHPAD:       return "button_touchpad";
            }
        }
        default:
            return "";
    }
    return "";
}

std::string LLGameControl::controllerInputStringFromAction(const ActionType actionType, const U8 action)
{
    //TODO map to recent controller
    std::string aa = stringFromAction(actionType, action);
    if(actionType < 2) {
        LL_WARNS() << "MAP KEYBIND " << (S32)actionType << ":" << (S32)action << " = " << aa << LL_ENDL;
    }
    return aa;
}

// static
bool LLGameControl::isInitialized()
{
    return g_gameControl != nullptr;
}

// static
void LLGameControl::init(const std::string& gamecontrollerdb_path,
    LoadSettingsFn loadSettings,
    SaveSettingsFn saveSettings,
    std::function<void()> updateUI)
{
    if (g_gameControl)
        return;

    llassert(loadSettings);
    llassert(saveSettings);
    llassert(updateUI);

    bool result = SDL_InitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_SENSOR);
    if (!result)
    {
        // This error is critical, we stop working with SDL and return
        LL_WARNS("SDL3") << "Error initializing GameController subsystems : " << SDL_GetError() << LL_ENDL;
        return;
    }

    // The inability to read this file is not critical, we can continue working
    if (!LLFile::isfile(gamecontrollerdb_path.c_str()))
    {
        LL_WARNS("SDL3") << "Device mapping db file not found: " << gamecontrollerdb_path << LL_ENDL;
    }
    else
    {
        int count = SDL_AddGamepadMappingsFromFile(gamecontrollerdb_path.c_str());
        if (count < 0)
        {
            LL_WARNS("SDL3") << "Error adding mappings from " << gamecontrollerdb_path << " : " << SDL_GetError() << LL_ENDL;
        }
        else
        {
            LL_INFOS("SDL3") << "Total " << count << " mappings added from " << gamecontrollerdb_path << LL_ENDL;
        }
    }

    g_gameControl = LLGameControl::getInstance();

    s_loadSettings = loadSettings;
    s_saveSettings = saveSettings;
    s_updateUI = updateUI;

    loadFromSettings();
}

// static
void LLGameControl::terminate()
{
    g_manager.clear();
}

// static
const std::list<LLGameControl::Device>& LLGameControl::getDevices()
{
    return g_manager.mDevices;
}

//static
const std::map<std::string, std::string>& LLGameControl::getDeviceOptions()
{
    return g_deviceOptions;
}

//static
void LLGameControl::computeFinalState()
{
    g_manager.accumulateInternalState();
    // Note: LLGameControllerManager::computeFinalState() modifies g_nextResendPeriod as a side-effect
    g_manager.computeFinalState();
}

//static
// returns 'true' if GameControlInput message needs to go out,
// which will be the case for new data or resend. Call this right
// before deciding to put a GameControlInput packet on the wire
// or not.
bool LLGameControl::computeFinalStateAndCheckForChanges()
{
    computeFinalState();

    // should send input when:
    //     sending is enabled and
    //     g_lastSend has "expired"
    //         either because g_nextResendPeriod has been zeroed
    //         or the last send really has expired.
    return g_sendToServer && (g_lastSend + g_nextResendPeriod < get_now_nsec());
}

// static
void LLGameControl::clearAllStates()
{
    g_manager.clearAllStates();
}

// static
void LLGameControl::processEvents(bool app_has_focus)
{
    if (!gSDLMainHandled)
    {
        // This logic is used by non-linux platforms which only use SDL for GameController input
        SDL_Event event;
        while (g_gameControl && SDL_PollEvent(&event))
        {
            handleEvent(event, app_has_focus);
        }
    }

    if (!app_has_focus)
    {
        g_manager.clearAllStates();
    }
}

void LLGameControl::handleEvent(const SDL_Event& event, bool app_has_focus)
{
    switch (event.type)
    {
        case SDL_EVENT_JOYSTICK_ADDED:
            onJoystickDeviceAdded(event);
            break;
        case SDL_EVENT_JOYSTICK_REMOVED:
            onJoystickDeviceRemoved(event);
            break;
        case SDL_EVENT_GAMEPAD_ADDED:
            onControllerDeviceAdded(event);
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            onControllerDeviceRemoved(event);
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            /* FALLTHROUGH */
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            if (app_has_focus)
            {
                onControllerButton(event);
            }
            break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            if (app_has_focus)
            {
                onControllerAxis(event);
            }
            break;
        default:
            break;
    }
}

// static
const LLGameControl::ServerState& LLGameControl::getServerState()
{
    return g_finalState;
}

const LLGameControl::State& LLGameControl::getState()
{
    return g_innerState;
}

// static
LLGameControl::InputChannel LLGameControl::getActiveInputChannel()
{
    InputChannel input;

    ServerState state = g_finalState;
    if (state.mButtons > 0)
    {
        // check buttons
        input.mType = LLGameControl::InputChannel::TYPE_BUTTON;
        for (U8 i = 0; i < 32; ++i)
        {
            if ((0x1u << i) & state.mButtons)
            {
                input.mIndex = i;
                break;
            }
        }
    }
    else
    {
        // scan axes
        constexpr S16 threshold = std::numeric_limits<S16>::max() / 2;
        for (U8 i = 0; i < 6; ++i)
        {
            if (abs(state.mAxes[i]) > threshold)
            {
                input.mType = LLGameControl::InputChannel::TYPE_AXIS;
                // input.mIndex ultimately translates to a LLGameControl::KeyboardAxis
                // which distinguishes between negative and positive directions
                // so we must translate to axis index "i" according to the sign
                // of the axis value.
                input.mIndex = i;
                input.mSign = state.mAxes[i] > 0 ? 1 : -1;
                break;
            }
        }
    }

    return input;
}

// static
void LLGameControl::getFlycamInputs(std::vector<F32>& inputs_out)
{
    return g_manager.getFlycamInputs(inputs_out);
}

// static
void LLGameControl::setSendToServer(bool enable)
{
    g_sendToServer = enable;
    // SendDataToServer lives inside the GameControl map; it is persisted along with
    // the rest of that map (mode mappings, device config) when settings are saved.
    ensureGameControlSettings();
    g_gameControlSettings[GC_SENDTOSERVER] = g_sendToServer;
}

// static
bool LLGameControl::sendToServer()
{
    return g_sendToServer;
}

// static
void LLGameControl::setAgentControlMode(AgentControlMode mode)
{
    // AgentControlMode is purely runtime state, auto-derived from avatar state
    // every frame (see LLAgent::updateGameControlMode).  It is intentionally not
    // persisted: the panel's "action_mode" selector only chooses which mode's
    // mappings are shown for editing, not the live mode.
    g_agentControlMode = mode;
}

// static
LLGameControl::AgentControlMode LLGameControl::getAgentControlMode()
{
    return g_agentControlMode;
}

// static
bool LLGameControl::isEnabled()
{
    // The feature no longer has a separate on/off toggle; it is "enabled"
    // whenever at least one controller is connected.
    return !g_manager.mDevices.empty();
}

// static
bool LLGameControl::willControlAvatar()
{
    return isEnabled()
        && (g_agentControlMode == CONTROL_MODE_AVATAR
            || g_agentControlMode == CONTROL_MODE_CAPTIVE);
}

// static
bool LLGameControl::willControlFlycam()
{
    return isEnabled() && g_agentControlMode == CONTROL_MODE_FLYCAM;
}

// static
std::string LLGameControl::getModeName(AgentControlMode mode)
{
    return modeToString(mode);
}

// static
LLSD LLGameControl::getDefaultModeMappings()
{
    return buildDefaultModeMappings();
}

// static
LLSD LLGameControl::getDefaultGameControlSettings()
{
    return buildDefaultGameControlSettings();
}

// static
const LLSD& LLGameControl::getGameControlSettings()
{
    ensureGameControlSettings();
    return g_gameControlSettings;
}

// static
void LLGameControl::setGameControlSettings(const LLSD& settings)
{
    g_gameControlSettings = settings;
    ensureGameControlSettings();
    // Mappings may have changed: refresh the runtime action lookup immediately.
    g_manager.rebuildActionLookup(true);
}

// static
LLSD LLGameControl::getModeMapping(const std::string& mode, const std::string& kind)
{
    ensureGameControlSettings();
    LLSD mapping = g_gameControlSettings[GC_MODEMAPPINGS][mode][kind];
    if (!mapping.isMap())
    {
        // Fall back to the built-in defaults for this mode/kind.
        mapping = buildDefaultModeMappings()[mode][kind];
    }
    return mapping;
}

// static
void LLGameControl::setModeMapping(const std::string& mode, const std::string& kind,
    const LLSD& mapping)
{
    ensureGameControlSettings();
    g_gameControlSettings[GC_MODEMAPPINGS][mode][kind] = mapping;
    // Take effect immediately (e.g. panel "reset to defaults"), without waiting
    // for OK: refresh the runtime action lookup for the active mode.
    g_manager.rebuildActionLookup(true);
}

// static
void LLGameControl::updateModeMapping(const std::string& mode, const std::string& kind,
    const std::string& action, const std::string& input)
{
    ensureGameControlSettings();
    g_gameControlSettings[GC_MODEMAPPINGS][mode][kind][action] = input;
    // Take effect immediately (live panel edit), without waiting for OK: refresh
    // the runtime action lookup for the active mode.
    g_manager.rebuildActionLookup(true);
}

// static
std::string LLGameControl::getDeviceConfig(const std::string& guid)
{
    ensureGameControlSettings();
    const LLSD& devices = g_gameControlSettings[GC_DEVICES];
    return devices.has(guid) ? devices[guid][GC_CONFIG].asString() : LLStringUtil::null;
}

// static
void LLGameControl::setDeviceConfig(const std::string& guid, const std::string& config)
{
    ensureDeviceEntry(guid);
    g_gameControlSettings[GC_DEVICES][guid][GC_CONFIG] = config;
}


// static
LLGameControl::ActionNameType LLGameControl::getActionNameType(const std::string& action)
{
    return g_manager.getActionNameType(action);
}

// static
//
// Given a name like "AXIS_1-" or "BUTTON_5" returns the corresponding InputChannel
// If the axis name lacks the +/- postfix it assumes '+' postfix.
LLGameControl::InputChannel LLGameControl::getChannelByName(const std::string& name)
{
    LLGameControl::InputChannel channel;

    // 'name' has two acceptable formats: AXIS_<index>[sign] or BUTTON_<index>
    if (LLStringUtil::startsWith(name, "AXIS_"))
    {
        channel.mType = LLGameControl::InputChannel::Type::TYPE_AXIS;
        // Decimal postfix is only one character
        channel.mIndex = atoi(name.substr(5, 1).c_str());
        // AXIS_n can have an optional +/- at index 6
        // Assume positive axis when sign not provided
        channel.mSign = name.back() == '-' ? -1 : 1;
    }
    else if (LLStringUtil::startsWith(name, "BUTTON_"))
    {
        channel.mType = LLGameControl::InputChannel::Type::TYPE_BUTTON;
        // Decimal postfix is only one or two characters
        channel.mIndex = atoi(name.substr(7).c_str());
    }

    return channel;
}

// static
U32 LLGameControl::computeInternalActionFlags()
{
    return g_manager.computeInternalActionFlags();
}

// static
void LLGameControl::setExternalInput(U32 action_flags, U32 buttons)
{
    g_manager.setExternalInput(action_flags, buttons);
}

//static
void LLGameControl::updateResendPeriod()
{
    // we expect this method to be called right after data is sent
    g_lastSend = get_now_nsec();
    if (g_nextResendPeriod == 0)
    {
        g_nextResendPeriod = FIRST_RESEND_PERIOD;
    }
    else
    {
        // Reset mPrevAxes only on second resend or higher
        // because when the joysticks are being used we expect a steady stream
        // of recorrection data rather than sparse changes.
        //
        // (The above assumption is not necessarily true for "Actions" input
        // (e.g. keyboard events).  TODO: figure out what to do about this.)
        //
        // In other words: we want to include changed axes in the first resend
        // so we only overwrite g_finalState.mPrevAxes on higher resends.
        g_finalState.mPrevAxes = g_finalState.mAxes;
        g_nextResendPeriod *= RESEND_EXPANSION_RATE;
    }
}

// static
bool LLGameControl::parseDeviceOptions(const std::string& options, std::string& name,
    std::vector<LLGameControl::Options::AxisOptions>& axis_options,
    std::vector<U8>& axis_map, std::vector<U8>& button_map)
{
    if (options.empty())
        return false;

    name.clear();
    axis_options.resize(NUM_AXES);
    axis_map.resize(NUM_AXES);
    button_map.resize(NUM_BUTTONS);

    for (size_t i = 0; i < NUM_AXES; ++i)
    {
        axis_options[i].resetToDefaults();
        axis_map[i] = (U8)i;
    }

    for (size_t i = 0; i < NUM_BUTTONS; ++i)
    {
        button_map[i] = (U8)i;
    }

    std::map<std::string, std::string> pairs;
    if (!parse(pairs, options))
    {
        LL_WARNS("SDL3") << "Invalid options: '" << options << "'" << LL_ENDL;
        return false;
    }

    std::map<std::string, std::string> axis_string_options;
    if (!parse(axis_string_options, pairs["axis_options"]))
    {
        LL_WARNS("SDL3") << "Invalid axis_options: '" << pairs["axis_options"] << "'" << LL_ENDL;
        return false;
    }

    std::map<std::string, std::string> axis_string_map;
    if (!parse(axis_string_map, pairs["axis_map"]))
    {
        LL_WARNS("SDL3") << "Invalid axis_map: '" << pairs["axis_map"] << "'" << LL_ENDL;
        return false;
    }

    std::map<std::string, std::string> button_string_map;
    if (!parse(button_string_map, pairs["button_map"]))
    {
        LL_WARNS("SDL3") << "Invalid button_map: '" << pairs["button_map"] << "'" << LL_ENDL;
        return false;
    }

    name = pairs["name"];

    for (size_t i = 0; i < NUM_AXES; ++i)
    {
        std::string key = std::to_string(i);

        std::string one_axis_options = axis_string_options[key];
        if (!one_axis_options.empty())
        {
            axis_options[i].loadFromString(one_axis_options);
        }

        std::string value = axis_string_map[key];
        if (!value.empty())
        {
            size_t number = std::stoull(value);
            if (number >= NUM_AXES || std::to_string(number) != value)
            {
                LL_WARNS("SDL3") << "Invalid axis mapping: " << i << "->" << value << LL_ENDL;
            }
            else
            {
                axis_map[i] = (U8)number;
            }
        }
    }

    for (size_t i = 0; i < NUM_BUTTONS; ++i)
    {
        std::string value = button_string_map[std::to_string(i)];
        if (!value.empty())
        {
            size_t number = std::stoull(value);
            if (number >= NUM_BUTTONS || std::to_string(number) != value)
            {
                LL_WARNS("SDL3") << "Invalid button mapping: " << i << "->" << value << LL_ENDL;
            }
            else
            {
                button_map[i] = (U8)number;
            }
        }
    }

    return true;
}

// static
std::string LLGameControl::stringifyDeviceOptions(const std::string& name,
    const std::vector<LLGameControl::Options::AxisOptions>& axis_options,
    const std::vector<U8>& axis_map, const std::vector<U8>& button_map,
    bool force_empty)
{
    std::list<std::string> options;

    auto opts2str = [](size_t i, const Options::AxisOptions& options) -> std::string
        {
            std::string string = options.saveToString();
            return string.empty() ? string : llformat("%u:%s", i, string.c_str());
        };

    std::string axis_options_string = LLStringUtil::join<std::vector<Options::AxisOptions>, Options::AxisOptions>(axis_options, opts2str);
    if (!axis_options_string.empty())
    {
        options.push_back("axis_options:{" + axis_options_string + "}");
    }

    auto map2str = [](size_t index, const U8& value) -> std::string
        {
            return value == index ? LLStringUtil::null : llformat("%u:%u", index, value);
        };

    std::string axis_map_string = LLStringUtil::join<std::vector<U8>, U8>(axis_map, map2str);
    if (!axis_map_string.empty())
    {
        options.push_back("axis_map:{" + axis_map_string + "}");
    }

    std::string button_map_string = LLStringUtil::join<std::vector<U8>, U8>(button_map, map2str);
    if (!button_map_string.empty())
    {
        options.push_back("button_map:{" + button_map_string + "}");
    }

    if (!force_empty && options.empty())
        return LLStringUtil::null;

    // Remove control characters [',', '{', '}'] from name
    std::string safe_name;
    safe_name.reserve(name.size());
    for (char c : name)
    {
        if (c != ',' && c != '{' && c != '}')
        {
            safe_name.push_back(c);
        }
    }
    options.push_front(llformat("name:%s", safe_name.c_str()));

    std::string result = LLStringUtil::join(options);

    return "{" + result + "}";
}

// static
void LLGameControl::initByDefault()
{
    g_sendToServer = false;
    g_agentControlMode = CONTROL_MODE_AVATAR;
    g_gameControlSettings = buildDefaultGameControlSettings();
    g_manager.resetDeviceOptionsToDefaults();
    g_deviceOptions.clear();
}

// static
const std::vector<std::string>& LLGameControl::getSettingKeys()
{
    static const std::vector<std::string> keys = {
        SETTING_GAMECONTROL
    };
    return keys;
}

// static
LLSD LLGameControl::getSettingsAsLLSD()
{
    ensureGameControlSettings();

    // Fold each device's serialized options into GameControl/Devices/<guid>/Config.
    g_manager.rememberDeviceOptions();  // refresh g_deviceOptions from current device state
    for (const auto& [guid, options] : g_deviceOptions)
    {
        if (!options.empty())
        {
            g_gameControlSettings[GC_DEVICES][guid][GC_CONFIG] = options;
        }
    }

    g_gameControlSettings[GC_SENDTOSERVER] = g_sendToServer;

    LLSD result = LLSD::emptyMap();
    result[SETTING_GAMECONTROL] = g_gameControlSettings;
    return result;
}

// static
void LLGameControl::applySettingsFromLLSD(const LLSD& settings)
{
    if (settings.has(SETTING_GAMECONTROL))       g_gameControlSettings = settings[SETTING_GAMECONTROL];
    ensureGameControlSettings();

    // SendDataToServer is nested inside the GameControl map.
    if (g_gameControlSettings.has(GC_SENDTOSERVER))
        g_sendToServer = g_gameControlSettings[GC_SENDTOSERVER].asBoolean();

    // Rebuild the device-options cache from GameControl/Devices/<guid>/Config and
    // apply it to the connected devices.
    g_deviceOptions.clear();
    const LLSD& devices = g_gameControlSettings[GC_DEVICES];
    for (auto it = devices.beginMap(); it != devices.endMap(); ++it)
    {
        if (it->first == GC_DEFAULT_DEVICE)
            continue;  // the "Default" template is not a real device
        std::string config = it->second[GC_CONFIG].asString();
        if (!config.empty())
        {
            g_deviceOptions.emplace(it->first, config);
        }
    }
    g_manager.applyRememberedDeviceOptions();

    // ModeMappings may have changed (e.g. live edits in the preferences panel);
    // force the runtime action lookup to rebuild for the active mode.
    g_manager.rebuildActionLookup(true);
}

// static
void LLGameControl::loadFromSettings()
{
    applySettingsFromLLSD(s_loadSettings(getSettingKeys()));
}

// static
void LLGameControl::saveToSettings()
{
    s_saveSettings(getSettingsAsLLSD());
}

// static
void LLGameControl::setDeviceOptions(const std::string& guid, const Options& options)
{
    g_manager.setDeviceOptions(guid, options);
}

static bool mapLocalStringToTypeAndIndex(const std::string control, LLGameControl::InputChannel::Type& cType, U8& cIndex)
{
    // HACK: needs proper method to map strings to Input Types and Indexes
    for(U8 i = 0;i < 8;i++)
    {
        if(control == ("AXIS_" + std::to_string((U32)i)))
        {
            cIndex = i;
            cType = LLGameControl::InputChannel::Type::TYPE_AXIS;
            return true;
        }
    }

    for(U8 i = 0;i < 32;i++)
    {
        if(control == ("BUTTON_" + std::to_string((U32)i)))
        {
            cIndex = i;
            cType = LLGameControl::InputChannel::Type::TYPE_BUTTON;
            return true;
        }
    }
    // /HACK
    return false;
}

// virtual, from LLGameControllerBindingToStringHandler
std::string LLGameControl::getBindingAsString(const std::string& control) const
{
    if(!hasHandlingDevice()) {
        return std::string();
    }

    InputChannel::Type cType = InputChannel::Type::TYPE_NONE;
    U8 cIndex = 255;
    if(!mapLocalStringToTypeAndIndex(control, cType, cIndex))
    {
        return std::string();
    }

    if(cType == InputChannel::Type::TYPE_NONE)
    {
        return std::string();
    }

    const LLGameControl::Device* device = g_manager.getLastActiveDevice();

    if(!device)
    {
        return std::string();
    }

    SDL_Gamepad* gp = SDL_GetGamepadFromID(device->getJoystickID());

    if(cType == InputChannel::Type::TYPE_AXIS)
    {
        U8 axis = device->getOptions().unmapAxis(cIndex);
        return std::to_string((SDL_GamepadAxis)axis);
    }
    else
    {
        U8 button = device->getOptions().unmapButton(cIndex);
        if(button < 4)
        {
            return std::to_string(SDL_GetGamepadButtonLabel(gp, (SDL_GamepadButton)button));
        }
        return std::to_string((SDL_GamepadButton)button);

    }
}

// virtual, from LLGameControllerBindingToStringHandler
bool LLGameControl::hasHandlingDevice() const
{
    return !g_manager.mDevices.empty();
}

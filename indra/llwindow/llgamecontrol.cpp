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

#include "SDL2/SDL.h"
#include "SDL2/SDL_gamecontroller.h"
#include "SDL2/SDL_joystick.h"

#include "indra_constants.h"
#include "llfile.h"
#include "llgamecontroltranslator.h"
#include "llsd.h"

namespace std
{
    string to_string(const SDL_JoystickGUID& guid)
    {
        char buffer[33] = { 0 };
        SDL_JoystickGetGUIDString(guid, buffer, sizeof(guid));
        return buffer;
    }

    string to_string(SDL_JoystickType type)
    {
        switch (type)
        {
        case SDL_JOYSTICK_TYPE_GAMECONTROLLER:
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

    string to_string(SDL_GameControllerType type)
    {
        switch (type)
        {
        case SDL_CONTROLLER_TYPE_XBOX360:
            return "XBOX360";
        case SDL_CONTROLLER_TYPE_XBOXONE:
            return "XBOXONE";
        case SDL_CONTROLLER_TYPE_PS3:
            return "PS3";
        case SDL_CONTROLLER_TYPE_PS4:
            return "PS4";
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
            return "NINTENDO_SWITCH_PRO";
        case SDL_CONTROLLER_TYPE_VIRTUAL:
            return "VIRTUAL";
        case SDL_CONTROLLER_TYPE_PS5:
            return "PS5";
        case SDL_CONTROLLER_TYPE_AMAZON_LUNA:
            return "AMAZON_LUNA";
        case SDL_CONTROLLER_TYPE_GOOGLE_STADIA:
            return "GOOGLE_STADIA";
        case SDL_CONTROLLER_TYPE_NVIDIA_SHIELD:
            return "NVIDIA_SHIELD";
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
            return "NINTENDO_SWITCH_JOYCON_LEFT";
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
            return "NINTENDO_SWITCH_JOYCON_RIGHT";
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
            return "NINTENDO_SWITCH_JOYCON_PAIR";
        default:;
        }
        return "UNKNOWN";
    }
}

// Util for dumping SDL_JoystickGUID info
std::ostream& operator<<(std::ostream& out, SDL_JoystickGUID& guid)
{
    return out << std::to_string(guid);
}

// Util for dumping SDL_JoystickType type name
std::ostream& operator<<(std::ostream& out, SDL_JoystickType type)
{
    return out << std::to_string(type);
}

// Util for dumping SDL_GameControllerType type name
std::ostream& operator<<(std::ostream& out, SDL_GameControllerType type)
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

        ss << "{id:" << SDL_JoystickInstanceID(joystick);
        SDL_JoystickGUID guid = SDL_JoystickGetGUID(joystick);
        ss << ",guid:'" << guid << "'";
        ss << ",type:'" << SDL_JoystickGetType(joystick) << "'";
        ss << ",name:'" << ll_safe_string(SDL_JoystickName(joystick)) << "'";
        ss << ",vendor:" << SDL_JoystickGetVendor(joystick);
        ss << ",product:" << SDL_JoystickGetProduct(joystick);
        if (U16 version = SDL_JoystickGetProductVersion(joystick))
        {
            ss << ",version:" << version;
        }
        if (const char* serial = SDL_JoystickGetSerial(joystick))
        {
            ss << ",serial:'" << serial << "'";
        }
        ss << ",num_axes:" << SDL_JoystickNumAxes(joystick);
        ss << ",num_balls:" << SDL_JoystickNumBalls(joystick);
        ss << ",num_hats:" << SDL_JoystickNumHats(joystick);
        ss << ",num_buttons:" << SDL_JoystickNumHats(joystick);
        ss << "}";

        return ss.str();
    }

    string to_string(SDL_GameController* controller)
    {
        if (!controller)
        {
            return "nullptr";
        }

        stringstream ss;

        ss << "{type:'" << SDL_GameControllerGetType(controller) << "'";
        ss << ",name:'" << ll_safe_string(SDL_GameControllerName(controller)) << "'";
        ss << ",vendor:" << SDL_GameControllerGetVendor(controller);
        ss << ",product:" << SDL_GameControllerGetProduct(controller);
        if (U16 version = SDL_GameControllerGetProductVersion(controller))
        {
            ss << ",version:" << version;
        }
        if (const char* serial = SDL_GameControllerGetSerial(controller))
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
std::ostream& operator<<(std::ostream& out, SDL_GameController* controller)
{
    return out << std::to_string(controller);
}

std::string LLGameControl::InputChannel::getLocalName() const
{
    // HACK: we hard-code English channel names, but
    // they should be loaded from localized XML config files.

    if ((mType == LLGameControl::InputChannel::TYPE_AXIS) && (mIndex < NUM_AXES))
    {
        return "AXIS_" + std::to_string((U32)mIndex) +
            (mSign < 0 ? "-" : mSign > 0 ? "+" : "");
    }

    if ((mType == LLGameControl::InputChannel::TYPE_BUTTON) && (mIndex < NUM_BUTTONS))
    {
        return "BUTTON_" + std::to_string((U32)mIndex);
    }

    return "NONE";
}

std::string LLGameControl::InputChannel::getRemoteName() const
{
    // HACK: we hard-code English channel names, but
    // they should be loaded from localized XML config files.
    std::string name = " ";
    // GAME_CONTROL_AXIS_LEFTX, GAME_CONTROL_BUTTON_A, etc
    if (mType == LLGameControl::InputChannel::TYPE_AXIS)
    {
        switch (mIndex)
        {
            case 0:
                name = "GAME_CONTROL_AXIS_LEFTX";
                break;
            case 1:
                name = "GAME_CONTROL_AXIS_LEFTY";
                break;
            case 2:
                name = "GAME_CONTROL_AXIS_RIGHTX";
                break;
            case 3:
                name = "GAME_CONTROL_AXIS_RIGHTY";
                break;
            case 4:
                name = "GAME_CONTROL_AXIS_PADDLELEFT";
                break;
            case 5:
                name = "GAME_CONTROL_AXIS_PADDLERIGHT";
                break;
            default:
                break;
        }
    }
    else if (mType == LLGameControl::InputChannel::TYPE_BUTTON)
    {
        switch(mIndex)
        {
            case 0:
                name = "GAME_CONTROL_BUTTON_A";
                break;
            case 1:
                name = "GAME_CONTROL_BUTTON_B";
                break;
            case 2:
                name = "GAME_CONTROL_BUTTON_X";
                break;
            case 3:
                name = "GAME_CONTROL_BUTTON_Y";
                break;
            case 4:
                name = "GAME_CONTROL_BUTTON_BACK";
                break;
            case 5:
                name = "GAME_CONTROL_BUTTON_GUIDE";
                break;
            case 6:
                name = "GAME_CONTROL_BUTTON_START";
                break;
            case 7:
                name = "GAME_CONTROL_BUTTON_LEFTSTICK";
                break;
            case 8:
                name = "GAME_CONTROL_BUTTON_RIGHTSTICK";
                break;
            case 9:
                name = "GAME_CONTROL_BUTTON_LEFTSHOULDER";
                break;
            case 10:
                name = "GAME_CONTROL_BUTTON_RIGHTSHOULDER";
                break;
            case 11:
                name = "GAME_CONTROL_BUTTON_DPAD_UP";
                break;
            case 12:
                name = "GAME_CONTROL_BUTTON_DPAD_DOWN";
                break;
            case 13:
                name = "GAME_CONTROL_BUTTON_DPAD_LEFT";
                break;
            case 14:
                name = "GAME_CONTROL_BUTTON_DPAD_RIGHT";
                break;
            case 15:
                name = "GAME_CONTROL_BUTTON_MISC1";
                break;
            case 16:
                name = "GAME_CONTROL_BUTTON_PADDLE1";
                break;
            case 17:
                name = "GAME_CONTROL_BUTTON_PADDLE2";
                break;
            case 18:
                name = "GAME_CONTROL_BUTTON_PADDLE3";
                break;
            case 19:
                name = "GAME_CONTROL_BUTTON_PADDLE4";
                break;
            case 20:
                name = "GAME_CONTROL_BUTTON_TOUCHPAD";
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

    static void getDefaultMappings(std::vector<std::pair<std::string, LLGameControl::InputChannel>>& agent_channels,
        std::vector<LLGameControl::InputChannel>& flycam_channels);
    void getDefaultMappings(std::vector<std::pair<std::string, LLGameControl::InputChannel>>& mappings);
    void initializeMappingsByDefault();
    void resetDeviceOptionsToDefaults();
    void loadDeviceOptionsFromSettings();
    void saveDeviceOptionsToSettings() const;
    void setDeviceOptions(const std::string& guid, const LLGameControl::Options& options);

    void addController(SDL_JoystickID id, const std::string& guid, const std::string& name);
    void removeController(SDL_JoystickID id);

    void onAxis(SDL_JoystickID id, U8 axis, S16 value);
    void onButton(SDL_JoystickID id, U8 button, bool pressed);

    void clearAllStates();

    void accumulateInternalState();
    void computeFinalState();

    LLGameControl::ActionNameType getActionNameType(const std::string& action) const;
    LLGameControl::InputChannel getChannelByAction(const std::string& action) const;
    LLGameControl::InputChannel getFlycamChannelByAction(const std::string& action) const;

    bool updateActionMap(const std::string& name,  LLGameControl::InputChannel channel);
    U32 computeInternalActionFlags();
    void getFlycamInputs(std::vector<F32>& inputs_out);
    void setExternalInput(U32 action_flags, U32 buttons);

    U32 getMappedFlags() const { return mActionTranslator.getMappedFlags(); }

    void clear();

    std::string getAnalogMappings() const;
    std::string getBinaryMappings() const;
    std::string getFlycamMappings() const;

    void setAnalogMappings(const std::string& mappings);
    void setBinaryMappings(const std::string& mappings);
    void setFlycamMappings(const std::string& mappings);

private:
    void updateFlycamMap(const std::string& action, LLGameControl::InputChannel channel);

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
    std::vector <std::string> mAnalogActions;
    std::vector <std::string> mBinaryActions;
    std::vector <std::string> mFlycamActions;
    std::vector<LLGameControl::InputChannel> mFlycamChannels;
    std::vector<S32> mAxesAccumulator;
    U32 mButtonAccumulator { 0 };
    U32 mLastActiveFlags { 0 };
    U32 mLastFlycamActionFlags { 0 };

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
    LLGameControl::State g_outerState; // from controller devices
    LLGameControl::State g_innerState; // state from gAgent
    LLGameControl::State g_finalState; // sum of inner and outer
    U64 g_lastSend = 0;
    U64 g_nextResendPeriod = FIRST_RESEND_PERIOD;

    bool g_enabled = false;
    bool g_sendToServer = false;
    bool g_controlAgent = false;
    bool g_translateAgentActions = false;
    LLGameControl::AgentControlMode g_agentControlMode = LLGameControl::CONTROL_MODE_AVATAR;

    std::map<std::string, std::string> g_deviceOptions;

    std::function<bool(const std::string&)> s_loadBoolean;
    std::function<void(const std::string&, bool)> s_saveBoolean;
    std::function<std::string(const std::string&)> s_loadString;
    std::function<void(const std::string&, const std::string&)> s_saveString;
    std::function<LLSD(const std::string&)> s_loadObject;
    std::function<void(const std::string&, LLSD&)> s_saveObject;
    std::function<void()> s_updateUI;

    std::string SETTING_ENABLE("EnableGameControl");
    std::string SETTING_SENDTOSERVER("GameControlToServer");
    std::string SETTING_CONTROLAGENT("GameControlToAgent");
    std::string SETTING_TRANSLATEACTIONS("AgentToGameControl");
    std::string SETTING_AGENTCONTROLMODE("AgentControlMode");
    std::string SETTING_ANALOGMAPPINGS("AnalogChannelMappings");
    std::string SETTING_BINARYMAPPINGS("BinaryChannelMappings");
    std::string SETTING_FLYCAMMAPPINGS("FlycamChannelMappings");
    std::string SETTING_KNOWNCONTROLLERS("KnownGameControllers");

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

    const std::string& getDeviceOptionsString(const std::string& guid)
    {
        const auto& it = g_deviceOptions.find(guid);
        return it == g_deviceOptions.end() ? LLStringUtil::null : it->second;
    }
}

LLGameControl::~LLGameControl()
{
    terminate();
}

LLGameControl::State::State()
: mButtons(0)
{
    mAxes.resize(NUM_AXES, 0);
    mPrevAxes.resize(NUM_AXES, 0);
}

void LLGameControl::State::clear()
{
    std::fill(mAxes.begin(), mAxes.end(), 0);

    // DO NOT clear mPrevAxes because those are managed by external logic.
    //std::fill(mPrevAxes.begin(), mPrevAxes.end(), 0);

    mButtons = 0;
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

LLGameControl::Device::Device(int joystickID, const std::string& guid, const std::string& name)
: mJoystickID(joystickID)
, mGUID(guid)
, mName(name)
{
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
        LL_WARNS("SDL2") << "Invalid input axis: " << axis << LL_ENDL;
        return axis;
    }
    return mAxisMap[axis];
}

U8 LLGameControl::Options::mapButton(U8 button) const
{
    if (button >= NUM_BUTTONS)
    {
        LL_WARNS("SDL2") << "Invalid input button: " << button << LL_ENDL;
        return button;
    }
    return mButtonMap[button];
}

S16 LLGameControl::Options::fixAxisValue(U8 axis, S16 value) const
{
    if (axis >= NUM_AXES)
    {
        LL_WARNS("SDL2") << "Invalid input axis: " << axis << LL_ENDL;
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
        LL_WARNS("SDL2") << "Invalid axis options: '" << options << "'" << LL_ENDL;
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
            LL_WARNS("SDL2") << "Invalid invert value: '" << invert << "'" << LL_ENDL;
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
            LL_WARNS("SDL2") << "Invalid dead_zone value: '" << dead_zone << "'" << LL_ENDL;
        }
    }

    std::string offset = pairs["offset"];
    if (!offset.empty())
    {
        S32 number = std::stoi(offset);
        if (abs(number) > MAX_AXIS_OFFSET || std::to_string(number) != offset)
        {
            LL_WARNS("SDL2") << "Invalid offset value: '" << offset << "'" << LL_ENDL;
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
    mAxesAccumulator.resize(LLGameControl::NUM_AXES, 0);

    mAnalogActions = { "push", "slide", "jump", "turn", "look" };
    mBinaryActions = { "toggle_run", "toggle_fly", "toggle_flycam", "stop" };
    mFlycamActions = { "advance", "pan", "rise", "pitch", "yaw", "zoom" };

    // Collect all known action names with their types in one container
    for (const std::string& name : mAnalogActions)
    {
        mActions[name] = LLGameControl::ACTION_NAME_ANALOG;
        mActions[name + "+"] = LLGameControl::ACTION_NAME_ANALOG_POS;
        mActions[name + "-"] = LLGameControl::ACTION_NAME_ANALOG_NEG;
    }
    for (const std::string& name : mBinaryActions)
    {
        mActions[name] = LLGameControl::ACTION_NAME_BINARY;
    }
    for (const std::string& name : mFlycamActions)
    {
        mActions[name] = LLGameControl::ACTION_NAME_FLYCAM;
    }

    // Here we build an invariant map between the named agent actions
    // and control bit sent to the server. This map will be used,
    // in combination with the action->InputChannel map below,
    // to maintain an inverse map from control bit masks to GameControl data.
    LLGameControlTranslator::ActionToMaskMap actionMasks =
    {
    // Analog actions (pairs)
        { "push+",  AGENT_CONTROL_AT_POS    | AGENT_CONTROL_FAST_AT   },
        { "push-",  AGENT_CONTROL_AT_NEG    | AGENT_CONTROL_FAST_AT   },
        { "slide+", AGENT_CONTROL_LEFT_POS  | AGENT_CONTROL_FAST_LEFT },
        { "slide-", AGENT_CONTROL_LEFT_NEG  | AGENT_CONTROL_FAST_LEFT },
        { "jump+",  AGENT_CONTROL_UP_POS    | AGENT_CONTROL_FAST_UP   },
        { "jump-",  AGENT_CONTROL_UP_NEG    | AGENT_CONTROL_FAST_UP   },
        { "turn+",  AGENT_CONTROL_YAW_POS   },
        { "turn-",  AGENT_CONTROL_YAW_NEG   },
        { "look+",  AGENT_CONTROL_PITCH_POS },
        { "look-",  AGENT_CONTROL_PITCH_NEG },
    // Button actions
        { "stop",   AGENT_CONTROL_STOP      },
    // These are HACKs. We borrow some AGENT_CONTROL bits for "unrelated" features.
    // Not a problem because these bits are only used internally.
        { "toggle_run",    AGENT_CONTROL_NUDGE_AT_POS }, // HACK
        { "toggle_fly",    AGENT_CONTROL_FLY          }, // HACK
        { "toggle_flycam", AGENT_CONTROL_NUDGE_AT_NEG }, // HACK
    };
    mActionTranslator.setAvailableActionMasks(actionMasks);

    initializeMappingsByDefault();
}

// static
void LLGameControllerManager::getDefaultMappings(
    std::vector<std::pair<std::string, LLGameControl::InputChannel>>& agent_channels,
    std::vector<LLGameControl::InputChannel>& flycam_channels)
{
    // Here we build a list of pairs between named agent actions and
    // GameControl channels. Note: we only supply the non-signed names
    // (e.g. "push" instead of "push+" and "push-") because mActionTranslator
    // automatially expands action names as necessary.
    using type = LLGameControl::InputChannel::Type;
    agent_channels =
    {
    // Analog actions (associated by common name - without '+' or '-')
        { "push",  { type::TYPE_AXIS,   LLGameControl::AXIS_LEFTY,       1 } },
        { "slide", { type::TYPE_AXIS,   LLGameControl::AXIS_LEFTX,       1 } },
        { "jump",  { type::TYPE_AXIS,   LLGameControl::AXIS_TRIGGERLEFT, 1 } },
        { "turn",  { type::TYPE_AXIS,   LLGameControl::AXIS_RIGHTX,      1 } },
        { "look",  { type::TYPE_AXIS,   LLGameControl::AXIS_RIGHTY,      1 } },
    // Button actions (associated by name)
        { "toggle_run",    { type::TYPE_BUTTON, LLGameControl::BUTTON_LEFTSHOULDER }  },
        { "toggle_fly",    { type::TYPE_BUTTON, LLGameControl::BUTTON_DPAD_UP }       },
        { "toggle_flycam", { type::TYPE_BUTTON, LLGameControl::BUTTON_RIGHTSHOULDER } },
        { "stop",          { type::TYPE_BUTTON, LLGameControl::BUTTON_LEFTSTICK }     }
    };

    // Flycam actions don't need bitwise translation, so we maintain the map
    // of channels here directly rather than using an LLGameControlTranslator.
    flycam_channels =
    {
    // Flycam actions (associated just by an order index)
        { type::TYPE_AXIS, LLGameControl::AXIS_LEFTY,        1 }, // advance
        { type::TYPE_AXIS, LLGameControl::AXIS_LEFTX,        1 }, // pan
        { type::TYPE_AXIS, LLGameControl::AXIS_TRIGGERRIGHT, 1 }, // rise
        { type::TYPE_AXIS, LLGameControl::AXIS_RIGHTY,      -1 }, // pitch
        { type::TYPE_AXIS, LLGameControl::AXIS_RIGHTX,       1 }, // yaw
        { type::TYPE_NONE, 0                                   }  // zoom
    };
}

void LLGameControllerManager::getDefaultMappings(std::vector<std::pair<std::string, LLGameControl::InputChannel>>& mappings)
{
    // Join two different data structures into the one
    std::vector<LLGameControl::InputChannel> flycam_channels;
    getDefaultMappings(mappings, flycam_channels);
    for (size_t i = 0; i < flycam_channels.size(); ++i)
    {
        mappings.emplace_back(mFlycamActions[i], flycam_channels[i]);
    }
}

void LLGameControllerManager::initializeMappingsByDefault()
{
    std::vector<std::pair<std::string, LLGameControl::InputChannel>> agent_channels;
    getDefaultMappings(agent_channels, mFlycamChannels);
    mActionTranslator.setMappings(agent_channels);
}

void LLGameControllerManager::resetDeviceOptionsToDefaults()
{
    for (LLGameControl::Device& device : mDevices)
    {
        device.resetOptionsToDefaults();
    }
}

void LLGameControllerManager::loadDeviceOptionsFromSettings()
{
    for (LLGameControl::Device& device : mDevices)
    {
        device.loadOptionsFromString(getDeviceOptionsString(device.getGUID()));
    }
}

void LLGameControllerManager::saveDeviceOptionsToSettings() const
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
    // find Device by name
    for (LLGameControl::Device& device : mDevices)
    {
        if (device.getGUID() == guid)
        {
            device.mOptions = options;
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
            LL_WARNS("SDL2") << "device with id=" << id << " was already added"
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
    LL_INFOS("SDL2") << "joystick id: " << id << LL_ENDL;

    mDevices.remove_if([id](LLGameControl::Device& device)
        {
            return device.getJoystickID() == id;
        });
}

void LLGameControllerManager::onAxis(SDL_JoystickID id, U8 axis, S16 value)
{
    device_it it = findDevice(id);
    if (it == mDevices.end())
    {
        LL_WARNS("SDL2") << "Unknown device: joystick=0x" << std::hex << id << std::dec
            << " axis=" << (S32)axis
            << " value=" << (S32)value << LL_ENDL;
        return;
    }

    // Map axis using device-specific settings
    // or leave the value unchanged
    U8 mapped_axis = it->mOptions.mapAxis(axis);
    if (mapped_axis != axis)
    {
        LL_DEBUGS("SDL2") << "Axis mapped: joystick=0x" << std::hex << id << std::dec
            << " input axis i=" << (S32)axis
            << " mapped axis i=" << (S32)mapped_axis << LL_ENDL;
        axis = mapped_axis;
    }

    if (axis >= LLGameControl::NUM_AXES)
    {
        LL_WARNS("SDL2") << "Unknown axis: joystick=0x" << std::hex << id << std::dec
            << " axis=" << (S32)(axis)
            << " value=" << (S32)(value) << LL_ENDL;
        return;
    }

    // Fix value using device-specific settings
    // or leave the value unchanged
    S16 fixed_value = it->mOptions.fixAxisValue(axis, value);
    if (fixed_value != value)
    {
        LL_DEBUGS("SDL2") << "Value fixed: joystick=0x" << std::hex << id << std::dec
            << " axis i=" << (S32)axis
            << " input value=" << (S32)value
            << " fixed value=" << (S32)fixed_value << LL_ENDL;
        value = fixed_value;
    }

    // Note: the RAW analog joysticks provide NEGATIVE X,Y values for LEFT,FORWARD
    // whereas those directions are actually POSITIVE in SL's local right-handed
    // reference frame.  Therefore we implicitly negate those axes here where
    // they are extracted from SDL, before being used anywhere.
    if (axis < SDL_CONTROLLER_AXIS_TRIGGERLEFT)
    {
        // Note: S16 value is in range [-32768, 32767] which means
        // the negative range has an extra possible value.  We need
        // to add (or subtract) one during negation.
        if (value < 0)
        {
            value = - (value + 1);
        }
        else if (value > 0)
        {
            value = (-value) - 1;
        }
    }

    LL_DEBUGS("SDL2") << "joystick=0x" << std::hex << id << std::dec
        << " axis=" << (S32)(axis)
        << " value=" << (S32)(value) << LL_ENDL;
    it->mState.mAxes[axis] = value;
}

void LLGameControllerManager::onButton(SDL_JoystickID id, U8 button, bool pressed)
{
    device_it it = findDevice(id);
    if (it == mDevices.end())
    {
        LL_WARNS("SDL2") << "Unknown device: joystick=0x" << std::hex << id << std::dec
            << " button i=" << (S32)button << LL_ENDL;
        return;
    }

    // Map button using device-specific settings
    // or leave the value unchanged
    U8 mapped_button = it->mOptions.mapButton(button);
    if (mapped_button != button)
    {
        LL_DEBUGS("SDL2") << "Button mapped: joystick=0x" << std::hex << id << std::dec
            << " input button i=" << (S32)button
            << " mapped button i=" << (S32)mapped_button << LL_ENDL;
        button = mapped_button;
    }

    if (button >= LLGameControl::NUM_BUTTONS)
    {
        LL_WARNS("SDL2") << "Unknown button: joystick=0x" << std::hex << id << std::dec
            << " button i=" << (S32)button << LL_ENDL;
        return;
    }

    if (it->mState.onButton(button, pressed))
    {
        LL_DEBUGS("SDL2") << "joystick=0x" << std::hex << id << std::dec
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
    // clear the old state
    std::fill(mAxesAccumulator.begin(), mAxesAccumulator.end(), 0);
    mButtonAccumulator = 0;

    // accumulate the controllers
    for (const auto& device : mDevices)
    {
        mButtonAccumulator |= device.mState.mButtons;
        for (size_t i = 0; i < LLGameControl::NUM_AXES; ++i)
        {
            // Note: we don't bother to clamp the axes yet
            // because at this stage we haven't yet accumulated the "inner" state.
            mAxesAccumulator[i] += (S32)device.mState.mAxes[i];
        }
    }
}

void LLGameControllerManager::computeFinalState()
{
    // We assume accumulateInternalState() has already been called and we will
    // finish by accumulating "external" state (if enabled)
    U32 old_buttons = g_finalState.mButtons;
    g_finalState.mButtons = mButtonAccumulator;
    if (g_translateAgentActions)
    {
        // accumulate from mExternalState
        g_finalState.mButtons |= mExternalState.mButtons;
    }
    if (old_buttons != g_finalState.mButtons)
    {
        g_nextResendPeriod = 0; // packet needs to go out ASAP
    }

    // clamp the accumulated axes
    for (size_t i = 0; i < LLGameControl::NUM_AXES; ++i)
    {
        S32 axis = mAxesAccumulator[i];
        if (g_translateAgentActions)
        {
            // Note: we accumulate mExternalState onto local 'axis' variable
            // rather than onto mAxisAccumulator[i] because the internal
            // accumulated value is also used to drive the Flycam, and
            // we don't want any external state leaking into that value.
            axis += (S32)mExternalState.mAxes[i];
        }
        axis = (S16)std::min(std::max(axis, -32768), 32767);
        // check for change
        if (g_finalState.mAxes[i] != axis)
        {
            // When axis changes we explicitly update the corresponding prevAxis
            // prior to storing axis.  The only other place where prevAxis
            // is updated in updateResendPeriod() which is explicitly called after
            // a packet is sent.  The result is: unchanged axes are included in
            // first resend but not later ones.
            g_finalState.mPrevAxes[i] = g_finalState.mAxes[i];
            g_finalState.mAxes[i] = axis;
            g_nextResendPeriod = 0; // packet needs to go out ASAP
        }
    }
}

LLGameControl::ActionNameType LLGameControllerManager::getActionNameType(const std::string& action) const
{
    auto it = mActions.find(action);
    return it == mActions.end() ? LLGameControl::ACTION_NAME_UNKNOWN : it->second;
}

LLGameControl::InputChannel LLGameControllerManager::getChannelByAction(const std::string& action) const
{
    LLGameControl::InputChannel channel;
    auto action_it = mActions.find(action);
    if (action_it != mActions.end())
    {
        if (action_it->second == LLGameControl::ACTION_NAME_FLYCAM)
        {
            channel = getFlycamChannelByAction(action);
        }
        else
        {
            channel = mActionTranslator.getChannelByAction(action);
        }
    }
    return channel;
}

LLGameControl::InputChannel LLGameControllerManager::getFlycamChannelByAction(const std::string& action) const
{
    auto flycam_it = std::find(mFlycamActions.begin(), mFlycamActions.end(), action);
    llassert(flycam_it != mFlycamActions.end());
    std::ptrdiff_t index = std::distance(mFlycamActions.begin(), flycam_it);
    return mFlycamChannels[(std::size_t)index];
}

// Common implementation of getAnalogMappings(), getBinaryMappings() and getFlycamMappings()
static std::string getMappings(const std::vector<std::string>& actions, LLGameControl::InputChannel::Type type,
    std::function<LLGameControl::InputChannel(const std::string& action)> getChannel)
{
    std::list<std::string> mappings;

    std::vector<std::pair<std::string, LLGameControl::InputChannel>> default_mappings;
    LLGameControl::getDefaultMappings(default_mappings);

    // Walk through the all known actions of the chosen type
    for (const std::string& action : actions)
    {
        LLGameControl::InputChannel channel = getChannel(action);
        // Only channels of the expected type should be stored
        if (channel.mType == type)
        {
            bool mapping_differs = false;
            for (const auto& pair : default_mappings)
            {
                if (pair.first == action)
                {
                    mapping_differs = !channel.isEqual(pair.second);
                    break;
                }
            }
            // Only mappings different from the default should be stored
            if (mapping_differs)
            {
                mappings.push_back(action + ":" + channel.getLocalName());
            }
        }
    }

    std::string result = LLStringUtil::join(mappings);

    return result;
}

std::string LLGameControllerManager::getAnalogMappings() const
{
    return getMappings(mAnalogActions, LLGameControl::InputChannel::TYPE_AXIS,
        [&](const std::string& action) -> LLGameControl::InputChannel
        {
            return mActionTranslator.getChannelByAction(action + "+");
        });
}

std::string LLGameControllerManager::getBinaryMappings() const
{
    return getMappings(mBinaryActions, LLGameControl::InputChannel::TYPE_BUTTON,
        [&](const std::string& action) -> LLGameControl::InputChannel
        {
            return mActionTranslator.getChannelByAction(action);
        });
}

std::string LLGameControllerManager::getFlycamMappings() const
{
    return getMappings(mFlycamActions, LLGameControl::InputChannel::TYPE_AXIS,
        [&](const std::string& action) -> LLGameControl::InputChannel
        {
            return getFlycamChannelByAction(action);
        });
}

// Common implementation of setAnalogMappings(), setBinaryMappings() and setFlycamMappings()
static void setMappings(const std::string& mappings,
    const std::vector<std::string>& actions, LLGameControl::InputChannel::Type type,
    std::function<void(const std::string& action, LLGameControl::InputChannel channel)> updateMap)
{
    if (mappings.empty())
        return;

    std::map<std::string, std::string> pairs;
    LLStringOps::splitString(mappings, ',', [&](const std::string& mapping)
        {
            std::size_t pos = mapping.find(':');
            if (pos > 0 && pos != std::string::npos)
            {
                pairs[mapping.substr(0, pos)] = mapping.substr(pos + 1);
            }
        });

    static const LLGameControl::InputChannel channelNone;

    for (const std::string& action : actions)
    {
        auto it = pairs.find(action);
        if (it != pairs.end())
        {
            LLGameControl::InputChannel channel = LLGameControl::getChannelByName(it->second);
            if (channel.isNone() || channel.mType == type)
            {
                updateMap(action, channel);
                continue;
            }
        }
        updateMap(action, channelNone);
    }
}

void LLGameControllerManager::setAnalogMappings(const std::string& mappings)
{
    setMappings(mappings, mAnalogActions, LLGameControl::InputChannel::TYPE_AXIS,
        [&](const std::string& action, LLGameControl::InputChannel channel)
        {
            mActionTranslator.updateMap(action, channel);
        });
}

void LLGameControllerManager::setBinaryMappings(const std::string& mappings)
{
    setMappings(mappings, mBinaryActions, LLGameControl::InputChannel::TYPE_BUTTON,
        [&](const std::string& action, LLGameControl::InputChannel channel)
        {
            mActionTranslator.updateMap(action, channel);
        });
}

void LLGameControllerManager::setFlycamMappings(const std::string& mappings)
{
    setMappings(mappings, mFlycamActions, LLGameControl::InputChannel::TYPE_AXIS,
        [&](const std::string& action, LLGameControl::InputChannel channel)
        {
            updateFlycamMap(action, channel);
        });
}

bool LLGameControllerManager::updateActionMap(const std::string& action, LLGameControl::InputChannel channel)
{
    auto action_it = mActions.find(action);
    if (action_it == mActions.end())
    {
        LL_WARNS("SDL2") << "unmappable action='" << action << "'" << LL_ENDL;
        return false;
    }

    if (action_it->second == LLGameControl::ACTION_NAME_FLYCAM)
    {
        updateFlycamMap(action, channel);
    }
    else
    {
        mActionTranslator.updateMap(action, channel);
    }
    return true;
}

void LLGameControllerManager::updateFlycamMap(const std::string& action, LLGameControl::InputChannel channel)
{
    auto flycam_it = std::find(mFlycamActions.begin(), mFlycamActions.end(), action);
    llassert(flycam_it != mFlycamActions.end());
    std::ptrdiff_t index = std::distance(mFlycamActions.begin(), flycam_it);
    llassert(index >= 0 && (std::size_t)index < mFlycamChannels.size());
    mFlycamChannels[(std::size_t)index] = channel;
}

U32 LLGameControllerManager::computeInternalActionFlags()
{
    // add up device inputs
    accumulateInternalState();
    if (g_controlAgent)
    {
        return mActionTranslator.computeFlagsFromState(mAxesAccumulator, mButtonAccumulator);
    }
    return 0;
}

void LLGameControllerManager::getFlycamInputs(std::vector<F32>& inputs)
{
    // The inputs are packed in the same order as they exist in mFlycamChannels:
    //
    //     advance
    //     pan
    //     rise
    //     pitch
    //     yaw
    //     zoom
    //
    for (const auto& channel: mFlycamChannels)
    {
        S16 axis;
        if (channel.mIndex == LLGameControl::AXIS_TRIGGERLEFT ||
            channel.mIndex == LLGameControl::AXIS_TRIGGERRIGHT)
        {
            // TIED TRIGGER HACK: we assume the two triggers are paired together
            S32 total_axis = mAxesAccumulator[LLGameControl::AXIS_TRIGGERLEFT]
                - mAxesAccumulator[LLGameControl::AXIS_TRIGGERRIGHT];
            if (channel.mIndex == LLGameControl::AXIS_TRIGGERRIGHT)
            {
                // negate previous math when TRIGGERRIGHT is positive channel
                total_axis *= -1;
            }
            axis = S16(std::min(std::max(total_axis, -32768), 32767));
        }
        else
        {
            axis = S16(std::min(std::max(mAxesAccumulator[channel.mIndex], -32768), 32767));
        }
        // value arrives as S16 in range [-32768, 32767]
        // so we scale positive and negative values by slightly different factors
        // to try to map it to [-1, 1]
        F32 input = F32(axis) / ((axis > 0.0f) ? 32767 : 32768) * channel.mSign;
        inputs.push_back(input);
    }
}

void LLGameControllerManager::setExternalInput(U32 action_flags, U32 buttons)
{
    if (g_translateAgentActions)
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
    SDL_JoystickGUID guid(SDL_JoystickGetDeviceGUID(event.cdevice.which));
    SDL_JoystickType type(SDL_JoystickGetDeviceType(event.cdevice.which));
    std::string name(ll_safe_string(SDL_JoystickNameForIndex(event.cdevice.which)));

    LL_INFOS("SDL2") << "joystick {id:" << event.cdevice.which
        << ",guid:'" << guid << "'"
        << ",type:'" << type << "'"
        << ",name:'" << name << "'"
        << "}" << LL_ENDL;

    if (SDL_Joystick* joystick = SDL_JoystickOpen(event.cdevice.which))
    {
        LL_INFOS("SDL2") << "joystick " << joystick << LL_ENDL;
    }
    else
    {
        LL_WARNS("SDL2") << "Can't open joystick: " << SDL_GetError() << LL_ENDL;
    }
}

void onJoystickDeviceRemoved(const SDL_Event& event)
{
    LL_INFOS("SDL2") << "joystick id: " << event.cdevice.which << LL_ENDL;
}

void onControllerDeviceAdded(const SDL_Event& event)
{
    std::string guid(std::to_string(SDL_JoystickGetDeviceGUID(event.cdevice.which)));
    SDL_GameControllerType type(SDL_GameControllerTypeForIndex(event.cdevice.which));
    std::string name(ll_safe_string(SDL_GameControllerNameForIndex(event.cdevice.which)));

    LL_INFOS("SDL2") << "controller {id:" << event.cdevice.which
        << ",guid:'" << guid << "'"
        << ",type:'" << type << "'"
        << ",name:'" << name << "'"
        << "}" << LL_ENDL;

    SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(event.cdevice.which);
    if (id < 0)
    {
        LL_WARNS("SDL2") << "Can't get device instance ID: " << SDL_GetError() << LL_ENDL;
        return;
    }

    SDL_GameController* controller = SDL_GameControllerOpen(event.cdevice.which);
    if (!controller)
    {
        LL_WARNS("SDL2") << "Can't open game controller: " << SDL_GetError() << LL_ENDL;
        return;
    }

    g_manager.addController(id, guid, name);

    // this event could happen while the preferences UI is open
    // in which case we need to force it to update
    s_updateUI();
}

void onControllerDeviceRemoved(const SDL_Event& event)
{
    LL_INFOS("SDL2") << "joystick id=" << event.cdevice.which << LL_ENDL;

    SDL_JoystickID id = event.cdevice.which;
    g_manager.removeController(id);

    // this event could happen while the preferences UI is open
    // in which case we need to force it to update
    s_updateUI();
}

void onControllerButton(const SDL_Event& event)
{
    g_manager.onButton(event.cbutton.which, event.cbutton.button, event.cbutton.state == SDL_PRESSED);
}

void onControllerAxis(const SDL_Event& event)
{
    LL_DEBUGS("SDL2") << "joystick=0x" << std::hex << event.caxis.which << std::dec
        << " axis=" << (S32)(event.caxis.axis)
        << " value=" << (S32)(event.caxis.value) << LL_ENDL;
    g_manager.onAxis(event.caxis.which, event.caxis.axis, event.caxis.value);
}

// static
bool LLGameControl::isEnabled()
{
    return g_enabled;
}

void LLGameControl::setEnabled(bool enable)
{
    if (enable != g_enabled)
    {
        g_enabled = enable;
        s_saveBoolean(SETTING_ENABLE, g_enabled);
    }
}

// static
bool LLGameControl::isInitialized()
{
    return g_gameControl != nullptr;
}

// static
// TODO: find a cleaner way to provide callbacks to LLGameControl
void LLGameControl::init(const std::string& gamecontrollerdb_path,
    std::function<bool(const std::string&)> loadBoolean,
    std::function<void(const std::string&, bool)> saveBoolean,
    std::function<std::string(const std::string&)> loadString,
    std::function<void(const std::string&, const std::string&)> saveString,
    std::function<LLSD(const std::string&)> loadObject,
    std::function<void(const std::string&, const LLSD&)> saveObject,
    std::function<void()> updateUI)
{
    if (g_gameControl)
        return;

    llassert(loadBoolean);
    llassert(saveBoolean);
    llassert(loadString);
    llassert(saveString);
    llassert(loadObject);
    llassert(saveObject);
    llassert(updateUI);

#ifndef LL_DARWIN
    // SDL2 is temporarily disabled on Mac, so this needs to be a no-op on that platform

    int result = SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR);
    if (result < 0)
    {
        // This error is critical, we stop working with SDL and return
        LL_WARNS("SDL2") << "Error initializing GameController subsystems : " << SDL_GetError() << LL_ENDL;
        return;
    }

    // The inability to read this file is not critical, we can continue working
    if (!LLFile::isfile(gamecontrollerdb_path.c_str()))
    {
        LL_WARNS("SDL2") << "Device mapping db file not found: " << gamecontrollerdb_path << LL_ENDL;
    }
    else
    {
        int count = SDL_GameControllerAddMappingsFromFile(gamecontrollerdb_path.c_str());
        if (count < 0)
        {
            LL_WARNS("SDL2") << "Error adding mappings from " << gamecontrollerdb_path << " : " << SDL_GetError() << LL_ENDL;
        }
        else
        {
            LL_INFOS("SDL2") << "Total " << count << " mappings added from " << gamecontrollerdb_path << LL_ENDL;
        }
    }
#endif // LL_DARWIN

    g_gameControl = LLGameControl::getInstance();

    s_loadBoolean = loadBoolean;
    s_saveBoolean = saveBoolean;
    s_loadString = loadString;
    s_saveString = saveString;
    s_loadObject = loadObject;
    s_saveObject = saveObject;
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
// returns 'true' if GameControlInput message needs to go out,
// which will be the case for new data or resend. Call this right
// before deciding to put a GameControlInput packet on the wire
// or not.
bool LLGameControl::computeFinalStateAndCheckForChanges()
{
    // Note: LLGameControllerManager::computeFinalState() modifies g_nextResendPeriod as a side-effect
    g_manager.computeFinalState();

    // should send input when:
    //     sending is enabled and
    //     g_lastSend has "expired"
    //         either because g_nextResendPeriod has been zeroed
    //         or the last send really has expired.
    return g_enabled && g_sendToServer && (g_lastSend + g_nextResendPeriod < get_now_nsec());
}

// static
void LLGameControl::clearAllStates()
{
    g_manager.clearAllStates();
}

// static
void LLGameControl::processEvents(bool app_has_focus)
{
#ifndef LL_DARWIN
    // SDL2 is temporarily disabled on Mac, so this needs to be a no-op on that platform

    // This method used by non-linux platforms which only use SDL for GameController input
    SDL_Event event;
    if (!app_has_focus)
    {
        // when SL window lacks focus: pump SDL events but ignore them
        while (g_gameControl && SDL_PollEvent(&event))
        {
            // do nothing: SDL_PollEvent() is the operator
        }
        g_manager.clearAllStates();
        return;
    }

    while (g_gameControl && SDL_PollEvent(&event))
    {
        handleEvent(event, app_has_focus);
    }
#endif // LL_DARWIN
}

void LLGameControl::handleEvent(const SDL_Event& event, bool app_has_focus)
{
    switch (event.type)
    {
        case SDL_JOYDEVICEADDED:
            onJoystickDeviceAdded(event);
            break;
        case SDL_JOYDEVICEREMOVED:
            onJoystickDeviceRemoved(event);
            break;
        case SDL_CONTROLLERDEVICEADDED:
            onControllerDeviceAdded(event);
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            onControllerDeviceRemoved(event);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            /* FALLTHROUGH */
        case SDL_CONTROLLERBUTTONUP:
            if (app_has_focus)
            {
                onControllerButton(event);
            }
            break;
        case SDL_CONTROLLERAXISMOTION:
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
const LLGameControl::State& LLGameControl::getState()
{
    return g_finalState;
}

// static
LLGameControl::InputChannel LLGameControl::getActiveInputChannel()
{
    InputChannel input;

    State state = g_finalState;
    if (state.mButtons > 0)
    {
        // check buttons
        input.mType = LLGameControl::InputChannel::TYPE_BUTTON;
        for (U8 i = 0; i < 32; ++i)
        {
            if ((0x1 << i) & state.mButtons)
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
    s_saveBoolean(SETTING_SENDTOSERVER, g_sendToServer);
}

// static
void LLGameControl::setControlAgent(bool enable)
{
    g_controlAgent = enable;
    s_saveBoolean(SETTING_CONTROLAGENT, g_controlAgent);
}

// static
void LLGameControl::setTranslateAgentActions(bool enable)
{
    g_translateAgentActions = enable;
    s_saveBoolean(SETTING_TRANSLATEACTIONS, g_translateAgentActions);
}

// static
void LLGameControl::setAgentControlMode(LLGameControl::AgentControlMode mode)
{
    g_agentControlMode = mode;
    s_saveString(SETTING_AGENTCONTROLMODE, convertAgentControlModeToString(mode));
}

// static
bool LLGameControl::getSendToServer()
{
    return g_sendToServer;
}

// static
bool LLGameControl::getControlAgent()
{
    return g_controlAgent;
}

// static
bool LLGameControl::getTranslateAgentActions()
{
    return g_translateAgentActions;
}

// static
LLGameControl::AgentControlMode LLGameControl::getAgentControlMode()
{
    return g_agentControlMode;
}

// static
LLGameControl::ActionNameType LLGameControl::getActionNameType(const std::string& action)
{
    return g_manager.getActionNameType(action);
}

// static
bool LLGameControl::willControlAvatar()
{
    return g_enabled && g_controlAgent && g_agentControlMode == CONTROL_MODE_AVATAR;
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
// Given an action_name like "push+", or "strafe-", returns the InputChannel
// mapped to it if found, else channel.isNone() will be true.
LLGameControl::InputChannel LLGameControl::getChannelByAction(const std::string& action)
{
    return g_manager.getChannelByAction(action);
}

// static
bool LLGameControl::updateActionMap(const std::string& action, LLGameControl::InputChannel channel)
{
    return g_manager.updateActionMap(action, channel);
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
std::string LLGameControl::stringifyAnalogMappings(getChannel_t getChannel)
{
    return getMappings(g_manager.mAnalogActions, InputChannel::TYPE_AXIS, getChannel);
}

// static
std::string LLGameControl::stringifyBinaryMappings(getChannel_t getChannel)
{
    return getMappings(g_manager.mBinaryActions, InputChannel::TYPE_BUTTON, getChannel);
}

// static
std::string LLGameControl::stringifyFlycamMappings(getChannel_t getChannel)
{
    return getMappings(g_manager.mFlycamActions, InputChannel::TYPE_AXIS, getChannel);
}

// static
void LLGameControl::getDefaultMappings(std::vector<std::pair<std::string, LLGameControl::InputChannel>>& mappings)
{
    g_manager.getDefaultMappings(mappings);
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
        LL_WARNS("SDL2") << "Invalid options: '" << options << "'" << LL_ENDL;
        return false;
    }

    std::map<std::string, std::string> axis_string_options;
    if (!parse(axis_string_options, pairs["axis_options"]))
    {
        LL_WARNS("SDL2") << "Invalid axis_options: '" << pairs["axis_options"] << "'" << LL_ENDL;
        return false;
    }

    std::map<std::string, std::string> axis_string_map;
    if (!parse(axis_string_map, pairs["axis_map"]))
    {
        LL_WARNS("SDL2") << "Invalid axis_map: '" << pairs["axis_map"] << "'" << LL_ENDL;
        return false;
    }

    std::map<std::string, std::string> button_string_map;
    if (!parse(button_string_map, pairs["button_map"]))
    {
        LL_WARNS("SDL2") << "Invalid button_map: '" << pairs["button_map"] << "'" << LL_ENDL;
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
                LL_WARNS("SDL2") << "Invalid axis mapping: " << i << "->" << value << LL_ENDL;
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
                LL_WARNS("SDL2") << "Invalid button mapping: " << i << "->" << value << LL_ENDL;
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
    g_controlAgent = false;
    g_translateAgentActions = false;
    g_agentControlMode = CONTROL_MODE_AVATAR;
    g_manager.initializeMappingsByDefault();
    g_manager.resetDeviceOptionsToDefaults();
    g_deviceOptions.clear();
}

// static
void LLGameControl::loadFromSettings()
{
    // In case of absence of the required setting the default value is assigned
    g_enabled = s_loadBoolean(SETTING_ENABLE);
    g_sendToServer = s_loadBoolean(SETTING_SENDTOSERVER);
    g_controlAgent = s_loadBoolean(SETTING_CONTROLAGENT);
    g_translateAgentActions = s_loadBoolean(SETTING_TRANSLATEACTIONS);
    g_agentControlMode = convertStringToAgentControlMode(s_loadString(SETTING_AGENTCONTROLMODE));

    g_manager.initializeMappingsByDefault();

    // Load action-to-channel mappings
    std::string analogMappings = s_loadString(SETTING_ANALOGMAPPINGS);
    std::string binaryMappings = s_loadString(SETTING_BINARYMAPPINGS);
    std::string flycamMappings = s_loadString(SETTING_FLYCAMMAPPINGS);
    g_manager.setAnalogMappings(analogMappings);
    g_manager.setBinaryMappings(binaryMappings);
    g_manager.setFlycamMappings(flycamMappings);

    // Load device-specific settings
    g_deviceOptions.clear();
    LLSD options = s_loadObject(SETTING_KNOWNCONTROLLERS);
    for (auto it = options.beginMap(); it != options.endMap(); ++it)
    {
        g_deviceOptions.emplace(it->first, it->second);
    }
    g_manager.loadDeviceOptionsFromSettings();
}

// static
void LLGameControl::saveToSettings()
{
    s_saveBoolean(SETTING_ENABLE, g_enabled);
    s_saveBoolean(SETTING_SENDTOSERVER, g_sendToServer);
    s_saveBoolean(SETTING_CONTROLAGENT, g_controlAgent);
    s_saveBoolean(SETTING_TRANSLATEACTIONS, g_translateAgentActions);
    s_saveString(SETTING_AGENTCONTROLMODE, convertAgentControlModeToString(g_agentControlMode));
    s_saveString(SETTING_ANALOGMAPPINGS, g_manager.getAnalogMappings());
    s_saveString(SETTING_BINARYMAPPINGS, g_manager.getBinaryMappings());
    s_saveString(SETTING_FLYCAMMAPPINGS, g_manager.getFlycamMappings());

    g_manager.saveDeviceOptionsToSettings();

    // construct LLSD version of g_deviceOptions but only include non-empty values
    LLSD deviceOptions = LLSD::emptyMap();
    for (const auto& data_pair : g_deviceOptions)
    {
        if (!data_pair.second.empty())
        {
            LLSD value(data_pair.second);
            deviceOptions.insert(data_pair.first, value);
        }
    }

    s_saveObject(SETTING_KNOWNCONTROLLERS, deviceOptions);
}

// static
void LLGameControl::setDeviceOptions(const std::string& guid, const Options& options)
{
    g_manager.setDeviceOptions(guid, options);
}

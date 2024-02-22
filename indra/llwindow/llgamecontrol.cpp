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
#include "llgamecontroltranslator.h"

constexpr size_t NUM_AXES = 6;

std::unordered_map<std::string, LLGameControl::InputChannel> gChannelMap;

// static
void LLGameControl::InputChannel::initChannelMap()
{
    using type = LLGameControl::InputChannel::Type;
    gChannelMap["AXIS_0-"] = LLGameControl::InputChannel(type::TYPE_AXIS, 0);
    gChannelMap["AXIS_0+"] = LLGameControl::InputChannel(type::TYPE_AXIS, 1);
    gChannelMap["AXIS_1-"] = LLGameControl::InputChannel(type::TYPE_AXIS, 2);
    gChannelMap["AXIS_1+"] = LLGameControl::InputChannel(type::TYPE_AXIS, 3);
    gChannelMap["AXIS_2-"] = LLGameControl::InputChannel(type::TYPE_AXIS, 4);
    gChannelMap["AXIS_2+"] = LLGameControl::InputChannel(type::TYPE_AXIS, 5);
    gChannelMap["AXIS_3-"] = LLGameControl::InputChannel(type::TYPE_AXIS, 6);
    gChannelMap["AXIS_3+"] = LLGameControl::InputChannel(type::TYPE_AXIS, 7);
    gChannelMap["AXIS_4-"] = LLGameControl::InputChannel(type::TYPE_AXIS, 8);
    gChannelMap["AXIS_4+"] = LLGameControl::InputChannel(type::TYPE_AXIS, 9);
    gChannelMap["AXIS_5-"] = LLGameControl::InputChannel(type::TYPE_AXIS, 10);
    gChannelMap["AXIS_5+"] = LLGameControl::InputChannel(type::TYPE_AXIS, 11);

    gChannelMap["BUTTON_0"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 0);
    gChannelMap["BUTTON_1"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 1);
    gChannelMap["BUTTON_2"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 2);
    gChannelMap["BUTTON_3"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 3);
    gChannelMap["BUTTON_4"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 4);
    gChannelMap["BUTTON_5"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 5);
    gChannelMap["BUTTON_6"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 6);
    gChannelMap["BUTTON_7"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 7);
    gChannelMap["BUTTON_8"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 8);
    gChannelMap["BUTTON_9"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 9);
    gChannelMap["BUTTON_10"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 10);
    gChannelMap["BUTTON_11"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 11);
    gChannelMap["BUTTON_12"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 12);
    gChannelMap["BUTTON_13"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 13);
    gChannelMap["BUTTON_14"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 14);
    gChannelMap["BUTTON_15"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 15);
    gChannelMap["BUTTON_16"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 16);
    gChannelMap["BUTTON_17"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 17);
    gChannelMap["BUTTON_18"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 18);
    gChannelMap["BUTTON_19"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 19);
    gChannelMap["BUTTON_20"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 20);
    gChannelMap["BUTTON_21"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 21);
    gChannelMap["BUTTON_22"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 22);
    gChannelMap["BUTTON_23"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 23);
    gChannelMap["BUTTON_24"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 24);
    gChannelMap["BUTTON_25"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 25);
    gChannelMap["BUTTON_26"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 26);
    gChannelMap["BUTTON_27"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 27);
    gChannelMap["BUTTON_28"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 28);
    gChannelMap["BUTTON_29"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 29);
    gChannelMap["BUTTON_30"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 30);
    gChannelMap["BUTTON_31"] = LLGameControl::InputChannel(type::TYPE_BUTTON, 31);
}

bool LLGameControl::InputChannel::isNegAxis() const
{
    // The organization of negative axis is odd
    return mType == LLGameControl::InputChannel::TYPE_AXIS
        && mIndex == 2 * (mIndex / 2);
}

std::string LLGameControl::InputChannel::getLocalName() const
{
    // HACK: we hard-code English channel names, but
    // they should be loaded from localized XML config files.
    std::string name = " ";
    // AXIS_0_NEG, AXIS_0_POS, BUTTON_0, etc
    if (mType == LLGameControl::InputChannel::TYPE_AXIS)
    {
        constexpr U8 NUM_AXIS_KEYBOARD_CHANNELS = (U8)(NUM_AXES * 2);
        if (mIndex < 2 * NUM_AXIS_KEYBOARD_CHANNELS)
        {
            U8 axis_index = mIndex / 2;
            name = "AXIS_";
            name.append(std::to_string((S32)(axis_index)));
            if (mIndex - (axis_index * 2) > 0)
            {
                name.append("+");
            }
            else
            {
                name.append("-");
            }
        }
    }
    else if (mType == LLGameControl::InputChannel::TYPE_BUTTON)
    {
        constexpr U8 NUM_BUTTONS = 32;
        if (mIndex < NUM_BUTTONS)
        {
            name = "BUTTON_";
            name.append(std::to_string((S32)(mIndex)));
        }
    }
    return name;
}

std::string LLGameControl::InputChannel::getRemoteName() const
{
    // HACK: we hard-code English channel names, but
    // they should be loaded from localized XML config files.
    std::string name = " ";
    // GAME_CONTROL_AXIS_LEFTX, GAME_CONTROL_BUTTON_A, etc
    if (mType == LLGameControl::InputChannel::TYPE_AXIS)
    {
        // TODO: change these to indexe arrays
        U8 axis_index = mIndex / 2;
        switch(axis_index)
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
    LLGameControllerManager();

    void addController(SDL_JoystickID id, SDL_GameController* controller);
    void removeController(SDL_JoystickID id);

    void onAxis(SDL_JoystickID id, U8 axis, S16 value);
    void onButton(SDL_JoystickID id, U8 button, bool pressed);

    void clearAllState();
    size_t getControllerIndex(SDL_JoystickID id) const;

    void accumulateInternalState();
    void computeFinalState(LLGameControl::State& state);

    LLGameControl::InputChannel getChannelByActionName(const std::string& name) const;
    void addActionMapping(const std::string& name,  LLGameControl::InputChannel channel);
    U32 computeInternalActionFlags();
    void setExternalActionFlags(U32 action_flags);

    void clear();

private:
    std::vector<SDL_JoystickID> mControllerIDs;
    std::vector<SDL_GameController*> mControllers;
    std::vector<LLGameControl::State> mStates; // one state per device

    LLGameControl::State mExternalState;
    LLGameControlTranslator mActionTranslator;
    LLGameControlTranslator mCameraTranslator;
    std::vector<S32> mAxesAccumulator;
    U32 mButtonAccumulator { 0 };
    U32 mLastActionFlags { 0 };
    U32 mLastCameraActionFlags { 0 };
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
    // data state. However, to keep th ambient resend bandwidth low we
    // expand the resend period at a geometric rate.
    //
    constexpr U64 MSEC_PER_NSEC = 1e6;
    constexpr U64 FIRST_RESEND_PERIOD = 100 * MSEC_PER_NSEC;
    constexpr U64 RESEND_EXPANSION_RATE = 10;
    LLGameControl::State g_outerState; // from controller devices
    LLGameControl::State g_innerState; // state from gAgent
    LLGameControl::State g_finalState; // sum of inner and outer
    U64 g_lastSend = 0;
    U64 g_nextResendPeriod = FIRST_RESEND_PERIOD;

    bool g_sendToServer = false;
    bool g_controlAgent = false;
    bool g_translateAgentActions = false;
    LLGameControl::AgentControlMode g_agentControlMode = LLGameControl::CONTROL_MODE_AVATAR;

    constexpr U8 MAX_AXIS = 5;
    constexpr U8 MAX_BUTTON = 31;
}

LLGameControl::~LLGameControl()
{
    terminate();
}

LLGameControl::State::State() : mButtons(0)
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
    if (button <= MAX_BUTTON)
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
    bool changed = (old_buttons != mButtons);
    return changed;
}

LLGameControllerManager::LLGameControllerManager()
{
    mAxesAccumulator.resize(NUM_AXES, 0);

    // build the invariant name-to-mask maps
    LLGameControlTranslator::NameToMaskMap avatar_map;
    avatar_map["push_forward"] = AGENT_CONTROL_AT_POS | AGENT_CONTROL_FAST_AT;
    avatar_map["push_backward"] = AGENT_CONTROL_AT_NEG | AGENT_CONTROL_FAST_AT;
    avatar_map["slide_left"] = AGENT_CONTROL_LEFT_POS | AGENT_CONTROL_FAST_LEFT;
    avatar_map["slide_right"] = AGENT_CONTROL_LEFT_NEG | AGENT_CONTROL_FAST_LEFT;
    avatar_map["jump"] = AGENT_CONTROL_UP_POS | AGENT_CONTROL_FAST_UP;
    avatar_map["push_down"] = AGENT_CONTROL_UP_NEG | AGENT_CONTROL_FAST_UP;
    avatar_map["turn_left"] = AGENT_CONTROL_YAW_POS;
    avatar_map["turn_right"] = AGENT_CONTROL_YAW_NEG;
    avatar_map["look_up"] = AGENT_CONTROL_PITCH_NEG;
    avatar_map["look_down"] = AGENT_CONTROL_PITCH_POS;
    // These are HACKs. We borrow some AGENT_CONTROL bits for "unrelated" features.
    avatar_map["toggle_run"] = AGENT_CONTROL_NUDGE_AT_POS; // HACK
    avatar_map["toggle_fly"] = AGENT_CONTROL_FLY; // HACK
    avatar_map["toggle_sit"] = AGENT_CONTROL_SIT_ON_GROUND; // HACK
    avatar_map["flycam"]     = AGENT_CONTROL_NUDGE_AT_NEG; // HACK
    avatar_map["stop_moving"] = AGENT_CONTROL_STOP;
    mActionTranslator.setNameToMaskMap(avatar_map);

    using type = LLGameControl::InputChannel::Type;

    // load defaults
    std::vector< std::pair<std::string, LLGameControl::InputChannel> > avatar_defaults =
    {
        { "push_forward",  { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_LEFTY_POS) }         },
        { "push_backward", { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_LEFTY_NEG) }         },
        { "slide_left",    { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_LEFTX_POS) }         },
        { "slide_right",   { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_LEFTX_NEG) }         },
        { "jump",          { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_TRIGGERRIGHT_POS) }  },
        { "push_down",     { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_TRIGGERLEFT_POS) }   },
        { "turn_left",     { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_RIGHTX_POS) }        },
        { "turn_right",    { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_RIGHTX_NEG) }        },
        { "look_up",       { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_RIGHTY_NEG) }        },
        { "look_down",     { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_RIGHTY_POS) }        },
        { "toggle_run",    { type::TYPE_BUTTON, (U8)(LLGameControl::BUTTON_LEFTSHOULDER) }  },
        { "toggle_fly",    { type::TYPE_BUTTON, (U8)(LLGameControl::BUTTON_DPAD_UP) }       },
        { "toggle_sit",    { type::TYPE_BUTTON, (U8)(LLGameControl::BUTTON_DPAD_DOWN) }     },
        { "flycam",        { type::TYPE_BUTTON, (U8)(LLGameControl::BUTTON_RIGHTSHOULDER) } },
        { "stop_moving",   { type::TYPE_BUTTON, (U8)(LLGameControl::BUTTON_LEFTSTICK) }     }
    };
    mActionTranslator.setMappings(avatar_defaults);

    // similarly for camera
    LLGameControlTranslator::NameToMaskMap camera_map;
    camera_map["move_forward"] = AGENT_CONTROL_AT_POS;
    camera_map["move_backward"] = AGENT_CONTROL_AT_NEG;
    camera_map["pan_left"] = AGENT_CONTROL_LEFT_POS;
    camera_map["pan_right"] = AGENT_CONTROL_LEFT_NEG;
    camera_map["rise_up"] = AGENT_CONTROL_UP_POS;
    camera_map["drop_down"] = AGENT_CONTROL_UP_NEG;
    camera_map["pitch_up"] = AGENT_CONTROL_PITCH_NEG;
    camera_map["pitch_down"] = AGENT_CONTROL_PITCH_POS;
    camera_map["yaw_left"] = AGENT_CONTROL_YAW_POS;
    camera_map["yaw_right"] = AGENT_CONTROL_YAW_NEG;
    mCameraTranslator.setNameToMaskMap(camera_map);
    std::vector< std::pair< std::string, LLGameControl::InputChannel > > camera_defaults =
    {
        { "move_forward",  { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_LEFTY_POS) }        },
        { "move_backward", { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_LEFTY_NEG) }        },
        { "pan_left",      { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_LEFTX_NEG) }        },
        { "pan_right",     { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_LEFTX_POS) }        },
        { "rise_up",       { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_TRIGGERRIGHT_POS) } },
        { "drop_down",     { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_TRIGGERLEFT_POS) }  },
        { "pitch_up",      { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_RIGHTY_POS) }       },
        { "pitch_down",    { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_RIGHTY_NEG) }       },
        { "yaw_left",      { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_RIGHTX_POS) }       },
        { "yaw_right",     { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_RIGHTX_NEG) }       }
        //{ "zoom_in",       { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_TRIGGERRIGHT_POS) } },
        //{ "zoom_out",      { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_TRIGGERLEFT_POS) }  },
        //{ "roll_ccw",      { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_TRIGGERRIGHT_POS) } },
        //{ "roll_cw",       { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_TRIGGERLEFT_POS) }  }

    };
    mCameraTranslator.setMappings(camera_defaults);
}

void LLGameControllerManager::addController(SDL_JoystickID id, SDL_GameController* controller)
{
    if (controller)
    {
        size_t i = 0;
        for (; i < mControllerIDs.size(); ++i)
        {
            if (id == mControllerIDs[i])
            {
                break;
            }
        }
        if (i == mControllerIDs.size())
        {
            mControllerIDs.push_back(id);
            mControllers.push_back(controller);
            mStates.push_back(LLGameControl::State());
            LL_DEBUGS("SDL2") << "joystick=0x" << std::hex << id << std::dec
                << " controller=" << controller
                << LL_ENDL;
        }
    }
}

void LLGameControllerManager::removeController(SDL_JoystickID id)
{
    size_t i = 0;
    size_t num_controllers = mControllerIDs.size();
    for (; i < num_controllers; ++i)
    {
        if (id == mControllerIDs[i])
        {
            LL_DEBUGS("SDL2") << "joystick=0x" << std::hex << id << std::dec
                << " controller=" << mControllers[i]
                << LL_ENDL;

            mControllerIDs[i] = mControllerIDs[num_controllers - 1];
            mControllers[i] = mControllers[num_controllers - 1];
            mStates[i] = mStates[num_controllers - 1];

            mControllerIDs.pop_back();
            mControllers.pop_back();
            mStates.pop_back();
            break;
        }
    }
}

size_t LLGameControllerManager::getControllerIndex(SDL_JoystickID id) const
{
    constexpr size_t UNREASONABLY_HIGH_INDEX = 1e6;
    size_t index = UNREASONABLY_HIGH_INDEX;
    for (size_t i = 0; i < mControllers.size(); ++i)
    {
        if (id == mControllerIDs[i])
        {
            index = i;
            break;
        }
    }
    return index;
}

void LLGameControllerManager::onAxis(SDL_JoystickID id, U8 axis, S16 value)
{
    if (axis > MAX_AXIS)
    {
        return;
    }
    size_t index = getControllerIndex(id);
    if (index < mControllers.size())
    {
        // Note: the RAW analog joystics provide NEGATIVE X,Y values for LEFT,FORWARD
        // whereas those directions are actually POSITIVE in SL's local right-handed
        // reference frame.  Therefore we implicitly negate those axes here where
        // they are extracted from SDL, before being used anywhere.
        if (axis < SDL_CONTROLLER_AXIS_TRIGGERLEFT)
        {
            // danger: S16 value is in range [-32768, 32767] which means
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
        mStates[index].mAxes[axis] = value;
    }
}

void LLGameControllerManager::onButton(SDL_JoystickID id, U8 button, bool pressed)
{
    size_t index = getControllerIndex(id);
    if (index < mControllers.size())
    {
        if (mStates[index].onButton(button, pressed))
        {
            LL_DEBUGS("SDL2") << "joystick=0x" << std::hex << id << std::dec
                << " button i=" << (S32)(button)
                << " pressed=" <<  pressed << LL_ENDL;
        }
    }
}

void LLGameControllerManager::clearAllState()
{
    for (auto& state : mStates)
    {
        state.clear();
    }
    mExternalState.clear();
    mLastActionFlags = 0;
    mLastCameraActionFlags = 0;
}

void LLGameControllerManager::accumulateInternalState()
{
    // clear the old state
    std::fill(mAxesAccumulator.begin(), mAxesAccumulator.end(), 0);
    mButtonAccumulator = 0;

    // accumulate the controllers
    for (const auto& state : mStates)
    {
        mButtonAccumulator |= state.mButtons;
        for (size_t i = 0; i < NUM_AXES; ++i)
        {
            // Note: we don't bother to clamp the axes yet
            // because at this stage we haven't yet accumulated the "inner" state.
            mAxesAccumulator[i] += (S32)(state.mAxes[i]);
        }
    }
}

void LLGameControllerManager::computeFinalState(LLGameControl::State& final_state)
{
    // We assume accumulateInternalState() has already been called and we will
    // finish by accumulating "external" state (if enabled)
    U32 old_buttons = final_state.mButtons;
    final_state.mButtons = mButtonAccumulator;
    if (g_translateAgentActions)
    {
        // accumulate from mExternalState
        final_state.mButtons |= mExternalState.mButtons;
        for (size_t i = 0; i < NUM_AXES; ++i)
        {
            mAxesAccumulator[i] += (S32)(mExternalState.mAxes[i]);
        }
    }
    if (old_buttons != final_state.mButtons)
    {
        g_nextResendPeriod = 0; // packet needs to go out ASAP
    }

    // clamp the accumulated axes
    for (size_t i = 0; i < NUM_AXES; ++i)
    {
        S32 new_axis = (S16)(std::min(std::max(mAxesAccumulator[i], -32768), 32767));
        // check for change
        if (final_state.mAxes[i] != new_axis)
        {
            // When axis changes we explicitly update the corresponding prevAxis
            // prior to storing new_axis.  The only other place where prevAxis
            // is updated in updateResendPeriod() which is explicitly called after
            // a packet is sent.  The result is: unchanged axes are included in
            // first resend but not later ones.
            final_state.mPrevAxes[i] = final_state.mAxes[i];
            final_state.mAxes[i] = new_axis;
            g_nextResendPeriod = 0; // packet needs to go out ASAP
        }
    }
}

LLGameControl::InputChannel LLGameControllerManager::getChannelByActionName(const std::string& name) const
{
    LLGameControl::InputChannel channel = mActionTranslator.getChannelByName(name);
    if (channel.isNone())
    {
        // maybe we're looking for a camera action
        channel = mCameraTranslator.getChannelByName(name);
    }
    return channel;
}

void LLGameControllerManager::addActionMapping(const std::string& name,  LLGameControl::InputChannel channel)
{
    bool success = mActionTranslator.addMapping(name, channel);
    if (success)
    {
        mLastActionFlags = 0;
    }
    else
    {
        // did not find action by name
        // maybe we're trying to map a camera action
        success = mCameraTranslator.addMapping(name, channel);
        if (success)
        {
            mLastCameraActionFlags = 0;
        }
    }
}

// static
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

// static
void LLGameControllerManager::setExternalActionFlags(U32 action_flags)
{
    if (g_translateAgentActions)
    {
        U32 active_flags = action_flags & mActionTranslator.getMappedFlags();
        if (active_flags != mLastActionFlags)
        {
            mLastActionFlags = active_flags;
            mExternalState = mActionTranslator.computeStateFromFlags(action_flags);
        }
    }
}

void LLGameControllerManager::clear()
{
    mControllerIDs.clear();
    mControllers.clear();
    mStates.clear();
}

U64 get_now_nsec()
{
    std::chrono::time_point<std::chrono::steady_clock> t0;
    return (std::chrono::steady_clock::now() - t0).count();
}

// util for dumping SDL_GameController info
std::ostream& operator<<(std::ostream& out, SDL_GameController* c)
{
    if (! c)
    {
        return out << "nullptr";
    }
    out << "{";
    out << " name='" << SDL_GameControllerName(c) << "'";
    out << " type='" << SDL_GameControllerGetType(c) << "'";
    out << " vendor='" << SDL_GameControllerGetVendor(c) << "'";
    out << " product='" << SDL_GameControllerGetProduct(c) << "'";
    out << " version='" << SDL_GameControllerGetProductVersion(c) << "'";
    //CRASH! out << " serial='" << SDL_GameControllerGetSerial(c) << "'";
    out << " }";
    return out;
}

// util for dumping SDL_Joystick info
std::ostream& operator<<(std::ostream& out, SDL_Joystick* j)
{
    if (! j)
    {
        return out << "nullptr";
    }
    out << "{";
    out << " p=0x" << (void*)(j);
    out << " name='" << SDL_JoystickName(j) << "'";
    out << " type='" << SDL_JoystickGetType(j) << "'";
    out << " instance='" << SDL_JoystickInstanceID(j) << "'";
    out << " product='" << SDL_JoystickGetProduct(j) << "'";
    out << " version='" << SDL_JoystickGetProductVersion(j) << "'";
    out << " num_axes=" << SDL_JoystickNumAxes(j);
    out << " num_balls=" << SDL_JoystickNumBalls(j);
    out << " num_hats=" << SDL_JoystickNumHats(j);
    out << " num_buttons=" << SDL_JoystickNumHats(j);
    out << " }";
    return out;
}

void onControllerDeviceAdded(const SDL_Event& event)
{
    int device_index = event.cdevice.which;
    SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(device_index);
    SDL_GameController* controller = SDL_GameControllerOpen(device_index);

    g_manager.addController(id, controller);
}

void onControllerDeviceRemoved(const SDL_Event& event)
{
    SDL_JoystickID id = event.cdevice.which;
    g_manager.removeController(id);
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
bool LLGameControl::isInitialized()
{
    return g_gameControl != nullptr;
}

void sdl_logger(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    LL_DEBUGS("SDL2") << "log='" << message << "'" << LL_ENDL;
}

// static
void LLGameControl::init()
{
    if (!g_gameControl)
    {
        g_gameControl = LLGameControl::getInstance();
        SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
        SDL_LogSetOutputFunction(&sdl_logger, nullptr);
    }
    LLGameControl::InputChannel::initChannelMap();
}

// static
void LLGameControl::terminate()
{
    g_manager.clear();
    SDL_Quit();
}

//static
// returns 'true' if GameControlInput message needs to go out,
// which will be the case for new data or resend. Call this right
// before deciding to put a GameControlInput packet on the wire
// or not.
bool LLGameControl::computeFinalStateAndCheckForChanges()
{
    // Note: LLGameControllerManager::computeFinalState() can modify g_nextResendPeriod as a side-effect
    g_manager.computeFinalState(g_finalState);

    // should_send_input is 'true' when g_nextResendPeriod has been zeroed
    // or the last send really has expired.
    U64 now = get_now_nsec();
    bool should_send_input = (g_lastSend + g_nextResendPeriod < now);
    return should_send_input;
}

// static
void LLGameControl::clearAllState()
{
    g_manager.clearAllState();
}

// static
void LLGameControl::processEvents(bool app_has_focus)
{
    SDL_Event event;
    if (!app_has_focus)
    {
        // when SL window lacks focus: pump SDL events but ignore them
        while (g_gameControl && SDL_PollEvent(&event))
        {
            // do nothing: SDL_PollEvent() is the operator
        }
        g_manager.clearAllState();
        return;
    }

    while (g_gameControl && SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_CONTROLLERDEVICEADDED:
                onControllerDeviceAdded(event);
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                onControllerDeviceRemoved(event);
                break;
            case SDL_CONTROLLERBUTTONDOWN:
                /* FALLTHROUGH */
            case SDL_CONTROLLERBUTTONUP:
                onControllerButton(event);
                break;
            case SDL_CONTROLLERAXISMOTION:
                onControllerAxis(event);
                break;
            default:
                break;
        }
    }
}

// static
const LLGameControl::State& LLGameControl::getState()
{
    return g_finalState;
}

// static
void LLGameControl::enableSendToServer(bool enable)
{
    g_sendToServer = enable;
}

// static
void LLGameControl::enableControlAgent(bool enable)
{
    g_controlAgent = enable;
}

// static
void LLGameControl::enableTranslateAgentActions(bool enable)
{
    g_translateAgentActions = enable;
}

void LLGameControl::setAgentControlMode(LLGameControl::AgentControlMode mode)
{
    g_agentControlMode = mode;
}

// static
bool LLGameControl::willSendToServer()
{
    return g_sendToServer;
}

// static
bool LLGameControl::willControlAvatar()
{
    return g_controlAgent && g_agentControlMode == CONTROL_MODE_AVATAR;
}

// static
bool LLGameControl::willControlFlycam()
{
    return g_controlAgent && g_agentControlMode == CONTROL_MODE_FLYCAM;
}

// static
bool LLGameControl::willTranslateAgentActions()
{
    return g_translateAgentActions;
}

/*
// static
LLGameControl::LocalControlMode LLGameControl::getLocalControlMode()
{
    return g_agentControlMode;
}
*/

// static
LLGameControl::InputChannel LLGameControl::getChannelByName(const std::string& name)
{
    std::unordered_map<std::string, LLGameControl::InputChannel>::const_iterator itr;
    itr = gChannelMap.find(name);
    LLGameControl::InputChannel channel;
    if (itr != gChannelMap.end())
    {
        channel = itr->second;
    }
    return channel;
}

// static
LLGameControl::InputChannel LLGameControl::getChannelByActionName(const std::string& name)
{
    return g_manager.getChannelByActionName(name);
}

// static
void LLGameControl::addActionMapping(const std::string& name,  LLGameControl::InputChannel channel)
{
    g_manager.addActionMapping(name, channel);
}

// static
U32 LLGameControl::computeInternalActionFlags()
{
    return g_manager.computeInternalActionFlags();
}

// static
void LLGameControl::setExternalActionFlags(U32 action_flags)
{
    g_manager.setExternalActionFlags(action_flags);
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
        // so we only overrite g_finalState.mPrevAxes on higher resends.
        g_finalState.mPrevAxes = g_finalState.mAxes;
        g_nextResendPeriod *= RESEND_EXPANSION_RATE;
    }
}


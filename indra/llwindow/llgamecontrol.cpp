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

std::string LLGameControl::InputChannel::getLocalName() const
{
    // HACK: we hard-code English channel names, but
    // they should be loaded from localized XML config files.
    std::string name = " ";
    if (mType == LLGameControl::InputChannel::TYPE_AXIS)
    {
        if (mIndex < (U8)(NUM_AXES))
        {
            name = "AXIS_";
            name.append(std::to_string((S32)(mIndex)));
            if (mSign < 0)
            {
                name.append("-");
            }
            else if (mSign > 0)
            {
                name.append("+");
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
        switch(mIndex)
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

    void addController(SDL_JoystickID id, SDL_GameController* controller);
    void removeController(SDL_JoystickID id);

    void onAxis(SDL_JoystickID id, U8 axis, S16 value);
    void onButton(SDL_JoystickID id, U8 button, bool pressed);

    void clearAllState();
    size_t getControllerIndex(SDL_JoystickID id) const;

    void accumulateInternalState();
    void computeFinalState(LLGameControl::State& state);

    LLGameControl::InputChannel getChannelByActionName(const std::string& action_name) const;
    LLGameControl::InputChannel getFlycamChannelByActionName(const std::string& action_name) const;

    bool updateActionMap(const std::string& name,  LLGameControl::InputChannel channel);
    U32 computeInternalActionFlags();
    void getFlycamInputs(std::vector<F32>& inputs_out);
    void setExternalInput(U32 action_flags, U32 buttons);

    void clear();

private:
    bool updateFlycamMap(const std::string& action,  LLGameControl::InputChannel channel);

    std::vector<SDL_JoystickID> mControllerIDs;
    std::vector<SDL_GameController*> mControllers;
    std::vector<LLGameControl::State> mStates; // one state per device

    LLGameControl::State mExternalState;
    LLGameControlTranslator mActionTranslator;
    std::vector<LLGameControl::InputChannel> mFlycamChannels;
    std::vector<S32> mAxesAccumulator;
    U32 mButtonAccumulator { 0 };
    U32 mLastActiveFlags { 0 };
    U32 mLastFlycamActionFlags { 0 };
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

    // Here we build an invarient map between the named agent actions
    // and control bit sent to the server.  This map will be used,
    // in combination with the action->InputChannel map below,
    // to maintain an inverse map from control bit masks to GameControl data.
    LLGameControlTranslator::ActionToMaskMap actions;
    actions["push+"]  = AGENT_CONTROL_AT_POS   | AGENT_CONTROL_FAST_AT;
    actions["push-"]  = AGENT_CONTROL_AT_NEG   | AGENT_CONTROL_FAST_AT;
    actions["slide+"] = AGENT_CONTROL_LEFT_POS | AGENT_CONTROL_FAST_LEFT;
    actions["slide-"] = AGENT_CONTROL_LEFT_NEG | AGENT_CONTROL_FAST_LEFT;
    actions["jump+"]  = AGENT_CONTROL_UP_POS   | AGENT_CONTROL_FAST_UP;
    actions["jump-"]  = AGENT_CONTROL_UP_NEG   | AGENT_CONTROL_FAST_UP;
    actions["turn+"]  = AGENT_CONTROL_YAW_POS;
    actions["turn-"]  = AGENT_CONTROL_YAW_NEG;
    actions["look+"]  = AGENT_CONTROL_PITCH_POS;
    actions["look-"]  = AGENT_CONTROL_PITCH_NEG;
    actions["stop"]   = AGENT_CONTROL_STOP;
    // These are HACKs. We borrow some AGENT_CONTROL bits for "unrelated" features.
    // Not a problem because these bits are only used internally.
    actions["toggle_run"]    = AGENT_CONTROL_NUDGE_AT_POS; // HACK
    actions["toggle_fly"]    = AGENT_CONTROL_FLY; // HACK
    actions["toggle_flycam"] = AGENT_CONTROL_NUDGE_AT_NEG; // HACK
    mActionTranslator.setAvailableActions(actions);

    // Here we build a list of pairs between named agent actions and
    // GameControl channels. Note: we only supply the non-signed names
    // (e.g. "push" instead of "push+" and "push-") because mActionTranator
    // automatially expands action names as necessary.
    using type = LLGameControl::InputChannel::Type;
    std::vector< std::pair< std::string, LLGameControl::InputChannel> > agent_defaults =
    {
        { "push",  { type::TYPE_AXIS,   (U8)(LLGameControl::AXIS_LEFTY),       1 } },
        { "slide", { type::TYPE_AXIS,   (U8)(LLGameControl::AXIS_LEFTX),       1 } },
        { "jump",  { type::TYPE_AXIS,   (U8)(LLGameControl::AXIS_TRIGGERLEFT), 1 } },
        { "turn",  { type::TYPE_AXIS,   (U8)(LLGameControl::AXIS_RIGHTX),      1 } },
        { "look",  { type::TYPE_AXIS,   (U8)(LLGameControl::AXIS_RIGHTY),      1 } },
        { "toggle_run",    { type::TYPE_BUTTON, (U8)(LLGameControl::BUTTON_LEFTSHOULDER) }  },
        { "toggle_fly",    { type::TYPE_BUTTON, (U8)(LLGameControl::BUTTON_DPAD_UP) }       },
        { "toggle_flycam", { type::TYPE_BUTTON, (U8)(LLGameControl::BUTTON_RIGHTSHOULDER) } },
        { "stop",          { type::TYPE_BUTTON, (U8)(LLGameControl::BUTTON_LEFTSTICK) }     }
    };
    mActionTranslator.setMappings(agent_defaults);

    // Flycam actions don't need bitwise translation, so we maintain the map
    // of channels here directly rather than using an LLGameControlTranslator.
    mFlycamChannels = {
        { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_LEFTY),        1 }, // advance
        { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_LEFTX),        1 }, // pan
        { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_TRIGGERRIGHT), 1 }, // rise
        { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_RIGHTY),      -1 }, // pitch
        { type::TYPE_AXIS, (U8)(LLGameControl::AXIS_RIGHTX),       1 }, // yaw
        { type::TYPE_NONE, 0                                         }  // zoom
    };
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
    mLastActiveFlags = 0;
    mLastFlycamActionFlags = 0;
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
    }
    if (old_buttons != final_state.mButtons)
    {
        g_nextResendPeriod = 0; // packet needs to go out ASAP
    }

    // clamp the accumulated axes
    for (size_t i = 0; i < NUM_AXES; ++i)
    {
        S32 axis = mAxesAccumulator[i];
        if (g_translateAgentActions)
        {
            // Note: we accumulate mExternalState onto local 'axis' variable
            // rather than onto mAxisAccumulator[i] because the internal
            // accumulated value is also used to drive the Flycam, and
            // we don't want any external state leaking into that value.
            axis += (S32)(mExternalState.mAxes[i]);
        }
        axis = (S16)(std::min(std::max(axis, -32768), 32767));
        // check for change
        if (final_state.mAxes[i] != axis)
        {
            // When axis changes we explicitly update the corresponding prevAxis
            // prior to storing axis.  The only other place where prevAxis
            // is updated in updateResendPeriod() which is explicitly called after
            // a packet is sent.  The result is: unchanged axes are included in
            // first resend but not later ones.
            final_state.mPrevAxes[i] = final_state.mAxes[i];
            final_state.mAxes[i] = axis;
            g_nextResendPeriod = 0; // packet needs to go out ASAP
        }
    }
}

LLGameControl::InputChannel LLGameControllerManager::getChannelByActionName(const std::string& name) const
{
    LLGameControl::InputChannel channel = mActionTranslator.getChannelByAction(name);
    if (channel.isNone())
    {
        // maybe we're looking for a flycam action
        channel = getFlycamChannelByActionName(name);
    }
    return channel;
}

// helper
S32 get_flycam_index_by_name(const std::string& name)
{
    // the Flycam action<-->channel relationship
    // is implicitly stored in std::vector in a known order
    S32 index = -1;
    if (name.rfind("advance", 0) == 0)
    {
        index = 0;
    }
    else if (name.rfind("pan", 0) == 0)
    {
        index = 1;
    }
    else if (name.rfind("rise", 0) == 0)
    {
        index = 2;
    }
    else if (name.rfind("pitch", 0) == 0)
    {
        index = 3;
    }
    else if (name.rfind("yaw", 0) == 0)
    {
        index = 4;
    }
    else if (name.rfind("zoom", 0) == 0)
    {
        index = 5;
    }
    return index;
}

LLGameControl::InputChannel LLGameControllerManager::getFlycamChannelByActionName(const std::string& name) const
{
    // the Flycam channels are stored in a strict order
    LLGameControl::InputChannel channel;
    S32 index = get_flycam_index_by_name(name);
    if (index != -1)
    {
        channel = mFlycamChannels[index];
    }
    return channel;
}

bool LLGameControllerManager::updateActionMap(const std::string& action,  LLGameControl::InputChannel channel)
{
    bool success = mActionTranslator.updateMap(action, channel);
    if (success)
    {
        mLastActiveFlags = 0;
    }
    else
    {
        // maybe we're looking for a flycam action
        success = updateFlycamMap(action, channel);
    }
    if (!success)
    {
        LL_WARNS("GameControl") << "unmappable action='" << action << "'" << LL_ENDL;
    }
    return success;
}

bool LLGameControllerManager::updateFlycamMap(const std::string& action,  LLGameControl::InputChannel channel)
{
    S32 index = get_flycam_index_by_name(action);
    if (index != -1)
    {
        mFlycamChannels[index] = channel;
        return true;
    }
    return false;
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
        if (channel.mIndex == (U8)(LLGameControl::AXIS_TRIGGERLEFT)
            || channel.mIndex == (U8)(LLGameControl::AXIS_TRIGGERRIGHT))
        {
            // TIED TRIGGER HACK: we assume the two triggers are paired together
            S32 total_axis = mAxesAccumulator[(U8)(LLGameControl::AXIS_TRIGGERLEFT)]
                - mAxesAccumulator[(U8)(LLGameControl::AXIS_TRIGGERRIGHT)];
            if (channel.mIndex == (U8)(LLGameControl::AXIS_TRIGGERRIGHT))
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
        F32 input = F32(axis) * ((axis > 0.0f) ? 3.051850476e-5 : 3.0517578125e-5f) * channel.mSign;
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
    // Note: LLGameControllerManager::computeFinalState() modifies g_nextResendPeriod as a side-effect
    g_manager.computeFinalState(g_finalState);

    // should send input when:
    //     sending is enabled and
    //     g_lastSend has "expired"
    //         either because g_nextResendPeriod has been zeroed
    //         or the last send really has expired.
    return g_sendToServer && (g_lastSend + g_nextResendPeriod < get_now_nsec());
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
void LLGameControl::getFlycamInputs(std::vector<F32>& inputs_out)
{
    return g_manager.getFlycamInputs(inputs_out);
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
bool LLGameControl::willControlAvatar()
{
    return g_controlAgent && g_agentControlMode == CONTROL_MODE_AVATAR;
}

// static
//
// Given a name like "AXIS_1-" or "BUTTON_5" returns the corresponding InputChannel
// If the axis name lacks the +/- postfix it assumes '+' postfix.
LLGameControl::InputChannel LLGameControl::getChannelByName(const std::string& name)
{
    LLGameControl::InputChannel channel;
    // 'name' has two acceptable formats: AXIS_<index>[sign] or BUTTON_<index>
    if (name.length() < 6)
    {
        // name must be at least as long as 'AXIS_n'
        return channel;
    }
    if (name.rfind("AXIS_", 0) == 0)
    {
        char c = name[5];
        if (c >= '0')
        {
            channel.mType = LLGameControl::InputChannel::Type::TYPE_AXIS;
            channel.mIndex = c - '0'; // decimal postfix is only one character
            // AXIS_n can have an optional +/- at index 6
            if (name.length() >= 6)
            {
                channel.mSign = (name[6] == '-') ? -1 : 1;
            }
            else
            {
                // assume positive axis when sign not provided
                channel.mSign = 1;
            }
        }
    }
    else if (name.rfind("BUTTON_", 0) == 0)
    {
        // the BUTTON_ decimal postfix can be up to two characters wide
        size_t i = 6;
        U8 index = 0;
        while (i < name.length() && i < 8 && name[i] >= '0')
        {
            index = index * 10 + name[i] - '0';
        }
        channel.mType = LLGameControl::InputChannel::Type::TYPE_BUTTON;
        channel.mIndex = index;
    }
    return channel;
}

// static
// Given an action_name like "push+", or "strafe-", returns the InputChannel
// mapped to it if found, else channel.isNone() will be true.
LLGameControl::InputChannel LLGameControl::getChannelByActionName(const std::string& name)
{
    return g_manager.getChannelByActionName(name);
}

// static
bool LLGameControl::updateActionMap(const std::string& action_name,  LLGameControl::InputChannel channel)
{
    return g_manager.updateActionMap(action_name, channel);
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


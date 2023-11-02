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
#include <map>

#include "SDL2/SDL.h"
#include "SDL2/SDL_gamecontroller.h"
#include "SDL2/SDL_joystick.h"

constexpr size_t NUM_AXES = 6;

// internal class for managing list of controllers and per-controller state
class LLGameControllerManager
{
public:
    void addController(SDL_JoystickID id, SDL_GameController* controller);
    void removeController(SDL_JoystickID id);

    void onAxis(SDL_JoystickID id, U8 axis, S16 value);
    void onButton(SDL_JoystickID id, U8 button, bool pressed);

    void onKeyButton(U8 button, bool pressed);
    void onKeyAxis(U8 axis, U16 value);

    void clearAllInput();
    void clearAllKeys();
    size_t getControllerIndex(SDL_JoystickID id) const;

    void computeFinalState(LLGameControl::State& state);

    void clear();

private:
    std::vector<SDL_JoystickID> mControllerIDs;
    std::vector<SDL_GameController*> mControllers;
    std::vector<LLGameControl::State> mStates;
    LLGameControl::State mKeyboardState;
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
    LLGameControl::State g_gameControlState;
    U64 g_lastSend = 0;
    U64 g_nextResendPeriod = FIRST_RESEND_PERIOD;

    std::map<U16, U8> g_keyButtonMap;
    std::map<U16, U8> g_keyAxisMapPositive;
    std::map<U16, U8> g_keyAxisMapNegative;

    bool g_includeKeyboardButtons = false;

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

void LLGameControllerManager::onKeyButton(U8 button, bool pressed)
{
    if (mKeyboardState.onButton(button, pressed))
    {
        LL_DEBUGS("SDL2") << " keyboard button i=" << (S32)(button) << " pressed=" <<  pressed << LL_ENDL;
    }
}

void LLGameControllerManager::onKeyAxis(U8 axis, U16 value)
{
    if (mKeyboardState.mAxes[axis] != value)
    {
        mKeyboardState.mAxes[axis] = value;
        LL_DEBUGS("SDL2") << " keyboard axis i=" << (S32)(axis) << " value=" << (S32)(value) << LL_ENDL;
    }
}

void LLGameControllerManager::clearAllInput()
{
    for (auto& state : mStates)
    {
        state.mButtons = 0;
        std::fill(state.mAxes.begin(), state.mAxes.end(), 0);
    }
    mKeyboardState.mButtons = 0;
    std::fill(mKeyboardState.mAxes.begin(), mKeyboardState.mAxes.end(), 0);
}

void LLGameControllerManager::clearAllKeys()
{
    mKeyboardState.mButtons = 0;
    std::fill(mKeyboardState.mAxes.begin(), mKeyboardState.mAxes.end(), 0);
}

void LLGameControllerManager::computeFinalState(LLGameControl::State& state)
{
    // clear the slate
    std::vector<S32> axes_accumulator;
    axes_accumulator.resize(NUM_AXES, 0);
    U32 old_buttons = state.mButtons;
    state.mButtons = 0;

    // accumulate the controllers
    for (const auto& s : mStates)
    {
        state.mButtons |= s.mButtons;
        for (size_t i = 0; i < NUM_AXES; ++i)
        {
            axes_accumulator[i] += (S32)(s.mAxes[i]);
        }
    }

    // accumulate the keyboard
    state.mButtons |= mKeyboardState.mButtons;
    for (size_t i = 0; i < NUM_AXES; ++i)
    {
        axes_accumulator[i] += (S32)(mKeyboardState.mAxes[i]);
    }
    if (old_buttons != state.mButtons)
    {
        g_nextResendPeriod = 0; // packet needs to go out ASAP
    }

    // clamp the axes
    for (size_t i = 0; i < NUM_AXES; ++i)
    {
        S32 new_axis = (S16)(std::min(std::max(axes_accumulator[i], -32768), 32767));
        // check for change
        if (state.mAxes[i] != new_axis)
        {
            // When axis changes we explicitly update the corresponding prevAxis
            // otherwise, we let prevAxis get updated in updateResendPeriod()
            // which is explicitly called after a packet is sent.  This allows
            // unchanged axes to be included in first resend but not later ones.
            state.mPrevAxes[i] = state.mAxes[i];
            state.mAxes[i] = new_axis;
            g_nextResendPeriod = 0; // packet needs to go out ASAP
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
}

// static
void LLGameControl::terminate()
{
    g_manager.clear();
    SDL_Quit();
}

// static
void LLGameControl::addKeyButtonMap(U16 key, U8 button)
{
    g_keyButtonMap[key] = button;
}

// static
void LLGameControl::removeKeyButtonMap(U16 key)
{
    g_keyButtonMap.erase(key);
}

// static
void LLGameControl::addKeyAxisMap(U16 key, U8 axis, bool positive)
{
    if (axis > MAX_AXIS)
    {
        return;
    }
    if (positive)
    {
        g_keyAxisMapPositive[key] = axis;
        g_keyAxisMapNegative.erase(key);
    }
    else
    {
        g_keyAxisMapNegative[key] = axis;
        g_keyAxisMapPositive.erase(key);
    }
}

// static
void LLGameControl::removeKeyAxisMap(U16 key)
{
    g_keyAxisMapPositive.erase(key);
    g_keyAxisMapNegative.erase(key);
}

// static
void LLGameControl::onKeyDown(U16 key, U32 mask)
{
    auto itr = g_keyButtonMap.find(key);
    if (itr != g_keyButtonMap.end())
    {
        g_manager.onKeyButton(itr->second, true);
    }
    else
    {
        itr = g_keyAxisMapPositive.find(key);
        if (itr != g_keyAxisMapPositive.end())
        {
            g_manager.onKeyAxis(itr->second, 32767);
        }
        else
        {
            itr = g_keyAxisMapNegative.find(key);
            if (itr != g_keyAxisMapNegative.end())
            {
                g_manager.onKeyAxis(itr->second, -32768);
            }
        }
    }
}

// static
void LLGameControl::onKeyUp(U16 key, U32 mask)
{
    auto itr = g_keyButtonMap.find(key);
    if (itr != g_keyButtonMap.end())
    {
        g_manager.onKeyButton(itr->second, true);
    }
    else
    {
        itr = g_keyAxisMapPositive.find(key);
        if (itr != g_keyAxisMapPositive.end())
        {
            g_manager.onKeyAxis(itr->second, 0);
        }
        else
        {
            itr = g_keyAxisMapNegative.find(key);
            if (itr != g_keyAxisMapNegative.end())
            {
                g_manager.onKeyAxis(itr->second, 0);
            }
        }
    }
}

//static
// returns 'true' if GameControlInput message needs to go out,
// which will be the case for new data or resend. Call this right
// before deciding to put a GameControlInput packet on the wire
// or not.
bool LLGameControl::computeFinalInputAndCheckForChanges()
{
    g_manager.computeFinalState(g_gameControlState);
    return g_lastSend + g_nextResendPeriod < get_now_nsec();
}

// static
void LLGameControl::clearAllInput()
{
    g_manager.clearAllInput();
}

// static
void LLGameControl::clearAllKeys()
{
    g_manager.clearAllKeys();
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
        clearAllInput();
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
    return g_gameControlState;
}

// static
void LLGameControl::setIncludeKeyboardButtons(bool include)
{
    g_includeKeyboardButtons = include;
}

// static
bool LLGameControl::getIncludeKeyboardButtons()
{
    return g_includeKeyboardButtons;
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
        // In other words: we want to include changed axes in the first resend
        // so we only overrite g_gameControlState.mPrevAxes on higher resends.
        g_gameControlState.mPrevAxes = g_gameControlState.mAxes;
        g_nextResendPeriod *= RESEND_EXPANSION_RATE;
    }
}


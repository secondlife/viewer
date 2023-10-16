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

#include <chrono>

#include "SDL2/SDL.h"
#include "SDL2/SDL_gamecontroller.h"
#include "SDL2/SDL_joystick.h"


// local globals
namespace
{
    LLGameControl::State g_gameControlState;
    LLGameControl* g_manager = nullptr;
    std::vector<SDL_GameController*> g_controllers;

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
    U64 g_lastSend = 0;
    U64 g_nextResendPeriod = FIRST_RESEND_PERIOD;

    bool g_includeKeyboardButtons = false;
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

    if (controller)
    {
        bool found = false;
        for (auto c : g_controllers)
        {
            if (controller == c)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            g_controllers.push_back(controller);
        }
    }
    LL_DEBUGS("SDL2") << "index=" << device_index
        << " id=0x" << std::hex << id << std::dec
        << " controller=" << controller
        << " num_controllers=" << g_controllers.size() << LL_ENDL;
}

void onControllerDeviceRemoved(const SDL_Event& event)
{
    int device_index = event.cdevice.which;
    SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(device_index);

    SDL_GameController* controller = SDL_GameControllerFromInstanceID(id);
    if (controller)
    {
        size_t num_controllers = g_controllers.size();
        for (size_t i = 0; i < num_controllers; ++i)
        {
            if (controller == g_controllers[i])
            {
                if (i < num_controllers - 1)
                {
                    g_controllers[i] = g_controllers[num_controllers - 1];
                }
                g_controllers.pop_back();
                break;
            }
        }
    }
    LL_DEBUGS("SDL2") << "index=" << device_index
        << " id=0x" << std::hex << id << std::dec
        << " controller=" << controller
        << " num_controllers=" << g_controllers.size() << LL_ENDL;
}

void onControllerButton(const SDL_Event& event)
{
    LLGameControl::onButton(event.cbutton.button, event.cbutton.state == SDL_PRESSED);
}

void onControllerAxis(const SDL_Event& event)
{
    U8 axis = event.caxis.axis;
    if (axis < g_gameControlState.mAxes.size())
    {
        S16 value = event.caxis.value;
        LL_DEBUGS("SDL2") << " axis i=" << (S32)(event.caxis.axis)
            << " state:" << g_gameControlState.mAxes[axis] << "-->" << value << LL_ENDL;
        g_gameControlState.mAxes[axis] = value;
        g_nextResendPeriod = 0;
    }
}

LLGameControl::~LLGameControl()
{
    terminate();
}

// static
void LLGameControl::onButton(U8 button, bool pressed)
{
    // find the pressed button's index if it exists
    std::vector<U8>& buttons = g_gameControlState.mPressedButtons;
    S32 num_buttons = (S32)(buttons.size());
    S32 button_index = -1;
    for (S32 i = 0; i < num_buttons; ++i)
    {
        if (button == buttons[i])
        {
            button_index = i;
            break;
        }
    }

    // maintain a list of pressed buttons
    if (pressed)
    {
        // add button
        if (button_index == -1)
        {
            buttons.push_back(button);
        }
    }
    else
    {
        // remove button
        if (button_index > -1)
        {
            if (button_index != num_buttons - 1)
            {
                buttons[button_index] = buttons[num_buttons - 1];
            }
            buttons.pop_back();
        }
    }

    LL_DEBUGS("SDL2") << " button i=" << (S32)(button)
        << " state:" << (!pressed) << "-->" << pressed << LL_ENDL;
    g_nextResendPeriod = 0;
}

void LLGameControl::clearAllButtons()
{
    if (g_gameControlState.mPressedButtons.size() > 0)
    {
        g_gameControlState.mPressedButtons.clear();
        g_nextResendPeriod = 0;
    }
}

// static
bool LLGameControl::isInitialized()
{
    return g_manager != nullptr;
}

void sdl_logger(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    LL_DEBUGS("SDL2") << "log='" << message << "'" << LL_ENDL;
}

// static
void LLGameControl::init()
{
    if (!g_manager)
    {
        g_manager = LLGameControl::getInstance();
        SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
        SDL_LogSetOutputFunction(&sdl_logger, nullptr);
    }
}

// static
void LLGameControl::terminate()
{
    SDL_Quit();
    g_manager = nullptr;
}

// static
void LLGameControl::processEvents(bool app_has_focus)
{
    SDL_Event event;
    if (!app_has_focus)
    {
        // when SL window lacks focus: pump SDL events but ignore them
        while (g_manager && SDL_PollEvent(&event))
        {
            // ignore
        }
        clearAllButtons();
        return;
    }

    while (g_manager && SDL_PollEvent(&event))
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
bool LLGameControl::hasInput()
{
    return g_lastSend + g_nextResendPeriod < get_now_nsec();
}

//static
void LLGameControl::clearInput()
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
        g_gameControlState.mPrevAxes = g_gameControlState.mAxes;
        g_nextResendPeriod *= RESEND_EXPANSION_RATE;
    }
}


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

#include "SDL2/SDL.h"
#include "SDL2/SDL_gamecontroller.h"
#include "SDL2/SDL_joystick.h"


// globals
LLGameControl::State g_gameControlState;
LLGameControl* g_manager = nullptr;
std::vector<SDL_GameControllerAxis> g_axisEnums;
std::vector<SDL_GameControllerButton> g_buttonEnums;
std::vector<SDL_GameController*> g_controllers;
bool g_hasInput = false;


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
        g_hasInput = true;
    }
}

/* SDL keyboard events don't work without an SDL window
   and we aren't using SDL for window management
void onKeyboard(const SDL_Event& event)
{
    SDL_Keycode sym = event.key.keysym.sym;
    U32 mod = event.key.keysym.mod;
    U8 state = event.key.state;
    LL_DEBUGS("SDL2") << "sym=" << (S32)(sym)
        << " mod=0x" << std::hex << mod << std::dec
        << " state=" << (S32)(state) << LL_ENDL;
}
*/

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
    g_hasInput = true;
}

void LLGameControl::clearAllButtons()
{
    if (g_gameControlState.mPressedButtons.size() > 0)
    {
        g_gameControlState.mPressedButtons.clear();
        g_hasInput = true;
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
        //SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
        SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);

        g_axisEnums = {
            SDL_CONTROLLER_AXIS_LEFTX,
            SDL_CONTROLLER_AXIS_LEFTY,
            SDL_CONTROLLER_AXIS_RIGHTX,
            SDL_CONTROLLER_AXIS_RIGHTY,
            SDL_CONTROLLER_AXIS_TRIGGERLEFT,
            SDL_CONTROLLER_AXIS_TRIGGERRIGHT
        };

        g_buttonEnums = {
            SDL_CONTROLLER_BUTTON_A,
            SDL_CONTROLLER_BUTTON_B,
            SDL_CONTROLLER_BUTTON_X,
            SDL_CONTROLLER_BUTTON_Y,
            SDL_CONTROLLER_BUTTON_BACK,
            SDL_CONTROLLER_BUTTON_GUIDE,
            SDL_CONTROLLER_BUTTON_START,
            SDL_CONTROLLER_BUTTON_LEFTSTICK,
            SDL_CONTROLLER_BUTTON_RIGHTSTICK,
            SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
            SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
            SDL_CONTROLLER_BUTTON_DPAD_UP,
            SDL_CONTROLLER_BUTTON_DPAD_DOWN,
            SDL_CONTROLLER_BUTTON_DPAD_LEFT,
            SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
            SDL_CONTROLLER_BUTTON_MISC1,
            SDL_CONTROLLER_BUTTON_PADDLE1,
            SDL_CONTROLLER_BUTTON_PADDLE2,
            SDL_CONTROLLER_BUTTON_PADDLE3,
            SDL_CONTROLLER_BUTTON_PADDLE4,
            SDL_CONTROLLER_BUTTON_TOUCHPAD
        };

        //SDL_SetHint("SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS", "1");
        //SDL_SetHint("SDL_EVENT_LOGGING", "3");
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
void LLGameControl::processEvents()
{
    SDL_Event event;
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

/*
// static
bool LLGameControl::packGameControlInput(LLMessageSystem* msg)
{
    if (g_hasInput && msg)
    {
        msg->newMessageFast(_PREHASH_GameControlInput);
        msg->nextBlock("AgentData");
        msg->addUUID("AgentID", gAgentID);
        msg->addUUID("SessionID", gAgentSessionID);

        size_t num_axes_packed = 0;
        size_t num_indices = g_gameControlState.mAxes.size();
        for (U8 i = 0; i < num_indices; ++i)
        {
            if (g_gameControlState.mAxes[i] != g_gameControlState.mPrevAxes[i])
            {
                // only pack an axis if it differes from previously packed value
                msg->nextBlockFast(_PREHASH_AxisData);
                msg->addU8Fast(_PREHASH_Index, i);
                msg->addS16Fast(_PREHASH_Value, g_gameControlState.mAxes[i]);
                g_gameControlState.mPrevAxes[i] = g_gameControlState.mAxes[i];
                ++num_axes_packed;
            }
        }

        const std::vector<U8>& buttons = g_gameControlState.mPressedButtons;
        if (!buttons.empty())
        {
            msg->nextBlockFast(_PREHASH_ButtonData);
            msg->addBinaryDataFast(_PREHASH_Data, (void*)(buttons.data()), (S32)(buttons.size()));
        }
        // TODO: remove this LL_DEBUGS, and num_axes_packed later
        LL_DEBUGS("game_control_input") << "num_axes=" << num_axes_packed << " num_buttons=" << buttons.size() << LL_ENDL;
        g_hasInput = false;
        return true;

        // TODO: resend last GameControlInput a few times on a timer
        // to help reduce the likelihood of a lost "final send"
    }
    return false;
}
*/

// static
const LLGameControl::State& LLGameControl::getState()
{
    return g_gameControlState;
}

//static
bool LLGameControl::hasInput()
{
    // TODO: modify this to trigger resends to reduce problems caused by packet loss of final state
    return g_hasInput;
}

//static
void LLGameControl::clearInput()
{
    g_gameControlState.mPrevAxes = g_gameControlState.mAxes;
    g_hasInput = false;
}


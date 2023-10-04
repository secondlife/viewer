/**
 * @file llgamepadmanager.h
 * @brief Gamepad detection and management
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

#include "llgamecontrollermanager.h"

#include "SDL2/SDL.h"
#include "SDL2/SDL_gamecontroller.h"
#include "SDL2/SDL_joystick.h"

#include "llagent.h"
#include "message.h"

// LLGameControllerState is a minimal class for storing axes and buttons values
class LLGameControllerState
{
public:
    LLGameControllerState()
    {
        mAxes.resize(SDL_CONTROLLER_AXIS_MAX, 0);
        mPrevAxes.resize(SDL_CONTROLLER_AXIS_MAX, 0);
        mButtons.resize(SDL_CONTROLLER_BUTTON_MAX, false);
    }

    std::vector<S16> mAxes; // [ -32768, 32767 ]
    std::vector<S16> mPrevAxes;
    std::vector<bool> mButtons;
};

LLGameControllerState g_controllerState;

LLGameControllerManager* g_manager = nullptr;
std::vector<SDL_GameControllerAxis> g_axisEnums;
std::vector<SDL_GameControllerButton> g_buttonEnums;
std::vector<SDL_GameController*> g_controllers;
bool g_hasInput = false;

/*
class GameController {
public:
    S32 mDeviceIndex;
    SDL_JoystickID mJoystickID;
};
std::vector<GameController> g_activeControllers;
*/

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
    U8 button = event.cbutton.button;
    if (button < g_controllerState.mButtons.size())
    {
        bool state = (bool)(event.cbutton.state == SDL_PRESSED);
        LL_DEBUGS("SDL2") << " button i=" << (S32)(event.cbutton.button)
            << " state:" << g_controllerState.mButtons[button] << "-->" << state << LL_ENDL;
        g_controllerState.mButtons[button] = state;
        g_hasInput = true;
    }
}

void onControllerAxis(const SDL_Event& event)
{
    U8 axis = event.caxis.axis;
    if (axis < g_controllerState.mAxes.size())
    {
        S16 value = event.caxis.value;
        LL_DEBUGS("SDL2") << " axis i=" << (S32)(event.caxis.axis)
            << " state:" << g_controllerState.mAxes[axis] << "-->" << value << LL_ENDL;
        g_controllerState.mAxes[axis] = value;
        g_hasInput = true;
    }
}

LLGameControllerManager::~LLGameControllerManager()
{
    terminate();
}

// static
bool LLGameControllerManager::isInitialized()
{
    return g_manager != nullptr;
}

void sdl_logger(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    LL_DEBUGS("SDL2") << "log='" << message << "'" << LL_ENDL;
}

// static
void LLGameControllerManager::init()
{
    if (!g_manager)
    {
        g_manager = LLGameControllerManager::getInstance();
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
void LLGameControllerManager::terminate()
{
    SDL_Quit();
    g_manager = nullptr;
}

// static
void LLGameControllerManager::processEvents()
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

// static
bool LLGameControllerManager::packGameControlInput(LLMessageSystem* msg)
{
    if (g_hasInput && msg)
    {
        msg->newMessageFast(_PREHASH_GameControlInput);
        msg->nextBlock("AgentData");
        msg->addUUID("AgentID", gAgentID);
        msg->addUUID("SessionID", gAgentSessionID);

        size_t num_axes_packed = 0;
        size_t num_indices = g_controllerState.mAxes.size();
        for (U8 i = 0; i < num_indices; ++i)
        {
            if (g_controllerState.mAxes[i] != g_controllerState.mPrevAxes[i])
            {
                // only pack an axis if it differes from previously packed value
                msg->nextBlockFast(_PREHASH_AxisData);
                msg->addU8Fast(_PREHASH_Index, i);
                msg->addS16Fast(_PREHASH_Value, g_controllerState.mAxes[i]);
                g_controllerState.mPrevAxes[i] = g_controllerState.mAxes[i];
                ++num_axes_packed;
            }
        }

        // we expect the typical number of simultaneously pressed buttons to be one or two
        // which means it is more compact to send the list of pressed button indices
        // instead of a U32 bitmask
        std::vector<U8> buttons;
        num_indices = g_controllerState.mButtons.size();
        for (size_t i = 0; i < num_indices; ++i)
        {
            if (g_controllerState.mButtons[i])
            {
                buttons.push_back(i);
            }
        }
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


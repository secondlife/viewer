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

/*
void onJoyDeviceAdded(const SDL_Event& event)
{
    int device_index = event.cdevice.which;
    SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(device_index);
    bool is_controller = SDL_IsGameController(device_index);
    SDL_Joystick* joystick = nullptr;
    joystick = SDL_JoystickOpen(device_index);
    LL_DEBUGS("SDL2") << "index=" << device_index
        << " is_controller=" << is_controller
        << " id=0x" << std::hex << id << std::dec
        << " joystick=" << joystick << LL_ENDL;
}

void onJoyDeviceRemoved(const SDL_Event& event)
{
    int device_index = event.cdevice.which;
    SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(device_index);
    SDL_Joystick* joystick = SDL_JoystickFromInstanceID(id);
    LL_DEBUGS("SDL2") << "index=" << device_index
        << " id=0x" << std::hex << id << std::dec
        << " joystick=" << joystick << LL_ENDL;
}
*/

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
    LL_DEBUGS("SDL2") << "button i=" << event.cbutton.button << " state=" << event.cbutton.state << LL_ENDL;
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

        g_controllerState.mAxes.resize(SDL_CONTROLLER_AXIS_MAX);
        g_controllerState.mButtons.resize(SDL_CONTROLLER_BUTTON_MAX);

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
        /*
        LL_DEBUGS("SDL2") << "type=" << event.type << LL_ENDL;
        if (event.type == SDL_JOYDEVICEADDED)
        {
            onJoyDeviceAdded(event);
        }
        else if (event.type == SDL_JOYDEVICEREMOVED)
        {
            onJoyDeviceRemoved(event);
        }
        else
        */
        if (event.type == SDL_CONTROLLERDEVICEADDED)
        {
            onControllerDeviceAdded(event);
        }
        else if (event.type == SDL_CONTROLLERDEVICEREMOVED)
        {
            onControllerDeviceRemoved(event);
        }
        else if (event.type == SDL_CONTROLLERBUTTONDOWN)
        {
            onControllerButton(event);
        }
    }

    /*
    // copy state
    for (auto controller : g_controllers)
    {
        size_t num_axes = g_controllerState.mAxes.size();
        for (size_t a = 0; a < num_axes; ++a)
        {
            if (SDL_GameControllerHasAxis(controller, g_axisEnums[a]))
            {
                S16 old_state = g_controllerState.mAxes[a];
                g_controllerState.mAxes[a] = SDL_GameControllerGetAxis(controller, g_axisEnums[a]);
                if (old_state != g_controllerState.mAxes[a])
                {
                    LL_DEBUGS("SDL2") << "a=" << a << " " << old_state << "-->" << g_controllerState.mAxes[a] << LL_ENDL;
                }
            }
        }
        size_t num_buttons = g_controllerState.mButtons.size();
        for (size_t b = 0; b < num_buttons; ++b)
        {
            if (SDL_GameControllerHasButton(controller, g_buttonEnums[b]))
            {
                bool old_state = g_controllerState.mButtons[b];
                g_controllerState.mButtons[b] = SDL_GameControllerGetButton(controller, g_buttonEnums[b]);
                if (old_state != g_controllerState.mButtons[b])
                {
                    LL_DEBUGS("SDL2") << "b=" << b << " " << old_state << "-->" << g_controllerState.mButtons[b] << LL_ENDL;
                }
            }
        }
    }
    */
}

// static
bool LLGameControllerManager::hasInput()
{
    // TODO: determine if this is useful or not
    return g_hasInput;
}

// static
const LLGameControllerState& LLGameControllerManager::getState()
{
    return g_controllerState;
}

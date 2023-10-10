/**
 * @file llgamecontrol_stub.h
 * @brief Stubbery for LLGameControl
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

#include "SDL2/SDL_events.h"


void onJoyDeviceAdded(const SDL_Event& event)
{
}

void onJoyDeviceRemoved(const SDL_Event& event)
{
}

void onControllerDeviceAdded(const SDL_Event& event)
{
}

void onControllerDeviceRemoved(const SDL_Event& event)
{
}

LLGameControl::~LLGameControl()
{
}

void LLGameControl::onButton(U8 button, bool pressed)
{
}

// static
bool LLGameControl::isInitialized()
{
    return false;
}

// static
void LLGameControl::init()
{
}

// static
void LLGameControl::terminate()
{
}

// static
void LLGameControl::processEvents()
{
}


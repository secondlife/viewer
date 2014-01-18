/** 
 * @file llviewercontrol.h
 * @brief references to viewer-specific control files
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#ifndef LL_LLVIEWERCONTROL_H
#define LL_LLVIEWERCONTROL_H

#include <map>
#include "llcontrol.h"

// Enabled this definition to compile a 'hacked' viewer that
// allows a hacked godmode to be toggled on and off.
#define TOGGLE_HACKED_GODLIKE_VIEWER 
#ifdef TOGGLE_HACKED_GODLIKE_VIEWER
extern BOOL gHackGodmode;
#endif

bool toggle_show_navigation_panel(const LLSD& newvalue);
bool toggle_show_favorites_panel(const LLSD& newvalue);

// These functions found in llcontroldef.cpp *TODO: clean this up!
//setting variables are declared in this function
void settings_setup_listeners();

// for the graphics settings
void create_graphics_group(LLControlGroup& group);

// saved at end of session
extern LLControlGroup gSavedSettings;
extern LLControlGroup gSavedPerAccountSettings;
extern LLControlGroup gWarningSettings;

// Saved at end of session
extern LLControlGroup gCrashSettings;

// Set after settings loaded
extern std::string gLastRunVersion;

#endif // LL_LLVIEWERCONTROL_H

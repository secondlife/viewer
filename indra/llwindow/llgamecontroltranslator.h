/**
 * @file llgamecontroltranslator.h
 * @brief LLGameControlTranslator class definition
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

#pragma once

#include <map>

#include "stdtypes.h"
#include "llgamecontrol.h"

// a utility for converting mapped key press events to LLGameControl::State
class LLGameControlTranslator
{
public:
    using NameToMaskMap = std::map< std::string, U32 >; // < name : mask >
    using MaskToChannelMap = std::map< U32, LLGameControl::InputChannel >; // < mask : channel >

    LLGameControlTranslator();
    LLGameControl::InputChannel getChannelByActionName(const std::string& name) const;
    void clearActionMap();
    void loadDefaults();
    bool addActionMapping(const std::string& name, const LLGameControl::InputChannel& channel);
    // Note: to remove mapping: call addMapping() with a TYPE_UNKNOWN channel
    void translateActionFlags(U32 action_flags, LLGameControl::State& state_out) const;

private:
    NameToMaskMap mNameToMask; // invariant map after init
    MaskToChannelMap mMaskToChannel; // dynamic map, per preference changes
};

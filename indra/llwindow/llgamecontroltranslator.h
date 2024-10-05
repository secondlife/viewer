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


// GameControl data is sent to the server to expose game controller input to LSL scripts,
// however not everyone will have a game controller device. To allow keyboard users to provide
// GameControl data we allow the User to configure equivalences between avatar actions
// (i.e. "push forward", "strafe left", etc) and keyboard buttons to GameControl axes
// and buttons.
//
// The LLGameControlTranslator stores the equivalences and translates avatar action_flags
// and keyboard state into GameContrl data, and in some cases the other direction: from
// LLGameControl::State into avatar action_flags.
//

class LLGameControlTranslator
{
public:

    using ActionToMaskMap = std::map< std::string, U32 >; // < action : mask >
    using MaskToChannelMap = std::map< U32, LLGameControl::InputChannel >; // < mask : channel >
    using NamedChannel = std::pair < std::string , LLGameControl::InputChannel >;
    using NamedChannels = std::vector< NamedChannel >;


    LLGameControlTranslator();
    void setAvailableActionMasks(ActionToMaskMap& action_to_mask);
    LLGameControl::InputChannel getChannelByAction(const std::string& action) const;
    void setMappings(NamedChannels& named_channels);
    void updateMap(const std::string& action, const LLGameControl::InputChannel& channel);
    // Note: to remove a mapping you can call updateMap() with a TYPE_NONE channel

    // Given external action_flags (i.e. raw avatar input)
    // compute the corresponding LLGameControl::State that would have produced those flags.
    // Note: "action_flags" are similar to, but not quite the same as, "control_flags".
    const LLGameControl::State& computeStateFromFlags(U32 action_flags);

    // Given LLGameControl::State (i.e. from a real controller)
    // compute corresponding action flags (e.g. for moving the avatar around)
    U32 computeFlagsFromState(const std::vector<S32>& axes, U32 buttons);

    U32 getMappedFlags() const { return mMappedFlags; }

private:
    void updateMapInternal(const std::string& name, const LLGameControl::InputChannel& channel);

private:
    // mActionToMask is an invarient map between the possible actions
    // and the action bit masks.  Only actions therein can have their
    // bit masks mapped to channels.
    ActionToMaskMap mActionToMask; // invariant map after init

    // mMaskToChannel is a dynamic map between action bit masks
    // and GameControl channels.
    MaskToChannelMap mMaskToChannel; // dynamic map, per preference changes

    // mCachedState is an optimization:
    // it is only recomputed when external action_flags change
    LLGameControl::State mCachedState;

    U32 mMappedFlags { 0 };
    U32 mPrevActiveFlags { 0 };
};

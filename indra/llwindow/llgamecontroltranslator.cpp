/**
 * @file llgamecontroltranslator.cpp
 * @brief LLGameControlTranslator class implementation
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

/*
 * App-wide preferences.  Note that these are not per-user,
 * because we need to load many preferences before we have
 * a login name.
 */

#include "llgamecontroltranslator.h"

#include "indra_constants.h"


LLGameControlTranslator::LLGameControlTranslator()
{
    // TODO:? generalize the API and supply defaults from XML config file

    // build the invariant list of pairs:
    // < keyboard-mapped action name : bitwise_flag >
    mNameToMask["push_forward"] = AGENT_CONTROL_AT_POS | AGENT_CONTROL_FAST_AT;
    mNameToMask["push_backward"] = AGENT_CONTROL_AT_NEG | AGENT_CONTROL_FAST_AT;
    mNameToMask["turn_left"] = AGENT_CONTROL_YAW_POS;
    mNameToMask["turn_right"] = AGENT_CONTROL_YAW_NEG;
    // WTF?  need to swap LEFT signs
    mNameToMask["slide_left"] = AGENT_CONTROL_LEFT_NEG | AGENT_CONTROL_FAST_LEFT;
    mNameToMask["slide_right"] = AGENT_CONTROL_LEFT_POS | AGENT_CONTROL_FAST_LEFT;
    mNameToMask["jump"] = AGENT_CONTROL_UP_POS | AGENT_CONTROL_FAST_UP;
    mNameToMask["push_down"] = AGENT_CONTROL_UP_NEG | AGENT_CONTROL_FAST_UP;

    // HACK: we use NUDGE flags for run_*
    //mNameToMask["run_forward"] = AGENT_CONTROL_NUDGE_AT_POS;
    //mNameToMask["run_backward"] = AGENT_CONTROL_NUDGE_AT_NEG;
    //mNameToMask["run_left"] = AGENT_CONTROL_NUDGE_LEFT_POS;
    //mNameToMask["run_right"] = AGENT_CONTROL_NUDGE_LEFT_NEG;

    mNameToMask["toggle_run"] = AGENT_CONTROL_FAST_AT; // HACK
    mNameToMask["toggle_fly"] = AGENT_CONTROL_FLY; // HACK
    mNameToMask["toggle_sit"] = AGENT_CONTROL_SIT_ON_GROUND; // HACK

    mNameToMask["stop_moving"] = AGENT_CONTROL_STOP;

    loadDefaults();
}

LLGameControl::InputChannel LLGameControlTranslator::getChannelByActionName(const std::string& name) const
{
    LLGameControl::InputChannel channel;
    LLGameControlTranslator::NameToMaskMap::const_iterator mask_itr = mNameToMask.find(name);
    if (mask_itr != mNameToMask.end())
    {
        U32 mask = mask_itr->second;
        LLGameControlTranslator::MaskToChannelMap::const_iterator channel_itr = mMaskToChannel.find(mask);
        if (channel_itr != mMaskToChannel.end())
        {
            channel = channel_itr->second;
        }
    }
    return channel;
}

void LLGameControlTranslator::clearActionMap()
{
    mMaskToChannel.clear();
    mMappedFlags = 0;
    mPrevActiveFlags = 0;
}

void LLGameControlTranslator::loadDefaults()
{
    // TODO?: load these defaults from XML config file?
    clearActionMap();

    // axes
    std::vector< std::pair< std::string, U8 > > default_axes =
    {
        { "push_forward",  (U8)(LLGameControl::AXIS_LEFTY_NEG)        },
        { "push_backward", (U8)(LLGameControl::AXIS_LEFTY_POS)        },
        { "slide_left",    (U8)(LLGameControl::AXIS_LEFTX_NEG)        },
        { "slide_right",   (U8)(LLGameControl::AXIS_LEFTX_POS)        },
        { "turn_left",     (U8)(LLGameControl::AXIS_RIGHTX_NEG)       },
        { "turn_right",    (U8)(LLGameControl::AXIS_RIGHTX_POS)       },
        { "jump",          (U8)(LLGameControl::AXIS_TRIGGERRIGHT_POS) },
        { "push_down",     (U8)(LLGameControl::AXIS_TRIGGERLEFT_POS)  }
    };
    LLGameControl::InputChannel channel;
    channel.mType = LLGameControl::InputChannel::TYPE_AXIS;
    for (auto item : default_axes)
    {
        channel.mIndex = item.second;
        addActionMapping(item.first, channel);
    }

    // buttons
    std::vector< std::pair< std::string, U8 > > default_buttons =
    {
        // TODO: figure out how to add these
        //{ "run_forward",  (U8)(LLGameControl::BUTTON_DPAD_UP)       },
        //{ "run_backward", (U8)(LLGameControl::BUTTON_DPAD_DOWN)     },
        //{ "run_left",     (U8)(LLGameControl::BUTTON_DPAD_LEFT)     },
        //{ "run_right",    (U8)(LLGameControl::BUTTON_DPAD_RIGHT)    },
        //{ "toggle_run",   (U8)(LLGameControl::BUTTON_RIGHTSHOULDER) },
        { "toggle_fly",   (U8)(LLGameControl::BUTTON_B)             },
        { "toggle_sit",   (U8)(LLGameControl::BUTTON_X)             },
        { "stop_moving",  (U8)(LLGameControl::BUTTON_LEFTSHOULDER)  }
    };
    channel.mType = LLGameControl::InputChannel::TYPE_BUTTON;
    for (auto item : default_buttons)
    {
        channel.mIndex = item.second;
        addActionMapping(item.first, channel);
    }
}

bool LLGameControlTranslator::addActionMapping(const std::string& name, const LLGameControl::InputChannel& channel)
{
    bool something_changed = false;
    LLGameControlTranslator::NameToMaskMap::iterator mask_itr = mNameToMask.find(name);
    if (mask_itr != mNameToMask.end())
    {
        U32 mask = mask_itr->second;
        LLGameControlTranslator::MaskToChannelMap::iterator channel_itr = mMaskToChannel.find(mask);
        if (channel_itr != mMaskToChannel.end())
        {
            LLGameControl::InputChannel old_channel = channel_itr->second;
            if (old_channel.mType != channel.mType || old_channel.mIndex != channel.mIndex)
            {
                if (channel.isNone())
                {
                    // remove old mapping
                    mMaskToChannel.erase(channel_itr);
                }
                else
                {
                    // update old mapping
                    channel_itr->second = channel;
                }
                something_changed = true;
            }
        }
        else if (! channel.isNone())
        {
            // create new mapping
            mMaskToChannel[mask] = channel;
            something_changed = true;
        }
    }
    else
    {
        LL_WARNS("game_control") << "unknown translation action '" << name << "'" << LL_ENDL;
    }

    // recompute mMappedFlags
    mMappedFlags = 0;
    for (auto& pair : mMaskToChannel)
    {
        mMappedFlags |= pair.first;
    }
    mPrevActiveFlags = 0;

    return something_changed;
}

// Given external action_flags (i.e. raw avatar input)
// compute the corresponding LLGameControl::State that would have produced those flags.
// Note: "action flags" are similar to, but not quite the same as, "control flags".
// "Action flags" are the raw input of avatar movement intent, whereas "control flags"
// are the consequential set of instructions that are sent to the server for moving
// the avatar character.
const LLGameControl::State& LLGameControlTranslator::computeStateFromActionFlags(U32 action_flags)
{
    static U32 last_action_flags = 0;
    if (last_action_flags != action_flags)
    {
        last_action_flags = action_flags;
    }
    // translate action_flag bits to equivalent game controller state
    // according to data in mMaskToChannel

    // only bother to update mCachedState if active_flags have changed
    U32 active_flags = action_flags & mMappedFlags;
    //if (active_flags != mPrevActiveFlags)
    {
        mCachedState.clear();
        for (const auto& pair : mMaskToChannel)
        {
            U32 mask = pair.first;
            if (mask == (mask & action_flags))
            {
                LLGameControl::InputChannel channel = pair.second;
                if (channel.isAxis())
                {
                    U8 axis_index = channel.mIndex / 2;
                    if (channel.mIndex - (axis_index * 2) > 0)
                    {
                        mCachedState.mAxes[axis_index] = std::numeric_limits<S16>::min();
                    }
                    else
                    {
                        mCachedState.mAxes[axis_index] = std::numeric_limits<S16>::max();
                    }
                }
                else if (channel.isButton())
                {
                    mCachedState.mButtons |= (0x01U << channel.mIndex);
                }
            }
        }
        mPrevActiveFlags = active_flags;
    }
    return mCachedState;
}

// Given LLGameControl::State (i.e. from a real controller)
// compute corresponding action flags (e.g. for moving the avatar around)
U32 LLGameControlTranslator::computeInternalActionFlags(const std::vector<S32>& axes, U32 buttons)
{
    // HACK: supply hard-coded threshold for ON/OFF zones
    constexpr S32 AXIS_THRESHOLD = 32768 / 8;
    U32 action_flags = 0;
    for (const auto& pair : mMaskToChannel)
    {
        // pair = { mask, channel }
        const LLGameControl::InputChannel& channel = pair.second;
        if (channel.isAxis())
        {
            U8 axis_index = channel.mIndex / 2;
            if (channel.isNegAxis())
            {
                if (axes[axis_index] < -AXIS_THRESHOLD)
                {
                    action_flags |= pair.first;
                }
            }
            else if (axes[axis_index] > AXIS_THRESHOLD)
            {
                action_flags |= pair.first;
            }
        }
        else if (channel.isButton())
        {
            U32 bit_set = buttons & (0x01U << channel.mIndex);
            if (bit_set)
            {
                action_flags |= pair.first;
            }
        }
    }
    return action_flags;
}


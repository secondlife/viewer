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
#include "llsd.h"


using ActionToMaskMap = LLGameControlTranslator::ActionToMaskMap;

LLGameControlTranslator::LLGameControlTranslator()
{
}

void LLGameControlTranslator::setAvailableActionMasks(ActionToMaskMap& action_to_mask)
{
    mActionToMask = std::move(action_to_mask);
}

LLGameControl::InputChannel LLGameControlTranslator::getChannelByAction(const std::string& action) const
{
    LLGameControl::InputChannel channel;
    auto mask_itr = mActionToMask.find(action);
    if (mask_itr != mActionToMask.end())
    {
        U32 mask = mask_itr->second;
        auto channel_itr = mMaskToChannel.find(mask);
        if (channel_itr != mMaskToChannel.end())
        {
            channel = channel_itr->second;
        }
    }
    else
    {
        // It is expected that sometimes 'action' lacks the postfix '+' or '-'.
        // When it is missing we append '+' and try again.
        std::string action_plus = action;
        action_plus.append("+");
        mask_itr = mActionToMask.find(action_plus);
        if (mask_itr != mActionToMask.end())
        {
            U32 mask = mask_itr->second;
            auto channel_itr = mMaskToChannel.find(mask);
            if (channel_itr != mMaskToChannel.end())
            {
                channel = channel_itr->second;
            }
        }
    }
    return channel;
}

void LLGameControlTranslator::setMappings(LLGameControlTranslator::NamedChannels& named_channels)
{
    mMaskToChannel.clear();
    mMappedFlags = 0;
    mPrevActiveFlags = 0;
    mCachedState.clear();

    for (const auto& named_channel : named_channels)
    {
        updateMap(named_channel.first, named_channel.second);
    }
}

void LLGameControlTranslator::updateMap(const std::string& action, const LLGameControl::InputChannel& channel)
{
    // First, get the action name type
    LLGameControl::ActionNameType actionNameType = LLGameControl::getActionNameType(action);
    if (actionNameType == LLGameControl::ACTION_NAME_UNKNOWN ||
        actionNameType == LLGameControl::ACTION_NAME_FLYCAM)
    {
        LL_WARNS("SDL2") << "unmappable action='" << action << "' (type=" << actionNameType << ")" << LL_ENDL;
        return;
    }

    // Second, get the expected associated channel type (except of TYPE_NONE)
    LLGameControl::InputChannel::Type expectedChannelType =
        actionNameType == LLGameControl::ACTION_NAME_BINARY ?
        LLGameControl::InputChannel::TYPE_BUTTON :
        LLGameControl::InputChannel::TYPE_AXIS;
    if (!channel.isNone() && (channel.mType != expectedChannelType))
    {
        LL_WARNS("SDL2") << "unmappable channel (type=" << channel.mType << ")"
            << " for action='" << action << "' (type=" << actionNameType << ")" << LL_ENDL;
        return;
    }

    if (actionNameType == LLGameControl::ACTION_NAME_ANALOG) // E.g., "push"
    {
        // Special (double) processing for analog action names
        // sequentially adding '+' and '-' to the given action
        updateMapInternal(action + "+", channel);

        // For the channel type TYPE_NONE we can map the same channel
        // In fact, the mapping will be removed from the mapping list
        if (channel.isNone())
        {
            updateMapInternal(action + "-", channel);
        }
        else
        {
            // For the channel type except of TYPE_NONE we construct the other channel
            // with the same type and index but with the opposite sign
            LLGameControl::InputChannel other_channel(channel.mType, channel.mIndex, -channel.mSign);

            // TIED TRIGGER HACK: this works for XBox and similar controllers,
            // and those are pretty much the only supported devices right now
            // however TODO: figure out how to do this better.
            //
            // AXIS_TRIGGERLEFT and AXIS_TRIGGERRIGHT are separate axes and most devices
            // only allow them to read positive, not negative. When used for motion control
            // they are typically paired together. We assume as much here when computing
            // the other_channel.
            if (channel.mIndex == LLGameControl::AXIS_TRIGGERLEFT)
            {
                other_channel.mIndex = LLGameControl::AXIS_TRIGGERRIGHT;
                other_channel.mSign = 1;
            }
            else if (channel.mIndex == LLGameControl::AXIS_TRIGGERRIGHT)
            {
                other_channel.mIndex = LLGameControl::AXIS_TRIGGERLEFT;
                other_channel.mSign = 1;
            }
            updateMapInternal(action + "-", other_channel);
        }
    }
    else
    {
        // Simple (single) processing for other action name types
        updateMapInternal(action, channel);
    }

    // recompute mMappedFlags
    mMappedFlags = 0;
    for (auto& pair : mMaskToChannel)
    {
        mMappedFlags |= pair.first;
    }
    mPrevActiveFlags = 0;
    mCachedState.clear();
}

// Given external action_flags (i.e. raw avatar input)
// compute the corresponding LLGameControl::State that would have produced those flags.
// Note: "action flags" are similar to, but not quite the same as, "control flags".
// "Action flags" are the raw input of avatar movement intent, whereas "control flags"
// are the consequential set of instructions that are sent to the server for moving
// the avatar character.
const LLGameControl::State& LLGameControlTranslator::computeStateFromFlags(U32 action_flags)
{
    // translate action_flag bits to equivalent game controller state
    // according to data in mMaskToChannel

    // only bother to update mCachedState if active_flags have changed
    U32 active_flags = action_flags & mMappedFlags;
    if (active_flags != mPrevActiveFlags)
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
                    if (channel.mSign < 0)
                    {
                        mCachedState.mAxes[channel.mIndex] = std::numeric_limits<S16>::min();
                    }
                    else
                    {
                        mCachedState.mAxes[channel.mIndex] = std::numeric_limits<S16>::max();
                    }
                }
                else if (channel.isButton())
                {
                    mCachedState.mButtons |= 0x01U << channel.mIndex;
                }
            }
        }
        mPrevActiveFlags = active_flags;
    }
    return mCachedState;
}

// Given LLGameControl::State (i.e. from a real controller)
// compute corresponding action flags (e.g. for moving the avatar around)
U32 LLGameControlTranslator::computeFlagsFromState(const std::vector<S32>& axes, U32 buttons)
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
            if (channel.mSign < 0)
            {
                if (axes[channel.mIndex] < -AXIS_THRESHOLD)
                {
                    action_flags |= pair.first;
                }
            }
            else if (axes[channel.mIndex] > AXIS_THRESHOLD)
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

void LLGameControlTranslator::updateMapInternal(const std::string& name, const LLGameControl::InputChannel& channel)
{
    auto action_it = mActionToMask.find(name);
    llassert(action_it != mActionToMask.end());
    U32 mask = action_it->second;
    auto channel_it = mMaskToChannel.find(mask);
    if (channel_it == mMaskToChannel.end())
    {
        // create new mapping
        mMaskToChannel[mask] = channel;
    }
    else if (channel.isNone())
    {
        // remove old mapping
        mMaskToChannel.erase(channel_it);
    }
    else
    {
        // update old mapping
        channel_it->second = channel;
    }
}


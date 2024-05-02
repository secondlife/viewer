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

void LLGameControlTranslator::setAvailableActions(ActionToMaskMap& action_to_mask)
{
    mActionToMask = std::move(action_to_mask);
}

LLGameControl::InputChannel LLGameControlTranslator::getChannelByAction(const std::string& action) const
{
    LLGameControl::InputChannel channel;
    ActionToMaskMap::const_iterator mask_itr = mActionToMask.find(action);
    if (mask_itr != mActionToMask.end())
    {
        U32 mask = mask_itr->second;
        LLGameControlTranslator::MaskToChannelMap::const_iterator channel_itr = mMaskToChannel.find(mask);
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
            LLGameControlTranslator::MaskToChannelMap::const_iterator channel_itr = mMaskToChannel.find(mask);
            if (channel_itr != mMaskToChannel.end())
            {
                channel = channel_itr->second;
            }
        }
    }
    return channel;
}

void LLGameControlTranslator::setMappings(LLGameControlTranslator::NamedChannels& list)
{
    mMaskToChannel.clear();
    mMappedFlags = 0;
    mPrevActiveFlags = 0;
    mCachedState.clear();

    for (auto& name_channel : list)
    {
        updateMap(name_channel.first, name_channel.second);
    }
}

bool LLGameControlTranslator::updateMap(const std::string& name, const LLGameControl::InputChannel& channel)
{
    bool map_changed = false;
    size_t name_length = name.length();
    if (name_length > 1)
    {
        if (channel.isButton())
        {
            map_changed = updateMapInternal(name, channel);
        }
        else if (channel.isAxis())
        {
            U8 last_char = name.at(name_length - 1);
            if (last_char == '+' || last_char == '=')
            {
                map_changed = updateMapInternal(name, channel);
            }
            else
            {
                // try to map both "name+" and "name-"
                std::string new_name = name;
                new_name.append("+");
                bool success = updateMapInternal(new_name, channel);
                if (success)
                {
                    new_name.data()[name_length] = '-';
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
                    updateMapInternal(new_name, other_channel);
                    map_changed = true;
                }
            }
        }
        else
        {
            // channel type is NONE, which means the action needs to be removed from the map
            // but we don't know if it mapped to button or axis which is important because
            // it if it axis then we need to also remove the other entry.
            // So we try to look it up
            ActionToMaskMap::iterator mask_itr = mActionToMask.find(name);
            if (mask_itr != mActionToMask.end())
            {
                // we found the action --> was it mapped to an axis?
                bool is_axis = false;
                U32 mask = mask_itr->second;
                LLGameControlTranslator::MaskToChannelMap::iterator channel_itr = mMaskToChannel.find(mask);
                if (channel_itr != mMaskToChannel.end())
                {
                    if (channel_itr->second.isAxis())
                    {
                        // yes, it is an axis
                        is_axis = true;
                    }
                }
                // remove from map, whether button or axis
                updateMapInternal(name, channel);

                if (is_axis)
                {
                    // also need to remove the other entry
                    std::string other_name = name;
                    if (other_name.data()[name.length() - 1] == '-')
                    {
                        other_name.data()[name.length() - 1] = '+';
                    }
                    else
                    {
                        other_name.data()[name.length() - 1] = '-';
                    }
                    // remove from map
                    updateMapInternal(other_name, channel);
                }
            }
            else if (name.data()[name.length() - 1] == '+'
                    || name.data()[name.length() - 1] == '-')
            {
                // action was not found but name doesn't end with +/-
                // maybe it is an axis-name sans the +/- on the end
                // postfix with '+' and try again
                std::string other_name = name;
                other_name.append("+");
                map_changed = updateMapInternal(other_name, channel);
                if (map_changed)
                {
                    // that worked! now do the other one
                    other_name.data()[name.length()] = '-';
                    updateMapInternal(other_name, channel);
                }
            }
        }
    }

    if (map_changed)
    {
        // recompute mMappedFlags
        mMappedFlags = 0;
        for (auto& pair : mMaskToChannel)
        {
            mMappedFlags |= pair.first;
        }
        mPrevActiveFlags = 0;
        mCachedState.clear();
    }
    return map_changed;
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

bool LLGameControlTranslator::updateMapInternal(const std::string& name, const LLGameControl::InputChannel& channel)
{
    bool something_changed = false;
    ActionToMaskMap::iterator mask_itr = mActionToMask.find(name);
    if (mask_itr != mActionToMask.end())
    {
        U32 mask = mask_itr->second;
        something_changed = addOrRemoveMaskMapping(mask, channel);
    }
    return something_changed;
}

bool LLGameControlTranslator::addOrRemoveMaskMapping(U32 mask, const LLGameControl::InputChannel& channel)
{
    bool success = false;
    LLGameControlTranslator::MaskToChannelMap::iterator channel_itr = mMaskToChannel.find(mask);
    if (channel_itr != mMaskToChannel.end())
    {
        LLGameControl::InputChannel old_channel = channel_itr->second;
        if (old_channel.mType != channel.mType || old_channel.mIndex != channel.mIndex || old_channel.mSign != channel.mSign)
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
            success = true;
        }
    }
    else if (! channel.isNone())
    {
        // create new mapping
        mMaskToChannel[mask] = channel;
        success = true;
    }
    return success;
}


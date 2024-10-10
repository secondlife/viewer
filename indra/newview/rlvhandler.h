/**
 * @file rlvhandler.h
 * @author Kitty Barnett
 * @brief Primary command process and orchestration class
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

#include "llchat.h"
#include "llsingleton.h"

#include "rlvhelper.h"

class LLViewerObject;

// ============================================================================
// RlvHandler class
//

class RlvHandler : public LLSingleton<RlvHandler>
{
    template<Rlv::EParamType> friend struct Rlv::CommandHandlerBaseImpl;

    LLSINGLETON_EMPTY_CTOR(RlvHandler);

    /*
     * Command processing
     */
public:
    // Command processing helper functions
    bool         handleSimulatorChat(std::string& message, const LLChat& chat, const LLViewerObject* chatObj);
    Rlv::ECmdRet processCommand(const LLUUID& idObj, const std::string& stCmd, bool fromObj);
protected:
    Rlv::ECmdRet processCommand(std::reference_wrapper<const RlvCommand> rlvCmdRef, bool fromObj);

    /*
     * Helper functions
     */
public:
    // Initialization (deliberately static so they can safely be called in tight loops)
    static bool canEnable();
    static bool isEnabled() { return mIsEnabled; }
    static bool setEnabled(bool enable);

    /*
     * Event handling
     */
public:
    // The command output signal is triggered whenever a command produces channel or debug output
    using command_output_signal_t = boost::signals2::signal<void (const RlvCommand&, S32, const std::string&)>;
    boost::signals2::connection setCommandOutputCallback(const command_output_signal_t::slot_type& cb) { return mOnCommandOutput.connect(cb); }

protected:
    command_output_signal_t mOnCommandOutput;
private:
    static bool mIsEnabled;
};

// ============================================================================

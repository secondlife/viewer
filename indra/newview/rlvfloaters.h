/**
 * @file rlvfloaters.h
 * @author Kitty Barnett
 * @brief RLVa floaters class implementations
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

#include "llfloater.h"

#include "rlvdefines.h"

class LLChatEntry;
class LLFloaterReg;
class LLLayoutPanel;
class LLTextEditor;
class RlvCommand;
class RlvHandler;

namespace Rlv
{
    // ============================================================================
    // FloaterConsole - debug console to allow command execution without the need for a script
    //

    class FloaterConsole : public LLFloater
    {
        friend class ::LLFloaterReg;
        FloaterConsole(const LLSD& sdKey) : LLFloater(sdKey) {}

    public:
        bool postBuild() override;
        void onClose(bool fQuitting) override;
    protected:
        void onInput();
        void reshapeLayoutPanel();

    private:
        boost::signals2::scoped_connection mCommandOutputConn;
        int mInputEditPad = 0;
        LLLayoutPanel* mInputPanel = nullptr;
        LLChatEntry* mInputEdit = nullptr;
        LLTextEditor* mOutputText = nullptr;
    };

    // ============================================================================
};

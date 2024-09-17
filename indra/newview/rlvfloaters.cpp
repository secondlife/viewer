/**
 * @file rlvfloaters.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llagentdata.h"
#include "llchatentry.h"
#include "lltexteditor.h"
#include "lltrans.h"
#include "llvoavatarself.h"

#include "rlvfloaters.h"
#include "rlvhandler.h"

using namespace Rlv;

// ============================================================================
// FloaterConsole
//

bool FloaterConsole::postBuild()
{
    mInputEdit = getChild<LLChatEntry>("console_input");
    mInputEdit->setCommitCallback(std::bind(&FloaterConsole::onInput, this));
    mInputEdit->setTextExpandedCallback(std::bind(&FloaterConsole::reshapeLayoutPanel, this));
    mInputEdit->setFocus(true);
    mInputEdit->setCommitOnFocusLost(false);

    mInputPanel = getChild<LLLayoutPanel>("input_panel");
    mInputEditPad = mInputPanel->getRect().getHeight() - mInputEdit->getRect().getHeight();

    mOutputText = getChild<LLTextEditor>("console_output");
    mOutputText->appendText(Constants::ConsolePrompt, false);

    if (RlvHandler::isEnabled())
    {
        mCommandOutputConn = RlvHandler::instance().setCommandOutputCallback([this](const RlvCommand& rlvCmd, S32, const std::string strText)
        {
            if (rlvCmd.getObjectID() == gAgentID)
            {
                mOutputText->appendText(rlvCmd.getBehaviour() + ": ", true);
                mOutputText->appendText(strText, false);
            }
        });
    }

    return true;
}

void FloaterConsole::onClose(bool fQuitting)
{
    if (RlvHandler::isEnabled())
    {
        RlvHandler::instance().processCommand(gAgentID, "clear", true);
    }
}

void FloaterConsole::onInput()
{
    if (!isAgentAvatarValid())
    {
        return;
    }

    std::string strText = mInputEdit->getText();
    LLStringUtil::trim(strText);

    mOutputText->appendText(strText, false);
    mInputEdit->setText(LLStringUtil::null);

    if (!RlvHandler::isEnabled())
    {
        mOutputText->appendText(LLTrans::getString("RlvConsoleDisable"), true);
    }
    else if (strText.length() <= 3 || Constants::CmdPrefix != strText[0])
    {
        mOutputText->appendText(LLTrans::getString("RlvConsoleInvalidCmd"), true);
    }
    else
    {
        LLChat chat;
        chat.mFromID = gAgentID;
        chat.mChatType = CHAT_TYPE_OWNER;

        RlvHandler::instance().handleSimulatorChat(strText, chat, gAgentAvatarp);

        mOutputText->appendText(strText, true);
    }

    mOutputText->appendText(Constants::ConsolePrompt, true);
}

void FloaterConsole::reshapeLayoutPanel()
{
    mInputPanel->reshape(mInputPanel->getRect().getWidth(), mInputEdit->getRect().getHeight() + mInputEditPad, false);
}

// ============================================================================

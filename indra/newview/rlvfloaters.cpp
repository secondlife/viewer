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

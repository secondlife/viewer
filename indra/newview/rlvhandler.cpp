#include "llviewerprecompiledheaders.h"
#include "llappviewer.h"
#include "llstartup.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"

#include "rlvcommon.h"
#include "rlvhandler.h"
#include "rlvhelper.h"

using namespace Rlv;

// ============================================================================
// Static variable initialization
//

bool RlvHandler::mIsEnabled = false;

// ============================================================================
// Command processing functions
//

bool RlvHandler::handleSimulatorChat(std::string& message, const LLChat& chat, const LLViewerObject* chatObj)
{
    static LLCachedControl<bool> enable_temp_attach(gSavedSettings, Settings::EnableTempAttach);
    static LLCachedControl<bool> show_debug_output(gSavedSettings, Settings::Debug);
    static LLCachedControl<bool> hide_unset_dupes(gSavedSettings, Settings::DebugHideUnsetDup);

    if ( message.length() <= 3 || Rlv::Constants::CmdPrefix != message[0] || CHAT_TYPE_OWNER != chat.mChatType ||
         (chatObj && chatObj->isTempAttachment() && !enable_temp_attach()) )
    {
        return false;
    }

    message.erase(0, 1);
    LLStringUtil::toLower(message);
    CommandDbgOut cmdDbgOut(message);

    boost_tokenizer tokens(message, boost::char_separator<char>(",", "", boost::drop_empty_tokens));
    for (const std::string& strCmd : tokens)
    {
        ECmdRet eRet = (ECmdRet)processCommand(chat.mFromID, strCmd, true);
        if ( show_debug_output() &&
             (!hide_unset_dupes() || (ECmdRet::SuccessUnset != eRet && ECmdRet::SuccessDuplicate != eRet)) )
        {
            cmdDbgOut.add(strCmd, eRet);
        }
    }

    message = cmdDbgOut.get();
    return true;
}

ECmdRet RlvHandler::processCommand(const LLUUID& idObj, const std::string& strCmd, bool fromObj)
{
    return ECmdRet::FailedNoProcessor;
}

// ============================================================================
// Initialization helper functions
//

bool RlvHandler::canEnable()
{
    return LLStartUp::getStartupState() <= STATE_LOGIN_CLEANUP;
}

bool RlvHandler::setEnabled(bool enable)
{
    if (mIsEnabled == enable)
        return enable;

    if (enable && canEnable())
    {
        RLV_INFOS << "Enabling Restrained Love API support - " << RlvStrings::getVersionAbout() << RLV_ENDL;
        mIsEnabled = true;
    }

    return mIsEnabled;
}

// ============================================================================
